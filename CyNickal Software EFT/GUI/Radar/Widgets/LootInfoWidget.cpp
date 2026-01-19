#include "pch.h"
#include "GUI/Radar/Widgets/LootInfoWidget.h"
#include "GUI/LootFilter/LootFilter.h"
#include "GUI/Color Picker/Color Picker.h"
#include "Game/EFT.h"
#include "Game/Classes/CObservedLootItem/CObservedLootItem.h"
#include "Game/Classes/CLootList/CLootList.h"
#include "Database/ItemDatabase.h"
#include "DebugMode.h"
#include <algorithm>
#include <cmath>

// Helper struct for sorting loot
struct LootSortEntry
{
	const CObservedLootItem* item = nullptr;
	int32_t price = 0;
	float distance = 0.0f;
};

void LootInfoWidget::Render()
{
	if (!EFT::IsInRaid() && !DebugMode::IsDebugMode())
	{
		ImGui::TextDisabled("Not in raid");
		return;
	}

	// Update highlight timer
	if (HighlightedItemAddr != 0)
	{
		HighlightTimer += ImGui::GetIO().DeltaTime;
		if (HighlightTimer > 3.0f) // 3 second highlight duration
		{
			ClearHighlight();
		}
	}

	// Get local player position
	Vector3 localPos = EFT::GetRegisteredPlayers().GetLocalPlayerPosition();

	// Collect all visible loot with prices
	std::vector<LootSortEntry> entries;

	{
		auto& lootList = EFT::GetLootList();
		auto& observedItems = lootList.m_ObservedItems;
		std::scoped_lock lock(observedItems.m_Mut);

		for (const auto& item : observedItems.m_Entities)
		{
			if (item.IsInvalid())
				continue;

			// Skip items that shouldn't be shown (blacklisted, below threshold, etc.)
			if (!LootFilter::ShouldShow(item))
				continue;

			LootSortEntry entry;
			entry.item = &item;
			entry.price = LootFilter::GetEffectivePrice(item);
			entry.distance = item.m_Position.DistanceTo(localPos);

			entries.push_back(entry);
		}
	}

	// Sort by value or distance
	if (SortByValue)
	{
		std::sort(entries.begin(), entries.end(),
			[](const LootSortEntry& a, const LootSortEntry& b)
			{
				return a.price > b.price; // Descending by value
			});
	}
	else
	{
		std::sort(entries.begin(), entries.end(),
			[](const LootSortEntry& a, const LootSortEntry& b)
			{
				return a.distance < b.distance; // Ascending by distance
			});
	}

	// Limit to max display
	if (entries.size() > static_cast<size_t>(MaxDisplayItems))
		entries.resize(MaxDisplayItems);

	// Display count and total value
	int32_t totalValue = 0;
	for (const auto& entry : entries)
		totalValue += entry.price;

	ImGui::Text("Loot: %zu | Total: %dk", entries.size(), totalValue / 1000);
	ImGui::Separator();

	// Table
	int numCols = 1;
	if (ShowPrice) numCols++;
	if (ShowDistance) numCols++;

	if (ImGui::BeginTable("LootTable", numCols, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
		ImVec2(0, ImGui::GetContentRegionAvail().y)))
	{
		ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
		if (ShowPrice)
			ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		if (ShowDistance)
			ImGui::TableSetupColumn("Dist", ImGuiTableColumnFlags_WidthFixed, 50.0f);
		ImGui::TableHeadersRow();

		int index = 0;
		for (const auto& entry : entries)
		{
			if (entry.item)
				RenderLootRow(*entry.item, localPos, index++);
		}

		ImGui::EndTable();
	}
}

void LootInfoWidget::RenderLootRow(const CObservedLootItem& item, const Vector3& localPos, int index)
{
	ImGui::TableNextRow();

	float distance = item.m_Position.DistanceTo(localPos);
	int32_t price = LootFilter::GetEffectivePrice(item);
	ImU32 color = LootFilter::GetItemColor(item);
	bool isHighlighted = HighlightedItemAddr == item.m_EntityAddress;

	// Item name column
	ImGui::TableSetColumnIndex(0);

	// Apply highlight effect
	if (isHighlighted)
	{
		float pulse = 0.5f + 0.5f * sinf(HighlightTimer * 6.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, pulse, 1.0f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
	}

	// Get item name from database
	std::string itemName = item.GetName();
	if (itemName.empty())
	{
		const ItemData* data = ItemDatabase::GetItem(item.GetTemplateId());
		if (data)
			itemName = data->shortName.empty() ? data->name : data->shortName;
		else
			itemName = "Unknown";
	}

	// Truncate if too long
	if (itemName.length() > 20)
		itemName = itemName.substr(0, 18) + "..";

	std::string selectableId = itemName + "##loot" + std::to_string(index);
	if (ImGui::Selectable(selectableId.c_str(), isHighlighted, ImGuiSelectableFlags_SpanAllColumns))
	{
		// Click to highlight on radar
		HighlightedItemAddr = item.m_EntityAddress;
		HighlightTimer = 0.0f;
	}
	ImGui::PopStyleColor();

	// Price column
	if (ShowPrice)
	{
		ImGui::TableSetColumnIndex(1);
		if (price >= 1000000)
			ImGui::Text("%.1fM", price / 1000000.0f);
		else if (price >= 1000)
			ImGui::Text("%dk", price / 1000);
		else
			ImGui::Text("%d", price);
	}

	// Distance column
	if (ShowDistance)
	{
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%dm", static_cast<int>(distance));
	}
}

float LootInfoWidget::GetHighlightIntensity()
{
	if (HighlightedItemAddr == 0)
		return 0.0f;

	// Pulsing intensity
	return 0.5f + 0.5f * sinf(HighlightTimer * 6.0f);
}

json LootInfoWidget::Serialize() const
{
	json j = RadarWidget::Serialize();
	j["MaxDisplayItems"] = MaxDisplayItems;
	j["SortByValue"] = SortByValue;
	j["ShowDistance"] = ShowDistance;
	j["ShowPrice"] = ShowPrice;
	return j;
}

void LootInfoWidget::Deserialize(const json& j)
{
	RadarWidget::Deserialize(j);
	if (j.contains("MaxDisplayItems")) MaxDisplayItems = j["MaxDisplayItems"].get<int>();
	if (j.contains("SortByValue")) SortByValue = j["SortByValue"].get<bool>();
	if (j.contains("ShowDistance")) ShowDistance = j["ShowDistance"].get<bool>();
	if (j.contains("ShowPrice")) ShowPrice = j["ShowPrice"].get<bool>();
}
