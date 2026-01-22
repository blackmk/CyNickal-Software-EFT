#include "pch.h"
#include "GUI/Radar/Radar2D.h"
#include "GUI/Color Picker/Color Picker.h"
#include "GUI/Main Window/Main Window.h"
#include "Game/EFT.h"
#include "Game/Classes/Players/CObservedPlayer/CObservedPlayer.h"
#include "Game/Enums/EBoneIndex.h"
#include "Game/Classes/Players/CClientPlayer/CClientPlayer.h"
#include "Game/Classes/CObservedLootItem/CObservedLootItem.h"
#include "Game/Classes/CExfilPoint/CExfilPoint.h"
#include "Game/Classes/Quest/CQuestManager.h"
#include "GUI/LootFilter/LootFilter.h"
#include "GUI/Radar/Widgets/WidgetManager.h"
#include <fstream>
#include <filesystem>
#include <optional>

// NanoSVG for SVG parsing and rasterization
#define NANOSVG_IMPLEMENTATION
#include "../../Dependencies/nanosvg/nanosvg.h"
#undef NANOSVG_IMPLEMENTATION

#define NANOSVGRAST_IMPLEMENTATION
#include "../../Dependencies/nanosvg/nanosvgrast.h"
#undef NANOSVGRAST_IMPLEMENTATION

#include "../../Dependencies/nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

#include <regex>

static std::string ToLower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
	return str;
}

// NanoSVG doesn't support <style> blocks. This helper inlines them.
static std::string FlattenSvgStyles(const std::string& svg)
{
	std::string result = svg;
	
	// Find style block
	std::regex styleRegex("<style[^>]*>([\\s\\S]*?)</style>");
	std::smatch styleMatch;
	
	if (std::regex_search(svg, styleMatch, styleRegex))
	{
		std::string css = styleMatch[1].str();
		
		// Map of class name -> styles (e.g. "st0" -> "fill:#123;stroke:#000;")
		std::unordered_map<std::string, std::string> classStyles;
		
		// Regex to find class rules: .name { styles }
		std::regex ruleRegex("\\.([a-zA-Z0-9_-]+)\\s*\\{([^}]*)\\}");
		auto rulesBegin = std::sregex_iterator(css.begin(), css.end(), ruleRegex);
		auto rulesEnd = std::sregex_iterator();
		
		for (std::sregex_iterator i = rulesBegin; i != rulesEnd; ++i)
		{
			std::string className = (*i)[1].str();
			std::string styles = (*i)[2].str();
			
			// Clean up styles (remove newlines and extra spaces)
			styles.erase(std::remove(styles.begin(), styles.end(), '\n'), styles.end());
			styles.erase(std::remove(styles.begin(), styles.end(), '\r'), styles.end());
			
			// Replace ':' with '=' and wrap in attributes for simplicity later? 
			// No, NanoSVG likes style="fill:#123;stroke:#000;" or separate attributes.
			// Separate attributes are more reliable in NanoSVG.
			classStyles[className] = styles;
		}
		
		// Now find all class="stX" in the SVG and replace with style="..." or direct attributes
		// Note: This is an approximation.
		for (const auto& [className, styles] : classStyles)
		{
			std::string classAttr = "class=\"" + className + "\"";
			// Convert CSS-like styles to NanoSVG compatible. 
			// Most EFT maps just use fill and stroke.
			std::string styleAttr = "";
			
			// Simple parser for properties
			std::regex propRegex("([a-z-]+)\\s*:\\s*([^;]+);?");
			auto propsBegin = std::sregex_iterator(styles.begin(), styles.end(), propRegex);
			auto propsEnd = std::sregex_iterator();
			
			for (std::sregex_iterator j = propsBegin; j != propsEnd; ++j)
			{
				std::string key = (*j)[1].str();
				std::string val = (*j)[2].str();
				// Trim val
				val.erase(0, val.find_first_not_of(" \t"));
				val.erase(val.find_last_not_of(" \t") + 1);
				
				styleAttr += key + "=\"" + val + "\" ";
			}
			
			// Replace class="stX" with the attributes
			size_t pos = 0;
			while ((pos = result.find(classAttr, pos)) != std::string::npos)
			{
				result.replace(pos, classAttr.length(), styleAttr);
				pos += styleAttr.length();
			}
		}
	}
	
	return result;
}

// Helper to read Unity strings
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

static void AutoDetectMap()
{
	auto gameWorld = EFT::GetGameWorld();
	if (!Radar2D::bAutoMap || !gameWorld)
		return;

	static auto lastCheck = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	if (now - lastCheck < std::chrono::seconds(2))
		return;
	lastCheck = now;

	DMA_Connection* Conn = DMA_Connection::GetInstance();
	if (!Conn || !Conn->IsConnected()) return;

	uintptr_t mapPtr = EFT::GetProcess().ReadMem<uintptr_t>(Conn, gameWorld->m_EntityAddress + 0xC8);
	std::string internalName = ReadUnityString(mapPtr);

	if (internalName.empty())
		return;

	// Map internal names to display names that match JSON filenames
	static const std::unordered_map<std::string, std::string> mapNames = {
		{"bigmap", "Customs"},
		{"factory4_day", "Factory"},
		{"factory4_night", "Factory"},
		{"Woods", "Woods"},
		{"RezervBase", "Reserve"},
		{"Interchange", "Interchange"},
		{"Shoreline", "Shoreline"},
		{"laboratory", "Labs"},
		{"Lighthouse", "Lighthouse"},
		{"TarkovStreets", "Streets"},
		{"Sandbox", "GroundZero"}
	};

	auto it = mapNames.find(internalName);
	if (it != mapNames.end())
	{
		Radar2D::SetCurrentMap(it->second);
	}
	else
	{
		// Try direct matching if not in table
		Radar2D::SetCurrentMap(internalName);
	}
}

