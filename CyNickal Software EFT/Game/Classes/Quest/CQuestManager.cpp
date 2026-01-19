#include "pch.h"
#include "CQuestManager.h"
#include "../../Offsets/Offsets.h"
#include "../../../DMA/DMA.h"
#include "../../EFT.h"
#include <iostream>

namespace Quest
{
	bool CQuestManager::Initialize(const std::string& dataFilePath)
	{
		return QuestDatabase::GetInstance().LoadFromFile(dataFilePath);
	}

	void CQuestManager::Refresh(uintptr_t profileAddr)
	{
		if (!m_Settings.bEnabled || profileAddr == 0)
			return;

		// Throttle refresh rate
		auto now = std::chrono::steady_clock::now();
		if (now - m_LastRefresh < REFRESH_INTERVAL)
			return;
		m_LastRefresh = now;

		auto* Conn = DMA_Connection::GetInstance();
		if (!Conn || !Conn->IsConnected())
			return;

		auto PID = EFT::GetProcess().GetPID();
		if (PID == 0)
			return;

		try
		{
			std::scoped_lock lock(m_Mutex);

			// Read QuestsData list from profile
			uintptr_t questsDataAddr = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, profileAddr + Offsets::CProfile::pQuestsData,
				reinterpret_cast<PBYTE>(&questsDataAddr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
			if (questsDataAddr == 0)
				return;

			// Read list count and array base
			int32_t questCount = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, questsDataAddr + Offsets::CUnityList::Count,
				reinterpret_cast<PBYTE>(&questCount), sizeof(int32_t), nullptr, VMMDLL_FLAG_NOCACHE);

			uintptr_t questArrayBase = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, questsDataAddr + Offsets::CUnityList::ArrayBase,
				reinterpret_cast<PBYTE>(&questArrayBase), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

			if (questCount <= 0 || questCount > 500 || questArrayBase == 0)
				return;

			// Track which quests are still active
			std::unordered_set<std::string> activeQuestIds;

			// Read each quest entry
			for (int32_t i = 0; i < questCount; i++)
			{
				uintptr_t questDataEntry = 0;
				VMMDLL_MemReadEx(Conn->GetHandle(), PID, questArrayBase + 0x20 + (i * 0x8),
					reinterpret_cast<PBYTE>(&questDataEntry), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
				if (questDataEntry == 0)
					continue;

				// Read quest status
				int32_t status = 0;
				VMMDLL_MemReadEx(Conn->GetHandle(), PID, questDataEntry + Offsets::CQuestsData::Status,
					reinterpret_cast<PBYTE>(&status), sizeof(int32_t), nullptr, VMMDLL_FLAG_NOCACHE);
				if (status != static_cast<int32_t>(EQuestStatus::Started))
					continue; // Only care about started quests

				// Read quest ID
				std::string questId = ReadQuestId(questDataEntry);
				if (questId.empty())
					continue;

				activeQuestIds.insert(questId);

				// Get task definition from database
				const auto* taskDef = QuestDatabase::GetInstance().GetTask(questId);
				if (!taskDef)
					continue;

				// Add or update quest entry
				if (!m_ActiveQuests.contains(questId))
				{
					m_ActiveQuests[questId] = QuestEntry(questId, taskDef->Name);
					m_ActiveQuests[questId].bIsEnabled = IsQuestEnabled(questId);
				}

				// Read completed conditions if quest is enabled
				if (m_ActiveQuests[questId].bIsEnabled)
				{
					uintptr_t completedAddr = 0;
					VMMDLL_MemReadEx(Conn->GetHandle(), PID, questDataEntry + Offsets::CQuestsData::pCompletedConditions,
						reinterpret_cast<PBYTE>(&completedAddr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
					if (completedAddr != 0)
					{
						m_CompletedConditions[questId] = ReadCompletedConditions(completedAddr);
					}
				}
			}

			// Remove quests that are no longer active
			for (auto it = m_ActiveQuests.begin(); it != m_ActiveQuests.end(); )
			{
				if (!activeQuestIds.contains(it->first))
				{
					m_CompletedConditions.erase(it->first);
					it = m_ActiveQuests.erase(it);
				}
				else
				{
					++it;
				}
			}

			// Update derived data
			UpdateQuestItems();
			UpdateQuestLocations();
		}
		catch (const std::exception& e)
		{
			std::cout << "[QuestManager] Error refreshing quests: " << e.what() << std::endl;
		}
	}

	std::string CQuestManager::ReadQuestId(uintptr_t questDataAddr)
	{
		auto* Conn = DMA_Connection::GetInstance();
		if (!Conn)
			return "";

		auto PID = EFT::GetProcess().GetPID();
		if (PID == 0)
			return "";

		try
		{
			uintptr_t idStringAddr = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, questDataAddr + Offsets::CQuestsData::pId,
				reinterpret_cast<PBYTE>(&idStringAddr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
			if (idStringAddr == 0)
				return "";

			// Read Unity string length
			int32_t length = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, idStringAddr + 0x10,
				reinterpret_cast<PBYTE>(&length), sizeof(int32_t), nullptr, VMMDLL_FLAG_NOCACHE);
			if (length <= 0 || length > 64)
				return "";

			// Read Unity string data (wchar_t)
			std::wstring wstr(length, L'\0');
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, idStringAddr + 0x14,
				reinterpret_cast<PBYTE>(wstr.data()), length * sizeof(wchar_t), nullptr, VMMDLL_FLAG_NOCACHE);

			// Convert to std::string
			std::string result(length, '\0');
			for (int i = 0; i < length; i++)
			{
				result[i] = static_cast<char>(wstr[i]);
			}
			return result;
		}
		catch (...)
		{
			return "";
		}
	}

	std::unordered_set<std::string> CQuestManager::ReadCompletedConditions(uintptr_t completedConditionsAddr)
	{
		std::unordered_set<std::string> result;
		auto* Conn = DMA_Connection::GetInstance();
		if (!Conn || completedConditionsAddr == 0)
			return result;

		auto PID = EFT::GetProcess().GetPID();
		if (PID == 0)
			return result;

		try
		{
			// HashSet<MongoID> structure
			// MongoID is a string wrapper
			uintptr_t bucketsAddr = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, completedConditionsAddr + 0x10,
				reinterpret_cast<PBYTE>(&bucketsAddr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

			int32_t count = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, completedConditionsAddr + 0x28,
				reinterpret_cast<PBYTE>(&count), sizeof(int32_t), nullptr, VMMDLL_FLAG_NOCACHE);

			if (bucketsAddr == 0 || count <= 0 || count > 100)
				return result;

			// For now, just return empty - reading HashSet is complex
			// The main quest item/location logic works without this
		}
		catch (...)
		{
		}

		return result;
	}

	void CQuestManager::SetCurrentMap(const std::string& mapId)
	{
		std::scoped_lock lock(m_Mutex);
		if (m_CurrentMapId != mapId)
		{
			m_CurrentMapId = mapId;
			UpdateQuestLocations();
		}
	}

	void CQuestManager::SetQuestEnabled(const std::string& questId, bool enabled)
	{
		std::scoped_lock lock(m_Mutex);
		if (enabled)
		{
			m_Settings.BlacklistedQuests.erase(questId);
		}
		else
		{
			m_Settings.BlacklistedQuests.insert(questId);
		}

		if (m_ActiveQuests.contains(questId))
		{
			m_ActiveQuests[questId].bIsEnabled = enabled;
		}

		// Refresh derived data
		UpdateQuestItems();
		UpdateQuestLocations();
	}

	void CQuestManager::ClearCache()
	{
		std::scoped_lock lock(m_Mutex);
		m_ActiveQuests.clear();
		m_CompletedConditions.clear();
		m_QuestItemIds.clear();
		m_QuestLocations.clear();
		m_CurrentMapId.clear();
	}

	void CQuestManager::UpdateQuestItems()
	{
		m_QuestItemIds.clear();

		for (const auto& [questId, quest] : m_ActiveQuests)
		{
			if (!quest.bIsEnabled)
				continue;

			const auto* taskDef = QuestDatabase::GetInstance().GetTask(questId);
			if (!taskDef)
				continue;

			const auto& completed = m_CompletedConditions.contains(questId)
				? m_CompletedConditions.at(questId)
				: std::unordered_set<std::string>{};

			for (const auto& obj : taskDef->Objectives)
			{
				// Skip completed objectives
				if (completed.contains(obj.Id))
					continue;

				auto type = obj.GetType();

				// Add quest items from findQuestItem objectives
				if (type == EQuestObjectiveType::FindQuestItem && !obj.QuestItemRef.Id.empty())
				{
					m_QuestItemIds.insert(obj.QuestItemRef.Id);
				}
				// Add regular items from findItem objectives
				else if (type == EQuestObjectiveType::FindItem && !obj.Item.Id.empty())
				{
					m_QuestItemIds.insert(obj.Item.Id);
				}
			}
		}
	}

	void CQuestManager::UpdateQuestLocations()
	{
		m_QuestLocations.clear();

		if (m_CurrentMapId.empty())
			return;

		const auto* zonesForMap = QuestDatabase::GetInstance().GetZonesForMap(m_CurrentMapId);
		if (!zonesForMap)
			return;

		for (const auto& [questId, quest] : m_ActiveQuests)
		{
			if (!quest.bIsEnabled)
				continue;

			const auto* taskDef = QuestDatabase::GetInstance().GetTask(questId);
			if (!taskDef)
				continue;

			const auto& completed = m_CompletedConditions.contains(questId)
				? m_CompletedConditions.at(questId)
				: std::unordered_set<std::string>{};

			for (const auto& obj : taskDef->Objectives)
			{
				// Skip completed objectives
				if (completed.contains(obj.Id))
					continue;

				auto type = obj.GetType();

				// Only care about location-based objectives
				if (type != EQuestObjectiveType::Visit &&
					type != EQuestObjectiveType::Mark &&
					type != EQuestObjectiveType::PlantItem &&
					type != EQuestObjectiveType::PlantQuestItem)
					continue;

				for (const auto& zone : obj.Zones)
				{
					if (zone.MapNameId != m_CurrentMapId)
						continue;

					// Check if we have position data for this zone
					auto zoneIt = zonesForMap->find(zone.Id);
					if (zoneIt != zonesForMap->end())
					{
						m_QuestLocations.emplace_back(
							questId,
							quest.Name,
							obj.Id,
							zone.Id,
							type,
							zoneIt->second
						);
					}
				}
			}
		}
	}
}
