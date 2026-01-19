#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include "Game/Classes/CObservedLootItem/CObservedLootItem.h"
#include "GUI/LootFilter/LootFilterEntry.h"
#include "GUI/LootFilter/UserLootFilter.h"

// Configuration for loot filtering with dual-tier pricing and named filter presets
class LootFilter
{
public:
	// === Global filter settings ===
	static inline std::string SearchString = "";
	static inline bool ShowMeds = true;
	static inline bool ShowFood = false;
	static inline bool ShowBackpacks = false;
	static inline bool ShowKeys = true;
	static inline bool ShowBarter = false;
	static inline bool ShowWeapons = false;
	static inline bool ShowAmmo = false;
	static inline bool ShowAll = false;  // Override: show everything

	// === Dual-tier pricing system ===
	static inline int32_t MinValueRegular = 20000;      // Regular loot threshold (show items above this)
	static inline int32_t MinValueValuable = 100000;    // Valuable loot threshold (highlight gold)
	static inline bool PricePerSlot = false;            // Normalize price by inventory slots
	static inline bool UseFleaPrice = true;             // true = flea price, false = trader price

	// Legacy price filter (for compatibility with existing settings)
	static inline int32_t MinPrice = 0;
	static inline int32_t MaxPrice = 999999999;

	// === Named filter presets ===
	static inline std::string SelectedFilterName = "default";
	static inline std::unordered_map<std::string, UserLootFilter> FilterPresets;

	// === Quest item integration (Phase 3 placeholder) ===
	static inline bool ShowQuestItems = true;
	static inline bool HighlightQuestItems = true;

	// === Initialization ===
	static bool Initialize();

	// === Filter preset management ===
	static void CreateFilter(const std::string& name);
	static void DeleteFilter(const std::string& name);
	static void RenameFilter(const std::string& oldName, const std::string& newName);
	static UserLootFilter* GetCurrentFilter();
	static void EnsureDefaultFilter();

	// === Entry management (via current filter) ===
	static void AddImportantItem(const std::string& itemId, const std::string& itemName);
	static void AddBlacklistItem(const std::string& itemId, const std::string& itemName);
	static void RemoveItem(const std::string& itemId);
	static void SetItemColor(const std::string& itemId, ImU32 color);

	// === Enhanced filtering checks ===
	static bool IsImportant(const CObservedLootItem& item);   // User-marked as important
	static bool IsBlacklisted(const CObservedLootItem& item); // User-marked to hide
	static bool IsValuable(const CObservedLootItem& item);    // Above valuable threshold
	static bool IsQuestItem(const CObservedLootItem& item);   // Quest item (Phase 3)

	// === Price calculations ===
	static int32_t GetEffectivePrice(const CObservedLootItem& item);  // Handles price-per-slot
	static int32_t GetItemSlots(const CObservedLootItem& item);

	// === Main filter functions ===
	static bool ShouldShow(const CObservedLootItem& item);
	static ImU32 GetItemColor(const CObservedLootItem& item);

	// === UI Rendering ===
	static void RenderSettings();
	static void RenderFilterPresetUI();
	static void RenderImportantItemsUI();
	static void RenderBlacklistUI();
	static void RenderItemSearchUI();

private:
	static inline bool s_initialized = false;

	// Internal filter checks
	static bool MatchesSearch(const std::string& itemName);
	static bool PassesPriceFilter(const CObservedLootItem& item);
	static bool PassesCategoryFilter(const CObservedLootItem& item);
};
