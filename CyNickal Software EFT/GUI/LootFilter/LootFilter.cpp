#include "pch.h"
#include "GUI/LootFilter/LootFilter.h"
#include "Database/ItemDatabase.h"
#include "GUI/Color Picker/Color Picker.h"
#include <algorithm>
#include <cctype>

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

	s_initialized = true;
	printf("[LootFilter] Initialized with %zu items in database\n", ItemDatabase::GetItemCount());
	return true;
}

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
	// Get item data from database
	const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
	if (!data)
		return true; // Unknown items pass by default

	int32_t price = UseFleaPrice ? data->fleaPrice : data->price;
	return (price >= MinPrice && price <= MaxPrice);
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

bool LootFilter::ShouldShow(const CObservedLootItem& item)
{
	if (!s_initialized)
		Initialize();

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

ImU32 LootFilter::GetItemColor(const CObservedLootItem& item)
{
	const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
	if (!data)
		return IM_COL32(255, 255, 255, 255); // White for unknown items

	int32_t price = UseFleaPrice ? data->fleaPrice : data->price;

	// Color based on value tiers
	if (price >= 500000)  // 500k+ = Gold (extremely valuable)
		return IM_COL32(255, 215, 0, 255);
	if (price >= 100000)  // 100k+ = Purple (very valuable)
		return IM_COL32(186, 85, 211, 255);
	if (price >= 50000)   // 50k+ = Blue (valuable)
		return IM_COL32(65, 105, 225, 255);
	if (price >= 20000)   // 20k+ = Green (decent)
		return IM_COL32(50, 205, 50, 255);
	if (price >= 10000)   // 10k+ = White (normal)
		return IM_COL32(255, 255, 255, 255);
	
	// Low value = Gray
	return IM_COL32(169, 169, 169, 255);
}

void LootFilter::RenderSettings()
{
	if (ImGui::CollapsingHeader("Loot Filter"))
	{
		ImGui::Indent();

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

		// Price filter
		ImGui::Text("Price Filter:");
		ImGui::Checkbox("Use Flea Price##LF", &UseFleaPrice);
		ImGui::SameLine();
		ImGui::TextDisabled("(vs Trader)");
		
		ImGui::SliderInt("Min Price##LF", &MinPrice, 0, 100000, "%d RUB");
		ImGui::SliderInt("Max Price##LF", &MaxPrice, 0, 10000000, "%d RUB");
		
		if (ImGui::Button("Reset Price Filter##LF"))
		{
			MinPrice = 0;
			MaxPrice = 999999999;
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
