#pragma once
#include "Game/Classes/Vector.h"
#include "GUI/Radar/MapParams.h"
#include "imgui.h"

// Abstract interface for renderable entities on the radar
// Inspired by Lum0s's IMapEntity interface, adapted for C++/ImGui
//
// Purpose: Provides uniform rendering pipeline for all entity types
// Benefits:
//   - Separation of entity data from presentation logic
//   - Easy to add new entity types (just implement interface)
//   - Testable without game memory access
//   - Consistent rendering behavior across all entities
//
// Concrete implementations:
//   - RadarPlayerEntity (wraps CObservedPlayer/CClientPlayer)
//   - RadarLootEntity (wraps CObservedLootItem)
//   - RadarExfilEntity (wraps CExfilPoint)
//   - RadarQuestEntity (wraps QuestLocation)
class IRadarEntity
{
public:
	virtual ~IRadarEntity() = default;

	// Entity properties
	virtual Vector3 GetPosition() const = 0;
	virtual ImU32 GetColor() const = 0;
	virtual bool IsValid() const = 0;

	// Rendering
	// Draws this entity onto the provided ImGui draw list
	// Parameters:
	//   drawList - ImGui draw list for rendering
	//   params - Map parameters (zoom, bounds, scale)
	//   canvasMin/Max - Screen-space canvas bounds for clipping
	//   localPlayerPos - Local player position for distance calculations
	virtual void Draw(ImDrawList* drawList,
	                 const MapParams& params,
	                 const ImVec2& canvasMin,
	                 const ImVec2& canvasMax,
	                 const Vector3& localPlayerPos) const = 0;

	// Filtering/culling
	// Returns true if this entity should be rendered (within range, visible, etc.)
	virtual bool ShouldRender(float maxDistance, const Vector3& localPlayerPos) const = 0;

	// Calculate distance to a position (for sorting/filtering)
	virtual float GetDistanceTo(const Vector3& pos) const = 0;
};
