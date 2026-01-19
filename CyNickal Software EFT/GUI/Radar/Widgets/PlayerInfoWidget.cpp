#include "pch.h"
#include "GUI/Radar/Widgets/PlayerInfoWidget.h"
#include "GUI/Radar/PlayerFocus.h"
#include "GUI/Color Picker/Color Picker.h"
#include "Game/EFT.h"
#include "Game/Classes/Players/CClientPlayer/CClientPlayer.h"
#include "Game/Classes/Players/CObservedPlayer/CObservedPlayer.h"
#include "Game/Classes/CRegisteredPlayers/CRegisteredPlayers.h"
#include "Game/Enums/EBoneIndex.h"
#include "DebugMode.h"
#include <algorithm>
#include <cmath>

// Helper struct for sorting players
struct PlayerSortEntry
{
	const CObservedPlayer* observed = nullptr;
	const CClientPlayer* client = nullptr;
	float distance = 0.0f;
	bool isObserved = true;
};

void PlayerInfoWidget::Render()
{
	if (!EFT::IsInRaid() && !DebugMode::IsDebugMode())
	{
		ImGui::TextDisabled("Not in raid");
		return;
	}

	// Get local player position
	Vector3 localPos = EFT::GetRegisteredPlayers().GetLocalPlayerPosition();

	// Collect all players with distances
	std::vector<PlayerSortEntry> entries;

	{
		auto& regPlayers = EFT::GetRegisteredPlayers();
		std::scoped_lock lock(regPlayers.m_Mut);

		for (const auto& playerVariant : regPlayers.m_Players)
		{
			std::visit([&](auto&& player)
			{
				using T = std::decay_t<decltype(player)>;

				if (player.IsInvalid())
					return;

				// Skip local player
				if (player.IsLocalPlayer())
					return;

				// Skip temp teammates
				if (PlayerFocus::IsTempTeammate(player.m_EntityAddress))
					return;

				float dist = player.GetBonePosition(EBoneIndex::Root).DistanceTo(localPos);

				PlayerSortEntry entry;
				entry.distance = dist;

				if constexpr (std::is_same_v<T, CObservedPlayer>)
				{
					entry.observed = &player;
					entry.isObserved = true;
				}
				else
				{
					entry.client = &player;
					entry.isObserved = false;
				}

				entries.push_back(entry);
			}, playerVariant);
		}
	}

	// Sort by distance if enabled
	if (SortByDistance)
	{
		std::sort(entries.begin(), entries.end(),
			[](const PlayerSortEntry& a, const PlayerSortEntry& b)
			{
				return a.distance < b.distance;
			});
	}

	// Limit to max display
	if (entries.size() > static_cast<size_t>(MaxDisplayPlayers))
		entries.resize(MaxDisplayPlayers);

	// Display count
	ImGui::Text("Players: %zu", entries.size());
	ImGui::Separator();

	// Table header
	if (ImGui::BeginTable("PlayerTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
		ImVec2(0, ImGui::GetContentRegionAvail().y)))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		if (ShowWeapon)
			ImGui::TableSetupColumn("Weapon", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		if (ShowHealth)
			ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 50.0f);
		if (ShowDistance)
			ImGui::TableSetupColumn("Dist", ImGuiTableColumnFlags_WidthFixed, 50.0f);
		ImGui::TableHeadersRow();

		int index = 0;
		for (const auto& entry : entries)
		{
			if (entry.isObserved && entry.observed)
				RenderPlayerRow(*entry.observed, localPos, index++);
			else if (entry.client)
				RenderPlayerRow(*entry.client, localPos, index++);
		}

		ImGui::EndTable();
	}
}

void PlayerInfoWidget::RenderPlayerRow(const CObservedPlayer& player, const Vector3& localPos, int index)
{
	ImGui::TableNextRow();

	float distance = player.GetBonePosition(EBoneIndex::Root).DistanceTo(localPos);
	ImU32 color = GetPlayerTypeColor(player);
	bool isFocused = PlayerFocus::FocusedPlayerAddr == player.m_EntityAddress;

	// Name column
	ImGui::TableSetColumnIndex(0);
	if (isFocused)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
	}

	std::string selectableId = player.GetBaseName() + "##player" + std::to_string(index);
	if (ImGui::Selectable(selectableId.c_str(), isFocused, ImGuiSelectableFlags_SpanAllColumns))
	{
		PlayerFocus::CycleFocusState(player.m_EntityAddress);
	}
	ImGui::PopStyleColor();

	// Weapon column
	if (ShowWeapon)
	{
		ImGui::TableSetColumnIndex(1);
		if (player.m_pHands && player.m_pHands->m_pHeldItem)
		{
			const std::string& weaponNameRef = player.m_pHands->m_pHeldItem->GetItemName();
			if (!weaponNameRef.empty())
			{
				// Truncate weapon name if too long
				std::string weaponName = weaponNameRef;
				if (weaponName.length() > 12)
					weaponName = weaponName.substr(0, 10) + "..";
				ImGui::TextDisabled("%s", weaponName.c_str());
			}
			else
			{
				ImGui::TextDisabled("-");
			}
		}
		else
		{
			ImGui::TextDisabled("-");
		}
	}

	// Health column
	if (ShowHealth)
	{
		ImGui::TableSetColumnIndex(2);
		const char* healthStr = GetHealthStatusString(player.m_TagStatus);
		ImVec4 healthColor;
		if (player.m_TagStatus & static_cast<uint32_t>(ETagStatus::Dying))
			healthColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
		else if (player.m_TagStatus & static_cast<uint32_t>(ETagStatus::BadlyInjured))
			healthColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
		else if (player.m_TagStatus & static_cast<uint32_t>(ETagStatus::Injured))
			healthColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
		else
			healthColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);

		ImGui::TextColored(healthColor, "%s", healthStr);
	}

	// Distance column
	if (ShowDistance)
	{
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%s", GetDistanceString(distance).c_str());
	}
}

