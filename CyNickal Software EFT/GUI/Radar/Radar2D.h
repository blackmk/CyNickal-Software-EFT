#pragma once
#include "GUI/Radar/EftMapConfig.h"
#include "GUI/Radar/MapParams.h"
#include "GUI/Radar/CoordTransform.h"
#include "GUI/Radar/PlayerFocus.h"
#include "GUI/Radar/RadarSettings.h"
#include "GUI/Radar/Core/RadarMapManager.h"
#include "GUI/Radar/Core/RadarRenderer.h"
#include "GUI/Radar/Core/RadarViewport.h"
#include "GUI/Radar/Core/IRadarEntity.h"
#include "Game/Classes/Vector.h"
#include <string>
#include <memory>
#include <vector>
#include <d3d11.h>

// Forward declarations
class CRegisteredPlayers;
class CLootList;
class CExfilController;
struct ID3D11ShaderResourceView;

// Rasterized map layer texture
// NOTE: Keep this for backward compatibility with existing code
struct MapLayerTexture
{
	ID3D11ShaderResourceView* textureView = nullptr;
	int width = 0;
	int height = 0;
	float rawSvgWidth = 0;   // Original SVG width before rasterization
	float rawSvgHeight = 0;  // Original SVG height before rasterization
	const EftMapLayer* layer = nullptr;  // Layer config this texture belongs to
};

// Main 2D Radar class - simplified facade over new architecture
// Now delegates to:
//   - RadarMapManager for map loading/caching
//   - RadarRenderer for rendering
//   - RadarViewport for zoom/pan
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
	static const std::string& GetCurrentMapId();

	// Get local player position for status display
	static const Vector3& GetLocalPlayerPos() { return s_localPlayerPos; }

	// Get current floor name for status display
	static std::string GetCurrentFloorName();

public:
	// Settings (centralized in RadarSettings struct)
	static inline RadarSettings Settings;

	// Backward compatibility aliases (deprecated - use Settings instead)
	// These delegate to Settings for compatibility with existing code
	static inline bool& bEnabled = Settings.bEnabled;
	static inline bool& bShowPlayers = Settings.bShowPlayers;
	static inline bool& bShowLoot = Settings.bShowLoot;
	static inline bool& bShowExfils = Settings.bShowExfils;
	static inline bool& bShowLocalPlayer = Settings.bShowLocalPlayer;
	static inline bool& bShowMapImage = Settings.bShowMapImage;
	static inline bool& bAutoFloorSwitch = Settings.bAutoFloorSwitch;
	static inline bool& bAutoMap = Settings.bAutoMap;
	static inline bool& bShowQuestMarkers = Settings.bShowQuestMarkers;
	static inline int& iZoom = Settings.iZoom;
	static inline float& fPlayerIconSize = Settings.fPlayerIconSize;
	static inline float& fLootIconSize = Settings.fLootIconSize;
	static inline float& fExfilIconSize = Settings.fExfilIconSize;
	static inline int& iCurrentFloor = Settings.iCurrentFloor;
	static inline bool& bShowGroupLines = Settings.bShowGroupLines;
	static inline bool& bShowFocusHighlight = Settings.bShowFocusHighlight;
	static inline bool& bShowHoverTooltip = Settings.bShowHoverTooltip;
	static inline float& fMaxPlayerDistance = Settings.fMaxPlayerDistance;
	static inline float& fMaxLootDistance = Settings.fMaxLootDistance;

	// Zoom constants
	static constexpr int MIN_ZOOM = RadarSettings::MIN_ZOOM;
	static constexpr int MAX_ZOOM = RadarSettings::MAX_ZOOM;

	// Hover state (set during render)
	static inline uintptr_t HoveredEntityAddr = 0;

	// Render overlay widgets (call after main radar render)
	static void RenderOverlayWidgets();

private:
	// Gather entities from game state (adapter pattern)
	static std::vector<IRadarEntity*> GatherPlayerEntities(CRegisteredPlayers* playerList);
	static std::vector<IRadarEntity*> GatherLootEntities(CLootList* lootList);
	static std::vector<IRadarEntity*> GatherExfilEntities(CExfilController* exfilController);
	static std::vector<IRadarEntity*> GatherQuestEntities();

	// Handle input (zoom, pan)
	static void HandleInput();

private:
	static inline bool s_initialized = false;

	// New architecture components
	static inline std::unique_ptr<RadarRenderer> s_renderer;
	static inline RadarViewport s_viewport;

	// Entity storage (for lifetime management)
	static inline std::vector<std::unique_ptr<IRadarEntity>> s_entityStorage;

	// Window state
	static inline ImVec2 s_windowPos = ImVec2(0, 0);
	static inline ImVec2 s_windowSize = ImVec2(400, 400);

	// Reference position (local player)
	static inline Vector3 s_localPlayerPos = Vector3(0, 0, 0);
};