bool Radar2D::Initialize(ID3D11Device* device)
{
	if (s_initialized)
		return true;
	
	if (!device)
	{
		printf("[Radar2D] Error: D3D11 device is null\n");
		return false;
	}
	
	s_device = device;
	
	// Load map configurations from Maps directory
	char exePath[MAX_PATH];
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	fs::path exeDir = fs::path(exePath).parent_path();
	fs::path mapsDir = exeDir / "Maps";
	
	printf("[Radar2D] Looking for maps in: %s\n", mapsDir.string().c_str());
	
	int loadedCount = 0;
	if (fs::exists(mapsDir))
	{
		for (const auto& entry : fs::directory_iterator(mapsDir))
		{
			if (entry.path().extension() == ".json")
			{
				if (LoadMapConfig(entry.path().string()))
				{
					printf("[Radar2D] Registered map: %s\n", entry.path().filename().string().c_str());
					loadedCount++;
				}
			}
		}
	}
	
	printf("[Radar2D] Successfully loaded %d map configurations\n", loadedCount);
	s_initialized = true; // Set to true even if 0 for now so window shows
	return true;
}

void Radar2D::Cleanup()
{
	// Release all loaded map textures
	for (auto& [id, loadedMap] : s_loadedMaps)
	{
		for (auto& layer : loadedMap.layers)
		{
			if (layer.textureView)
			{
				layer.textureView->Release();
				layer.textureView = nullptr;
			}
		}
	}
	s_loadedMaps.clear();
	s_mapConfigs.clear();
	s_currentMap = nullptr;
	s_device = nullptr;
	s_initialized = false;
	
	printf("[Radar2D] Cleaned up\n");
}

bool Radar2D::LoadMapConfig(const std::string& jsonPath)
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
			s_mapConfigs[ToLower(mapId)] = config;
		}
		
		// Also register under filename stem (e.g., "Customs")
		fs::path p(jsonPath);
		std::string stem = p.stem().string();
		s_mapConfigs[ToLower(stem)] = config;
		s_mapConfigs[stem] = config; // Keep original case for UI matching if needed
		
		return true;
	}
	catch (const std::exception& e)
	{
		printf("[Radar2D] Error loading %s: %s\n", jsonPath.c_str(), e.what());
		return false;
	}
}

bool Radar2D::LoadMapTextures(const std::string& mapId)
{
	// Check if already loaded
	auto it = s_loadedMaps.find(mapId);
	if (it != s_loadedMaps.end())
	{
		s_currentMap = &it->second;
		return true;
	}
	
	// Find config
	auto configIt = s_mapConfigs.find(mapId);
	if (configIt == s_mapConfigs.end())
	{
		printf("[Radar2D] No config found for map: %s\n", mapId.c_str());
		return false;
	}
	
	const EftMapConfig& config = configIt->second;
	
	// Get maps directory
	char exePath[MAX_PATH];
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	fs::path exeDir = fs::path(exePath).parent_path();
	fs::path mapsDir = exeDir / "Maps";
	
	LoadedMap loadedMap;
	loadedMap.config = config;
	loadedMap.id = mapId;
	
	// Load each layer's SVG
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	if (!rast)
	{
		printf("[Radar2D] Failed to create NanoSVG rasterizer\n");
		return false;
	}
	
	for (size_t i = 0; i < config.mapLayers.size(); i++)
	{
		const auto& layerConfig = config.mapLayers[i];
		fs::path svgPath = mapsDir / layerConfig.filename;
		
		if (!fs::exists(svgPath))
		{
			printf("[Radar2D] SVG not found: %s\n", svgPath.string().c_str());
			continue;
		}
		
		// Read SVG content
		std::ifstream svgFile(svgPath);
		if (!svgFile.is_open())
		{
			printf("[Radar2D] Failed to open SVG: %s\n", svgPath.string().c_str());
			continue;
		}
		
		std::string svgContent((std::istreambuf_iterator<char>(svgFile)), std::istreambuf_iterator<char>());
		svgFile.close();
		
		// Flatten styles for NanoSVG
		std::string flattenedSvg = FlattenSvgStyles(svgContent);
		
		// Parse SVG
		NSVGimage* svgImage = nsvgParse((char*)flattenedSvg.c_str(), "px", 96.0f);
		if (!svgImage)
		{
			printf("[Radar2D] Failed to parse SVG: %s\n", svgPath.string().c_str());
			continue;
		}
		
		// Rasterize at 4x scale for quality (matching C# RasterScale = 4)
		const float rasterScale = 4.0f;
		int width = (int)(svgImage->width * rasterScale);
		int height = (int)(svgImage->height * rasterScale);
		
		// Limit max size
		if (width > 8192) { width = 8192; }
		if (height > 8192) { height = 8192; }
		
		std::vector<unsigned char> pixels(width * height * 4, 0);
		nsvgRasterize(rast, svgImage, 0, 0, rasterScale, pixels.data(), width, height, width * 4);
		
		unsigned char sampledBgR = pixels[0], sampledBgG = pixels[1], sampledBgB = pixels[2], sampledBgA = pixels[3];
		printf("[Radar2D] Loaded SVG: %s (Size: %.2fx%.2f) - BG Color: %02x%02x%02x%02x\n", 
			layerConfig.filename.c_str(), svgImage->width, svgImage->height, sampledBgR, sampledBgG, sampledBgB, sampledBgA);

		// Map visibility correction: brighten the map and handle transparency
		// EFT Maps often use very dark colors that blend into background
		
		// Background removal (Chroma Key) for overlay layers
		// Overlay layers (non-base) often have a solid background color that hides the foundation
		bool isMapBase = !layerConfig.minHeight.has_value() && !layerConfig.maxHeight.has_value();
		bool doBackgroundRemoval = !isMapBase && (sampledBgA > 0);

		for (size_t p = 0; p < pixels.size(); p += 4)
		{
			if (pixels[p + 3] > 0) // Only process non-transparent pixels
			{
				// 1. Background Removal: If overlay, check if it matches the corner/bg color
				if (doBackgroundRemoval)
				{
					if (pixels[p] == sampledBgR && pixels[p + 1] == sampledBgG && pixels[p + 2] == sampledBgB)
					{
						pixels[p + 3] = 0; // Make it transparent
						continue;
					}
				}

				// 2. Brightness Boost: Boost RGB by 1.5x + higher fixed offset
				// Using +60 instead of +40 for better visibility of dark base maps
				for (int c = 0; c < 3; c++)
				{
					int val = (int)(pixels[p + c] * 1.5f) + 60;
					pixels[p + c] = (val < 0) ? 0 : (val > 255 ? 255 : (unsigned char)val);
				}
				
				// Ensure alpha is boosted too if it's low
				if (pixels[p + 3] < 200) 
				{
					int aVal = (int)(pixels[p + 3] * 1.2f);
					pixels[p + 3] = (aVal > 255) ? 255 : (unsigned char)aVal;
				}
			}
		}

		// Create D3D11 texture
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		
		D3D11_SUBRESOURCE_DATA subRes = {};
		subRes.pSysMem = pixels.data();
		subRes.SysMemPitch = width * 4;
		
		ID3D11Texture2D* texture = nullptr;
		HRESULT hr = s_device->CreateTexture2D(&texDesc, &subRes, &texture);
		if (FAILED(hr))
		{
			printf("[Radar2D] CreateTexture2D failed: 0x%08X\n", hr);
			nsvgDelete(svgImage);
			continue;
		}
		
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		
		ID3D11ShaderResourceView* srv = nullptr;
		hr = s_device->CreateShaderResourceView(texture, &srvDesc, &srv);
		texture->Release();
		
		if (FAILED(hr))
		{
			printf("[Radar2D] CreateShaderResourceView failed: 0x%08X\n", hr);
			nsvgDelete(svgImage);
			continue;
		}
		
		MapLayerTexture layerTex;
		layerTex.textureView = srv;
		layerTex.width = width;
		layerTex.height = height;
		layerTex.rawSvgWidth = svgImage->width;
		layerTex.rawSvgHeight = svgImage->height;
		layerTex.layer = &config.mapLayers[i];  // Will be fixed after insert
		
		loadedMap.layers.push_back(layerTex);
		
		printf("[Radar2D] Loaded layer: %s (%dx%d)\n", layerConfig.filename.c_str(), width, height);
		nsvgDelete(svgImage);
	}
	
	nsvgDeleteRasterizer(rast);
	
	if (loadedMap.layers.empty())
	{
		printf("[Radar2D] No layers loaded for map: %s\n", mapId.c_str());
		return false;
	}
	
	// Store and set current
	s_loadedMaps[mapId] = std::move(loadedMap);
	s_currentMap = &s_loadedMaps[mapId];
	
	// Fix layer pointers (they point to the config in the loaded map now)
	for (size_t i = 0; i < s_currentMap->layers.size(); i++)
	{
		s_currentMap->layers[i].layer = &s_currentMap->config.mapLayers[i];
	}
	
	printf("[Radar2D] Loaded map: %s with %zu layers\n", mapId.c_str(), s_currentMap->layers.size());
	return true;
}

