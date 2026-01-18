#pragma once
#include "GUI/Radar/EftMapConfig.h"

// Map parameters for rendering - matches C# EftMapParams
// Contains the view bounds and scale factors for the current frame
struct MapParams
{
	// Currently loaded map configuration
	const EftMapConfig* config = nullptr;
	
	// Rectangular 'zoomed' bounds of the map to display (in map-space coordinates)
	float boundsLeft = 0.0f;
	float boundsTop = 0.0f;
	float boundsRight = 0.0f;
	float boundsBottom = 0.0f;
	
	// Regular -> Zoomed scale corrections
	float xScale = 1.0f;  // Map-to-screen X scale
	float yScale = 1.0f;  // Map-to-screen Y scale
	
	// Centering offset for uniform scale (to center map content in canvas)
	float xOffset = 0.0f;
	float yOffset = 0.0f;
	
	// Helper methods
	float GetBoundsWidth() const { return boundsRight - boundsLeft; }
	float GetBoundsHeight() const { return boundsBottom - boundsTop; }
};
