#include "pch.h"
#include "Config.h"

#include "GUI/Aimbot/Aimbot.h"
#include "GUI/Fuser/Fuser.h"
#include "GUI/Fuser/Overlays/Ammo Count/Ammo Count.h"
#include "GUI/Color Picker/Color Picker.h"
#include "GUI/Radar/Radar2D.h"
#include "GUI/Fuser/Draw/Loot.h"
#include "GUI/Fuser/Draw/Players.h"
#include "GUI/Fuser/Draw/Exfils.h"
#include "GUI/ESP/ESPSettings.h"
#include "GUI/Keybinds/Keybinds.h"
#include "GUI/KmboxSettings/KmboxSettings.h"
#include "GUI/Flea Bot/Flea Bot.h"
#include "Game/Camera List/Camera List.h"
#include "GUI/Player Table/Player Table.h"
#include "GUI/Item Table/Item Table.h"
#include "GUI/LootFilter/LootFilter.h"
#include "GUI/Radar/Widgets/WidgetManager.h"
#include "GUI/Radar/PlayerFocus.h"
#include "Game/Classes/Quest/CQuestManager.h"

#include <shlobj.h>
#include <fstream>
#include <chrono>


std::string Config::getConfigDir() {
	char path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, path))) {
		std::filesystem::path docPath(path);
		docPath /= "EFT-DMA";
		docPath /= "Configs";
		if (!std::filesystem::exists(docPath)) {
			std::filesystem::create_directories(docPath);
		}
		return docPath.string();
	}

	std::filesystem::path fallback("Configs");
	if (!std::filesystem::exists(fallback))
		std::filesystem::create_directory(fallback);
	return fallback.string();
}

std::string Config::getConfigPath(const std::string& configName) {
	std::filesystem::path p = getConfigDir();
	p /= configName + ".json";
	return p.string();
}

void Config::RefreshConfigFilesList(std::vector<std::string>& outList) {
	outList.clear();
	try {
		for (const auto& entry : std::filesystem::directory_iterator(getConfigDir())) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				outList.push_back(entry.path().stem().string());
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		std::println("[Config] Error listing config files: {}", e.what());
	}
}

