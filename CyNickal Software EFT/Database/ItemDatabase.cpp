#include "pch.h"
#include "Database/ItemDatabase.h"
#include "../../Dependencies/nlohmann/json.hpp"
#include <mutex>

using json = nlohmann::json;

// Thread-safe initialization
static std::once_flag s_initFlag;
static bool s_initResult = false;

// Category checking helpers
bool ItemData::IsMeds() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Meds" || cat == "Medikit" || cat == "Drug" || cat == "Medical item" || cat == "Medical supplies")
			return true;
	}
	return false;
}

bool ItemData::IsFood() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Food" || cat == "Food and drink")
			return true;
	}
	return false;
}

bool ItemData::IsDrink() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Drink" || cat == "Food and drink")
			return true;
	}
	return false;
}

bool ItemData::IsBackpack() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Backpack")
			return true;
	}
	return false;
}

bool ItemData::IsWeapon() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Weapon" || cat == "Assault rifle" || cat == "SMG" || cat == "Sniper rifle" || 
			cat == "Handgun" || cat == "Shotgun" || cat == "Marksman rifle" || cat == "Assault carbine")
			return true;
	}
	return false;
}

bool ItemData::IsAmmo() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Ammo" || cat == "Ammo container")
			return true;
	}
	return false;
}

bool ItemData::IsKey() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Key" || cat == "Mechanical Key")
			return true;
	}
	return false;
}

bool ItemData::IsQuestItem() const
{
	// Quest items typically have specific naming or are in "Special item" category
	// This is a simplified check - more sophisticated detection may be needed
	for (const auto& cat : categories)
	{
		if (cat == "Special item" || cat == "Quest item")
			return true;
	}
	return false;
}

bool ItemData::IsBarter() const
{
	for (const auto& cat : categories)
	{
		if (cat == "Barter item")
			return true;
	}
	return false;
}

bool ItemDatabase::Initialize(const std::string& jsonPath)
{
	std::call_once(s_initFlag, [&jsonPath]() {
		std::ifstream file(jsonPath);
		if (!file.is_open())
		{
			printf("[ItemDatabase] Failed to open: %s\n", jsonPath.c_str());
			s_initResult = false;
			return;
		}

		try
		{
			json data = json::parse(file);
			
			if (!data.contains("items") || !data["items"].is_array())
			{
				printf("[ItemDatabase] Invalid JSON format - missing 'items' array\n");
				s_initResult = false;
				return;
			}

			for (const auto& item : data["items"])
			{
				ItemData itemData;
				
				if (item.contains("bsgID") && item["bsgID"].is_string())
					itemData.bsgId = item["bsgID"].get<std::string>();
				else
					continue; // Skip items without valid ID
				
				if (item.contains("name") && item["name"].is_string())
					itemData.name = item["name"].get<std::string>();
				
				if (item.contains("shortName") && item["shortName"].is_string())
					itemData.shortName = item["shortName"].get<std::string>();
				
				if (item.contains("price") && item["price"].is_number())
					itemData.price = item["price"].get<int32_t>();
				
				if (item.contains("fleaPrice") && item["fleaPrice"].is_number())
					itemData.fleaPrice = item["fleaPrice"].get<int32_t>();
				
				if (item.contains("slots") && item["slots"].is_number())
					itemData.slots = item["slots"].get<int32_t>();
				
				if (item.contains("categories") && item["categories"].is_array())
				{
					for (const auto& cat : item["categories"])
					{
						if (cat.is_string())
							itemData.categories.push_back(cat.get<std::string>());
					}
				}

				s_items[itemData.bsgId] = std::move(itemData);
			}

			s_initialized = true;
			s_initResult = true;
			printf("[ItemDatabase] Loaded %zu items from database\n", s_items.size());
		}
		catch (const json::exception& e)
		{
			printf("[ItemDatabase] JSON parse error: %s\n", e.what());
			s_initResult = false;
		}
	});
	return s_initResult;
}

const ItemData* ItemDatabase::GetItem(const std::string& bsgId)
{
	auto it = s_items.find(bsgId);
	if (it != s_items.end())
		return &it->second;
	return nullptr;
}
