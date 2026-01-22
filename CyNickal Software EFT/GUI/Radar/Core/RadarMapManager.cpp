#include "pch.h"
#include "GUI/Radar/Core/RadarMapManager.h"
#include "GUI/Radar/Maps/EftMap.h"
#include "GUI/Radar/Maps/MapTextureFactory.h"
#include "Game/EFT.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "../../../Dependencies/nlohmann/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

// Helper to convert string to lowercase
static std::string ToLower(const std::string& str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
	return result;
}

// Helper to read Unity strings from memory
static std::string ReadUnityString(uintptr_t ptr)
{
	if (!ptr) return "";
	auto& Proc = EFT::GetProcess();
	DMA_Connection* Conn = DMA_Connection::GetInstance();
	if (!Conn || !Conn->IsConnected()) return "";

	int len = Proc.ReadMem<int>(Conn, ptr + 0x10);
	if (len <= 0 || len > 256) return "";

	std::vector<wchar_t> buf(len + 1, 0);
	if (!Proc.ReadBuffer(Conn, ptr + 0x14, (BYTE*)buf.data(), len * 2))
		return "";

	// Convert UTF-16 to UTF-8
	std::string result;
	for (int i = 0; i < len; i++)
	{
		if (buf[i] < 128) result += (char)buf[i];
		else result += '?';
	}
	return result;
}

RadarMapManager& RadarMapManager::GetInstance()
{
	static RadarMapManager instance;
	return instance;
}

bool RadarMapManager::Initialize(ID3D11Device* device, const std::string& mapsDir)
{
	if (m_initialized)
		return true;

	if (!device)
	{
		printf("[RadarMapManager] Invalid D3D11 device\n");
		return false;
	}

	m_device = device;
	m_mapsDir = mapsDir;

	// Load all map configs
	try
	{
		if (!fs::exists(mapsDir))
		{
			printf("[RadarMapManager] Maps directory not found: %s\n", mapsDir.c_str());
			return false;
		}

		int loadedConfigs = 0;
		for (const auto& entry : fs::directory_iterator(mapsDir))
		{
			if (entry.path().extension() == ".json")
			{
				if (LoadMapConfig(entry.path().string()))
					loadedConfigs++;
			}
		}

		printf("[RadarMapManager] Loaded %d map configs\n", loadedConfigs);
		m_initialized = true;
		return loadedConfigs > 0;
	}
	catch (const std::exception& e)
	{
		printf("[RadarMapManager] Initialization error: %s\n", e.what());
		return false;
	}
}

void RadarMapManager::Cleanup()
{
	std::scoped_lock lock(m_mapMutex);

	// Unload all maps (releases D3D11 resources)
	m_loadedMaps.clear();
	m_mapConfigs.clear();
	m_currentMap = nullptr;
	m_initialized = false;

	printf("[RadarMapManager] Cleaned up\n");
}

IRadarMap* RadarMapManager::GetMap(const std::string& mapId)
{
	if (!m_initialized)
		return nullptr;

	std::scoped_lock lock(m_mapMutex);

	// Check if already loaded
	std::string lowerMapId = ToLower(mapId);
	auto it = m_loadedMaps.find(lowerMapId);
	if (it != m_loadedMaps.end())
	{
		m_currentMap = it->second.get();
		return m_currentMap;
	}

	// Not loaded - load it now
	return LoadMapTextures(mapId);
}

bool RadarMapManager::SetCurrentMap(const std::string& mapId)
{
	IRadarMap* map = GetMap(mapId);
	if (!map)
	{
		printf("[RadarMapManager] Failed to set map: %s\n", mapId.c_str());
		return false;
	}

	m_currentMap = map;
	return true;
}

void RadarMapManager::UpdateAutoDetection()
{
	if (!m_autoDetect)
		return;

	// Check at intervals
	auto now = std::chrono::steady_clock::now();
	if (now - m_lastDetectionCheck < DETECTION_INTERVAL)
		return;
	m_lastDetectionCheck = now;

	std::string detectedMapId = DetectCurrentMapId();
	if (detectedMapId.empty() || detectedMapId == m_lastDetectedMapId)
		return;

	// New map detected
	printf("[RadarMapManager] Auto-detected map: %s\n", detectedMapId.c_str());
	m_lastDetectedMapId = detectedMapId;
	SetCurrentMap(detectedMapId);
}