void Radar2D::SetCurrentMap(const std::string& mapId)
{
	if (s_currentMapId == mapId && s_currentMap)
		return;
	
	std::string idLower = ToLower(mapId);
	auto configIt = s_mapConfigs.find(idLower);
	
	if (configIt == s_mapConfigs.end())
	{
		// Try original case as fallback
		configIt = s_mapConfigs.find(mapId);
	}

	if (configIt != s_mapConfigs.end())
	{
		s_currentMapId = mapId;
		LoadMapTextures(configIt->first); // Use the registered key
		return;
	}
	
	printf("[Radar2D] Map not found: %s\n", mapId.c_str());
}

MapParams Radar2D::CalculateMapParams(const Vector3& centerPos, float canvasWidth, float canvasHeight)
{
	MapParams params;
	
	if (!s_currentMap)
	{
		params.boundsLeft = 0;
		params.boundsTop = 0;
		params.boundsRight = canvasWidth;
		params.boundsBottom = canvasHeight;
		params.xScale = 1.0f;
		params.yScale = 1.0f;
		return params;
	}
	
	params.config = &s_currentMap->config;
	const auto* baseLayer = s_currentMap->GetBaseLayer();
	if (!baseLayer)
		return params;
	
	// Calculate full map dimensions in map-space (matches C# GetParameters)
	float fullWidth = baseLayer->rawSvgWidth * params.config->svgScale;
	float fullHeight = baseLayer->rawSvgHeight * params.config->svgScale;
	
	// Calculate zoomed view size (zoom 100 = 1:1, zoom 50 = 2x zoomed in)
	// This matches C# logic: zoomWidth = fullWidth * (0.01f * zoom)
	float zoomFactor = iZoom * 0.01f;
	float zoomWidth = fullWidth * zoomFactor;
	float zoomHeight = fullHeight * zoomFactor;
	
	// Get center position in map-space
	ImVec2 centerMapPos = CoordTransform::ToMapPos(centerPos, *params.config);
	centerMapPos.x += s_panOffset.x;
	centerMapPos.y += s_panOffset.y;
	
	// Calculate bounds centered on player
	params.boundsLeft = centerMapPos.x - zoomWidth * 0.5f;
	params.boundsTop = centerMapPos.y - zoomHeight * 0.5f;
	params.boundsRight = centerMapPos.x + zoomWidth * 0.5f;
	params.boundsBottom = centerMapPos.y + zoomHeight * 0.5f;
	
	// Constrain bounds to map (matches C# ConstrainBoundsToMap)
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
	
	// Uniform Scale: Use a single scale factor for both axes to preserve aspect ratio
	// The window acts as a viewport/clip, not a stretch
	float boundsWidth = params.GetBoundsWidth();
	float boundsHeight = params.GetBoundsHeight();
	
	// Calculate uniform scale based on how much map-space fits in the canvas
	// Use the smaller scale to ensure the entire zoomed area fits (AspectFit)
	float scaleX = canvasWidth / boundsWidth;
	float scaleY = canvasHeight / boundsHeight;
	float uniformScale = std::min(scaleX, scaleY);
	
	// Use uniform scale for both axes - no distortion
	params.xScale = uniformScale;
	params.yScale = uniformScale;
	
	// Calculate rendered content size and centering offsets
	float renderedWidth = boundsWidth * uniformScale;
	float renderedHeight = boundsHeight * uniformScale;
	params.xOffset = (canvasWidth - renderedWidth) * 0.5f;
	params.yOffset = (canvasHeight - renderedHeight) * 0.5f;
	
	return params;
}