void Config::Render()
{
	ImGui::Begin("Configs");
	static char configNameBuf[128] = "default";
	static int selectedConfig = -1;
	static std::vector<std::string> configFiles;

	static bool bFirstRun{ true };
	if (bFirstRun) {
		RefreshConfigFilesList(configFiles);
		bFirstRun = false;
	}

	ImGui::BeginChild("##ConfigsSettings", ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y), false);

	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputText("Config Name", configNameBuf, IM_ARRAYSIZE(configNameBuf));

	if (ImGui::Button("Save Config"))
	{
		SaveConfig(configNameBuf);
		RefreshConfigFilesList(configFiles);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Config"))
	{
		LoadConfig(configNameBuf);
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete Config")) {
		if (selectedConfig >= 0 && selectedConfig < static_cast<int>(configFiles.size())) {
			std::string fileToDelete = Config::getConfigPath(configFiles[selectedConfig]);
			if (std::filesystem::exists(fileToDelete)) {
				std::error_code ec;
				std::filesystem::remove(fileToDelete, ec);
				if (!ec) {
					selectedConfig = -1;
					strncpy_s(configNameBuf, sizeof(configNameBuf), "default", _TRUNCATE);
					configNameBuf[sizeof(configNameBuf) - 1] = '\0';
					RefreshConfigFilesList(configFiles);
				}
			}
		}
	}
	if (ImGui::Button("Open Folder")) {
		ShellExecuteA(nullptr, "open", getConfigDir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh List")) {
		RefreshConfigFilesList(configFiles);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##ConfigFilesList", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

	ImGui::Text("Config Files");
	ImGui::Separator();

	for (size_t i = 0; i < configFiles.size(); ++i) {
		if (ImGui::Selectable(configFiles[i].c_str(), selectedConfig == static_cast<int>(i))) {
			selectedConfig = static_cast<int>(i);
			strncpy_s(configNameBuf, sizeof(configNameBuf), configFiles[i].c_str(), _TRUNCATE);
			configNameBuf[sizeof(configNameBuf) - 1] = '\0';
		}
	}

	ImGui::EndChild();
	ImGui::End();
}

static void DeserializeKeybindObj(const json& Table, const std::string& Name, CKeybind& Out)
{
	if (!Table.contains(Name))
		return;

	const auto& k = Table[Name];
	if (k.contains("m_Key")) Out.m_Key = k["m_Key"].get<uint32_t>();
	if (k.contains("m_bTargetPC")) Out.m_bTargetPC = k["m_bTargetPC"].get<bool>();
	if (k.contains("m_bRadarPC")) Out.m_bRadarPC = k["m_bRadarPC"].get<bool>();
	if (k.contains("m_bToggle")) Out.m_bToggle = k["m_bToggle"].get<bool>();
}


void Config::DeserializeKeybinds(const json& Table) {
	if (Table.contains("bSettings")) {
		Aimbot::bSettings = Table["bSettings"].get<bool>();
	}
	DeserializeKeybindObj(Table, "DMARefresh", Keybinds::DMARefresh);
	DeserializeKeybindObj(Table, "PlayerRefresh", Keybinds::PlayerRefresh);
	DeserializeKeybindObj(Table, "Aimbot", Keybinds::Aimbot);
	DeserializeKeybindObj(Table, "FleaBot", Keybinds::FleaBot);
	DeserializeKeybindObj(Table, "OpticESP", Keybinds::OpticESP);
}


static json SerializeKeybindEntryObj(const CKeybind& kb) {
	return json{
		{ "m_Key", kb.m_Key },
		{ "m_bTargetPC", kb.m_bTargetPC },
		{ "m_bRadarPC", kb.m_bRadarPC },
		{ "m_bToggle", kb.m_bToggle }
	};
}


json Config::SerializeKeybinds(json& j) {
	j["Keybinds"] = {
		{ "bSettings", Keybinds::bSettings },
		{ "DMARefresh", SerializeKeybindEntryObj(Keybinds::DMARefresh) },
		{ "PlayerRefresh", SerializeKeybindEntryObj(Keybinds::PlayerRefresh) },
		{ "Aimbot", SerializeKeybindEntryObj(Keybinds::Aimbot) },
		{ "FleaBot", SerializeKeybindEntryObj(Keybinds::FleaBot) },
		{ "OpticESP", SerializeKeybindEntryObj(Keybinds::OpticESP) }
	};
	return j;
}


json Config::SerializeConfig() {
	json j;

	j["Aimbot"] = {
		{"bSettings", Aimbot::bSettings},
		{"bMasterToggle", Aimbot::bMasterToggle},
		{"bDrawFOV", Aimbot::bDrawFOV},
		{"fDampen", Aimbot::fDampen},
		{"fPixelFOV", Aimbot::fPixelFOV},
		{"fDeadzoneFov", Aimbot::fDeadzoneFov},
		{"eTargetBone", static_cast<int>(Aimbot::eTargetBone)},
		{"fRandomHeadChance", Aimbot::fRandomHeadChance},
		{"fRandomNeckChance", Aimbot::fRandomNeckChance},
		{"fRandomChestChance", Aimbot::fRandomChestChance},
		{"fRandomTorsoChance", Aimbot::fRandomTorsoChance},
		{"fRandomPelvisChance", Aimbot::fRandomPelvisChance},
		{"bDrawAimline", Aimbot::bDrawAimline},
		{"fAimlineLength", Aimbot::fAimlineLength},
		{"fAimlineThickness", Aimbot::fAimlineThickness},
		{"aimlineColor", static_cast<uint32_t>(Aimbot::aimlineColor)},
		// Fireport experimental
		{"bUseFireportAiming", Aimbot::bUseFireportAiming},
		{"fFireportProjectionDistance", Aimbot::fFireportProjectionDistance},
		{"bDrawAimPointDot", Aimbot::bDrawAimPointDot},
		{"fAimPointDotSize", Aimbot::fAimPointDotSize},
		{"aimPointDotColor", static_cast<uint32_t>(Aimbot::aimPointDotColor)},
		{"bDrawDebugVisualization", Aimbot::bDrawDebugVisualization},
		// Target filters
		{"bTargetPMC", Aimbot::bTargetPMC},
		{"bTargetPlayerScav", Aimbot::bTargetPlayerScav},
		{"bTargetAIScav", Aimbot::bTargetAIScav},
		{"bTargetBoss", Aimbot::bTargetBoss},
		{"bTargetRaider", Aimbot::bTargetRaider},
		{"bTargetGuard", Aimbot::bTargetGuard},
		// Advanced
		{"fMaxDistance", Aimbot::fMaxDistance},
		{"bOnlyOnAim", Aimbot::bOnlyOnAim},
		{"eTargetingMode", static_cast<int>(Aimbot::eTargetingMode)},

		// Prediction
		{"bEnablePrediction", Aimbot::bEnablePrediction},

		{"bShowTrajectory", Aimbot::bShowTrajectory},
		// KMbox/Net Device Settings
		{"bUseKmboxNet", Aimbot::bUseKmboxNet},
		{"sKmboxNetIp", Aimbot::sKmboxNetIp},
		{"nKmboxNetPort", Aimbot::nKmboxNetPort},
		{"sKmboxNetMac", Aimbot::sKmboxNetMac}
	};


	j["ExternalAimDevice"] = {
		{"bUseKmboxNet", KmboxSettings::bUseKmboxNet},
		{"sKmboxNetIp", KmboxSettings::sKmboxNetIp},
		{"nKmboxNetPort", KmboxSettings::nKmboxNetPort},
		{"sKmboxNetMac", KmboxSettings::sKmboxNetMac},
		{"bAutoConnect", KmboxSettings::bAutoConnect}
	};

	j["Fuser"] = {
		{"bSettings", Fuser::bSettings},
		{"bMasterToggle", Fuser::bMasterToggle},
		{"bRequireRaidForESP", Fuser::bRequireRaidForESP},
		{"SelectedMonitor", Fuser::m_SelectedMonitor},
		{"ScreenSize", {Fuser::m_ScreenSize.x, Fuser::m_ScreenSize.y}},

		{"AmmoCounter", {
			{"bMasterToggle", AmmoCountOverlay::bMasterToggle}
		}},

		{"Player", {
			{"bNameText", ESPSettings::Enemy::bNameEnabled},
			{"bSkeleton", ESPSettings::Enemy::bSkeletonEnabled},
			{"bHeadDot", ESPSettings::Enemy::bHeadDotEnabled}
		}},

		{"Loot", {
			{"bMasterToggle", DrawESPLoot::bMasterToggle},
			{"bContainerToggle", DrawESPLoot::bContainerToggle},
			{"bItemToggle", DrawESPLoot::bItemToggle},
			{"fMaxItemDistance", DrawESPLoot::fMaxItemDistance},
			{"fMaxContainerDistance", DrawESPLoot::fMaxContainerDistance},
			{"m_MinItemPrice", DrawESPLoot::m_MinItemPrice}
		}},

		{"Exfils", {
			{"bMasterToggle", DrawExfils::bMasterToggle}
		}},

	};

	j["Radar2D"] = {
		{"bEnabled", Radar2D::bEnabled},
		{"bShowPlayers", Radar2D::bShowPlayers},
		{"bShowLoot", Radar2D::bShowLoot},
		{"bShowExfils", Radar2D::bShowExfils},
		{"bShowLocalPlayer", Radar2D::bShowLocalPlayer},
		{"bShowMapImage", Radar2D::bShowMapImage},
		{"bAutoFloorSwitch", Radar2D::bAutoFloorSwitch},
		{"bAutoMap", Radar2D::bAutoMap},
		{"bShowQuestMarkers", Radar2D::bShowQuestMarkers},
		{"iZoom", Radar2D::iZoom},
		{"fPlayerIconSize", Radar2D::fPlayerIconSize},
		{"fLootIconSize", Radar2D::fLootIconSize},
		{"fExfilIconSize", Radar2D::fExfilIconSize},
		{"iCurrentFloor", Radar2D::iCurrentFloor},
		// Phase 2 settings
		{"bShowGroupLines", Radar2D::bShowGroupLines},
		{"bShowFocusHighlight", Radar2D::bShowFocusHighlight},
		{"bShowHoverTooltip", Radar2D::bShowHoverTooltip},
		{"fMaxPlayerDistance", Radar2D::fMaxPlayerDistance},
		{"fMaxLootDistance", Radar2D::fMaxLootDistance}
	};

	// Serialize Quest Helper settings (Phase 3)
	{
		auto& questMgr = Quest::CQuestManager::GetInstance();
		const auto settings = questMgr.GetSettingsCopy();

		// Convert blacklisted quests set to JSON array
		json blacklistedJson = json::array();
		for (const auto& questId : settings.BlacklistedQuests)
		{
			blacklistedJson.push_back(questId);
		}

		j["QuestHelper"] = {
			{"bEnabled", settings.bEnabled},
			{"bShowQuestLocations", settings.bShowQuestLocations},
			{"bHighlightQuestItems", settings.bHighlightQuestItems},
			{"bShowQuestPanel", settings.bShowQuestPanel},
			{"fQuestMarkerSize", settings.fQuestMarkerSize},
			{"QuestLocationColor", {
				settings.QuestLocationColor.x,
				settings.QuestLocationColor.y,
				settings.QuestLocationColor.z,
				settings.QuestLocationColor.w
			}},
			{"QuestItemColor", {
				settings.QuestItemColor.x,
				settings.QuestItemColor.y,
				settings.QuestItemColor.z,
				settings.QuestItemColor.w
			}},
			{"BlacklistedQuests", blacklistedJson}
		};
	}

	// Serialize widgets
	j["Widgets"] = WidgetManager::SerializeWidgets();

	// Serialize PlayerFocus settings
	j["PlayerFocus"] = {
		{"FocusHighlightColor", PlayerFocus::FocusHighlightColor},
		{"TempTeammateColor", PlayerFocus::TempTeammateColor},
		{"GroupLineColor", PlayerFocus::GroupLineColor}
	};


	j["Colors"] = {
		{"bMasterToggle", ColorPicker::bMasterToggle},
		{"Radar",{
			{"m_PMCColor", static_cast<uint32_t>(ColorPicker::Radar::m_PMCColor)},
			{"m_ScavColor", static_cast<uint32_t>(ColorPicker::Radar::m_ScavColor)},
			{"m_PlayerScavColor", static_cast<uint32_t>(ColorPicker::Radar::m_PlayerScavColor)},
			{"m_LocalPlayerColor", static_cast<uint32_t>(ColorPicker::Radar::m_LocalPlayerColor)},
			{"m_BossColor", static_cast<uint32_t>(ColorPicker::Radar::m_BossColor)},
			{"m_LootColor", static_cast<uint32_t>(ColorPicker::Radar::m_LootColor)},
			{"m_ContainerColor", static_cast<uint32_t>(ColorPicker::Radar::m_ContainerColor)},
			{"m_ExfilColor", static_cast<uint32_t>(ColorPicker::Radar::m_ExfilColor)},
		}},
		{"Fuser",{
			{"m_PMCColor", static_cast<uint32_t>(ColorPicker::Fuser::m_PMCColor)},
			{"m_ScavColor", static_cast<uint32_t>(ColorPicker::Fuser::m_ScavColor)},
			{"m_PlayerScavColor", static_cast<uint32_t>(ColorPicker::Fuser::m_PlayerScavColor)},
			{"m_BossColor", static_cast<uint32_t>(ColorPicker::Fuser::m_BossColor)},
			{"m_LootColor", static_cast<uint32_t>(ColorPicker::Fuser::m_LootColor)},
			{"m_ContainerColor", static_cast<uint32_t>(ColorPicker::Fuser::m_ContainerColor)},
			{"m_ExfilColor", static_cast<uint32_t>(ColorPicker::Fuser::m_ExfilColor)},
			{"m_WeaponTextColor", static_cast<uint32_t>(ColorPicker::Fuser::m_WeaponTextColor)},
		}},
	};

	j["ESP"] = {
		{"Enemy", {
			{"bBoxEnabled", ESPSettings::Enemy::bBoxEnabled},
			{"boxStyle", ESPSettings::Enemy::boxStyle},
			{"boxColor", static_cast<uint32_t>(ESPSettings::Enemy::boxColor)},
			{"boxThickness", ESPSettings::Enemy::boxThickness},
			{"bBoxFilled", ESPSettings::Enemy::bBoxFilled},
			{"boxFillColor", static_cast<uint32_t>(ESPSettings::Enemy::boxFillColor)},
			{"bSkeletonEnabled", ESPSettings::Enemy::bSkeletonEnabled},
			{"skeletonColor", static_cast<uint32_t>(ESPSettings::Enemy::skeletonColor)},
			{"skeletonThickness", ESPSettings::Enemy::skeletonThickness},
			{"bBonesHead", ESPSettings::Enemy::bBonesHead},
			{"bBonesSpine", ESPSettings::Enemy::bBonesSpine},
			{"bBonesArmsL", ESPSettings::Enemy::bBonesArmsL},
			{"bBonesArmsR", ESPSettings::Enemy::bBonesArmsR},
			{"bBonesLegsL", ESPSettings::Enemy::bBonesLegsL},
			{"bBonesLegsR", ESPSettings::Enemy::bBonesLegsR},
			{"bNameEnabled", ESPSettings::Enemy::bNameEnabled},
			{"nameColor", static_cast<uint32_t>(ESPSettings::Enemy::nameColor)},
			{"bDistanceEnabled", ESPSettings::Enemy::bDistanceEnabled},
			{"distanceColor", static_cast<uint32_t>(ESPSettings::Enemy::distanceColor)},
			{"bHealthEnabled", ESPSettings::Enemy::bHealthEnabled},
			{"bWeaponEnabled", ESPSettings::Enemy::bWeaponEnabled},
			{"weaponColor", static_cast<uint32_t>(ESPSettings::Enemy::weaponColor)},
			{"bHeadDotEnabled", ESPSettings::Enemy::bHeadDotEnabled},
			{"headDotColor", static_cast<uint32_t>(ESPSettings::Enemy::headDotColor)},
			{"headDotRadius", ESPSettings::Enemy::headDotRadius}
		}},
		{"RenderRange", {
			// Player ranges
			{"fPMCRange", ESPSettings::RenderRange::fPMCRange},
			{"fPlayerScavRange", ESPSettings::RenderRange::fPlayerScavRange},
			{"fBossRange", ESPSettings::RenderRange::fBossRange},
			{"fAIRange", ESPSettings::RenderRange::fAIRange},
			// Player colors
			{"PMCColor", static_cast<uint32_t>(ESPSettings::RenderRange::PMCColor)},
			{"PlayerScavColor", static_cast<uint32_t>(ESPSettings::RenderRange::PlayerScavColor)},
			{"BossColor", static_cast<uint32_t>(ESPSettings::RenderRange::BossColor)},
			{"AIColor", static_cast<uint32_t>(ESPSettings::RenderRange::AIColor)},
			// Item ranges
			{"fItemHighRange", ESPSettings::RenderRange::fItemHighRange},
			{"fItemMediumRange", ESPSettings::RenderRange::fItemMediumRange},
			{"fItemLowRange", ESPSettings::RenderRange::fItemLowRange},
			{"fItemRestRange", ESPSettings::RenderRange::fItemRestRange},
			// Item colors
			{"ItemHighColor", static_cast<uint32_t>(ESPSettings::RenderRange::ItemHighColor)},
			{"ItemMediumColor", static_cast<uint32_t>(ESPSettings::RenderRange::ItemMediumColor)},
			{"ItemLowColor", static_cast<uint32_t>(ESPSettings::RenderRange::ItemLowColor)},
			{"ItemRestColor", static_cast<uint32_t>(ESPSettings::RenderRange::ItemRestColor)},
			// Container range + color
			{"fContainerRange", ESPSettings::RenderRange::fContainerRange},
			{"ContainerColor", static_cast<uint32_t>(ESPSettings::RenderRange::ContainerColor)},
			// Exfil range + color
			{"fExfilRange", ESPSettings::RenderRange::fExfilRange},
			{"ExfilColor", static_cast<uint32_t>(ESPSettings::RenderRange::ExfilColor)}
		}}
	};

	j["Optic"] = {
		{"bOpticESP", DrawESPPlayers::bOpticESP}
		// Removed: m_OpticIndex, fOpticRadius - now auto-managed
	};

	j["Tables"] = {
		{"PlayerTable", PlayerTable::bMasterToggle},
		{"ItemTable", ItemTable::bMasterToggle}
	};

	j["FleaBot"] = {
		{"bMasterToggle", FleaBot::bMasterToggle},
		{"bLimitBuy", FleaBot::bLimitBuy},
		{"bCycleBuy", FleaBot::bCycleBuy},
		{"m_ItemCount", FleaBot::m_ItemCount},
		{"TimeoutMs", FleaBot::TimeoutDuration.count()},
		{"CycleDelayMs", FleaBot::CycleDelay.count()},
		{"SuperCycleDelayMs", FleaBot::SuperCycleDelay.count()}
	};

	// Serialize LootFilter settings
	{
		// Serialize filter presets
		json presetsJson = json::object();
		for (const auto& [presetName, filter] : LootFilter::FilterPresets)
		{
			json entriesJson = json::array();
			for (const auto& entry : filter.entries)
			{
				entriesJson.push_back({
					{"itemId", entry.itemId},
					{"itemName", entry.itemName},
					{"type", static_cast<int>(entry.type)},
					{"enabled", entry.enabled},
					{"customColor", entry.customColor},
					{"comment", entry.comment}
				});
			}
			presetsJson[presetName] = {
				{"name", filter.name},
				{"defaultImportantColor", filter.defaultImportantColor},
				{"entries", entriesJson}
			};
		}

		j["LootFilter"] = {
			{"SelectedFilter", LootFilter::SelectedFilterName},
			{"SearchString", LootFilter::SearchString},
			{"ShowMeds", LootFilter::ShowMeds},
			{"ShowFood", LootFilter::ShowFood},
			{"ShowBackpacks", LootFilter::ShowBackpacks},
			{"ShowKeys", LootFilter::ShowKeys},
			{"ShowBarter", LootFilter::ShowBarter},
			{"ShowWeapons", LootFilter::ShowWeapons},
			{"ShowAmmo", LootFilter::ShowAmmo},
			{"ShowAll", LootFilter::ShowAll},
			{"MinValueRegular", LootFilter::MinValueRegular},
			{"MinValueValuable", LootFilter::MinValueValuable},
			{"PricePerSlot", LootFilter::PricePerSlot},
			{"UseFleaPrice", LootFilter::UseFleaPrice},
			{"MinPrice", LootFilter::MinPrice},
			{"MaxPrice", LootFilter::MaxPrice},
			{"ShowQuestItems", LootFilter::ShowQuestItems},
			{"HighlightQuestItems", LootFilter::HighlightQuestItems},
			{"FilterPresets", presetsJson}
		};
	}

	SerializeKeybinds(j);

	return j;
}


void Config::DeserializeConfig(const json& j) {

	if (j.contains("Aimbot")) {
		const auto& aimbotTable = j["Aimbot"];

		if (aimbotTable.contains("bSettings")) {
			Aimbot::bSettings = aimbotTable["bSettings"].get<bool>();
		}
		if (aimbotTable.contains("bMasterToggle")) {
			Aimbot::bMasterToggle = aimbotTable["bMasterToggle"].get<bool>();
		}
		if (aimbotTable.contains("bDrawFOV")) {
			Aimbot::bDrawFOV = aimbotTable["bDrawFOV"].get<bool>();
		}
		if (aimbotTable.contains("fDampen")) {
			Aimbot::fDampen = aimbotTable["fDampen"].get<float>();
		}
		if (aimbotTable.contains("fPixelFOV")) {
			Aimbot::fPixelFOV = aimbotTable["fPixelFOV"].get<float>();
		}
		if (aimbotTable.contains("fDeadzoneFov")) {
			Aimbot::fDeadzoneFov = aimbotTable["fDeadzoneFov"].get<float>();
		}
		if (aimbotTable.contains("eTargetBone")) {
			Aimbot::eTargetBone = static_cast<ETargetBone>(aimbotTable["eTargetBone"].get<int>());
		}
		if (aimbotTable.contains("fRandomHeadChance")) {
			Aimbot::fRandomHeadChance = aimbotTable["fRandomHeadChance"].get<float>();
		}
		if (aimbotTable.contains("fRandomNeckChance")) {
			Aimbot::fRandomNeckChance = aimbotTable["fRandomNeckChance"].get<float>();
		}
		if (aimbotTable.contains("fRandomChestChance")) {
			Aimbot::fRandomChestChance = aimbotTable["fRandomChestChance"].get<float>();
		}
		if (aimbotTable.contains("fRandomTorsoChance")) {
			Aimbot::fRandomTorsoChance = aimbotTable["fRandomTorsoChance"].get<float>();
		}
		if (aimbotTable.contains("fRandomPelvisChance")) {
			Aimbot::fRandomPelvisChance = aimbotTable["fRandomPelvisChance"].get<float>();
		}
		Aimbot::NormalizeRandomWeights();
		if (aimbotTable.contains("bDrawAimline")) {
			Aimbot::bDrawAimline = aimbotTable["bDrawAimline"].get<bool>();
		}
		if (aimbotTable.contains("fAimlineLength")) {
			Aimbot::fAimlineLength = aimbotTable["fAimlineLength"].get<float>();
		}
		if (aimbotTable.contains("fAimlineThickness")) {
			Aimbot::fAimlineThickness = aimbotTable["fAimlineThickness"].get<float>();
		}
		if (aimbotTable.contains("aimlineColor")) {
			Aimbot::aimlineColor = ImColor(aimbotTable["aimlineColor"].get<uint32_t>());
		}
		// Fireport experimental
		if (aimbotTable.contains("bUseFireportAiming")) {
			Aimbot::bUseFireportAiming = aimbotTable["bUseFireportAiming"].get<bool>();
		}
		if (aimbotTable.contains("fFireportProjectionDistance")) {
			Aimbot::fFireportProjectionDistance = aimbotTable["fFireportProjectionDistance"].get<float>();
		}
		if (aimbotTable.contains("bDrawAimPointDot")) {
			Aimbot::bDrawAimPointDot = aimbotTable["bDrawAimPointDot"].get<bool>();
		}
		if (aimbotTable.contains("fAimPointDotSize")) {
			Aimbot::fAimPointDotSize = aimbotTable["fAimPointDotSize"].get<float>();
		}
		if (aimbotTable.contains("aimPointDotColor")) {
			Aimbot::aimPointDotColor = ImColor(aimbotTable["aimPointDotColor"].get<uint32_t>());
		}
		if (aimbotTable.contains("bDrawDebugVisualization")) {
			Aimbot::bDrawDebugVisualization = aimbotTable["bDrawDebugVisualization"].get<bool>();
		}
		// Target filters
		if (aimbotTable.contains("bTargetPMC")) {
			Aimbot::bTargetPMC = aimbotTable["bTargetPMC"].get<bool>();
		}
		if (aimbotTable.contains("bTargetPlayerScav")) {
			Aimbot::bTargetPlayerScav = aimbotTable["bTargetPlayerScav"].get<bool>();
		}
		if (aimbotTable.contains("bTargetAIScav")) {
			Aimbot::bTargetAIScav = aimbotTable["bTargetAIScav"].get<bool>();
		}
		if (aimbotTable.contains("bTargetBoss")) {
			Aimbot::bTargetBoss = aimbotTable["bTargetBoss"].get<bool>();
		}
		if (aimbotTable.contains("bTargetRaider")) {
			Aimbot::bTargetRaider = aimbotTable["bTargetRaider"].get<bool>();
		}
		if (aimbotTable.contains("bTargetGuard")) {
			Aimbot::bTargetGuard = aimbotTable["bTargetGuard"].get<bool>();
		}
		// Advanced
		if (aimbotTable.contains("fMaxDistance")) {
			Aimbot::fMaxDistance = aimbotTable["fMaxDistance"].get<float>();
		}
		if (aimbotTable.contains("bOnlyOnAim")) {
			Aimbot::bOnlyOnAim = aimbotTable["bOnlyOnAim"].get<bool>();
		}
		if (aimbotTable.contains("eTargetingMode")) {
			Aimbot::eTargetingMode = static_cast<ETargetingMode>(aimbotTable["eTargetingMode"].get<int>());
		}
		// Prediction
		if (aimbotTable.contains("bEnablePrediction")) {
			Aimbot::bEnablePrediction = aimbotTable["bEnablePrediction"].get<bool>();
		}
		if (aimbotTable.contains("bShowTrajectory")) {
			Aimbot::bShowTrajectory = aimbotTable["bShowTrajectory"].get<bool>();
		}
		// KMbox/Net Device Settings
		if (aimbotTable.contains("bUseKmboxNet")) {
			Aimbot::bUseKmboxNet = aimbotTable["bUseKmboxNet"].get<bool>();
		}
		if (aimbotTable.contains("sKmboxNetIp")) {
			Aimbot::sKmboxNetIp = aimbotTable["sKmboxNetIp"].get<std::string>();
		}
		if (aimbotTable.contains("nKmboxNetPort")) {
			Aimbot::nKmboxNetPort = aimbotTable["nKmboxNetPort"].get<int>();
		}
		if (aimbotTable.contains("sKmboxNetMac")) {
			Aimbot::sKmboxNetMac = aimbotTable["sKmboxNetMac"].get<std::string>();
		}
	}


	if (j.contains("ExternalAimDevice")) {
		const auto& kmboxTable = j["ExternalAimDevice"];
		if (kmboxTable.contains("bUseKmboxNet")) KmboxSettings::bUseKmboxNet = kmboxTable["bUseKmboxNet"].get<bool>();
		if (kmboxTable.contains("sKmboxNetIp")) KmboxSettings::sKmboxNetIp = kmboxTable["sKmboxNetIp"].get<std::string>();
		if (kmboxTable.contains("nKmboxNetPort")) KmboxSettings::nKmboxNetPort = kmboxTable["nKmboxNetPort"].get<int>();
		if (kmboxTable.contains("sKmboxNetMac")) KmboxSettings::sKmboxNetMac = kmboxTable["sKmboxNetMac"].get<std::string>();
		if (kmboxTable.contains("bAutoConnect")) KmboxSettings::bAutoConnect = kmboxTable["bAutoConnect"].get<bool>();
		
		// Sync back to Aimbot for compatibility
		Aimbot::bUseKmboxNet = KmboxSettings::bUseKmboxNet;
		Aimbot::sKmboxNetIp = KmboxSettings::sKmboxNetIp;
		Aimbot::nKmboxNetPort = KmboxSettings::nKmboxNetPort;
		Aimbot::sKmboxNetMac = KmboxSettings::sKmboxNetMac;
	}

	if (j.contains("Fuser")) {
		const auto& fuserTable = j["Fuser"];

		if (fuserTable.contains("bSettings")) {
			Fuser::bSettings = fuserTable["bSettings"].get<bool>();
		}
		if (fuserTable.contains("bMasterToggle")) {
			Fuser::bMasterToggle = fuserTable["bMasterToggle"].get<bool>();
		}
		if (fuserTable.contains("bRequireRaidForESP")) {
			Fuser::bRequireRaidForESP = fuserTable["bRequireRaidForESP"].get<bool>();
		}
		if (fuserTable.contains("SelectedMonitor")) {
			Fuser::m_SelectedMonitor = fuserTable["SelectedMonitor"].get<int>();
		}
		if (fuserTable.contains("ScreenSize")) {
			const auto& screenSizeJson = fuserTable["ScreenSize"];
			if (screenSizeJson.is_array() && screenSizeJson.size() == 2) {
				Fuser::m_ScreenSize = ImVec2(screenSizeJson[0].get<float>(), screenSizeJson[1].get<float>());
			}
		}

		if (fuserTable.contains("AmmoCounter")) {
			const auto& AmmoCounterTable = fuserTable["AmmoCounter"];

			if (AmmoCounterTable.contains("bMasterToggle")) {
				AmmoCountOverlay::bMasterToggle = AmmoCounterTable["bMasterToggle"].get<bool>();
			}
		}

		if (fuserTable.contains("Player")) {
			const auto& PlayerTable = fuserTable["Player"];

			if (PlayerTable.contains("bNameText")) {
				ESPSettings::Enemy::bNameEnabled = PlayerTable["bNameText"].get<bool>();
			}
			if (PlayerTable.contains("bSkeleton")) {
				ESPSettings::Enemy::bSkeletonEnabled = PlayerTable["bSkeleton"].get<bool>();
			}
			if (PlayerTable.contains("bHeadDot")) {
				ESPSettings::Enemy::bHeadDotEnabled = PlayerTable["bHeadDot"].get<bool>();
			}
		}

		if (fuserTable.contains("Loot")) {
			const auto& LootTable = fuserTable["Loot"];

			if (LootTable.contains("bMasterToggle")) {
				DrawESPLoot::bMasterToggle = LootTable["bMasterToggle"].get<bool>();
			}
			if (LootTable.contains("bItemToggle")) {
				DrawESPLoot::bItemToggle = LootTable["bItemToggle"].get<bool>();
			}
			if (LootTable.contains("bContainerToggle")) {
				DrawESPLoot::bContainerToggle = LootTable["bContainerToggle"].get<bool>();
			}
			if (LootTable.contains("fMaxContainerDistance")) {
				DrawESPLoot::fMaxContainerDistance = LootTable["fMaxContainerDistance"].get<float>();
			}
			if (LootTable.contains("fMaxItemDistance")) {
				DrawESPLoot::fMaxItemDistance = LootTable["fMaxItemDistance"].get<float>();
			}
			if (LootTable.contains("m_MinItemPrice")) {
				DrawESPLoot::m_MinItemPrice = LootTable["m_MinItemPrice"].get<int32_t>();
			}
		}

		if (fuserTable.contains("Exfils")) {
			const auto& ExfilsTable = fuserTable["Exfils"];

			if (ExfilsTable.contains("bMasterToggle")) {
				DrawExfils::bMasterToggle = ExfilsTable["bMasterToggle"].get<bool>();
			}
		}
	}

	if (j.contains("Radar2D")) {
		const auto& r2d = j["Radar2D"];
		if (r2d.contains("bEnabled")) Radar2D::bEnabled = r2d["bEnabled"].get<bool>();
		if (r2d.contains("bShowPlayers")) Radar2D::bShowPlayers = r2d["bShowPlayers"].get<bool>();
		if (r2d.contains("bShowLoot")) Radar2D::bShowLoot = r2d["bShowLoot"].get<bool>();
		if (r2d.contains("bShowExfils")) Radar2D::bShowExfils = r2d["bShowExfils"].get<bool>();
		if (r2d.contains("bShowLocalPlayer")) Radar2D::bShowLocalPlayer = r2d["bShowLocalPlayer"].get<bool>();
		if (r2d.contains("bShowMapImage")) Radar2D::bShowMapImage = r2d["bShowMapImage"].get<bool>();
		if (r2d.contains("bAutoFloorSwitch")) Radar2D::bAutoFloorSwitch = r2d["bAutoFloorSwitch"].get<bool>();
		if (r2d.contains("bAutoMap")) Radar2D::bAutoMap = r2d["bAutoMap"].get<bool>();
		if (r2d.contains("bShowQuestMarkers")) Radar2D::bShowQuestMarkers = r2d["bShowQuestMarkers"].get<bool>();
		if (r2d.contains("iZoom")) Radar2D::iZoom = r2d["iZoom"].get<int>();
		if (r2d.contains("fPlayerIconSize")) Radar2D::fPlayerIconSize = r2d["fPlayerIconSize"].get<float>();
		if (r2d.contains("fLootIconSize")) Radar2D::fLootIconSize = r2d["fLootIconSize"].get<float>();
		if (r2d.contains("fExfilIconSize")) Radar2D::fExfilIconSize = r2d["fExfilIconSize"].get<float>();
		if (r2d.contains("iCurrentFloor")) Radar2D::iCurrentFloor = r2d["iCurrentFloor"].get<int>();
		// Phase 2 settings
		if (r2d.contains("bShowGroupLines")) Radar2D::bShowGroupLines = r2d["bShowGroupLines"].get<bool>();
		if (r2d.contains("bShowFocusHighlight")) Radar2D::bShowFocusHighlight = r2d["bShowFocusHighlight"].get<bool>();
		if (r2d.contains("bShowHoverTooltip")) Radar2D::bShowHoverTooltip = r2d["bShowHoverTooltip"].get<bool>();
		if (r2d.contains("fMaxPlayerDistance")) Radar2D::fMaxPlayerDistance = r2d["fMaxPlayerDistance"].get<float>();
		if (r2d.contains("fMaxLootDistance")) Radar2D::fMaxLootDistance = r2d["fMaxLootDistance"].get<float>();
	}

	// Deserialize Quest Helper settings (Phase 3)
	if (j.contains("QuestHelper")) {
		const auto& qh = j["QuestHelper"];
		auto& questMgr = Quest::CQuestManager::GetInstance();
		auto settings = questMgr.GetSettingsCopy();

		if (qh.contains("bEnabled")) settings.bEnabled = qh["bEnabled"].get<bool>();
		if (qh.contains("bShowQuestLocations")) settings.bShowQuestLocations = qh["bShowQuestLocations"].get<bool>();
		if (qh.contains("bHighlightQuestItems")) settings.bHighlightQuestItems = qh["bHighlightQuestItems"].get<bool>();
		if (qh.contains("bShowQuestPanel")) settings.bShowQuestPanel = qh["bShowQuestPanel"].get<bool>();
		if (qh.contains("fQuestMarkerSize")) settings.fQuestMarkerSize = qh["fQuestMarkerSize"].get<float>();

		if (qh.contains("QuestLocationColor") && qh["QuestLocationColor"].is_array() && qh["QuestLocationColor"].size() == 4) {
			const auto& col = qh["QuestLocationColor"];
			settings.QuestLocationColor = ImVec4(col[0].get<float>(), col[1].get<float>(), col[2].get<float>(), col[3].get<float>());
		}

		if (qh.contains("QuestItemColor") && qh["QuestItemColor"].is_array() && qh["QuestItemColor"].size() == 4) {
			const auto& col = qh["QuestItemColor"];
			settings.QuestItemColor = ImVec4(col[0].get<float>(), col[1].get<float>(), col[2].get<float>(), col[3].get<float>());
		}

		if (qh.contains("BlacklistedQuests") && qh["BlacklistedQuests"].is_array()) {
			settings.BlacklistedQuests.clear();
			for (const auto& questId : qh["BlacklistedQuests"]) {
				settings.BlacklistedQuests.insert(questId.get<std::string>());
			}
		}

		questMgr.UpdateSettings(settings);
	}

	// Deserialize widgets
	if (j.contains("Widgets")) {
		WidgetManager::DeserializeWidgets(j["Widgets"]);
	}

	// Deserialize PlayerFocus settings
	if (j.contains("PlayerFocus")) {
		const auto& pf = j["PlayerFocus"];
		if (pf.contains("FocusHighlightColor")) PlayerFocus::FocusHighlightColor = pf["FocusHighlightColor"].get<ImU32>();
		if (pf.contains("TempTeammateColor")) PlayerFocus::TempTeammateColor = pf["TempTeammateColor"].get<ImU32>();
		if (pf.contains("GroupLineColor")) PlayerFocus::GroupLineColor = pf["GroupLineColor"].get<ImU32>();
	}

	if (j.contains("Colors")) {
		const auto& colorsTable = j["Colors"];

		if (colorsTable.contains("bMasterToggle")) {
			ColorPicker::bMasterToggle = colorsTable["bMasterToggle"].get<bool>();
		}

		if (colorsTable.contains("Radar"))
		{
			using namespace ColorPicker::Radar;
			const auto& RadarTable = colorsTable["Radar"];
			if (RadarTable.contains("m_PMCColor")) {
				m_PMCColor = RadarTable["m_PMCColor"].get<ImU32>();
			}
			if (RadarTable.contains("m_ScavColor")) {
				m_ScavColor = RadarTable["m_ScavColor"].get<ImU32>();
			}
			if (RadarTable.contains("m_PlayerScavColor")) {
				m_PlayerScavColor = RadarTable["m_PlayerScavColor"].get<ImU32>();
			}
			if (RadarTable.contains("m_LocalPlayerColor")) {
				m_LocalPlayerColor = RadarTable["m_LocalPlayerColor"].get<ImU32>();
			}
			if (RadarTable.contains("m_BossColor")) {
				m_BossColor = RadarTable["m_BossColor"].get<ImU32>();
			}
			if (RadarTable.contains("m_LootColor")) {
				m_LootColor = RadarTable["m_LootColor"].get<ImU32>();
			}
			if (RadarTable.contains("m_ContainerColor")) {
				m_ContainerColor = RadarTable["m_ContainerColor"].get<ImU32>();
			}
			if (RadarTable.contains("m_ExfilColor")) {
				m_ExfilColor = RadarTable["m_ExfilColor"].get<ImU32>();
			}
		}

		if (colorsTable.contains("Fuser"))
		{
			using namespace ColorPicker::Fuser;
			const auto& FuserTable = colorsTable["Fuser"];
			if (FuserTable.contains("m_PMCColor")) {
				m_PMCColor = FuserTable["m_PMCColor"].get<ImU32>();
			}
			if (FuserTable.contains("m_ScavColor")) {
				m_ScavColor = FuserTable["m_ScavColor"].get<ImU32>();
			}
			if (FuserTable.contains("m_PlayerScavColor")) {
				m_PlayerScavColor = FuserTable["m_PlayerScavColor"].get<ImU32>();
			}
			if (FuserTable.contains("m_BossColor")) {
				m_BossColor = FuserTable["m_BossColor"].get<ImU32>();
			}
			if (FuserTable.contains("m_LootColor")) {
				m_LootColor = FuserTable["m_LootColor"].get<ImU32>();
			}
			if (FuserTable.contains("m_ContainerColor")) {
				m_ContainerColor = FuserTable["m_ContainerColor"].get<ImU32>();
			}
			if (FuserTable.contains("m_ExfilColor")) {
				m_ExfilColor = FuserTable["m_ExfilColor"].get<ImU32>();
			}
			if (FuserTable.contains("m_WeaponTextColor")) {
				m_WeaponTextColor = FuserTable["m_WeaponTextColor"].get<ImU32>();
			}
		}
	}

	if (j.contains("ESP")) {
		const auto& espTable = j["ESP"];
		if (espTable.contains("Enemy")) {
			const auto& enemy = espTable["Enemy"];
			if (enemy.contains("bBoxEnabled")) ESPSettings::Enemy::bBoxEnabled = enemy["bBoxEnabled"].get<bool>();
			if (enemy.contains("boxStyle")) ESPSettings::Enemy::boxStyle = enemy["boxStyle"].get<int>();
			if (enemy.contains("boxColor")) ESPSettings::Enemy::boxColor = ImColor(enemy["boxColor"].get<uint32_t>());
			if (enemy.contains("boxThickness")) ESPSettings::Enemy::boxThickness = enemy["boxThickness"].get<float>();
			if (enemy.contains("bBoxFilled")) ESPSettings::Enemy::bBoxFilled = enemy["bBoxFilled"].get<bool>();
			if (enemy.contains("boxFillColor")) ESPSettings::Enemy::boxFillColor = ImColor(enemy["boxFillColor"].get<uint32_t>());
			if (enemy.contains("bSkeletonEnabled")) ESPSettings::Enemy::bSkeletonEnabled = enemy["bSkeletonEnabled"].get<bool>();
			if (enemy.contains("skeletonColor")) ESPSettings::Enemy::skeletonColor = ImColor(enemy["skeletonColor"].get<uint32_t>());
			if (enemy.contains("skeletonThickness")) ESPSettings::Enemy::skeletonThickness = enemy["skeletonThickness"].get<float>();
			if (enemy.contains("bBonesHead")) ESPSettings::Enemy::bBonesHead = enemy["bBonesHead"].get<bool>();
			if (enemy.contains("bBonesSpine")) ESPSettings::Enemy::bBonesSpine = enemy["bBonesSpine"].get<bool>();
			if (enemy.contains("bBonesArmsL")) ESPSettings::Enemy::bBonesArmsL = enemy["bBonesArmsL"].get<bool>();
			if (enemy.contains("bBonesArmsR")) ESPSettings::Enemy::bBonesArmsR = enemy["bBonesArmsR"].get<bool>();
			if (enemy.contains("bBonesLegsL")) ESPSettings::Enemy::bBonesLegsL = enemy["bBonesLegsL"].get<bool>();
			if (enemy.contains("bBonesLegsR")) ESPSettings::Enemy::bBonesLegsR = enemy["bBonesLegsR"].get<bool>();
			if (enemy.contains("bNameEnabled")) ESPSettings::Enemy::bNameEnabled = enemy["bNameEnabled"].get<bool>();
			if (enemy.contains("nameColor")) ESPSettings::Enemy::nameColor = ImColor(enemy["nameColor"].get<uint32_t>());
			if (enemy.contains("bDistanceEnabled")) ESPSettings::Enemy::bDistanceEnabled = enemy["bDistanceEnabled"].get<bool>();
			if (enemy.contains("distanceColor")) ESPSettings::Enemy::distanceColor = ImColor(enemy["distanceColor"].get<uint32_t>());
			if (enemy.contains("bHealthEnabled")) ESPSettings::Enemy::bHealthEnabled = enemy["bHealthEnabled"].get<bool>();
			if (enemy.contains("bWeaponEnabled")) ESPSettings::Enemy::bWeaponEnabled = enemy["bWeaponEnabled"].get<bool>();
			if (enemy.contains("weaponColor")) ESPSettings::Enemy::weaponColor = ImColor(enemy["weaponColor"].get<uint32_t>());
			if (enemy.contains("bHeadDotEnabled")) ESPSettings::Enemy::bHeadDotEnabled = enemy["bHeadDotEnabled"].get<bool>();
			if (enemy.contains("headDotColor")) ESPSettings::Enemy::headDotColor = ImColor(enemy["headDotColor"].get<uint32_t>());
			if (enemy.contains("headDotRadius")) ESPSettings::Enemy::headDotRadius = enemy["headDotRadius"].get<float>();
		}
		if (espTable.contains("RenderRange")) {
			const auto& range = espTable["RenderRange"];
			// Player ranges
			if (range.contains("fPMCRange")) ESPSettings::RenderRange::fPMCRange = range["fPMCRange"].get<float>();
			if (range.contains("fPlayerScavRange")) ESPSettings::RenderRange::fPlayerScavRange = range["fPlayerScavRange"].get<float>();
			if (range.contains("fBossRange")) ESPSettings::RenderRange::fBossRange = range["fBossRange"].get<float>();
			if (range.contains("fAIRange")) ESPSettings::RenderRange::fAIRange = range["fAIRange"].get<float>();
			// Player colors
			if (range.contains("PMCColor")) ESPSettings::RenderRange::PMCColor = ImColor(range["PMCColor"].get<uint32_t>());
			if (range.contains("PlayerScavColor")) ESPSettings::RenderRange::PlayerScavColor = ImColor(range["PlayerScavColor"].get<uint32_t>());
			if (range.contains("BossColor")) ESPSettings::RenderRange::BossColor = ImColor(range["BossColor"].get<uint32_t>());
			if (range.contains("AIColor")) ESPSettings::RenderRange::AIColor = ImColor(range["AIColor"].get<uint32_t>());
			// Item ranges
			if (range.contains("fItemHighRange")) ESPSettings::RenderRange::fItemHighRange = range["fItemHighRange"].get<float>();
			if (range.contains("fItemMediumRange")) ESPSettings::RenderRange::fItemMediumRange = range["fItemMediumRange"].get<float>();
			if (range.contains("fItemLowRange")) ESPSettings::RenderRange::fItemLowRange = range["fItemLowRange"].get<float>();
			if (range.contains("fItemRestRange")) ESPSettings::RenderRange::fItemRestRange = range["fItemRestRange"].get<float>();
			// Item colors
			if (range.contains("ItemHighColor")) ESPSettings::RenderRange::ItemHighColor = ImColor(range["ItemHighColor"].get<uint32_t>());
			if (range.contains("ItemMediumColor")) ESPSettings::RenderRange::ItemMediumColor = ImColor(range["ItemMediumColor"].get<uint32_t>());
			if (range.contains("ItemLowColor")) ESPSettings::RenderRange::ItemLowColor = ImColor(range["ItemLowColor"].get<uint32_t>());
			if (range.contains("ItemRestColor")) ESPSettings::RenderRange::ItemRestColor = ImColor(range["ItemRestColor"].get<uint32_t>());
			// Container range + color
			if (range.contains("fContainerRange")) ESPSettings::RenderRange::fContainerRange = range["fContainerRange"].get<float>();
			if (range.contains("ContainerColor")) ESPSettings::RenderRange::ContainerColor = ImColor(range["ContainerColor"].get<uint32_t>());
			// Exfil range + color
			if (range.contains("fExfilRange")) ESPSettings::RenderRange::fExfilRange = range["fExfilRange"].get<float>();
			if (range.contains("ExfilColor")) ESPSettings::RenderRange::ExfilColor = ImColor(range["ExfilColor"].get<uint32_t>());
		}
	}

	if (j.contains("Optic")) {
		const auto& optic = j["Optic"];
		if (optic.contains("bOpticESP")) DrawESPPlayers::bOpticESP = optic["bOpticESP"].get<bool>();
		// Removed: m_OpticIndex, fOpticRadius - now auto-managed
	}

	if (j.contains("Tables")) {
		const auto& tables = j["Tables"];
		if (tables.contains("PlayerTable")) PlayerTable::bMasterToggle = tables["PlayerTable"].get<bool>();
		if (tables.contains("ItemTable")) ItemTable::bMasterToggle = tables["ItemTable"].get<bool>();
	}

	if (j.contains("FleaBot")) {
		const auto& fb = j["FleaBot"];
		if (fb.contains("bMasterToggle")) FleaBot::bMasterToggle = fb["bMasterToggle"].get<bool>();
		if (fb.contains("bLimitBuy")) FleaBot::bLimitBuy = fb["bLimitBuy"].get<bool>();
		if (fb.contains("bCycleBuy")) FleaBot::bCycleBuy = fb["bCycleBuy"].get<bool>();
		if (fb.contains("m_ItemCount")) FleaBot::m_ItemCount = fb["m_ItemCount"].get<uint32_t>();
		if (fb.contains("TimeoutMs")) FleaBot::TimeoutDuration = std::chrono::milliseconds(fb["TimeoutMs"].get<int64_t>());
		if (fb.contains("CycleDelayMs")) FleaBot::CycleDelay = std::chrono::milliseconds(fb["CycleDelayMs"].get<int64_t>());
		if (fb.contains("SuperCycleDelayMs")) FleaBot::SuperCycleDelay = std::chrono::milliseconds(fb["SuperCycleDelayMs"].get<int64_t>());
	}

	// Deserialize LootFilter settings
	if (j.contains("LootFilter")) {
		const auto& lf = j["LootFilter"];
		if (lf.contains("SelectedFilter")) LootFilter::SelectedFilterName = lf["SelectedFilter"].get<std::string>();
		if (lf.contains("SearchString")) LootFilter::SearchString = lf["SearchString"].get<std::string>();
		if (lf.contains("ShowMeds")) LootFilter::ShowMeds = lf["ShowMeds"].get<bool>();
		if (lf.contains("ShowFood")) LootFilter::ShowFood = lf["ShowFood"].get<bool>();
		if (lf.contains("ShowBackpacks")) LootFilter::ShowBackpacks = lf["ShowBackpacks"].get<bool>();
		if (lf.contains("ShowKeys")) LootFilter::ShowKeys = lf["ShowKeys"].get<bool>();
		if (lf.contains("ShowBarter")) LootFilter::ShowBarter = lf["ShowBarter"].get<bool>();
		if (lf.contains("ShowWeapons")) LootFilter::ShowWeapons = lf["ShowWeapons"].get<bool>();
		if (lf.contains("ShowAmmo")) LootFilter::ShowAmmo = lf["ShowAmmo"].get<bool>();
		if (lf.contains("ShowAll")) LootFilter::ShowAll = lf["ShowAll"].get<bool>();
		if (lf.contains("MinValueRegular")) LootFilter::MinValueRegular = lf["MinValueRegular"].get<int32_t>();
		if (lf.contains("MinValueValuable")) LootFilter::MinValueValuable = lf["MinValueValuable"].get<int32_t>();
		if (lf.contains("PricePerSlot")) LootFilter::PricePerSlot = lf["PricePerSlot"].get<bool>();
		if (lf.contains("UseFleaPrice")) LootFilter::UseFleaPrice = lf["UseFleaPrice"].get<bool>();
		if (lf.contains("MinPrice")) LootFilter::MinPrice = lf["MinPrice"].get<int32_t>();
		if (lf.contains("MaxPrice")) LootFilter::MaxPrice = lf["MaxPrice"].get<int32_t>();
		if (lf.contains("ShowQuestItems")) LootFilter::ShowQuestItems = lf["ShowQuestItems"].get<bool>();
		if (lf.contains("HighlightQuestItems")) LootFilter::HighlightQuestItems = lf["HighlightQuestItems"].get<bool>();

		// Deserialize filter presets
		if (lf.contains("FilterPresets")) {
			LootFilter::FilterPresets.clear();
			const auto& presets = lf["FilterPresets"];
			for (auto it = presets.begin(); it != presets.end(); ++it) {
				const std::string presetName = it.key();
				const auto& presetData = it.value();

				UserLootFilter filter(presetName);
				if (presetData.contains("defaultImportantColor"))
					filter.defaultImportantColor = presetData["defaultImportantColor"].get<ImU32>();

				if (presetData.contains("entries") && presetData["entries"].is_array()) {
					for (const auto& entryJson : presetData["entries"]) {
						LootFilterEntry entry;
						if (entryJson.contains("itemId")) entry.itemId = entryJson["itemId"].get<std::string>();
						if (entryJson.contains("itemName")) entry.itemName = entryJson["itemName"].get<std::string>();
						if (entryJson.contains("type")) entry.type = static_cast<LootFilterEntryType>(entryJson["type"].get<int>());
						if (entryJson.contains("enabled")) entry.enabled = entryJson["enabled"].get<bool>();
						if (entryJson.contains("customColor")) entry.customColor = entryJson["customColor"].get<ImU32>();
						if (entryJson.contains("comment")) entry.comment = entryJson["comment"].get<std::string>();
						filter.entries.push_back(entry);
					}
				}

				LootFilter::FilterPresets[presetName] = std::move(filter);
			}
		}

		// Ensure default filter exists
		LootFilter::EnsureDefaultFilter();
	}

	if (j.contains("Keybinds")) {
		DeserializeKeybinds(j["Keybinds"]);
	}
}



void Config::SaveConfig(const std::string& configName) {
	std::println("[Config] Saving config: {}", configName);
	json j = SerializeConfig();
	std::ofstream file(getConfigPath(configName));
	if (!file.is_open())
		return;
	file << std::setw(4) << j;
	file.close();
}

bool Config::LoadConfig(const std::string& configName) {
	std::println("[Config] Loading config: {}", configName);
	std::ifstream file(getConfigPath(configName));
	if (!file.is_open())
	{
		std::println("[Config] Failed to open config file: {}", getConfigPath(configName));
		return false;
	}
	json j;
	try {
		file >> j;
	}
	catch (...) {
		file.close();
		return false;
	}
	file.close();
	DeserializeConfig(j);
	return true;
}
