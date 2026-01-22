#pragma once
#include "GUI/Radar/MapParams.h"
#include "GUI/Radar/CoordTransform.h"
#include "GUI/Radar/Core/IRadarMap.h"
#include "Game/Classes/Vector.h"
#include "imgui.h"
#include <algorithm>

// Viewport/camera management for radar map
// Handles zoom, pan, and MapParams calculation
//
// Purpose: Isolate viewport logic from rendering
// Benefits:
//   - Testable coordinate calculations
//   - Easy to add features (rotation, bookmarks, etc.)
//   - Clean separation of concerns
class RadarViewport
{
public:
	RadarViewport() = default;

	// Calculate map parameters for the current frame
	// Matches C# GetParameters() logic from Lum0s
	MapParams CalculateParams(const Vector3& centerPos,
	                         float canvasWidth,
	                         float canvasHeight,
	                         IRadarMap* map) const
	{
		MapParams params;

		if (!map || !map->IsLoaded())
		{
			// Fallback for no map
			params.boundsLeft = 0;
			params.boundsTop = 0;
			params.boundsRight = canvasWidth;
			params.boundsBottom = canvasHeight;
			params.xScale = 1.0f;
			params.yScale = 1.0f;
			return params;
		}

		params.config = &map->GetConfig();
		const auto* baseLayer = map->GetBaseLayer();
		if (!baseLayer)
			return params;

		// Calculate full map dimensions in map-space
		float fullWidth = map->GetMapWidth();
		float fullHeight = map->GetMapHeight();

		// Calculate zoomed view size (zoom 100 = 1:1, zoom 50 = 2x zoomed in)
		float zoomFactor = m_zoom * 0.01f;
		float zoomWidth = fullWidth * zoomFactor;
		float zoomHeight = fullHeight * zoomFactor;

		// Get center position in map-space
		ImVec2 centerMapPos = CoordTransform::ToMapPos(centerPos, *params.config);
		centerMapPos.x += m_panOffset.x;
		centerMapPos.y += m_panOffset.y;

		// Calculate bounds centered on player
		params.boundsLeft = centerMapPos.x - zoomWidth * 0.5f;
		params.boundsTop = centerMapPos.y - zoomHeight * 0.5f;
		params.boundsRight = centerMapPos.x + zoomWidth * 0.5f;
		params.boundsBottom = centerMapPos.y + zoomHeight * 0.5f;

		// Constrain bounds to map (prevent showing black edges)
		if (params.boundsLeft < 0)
		{
			float shift = -params.boundsLeft;
			params.boundsLeft += shift;
			params.boundsRight += shift;
		}
		if (params.boundsTop < 0)
		{
			float shift = -params.boundsTop;
			params.boundsTop += shift;
			params.boundsBottom += shift;
		}
		if (params.boundsRight > fullWidth)
		{
			float shift = params.boundsRight - fullWidth;
			params.boundsLeft -= shift;
			params.boundsRight -= shift;
		}
		if (params.boundsBottom > fullHeight)
		{
			float shift = params.boundsBottom - fullHeight;
			params.boundsTop -= shift;
			params.boundsBottom -= shift;
		}

		// Clamp final bounds to valid range
		params.boundsLeft = std::max(0.0f, params.boundsLeft);
		params.boundsTop = std::max(0.0f, params.boundsTop);
		params.boundsRight = std::min(fullWidth, params.boundsRight);
		params.boundsBottom = std::min(fullHeight, params.boundsBottom);

		// Uniform scale: preserve aspect ratio (AspectFit)
		float boundsWidth = params.GetBoundsWidth();
		float boundsHeight = params.GetBoundsHeight();

		float scaleX = canvasWidth / boundsWidth;
		float scaleY = canvasHeight / boundsHeight;
		float uniformScale = std::min(scaleX, scaleY);

		params.xScale = uniformScale;
		params.yScale = uniformScale;

		// Calculate centering offsets for uniform scale
		float renderedWidth = boundsWidth * uniformScale;
		float renderedHeight = boundsHeight * uniformScale;
		params.xOffset = (canvasWidth - renderedWidth) * 0.5f;
		params.yOffset = (canvasHeight - renderedHeight) * 0.5f;

		return params;
	}

	// Input handling
	void HandleZoom(float wheelDelta)
	{
		int zoomDelta = (int)(wheelDelta * 10);
		m_zoom = std::clamp(m_zoom - zoomDelta, MIN_ZOOM, MAX_ZOOM);
	}

	void HandlePan(const ImVec2& mouseDelta)
	{
		// Convert screen delta to map-space delta (scale by zoom)
		float zoomFactor = m_zoom * 0.01f;
		m_panOffset.x += mouseDelta.x / zoomFactor;
		m_panOffset.y += mouseDelta.y / zoomFactor;
	}

	void ResetPan()
	{
		m_panOffset = ImVec2(0, 0);
	}

	// Settings
	void SetZoom(int zoomPercent)
	{
		m_zoom = std::clamp(zoomPercent, MIN_ZOOM, MAX_ZOOM);
	}

	int GetZoom() const { return m_zoom; }
	ImVec2 GetPanOffset() const { return m_panOffset; }

	// Zoom range constants (matches Radar2D)
	static constexpr int MIN_ZOOM = 1;
	static constexpr int MAX_ZOOM = 200;

private:
	int m_zoom = 100; // 1-200, where 100 = 1:1
	ImVec2 m_panOffset = ImVec2(0, 0); // Map-space pan offset
};
