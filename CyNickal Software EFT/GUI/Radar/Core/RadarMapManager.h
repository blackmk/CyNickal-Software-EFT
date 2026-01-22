#pragma once
#include "GUI/Radar/Core/IRadarMap.h"
#include "GUI/Radar/EftMapConfig.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <d3d11.h>

// Centralized map loading, caching, and lifecycle management
// Inspired by Lum0s's EftMapManager singleton
//
// Purpose: Single point of control for map resources
// Benefits:
//   - Lazy loading (maps loaded on first access)
//   - Thread-safe cache access (mutex protection)
//   - Auto-detection from game memory
//   - LRU eviction (max 3 loaded maps)
//   - Resource management (RAII with unique_ptr)
//
// Usage:
//   auto& mgr = RadarMapManager::GetInstance();
//   mgr.Initialize(device, "Maps/");
//   IRadarMap* map = mgr.GetMap("bigmap");
class RadarMapManager
{
public:
	// Get singleton instance (thread-safe lazy initialization)
	static RadarMapManager& GetInstance();

	// Lifecycle
	bool Initialize(ID3D11Device* device, const std::string& mapsDir);
	void Cleanup();
	bool IsInitialized() const { return m_initialized; }

	// Map access
	// Returns nullptr if map not found or failed to load
	IRadarMap* GetMap(const std::string& mapId);

	// Get current map (last set via SetCurrentMap or GetMap)
	IRadarMap* GetCurrentMap() const { return m_currentMap; }

	// Set current map by ID (loads if not already loaded)
	bool SetCurrentMap(const std::string& mapId);

	// Auto-detection
	void EnableAutoDetection(bool enabled) { m_autoDetect = enabled; }
	void UpdateAutoDetection(); // Call per frame to check for map changes

	// Available maps
	std::vector<std::string> GetAvailableMaps() const;

	// Get maps directory
	const std::string& GetMapsDirectory() const { return m_mapsDir; }

private:
	// Singleton pattern - private constructor/destructor
	RadarMapManager() = default;
	~RadarMapManager() { Cleanup(); }

	// Delete copy/move
	RadarMapManager(const RadarMapManager&) = delete;
	RadarMapManager& operator=(const RadarMapManager&) = delete;

	// Internal loading
	bool LoadMapConfig(const std::string& jsonPath);
	IRadarMap* LoadMapTextures(const std::string& mapId);

	// Auto-detection helper
	std::string DetectCurrentMapId() const;

	// LRU eviction (keep max 3 maps loaded)
	void EvictLRU();
	static constexpr size_t MAX_LOADED_MAPS = 3;

	// State
	bool m_initialized = false;
	ID3D11Device* m_device = nullptr;
	std::string m_mapsDir;

	// Cache of loaded maps (thread-safe with mutex)
	std::unordered_map<std::string, std::unique_ptr<IRadarMap>> m_loadedMaps;
	std::unordered_map<std::string, EftMapConfig> m_mapConfigs;
	mutable std::mutex m_mapMutex;

	// Current map
	IRadarMap* m_currentMap = nullptr;

	// Auto-detection
	bool m_autoDetect = true;
	std::string m_lastDetectedMapId;
	std::chrono::steady_clock::time_point m_lastDetectionCheck;
	static constexpr auto DETECTION_INTERVAL = std::chrono::seconds(2);
};
