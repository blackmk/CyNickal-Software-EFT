#include "pch.h"
#include "Aimbot.h"
#include "DMA/Input Manager.h"
#include "Game/Camera List/Camera List.h"
#include "GUI/Fuser/Fuser.h"
#include "GUI/Keybinds/Keybinds.h"
#include "Game/EFT.h"
#include "Makcu/MyMakcu.h"
#include "Input/KmboxNetController.h"
#include "GUI/KmboxSettings/KmboxSettings.h"
#include <limits>
#include <random>
#include <algorithm>
#include <type_traits>
#include "Game/Enums/EBoneIndex.h"
#include "Game/Classes/Players/CClientPlayer/CClientPlayer.h"
#include "Game/Classes/Players/CObservedPlayer/CObservedPlayer.h"
#include "Game/Ballistics/Ballistics.h"


// Forward declaration for fireport-based aim position
ImVec2 GetAimlineScreenPosition();

static std::mt19937& GetRandomEngine()
{
	static std::mt19937 rng{ std::random_device{}() };
	return rng;
}

void Aimbot::NormalizeRandomWeights()
{
	auto clampZero = [](float value)
	{
		return value < 0.0f ? 0.0f : value;
	};

	fRandomHeadChance = clampZero(fRandomHeadChance);
	fRandomNeckChance = clampZero(fRandomNeckChance);
	fRandomChestChance = clampZero(fRandomChestChance);
	fRandomTorsoChance = clampZero(fRandomTorsoChance);
	fRandomPelvisChance = clampZero(fRandomPelvisChance);

	float total = fRandomHeadChance + fRandomNeckChance + fRandomChestChance + fRandomTorsoChance + fRandomPelvisChance;

	if (total <= 0.001f)
	{
		fRandomHeadChance = 100.0f;
		fRandomNeckChance = 0.0f;
		fRandomChestChance = 0.0f;
		fRandomTorsoChance = 0.0f;
		fRandomPelvisChance = 0.0f;
		return;
	}

	float scale = 100.0f / total;

	fRandomHeadChance *= scale;
	fRandomNeckChance *= scale;
	fRandomChestChance *= scale;
	fRandomTorsoChance *= scale;
	fRandomPelvisChance *= scale;
}

static EBoneIndex RollRandomBone()
{
	Aimbot::NormalizeRandomWeights();

	const float weights[] = { Aimbot::fRandomHeadChance, Aimbot::fRandomNeckChance, Aimbot::fRandomChestChance, Aimbot::fRandomTorsoChance, Aimbot::fRandomPelvisChance };
	float total = weights[0] + weights[1] + weights[2] + weights[3] + weights[4];

	if (total <= 0.001f)
		return EBoneIndex::Head;

	std::uniform_real_distribution<float> dist(0.0f, total);
	float pick = dist(GetRandomEngine());

	float cumulative = weights[0];
	if (pick <= cumulative) return EBoneIndex::Head;

	cumulative += weights[1];
	if (pick <= cumulative) return EBoneIndex::Neck;

	cumulative += weights[2];
	if (pick <= cumulative) return EBoneIndex::Spine3;

	cumulative += weights[3];
	if (pick <= cumulative) return EBoneIndex::Spine2;

	return EBoneIndex::Pelvis;
}

