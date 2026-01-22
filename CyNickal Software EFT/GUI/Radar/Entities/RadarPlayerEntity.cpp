#include "pch.h"
#include "GUI/Radar/Entities/RadarPlayerEntity.h"
#include "GUI/Radar/CoordTransform.h"
#include "GUI/Color Picker/Color Picker.h"
#include <cmath>

const CBaseEFTPlayer* RadarPlayerEntity::GetPlayerPtr() const
{
	return std::visit([](auto&& p) -> const CBaseEFTPlayer* {
		if constexpr (std::is_pointer_v<std::decay_t<decltype(p)>>)
			return p;
		else
			return nullptr;
	}, m_player);
}

Vector3 RadarPlayerEntity::GetPosition() const
{
	return std::visit([](auto&& p) -> Vector3 {
		if constexpr (std::is_pointer_v<std::decay_t<decltype(p)>>)
		{
			if (p)
				return p->GetBonePosition(EBoneIndex::Root);
		}
		return Vector3{};
	}, m_player);
}

ImU32 RadarPlayerEntity::GetColor() const
{
	if (IsLocalPlayer())
		return ColorPicker::Radar::m_LocalPlayerColor;

	return std::visit([](auto&& p) -> ImU32 {
		if constexpr (std::is_pointer_v<std::decay_t<decltype(p)>>)
		{
			if (p)
				return p->GetRadarColor();
		}
		return IM_COL32(255, 255, 255, 255);
	}, m_player);
}

bool RadarPlayerEntity::IsValid() const
{
	return std::visit([](auto&& p) -> bool {
		if constexpr (std::is_pointer_v<std::decay_t<decltype(p)>>)
		{
			if (p)
				return !p->IsInvalid();
		}
		return false;
	}, m_player);
}

void RadarPlayerEntity::Draw(ImDrawList* drawList,
                            const MapParams& params,
                            const ImVec2& canvasMin,
                            const ImVec2& canvasMax,
                            const Vector3& localPlayerPos) const
{
	if (!IsValid() || !params.config)
		return;

	Vector3 playerPos = GetPosition();

	// Transform to screen position
	ImVec2 screenPos = CoordTransform::WorldToScreen(playerPos, *params.config, params);
	screenPos.x += canvasMin.x;
	screenPos.y += canvasMin.y;

	// Check bounds
	if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
	    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
		return;

	// Get color and size
	bool isLocal = IsLocalPlayer();
	ImU32 playerColor = GetColor();
	float iconSize = isLocal ? 8.0f * 1.5f : 8.0f;  // Hardcoded for now, TODO: use settings

	// Draw player circle
	drawList->AddCircleFilled(screenPos, iconSize, playerColor);
	drawList->AddCircle(screenPos, iconSize, IM_COL32(0, 0, 0, 200), 0, 1.0f); // Outline

	// Draw view direction line
	float yaw = GetYaw();
	constexpr float degToRad = 0.01745329f;
	float rayLen = 25.0f;
	ImVec2 viewEnd = ImVec2(
		screenPos.x + std::sin(yaw * degToRad) * rayLen,
		screenPos.y - std::cos(yaw * degToRad) * rayLen
	);
	drawList->AddLine(screenPos, viewEnd, IM_COL32(0, 0, 0, 200), 3.0f); // Outline
	drawList->AddLine(screenPos, viewEnd, playerColor, 2.0f);

	// Skip labels for local player
	if (isLocal)
		return;

	// Height indicator (▲X or ▼X)
	float heightDiff = playerPos.y - localPlayerPos.y;
	int roundedHeight = (int)std::round(heightDiff);
	if (roundedHeight != 0)
	{
		char heightBuf[16];
		if (roundedHeight > 0)
			snprintf(heightBuf, sizeof(heightBuf), "▲%d", roundedHeight);
		else
			snprintf(heightBuf, sizeof(heightBuf), "▼%d", std::abs(roundedHeight));

		ImVec2 heightPos = ImVec2(screenPos.x - iconSize - 4, screenPos.y + 4);
		drawList->AddText(heightPos, IM_COL32(255, 255, 255, 220), heightBuf);
	}

	// Distance - on right side
	float dist = GetDistanceTo(localPlayerPos);
	int roundedDist = (int)std::round(dist);
	char distBuf[16];
	snprintf(distBuf, sizeof(distBuf), "%dm", roundedDist);
	ImVec2 distPos = ImVec2(screenPos.x + iconSize + 4, screenPos.y + 4);
	drawList->AddText(distPos, IM_COL32(255, 255, 255, 200), distBuf);
}

bool RadarPlayerEntity::ShouldRender(float maxDistance, const Vector3& localPlayerPos) const
{
	if (!IsValid())
		return false;

	float dist = GetDistanceTo(localPlayerPos);
	return dist <= maxDistance;
}

float RadarPlayerEntity::GetDistanceTo(const Vector3& pos) const
{
	Vector3 playerPos = GetPosition();
	return playerPos.DistanceTo(pos);
}

float RadarPlayerEntity::GetYaw() const
{
	return std::visit([](auto&& p) -> float {
		if constexpr (std::is_pointer_v<std::decay_t<decltype(p)>>)
		{
			if (p)
				return p->m_Yaw;
		}
		return 0.0f;
	}, m_player);
}

bool RadarPlayerEntity::IsLocalPlayer() const
{
	return std::visit([](auto&& p) -> bool {
		if constexpr (std::is_pointer_v<std::decay_t<decltype(p)>>)
		{
			using PlayerType = std::decay_t<std::remove_pointer_t<decltype(p)>>;
			if constexpr (std::is_same_v<PlayerType, CClientPlayer>)
			{
				if (p)
					return p->IsLocalPlayer();
			}
		}
		return false;
	}, m_player);
}

std::string RadarPlayerEntity::GetName() const
{
	return std::visit([](auto&& p) -> std::string {
		if constexpr (std::is_pointer_v<std::decay_t<decltype(p)>>)
		{
			if (p)
				return p->GetBaseName();
		}
		return "";
	}, m_player);
}

float RadarPlayerEntity::GetHealth() const
{
	// TODO: Health not currently exposed in player classes
	return 0.0f;
}
