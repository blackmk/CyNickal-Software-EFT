#pragma once
#include <string>
#include "imgui.h"

// Loot filter entry types
enum class LootFilterEntryType
{
	ImportantLoot = 0,   // Highlight this item
	BlacklistedLoot = 1  // Hide this item
};

// Per-item filter entry for important/blacklisted items
struct LootFilterEntry
{
	std::string itemId;           // BSG item ID
	std::string itemName;         // Display name (for UI)
	LootFilterEntryType type = LootFilterEntryType::ImportantLoot;  // ImportantLoot or BlacklistedLoot
	bool enabled = true;          // Toggle without removing
	ImU32 customColor = 0;        // Custom color (0 = inherit from tier)
	std::string comment;          // User notes

	LootFilterEntry() = default;

	LootFilterEntry(const std::string& id, const std::string& name, LootFilterEntryType entryType)
		: itemId(id), itemName(name), type(entryType), enabled(true), customColor(0) {}

	LootFilterEntry(const std::string& id, const std::string& name, LootFilterEntryType entryType, ImU32 color)
		: itemId(id), itemName(name), type(entryType), enabled(true), customColor(color) {}

	bool operator==(const LootFilterEntry& other) const
	{
		return itemId == other.itemId;
	}
};
