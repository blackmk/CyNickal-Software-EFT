#include "pch.h"
#include "GUI/Radar/Widgets/AimviewWidget.h"
#include "GUI/Radar/PlayerFocus.h"
#include "GUI/Color Picker/Color Picker.h"
#include "Game/EFT.h"
#include "Game/Classes/Players/CClientPlayer/CClientPlayer.h"
#include "Game/Classes/Players/CObservedPlayer/CObservedPlayer.h"
#include "Game/Classes/CRegisteredPlayers/CRegisteredPlayers.h"
#include "Game/Enums/EBoneIndex.h"
#include "DebugMode.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void AimviewWidget::Render()
{
	if (!EFT::IsInRaid() && !DebugMode::IsDebugMode())
	{
		ImGui::TextDisabled("Not in raid");
		return;
	}

	// Get widget drawing area
	ImVec2 contentMin = ImGui::GetCursorScreenPos();
	ImVec2 contentSize = ImGui::GetContentRegionAvail();

	// Make it square based on the smaller dimension
	float viewSize = std::min(contentSize.x, contentSize.y);
	if (viewSize < 50.0f)
		viewSize = 50.0f;

	ImVec2 center = ImVec2(contentMin.x + viewSize * 0.5f, contentMin.y + viewSize * 0.5f);
	float radius = viewSize * 0.45f;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Draw background circle
	drawList->AddCircleFilled(center, radius, IM_COL32(20, 20, 20, 200));
	drawList->AddCircle(center, radius, IM_COL32(100, 100, 100, 255), 0, 2.0f);

	// Draw distance circles if enabled
	if (ShowDistanceCircles)
	{
		DrawDistanceCircles(drawList, center, radius);
	}

	// Draw crosshair
	if (ShowCrosshair)
	{
		DrawCrosshair(drawList, center);
	}

	// Get local player data
	CClientPlayer* localPlayer = EFT::GetRegisteredPlayers().GetLocalPlayer();
	if (!localPlayer)
	{
		ImGui::SetCursorScreenPos(contentMin);
		ImGui::Dummy(ImVec2(viewSize, viewSize));
		return;
	}

	Vector3 localPos = localPlayer->GetBonePosition(EBoneIndex::Root);
	float localYaw = localPlayer->m_Yaw;
	float localPitch = 0.0f; // Would need pitch from camera/view direction

	// Draw players
	{
		auto& regPlayers = EFT::GetRegisteredPlayers();
		std::scoped_lock lock(regPlayers.m_Mut);

		for (const auto& playerVariant : regPlayers.m_Players)
		{
			std::visit([&](auto&& player)
			{
				using T = std::decay_t<decltype(player)>;

				if (player.IsInvalid())
					return;

				if (player.IsLocalPlayer())
					return;

				// Skip temp teammates
				if (PlayerFocus::IsTempTeammate(player.m_EntityAddress))
					return;

				ImVec2 aimviewPos;
				float distance;

				if (!WorldToAimview(player.GetBonePosition(EBoneIndex::Root), localPos, localYaw, localPitch, aimviewPos, distance))
					return; // Behind player

				if (distance > MaxDistance)
					return;

				// Scale position to widget
				ImVec2 scaledPos;
				scaledPos.x = center.x + aimviewPos.x * radius;
				scaledPos.y = center.y + aimviewPos.y * radius;

				// Clamp to circle
				ImVec2 offset = ImVec2(scaledPos.x - center.x, scaledPos.y - center.y);
				float offsetLen = sqrtf(offset.x * offset.x + offset.y * offset.y);
				if (offsetLen > radius)
				{
					offset.x = offset.x / offsetLen * radius;
					offset.y = offset.y / offsetLen * radius;
					scaledPos.x = center.x + offset.x;
					scaledPos.y = center.y + offset.y;
				}

				ImU32 color;
				if constexpr (std::is_same_v<T, CObservedPlayer>)
					color = GetPlayerColor(player);
				else
					color = GetPlayerColor(player);

				bool isFocused = PlayerFocus::FocusedPlayerAddr == player.m_EntityAddress;

				DrawPlayerDot(drawList, center, scaledPos, distance, color, isFocused);

			}, playerVariant);
		}
	}

	// Reserve space for the widget
	ImGui::SetCursorScreenPos(contentMin);
	ImGui::Dummy(ImVec2(viewSize, viewSize));
}

bool AimviewWidget::WorldToAimview(const Vector3& worldPos, const Vector3& localPos,
	float localYaw, float localPitch, ImVec2& outPos, float& outDistance)
{
	// Calculate relative position
	Vector3 delta;
	delta.x = worldPos.x - localPos.x;
	delta.y = worldPos.y - localPos.y;
	delta.z = worldPos.z - localPos.z;

	outDistance = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

	if (outDistance < 0.1f)
		return false;

	// Convert yaw to radians (EFT uses degrees, 0 = north, clockwise)
	float yawRad = localYaw * static_cast<float>(M_PI) / 180.0f;

	// Rotate delta by negative yaw to get relative to view direction
	float cosYaw = cosf(-yawRad);
	float sinYaw = sinf(-yawRad);

	float relX = delta.x * cosYaw - delta.z * sinYaw;
	float relZ = delta.x * sinYaw + delta.z * cosYaw;

	// relZ is now forward/back, relX is left/right
	// Positive relZ means in front of player

	if (relZ <= 0)
		return false; // Behind player

	// Project to 2D based on FOV
	float fovRad = FOV * static_cast<float>(M_PI) / 180.0f;
	float halfFov = fovRad / 2.0f;

	// Calculate horizontal angle
	float angleX = atan2f(relX, relZ);
	float angleY = atan2f(delta.y, relZ);

	// Normalize to [-1, 1] based on FOV
	outPos.x = angleX / halfFov;
	outPos.y = -angleY / halfFov; // Negative because screen Y is inverted

	return true;
}

