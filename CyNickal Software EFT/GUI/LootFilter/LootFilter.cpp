#include "pch.h"
#include "GUI/LootFilter/LootFilter.h"
#include "Database/ItemDatabase.h"
#include "GUI/Color Picker/Color Picker.h"
#include "GUI/ESP/ESPSettings.h"
#include "Game/Classes/Quest/CQuestManager.h"
#include <algorithm>
#include <cctype>

// === File-scope variables for item search UI ===
static char s_ItemSearchBuffer[256] = "";
static std::vector<const ItemData*> s_SearchResults;
static int s_SelectedSearchResult = -1;

// === Color constants for value tiers ===
namespace LootColors
{
	constexpr ImU32 QuestItem = IM_COL32(186, 85, 255, 255);      // Purple - quest items
	constexpr ImU32 Important = IM_COL32(64, 224, 208, 255);      // Turquoise - user-marked important
	constexpr ImU32 Valuable = IM_COL32(255, 215, 0, 255);        // Gold - very valuable (>MinValueValuable)
	constexpr ImU32 HighValue = IM_COL32(186, 85, 211, 255);      // Purple - high value
	constexpr ImU32 MediumValue = IM_COL32(65, 105, 225, 255);    // Blue - medium value
	constexpr ImU32 LowValue = IM_COL32(50, 205, 50, 255);        // Green - low value
	constexpr ImU32 Normal = IM_COL32(255, 255, 255, 255);        // White - normal
	constexpr ImU32 BelowThreshold = IM_COL32(169, 169, 169, 255);// Gray - below threshold
}

// === Initialization ===

bool LootFilter::Initialize()
{
	if (s_initialized)
		return true;

	// Get executable directory
	char exePath[MAX_PATH];
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
	std::string dbPath = (exeDir / "Database" / "DEFAULT_DATA.json").string();

	if (!ItemDatabase::Initialize(dbPath))
	{
		printf("[LootFilter] Failed to initialize item database from: %s\n", dbPath.c_str());
		return false;
	}

	// Ensure default filter exists
	EnsureDefaultFilter();

	s_initialized = true;
	printf("[LootFilter] Initialized with %zu items in database\n", ItemDatabase::GetItemCount());
	return true;
}

// === Filter Preset Management ===

void LootFilter::EnsureDefaultFilter()
{
	if (FilterPresets.find("default") == FilterPresets.end())
	{
		FilterPresets["default"] = UserLootFilter("default");
	}
}

void LootFilter::CreateFilter(const std::string& name)
{
	if (name.empty() || FilterPresets.find(name) != FilterPresets.end())
		return;

	FilterPresets[name] = UserLootFilter(name);
}

void LootFilter::DeleteFilter(const std::string& name)
{
	// Don't delete the default filter
	if (name == "default")
		return;

	auto it = FilterPresets.find(name);
	if (it != FilterPresets.end())
	{
		FilterPresets.erase(it);
		// If we deleted the currently selected filter, switch to default
		if (SelectedFilterName == name)
			SelectedFilterName = "default";
	}
}

void LootFilter::RenameFilter(const std::string& oldName, const std::string& newName)
{
	if (oldName == "default" || newName.empty())
		return;

	auto it = FilterPresets.find(oldName);
	if (it == FilterPresets.end())
		return;

	if (FilterPresets.find(newName) != FilterPresets.end())
		return; // Name already exists

	UserLootFilter filter = std::move(it->second);
	filter.name = newName;
	FilterPresets.erase(it);
	FilterPresets[newName] = std::move(filter);

	if (SelectedFilterName == oldName)
		SelectedFilterName = newName;
}

UserLootFilter* LootFilter::GetCurrentFilter()
{
	EnsureDefaultFilter();
	auto it = FilterPresets.find(SelectedFilterName);
	if (it == FilterPresets.end())
	{
		SelectedFilterName = "default";
		it = FilterPresets.find("default");
	}
	return &it->second;
}

// === Entry Management ===

void LootFilter::AddImportantItem(const std::string& itemId, const std::string& itemName)
{
	auto* filter = GetCurrentFilter();
	if (filter)
		filter->AddImportant(itemId, itemName);
}

void LootFilter::AddBlacklistItem(const std::string& itemId, const std::string& itemName)
{
	auto* filter = GetCurrentFilter();
	if (filter)
		filter->AddBlacklist(itemId, itemName);
}

