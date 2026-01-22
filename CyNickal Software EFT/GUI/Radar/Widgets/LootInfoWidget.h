#pragma once
#include "GUI/Radar/Widgets/RadarWidget.h"
#include "Game/Classes/Vector.h"

// Forward declaration
class CObservedLootItem;

// Helper struct for sorting loot - stores copies of needed data for thread safety
struct LootSortEntry
{
	std::string itemName;
	std::string templateId;
	Vector3 position;
	uintptr_t entityAddress = 0;
	int32_t price = 0;
	float distance = 0.0f;
	ImU32 color = 0;
};

// Widget displaying top valuable loot items sorted by price
class LootInfoWidget : public RadarWidget
{
public:
	// Singleton instance
	static LootInfoWidget& GetInstance()
	{
		static LootInfoWidget instance;
		return instance;
	}

	// Settings
	static inline int MaxDisplayItems = 15;
	static inline bool SortByValue = true;   // true = by value, false = by distance
	static inline bool ShowDistance = true;
	static inline bool ShowPrice = true;

	void Render() override;

	// Serialize/Deserialize with additional settings
	json Serialize() const override;
	void Deserialize(const json& j) override;

private:
	LootInfoWidget() : RadarWidget("Top Loot", ImVec2(10, 270), ImVec2(300, 220)) {}
	
	// Render loot row from copied entry data (thread-safe)
	void RenderLootRowFromEntry(const LootSortEntry& entry, int index);

	// Highlighted item (for pulsing on radar)
	static inline uintptr_t HighlightedItemAddr = 0;
	static inline float HighlightTimer = 0.0f;

public:
	// Get highlighted item for radar rendering
	static uintptr_t GetHighlightedItem() { return HighlightedItemAddr; }
	static float GetHighlightIntensity();
	static void ClearHighlight() { HighlightedItemAddr = 0; HighlightTimer = 0.0f; }
};