static void UpdateRandomBoneState(bool bAimKeyActive)
{
	if (!bAimKeyActive || Aimbot::eTargetBone != ETargetBone::RandomBone)
	{
		Aimbot::bRandomBoneInitialized = false;
		Aimbot::s_LastRandomHandsController = 0;
		Aimbot::s_LastRandomAmmoCount = 0;
		return;
	}

	auto* pLocal = EFT::GetRegisteredPlayers().GetLocalPlayer();
	if (!pLocal || pLocal->IsInvalid())
	{
		// Full state reset to ensure clean re-initialization
		Aimbot::bRandomBoneInitialized = false;
		Aimbot::s_LastRandomHandsController = 0;
		Aimbot::s_LastRandomAmmoCount = 0;
		return;
	}

	auto* pFirearm = pLocal->GetFirearmManager();
	if (!pFirearm || !pFirearm->IsValid())
	{
		// Full state reset to ensure clean re-initialization
		Aimbot::bRandomBoneInitialized = false;
		Aimbot::s_LastRandomHandsController = 0;
		Aimbot::s_LastRandomAmmoCount = 0;
		return;
	}

	pFirearm->RefreshAmmoCount();

	uintptr_t handsAddr = pFirearm->GetHandsControllerAddress();
	uint32_t currentAmmo = pFirearm->GetCurrentAmmoCount();

	// Validate ammo count - ignore clearly invalid values (0 = read error, >200 = garbage)
	// This prevents spurious shot detection from corrupted memory reads
	constexpr uint32_t MAX_REALISTIC_MAG_SIZE = 200;
	if (currentAmmo == 0 || currentAmmo > MAX_REALISTIC_MAG_SIZE)
	{
		// Don't update ammo tracking with invalid values
		// Keep existing bone, skip shot detection this frame
		return;
	}

	if (handsAddr != Aimbot::s_LastRandomHandsController)
	{
		Aimbot::s_LastRandomHandsController = handsAddr;
		Aimbot::s_CurrentRandomBone = RollRandomBone();
		Aimbot::bRandomBoneInitialized = true;
		Aimbot::s_LastRandomAmmoCount = currentAmmo;
		return;
	}

	if (!Aimbot::bRandomBoneInitialized)
	{
		Aimbot::s_CurrentRandomBone = RollRandomBone();
		Aimbot::bRandomBoneInitialized = true;
		Aimbot::s_LastRandomAmmoCount = currentAmmo;
		return;
	}

	if (currentAmmo < Aimbot::s_LastRandomAmmoCount)
	{
		uint32_t shotsFired = Aimbot::s_LastRandomAmmoCount - currentAmmo;

		// Sanity check - max realistic shots per frame (prevents state corruption cascade)
		// If more than 10 shots detected, likely state corruption - just roll once
		constexpr uint32_t MAX_SHOTS_PER_FRAME = 10;
		if (shotsFired > MAX_SHOTS_PER_FRAME)
			shotsFired = 1;

		for (uint32_t i = 0; i < shotsFired; ++i)
		{
			Aimbot::s_CurrentRandomBone = RollRandomBone();
		}
	}

	Aimbot::s_LastRandomAmmoCount = currentAmmo;
}

static EBoneIndex GetTargetBoneIndex(uintptr_t targetAddr)
{
	switch (Aimbot::eTargetBone)
	{
	case ETargetBone::Head: return EBoneIndex::Head;
	case ETargetBone::Neck: return EBoneIndex::Neck;
	case ETargetBone::Chest: return EBoneIndex::Spine3;
	case ETargetBone::Torso: return EBoneIndex::Spine2;
	case ETargetBone::Pelvis: return EBoneIndex::Pelvis;
	case ETargetBone::RandomBone:
	{
		// Initialization is handled by UpdateRandomBoneState() - don't duplicate here
		// to avoid state desync (missing s_LastRandomAmmoCount update)
		return Aimbot::s_CurrentRandomBone;
	}
	default: return EBoneIndex::Head;
	}
}


template<typename T>
static bool ShouldTargetPlayer(const T& player)
{
	if (player.IsLocalPlayer()) return false;

	// Exclude dying/dead players (only CObservedPlayer has this status)
	if constexpr (std::is_same_v<T, CObservedPlayer>)
	{
		if (player.IsInCondition(ETagStatus::Dying))
			return false;
	}

	if (player.IsBoss()) return Aimbot::bTargetBoss;
	if (player.IsRaider()) return Aimbot::bTargetRaider;
	if (player.IsGuard()) return Aimbot::bTargetGuard;
	if (player.IsPlayerScav()) return Aimbot::bTargetPlayerScav;
	if (player.IsAi()) return Aimbot::bTargetAIScav;
	if (player.IsPMC()) return Aimbot::bTargetPMC;

	return false;
}

