#pragma once
#include "Game/Classes/Vector.h"
#include "GUI/Radar/EftMapConfig.h"
#include "GUI/Radar/MapParams.h"
#include "imgui.h"

// Coordinate transformation utilities matching C# SkiaExtensions.cs exactly

namespace CoordTransform
{
	// Convert Unity Position (X,Y,Z) to an unzoomed Map Position
	// This matches the C# ToMapPos() in SkiaExtensions.cs exactly:
	//   X = (map.X * map.SvgScale) + (vector.X * (map.Scale * map.SvgScale))
	//   Y = (map.Y * map.SvgScale) - (vector.Z * (map.Scale * map.SvgScale))
	inline ImVec2 ToMapPos(const Vector3& worldPos, const EftMapConfig& config)
	{
		float scale = config.scale * config.svgScale;
		return ImVec2(
			(config.x * config.svgScale) + (worldPos.x * scale),
			(config.y * config.svgScale) - (worldPos.z * scale)  // Z is negated!
		);
	}
	
	// Convert an Unzoomed Map Position to a Zoomed Map Position ready for 2D Drawing
	// This matches the C# ToZoomedPos() in SkiaExtensions.cs exactly:
	//   X = (mapPos.X - mapParams.Bounds.Left) * mapParams.XScale + offset
	//   Y = (mapPos.Y - mapParams.Bounds.Top) * mapParams.YScale + offset
	inline ImVec2 ToZoomedPos(const ImVec2& mapPos, const MapParams& params)
	{
		return ImVec2(
			(mapPos.x - params.boundsLeft) * params.xScale + params.xOffset,
			(mapPos.y - params.boundsTop) * params.yScale + params.yOffset
		);
	}
	
	// Combined: World Position -> Screen Position (convenience function)
	inline ImVec2 WorldToScreen(const Vector3& worldPos, const EftMapConfig& config, const MapParams& params)
	{
		ImVec2 mapPos = ToMapPos(worldPos, config);
		return ToZoomedPos(mapPos, params);
	}
}