void Radar2D::Render()
{
	if (!bEnabled)
		return;
	
	// Set window
	ImGui::SetNextWindowPos(ImVec2(600, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
	
	if (!ImGui::Begin("Radar", &bEnabled, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::End();
		return;
	}

	if (!s_initialized)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Radar not initialized!");
		if (MainWindow::g_pd3dDevice && ImGui::Button("Retry Initialization"))
		{
			Initialize(MainWindow::g_pd3dDevice);
		}
		ImGui::End();
		return;
	}
	
	bool inRaid = EFT::IsInRaid();
	auto gameWorld = EFT::GetGameWorld();
	if (!inRaid || !gameWorld)
	{
		s_localPlayerPos = {};
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Waiting for raid to start...");
		ImGui::End();
		return;
	}
	
	// Initialize textures on first render with D3D device
	static bool s_texturesInitialized = false;

	if (!s_texturesInitialized && MainWindow::g_pd3dDevice)
	{
		s_device = MainWindow::g_pd3dDevice;
		s_texturesInitialized = true;
	}
	
	s_windowPos = ImGui::GetWindowPos();
	s_windowSize = ImGui::GetWindowSize();
	auto drawList = ImGui::GetWindowDrawList();
	
	// Handle input
	HandleInput();
	
	// Use content region for proper canvas bounds (excludes title bar)
	ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
	ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
	ImVec2 contentSize = ImVec2(contentMax.x - contentMin.x, contentMax.y - contentMin.y);
	
	// Canvas bounds in screen coordinates
	ImVec2 canvasMin = ImVec2(s_windowPos.x + contentMin.x, s_windowPos.y + contentMin.y);
	ImVec2 canvasMax = ImVec2(s_windowPos.x + contentMax.x, s_windowPos.y + contentMax.y);
	
	// Reserve space for status bar at bottom
	const float statusBarHeight = 25.0f;
	canvasMax.y -= statusBarHeight;
	contentSize.y -= statusBarHeight;
	
	// Clip and fill background
	drawList->PushClipRect(canvasMin, canvasMax, true);
	drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(0, 0, 0, 255));
	
	// Get local player position
	if (gameWorld->m_pRegisteredPlayers)
		s_localPlayerPos = gameWorld->m_pRegisteredPlayers->GetLocalPlayerPosition();
	else
		s_localPlayerPos = {};
	
	// Auto-set map if enabled
	if (bAutoMap)
	{
		AutoDetectMap();
	}
	else if (s_currentMapId.empty() || !s_currentMap)
	{
		SetCurrentMap("Customs");
	}
	
	// Update floor based on player height
	if (bAutoFloorSwitch && s_currentMap)
	{
		// Floor detection logic would go here
	}
	
	// Calculate map parameters using proper content size
	MapParams params = CalculateMapParams(s_localPlayerPos, contentSize.x, contentSize.y);
	
	// Draw map layers
	if (bShowMapImage)
	{
		DrawMapLayers(drawList, s_localPlayerPos.y, params, canvasMin, canvasMax);
	}
	
	// Draw entities
	if (bShowExfils && gameWorld->m_pExfilController)
	{
		DrawExfils(drawList, params, canvasMin, canvasMax, gameWorld->m_pExfilController.get());
	}
	
	if (bShowLoot && gameWorld->m_pLootList)
	{
		DrawLoot(drawList, params, canvasMin, canvasMax, gameWorld->m_pLootList.get());
	}
	
	if (bShowPlayers && gameWorld->m_pRegisteredPlayers)
	{
		DrawPlayers(drawList, params, canvasMin, canvasMax, gameWorld->m_pRegisteredPlayers.get());
	}

	// Draw quest markers
	if (bShowQuestMarkers)
	{
		DrawQuestMarkers(drawList, params, canvasMin, canvasMax);
	}

	// Draw local player indicator
	if (bShowLocalPlayer)
	{
		DrawLocalPlayer(drawList, params, canvasMin);
	}

	drawList->PopClipRect();

	// Status bar
	ImGui::SetCursorPos(ImVec2(10, s_windowSize.y - 22));
	
	// Determine floor info
	std::string floorName = "Base";
	if (s_currentMap) {
		auto visible = s_currentMap->GetVisibleLayers(s_localPlayerPos.y);
		if (!visible.empty() && visible.back()->layer) {
			floorName = visible.back()->layer->label;
		}
	}

	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Map: %s (%s) | Zoom: %d%% | Pos: %.0f, %.0f, %.0f", 
		s_currentMapId.c_str(), floorName.c_str(), iZoom,
		s_localPlayerPos.x, s_localPlayerPos.y, s_localPlayerPos.z);
	
	ImGui::End();
}

