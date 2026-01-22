#include "pch.h"
#include "GUI/Radar/Entities/RadarLootEntity.h"
#include "GUI/Radar/CoordTransform.h"
#include "GUI/LootFilter/LootFilter.h"
#include <cmath>

Vector3 RadarLootEntity::GetPosition() const
{
	if (m_loot)
		return m_loot->m_Position;
	return Vector3{};
}

ImU32 RadarLootEntity::GetColor() const
{
	if (m_loot)
		return LootFilter::GetItemColor(*m_loot);
	return IM_COL32(255, 255, 255, 255);
}

bool RadarLootEntity::IsValid() const
{
	return m_loot && !m_loot->IsInvalid();
}

void RadarLootEntity::Draw(ImDrawList* drawList,
                          const MapParams& params,
                          const ImVec2& canvasMin,
                          const ImVec2& canvasMax,
                          const Vector3& localPlayerPos) const
{
	if (!IsValid() || !params.config)
		return;

	// Check filter
	if (!LootFilter::ShouldShow(*m_loot))
		return;

	Vector3 itemPos = GetPosition();

	// Transform to screen position
	ImVec2 screenPos = CoordTransform::WorldToScreen(itemPos, *params.config, params);
	screenPos.x += canvasMin.x;
	screenPos.y += canvasMin.y;

	// Check bounds
	if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
	    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
		return;

	// Get color
	ImU32 lootColor = GetColor();
	float iconSize = 4.0f;  // Hardcoded for now, TODO: use settings

	// Draw outlined square
	drawList->AddRectFilled(
		ImVec2(screenPos.x - iconSize, screenPos.y - iconSize),
		ImVec2(screenPos.x + iconSize, screenPos.y + iconSize),
		lootColor
	);
	drawList->AddRect(
		ImVec2(screenPos.x - iconSize, screenPos.y - iconSize),
		ImVec2(screenPos.x + iconSize, screenPos.y + iconSize),
		IM_COL32(0, 0, 0, 180), 0.0f, 0, 1.0f
	);

	// Draw height indicator if significant difference
	float heightDiff = itemPos.y - localPlayerPos.y;
	int roundedHeight = (int)std::round(heightDiff);

	if (std::abs(roundedHeight) > 2)
	{
		char heightBuf[8];
		if (roundedHeight > 0)
			snprintf(heightBuf, sizeof(heightBuf), "▲");
		else
			snprintf(heightBuf, sizeof(heightBuf), "▼");

		ImVec2 heightPos = ImVec2(screenPos.x - iconSize - 2, screenPos.y + 3);
		drawList->AddText(heightPos, IM_COL32(255, 255, 255, 200), heightBuf);
	}
}

bool RadarLootEntity::ShouldRender(float maxDistance, const Vector3& localPlayerPos) const
{
	if (!IsValid())
		return false;

	if (!LootFilter::ShouldShow(*m_loot))
		return false;

	float dist = GetDistanceTo(localPlayerPos);
	return dist <= maxDistance;
}

float RadarLootEntity::GetDistanceTo(const Vector3& pos) const
{
	Vector3 itemPos = GetPosition();
	return itemPos.DistanceTo(pos);
}
