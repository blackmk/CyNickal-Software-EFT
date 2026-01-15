#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// Item data structure parsed from DEFAULT_DATA.json
struct ItemData
{
	std::string bsgId;
	std::string name;
	std::string shortName;
	int32_t price = 0;         // Trader/vendor price
	int32_t fleaPrice = 0;     // Flea market price
	int32_t slots = 1;         // Inventory slots
	std::vector<std::string> categories;

	// Category helpers
	bool IsMeds() const;
	bool IsFood() const;
	bool IsDrink() const;
	bool IsBackpack() const;
	bool IsWeapon() const;
	bool IsAmmo() const;
	bool IsKey() const;
	bool IsQuestItem() const;
	bool IsBarter() const;
	
	// Price helpers
	int32_t GetBestPrice() const { return (fleaPrice > price) ? fleaPrice : price; }
};

// Singleton database for all items
class ItemDatabase
{
public:
	static bool Initialize(const std::string& jsonPath);
	static const ItemData* GetItem(const std::string& bsgId);
	static const std::unordered_map<std::string, ItemData>& GetAllItems() { return s_items; }
	static bool IsInitialized() { return s_initialized; }
	static size_t GetItemCount() { return s_items.size(); }

private:
	static inline bool s_initialized = false;
	static inline std::unordered_map<std::string, ItemData> s_items;
};