void Radar2D::DrawMapLayers(ImDrawList* drawList, float playerHeight, const MapParams& params,
                            const ImVec2& canvasMin, const ImVec2& canvasMax)
{
	if (!s_currentMap || s_currentMap->layers.empty())
		return;
	
	// Get visible layers based on player height
	auto visibleLayers = s_currentMap->GetVisibleLayers(playerHeight);
	if (visibleLayers.empty())
		return;
	
	// Sort layers to ensure proper depth: Base -> Upwards
	// Base layer (terrain) must be drawn first (at the bottom)
	std::sort(visibleLayers.begin(), visibleLayers.end(), [](const MapLayerTexture* a, const MapLayerTexture* b) {
		bool aBase = a->layer && (!a->layer->minHeight.has_value() && !a->layer->maxHeight.has_value());
		bool bBase = b->layer && (!b->layer->minHeight.has_value() && !b->layer->maxHeight.has_value());
		
		if (aBase && !bBase) return true;
		if (!aBase && bBase) return false;
		
		float aMin = (a->layer && a->layer->minHeight.has_value()) ? a->layer->minHeight.value() : -1000.0f;
		float bMin = (b->layer && b->layer->minHeight.has_value()) ? b->layer->minHeight.value() : -1000.0f;
		
		if (aMin != bMin) return aMin < bMin;
		
		float aMax = (a->layer && a->layer->maxHeight.has_value()) ? a->layer->maxHeight.value() : 1000.0f;
		float bMax = (b->layer && b->layer->maxHeight.has_value()) ? b->layer->maxHeight.value() : 1000.0f;
		
		return aMax < bMax;
	});
	
	// DEBUG: Print visible layer count periodically
	static int frameCounter = 0;
	if (frameCounter++ % 600 == 0) {
		printf("[Radar2D] Rendering %zu visible layers for map %s (Height: %.2f)\n", 
			visibleLayers.size(), s_currentMapId.c_str(), playerHeight);
		for (size_t i = 0; i < visibleLayers.size(); i++) {
			printf("  Layer %zu: %s (Base: %s)\n", i, 
				visibleLayers[i]->layer ? visibleLayers[i]->layer->filename.c_str() : "unknown",
				(visibleLayers[i]->layer && (!visibleLayers[i]->layer->minHeight.has_value() && !visibleLayers[i]->layer->maxHeight.has_value())) ? "YES" : "NO");
		}
	}
	
	// We need to transform map-space bounds to texture UV coordinates
	// and draw each visible layer
	
	const float rasterScale = 4.0f;  // Must match what we used to rasterize
	
	for (size_t i = 0; i < visibleLayers.size(); i++)
	{
		const auto* layer = visibleLayers[i];
		if (!layer || !layer->textureView)
			continue;
		
		// Calculate dimming and alpha stacking
		// If dimming is disabled in config, we still use alpha for stacking context
		bool isFront = (layer == visibleLayers.back());
		bool dim = !s_currentMap->config.disableDimming && !isFront && !visibleLayers.back()->layer->cannotDimLowerLayers;
		
		ImU32 tintColor;
		if (s_currentMap->config.disableDimming)
		{
			// If dimming is disabled, use slight opacity separation instead of color dimming
			tintColor = isFront ? IM_COL32(255, 255, 255, 210) : IM_COL32(230, 230, 230, 160);
		}
		else
		{
			// Standard dimming logic
			tintColor = dim ? IM_COL32(100, 100, 100, 140) : IM_COL32(255, 255, 255, 255);
		}
		
		// Calculate source UV coordinates from bounds
		float svgScale = s_currentMap->config.svgScale;
		float mapWidth = layer->rawSvgWidth * svgScale;
		float mapHeight = layer->rawSvgHeight * svgScale;
		
		// UV coordinates based on view bounds
		float uvLeft = params.boundsLeft / mapWidth;
		float uvTop = params.boundsTop / mapHeight;
		float uvRight = params.boundsRight / mapWidth;
		float uvBottom = params.boundsBottom / mapHeight;
		
		// Clamp UVs
		uvLeft = std::clamp(uvLeft, 0.0f, 1.0f);
		uvTop = std::clamp(uvTop, 0.0f, 1.0f);
		uvRight = std::clamp(uvRight, 0.0f, 1.0f);
		uvBottom = std::clamp(uvBottom, 0.0f, 1.0f);
		
		// Calculate the actual draw region (centered within canvas)
		float boundsWidth = params.GetBoundsWidth();
		float boundsHeight = params.GetBoundsHeight();
		float renderedWidth = boundsWidth * params.xScale;
		float renderedHeight = boundsHeight * params.yScale;
		
		ImVec2 drawMin(canvasMin.x + params.xOffset, canvasMin.y + params.yOffset);
		ImVec2 drawMax(drawMin.x + renderedWidth, drawMin.y + renderedHeight);
		
		// Draw the map texture to the centered region
		drawList->AddImage(
			(ImTextureID)layer->textureView,
			drawMin, drawMax,
			ImVec2(uvLeft, uvTop), ImVec2(uvRight, uvBottom),
			tintColor
		);
	}
}

void Radar2D::DrawPlayers(ImDrawList* drawList, const MapParams& params,
	                       const ImVec2& canvasMin, const ImVec2& canvasMax,
	                       CRegisteredPlayers* playerList)
{
	if (!params.config || !playerList)
		return;
	
	std::scoped_lock lk(playerList->m_Mut);
	
	for (const auto& player : playerList->m_Players)
	{
		std::visit([&](const auto& p) {
			if (p.IsInvalid())
				return;
			
			bool isLocal = false;
			if constexpr (std::is_same_v<std::decay_t<decltype(p)>, CClientPlayer>)
				isLocal = p.IsLocalPlayer();
			
			if (!bShowLocalPlayer && isLocal)
				return;
			
			Vector3 playerPos = p.GetBonePosition(EBoneIndex::Root);
			
			// Transform to screen position using the correct formula
			ImVec2 screenPos = CoordTransform::WorldToScreen(playerPos, *params.config, params);
			screenPos.x += canvasMin.x;
			screenPos.y += canvasMin.y;
			
			// Check bounds
			if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
			    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
				return;
			
			// Get color
			ImU32 playerColor = isLocal ? ColorPicker::Radar::m_LocalPlayerColor : p.GetRadarColor();
			float iconSize = isLocal ? fPlayerIconSize * 1.5f : fPlayerIconSize;
			
			// Draw player circle
			drawList->AddCircleFilled(screenPos, iconSize, playerColor);
			drawList->AddCircle(screenPos, iconSize, IM_COL32(0, 0, 0, 200), 0, 1.0f); // Outline
			
			// Draw view direction line
			float yaw = p.m_Yaw;
			constexpr float degToRad = 0.01745329f;
			float rayLen = 25.0f;
			ImVec2 viewEnd = ImVec2(
				screenPos.x + std::sin(yaw * degToRad) * rayLen,
				screenPos.y - std::cos(yaw * degToRad) * rayLen
			);
			drawList->AddLine(screenPos, viewEnd, IM_COL32(0, 0, 0, 200), 3.0f); // Outline
			drawList->AddLine(screenPos, viewEnd, playerColor, 2.0f);
			
			// Skip labels for local player
			if (isLocal)
				return;
			
			// Height indicator (▲X or ▼X) - matching C# reference
			float heightDiff = playerPos.y - s_localPlayerPos.y;
			int roundedHeight = (int)std::round(heightDiff);
			if (roundedHeight != 0)
			{
				char heightBuf[16];
				if (roundedHeight > 0)
					snprintf(heightBuf, sizeof(heightBuf), "▲%d", roundedHeight);
				else
					snprintf(heightBuf, sizeof(heightBuf), "▼%d", std::abs(roundedHeight));
				
				ImVec2 heightPos = ImVec2(screenPos.x - iconSize - 4, screenPos.y + 4);
				drawList->AddText(heightPos, IM_COL32(255, 255, 255, 220), heightBuf);
			}
			
			// Distance - on right side
			float dist = playerPos.DistanceTo(s_localPlayerPos);
			int roundedDist = (int)std::round(dist);
			char distBuf[16];
			snprintf(distBuf, sizeof(distBuf), "%dm", roundedDist);
			ImVec2 distPos = ImVec2(screenPos.x + iconSize + 4, screenPos.y + 4);
			drawList->AddText(distPos, IM_COL32(255, 255, 255, 200), distBuf);
			
		}, player);
	}
}

