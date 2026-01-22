#pragma once
#include "GUI/Radar/Core/IRadarEntity.h"
#include "Game/Classes/Quest/QuestLocation.h"
#include "imgui.h"

// Radar entity adapter for quest markers
// Wraps Quest::QuestLocation for polymorphic rendering
class RadarQuestEntity : public IRadarEntity
{
public:
	explicit RadarQuestEntity(const Quest::QuestLocation& quest, ImU32 color = IM_COL32(200, 130, 255, 255))
		: m_quest(quest), m_color(color)
	{
	}

	// IRadarEntity interface
	Vector3 GetPosition() const override;
	ImU32 GetColor() const override;
	bool IsValid() const override;

	void Draw(ImDrawList* drawList,
	         const MapParams& params,
	         const ImVec2& canvasMin,
	         const ImVec2& canvasMax,
	         const Vector3& localPlayerPos) const override;

	bool ShouldRender(float maxDistance, const Vector3& localPlayerPos) const override;
	float GetDistanceTo(const Vector3& pos) const override;

private:
	Quest::QuestLocation m_quest;
	ImU32 m_color;
};
