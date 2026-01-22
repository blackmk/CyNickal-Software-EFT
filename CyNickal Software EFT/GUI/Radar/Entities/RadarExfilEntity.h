#pragma once
#include "GUI/Radar/Core/IRadarEntity.h"
#include "Game/Classes/CExfilPoint/CExfilPoint.h"

// Radar entity adapter for exfil points
// Wraps CExfilPoint for polymorphic rendering
class RadarExfilEntity : public IRadarEntity
{
public:
	explicit RadarExfilEntity(const CExfilPoint* exfil)
		: m_exfil(exfil)
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
	const CExfilPoint* m_exfil;
};
