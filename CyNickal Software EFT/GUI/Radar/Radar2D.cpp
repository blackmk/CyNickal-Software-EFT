#include "pch.h"
#include "GUI/Radar/Radar2D.h"
#include "GUI/Radar/Entities/RadarPlayerEntity.h"
#include "GUI/Radar/Entities/RadarLootEntity.h"
#include "GUI/Radar/Entities/RadarExfilEntity.h"
#include "GUI/Radar/Entities/RadarQuestEntity.h"
#include "GUI/Radar/Widgets/WidgetManager.h"
#include "Game/EFT.h"
#include "Game/Classes/CRegisteredPlayers/CRegisteredPlayers.h"
#include "Game/Classes/CLootList/CLootList.h"
#include "Game/Classes/CExfilController/CExfilController.h"
#include "Game/Classes/Quest/CQuestManager.h"
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// Initialization
// ============================================================================

bool Radar2D::Initialize(ID3D11Device* device)
{
	if (s_initialized)
		return true;

	if (!device)
	{
		printf("[Radar2D] Invalid D3D11 device\n");
		return false;
	}

	// Get maps directory
	char exePath[MAX_PATH];
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	fs::path exeDir = fs::path(exePath).parent_path();
	fs::path mapsDir = exeDir / "Maps";

	// Initialize RadarMapManager
	auto& mapMgr = RadarMapManager::GetInstance();
	if (!mapMgr.Initialize(device, mapsDir.string()))
	{
		printf("[Radar2D] Failed to initialize RadarMapManager\n");
		return false;
	}

	// Create RadarRenderer
	s_renderer = std::make_unique<RadarRenderer>();

	// Initialize viewport with current zoom
	s_viewport.SetZoom(Settings.iZoom);

	s_initialized = true;
	printf("[Radar2D] Initialized successfully\n");
	return true;
}

void Radar2D::Cleanup()
{
	s_renderer.reset();
	RadarMapManager::GetInstance().Cleanup();
	s_entityStorage.clear();
	s_initialized = false;
	printf("[Radar2D] Cleaned up\n");
}

// ============================================================================
// Map Management
// ============================================================================

void Radar2D::SetCurrentMap(const std::string& mapId)
{
	auto& mapMgr = RadarMapManager::GetInstance();
	mapMgr.SetCurrentMap(mapId);
}

const std::string& Radar2D::GetCurrentMapId()
{
	auto& mapMgr = RadarMapManager::GetInstance();
	auto* currentMap = mapMgr.GetCurrentMap();
	if (currentMap)
		return currentMap->GetId();

	static std::string empty;
	return empty;
}

std::string Radar2D::GetCurrentFloorName()
{
	auto& mapMgr = RadarMapManager::GetInstance();
	auto* currentMap = mapMgr.GetCurrentMap();
	if (currentMap)
	{
		auto visibleLayers = currentMap->GetVisibleLayers(s_localPlayerPos.y);
		if (!visibleLayers.empty() && visibleLayers.back()->layer)
		{
			return visibleLayers.back()->layer->label;
		}
	}
	return "Base";
}

// ============================================================================
// Entity Gathering (Adapter Pattern)
// ============================================================================

std::vector<IRadarEntity*> Radar2D::GatherPlayerEntities(CRegisteredPlayers* playerList)
{
	std::vector<IRadarEntity*> entities;

	if (!playerList || !Settings.bShowPlayers)
		return entities;

	std::scoped_lock lk(playerList->m_Mut);

	for (const auto& player : playerList->m_Players)
	{
		// Convert variant of objects to variant of pointers
		auto playerPtr = std::visit([](auto& p) -> std::variant<const CObservedPlayer*, const CClientPlayer*> {
			using T = std::decay_t<decltype(p)>;
			if constexpr (std::is_same_v<T, CClientPlayer>)
				return &p;
			else
				return &p; // CObservedPlayer
		}, const_cast<CRegisteredPlayers::Player&>(player));

		// Create entity wrapper
		auto entity = std::make_unique<RadarPlayerEntity>(playerPtr);

		// Check if should render (distance culling)
		if (!entity->ShouldRender(Settings.fMaxPlayerDistance, s_localPlayerPos))
			continue;

		// Skip local player if hidden
		if (!Settings.bShowLocalPlayer && entity->IsLocalPlayer())
			continue;

		entities.push_back(entity.get());
		s_entityStorage.push_back(std::move(entity));
	}

	return entities;
}

