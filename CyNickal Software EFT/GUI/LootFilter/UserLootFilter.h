#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "imgui.h"
#include "GUI/LootFilter/LootFilterEntry.h"

// Named filter preset containing important and blacklisted items
struct UserLootFilter
{
	std::string name;             // Filter preset name
	ImU32 defaultImportantColor;  // Default color for important items (0 = Turquoise)
	std::vector<LootFilterEntry> entries;

	UserLootFilter() : name("default"), defaultImportantColor(IM_COL32(64, 224, 208, 255)) {}

	explicit UserLootFilter(const std::string& filterName)
		: name(filterName), defaultImportantColor(IM_COL32(64, 224, 208, 255)) {}

	// Check if an item exists in this filter
	bool HasItem(const std::string& itemId) const
	{
		return std::any_of(entries.begin(), entries.end(),
			[&itemId](const LootFilterEntry& e) { return e.itemId == itemId; });
	}

	// Get entry for an item (nullptr if not found)
	const LootFilterEntry* GetEntry(const std::string& itemId) const
	{
		auto it = std::find_if(entries.begin(), entries.end(),
			[&itemId](const LootFilterEntry& e) { return e.itemId == itemId; });
		return (it != entries.end()) ? &(*it) : nullptr;
	}

	// Get mutable entry for an item (nullptr if not found)
	LootFilterEntry* GetEntryMutable(const std::string& itemId)
	{
		auto it = std::find_if(entries.begin(), entries.end(),
			[&itemId](const LootFilterEntry& e) { return e.itemId == itemId; });
		return (it != entries.end()) ? &(*it) : nullptr;
	}

	// Check if item is marked as important
	bool IsImportant(const std::string& itemId) const
	{
		const auto* entry = GetEntry(itemId);
		return entry && entry->enabled && entry->type == LootFilterEntryType::ImportantLoot;
	}

	// Check if item is blacklisted
	bool IsBlacklisted(const std::string& itemId) const
	{
		const auto* entry = GetEntry(itemId);
		return entry && entry->enabled && entry->type == LootFilterEntryType::BlacklistedLoot;
	}

	// Add an important item
	void AddImportant(const std::string& itemId, const std::string& itemName)
	{
		if (HasItem(itemId))
		{
			// Update existing entry
			auto* entry = GetEntryMutable(itemId);
			if (entry)
			{
				entry->type = LootFilterEntryType::ImportantLoot;
				entry->enabled = true;
			}
		}
		else
		{
			entries.emplace_back(itemId, itemName, LootFilterEntryType::ImportantLoot);
		}
	}

	// Add a blacklisted item
	void AddBlacklist(const std::string& itemId, const std::string& itemName)
	{
		if (HasItem(itemId))
		{
			// Update existing entry
			auto* entry = GetEntryMutable(itemId);
			if (entry)
			{
				entry->type = LootFilterEntryType::BlacklistedLoot;
				entry->enabled = true;
			}
		}
		else
		{
			entries.emplace_back(itemId, itemName, LootFilterEntryType::BlacklistedLoot);
		}
	}

	// Remove an item from the filter
	void RemoveItem(const std::string& itemId)
	{
		entries.erase(
			std::remove_if(entries.begin(), entries.end(),
				[&itemId](const LootFilterEntry& e) { return e.itemId == itemId; }),
			entries.end());
	}

	// Get all important items (returns copies to avoid dangling pointers if entries vector is modified)
	std::vector<LootFilterEntry> GetImportantItems() const
	{
		std::vector<LootFilterEntry> result;
		for (const auto& entry : entries)
		{
			if (entry.enabled && entry.type == LootFilterEntryType::ImportantLoot)
				result.push_back(entry);  // Copy, not pointer
		}
		return result;
	}

	// Get all blacklisted items (returns copies to avoid dangling pointers if entries vector is modified)
	std::vector<LootFilterEntry> GetBlacklistedItems() const
	{
		std::vector<LootFilterEntry> result;
		for (const auto& entry : entries)
		{
			if (entry.enabled && entry.type == LootFilterEntryType::BlacklistedLoot)
				result.push_back(entry);  // Copy, not pointer
		}
		return result;
	}
};
