#include "pch.h"
#include "GUI/Item Table/Item Table.h"
#include "Game/EFT.h"
#include <chrono>
#include <mutex>
#include <print>

void ItemTable::Render()
{
	if (!bMasterToggle)	return;

	if (!EFT::pGameWorld || !EFT::pGameWorld->m_pLootList || !EFT::pGameWorld->m_pRegisteredPlayers)
		return;

	ImGui::Begin("Item Table", &bMasterToggle);

	ImGui::SetNextItemWidth(120.0f);
	ImGui::InputInt("##Price", &m_MinimumPrice, 1000, 10000);
	ImGui::SameLine();
	m_LootFilter.Draw("##ItemTableFilter", -FLT_MIN);

	ImGuiTableFlags TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoBordersInBody;
	if (ImGui::BeginTable("#ItemTable", 6, TableFlags))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Distance");
		ImGui::TableSetupColumn("Price");
		ImGui::TableSetupColumn("Total Slots");
		ImGui::TableSetupColumn("Price Per Slot");
		ImGui::TableSetupColumn("Stack Count");
		ImGui::TableHeadersRow();

		static auto LastBusyPlayerLog = std::chrono::steady_clock::now();
		static auto LastBusyLootLog = std::chrono::steady_clock::now();

		auto& PlayerList = EFT::GetRegisteredPlayers();
		auto PlayerLock = std::unique_lock<std::mutex>(PlayerList.m_Mut, std::try_to_lock);
		if (!PlayerLock.owns_lock())
		{
			auto Now = std::chrono::steady_clock::now();
			if (Now - LastBusyPlayerLog > std::chrono::seconds(1))
			{
				std::println("[ItemTable] Skipping render: player data busy");
				LastBusyPlayerLog = Now;
			}
			ImGui::EndTable();
			ImGui::End();
			return;
		}

		auto LocalPlayerPos = PlayerList.GetLocalPlayerPosition();
		auto& LootList = EFT::GetLootList();

		auto LootLock = std::unique_lock<std::mutex>(LootList.m_ObservedItems.m_Mut, std::try_to_lock);
		if (!LootLock.owns_lock())
		{
			auto Now = std::chrono::steady_clock::now();
			if (Now - LastBusyLootLog > std::chrono::seconds(1))
			{
				std::println("[ItemTable] Skipping render: loot data busy");
				LastBusyLootLog = Now;
			}
			ImGui::EndTable();
			ImGui::End();
			return;
		}

		for (auto& Item : LootList.m_ObservedItems.m_Entities)
			AddRow(Item, LocalPlayerPos);

		ImGui::EndTable();
	}

	ImGui::End();
}


void ItemTable::AddRow(const CObservedLootItem& Loot, const Vector3& LocalPlayerPos)
{
	if (Loot.IsInvalid())
		return;

	auto Price = Loot.GetItemPrice();
	if (Loot.GetItemPrice() < m_MinimumPrice)
		return;

	auto Name = Loot.GetName();
	if (m_LootFilter.IsActive() && !m_LootFilter.PassFilter(Name.c_str()))
		return;

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::Text(Name.c_str());
	ImGui::TableNextColumn();
	ImGui::Text("%.2f m", Loot.m_Position.DistanceTo(LocalPlayerPos));
	ImGui::TableNextColumn();
	ImGui::Text("%d", Price);
	ImGui::TableNextColumn();
	ImGui::Text("%d", Loot.GetSizeInSlots());
	ImGui::TableNextColumn();
	ImGui::Text("%.2f", Loot.GetPricePerSlot());
	ImGui::TableNextColumn();
	ImGui::Text("%d", Loot.GetStackCount());
}