std::vector<std::string> RadarMapManager::GetAvailableMaps() const
{
	std::scoped_lock lock(m_mapMutex);

	std::vector<std::string> maps;
	for (const auto& [mapId, config] : m_mapConfigs)
	{
		maps.push_back(mapId);
	}
	return maps;
}

bool RadarMapManager::LoadMapConfig(const std::string& jsonPath)
{
	try
	{
		std::ifstream file(jsonPath);
		if (!file.is_open())
			return false;

		json j;
		file >> j;

		EftMapConfig config;

		// Parse mapID array
		if (j.contains("mapID") && j["mapID"].is_array())
		{
			for (const auto& id : j["mapID"])
			{
				if (id.is_string())
					config.mapID.push_back(id.get<std::string>());
			}
		}

		// Parse numeric values
		if (j.contains("x")) config.x = j["x"].get<float>();
		if (j.contains("y")) config.y = j["y"].get<float>();
		if (j.contains("scale")) config.scale = j["scale"].get<float>();
		if (j.contains("svgScale")) config.svgScale = j["svgScale"].get<float>();
		if (j.contains("disableDimming")) config.disableDimming = j["disableDimming"].get<bool>();

		// Parse map layers
		if (j.contains("mapLayers") && j["mapLayers"].is_array())
		{
			for (const auto& layerJson : j["mapLayers"])
			{
				EftMapLayer layer;

				if (layerJson.contains("minHeight") && !layerJson["minHeight"].is_null())
					layer.minHeight = layerJson["minHeight"].get<float>();

				if (layerJson.contains("maxHeight") && !layerJson["maxHeight"].is_null())
					layer.maxHeight = layerJson["maxHeight"].get<float>();

				if (layerJson.contains("filename"))
					layer.filename = layerJson["filename"].get<std::string>();

				if (layerJson.contains("label"))
					layer.label = layerJson["label"].get<std::string>();
				else if (!layer.filename.empty())
				{
					// Infer label from filename (e.g., "Customs_1f.svg" -> "1f")
					std::string name = layer.filename;
					if (name.size() > 4 && name.ends_with(".svg"))
						name = name.substr(0, name.size() - 4);

					size_t lastUnderscore = name.find_last_of('_');
					if (lastUnderscore != std::string::npos)
						layer.label = name.substr(lastUnderscore + 1);
					else if (!layer.minHeight.has_value() && !layer.maxHeight.has_value())
						layer.label = "Base";
					else
						layer.label = name;
				}

				if (layerJson.contains("cannotDimLowerLayers"))
					layer.cannotDimLowerLayers = layerJson["cannotDimLowerLayers"].get<bool>();

				config.mapLayers.push_back(layer);
			}
		}

		// Register config for each mapID (lowercase for robust lookup)
		for (const auto& mapId : config.mapID)
		{
			m_mapConfigs[ToLower(mapId)] = config;
		}

		// Also register under filename stem (e.g., "Customs")
		fs::path p(jsonPath);
		std::string stem = p.stem().string();
		m_mapConfigs[ToLower(stem)] = config;
		m_mapConfigs[stem] = config; // Keep original case for UI matching if needed

		return true;
	}
	catch (const std::exception& e)
	{
		printf("[RadarMapManager] Error loading %s: %s\n", jsonPath.c_str(), e.what());
		return false;
	}
}

