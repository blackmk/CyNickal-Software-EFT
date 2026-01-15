#pragma once
#include <string>
#include <vector>
#include <optional>

// Map layer representing a floor/level of the map
struct EftMapLayer
{
	std::optional<float> minHeight;  // null = no minimum
	std::optional<float> maxHeight;  // null = no maximum
	std::string filename;
	std::string label;  // Display name for this floor
	bool cannotDimLowerLayers = false;
	
	// Check if player height is within this layer's range
	bool IsHeightInRange(float height) const
	{
		// Base layers (no height restrictions) always visible
		if (!minHeight.has_value() && !maxHeight.has_value())
			return true;
		
		bool minOk = !minHeight.has_value() || height >= minHeight.value();
		bool maxOk = !maxHeight.has_value() || height < maxHeight.value();
		return minOk && maxOk;
	}
	
	// Base layer has no height restrictions
	bool IsBaseLayer() const
	{
		return !minHeight.has_value() && !maxHeight.has_value();
	}
};

// Configuration for a single Tarkov map (loaded from JSON)
// Matches C# EftMapConfig exactly
struct EftMapConfig
{
	std::vector<std::string> mapID;  // e.g., ["bigmap"] for Customs
	
	// Bitmap 'X' Coordinate of map 'Origin Location' (where Unity X is 0)
	float x = 0.0f;
	
	// Bitmap 'Y' Coordinate of map 'Origin Location' (where Unity Y is 0)
	float y = 0.0f;
	
	// Arbitrary scale value to align map scale between the Bitmap and Game Coordinates
	float scale = 1.0f;
	
	// How much to scale up the original SVG Image
	float svgScale = 1.0f;
	
	// TRUE if the map drawing should not dim layers
	bool disableDimming = false;
	
	// Map layers for multi-floor maps
	std::vector<EftMapLayer> mapLayers;
};