void Aimbot::RenderSettings()
{
	if (!bSettings) return;

	ImGui::SetNextWindowSize(ImVec2(300, 280), ImGuiCond_FirstUseEver);
	ImGui::Begin("Aimbot Settings", &bSettings);
	
	// Master toggle with status indicator
	if (bMasterToggle)
	{
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Status: ACTIVE");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Status: Disabled");
	}
	ImGui::Separator();
	ImGui::Spacing();
	
	ImGui::Checkbox("Enable Aimbot", &bMasterToggle);
	
	ImGui::Spacing();
	
	// FOV Settings Section
	if (ImGui::CollapsingHeader("FOV Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		
		ImGui::Checkbox("Draw FOV Circle", &bDrawFOV);
		
		ImGui::Spacing();
		
		ImGui::SetNextItemWidth(180.0f);
		ImGui::SliderFloat("FOV Radius", &fPixelFOV, 10.0f, 300.0f, "%.0f px");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Target acquisition radius in pixels");
		
		ImGui::SetNextItemWidth(180.0f);
		ImGui::SliderFloat("Deadzone", &fDeadzoneFov, 1.0f, 20.0f, "%.1f px");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("No aim adjustment within this radius");
		
		ImGui::Unindent();
	}
	
	// Smoothing Section
	if (ImGui::CollapsingHeader("Smoothing", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		
		ImGui::SetNextItemWidth(180.0f);
		ImGui::SliderFloat("Dampen Factor", &fDampen, 0.01f, 1.0f, "%.2f");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Lower = smoother but slower\nHigher = faster but more obvious");
		
		ImGui::Unindent();
	}

	// Aimline Section
	if (ImGui::CollapsingHeader("Aimline Settings"))
	{
		ImGui::Indent();
		
		ImGui::Checkbox("Draw Weapon Aimline", &bDrawAimline);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Draws a line from your weapon's fireport");
		
		if (bDrawAimline)
		{
			ImGui::SetNextItemWidth(180.0f);
			ImGui::SliderFloat("Aimline Length", &fAimlineLength, 1.0f, 1000.0f, "%.0f m");
			ImGui::ColorEdit4("Aimline Color", (float*)&aimlineColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
		}
		
		ImGui::Unindent();
	}

	// Experimental Fireport Aiming Section
	if (ImGui::CollapsingHeader("Experimental (Fireport Aiming)"))
	{
		ImGui::Indent();
		
		ImGui::TextWrapped("Compensates for weapon sway by aiming based on actual barrel direction instead of screen center.");
		ImGui::Spacing();
		
		ImGui::Checkbox("Use Fireport Aiming", &bUseFireportAiming);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Calculate aim delta from where the weapon is actually pointed,\nnot screen center. Compensates for weapon sway.");
		
		if (bUseFireportAiming)
		{
			ImGui::SetNextItemWidth(180.0f);
			ImGui::SliderFloat("Projection Distance", &fFireportProjectionDistance, 10.0f, 500.0f, "%.0f m");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Distance to project the aimline forward.\nHigher = more precise for distant targets.");
		}
		
		ImGui::Separator();
		ImGui::Text("Visualization:");
		ImGui::Spacing();
		
		ImGui::Checkbox("Draw Aim Point Dot", &bDrawAimPointDot);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Shows a dot where the weapon is actually pointing.\nMoves with weapon sway.");
		
		if (bDrawAimPointDot)
		{
			ImGui::SetNextItemWidth(180.0f);
			ImGui::SliderFloat("Dot Size", &fAimPointDotSize, 1.0f, 20.0f, "%.1f px");
			ImGui::ColorEdit4("Dot Color", (float*)&aimPointDotColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
		}
		
		ImGui::Spacing();
		ImGui::Checkbox("Debug Visualization", &bDrawDebugVisualization);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Shows both screen center and fireport aim point\nfor debugging/comparison.");
		
		ImGui::Unindent();
	}

	// Device Settings Section
	if (ImGui::CollapsingHeader("Device Settings"))
	{
		ImGui::Indent();
		
		// Device connection status
		bool bMakcuConnected = MyMakcu::m_Device.isConnected();
		bool bKmboxConnected = KmboxNet::KmboxNetController::IsConnected();
		
		if (bMakcuConnected)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Makcu: Connected");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Makcu: Disconnected");
		}
		
		if (bKmboxConnected)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "KMbox NET: Connected");
		}
		else if (KmboxSettings::bUseKmboxNet)
		{
			ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "KMbox NET: Disconnected");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "KMbox NET: Not Selected");
		}
		
		ImGui::Separator();
		ImGui::Spacing();
		
		// Device selection
		ImGui::Checkbox("Use KMbox NET (instead of Makcu)", &KmboxSettings::bUseKmboxNet);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Enable to use KMbox NET device instead of Makcu");
		
		ImGui::Spacing();
		
		// KMbox NET Settings (only show when enabled)
		if (KmboxSettings::bUseKmboxNet)
		{
			ImGui::Text("KMbox NET Configuration:");
			ImGui::Spacing();
			
			// IP Address input
			static char ipBuffer[64];
			static bool ipBufferInit = false;
			if (!ipBufferInit)
			{
				strncpy_s(ipBuffer, sizeof(ipBuffer), sKmboxNetIp.c_str(), _TRUNCATE);
				ipBufferInit = true;
			}
			ImGui::SetNextItemWidth(180.0f);
			if (ImGui::InputText("IP Address", ipBuffer, sizeof(ipBuffer)))
			{
				KmboxSettings::sKmboxNetIp = ipBuffer;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("KMbox NET device IP address (e.g., 192.168.2.4)");
			
			// Port input
			ImGui::SetNextItemWidth(180.0f);
			ImGui::InputInt("Port", &KmboxSettings::nKmboxNetPort);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("KMbox NET device port (default: 8888)");
			
			// MAC Address input
			static char macBuffer[64];
			static bool macBufferInit = false;
			if (!macBufferInit)
			{
				strncpy_s(macBuffer, sizeof(macBuffer), sKmboxNetMac.c_str(), _TRUNCATE);
				macBufferInit = true;
			}
			ImGui::SetNextItemWidth(180.0f);
			if (ImGui::InputText("MAC Address", macBuffer, sizeof(macBuffer)))
			{
				KmboxSettings::sKmboxNetMac = macBuffer;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("KMbox NET MAC address (hex, e.g., AABBCCDD)");
			
			ImGui::Spacing();
			
			// Connect/Disconnect buttons
			if (!bKmboxConnected)
			{
				if (ImGui::Button("Connect KMbox NET"))
				{
					std::println("[Aimbot] Attempting to connect to KMbox NET at {}:{}", KmboxSettings::sKmboxNetIp, KmboxSettings::nKmboxNetPort);
					if (KmboxNet::KmboxNetController::Connect(KmboxSettings::sKmboxNetIp, KmboxSettings::nKmboxNetPort, KmboxSettings::sKmboxNetMac))
					{
						std::println("[Aimbot] KMbox NET connected successfully!");
					}
					else
					{
						std::println("[Aimbot] KMbox NET connection failed!");
					}
				}
			}
			else
			{
				if (ImGui::Button("Disconnect KMbox NET"))
				{
					KmboxNet::KmboxNetController::Disconnect();
					std::println("[Aimbot] KMbox NET disconnected");
				}
			}
		}
		
		ImGui::Unindent();
	}

	ImGui::End();
}

