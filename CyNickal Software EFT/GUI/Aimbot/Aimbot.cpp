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
	if (!bMasterToggle || !bDrawFOV) return;

	auto WindowSize = ImGui::GetWindowSize();
	auto Center = ImVec2(WindowPos.x + WindowSize.x / 2.0f, WindowPos.y + WindowSize.y / 2.0f);
	DrawList->AddCircle(Center, fPixelFOV, IM_COL32(255, 255, 255, 255), 100, 2.0f);
	DrawList->AddCircle(Center, fDeadzoneFov, IM_COL32(255, 0, 0, 255), 100, 2.0f);
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
	if (!bMasterToggle) return;

	// Check if any device is connected
	bool bDeviceConnected = MyMakcu::m_Device.isConnected() || KmboxNet::KmboxNetController::IsConnected();
	if (c_keys::IsInitialized() == false || bDeviceConnected == false) return;

	if (Keybinds::Aimbot.IsActive(Conn) == false) return;

	auto BestTarget = Aimbot::FindBestTarget();
	auto& RegisteredPlayers = EFT::GetRegisteredPlayers();

	do
	{
		RegisteredPlayers.QuickUpdate(Conn);
		CameraList::QuickUpdateNecessaryCameras(Conn);

		auto Delta = GetAimDeltaToTarget(BestTarget);
		static ImVec2 PreviousDelta{};

		if (Distance(Delta, PreviousDelta) < 0.5f)
			continue;

		PreviousDelta = Delta;
		Delta.x *= fDampen;
		Delta.y *= fDampen;

		SendDeviceMove(static_cast<int>(Delta.x), static_cast<int>(Delta.y));

	} while (Keybinds::Aimbot.IsActive(Conn));

}

ImVec2 Aimbot::GetAimDeltaToTarget(uintptr_t TargetAddress)
{
	ImVec2 Return{};

	if (TargetAddress == 0x0) return Return;

	auto CenterScreen = Fuser::GetCenterScreen();

	auto TargetWorldPos = EFT::GetRegisteredPlayers().GetPlayerBonePosition(TargetAddress, EBoneIndex::Head);

	Vector2 ScreenPos{};
	if (!CameraList::W2S(TargetWorldPos, ScreenPos)) return Return;

	float DistanceFromCenter = Distance(ScreenPos, CenterScreen);

	if (DistanceFromCenter < fDeadzoneFov) return Return;

	if (DistanceFromCenter > fPixelFOV) return Return;

	Return = Subtract(ScreenPos, CenterScreen);

	return Return;
}

uintptr_t Aimbot::FindBestTarget()
{
	auto& PlayerList = EFT::GetRegisteredPlayers();

	std::scoped_lock lk(PlayerList.m_Mut);

	auto Center = Fuser::GetCenterScreen();
	uintptr_t BestTarget = 0;
	float BestDistance = std::numeric_limits<float>::max();

	for (auto& Player : PlayerList.m_Players)
	{
		std::visit([&](auto& Player) {

			Vector2 ScreenPos{};
			if (!CameraList::W2S(Player.GetBonePosition(EBoneIndex::Head), ScreenPos)) return;

			float DistanceFromCenter = sqrt(pow(ScreenPos.x - Center.x, 2) + pow(ScreenPos.y - Center.y, 2));

			if (DistanceFromCenter > fPixelFOV) return;

			if (DistanceFromCenter < BestDistance)
			{
				BestTarget = Player.m_EntityAddress;
				BestDistance = DistanceFromCenter;
			}

			}, Player);
	}

	return BestTarget;
}