void Radar2D::DrawLoot(ImDrawList* drawList, const MapParams& params,
	                     const ImVec2& canvasMin, const ImVec2& canvasMax,
	                     CLootList* lootList)
{
	if (!params.config || !lootList)
		return;
	
	auto& observedItems = lootList->m_ObservedItems;
	std::scoped_lock lk(observedItems.m_Mut);
	
	for (const auto& item : observedItems.m_Entities)
	{
		if (item.IsInvalid())
			continue;
		
		if (!LootFilter::ShouldShow(item))
			continue;
		
		ImVec2 screenPos = CoordTransform::WorldToScreen(item.m_Position, *params.config, params);
		screenPos.x += canvasMin.x;
		screenPos.y += canvasMin.y;
		
		if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
		    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
			continue;
		
		// Height indicator
		float heightDiff = item.m_Position.y - s_localPlayerPos.y;
		int roundedHeight = (int)std::round(heightDiff);
		
		ImU32 lootColor = LootFilter::GetItemColor(item);
		
		// Draw outlined square
		drawList->AddRectFilled(
			ImVec2(screenPos.x - fLootIconSize, screenPos.y - fLootIconSize),
			ImVec2(screenPos.x + fLootIconSize, screenPos.y + fLootIconSize),
			lootColor
		);
		drawList->AddRect(
			ImVec2(screenPos.x - fLootIconSize, screenPos.y - fLootIconSize),
			ImVec2(screenPos.x + fLootIconSize, screenPos.y + fLootIconSize),
			IM_COL32(0, 0, 0, 180), 0.0f, 0, 1.0f
		);
		
		// Draw height indicator if significant difference
		if (std::abs(roundedHeight) > 2)
		{
			char heightBuf[8];
			if (roundedHeight > 0)
				snprintf(heightBuf, sizeof(heightBuf), "▲");
			else
				snprintf(heightBuf, sizeof(heightBuf), "▼");
			
			ImVec2 heightPos = ImVec2(screenPos.x - fLootIconSize - 2, screenPos.y + 3);
			drawList->AddText(heightPos, IM_COL32(255, 255, 255, 200), heightBuf);
		}
	}
}

void Radar2D::DrawExfils(ImDrawList* drawList, const MapParams& params,
	                      const ImVec2& canvasMin, const ImVec2& canvasMax,
	                      CExfilController* exfilController)
{
	if (!params.config || !exfilController)
		return;
	
	std::scoped_lock lk(exfilController->m_ExfilMutex);
	
	for (const auto& exfil : exfilController->m_Exfils)
	{
		if (exfil.IsInvalid())
			continue;
		
		ImVec2 screenPos = CoordTransform::WorldToScreen(exfil.m_Position, *params.config, params);
		screenPos.x += canvasMin.x;
		screenPos.y += canvasMin.y;
		
		if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
		    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
			continue;
		
		ImU32 exfilColor = exfil.GetRadarColor();
		float size = fExfilIconSize;
		
		// Draw diamond shape with outline
		ImVec2 points[4] = {
			ImVec2(screenPos.x, screenPos.y - size),
			ImVec2(screenPos.x + size, screenPos.y),
			ImVec2(screenPos.x, screenPos.y + size),
			ImVec2(screenPos.x - size, screenPos.y)
		};
		drawList->AddQuadFilled(points[0], points[1], points[2], points[3], exfilColor);
		drawList->AddQuad(points[0], points[1], points[2], points[3], IM_COL32(0, 0, 0, 200), 1.5f);
		
		// Height indicator
		float heightDiff = exfil.m_Position.y - s_localPlayerPos.y;
		int roundedHeight = (int)std::round(heightDiff);
		
		// Draw name and distance on right side
		if (!exfil.m_Name.empty())
		{
			float dist = exfil.m_Position.DistanceTo(s_localPlayerPos);
			int roundedDist = (int)std::round(dist);
			
			// Use friendly display name instead of raw internal name
			const std::string& displayName = exfil.GetDisplayName();
			
			char label[128];
			if (std::abs(roundedHeight) > 2)
			{
				const char* arrow = roundedHeight > 0 ? "▲" : "▼";
				snprintf(label, sizeof(label), "%s %s (%dm)", displayName.c_str(), arrow, roundedDist);
			}
			else
			{
				snprintf(label, sizeof(label), "%s (%dm)", displayName.c_str(), roundedDist);
			}
			
			drawList->AddText(ImVec2(screenPos.x + size + 4, screenPos.y - 6), 
			                  IM_COL32(255, 255, 255, 220), label);
		}
	}
}