void Aimbot::RenderFOVCircle(const ImVec2& WindowPos, ImDrawList* DrawList)
{
	if (!bMasterToggle)
		return;

	bool bHasVisual = bDrawFOV || bDrawAimline || bDrawAimPointDot || bDrawDebugVisualization || (bEnablePrediction && bShowTrajectory);
	if (!bHasVisual)
		return;

	bool bAimKeyActive = Keybinds::Aimbot.GetState();

	if (bDrawFOV)
	{
		auto WindowSize = ImGui::GetWindowSize();
		auto Center = ImVec2(WindowPos.x + WindowSize.x / 2.0f, WindowPos.y + WindowSize.y / 2.0f);

		ImU32 Color = bAimKeyActive ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 255, 255);

		DrawList->AddCircle(Center, fPixelFOV, Color, 100, 2.0f);
		DrawList->AddCircle(Center, fDeadzoneFov, IM_COL32(255, 0, 0, 255), 100, 2.0f);
	}

	// Calculate draw conditions early - aimline/trajectory can show without aim key if bOnlyOnAim is off
	bool isScoped = CameraList::IsScoped();
	bool shouldDrawVisuals = !bOnlyOnAim || isScoped || bAimKeyActive;

	bool bNeedsAimData = bDrawAimPointDot || bDrawDebugVisualization || bDrawAimline || (bEnablePrediction && bShowTrajectory);
	if (!bNeedsAimData)
		return;

	ImVec2 AimlineScreenPos = GetAimlineScreenPosition();

	// Aim point dot - requires aim key active
	if (bDrawAimPointDot && bAimKeyActive)
	{
		auto WindowSize = ImGui::GetWindowSize();
		ImVec2 dotPos = ImVec2(WindowPos.x + AimlineScreenPos.x, WindowPos.y + AimlineScreenPos.y);

		DrawList->AddCircleFilled(dotPos, fAimPointDotSize, (ImU32)aimPointDotColor);
	}

	// Debug visualization - requires aim key active
	if (bDrawDebugVisualization && bAimKeyActive)
	{
		auto WindowSize = ImGui::GetWindowSize();
		ImVec2 screenCenter = ImVec2(WindowPos.x + WindowSize.x / 2.0f, WindowPos.y + WindowSize.y / 2.0f);
		ImVec2 fireportPos = ImVec2(WindowPos.x + AimlineScreenPos.x, WindowPos.y + AimlineScreenPos.y);

		float crossSize = 8.0f;
		DrawList->AddLine(
			ImVec2(screenCenter.x - crossSize, screenCenter.y),
			ImVec2(screenCenter.x + crossSize, screenCenter.y),
			IM_COL32(255, 255, 255, 200), 2.0f);
		DrawList->AddLine(
			ImVec2(screenCenter.x, screenCenter.y - crossSize),
			ImVec2(screenCenter.x, screenCenter.y + crossSize),
			IM_COL32(255, 255, 255, 200), 2.0f);

		float diamondSize = 6.0f;
		DrawList->AddQuadFilled(
			ImVec2(fireportPos.x, fireportPos.y - diamondSize),
			ImVec2(fireportPos.x + diamondSize, fireportPos.y),
			ImVec2(fireportPos.x, fireportPos.y + diamondSize),
			ImVec2(fireportPos.x - diamondSize, fireportPos.y),
			IM_COL32(0, 255, 255, 200));

		DrawList->AddLine(screenCenter, fireportPos, IM_COL32(255, 255, 0, 150), 1.0f);
	}

	// Trajectory - uses shouldDrawVisuals (shows without aim key when bOnlyOnAim is off)
	if (bEnablePrediction && bShowTrajectory && shouldDrawVisuals)
	{
		auto* pLocalPlayer = EFT::GetRegisteredPlayers().GetLocalPlayer();
		if (pLocalPlayer && !pLocalPlayer->IsInvalid())
		{
			auto* pFirearm = pLocalPlayer->GetFirearmManager();
			if (pFirearm && pFirearm->IsValid())
			{
				auto ballistics = pFirearm->GetBallistics();
				if (ballistics.IsAmmoValid())
				{
					auto points = BallisticsSimulation::SimulateTrajectory(ballistics, 1.5f);

					if (!points.empty())
					{
						Vector3 fireport = pFirearm->GetFireportPosition();
						Vector3 dir = pFirearm->GetFireportRotation().Down();

						std::vector<ImVec2> screenPoints;
						screenPoints.reserve(points.size());

						for (const auto& pt : points)
						{
							Vector3 worldPt = fireport + (dir * pt.z) + Vector3{ 0.f, pt.y, 0.f };
							Vector2 screen;
							if (CameraList::W2S(worldPt, screen))
							{
								screenPoints.push_back(ImVec2(WindowPos.x + screen.x, WindowPos.y + screen.y));
							}
						}

						if (screenPoints.size() > 1)
						{
							DrawList->AddPolyline(screenPoints.data(), (int)screenPoints.size(), IM_COL32(0, 255, 255, 180), false, 2.0f);
						}
					}
				}
			}
		}
	}

	// Aimline - uses shouldDrawVisuals (shows without aim key when bOnlyOnAim is off)
	if (bDrawAimline && shouldDrawVisuals)
	{
		auto* pLocalPlayer = EFT::GetRegisteredPlayers().GetLocalPlayer();
		if (!pLocalPlayer || pLocalPlayer->IsInvalid())
			return;

		auto* pFirearmManager = pLocalPlayer->GetFirearmManager();
		if (!pFirearmManager || !pFirearmManager->IsValid())
			return;

		Vector3 fireportPos = pFirearmManager->GetFireportPosition();
		Quaternion fireportRot = pFirearmManager->GetFireportRotation();

		if (fireportPos.x == 0.0f && fireportPos.y == 0.0f && fireportPos.z == 0.0f)
			return;

		Vector3 shootDir = fireportRot.Down();

		if (bEnablePrediction && bShowTrajectory)
		{
			auto ballistics = pFirearmManager->GetBallistics();
			if (ballistics.IsAmmoValid())
			{
				float targetDistance = 0.0f;
				{
					auto& PlayerList = EFT::GetRegisteredPlayers();
					std::scoped_lock lk(PlayerList.m_Mut);
					ImVec2 AimOrigin = bUseFireportAiming ? AimlineScreenPos : Fuser::GetCenterScreen();
					float bestScore = std::numeric_limits<float>::max();

					for (auto& Player : PlayerList.m_Players)
					{
						std::visit([&](auto& Player) {
							if (!ShouldTargetPlayer(Player)) return;
							EBoneIndex bone = GetTargetBoneIndex(Player.m_EntityAddress);
							Vector3 bonePos = Player.GetBonePosition(bone);
							Vector2 screenPos{};
							if (!CameraList::W2S(bonePos, screenPos)) return;
							Vector2 screenOrigin{ AimOrigin.x, AimOrigin.y };
							float distanceFromOrigin = screenPos.DistanceTo(screenOrigin);
							if (distanceFromOrigin > fPixelFOV) return;
							float distToTarget = fireportPos.DistanceTo(bonePos);
							if (distToTarget <= 0.1f) return;
							if (distanceFromOrigin < bestScore)
							{
								bestScore = distanceFromOrigin;
								targetDistance = distToTarget;
							}
						}, Player);
					}
				}


				if (targetDistance > 0.0f)
				{
					Vector2 screenStart{};
					if (!CameraList::W2S(fireportPos, screenStart))
						return;

					float muzzleVel = ballistics.BulletSpeed > 1.0f ? ballistics.BulletSpeed : 1.0f;
					float timeBudget = (targetDistance / muzzleVel) + 0.2f;
					if (timeBudget < 0.1f) timeBudget = 0.1f;
					if (timeBudget > 2.5f) timeBudget = 2.5f;

					auto points = BallisticsSimulation::SimulateTrajectory(ballistics, timeBudget);
					if (!points.empty())
					{
						std::vector<ImVec2> screenPoints;
						screenPoints.reserve(points.size() + 1);
						screenPoints.push_back(ImVec2(WindowPos.x + screenStart.x, WindowPos.y + screenStart.y));

						for (const auto& pt : points)
						{
							if (pt.z > targetDistance) break;
							Vector3 worldPt = fireportPos + (shootDir * pt.z) + Vector3{ 0.f, pt.y, 0.f };
							Vector2 screenPt{};
							if (CameraList::W2S(worldPt, screenPt))
							{
								screenPoints.push_back(ImVec2(WindowPos.x + screenPt.x, WindowPos.y + screenPt.y));
							}
						}

						if (screenPoints.size() > 2)
						{
							DrawList->AddPolyline(screenPoints.data(), (int)screenPoints.size(), (ImU32)aimlineColor, false, fAimlineThickness);
							return;
						}
					}
				}
			}
		}

		Vector3 endPos = {
			fireportPos.x + shootDir.x * fAimlineLength,
			fireportPos.y + shootDir.y * fAimlineLength,
			fireportPos.z + shootDir.z * fAimlineLength
		};

		Vector2 screenStart, screenEnd;

		if (!CameraList::W2S(fireportPos, screenStart))
			return;

		if (CameraList::W2S(endPos, screenEnd))
		{
			DrawList->AddLine(
				ImVec2(WindowPos.x + screenStart.x, WindowPos.y + screenStart.y),
				ImVec2(WindowPos.x + screenEnd.x, WindowPos.y + screenEnd.y),
				(ImU32)aimlineColor, fAimlineThickness
			);
		}
		else
		{
			Vector3 nearEnd = {
				fireportPos.x + shootDir.x * 5.0f,
				fireportPos.y + shootDir.y * 5.0f,
				fireportPos.z + shootDir.z * 5.0f
			};
			Vector2 screenNearEnd;
			if (CameraList::W2S(nearEnd, screenNearEnd))
			{
				float dx = screenNearEnd.x - screenStart.x;
				float dy = screenNearEnd.y - screenStart.y;
				float len = sqrtf(dx * dx + dy * dy);
				if (len > 0.5f)
				{
					float scale = 2000.0f / len;
					ImVec2 extendedEnd = { screenStart.x + dx * scale, screenStart.y + dy * scale };

					DrawList->AddLine(
						ImVec2(WindowPos.x + screenStart.x, WindowPos.y + screenStart.y),
						ImVec2(WindowPos.x + extendedEnd.x, WindowPos.y + extendedEnd.y),
						(ImU32)aimlineColor, fAimlineThickness
					);
				}
			}
		}
	}
}

