#pragma once
#include "GUI/Radar/Core/IRadarEntity.h"
#include "Game/Classes/Players/CObservedPlayer/CObservedPlayer.h"
#include "Game/Classes/Players/CClientPlayer/CClientPlayer.h"
#include <variant>

// Radar entity adapter for player objects
// Wraps CObservedPlayer or CClientPlayer for polymorphic rendering
//
// Purpose: Separate player data from presentation logic
// Benefits:
//   - Implements IRadarEntity interface
//   - Uniform rendering pipeline with other entities
//   - Easy to modify player rendering without affecting data structures
class RadarPlayerEntity : public IRadarEntity
{
public:
	// Constructor accepts either type of player via variant
	explicit RadarPlayerEntity(const std::variant<const CObservedPlayer*, const CClientPlayer*>& player)
		: m_player(player)
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

	// Additional player-specific properties
	float GetYaw() const;
	bool IsLocalPlayer() const;
	std::string GetName() const;
	float GetHealth() const;

private:
	std::variant<const CObservedPlayer*, const CClientPlayer*> m_player;

	// Helper to get the player pointer regardless of variant type
	const CBaseEFTPlayer* GetPlayerPtr() const;
};