void LootFilter::RemoveItem(const std::string& itemId)
{
	auto* filter = GetCurrentFilter();
	if (filter)
		filter->RemoveItem(itemId);
}

void LootFilter::SetItemColor(const std::string& itemId, ImU32 color)
{
	auto* filter = GetCurrentFilter();
	if (!filter)
		return;

	auto* entry = filter->GetEntryMutable(itemId);
	if (entry)
		entry->customColor = color;
}

// === Enhanced Filtering Checks ===

bool LootFilter::IsImportant(const CObservedLootItem& item)
{
	auto* filter = GetCurrentFilter();
	if (!filter)
		return false;

	return filter->IsImportant(item.GetTemplateId());
}

bool LootFilter::IsBlacklisted(const CObservedLootItem& item)
{
	auto* filter = GetCurrentFilter();
	if (!filter)
		return false;

	return filter->IsBlacklisted(item.GetTemplateId());
}

bool LootFilter::IsValuable(const CObservedLootItem& item)
{
	int32_t price = GetEffectivePrice(item);
	return price >= MinValueValuable;
}

bool LootFilter::IsQuestItem(const CObservedLootItem& item)
{
	// Check if CQuestManager says this is a quest item we need
	auto& questMgr = Quest::CQuestManager::GetInstance();
	if (questMgr.IsQuestItem(item.GetTemplateId()))
		return true;

	// Also check the ItemDatabase category for static quest item flag
	const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
	if (data && data->IsQuestItem())
		return true;

	return false;
}

// === Price Calculations ===

int32_t LootFilter::GetEffectivePrice(const CObservedLootItem& item)
{
	const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
	if (!data)
		return 0;

	int32_t basePrice = UseFleaPrice ? data->fleaPrice : data->price;

	if (PricePerSlot && data->slots > 0)
		return basePrice / data->slots;

	return basePrice;
}

int32_t LootFilter::GetItemSlots(const CObservedLootItem& item)
{
	const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
	if (!data)
		return 1;

	return data->slots > 0 ? data->slots : 1;
}

// === Internal Filter Checks ===

bool LootFilter::MatchesSearch(const std::string& itemName)
{
	if (SearchString.empty())
		return true;

	// Split search string by comma for multiple search terms
	std::string search = SearchString;
	std::vector<std::string> terms;

	size_t pos = 0;
	while ((pos = search.find(',')) != std::string::npos)
	{
		std::string term = search.substr(0, pos);
		// Trim whitespace
		term.erase(0, term.find_first_not_of(" \t"));
		term.erase(term.find_last_not_of(" \t") + 1);
		if (!term.empty())
			terms.push_back(term);
		search.erase(0, pos + 1);
	}
	// Add last term
	search.erase(0, search.find_first_not_of(" \t"));
	search.erase(search.find_last_not_of(" \t") + 1);
	if (!search.empty())
		terms.push_back(search);

	// Case-insensitive search
	std::string lowerName = itemName;
	std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

	for (const auto& term : terms)
	{
		std::string lowerTerm = term;
		std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(), ::tolower);

		if (lowerName.find(lowerTerm) != std::string::npos)
			return true;
	}

	return false;
}

bool LootFilter::PassesPriceFilter(const CObservedLootItem& item)
{
	int32_t price = GetEffectivePrice(item);

	// Use MinValueRegular as the minimum threshold for display
	if (price < MinValueRegular)
		return false;

	// Also check legacy price bounds if set
	if (MinPrice > 0 && price < MinPrice)
		return false;
	if (MaxPrice < 999999999 && price > MaxPrice)
		return false;

	return true;
}

bool LootFilter::PassesCategoryFilter(const CObservedLootItem& item)
{
	if (ShowAll)
		return true;

	// Get item data from database
	const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
	if (!data)
		return true; // Unknown items pass by default

	// Check category toggles
	if (ShowMeds && data->IsMeds())
		return true;
	if (ShowFood && (data->IsFood() || data->IsDrink()))
		return true;
	if (ShowBackpacks && data->IsBackpack())
		return true;
	if (ShowKeys && data->IsKey())
		return true;
	if (ShowBarter && data->IsBarter())
		return true;
	if (ShowWeapons && data->IsWeapon())
		return true;
	if (ShowAmmo && data->IsAmmo())
		return true;

	// If no categories are enabled, show nothing (unless ShowAll is true)
	// But if at least one category is enabled and item doesn't match, hide it
	bool anyCategoryEnabled = ShowMeds || ShowFood || ShowBackpacks || ShowKeys ||
		ShowBarter || ShowWeapons || ShowAmmo;

	// If no categories enabled, show all
	return !anyCategoryEnabled;
}