void AimviewWidget::DrawPlayerDot(ImDrawList* drawList, const ImVec2& center, const ImVec2& pos,
	float distance, ImU32 color, bool isFocused)
{
	// Size based on distance (closer = bigger)
	float minSize = 3.0f;
	float maxSize = 10.0f;
	float t = 1.0f - (distance / MaxDistance);
	t = std::max(0.0f, std::min(1.0f, t));
	float dotSize = minSize + t * (maxSize - minSize);

	if (isFocused)
	{
		// Draw focus ring
		drawList->AddCircle(pos, dotSize + 4.0f, PlayerFocus::FocusHighlightColor, 0, 2.0f);
	}

	// Draw dot
	drawList->AddCircleFilled(pos, dotSize, color);
	drawList->AddCircle(pos, dotSize, IM_COL32(0, 0, 0, 200), 0, 1.0f);
}

void AimviewWidget::DrawCrosshair(ImDrawList* drawList, const ImVec2& center)
{
	ImU32 crosshairColor = IM_COL32(255, 255, 255, 200);

	// Horizontal line
	drawList->AddLine(
		ImVec2(center.x - CrosshairSize, center.y),
		ImVec2(center.x + CrosshairSize, center.y),
		crosshairColor, 1.0f);

	// Vertical line
	drawList->AddLine(
		ImVec2(center.x, center.y - CrosshairSize),
		ImVec2(center.x, center.y + CrosshairSize),
		crosshairColor, 1.0f);

	// Center dot
	drawList->AddCircleFilled(center, 2.0f, crosshairColor);
}

void AimviewWidget::DrawDistanceCircles(ImDrawList* drawList, const ImVec2& center, float radius)
{
	ImU32 circleColor = IM_COL32(60, 60, 60, 150);

	// Draw circles at 25%, 50%, 75%
	drawList->AddCircle(center, radius * 0.25f, circleColor);
	drawList->AddCircle(center, radius * 0.50f, circleColor);
	drawList->AddCircle(center, radius * 0.75f, circleColor);

	// Labels
	ImU32 textColor = IM_COL32(100, 100, 100, 200);
	char buf[32];

	snprintf(buf, sizeof(buf), "%.0fm", MaxDistance * 0.25f);
	drawList->AddText(ImVec2(center.x + 2, center.y - radius * 0.25f - 10), textColor, buf);

	snprintf(buf, sizeof(buf), "%.0fm", MaxDistance * 0.5f);
	drawList->AddText(ImVec2(center.x + 2, center.y - radius * 0.5f - 10), textColor, buf);

	snprintf(buf, sizeof(buf), "%.0fm", MaxDistance * 0.75f);
	drawList->AddText(ImVec2(center.x + 2, center.y - radius * 0.75f - 10), textColor, buf);
}

ImU32 AimviewWidget::GetPlayerColor(const CObservedPlayer& player)
{
	if (player.IsBoss())
		return static_cast<ImU32>(ColorPicker::Radar::m_BossColor);
	if (player.IsPMC())
		return static_cast<ImU32>(ColorPicker::Radar::m_PMCColor);
	if (player.IsPlayerScav())
		return static_cast<ImU32>(ColorPicker::Radar::m_PlayerScavColor);
	return static_cast<ImU32>(ColorPicker::Radar::m_ScavColor);
}

ImU32 AimviewWidget::GetPlayerColor(const CClientPlayer& player)
{
	if (player.IsBoss())
		return static_cast<ImU32>(ColorPicker::Radar::m_BossColor);
	if (player.IsPMC())
		return static_cast<ImU32>(ColorPicker::Radar::m_PMCColor);
	if (player.IsPlayerScav())
		return static_cast<ImU32>(ColorPicker::Radar::m_PlayerScavColor);
	return static_cast<ImU32>(ColorPicker::Radar::m_ScavColor);
}

json AimviewWidget::Serialize() const
{
	json j = RadarWidget::Serialize();
	j["FOV"] = FOV;
	j["MaxDistance"] = MaxDistance;
	j["ShowCrosshair"] = ShowCrosshair;
	j["ShowDistanceCircles"] = ShowDistanceCircles;
	j["CrosshairSize"] = CrosshairSize;
	return j;
}

void AimviewWidget::Deserialize(const json& j)
{
	RadarWidget::Deserialize(j);
	if (j.contains("FOV")) FOV = j["FOV"].get<float>();
	if (j.contains("MaxDistance")) MaxDistance = j["MaxDistance"].get<float>();
	if (j.contains("ShowCrosshair")) ShowCrosshair = j["ShowCrosshair"].get<bool>();
	if (j.contains("ShowDistanceCircles")) ShowDistanceCircles = j["ShowDistanceCircles"].get<bool>();
	if (j.contains("CrosshairSize")) CrosshairSize = j["CrosshairSize"].get<float>();
}