// Returns screen position where the fireport is actually aiming
// Falls back to screen center if fireport data is unavailable
ImVec2 GetAimlineScreenPosition()
{

	auto CenterScreen = Fuser::GetCenterScreen();

	auto* pLocalPlayer = EFT::GetRegisteredPlayers().GetLocalPlayer();
	if (!pLocalPlayer || pLocalPlayer->IsInvalid())
		return CenterScreen;

	auto* pFirearmManager = pLocalPlayer->GetFirearmManager();
	if (!pFirearmManager || !pFirearmManager->IsValid())
		return CenterScreen;

	Vector3 fireportPos = pFirearmManager->GetFireportPosition();
	Quaternion fireportRot = pFirearmManager->GetFireportRotation();

	// Validate position isn't zero
	if (fireportPos.x == 0.0f && fireportPos.y == 0.0f && fireportPos.z == 0.0f)
		return CenterScreen;

	// Project forward along aim direction
	Vector3 shootDir = fireportRot.Down();  // EFT uses -Y as forward for weapons
	Vector3 aimPoint = {
		fireportPos.x + shootDir.x * Aimbot::fFireportProjectionDistance,
		fireportPos.y + shootDir.y * Aimbot::fFireportProjectionDistance,
		fireportPos.z + shootDir.z * Aimbot::fFireportProjectionDistance
	};

	// Convert to screen coordinates
	Vector2 screenAimPoint;
	if (!CameraList::W2S(aimPoint, screenAimPoint))
		return CenterScreen;  // Fallback if projection fails

	return ImVec2(screenAimPoint.x, screenAimPoint.y);
}