// === Main Filter Function ===

bool LootFilter::ShouldShow(const CObservedLootItem& item)
{
	if (!s_initialized)
		Initialize();

	// Priority 1: Blacklisted items are always hidden
	if (IsBlacklisted(item))
		return false;

	// Priority 2: Important items are always shown
	if (IsImportant(item))
		return true;

	// Priority 3: Quest items (if enabled)
	if (ShowQuestItems && IsQuestItem(item))
		return true;

	// Check search filter first (search overrides category filters)
	if (!SearchString.empty())
	{
		const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
		if (data)
		{
			if (!MatchesSearch(data->name) && !MatchesSearch(data->shortName))
				return false;
		}
		else
		{
			// For unknown items, try matching against the item's internal name if available
			return false;
		}

		// If search matches, only apply price filter
		return PassesPriceFilter(item);
	}

	// Apply category and price filters
	return PassesCategoryFilter(item) && PassesPriceFilter(item);
}

// === Color Assignment ===
// Priority order:
// 1. Quest Items -> Purple (if ShowQuestItems enabled)
// 2. Important (user-marked) -> Custom color or Turquoise
// 3. Blacklisted -> Return early (don't render, handled in ShouldShow)
// 4. Price tiers based on value using ESPSettings colors

ImU32 LootFilter::GetItemColor(const CObservedLootItem& item)
{
	if (!s_initialized)
		Initialize();

	// Priority 1: Quest items
	if (ShowQuestItems && HighlightQuestItems && IsQuestItem(item))
		return LootColors::QuestItem;

	// Priority 2: User-marked important items
	if (IsImportant(item))
	{
		auto* filter = GetCurrentFilter();
		if (filter)
		{
			const auto* entry = filter->GetEntry(item.GetTemplateId());
			if (entry && entry->customColor != 0)
				return entry->customColor;
			return filter->defaultImportantColor;
		}
		return LootColors::Important;
	}

	// Get effective price for tier-based coloring
	int32_t price = GetEffectivePrice(item);

	// Price-based tiers using ESPSettings configurable colors
	using namespace ESPSettings::RenderRange;

	if (price >= MinValueValuable)                 // >100k (default)
		return ItemHighColor;
	if (price >= MinValueValuable / 2)             // >50k
		return ItemMediumColor;
	if (price >= MinValueRegular)                  // >20k
		return ItemLowColor;

	// Below threshold
	return ItemRestColor;
}

// === UI Rendering ===