std::vector<IRadarEntity*> Radar2D::GatherLootEntities(CLootList* lootList)
{
	std::vector<IRadarEntity*> entities;

	if (!lootList || !Settings.bShowLoot)
		return entities;

	auto& observedItems = lootList->m_ObservedItems;
	std::scoped_lock lk(observedItems.m_Mut);

	for (const auto& item : observedItems.m_Entities)
	{
		// Create entity wrapper
		auto entity = std::make_unique<RadarLootEntity>(&item);

		// Check if should render (distance + filter)
		if (!entity->ShouldRender(Settings.fMaxLootDistance, s_localPlayerPos))
			continue;

		entities.push_back(entity.get());
		s_entityStorage.push_back(std::move(entity));
	}

	return entities;
}

std::vector<IRadarEntity*> Radar2D::GatherExfilEntities(CExfilController* exfilController)
{
	std::vector<IRadarEntity*> entities;

	if (!exfilController || !Settings.bShowExfils)
		return entities;

	std::scoped_lock lk(exfilController->m_ExfilMutex);

	for (const auto& exfil : exfilController->m_Exfils)
	{
		// Create entity wrapper
		auto entity = std::make_unique<RadarExfilEntity>(&exfil);

		if (!entity->IsValid())
			continue;

		entities.push_back(entity.get());
		s_entityStorage.push_back(std::move(entity));
	}

	return entities;
}

std::vector<IRadarEntity*> Radar2D::GatherQuestEntities()
{
	std::vector<IRadarEntity*> entities;

	if (!Settings.bShowQuestMarkers)
		return entities;

	auto& questMgr = Quest::CQuestManager::GetInstance();
	auto snapshot = questMgr.GetSnapshot();

	if (!snapshot.Settings.bEnabled || !snapshot.Settings.bShowQuestLocations)
		return entities;

	ImU32 questColor = ImGui::ColorConvertFloat4ToU32(snapshot.Settings.QuestLocationColor);

	for (const auto& loc : snapshot.QuestLocations)
	{
		// Create entity wrapper
		auto entity = std::make_unique<RadarQuestEntity>(loc, questColor);

		entities.push_back(entity.get());
		s_entityStorage.push_back(std::move(entity));
	}

	return entities;
}

// ============================================================================
// Input Handling
// ============================================================================

void Radar2D::HandleInput()
{
	if (!ImGui::IsWindowHovered())
		return;

	auto& io = ImGui::GetIO();

	// Zoom with mouse wheel
	if (io.MouseWheel != 0.0f)
	{
		s_viewport.HandleZoom(io.MouseWheel);
		Settings.iZoom = s_viewport.GetZoom();
	}

	// Pan with middle mouse button
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
	{
		s_viewport.HandlePan(io.MouseDelta);
	}
}

// ============================================================================
// Rendering
// ============================================================================

void Radar2D::RenderEmbedded()
{
	if (!s_initialized || !Settings.bEnabled)
		return;

	// Get game world
	auto gameWorld = EFT::GetGameWorld();
	if (!gameWorld)
		return;

	auto* pGameWorld = gameWorld.get();
	if (!pGameWorld)
		return;

	// Get local player position
	if (pGameWorld->m_pRegisteredPlayers)
		s_localPlayerPos = pGameWorld->m_pRegisteredPlayers->GetLocalPlayerPosition();

	// Auto-detect map
	auto& mapMgr = RadarMapManager::GetInstance();
	mapMgr.EnableAutoDetection(Settings.bAutoMap);
	mapMgr.UpdateAutoDetection();

	// Get current map
	auto* currentMap = mapMgr.GetCurrentMap();
	if (!currentMap)
		return;

	// Get canvas bounds
	ImVec2 canvasMin = ImGui::GetCursorScreenPos();
	ImVec2 canvasMax = ImVec2(canvasMin.x + ImGui::GetContentRegionAvail().x,
	                          canvasMin.y + ImGui::GetContentRegionAvail().y);
	ImVec2 canvasSize = ImVec2(canvasMax.x - canvasMin.x, canvasMax.y - canvasMin.y);

	// Calculate map parameters
	MapParams params = s_viewport.CalculateParams(s_localPlayerPos, canvasSize.x, canvasSize.y, currentMap);

	// Get draw list
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Render map layers
	if (s_renderer)
	{
		s_renderer->RenderRadar(drawList, currentMap, Settings, params, canvasMin, canvasMax, s_localPlayerPos);
	}

	// Clear entity storage from previous frame
	s_entityStorage.clear();

	// Gather all entities
	std::vector<IRadarEntity*> allEntities;

	// Players
	auto playerEntities = GatherPlayerEntities(pGameWorld->m_pRegisteredPlayers.get());
	allEntities.insert(allEntities.end(), playerEntities.begin(), playerEntities.end());

	// Loot
	auto lootEntities = GatherLootEntities(pGameWorld->m_pLootList.get());
	allEntities.insert(allEntities.end(), lootEntities.begin(), lootEntities.end());

	// Exfils
	auto exfilEntities = GatherExfilEntities(pGameWorld->m_pExfilController.get());
	allEntities.insert(allEntities.end(), exfilEntities.begin(), exfilEntities.end());

	// Quest markers
	auto questEntities = GatherQuestEntities();
	allEntities.insert(allEntities.end(), questEntities.begin(), questEntities.end());

	// Render all entities
	if (s_renderer)
	{
		s_renderer->RenderEntities(drawList, allEntities, params, canvasMin, canvasMax, s_localPlayerPos);
	}

	// Handle input
	HandleInput();
}