IRadarMap* RadarMapManager::LoadMapTextures(const std::string& mapId)
{
	// Find config
	std::string lowerMapId = ToLower(mapId);
	auto configIt = m_mapConfigs.find(lowerMapId);
	if (configIt == m_mapConfigs.end())
	{
		printf("[RadarMapManager] No config found for map: %s\n", mapId.c_str());
		return nullptr;
	}

	const EftMapConfig& config = configIt->second;

	// Create map instance
	auto map = std::make_unique<EftMap>(mapId, config);

	// Create texture factory
	MapTextureFactory factory(m_device);

	// Load each layer's SVG and create textures
	for (size_t i = 0; i < config.mapLayers.size(); i++)
	{
		const auto& layerConfig = config.mapLayers[i];
		fs::path svgPath = fs::path(m_mapsDir) / layerConfig.filename;

		if (!fs::exists(svgPath))
		{
			printf("[RadarMapManager] SVG not found: %s\n", svgPath.string().c_str());
			continue;
		}

		// Create texture from SVG at 4x scale (Lum0s pattern)
		MapLayerTexture layerTex = factory.CreateFromSVG(svgPath.string(), &layerConfig, 4.0f);
		if (!layerTex.textureView)
		{
			printf("[RadarMapManager] Failed to create texture for: %s\n", layerConfig.filename.c_str());
			continue;
		}

		map->AddLayer(std::move(layerTex));
		printf("[RadarMapManager] Loaded layer: %s\n", layerConfig.filename.c_str());
	}

	// Fix layer pointers (they need to point to the config in the map instance)
	auto& layers = map->GetLayers();
	for (size_t i = 0; i < layers.size(); i++)
	{
		layers[i].layer = &config.mapLayers[i];
	}

	if (!map->IsLoaded())
	{
		printf("[RadarMapManager] No layers loaded for map: %s\n", mapId.c_str());
		return nullptr;
	}

	printf("[RadarMapManager] Loaded map: %s with %zu layers\n", mapId.c_str(), layers.size());

	// Store map and return pointer
	IRadarMap* result = map.get();
	m_loadedMaps[lowerMapId] = std::move(map);
	m_currentMap = result;

	// Check LRU eviction
	EvictLRU();

	return result;
}

std::string RadarMapManager::DetectCurrentMapId() const
{
	auto gameWorld = EFT::GetGameWorld();
	if (!gameWorld)
		return "";

	DMA_Connection* Conn = DMA_Connection::GetInstance();
	if (!Conn || !Conn->IsConnected())
		return "";

	uintptr_t mapPtr = EFT::GetProcess().ReadMem<uintptr_t>(Conn, gameWorld->m_EntityAddress + 0xC8);
	std::string internalName = ReadUnityString(mapPtr);

	if (internalName.empty())
		return "";

	// Map internal names to display names that match JSON filenames
	static const std::unordered_map<std::string, std::string> nameMap = {
		{"bigmap", "Customs"},
		{"factory4_day", "Factory"},
		{"factory4_night", "Factory"},
		{"interchange", "Interchange"},
		{"laboratory", "Labs"},
		{"lighthouse", "Lighthouse"},
		{"rezervbase", "Reserve"},
		{"shoreline", "Shoreline"},
		{"woods", "Woods"},
		{"tarkovstreets", "Streets"},
		{"sandbox", "Ground Zero"}
	};

	auto it = nameMap.find(internalName);
	if (it != nameMap.end())
		return it->second;

	return internalName; // Return as-is if not mapped
}

void RadarMapManager::EvictLRU()
{
	// Simple eviction: if we have more than MAX_LOADED_MAPS, remove oldest
	// For now, just enforce the limit without tracking access time
	// (future optimization: track last access time per map)
	if (m_loadedMaps.size() <= MAX_LOADED_MAPS)
		return;

	printf("[RadarMapManager] Evicting LRU map (current count: %zu)\n", m_loadedMaps.size());

	// Find a map to evict (not the current map)
	for (auto it = m_loadedMaps.begin(); it != m_loadedMaps.end(); ++it)
	{
		if (it->second.get() != m_currentMap)
		{
			printf("[RadarMapManager] Evicting map: %s\n", it->first.c_str());
			m_loadedMaps.erase(it);
			return;
		}
	}

	// If all maps are current (shouldn't happen), evict first one
	if (!m_loadedMaps.empty())
	{
		auto it = m_loadedMaps.begin();
		printf("[RadarMapManager] Force evicting map: %s\n", it->first.c_str());
		m_loadedMaps.erase(it);
	}
}
