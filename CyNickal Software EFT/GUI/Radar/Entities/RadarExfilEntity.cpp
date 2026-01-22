#include "pch.h"
#include "GUI/Radar/Entities/RadarExfilEntity.h"
#include "GUI/Radar/CoordTransform.h"
#include <cmath>

Vector3 RadarExfilEntity::GetPosition() const
{
	if (m_exfil)
		return m_exfil->m_Position;
	return Vector3{};
}

ImU32 RadarExfilEntity::GetColor() const
{
	if (m_exfil)
		return m_exfil->GetRadarColor();
	return IM_COL32(255, 255, 255, 255);
}

bool RadarExfilEntity::IsValid() const
{
	return m_exfil && !m_exfil->IsInvalid();
}

void RadarExfilEntity::Draw(ImDrawList* drawList,
                           const MapParams& params,
                           const ImVec2& canvasMin,
                           const ImVec2& canvasMax,
                           const Vector3& localPlayerPos) const
{
	if (!IsValid() || !params.config)
		return;

	Vector3 exfilPos = GetPosition();

	// Transform to screen position
	ImVec2 screenPos = CoordTransform::WorldToScreen(exfilPos, *params.config, params);
	screenPos.x += canvasMin.x;
	screenPos.y += canvasMin.y;

	// Check bounds
	if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
	    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
		return;

	ImU32 exfilColor = GetColor();
	float size = 10.0f;  // Hardcoded for now, TODO: use settings

	// Draw diamond shape with outline
	ImVec2 points[4] = {
		ImVec2(screenPos.x, screenPos.y - size),
		ImVec2(screenPos.x + size, screenPos.y),
		ImVec2(screenPos.x, screenPos.y + size),
		ImVec2(screenPos.x - size, screenPos.y)
	};
	drawList->AddQuadFilled(points[0], points[1], points[2], points[3], exfilColor);
	drawList->AddQuad(points[0], points[1], points[2], points[3], IM_COL32(0, 0, 0, 200), 1.5f);

	// Height indicator
	float heightDiff = exfilPos.y - localPlayerPos.y;
	int roundedHeight = (int)std::round(heightDiff);

	// Draw name and distance on right side
	if (!m_exfil->m_Name.empty())
	{
		float dist = GetDistanceTo(localPlayerPos);
		int roundedDist = (int)std::round(dist);

		// Use friendly display name instead of raw internal name
		const std::string& displayName = m_exfil->GetDisplayName();

		char label[128];
		if (std::abs(roundedHeight) > 2)
		{
			const char* arrow = roundedHeight > 0 ? "▲" : "▼";
			snprintf(label, sizeof(label), "%s %s (%dm)", displayName.c_str(), arrow, roundedDist);
		}
		else
		{
			snprintf(label, sizeof(label), "%s (%dm)", displayName.c_str(), roundedDist);
		}

		drawList->AddText(ImVec2(screenPos.x + size + 4, screenPos.y - 6),
		                  IM_COL32(255, 255, 255, 220), label);
	}
}

bool RadarExfilEntity::ShouldRender(float maxDistance, const Vector3& localPlayerPos) const
{
	if (!IsValid())
		return false;

	// Exfils are always shown regardless of distance
	return true;
}

float RadarExfilEntity::GetDistanceTo(const Vector3& pos) const
{
	Vector3 exfilPos = GetPosition();
	return exfilPos.DistanceTo(pos);
}
