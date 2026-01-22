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

	// Collect all visible loot with prices - copy data inside lock scope
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
			// Copy all needed data to avoid dangling pointers
			entry.itemName = item.GetName();
			entry.templateId = item.GetTemplateId();
			entry.position = item.m_Position;
			entry.entityAddress = item.m_EntityAddress;
			entry.price = LootFilter::GetEffectivePrice(item);
			entry.distance = item.m_Position.DistanceTo(localPos);
			entry.color = LootFilter::GetItemColor(item);

			entries.push_back(std::move(entry));
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
			RenderLootRowFromEntry(entry, index++);
		}

		ImGui::EndTable();
	}
}

void LootInfoWidget::RenderLootRowFromEntry(const LootSortEntry& entry, int index)
{
	ImGui::TableNextRow();

	bool isHighlighted = HighlightedItemAddr == entry.entityAddress;

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
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(entry.color));
	}

	// Get item name from copied data or database
	std::string itemName = entry.itemName;
	if (itemName.empty())
	{
		const ItemData* data = ItemDatabase::GetItem(entry.templateId);
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
		HighlightedItemAddr = entry.entityAddress;
		HighlightTimer = 0.0f;
	}
	ImGui::PopStyleColor();

	// Price column
	if (ShowPrice)
	{
		ImGui::TableSetColumnIndex(1);
		if (entry.price >= 1000000)
			ImGui::Text("%.1fM", entry.price / 1000000.0f);
		else if (entry.price >= 1000)
			ImGui::Text("%dk", entry.price / 1000);
		else
			ImGui::Text("%d", entry.price);
	}

	// Distance column - use dynamic index based on which columns are present
	if (ShowDistance)
	{
		ImGui::TableSetColumnIndex(ShowPrice ? 2 : 1);
		ImGui::Text("%dm", static_cast<int>(entry.distance));
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
