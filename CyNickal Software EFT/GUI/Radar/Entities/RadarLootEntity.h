#pragma once
#include "GUI/Radar/Core/IRadarEntity.h"
#include "Game/Classes/CObservedLootItem/CObservedLootItem.h"

// Radar entity adapter for loot items
// Wraps CObservedLootItem for polymorphic rendering
class RadarLootEntity : public IRadarEntity
{
public:
	explicit RadarLootEntity(const CObservedLootItem* loot)
		: m_loot(loot)
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
	const CObservedLootItem* m_loot;
};