ImVec2 Subtract(const ImVec2& lhs, const ImVec2& rhs)
{
	return { lhs.x - rhs.x, lhs.y - rhs.y };
}
ImVec2 Subtract(const Vector2& lhs, const ImVec2& rhs)
{
	return { lhs.x - rhs.x, lhs.y - rhs.y };
}
float Distance(Vector2 a, ImVec2 b)
{
	return sqrtf(powf(b.x - a.x, 2) + powf(b.y - a.y, 2));
}
float Distance(ImVec2 a, ImVec2 b)
{
	return sqrtf(powf(b.x - a.x, 2) + powf(b.y - a.y, 2));
}
void Aimbot::SendDeviceMove(int dx, int dy)
{
	// Route to appropriate device based on settings
	if (KmboxSettings::bUseKmboxNet && KmboxNet::KmboxNetController::IsConnected())
	{
		KmboxNet::KmboxNetController::Move(dx, dy);
		return;
	}

	// Fall back to Makcu
	MyMakcu::m_Device.mouseMove(static_cast<float>(dx), static_cast<float>(dy));
}

void Aimbot::OnDMAFrame(DMA_Connection* Conn)
{
	bool bKeyActive = Keybinds::Aimbot.IsActive(Conn, true);
	if (!bKeyActive)
		return;

	if (!bMasterToggle)
		return;

	bool bDeviceConnected = MyMakcu::m_Device.isConnected() || KmboxNet::KmboxNetController::IsConnected();
	if (!c_keys::IsInitialized() || !bDeviceConnected)
		return;

	auto& RegisteredPlayers = EFT::GetRegisteredPlayers();

	RegisteredPlayers.QuickUpdate(Conn);
	CameraList::QuickUpdateNecessaryCameras(Conn);
	UpdateRandomBoneState(bKeyActive);

	ImVec2 AimOrigin = bUseFireportAiming ? GetAimlineScreenPosition() : Fuser::GetCenterScreen();

	auto BestTarget = Aimbot::FindBestTarget(AimOrigin);
	if (BestTarget == 0)
		return;

	auto Delta = GetAimDeltaToTarget(BestTarget, AimOrigin);
	static ImVec2 PreviousDelta{};

	if (Distance(Delta, PreviousDelta) < 0.5f)
		return;

	PreviousDelta = Delta;
	Delta.x *= fDampen;
	Delta.y *= fDampen;

	SendDeviceMove(static_cast<int>(Delta.x), static_cast<int>(Delta.y));
}


