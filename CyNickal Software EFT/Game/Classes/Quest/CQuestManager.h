#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include "QuestObjectiveType.h"
#include "QuestEntry.h"
#include "QuestLocation.h"
#include "QuestData.h"
#include "../Vector.h"

namespace Quest
{
	/// <summary>
	/// Settings for quest helper display
	/// </summary>
	struct QuestSettings
	{
		bool bEnabled = true;                    // Master toggle for quest helper
		bool bShowQuestLocations = true;         // Show quest markers on radar
		bool bHighlightQuestItems = true;        // Highlight quest items in loot
		bool bShowQuestPanel = true;             // Show quest list panel
		float fQuestMarkerSize = 8.0f;           // Size of quest markers on radar
		ImVec4 QuestLocationColor = ImVec4(0.8f, 0.4f, 0.8f, 1.0f);  // Purple
		ImVec4 QuestItemColor = ImVec4(0.8f, 0.4f, 0.8f, 1.0f);      // Purple
		std::unordered_set<std::string> BlacklistedQuests;           // User-disabled quests
	};

	/// <summary>
	/// Manages active quests from player profile memory
	/// </summary>
	class CQuestManager
	{
	public:
		/// <summary>
		/// Snapshot of quest data for UI rendering (lock-free reads)
		/// </summary>
		struct QuestSnapshot
		{
			std::unordered_map<std::string, QuestEntry> ActiveQuests;
			std::unordered_set<std::string> QuestItemIds;
			std::vector<QuestLocation> QuestLocations;
			QuestSettings Settings;
		};

		static CQuestManager& GetInstance()
		{
			static CQuestManager instance;
			return instance;
		}

		/// <summary>
		/// Initialize quest database from data file
		/// </summary>
		bool Initialize(const std::string& dataFilePath);

		/// <summary>
		/// Refresh active quests from player profile memory
		/// Called periodically from DMA thread
		/// </summary>
		void Refresh(uintptr_t profileAddr);

		/// <summary>
		/// Get all currently active quests (returns copy for thread safety)
		/// </summary>
		std::unordered_map<std::string, QuestEntry> GetActiveQuests() const
		{
			std::scoped_lock lock(m_Mutex);
			return m_ActiveQuests;
		}

		/// <summary>
		/// Get all quest item IDs we need to find (returns copy for thread safety)
		/// </summary>
		std::unordered_set<std::string> GetQuestItemIds() const
		{
			std::scoped_lock lock(m_Mutex);
			return m_QuestItemIds;
		}

		/// <summary>
		/// Get quest locations for current map (returns copy for thread safety)
		/// </summary>
		std::vector<QuestLocation> GetQuestLocations() const
		{
			std::scoped_lock lock(m_Mutex);
			return m_QuestLocations;
		}

		/// <summary>
		/// Get quest settings snapshot (thread-safe copy)
		/// </summary>
		QuestSettings GetSettingsCopy() const
		{
			std::scoped_lock lock(m_Mutex);
			return m_Settings;
		}

		/// <summary>
		/// Get quest data snapshot for UI rendering
		/// </summary>
		QuestSnapshot GetSnapshot() const;

		/// <summary>
		/// Check if an item is a quest item we need
		/// </summary>
		bool IsQuestItem(const std::string& itemId) const
		{
			std::scoped_lock lock(m_Mutex);
			if (!m_Settings.bHighlightQuestItems || !m_Settings.bEnabled)
				return false;
			return m_QuestItemIds.contains(itemId);
		}

		/// <summary>
		/// Set current map ID (e.g., "bigmap" for Customs)
		/// </summary>
		void SetCurrentMap(const std::string& mapId);

		/// <summary>
		/// Toggle quest enabled/disabled
		/// </summary>
		void SetQuestEnabled(const std::string& questId, bool enabled);

		/// <summary>
		/// Check if quest is enabled (not blacklisted)
		/// </summary>
		bool IsQuestEnabled(const std::string& questId) const
		{
			std::scoped_lock lock(m_Mutex);
			return !m_Settings.BlacklistedQuests.contains(questId);
		}

		/// <summary>
		/// Update quest settings from UI/config
		/// </summary>
		void UpdateSettings(const QuestSettings& newSettings);

		/// <summary>
		/// Get quest database
		/// </summary>
		QuestDatabase& GetDatabase() { return QuestDatabase::GetInstance(); }

		/// <summary>
		/// Clear all cached quest data (on raid exit)
		/// </summary>
		void ClearCache();

		std::mutex& GetMutex() { return m_Mutex; }

	private:
		CQuestManager() = default;
		~CQuestManager() = default;
		CQuestManager(const CQuestManager&) = delete;
		CQuestManager& operator=(const CQuestManager&) = delete;

		void UpdateQuestLocations();
		void UpdateQuestItems();
		std::string ReadQuestId(uintptr_t questDataAddr);
		std::unordered_set<std::string> ReadCompletedConditions(uintptr_t completedConditionsAddr);

		mutable std::mutex m_Mutex;
		QuestSettings m_Settings;

		// Active quests from memory (QuestId -> QuestEntry)
		std::unordered_map<std::string, QuestEntry> m_ActiveQuests;
		// Completed conditions per quest (QuestId -> set of condition IDs)
		std::unordered_map<std::string, std::unordered_set<std::string>> m_CompletedConditions;
		// Quest items we need to find (Item IDs)
		std::unordered_set<std::string> m_QuestItemIds;
		// Quest locations for current map
		std::vector<QuestLocation> m_QuestLocations;
		// Current map ID
		std::string m_CurrentMapId;

		mutable QuestSnapshot m_CachedSnapshot{};
		mutable std::atomic<bool> m_bSnapshotDirty{ true };

		// Refresh throttle
		std::chrono::steady_clock::time_point m_LastRefresh;
		static constexpr auto REFRESH_INTERVAL = std::chrono::seconds(1);
	};
}