void PlayerInfoWidget::RenderPlayerRow(const CClientPlayer& player, const Vector3& localPos, int index)
{
	ImGui::TableNextRow();

	float distance = player.GetBonePosition(EBoneIndex::Root).DistanceTo(localPos);
	ImU32 color = GetPlayerTypeColor(player);
	bool isFocused = PlayerFocus::FocusedPlayerAddr == player.m_EntityAddress;

	// Name column
	ImGui::TableSetColumnIndex(0);
	if (isFocused)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
	}

	std::string selectableId = player.GetBaseName() + "##player" + std::to_string(index);
	if (ImGui::Selectable(selectableId.c_str(), isFocused, ImGuiSelectableFlags_SpanAllColumns))
	{
		PlayerFocus::CycleFocusState(player.m_EntityAddress);
	}
	ImGui::PopStyleColor();

	// Weapon column
	if (ShowWeapon)
	{
		ImGui::TableSetColumnIndex(1);
		if (player.m_pHands && player.m_pHands->m_pHeldItem)
		{
			const std::string& weaponNameRef = player.m_pHands->m_pHeldItem->GetItemName();
			if (!weaponNameRef.empty())
			{
				std::string weaponName = weaponNameRef;
				if (weaponName.length() > 12)
					weaponName = weaponName.substr(0, 10) + "..";
				ImGui::TextDisabled("%s", weaponName.c_str());
			}
			else
			{
				ImGui::TextDisabled("-");
			}
		}
		else
		{
			ImGui::TextDisabled("-");
		}
	}

	// Health column
	if (ShowHealth)
	{
		ImGui::TableSetColumnIndex(2);
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "?");
	}

	// Distance column
	if (ShowDistance)
	{
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%s", GetDistanceString(distance).c_str());
	}
}

ImU32 PlayerInfoWidget::GetPlayerTypeColor(const CObservedPlayer& player)
{
	if (player.IsBoss())
		return static_cast<ImU32>(ColorPicker::Radar::m_BossColor);
	if (player.IsPMC())
		return static_cast<ImU32>(ColorPicker::Radar::m_PMCColor);
	if (player.IsPlayerScav())
		return static_cast<ImU32>(ColorPicker::Radar::m_PlayerScavColor);
	return static_cast<ImU32>(ColorPicker::Radar::m_ScavColor);
}

ImU32 PlayerInfoWidget::GetPlayerTypeColor(const CClientPlayer& player)
{
	if (player.IsBoss())
		return static_cast<ImU32>(ColorPicker::Radar::m_BossColor);
	if (player.IsPMC())
		return static_cast<ImU32>(ColorPicker::Radar::m_PMCColor);
	if (player.IsPlayerScav())
		return static_cast<ImU32>(ColorPicker::Radar::m_PlayerScavColor);
	return static_cast<ImU32>(ColorPicker::Radar::m_ScavColor);
}

std::string PlayerInfoWidget::GetDistanceString(float distance)
{
	if (distance < 1000.0f)
		return std::to_string(static_cast<int>(distance)) + "m";
	return std::to_string(static_cast<int>(distance / 1000.0f)) + "km";
}

json PlayerInfoWidget::Serialize() const
{
	json j = RadarWidget::Serialize();
	j["MaxDisplayPlayers"] = MaxDisplayPlayers;
	j["SortByDistance"] = SortByDistance;
	j["ShowWeapon"] = ShowWeapon;
	j["ShowDistance"] = ShowDistance;
	j["ShowHealth"] = ShowHealth;
	return j;
}

void PlayerInfoWidget::Deserialize(const json& j)
{
	RadarWidget::Deserialize(j);
	if (j.contains("MaxDisplayPlayers")) MaxDisplayPlayers = j["MaxDisplayPlayers"].get<int>();
	if (j.contains("SortByDistance")) SortByDistance = j["SortByDistance"].get<bool>();
	if (j.contains("ShowWeapon")) ShowWeapon = j["ShowWeapon"].get<bool>();
	if (j.contains("ShowDistance")) ShowDistance = j["ShowDistance"].get<bool>();
	if (j.contains("ShowHealth")) ShowHealth = j["ShowHealth"].get<bool>();
}