ImVec2 Aimbot::GetAimDeltaToTarget(uintptr_t TargetAddress, const ImVec2& AimOrigin)
{
	ImVec2 Return{};

	if (TargetAddress == 0x0) return Return;

	EBoneIndex bone = GetTargetBoneIndex(TargetAddress);
	auto TargetWorldPos = EFT::GetRegisteredPlayers().GetPlayerBonePosition(TargetAddress, bone);

	// Ballistics Prediction
	if (bEnablePrediction)
	{
		auto* pLocalPlayer = EFT::GetRegisteredPlayers().GetLocalPlayer();
		if (pLocalPlayer && !pLocalPlayer->IsInvalid())
		{
			auto* pFirearm = pLocalPlayer->GetFirearmManager();
			if (pFirearm && pFirearm->IsValid())
			{
				Vector3 StartPos = pFirearm->GetFireportPosition();
				
				BallisticsInfo info = pFirearm->GetBallistics();
				
				if (info.IsAmmoValid())
				{
					BallisticSimulationOutput sim = BallisticsSimulation::Run(StartPos, TargetWorldPos, info);
					
					TargetWorldPos.y += sim.BulletDrop;
				}
			}
		}
	}

	Vector2 ScreenPos{};
	if (!CameraList::W2S(TargetWorldPos, ScreenPos)) return Return;

	float DistanceFromOrigin = Distance(ScreenPos, AimOrigin);

	if (DistanceFromOrigin < fDeadzoneFov) return Return;

	if (DistanceFromOrigin > fPixelFOV) return Return;

	Return = Subtract(ScreenPos, AimOrigin);

	return Return;
}

