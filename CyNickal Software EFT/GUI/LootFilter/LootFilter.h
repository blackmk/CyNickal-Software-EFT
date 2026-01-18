#pragma once
#include <string>
#include <functional>
#include "Game/Classes/CObservedLootItem/CObservedLootItem.h"

// Loot filter entry types
enum class LootFilterEntryType
{
	ImportantLoot = 0,   // Highlight this item
	BlacklistedLoot = 1  // Hide this item
};

// Configuration for loot filtering
class LootFilter
{
public:
	// Global filter settings
	static inline std::string SearchString = "";
	static inline bool ShowMeds = true;
	static inline bool ShowFood = false;
	static inline bool ShowBackpacks = false;
	static inline bool ShowKeys = true;
	static inline bool ShowBarter = false;
	static inline bool ShowWeapons = false;
	static inline bool ShowAmmo = false;
	static inline bool ShowAll = false;  // Override: show everything
	
	// Price filtering
	static inline int32_t MinPrice = 0;
	static inline int32_t MaxPrice = 999999999;
	static inline bool UseFleaPrice = true;  // true = flea, false = trader

	// Initialize (loads item database if not already loaded)
	static bool Initialize();
	
	// Main filter function - returns true if item should be shown
	static bool ShouldShow(const CObservedLootItem& item);
	
	// Get display color for item based on value/category
	static ImU32 GetItemColor(const CObservedLootItem& item);
	
	// Render filter settings UI
	static void RenderSettings();

private:
	static inline bool s_initialized = false;
	
	// Check if item matches search string
	static bool MatchesSearch(const std::string& itemName);
	
	// Check if item passes price filter
	static bool PassesPriceFilter(const CObservedLootItem& item);
	
	// Check if item passes category filter
	static bool PassesCategoryFilter(const CObservedLootItem& item);
};
