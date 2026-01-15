#pragma once
#include "GUI/Radar/EftMapConfig.h"
#include "GUI/Radar/MapParams.h"
#include "GUI/Radar/CoordTransform.h"
#include "Game/Classes/Vector.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <d3d11.h>
#include <algorithm>

// Forward declarations
class CPlayer;
class CObservedLootItem;
class CExfilPoint;
struct ID3D11ShaderResourceView;

// Rasterized map layer texture
struct MapLayerTexture
{
	ID3D11ShaderResourceView* textureView = nullptr;
	int width = 0;
	int height = 0;
	float rawSvgWidth = 0;   // Original SVG width before rasterization
	float rawSvgHeight = 0;  // Original SVG height before rasterization
	const EftMapLayer* layer = nullptr;  // Layer config this texture belongs to
};

// Loaded map with all layer textures
struct LoadedMap
{
	EftMapConfig config;
	std::vector<MapLayerTexture> layers;
	std::string id;
	
	// Get the base layer (first layer, typically the main map)
	const MapLayerTexture* GetBaseLayer() const
	{
		if (layers.empty()) return nullptr;
		return &layers[0];
	}
	
	// Get visible layers for a given player height
	std::vector<const MapLayerTexture*> GetVisibleLayers(float playerHeight) const
	{
		std::vector<const MapLayerTexture*> visible;
		for (const auto& layer : layers)
		{
			// Base layers (terrain/foundation) are always included
			if (!layer.layer || (!layer.layer->minHeight.has_value() && !layer.layer->maxHeight.has_value()))
			{
				visible.push_back(&layer);
				continue;
			}
			
			// Non-base layers must be in range
			if (layer.layer->IsHeightInRange(playerHeight))
			{
				visible.push_back(&layer);
			}
		}
		return visible;
	}
};

// Main 2D Radar class - handles rendering the radar map with entity overlays
class Radar2D
{
public:
	// Initialize the radar system
	static bool Initialize(ID3D11Device* device);
	
	// Cleanup resources
	static void Cleanup();
	
	// Main render function - standalone window (legacy, for pop-out mode)
	static void Render();
	
	// Embedded render function - renders radar into current ImGui region
	// Call this from within a BeginChild/EndChild or content area
	static void RenderEmbedded();
	
	// Settings panel - call from settings menu
	static void RenderSettings();
	
	// Set current map by Tarkov map ID (e.g., "bigmap" for Customs)
	static void SetCurrentMap(const std::string& mapId);
	
	// Check if radar is initialized
	static bool IsInitialized() { return s_initialized; }
	
	// Get current map ID for display
	static const std::string& GetCurrentMapId() { return s_currentMapId; }
	
	// Get local player position for status display
	static const Vector3& GetLocalPlayerPos() { return s_localPlayerPos; }
	
	// Get current floor name for status display
	static std::string GetCurrentFloorName();

public:
	// Settings (exposed for UI)
	static inline bool bEnabled = true;
	static inline bool bShowPlayers = true;
	static inline bool bShowLoot = true;
	static inline bool bShowExfils = true;
	static inline bool bShowLocalPlayer = true;
	static inline bool bShowMapImage = true;
	static inline bool bAutoFloorSwitch = true;
	static inline bool bAutoMap = true;
	
	// Zoom: 1-200, where 100 = 1:1, lower = zoomed in, higher = zoomed out
	static inline int iZoom = 100;
	static constexpr int MIN_ZOOM = 1;
	static constexpr int MAX_ZOOM = 200;
	
	// Icon sizes
	static inline float fPlayerIconSize = 8.0f;
	static inline float fLootIconSize = 4.0f;
	static inline float fExfilIconSize = 10.0f;
	
	// Current floor (for multi-level maps)
	static inline int iCurrentFloor = 0;

private:
	// Calculate map parameters for current frame
	static MapParams CalculateMapParams(const Vector3& centerPos, float canvasWidth, float canvasHeight);
	
	// Draw the map background layers
	static void DrawMapLayers(ImDrawList* drawList, float playerHeight, const MapParams& params, 
	                          const ImVec2& canvasMin, const ImVec2& canvasMax);
	
	// Draw entities
	static void DrawPlayers(ImDrawList* drawList, const MapParams& params, 
	                        const ImVec2& canvasMin, const ImVec2& canvasMax);
	static void DrawLoot(ImDrawList* drawList, const MapParams& params,
	                     const ImVec2& canvasMin, const ImVec2& canvasMax);
	static void DrawExfils(ImDrawList* drawList, const MapParams& params,
	                       const ImVec2& canvasMin, const ImVec2& canvasMax);
	static void DrawLocalPlayer(ImDrawList* drawList, const MapParams& params, const ImVec2& canvasMin);
	
	// Handle input (zoom, pan)
	static void HandleInput();
	
	// Load map configuration from JSON
	static bool LoadMapConfig(const std::string& jsonPath);
	
	// Load and rasterize SVG map layers
	static bool LoadMapTextures(const std::string& mapId);

private:
	static inline bool s_initialized = false;
	static inline ID3D11Device* s_device = nullptr;
	
	// Map data
	static inline std::unordered_map<std::string, EftMapConfig> s_mapConfigs;
	static inline std::unordered_map<std::string, LoadedMap> s_loadedMaps;
	static inline LoadedMap* s_currentMap = nullptr;
	static inline std::string s_currentMapId;
	
	// Window state
	static inline ImVec2 s_windowPos = ImVec2(0, 0);
	static inline ImVec2 s_windowSize = ImVec2(400, 400);
	static inline ImVec2 s_panOffset = ImVec2(0, 0);  // For map panning
	
	// Reference position (local player)
	static inline Vector3 s_localPlayerPos = Vector3(0, 0, 0);
};
