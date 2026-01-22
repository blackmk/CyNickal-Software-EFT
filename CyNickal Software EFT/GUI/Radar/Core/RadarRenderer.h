#pragma once
#include "GUI/Radar/Core/IRadarMap.h"
#include "GUI/Radar/Core/IRadarEntity.h"
#include "GUI/Radar/RadarSettings.h"
#include "GUI/Radar/MapParams.h"
#include "imgui.h"
#include <vector>
#include <chrono>

// Forward declarations
struct MapLayerTexture;

// Core rendering engine for radar system
// Inspired by Lum0s's RadarViewModel rendering pipeline
//
// Purpose: Centralized rendering logic with layer dimming and entity pipeline
// Benefits:
//   - Separation of rendering from data management
//   - FPS capping to reduce CPU usage
//   - Layer dimming strategy from Lum0s
//   - Uniform entity rendering pipeline
class RadarRenderer
{
public:
	RadarRenderer() = default;

	// Main render call - renders map layers + entities
	// Parameters:
	//   drawList - ImGui draw list
	//   map - Current map to render
	//   settings - Radar settings (visibility, sizes, etc.)
	//   params - Map parameters (zoom, bounds, scale)
	//   canvasMin/Max - Screen-space canvas bounds
	//   centerPos - Center position (usually local player)
	void RenderRadar(ImDrawList* drawList,
	                IRadarMap* map,
	                const RadarSettings& settings,
	                const MapParams& params,
	                const ImVec2& canvasMin,
	                const ImVec2& canvasMax,
	                const Vector3& centerPos);

	// Render map layers with dimming strategy (from Lum0s)
	// Front layer: Full opacity
	// Lower layers: 50% dimmed (unless disabled)
	void RenderMapLayers(ImDrawList* drawList,
	                    IRadarMap* map,
	                    float playerHeight,
	                    const MapParams& params,
	                    const ImVec2& canvasMin,
	                    const ImVec2& canvasMax,
	                    bool enableDimming);

	// Render entities with culling
	void RenderEntities(ImDrawList* drawList,
	                   const std::vector<IRadarEntity*>& entities,
	                   const MapParams& params,
	                   const ImVec2& canvasMin,
	                   const ImVec2& canvasMax,
	                   const Vector3& localPlayerPos);

	// FPS capping (from Lum0s)
	// Returns true if frame should be rendered, false if skipped
	bool ShouldRenderFrame();

	// Set target FPS (default: 60)
	void SetTargetFPS(int fps) { m_targetFPS = fps; }

private:
	// Render a single layer with tint color
	void RenderLayer(ImDrawList* drawList,
	                const MapLayerTexture* layer,
	                const MapParams& params,
	                const ImVec2& canvasMin,
	                const ImVec2& canvasMax,
	                ImU32 tintColor);

	// FPS tracking
	std::chrono::steady_clock::time_point m_lastFrameTime;
	int m_targetFPS = 60;

	// Layer dimming colors (from Lum0s)
	static constexpr ImU32 TINT_FRONT = IM_COL32(255, 255, 255, 255);
	static constexpr ImU32 TINT_DIMMED = IM_COL32(127, 127, 127, 140);
};