void Radar2D::DrawQuestMarkers(ImDrawList* drawList, const MapParams& params,
                               const ImVec2& canvasMin, const ImVec2& canvasMax)
{
	if (!params.config)
		return;

	auto& questMgr = Quest::CQuestManager::GetInstance();
	auto snapshot = questMgr.GetSnapshot();
	const auto& settings = snapshot.Settings;

	if (!settings.bEnabled || !settings.bShowQuestLocations)
		return;

	const auto& questLocations = snapshot.QuestLocations;

	for (const auto& loc : questLocations)
	{
		ImVec2 screenPos = CoordTransform::WorldToScreen(loc.Position, *params.config, params);
		screenPos.x += canvasMin.x;
		screenPos.y += canvasMin.y;

		// Check bounds
		if (screenPos.x < canvasMin.x || screenPos.x > canvasMax.x ||
		    screenPos.y < canvasMin.y || screenPos.y > canvasMax.y)
			continue;

		// Quest marker color (purple by default)
		ImU32 markerColor = ImGui::ColorConvertFloat4ToU32(settings.QuestLocationColor);
		float size = settings.fQuestMarkerSize;

		// Draw star/asterisk shape for quest markers
		const int numPoints = 6;
		const float innerRadius = size * 0.4f;
		const float outerRadius = size;

		ImVec2 points[12];
		for (int i = 0; i < numPoints * 2; i++)
		{
			float angle = (float)i * 3.14159f / (float)numPoints - 3.14159f / 2.0f;
			float radius = (i % 2 == 0) ? outerRadius : innerRadius;
			points[i] = ImVec2(
				screenPos.x + std::cos(angle) * radius,
				screenPos.y + std::sin(angle) * radius
			);
		}

		// Draw filled star with outline
		drawList->AddConvexPolyFilled(points, numPoints * 2, markerColor);
		drawList->AddPolyline(points, numPoints * 2, IM_COL32(0, 0, 0, 200), ImDrawFlags_Closed, 1.0f);

		// Height indicator
		float heightDiff = loc.Position.y - s_localPlayerPos.y;
		int roundedHeight = (int)std::round(heightDiff);

		// Draw quest name and distance
		float dist = loc.Position.DistanceTo(s_localPlayerPos);
		int roundedDist = (int)std::round(dist);

		char label[128];
		if (std::abs(roundedHeight) > 2)
		{
			const char* arrow = roundedHeight > 0 ? "▲" : "▼";
			snprintf(label, sizeof(label), "%s %s (%dm)", loc.QuestName.c_str(), arrow, roundedDist);
		}
		else
		{
			snprintf(label, sizeof(label), "%s (%dm)", loc.QuestName.c_str(), roundedDist);
		}

		// Draw label with background for readability
		ImVec2 textPos(screenPos.x + size + 4, screenPos.y - 6);
		ImVec2 textSize = ImGui::CalcTextSize(label);
		drawList->AddRectFilled(
			ImVec2(textPos.x - 2, textPos.y - 1),
			ImVec2(textPos.x + textSize.x + 2, textPos.y + textSize.y + 1),
			IM_COL32(0, 0, 0, 160)
		);
		drawList->AddText(textPos, IM_COL32(200, 130, 255, 255), label);
	}
}

void Radar2D::DrawLocalPlayer(ImDrawList* drawList, const MapParams& params, const ImVec2& canvasMin)
{
	if (!params.config)
		return;

	ImVec2 screenPos = CoordTransform::WorldToScreen(s_localPlayerPos, *params.config, params);
	screenPos.x += canvasMin.x;
	screenPos.y += canvasMin.y;

	// Draw crosshair at local player position
	const float crossSize = 10.0f;
	ImU32 crossColor = IM_COL32(255, 255, 255, 150);
	drawList->AddLine(
		ImVec2(screenPos.x - crossSize, screenPos.y),
		ImVec2(screenPos.x + crossSize, screenPos.y),
		crossColor, 1.0f
	);
	drawList->AddLine(
		ImVec2(screenPos.x, screenPos.y - crossSize),
		ImVec2(screenPos.x, screenPos.y + crossSize),
		crossColor, 1.0f
	);
}

void Radar2D::HandleInput()
{
	if (!ImGui::IsWindowHovered())
		return;
	
	auto& io = ImGui::GetIO();
	
	// Zoom with mouse wheel
	if (io.MouseWheel != 0.0f)
	{
		int zoomDelta = (int)(io.MouseWheel * 10);
		iZoom = std::clamp(iZoom - zoomDelta, MIN_ZOOM, MAX_ZOOM);
	}
	
	// Pan with middle mouse button
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
	{
		s_panOffset.x += io.MouseDelta.x / (iZoom * 0.01f);
		s_panOffset.y += io.MouseDelta.y / (iZoom * 0.01f);
	}
}

std::string Radar2D::GetCurrentFloorName()
{
	if (s_currentMap) {
		auto visible = s_currentMap->GetVisibleLayers(s_localPlayerPos.y);
		if (!visible.empty() && visible.back()->layer) {
			return visible.back()->layer->label;
		}
	}
	return "Base";
}

