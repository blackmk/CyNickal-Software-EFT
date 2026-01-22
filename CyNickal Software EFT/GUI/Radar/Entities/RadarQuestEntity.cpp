#include "pch.h"
#include "GUI/Radar/Entities/RadarQuestEntity.h"
#include "GUI/Radar/CoordTransform.h"
#include <cmath>

Vector3 RadarQuestEntity::GetPosition() const
{
	return m_quest.Position;
}

ImU32 RadarQuestEntity::GetColor() const
{
	return m_color;
}

bool RadarQuestEntity::IsValid() const
{
	return true; // Quest locations are always valid
}

void RadarQuestEntity::Draw(ImDrawList* drawList,
                           const MapParams& params,
                           const ImVec2& canvasMin,
                           const ImVec2& canvasMax,
                           const Vector3& localPlayerPos) const
{
	if (!params.config)
		return;

	Vector3 questPos = GetPosition();

	// Transform to screen position
	ImVec2 screenPos = CoordTransform::WorldToScreen(questPos, *params.config, params);
	screenPos.x += canvasMin.x;
	screenPos.y += canvasMin.y;

	// Check bounds
	if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
	    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
		return;

	ImU32 markerColor = GetColor();
	float size = 10.0f;  // Hardcoded for now, TODO: use settings from snapshot

	// Draw star/asterisk shape for quest markers
	const int numPoints = 6;
	const float innerRadius = size * 0.4f;
	const float outerRadius = size;

	ImVec2 points[12];
	for (int i = 0; i < numPoints * 2; i++)
	{
		float angle = (float)i * 3.14159f / (float)numPoints - 3.14159f / 2.0f;
		float radius = (i % 2 == 0) ? outerRadius : innerRadius;
		points[i] = ImVec2(
			screenPos.x + std::cos(angle) * radius,
			screenPos.y + std::sin(angle) * radius
		);
	}

	// Draw filled star with outline
	drawList->AddConvexPolyFilled(points, numPoints * 2, markerColor);
	drawList->AddPolyline(points, numPoints * 2, IM_COL32(0, 0, 0, 200), ImDrawFlags_Closed, 1.0f);

	// Height indicator
	float heightDiff = questPos.y - localPlayerPos.y;
	int roundedHeight = (int)std::round(heightDiff);

	// Draw quest name and distance
	float dist = GetDistanceTo(localPlayerPos);
	int roundedDist = (int)std::round(dist);

	char label[128];
	if (std::abs(roundedHeight) > 2)
	{
		const char* arrow = roundedHeight > 0 ? "▲" : "▼";
		snprintf(label, sizeof(label), "%s %s (%dm)", m_quest.QuestName.c_str(), arrow, roundedDist);
	}
	else
	{
		snprintf(label, sizeof(label), "%s (%dm)", m_quest.QuestName.c_str(), roundedDist);
	}

	// Draw label with background for readability
	ImVec2 textPos(screenPos.x + size + 4, screenPos.y - 6);
	ImVec2 textSize = ImGui::CalcTextSize(label);
	drawList->AddRectFilled(
		ImVec2(textPos.x - 2, textPos.y - 1),
		ImVec2(textPos.x + textSize.x + 2, textPos.y + textSize.y + 1),
		IM_COL32(0, 0, 0, 160)
	);
	drawList->AddText(textPos, IM_COL32(200, 130, 255, 255), label);
}

bool RadarQuestEntity::ShouldRender(float maxDistance, const Vector3& localPlayerPos) const
{
	// Quest markers are always shown regardless of distance
	return true;
}

float RadarQuestEntity::GetDistanceTo(const Vector3& pos) const
{
	Vector3 questPos = GetPosition();
	return questPos.DistanceTo(pos);
}