void LootFilter::RenderSettings()
{
	if (ImGui::CollapsingHeader("Loot Filter"))
	{
		ImGui::Indent();

		// Filter preset selector
		RenderFilterPresetUI();

		ImGui::Separator();

		// Search box
		static char searchBuf[256] = "";
		if (ImGui::InputText("Search##LootSearch", searchBuf, sizeof(searchBuf)))
		{
			SearchString = searchBuf;
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear##SearchClear"))
		{
			searchBuf[0] = '\0';
			SearchString = "";
		}
		ImGui::TextDisabled("(Separate multiple items with commas)");

		ImGui::Separator();

		// Category toggles
		ImGui::Text("Categories:");
		ImGui::Checkbox("Show All##LF", &ShowAll);
		if (ShowAll)
			ImGui::BeginDisabled();

		ImGui::Columns(2, nullptr, false);
		ImGui::Checkbox("Meds##LF", &ShowMeds);
		ImGui::Checkbox("Food/Drink##LF", &ShowFood);
		ImGui::Checkbox("Backpacks##LF", &ShowBackpacks);
		ImGui::Checkbox("Keys##LF", &ShowKeys);
		ImGui::NextColumn();
		ImGui::Checkbox("Barter Items##LF", &ShowBarter);
		ImGui::Checkbox("Weapons##LF", &ShowWeapons);
		ImGui::Checkbox("Ammo##LF", &ShowAmmo);
		ImGui::Columns(1);

		if (ShowAll)
			ImGui::EndDisabled();

		ImGui::Separator();

		// Dual-tier price filter
		ImGui::Text("Price Thresholds:");
		ImGui::Checkbox("Use Flea Price##LF", &UseFleaPrice);
		ImGui::SameLine();
		ImGui::TextDisabled("(vs Trader)");

		ImGui::Checkbox("Price Per Slot##LF", &PricePerSlot);
		ImGui::SameLine();
		ImGui::TextDisabled("(normalize by inventory size)");

		ImGui::SliderInt("Min Value (Regular)##LF", &MinValueRegular, 0, 100000, "%d RUB");
		ImGui::SliderInt("Min Value (Valuable)##LF", &MinValueValuable, 50000, 500000, "%d RUB");

		if (ImGui::Button("Reset Price Thresholds##LF"))
		{
			MinValueRegular = 20000;
			MinValueValuable = 100000;
		}

		ImGui::Separator();

		// Quest items section
		ImGui::Text("Quest Items:");
		ImGui::Checkbox("Show Quest Items##LF", &ShowQuestItems);
		ImGui::SameLine();
		ImGui::Checkbox("Highlight##LF", &HighlightQuestItems);

		ImGui::Separator();

		// Important items management
		if (ImGui::TreeNode("Important Items"))
		{
			RenderImportantItemsUI();
			ImGui::TreePop();
		}

		// Blacklist management
		if (ImGui::TreeNode("Blacklisted Items"))
		{
			RenderBlacklistUI();
			ImGui::TreePop();
		}

		// Item search (for adding items)
		if (ImGui::TreeNode("Add Items by Search"))
		{
			RenderItemSearchUI();
			ImGui::TreePop();
		}

		ImGui::Separator();

		// Database info
		if (ItemDatabase::IsInitialized())
		{
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Database: %zu items loaded", ItemDatabase::GetItemCount());
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Database: Not loaded");
			if (ImGui::Button("Load Database"))
			{
				Initialize();
			}
		}

		ImGui::Unindent();
	}
}

void LootFilter::RenderFilterPresetUI()
{
	EnsureDefaultFilter();

	// Preset dropdown
	if (ImGui::BeginCombo("Filter Preset##LF", SelectedFilterName.c_str()))
	{
		for (const auto& [name, filter] : FilterPresets)
		{
			bool isSelected = (SelectedFilterName == name);
			if (ImGui::Selectable(name.c_str(), isSelected))
			{
				SelectedFilterName = name;
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// Preset management buttons
	static char newPresetName[64] = "";
	ImGui::InputText("##NewPresetName", newPresetName, sizeof(newPresetName));
	ImGui::SameLine();
	if (ImGui::Button("Create##LFPreset"))
	{
		if (strlen(newPresetName) > 0)
		{
			CreateFilter(newPresetName);
			SelectedFilterName = newPresetName;
			newPresetName[0] = '\0';
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete##LFPreset"))
	{
		if (SelectedFilterName != "default")
		{
			DeleteFilter(SelectedFilterName);
		}
	}
}

void LootFilter::RenderImportantItemsUI()
{
	auto* filter = GetCurrentFilter();
	if (!filter)
		return;

	auto importantItems = filter->GetImportantItems();

	if (importantItems.empty())
	{
		ImGui::TextDisabled("No important items added.");
	}
	else
	{
		// Display with table
		if (ImGui::BeginTable("ImportantItemsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableHeadersRow();

			std::vector<std::string> toRemove;
			for (const auto* entry : importantItems)
			{
				ImGui::TableNextRow();

				// Enabled toggle
				ImGui::TableSetColumnIndex(0);
				bool enabled = entry->enabled;
				std::string checkboxId = "##imp_en_" + entry->itemId;
				if (ImGui::Checkbox(checkboxId.c_str(), &enabled))
				{
					auto* mutableEntry = filter->GetEntryMutable(entry->itemId);
					if (mutableEntry)
						mutableEntry->enabled = enabled;
				}

				// Name
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", entry->itemName.c_str());

				// Color picker
				ImGui::TableSetColumnIndex(2);
				ImVec4 color = ImGui::ColorConvertU32ToFloat4(entry->customColor != 0 ? entry->customColor : filter->defaultImportantColor);
				std::string colorId = "##imp_col_" + entry->itemId;
				if (ImGui::ColorEdit4(colorId.c_str(), &color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
				{
					auto* mutableEntry = filter->GetEntryMutable(entry->itemId);
					if (mutableEntry)
						mutableEntry->customColor = ImGui::ColorConvertFloat4ToU32(color);
				}

				// Remove button
				ImGui::TableSetColumnIndex(3);
				std::string removeId = "X##imp_rm_" + entry->itemId;
				if (ImGui::Button(removeId.c_str()))
				{
					toRemove.push_back(entry->itemId);
				}
			}

			ImGui::EndTable();

			// Remove marked items
			for (const auto& id : toRemove)
			{
				filter->RemoveItem(id);
			}
		}
	}
}

void LootFilter::RenderBlacklistUI()
{
	auto* filter = GetCurrentFilter();
	if (!filter)
		return;

	auto blacklistedItems = filter->GetBlacklistedItems();

	if (blacklistedItems.empty())
	{
		ImGui::TextDisabled("No blacklisted items added.");
	}
	else
	{
		if (ImGui::BeginTable("BlacklistTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableHeadersRow();

			std::vector<std::string> toRemove;
			for (const auto* entry : blacklistedItems)
			{
				ImGui::TableNextRow();

				// Enabled toggle
				ImGui::TableSetColumnIndex(0);
				bool enabled = entry->enabled;
				std::string checkboxId = "##bl_en_" + entry->itemId;
				if (ImGui::Checkbox(checkboxId.c_str(), &enabled))
				{
					auto* mutableEntry = filter->GetEntryMutable(entry->itemId);
					if (mutableEntry)
						mutableEntry->enabled = enabled;
				}

				// Name
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", entry->itemName.c_str());

				// Remove button
				ImGui::TableSetColumnIndex(2);
				std::string removeId = "X##bl_rm_" + entry->itemId;
				if (ImGui::Button(removeId.c_str()))
				{
					toRemove.push_back(entry->itemId);
				}
			}

			ImGui::EndTable();

			// Remove marked items
			for (const auto& id : toRemove)
			{
				filter->RemoveItem(id);
			}
		}
	}
}

void LootFilter::RenderItemSearchUI()
{
	if (!ItemDatabase::IsInitialized())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Item database not loaded!");
		return;
	}

	// Search input
	bool searchChanged = ImGui::InputText("Search Items##AddSearch", s_ItemSearchBuffer, sizeof(s_ItemSearchBuffer));

	if (searchChanged && strlen(s_ItemSearchBuffer) >= 2)
	{
		// Update search results
		s_SearchResults.clear();
		s_SelectedSearchResult = -1;

		std::string searchLower = s_ItemSearchBuffer;
		std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

		const auto& allItems = ItemDatabase::GetAllItems();
		for (const auto& [id, item] : allItems)
		{
			std::string nameLower = item.name;
			std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

			std::string shortNameLower = item.shortName;
			std::transform(shortNameLower.begin(), shortNameLower.end(), shortNameLower.begin(), ::tolower);

			if (nameLower.find(searchLower) != std::string::npos ||
				shortNameLower.find(searchLower) != std::string::npos)
			{
				s_SearchResults.push_back(&item);
				if (s_SearchResults.size() >= 20) // Limit results
					break;
			}
		}
	}
	else if (strlen(s_ItemSearchBuffer) < 2)
	{
		s_SearchResults.clear();
		s_SelectedSearchResult = -1;
	}

	// Display search results
	if (!s_SearchResults.empty())
	{
		ImGui::Text("Results (%zu):", s_SearchResults.size());

		if (ImGui::BeginChild("SearchResults", ImVec2(0, 150), true))
		{
			for (size_t i = 0; i < s_SearchResults.size(); ++i)
			{
				const ItemData* item = s_SearchResults[i];
				std::string label = item->name + " (" + std::to_string(item->fleaPrice) + " RUB)";

				if (ImGui::Selectable(label.c_str(), s_SelectedSearchResult == static_cast<int>(i)))
				{
					s_SelectedSearchResult = static_cast<int>(i);
				}
			}
		}
		ImGui::EndChild();

		// Add buttons
		if (s_SelectedSearchResult >= 0 && s_SelectedSearchResult < static_cast<int>(s_SearchResults.size()))
		{
			const ItemData* selectedItem = s_SearchResults[s_SelectedSearchResult];

			if (ImGui::Button("Add to Important"))
			{
				AddImportantItem(selectedItem->bsgId, selectedItem->name);
			}
			ImGui::SameLine();
			if (ImGui::Button("Add to Blacklist"))
			{
				AddBlacklistItem(selectedItem->bsgId, selectedItem->name);
			}
		}
	}
	else if (strlen(s_ItemSearchBuffer) >= 2)
	{
		ImGui::TextDisabled("No items found.");
	}
	else
	{
		ImGui::TextDisabled("Type at least 2 characters to search.");
	}
}