void Radar2D::RenderEmbedded()
{
	// Get available content region
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	if (contentSize.x <= 0 || contentSize.y <= 0)
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Radar view area too small");
		return;
	}
	
	// Reserve space for status bar at bottom
	const float statusBarHeight = 25.0f;
	float radarHeight = contentSize.y - statusBarHeight;
	
	// Create child window for radar content
	ImGui::BeginChild("RadarCanvas", ImVec2(contentSize.x, radarHeight), false, 
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
	if (!s_initialized)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Radar not initialized!");
		if (MainWindow::g_pd3dDevice && ImGui::Button("Retry Initialization"))
		{
			Initialize(MainWindow::g_pd3dDevice);
		}
		ImGui::EndChild();
		return;
	}
	
	bool inRaid = EFT::IsInRaid();
	auto gameWorld = EFT::GetGameWorld();
	if (!inRaid || !gameWorld)
	{
		s_localPlayerPos = {};
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Waiting for raid to start...");
		ImGui::EndChild();
		return;
	}
	
	// Initialize textures on first render with D3D device
	static bool s_texturesInitialized = false;
	if (!s_texturesInitialized && MainWindow::g_pd3dDevice)
	{
		s_device = MainWindow::g_pd3dDevice;
		s_texturesInitialized = true;
	}
	
	ImVec2 childPos = ImGui::GetWindowPos();
	ImVec2 childSize = ImGui::GetWindowSize();
	auto drawList = ImGui::GetWindowDrawList();
	
	// Handle input (zoom/pan) - check if this child is hovered
	if (ImGui::IsWindowHovered())
	{
		auto& io = ImGui::GetIO();
		
		// Zoom with mouse wheel
		if (io.MouseWheel != 0.0f)
		{
			int zoomDelta = (int)(io.MouseWheel * 10);
			iZoom = std::clamp(iZoom - zoomDelta, MIN_ZOOM, MAX_ZOOM);
		}
		
		// Pan with middle mouse button
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
		{
			s_panOffset.x += io.MouseDelta.x / (iZoom * 0.01f);
			s_panOffset.y += io.MouseDelta.y / (iZoom * 0.01f);
		}
	}
	
	// Canvas bounds in screen coordinates
	ImVec2 canvasMin = childPos;
	ImVec2 canvasMax = ImVec2(childPos.x + childSize.x, childPos.y + childSize.y);
	
	// Clip and fill background
	drawList->PushClipRect(canvasMin, canvasMax, true);
	drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(0, 0, 0, 255));
	
	// Get local player position
	if (gameWorld->m_pRegisteredPlayers)
		s_localPlayerPos = gameWorld->m_pRegisteredPlayers->GetLocalPlayerPosition();
	else
		s_localPlayerPos = {};
	
	// Auto-set map if enabled
	if (bAutoMap)
	{
		AutoDetectMap();
	}
	else if (s_currentMapId.empty() || !s_currentMap)
	{
		SetCurrentMap("Customs");
	}
	
	// Update floor based on player height
	if (bAutoFloorSwitch && s_currentMap)
	{
		// Floor detection logic would go here
	}
	
	// Calculate map parameters
	MapParams params = CalculateMapParams(s_localPlayerPos, childSize.x, childSize.y);
	
	// Draw map layers
	if (bShowMapImage)
	{
		DrawMapLayers(drawList, s_localPlayerPos.y, params, canvasMin, canvasMax);
	}
	
	// Draw entities
	if (bShowExfils && gameWorld->m_pExfilController)
	{
		DrawExfils(drawList, params, canvasMin, canvasMax, gameWorld->m_pExfilController.get());
	}
	
	if (bShowLoot && gameWorld->m_pLootList)
	{
		DrawLoot(drawList, params, canvasMin, canvasMax, gameWorld->m_pLootList.get());
	}
	
	if (bShowPlayers && gameWorld->m_pRegisteredPlayers)
	{
		DrawPlayers(drawList, params, canvasMin, canvasMax, gameWorld->m_pRegisteredPlayers.get());
	}

	// Draw quest markers
	if (bShowQuestMarkers)
	{
		DrawQuestMarkers(drawList, params, canvasMin, canvasMax);
	}

	// Draw local player indicator
	if (bShowLocalPlayer)
	{
		DrawLocalPlayer(drawList, params, canvasMin);
	}

	drawList->PopClipRect();

	ImGui::EndChild();

	// Status bar below the radar canvas
	std::string floorName = GetCurrentFloorName();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Map: %s (%s) | Zoom: %d%% | Pos: %.0f, %.0f, %.0f", 
		s_currentMapId.c_str(), floorName.c_str(), iZoom,
		s_localPlayerPos.x, s_localPlayerPos.y, s_localPlayerPos.z);
}

void Radar2D::RenderSettings()
{
	if (ImGui::CollapsingHeader("2D Radar"))
	{
		ImGui::Indent();
		
		ImGui::Checkbox("Enable Radar", &bEnabled);
		ImGui::Checkbox("Show Players", &bShowPlayers);
		ImGui::Checkbox("Show Loot", &bShowLoot);
		ImGui::Checkbox("Show Exfils", &bShowExfils);
		ImGui::Checkbox("Show Local Player", &bShowLocalPlayer);
		ImGui::Checkbox("Show Map Image", &bShowMapImage);
		ImGui::Checkbox("Show Quest Markers", &bShowQuestMarkers);
		ImGui::Checkbox("Auto Floor Switch", &bAutoFloorSwitch);
		ImGui::Checkbox("Auto-detect Map", &bAutoMap);

		ImGui::Separator();
		
		ImGui::SliderInt("Zoom", &iZoom, MIN_ZOOM, MAX_ZOOM, "%d%%");
		ImGui::SliderFloat("Player Icon Size", &fPlayerIconSize, 2.0f, 20.0f, "%.1f");
		ImGui::SliderFloat("Loot Icon Size", &fLootIconSize, 1.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Exfil Icon Size", &fExfilIconSize, 4.0f, 20.0f, "%.1f");
		
		if (ImGui::Button("Reset Pan"))
		{
			s_panOffset = ImVec2(0, 0);
		}
		
		// Map selection
		ImGui::Separator();
		const char* mapNames[] = { "Customs", "Factory", "GroundZero", "Interchange", 
		                           "Labs", "Labyrinth", "Lighthouse", "Reserve", 
		                           "Shoreline", "Streets", "Woods" };
		
		static int selectedMap = 0;
		// Sync selectedMap index to current map if it matches one of the names
		for (int i = 0; i < IM_ARRAYSIZE(mapNames); i++) {
			if (s_currentMapId == mapNames[i]) {
				selectedMap = i;
				break;
			}
		}

		if (bAutoMap) ImGui::BeginDisabled();
		if (ImGui::Combo("Map Selection", &selectedMap, mapNames, IM_ARRAYSIZE(mapNames)))
		{
			SetCurrentMap(mapNames[selectedMap]);
		}
		if (bAutoMap) ImGui::EndDisabled();
		if (bAutoMap) ImGui::TextColored(ImVec4(1, 1, 0, 1), "Auto-detection active: %s", s_currentMapId.c_str());
		
		ImGui::Unindent();
	}
	
	// Include loot filter settings
	LootFilter::RenderSettings();

	// Include widget settings
	WidgetManager::RenderSettings();

	// Player focus settings
	if (ImGui::CollapsingHeader("Player Focus & Groups"))
	{
		ImGui::Indent();
		ImGui::Checkbox("Show Focus Highlight##PF", &bShowFocusHighlight);
		ImGui::Checkbox("Show Group Lines##PF", &bShowGroupLines);
		ImGui::Checkbox("Show Hover Tooltip##PF", &bShowHoverTooltip);

		ImGui::Separator();
		ImGui::Text("Temp Teammates: %zu", PlayerFocus::TempTeammates.size());
		if (ImGui::Button("Clear All Temp Teammates##PF"))
		{
			PlayerFocus::ClearTempTeammates();
		}
		if (PlayerFocus::HasFocusedPlayer())
		{
			ImGui::SameLine();
			if (ImGui::Button("Clear Focus##PF"))
			{
				PlayerFocus::ClearFocus();
			}
		}
		ImGui::Unindent();
	}
}

void Radar2D::RenderOverlayWidgets()
{
	if (!bEnabled)
		return;

	// Render all active widgets
	WidgetManager::RenderWidgets();
}