uintptr_t Aimbot::FindBestTarget(const ImVec2& AimOrigin)
{
	auto& PlayerList = EFT::GetRegisteredPlayers();

	std::scoped_lock lk(PlayerList.m_Mut);

	uintptr_t BestTarget = 0;
	float BestDistance = std::numeric_limits<float>::max();

	for (auto& Player : PlayerList.m_Players)
	{
		std::visit([&](auto& Player) {
			if (!ShouldTargetPlayer(Player)) return;

			EBoneIndex bone = GetTargetBoneIndex(Player.m_EntityAddress);
			Vector3 bonePos = Player.GetBonePosition(bone);

			Vector2 ScreenPos{};
			if (!CameraList::W2S(bonePos, ScreenPos)) return;

			float DistanceFromOrigin = sqrt(pow(ScreenPos.x - AimOrigin.x, 2) + pow(ScreenPos.y - AimOrigin.y, 2));

			if (DistanceFromOrigin > fPixelFOV) return;

			if (fMaxDistance > 0.0f)
			{
				// Future: add world-distance limit
			}

			if (DistanceFromOrigin < BestDistance)
			{
				BestTarget = Player.m_EntityAddress;
				BestDistance = DistanceFromOrigin;
			}

			}, Player);
	}

	return BestTarget;
}


