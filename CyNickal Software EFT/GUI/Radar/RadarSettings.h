#pragma once
#include "../../Dependencies/nlohmann/json.hpp"

using json = nlohmann::json;

// Centralized radar settings structure
// Replaces scattered static inline variables in Radar2D class
//
// Purpose: Clean serialization interface for Config.cpp
// Benefits:
//   - All settings in one place
//   - Easy to serialize/deserialize
//   - Clearer ownership and lifecycle
//   - Simpler to test and validate
struct RadarSettings
{
	// === Visibility Settings ===
	bool bEnabled = true;
	bool bShowPlayers = true;
	bool bShowLoot = true;
	bool bShowExfils = true;
	bool bShowLocalPlayer = true;
	bool bShowMapImage = true;
	bool bShowQuestMarkers = true;
	bool bAutoFloorSwitch = true;
	bool bAutoMap = true;

	// === Rendering Settings ===
	// Zoom: 1-200, where 100 = 1:1, lower = zoomed in, higher = zoomed out
	int iZoom = 100;
	static constexpr int MIN_ZOOM = 1;
	static constexpr int MAX_ZOOM = 200;

	// Icon sizes (in pixels)
	float fPlayerIconSize = 8.0f;
	float fLootIconSize = 4.0f;
	float fExfilIconSize = 10.0f;

	// Current floor for multi-level maps (0-based index)
	int iCurrentFloor = 0;

	// === Widget and Focus Settings ===
	bool bShowGroupLines = true;       // Draw lines between group members
	bool bShowFocusHighlight = true;   // Highlight focused player
	bool bShowHoverTooltip = true;     // Show tooltip on hover

	// === Distance Filters ===
	float fMaxPlayerDistance = 500.0f; // Max distance for player rendering (meters)
	float fMaxLootDistance = 100.0f;   // Max distance for loot rendering (meters)

	// Serialization methods for Config.cpp integration
	json Serialize() const
	{
		return json{
			{"enabled", bEnabled},
			{"showPlayers", bShowPlayers},
			{"showLoot", bShowLoot},
			{"showExfils", bShowExfils},
			{"showLocalPlayer", bShowLocalPlayer},
			{"showMapImage", bShowMapImage},
			{"showQuestMarkers", bShowQuestMarkers},
			{"autoFloorSwitch", bAutoFloorSwitch},
			{"autoMap", bAutoMap},
			{"zoom", iZoom},
			{"playerIconSize", fPlayerIconSize},
			{"lootIconSize", fLootIconSize},
			{"exfilIconSize", fExfilIconSize},
			{"currentFloor", iCurrentFloor},
			{"showGroupLines", bShowGroupLines},
			{"showFocusHighlight", bShowFocusHighlight},
			{"showHoverTooltip", bShowHoverTooltip},
			{"maxPlayerDistance", fMaxPlayerDistance},
			{"maxLootDistance", fMaxLootDistance}
		};
	}

	void Deserialize(const json& j)
	{
		if (j.contains("enabled")) bEnabled = j["enabled"];
		if (j.contains("showPlayers")) bShowPlayers = j["showPlayers"];
		if (j.contains("showLoot")) bShowLoot = j["showLoot"];
		if (j.contains("showExfils")) bShowExfils = j["showExfils"];
		if (j.contains("showLocalPlayer")) bShowLocalPlayer = j["showLocalPlayer"];
		if (j.contains("showMapImage")) bShowMapImage = j["showMapImage"];
		if (j.contains("showQuestMarkers")) bShowQuestMarkers = j["showQuestMarkers"];
		if (j.contains("autoFloorSwitch")) bAutoFloorSwitch = j["autoFloorSwitch"];
		if (j.contains("autoMap")) bAutoMap = j["autoMap"];
		if (j.contains("zoom")) iZoom = j["zoom"];
		if (j.contains("playerIconSize")) fPlayerIconSize = j["playerIconSize"];
		if (j.contains("lootIconSize")) fLootIconSize = j["lootIconSize"];
		if (j.contains("exfilIconSize")) fExfilIconSize = j["exfilIconSize"];
		if (j.contains("currentFloor")) iCurrentFloor = j["currentFloor"];
		if (j.contains("showGroupLines")) bShowGroupLines = j["showGroupLines"];
		if (j.contains("showFocusHighlight")) bShowFocusHighlight = j["showFocusHighlight"];
		if (j.contains("showHoverTooltip")) bShowHoverTooltip = j["showHoverTooltip"];
		if (j.contains("maxPlayerDistance")) fMaxPlayerDistance = j["maxPlayerDistance"];
		if (j.contains("maxLootDistance")) fMaxLootDistance = j["maxLootDistance"];

		// Clamp zoom to valid range
		iZoom = std::clamp(iZoom, MIN_ZOOM, MAX_ZOOM);
	}
};