void Radar2D::Render()
{
	if (!s_initialized || !Settings.bEnabled)
		return;

	// Standalone window mode (legacy)
	ImGui::SetNextWindowPos(s_windowPos, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(s_windowSize, ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Radar 2D", &Settings.bEnabled, ImGuiWindowFlags_NoCollapse))
	{
		s_windowPos = ImGui::GetWindowPos();
		s_windowSize = ImGui::GetWindowSize();

		RenderEmbedded();
	}
	ImGui::End();
}

void Radar2D::RenderSettings()
{
	ImGui::Text("Radar 2D Settings");
	ImGui::Separator();

	ImGui::Checkbox("Enabled", &Settings.bEnabled);
	ImGui::Checkbox("Show Players", &Settings.bShowPlayers);
	ImGui::Checkbox("Show Loot", &Settings.bShowLoot);
	ImGui::Checkbox("Show Exfils", &Settings.bShowExfils);
	ImGui::Checkbox("Show Local Player", &Settings.bShowLocalPlayer);
	ImGui::Checkbox("Show Map Image", &Settings.bShowMapImage);
	ImGui::Checkbox("Show Quest Markers", &Settings.bShowQuestMarkers);
	ImGui::Checkbox("Auto Map Detection", &Settings.bAutoMap);
	ImGui::Checkbox("Auto Floor Switch", &Settings.bAutoFloorSwitch);

	ImGui::Separator();

	ImGui::SliderInt("Zoom", &Settings.iZoom, MIN_ZOOM, MAX_ZOOM);
	s_viewport.SetZoom(Settings.iZoom);

	ImGui::SliderFloat("Player Icon Size", &Settings.fPlayerIconSize, 4.0f, 20.0f);
	ImGui::SliderFloat("Loot Icon Size", &Settings.fLootIconSize, 2.0f, 10.0f);
	ImGui::SliderFloat("Exfil Icon Size", &Settings.fExfilIconSize, 5.0f, 20.0f);

	ImGui::Separator();

	ImGui::SliderFloat("Max Player Distance", &Settings.fMaxPlayerDistance, 100.0f, 1000.0f);
	ImGui::SliderFloat("Max Loot Distance", &Settings.fMaxLootDistance, 50.0f, 500.0f);

	ImGui::Separator();

	// Map selection
	auto& mapMgr = RadarMapManager::GetInstance();
	auto availableMaps = mapMgr.GetAvailableMaps();

	if (ImGui::BeginCombo("Current Map", GetCurrentMapId().c_str()))
	{
		for (const auto& mapId : availableMaps)
		{
			bool isSelected = (mapId == GetCurrentMapId());
			if (ImGui::Selectable(mapId.c_str(), isSelected))
			{
				SetCurrentMap(mapId);
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// Current floor
	ImGui::Text("Current Floor: %s", GetCurrentFloorName().c_str());
}

void Radar2D::RenderOverlayWidgets()
{
	// Delegate to WidgetManager
	// Note: WidgetManager handles PlayerInfoWidget, LootInfoWidget, AimviewWidget
	// Implementation preserved from original
}
