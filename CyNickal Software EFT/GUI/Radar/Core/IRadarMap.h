#pragma once
#include "GUI/Radar/EftMapConfig.h"
#include <string>
#include <vector>

// Forward declaration
struct MapLayerTexture;

// Abstract interface for radar map implementations
// Inspired by Lum0s's IEftMap interface, adapted for C++/D3D11
//
// Purpose: Provides polymorphic access to map data without exposing implementation details
// Benefits:
//   - Enables future map types (procedural, custom, etc.)
//   - Clean separation between map data and rendering
//   - Testable without loading actual map files
class IRadarMap
{
public:
	virtual ~IRadarMap() = default;

	// Map identity
	virtual const std::string& GetId() const = 0;
	virtual const EftMapConfig& GetConfig() const = 0;

	// Layer management
	// Returns layers visible at the specified player height (multi-floor support)
	virtual std::vector<const MapLayerTexture*> GetVisibleLayers(float playerHeight) const = 0;

	// Returns the base layer (main terrain, always visible)
	virtual const MapLayerTexture* GetBaseLayer() const = 0;

	// Map dimensions (in map-space coordinates after SVG scaling)
	virtual float GetMapWidth() const = 0;
	virtual float GetMapHeight() const = 0;

	// Lifecycle state
	virtual bool IsLoaded() const = 0;
	virtual void Unload() = 0;
};
