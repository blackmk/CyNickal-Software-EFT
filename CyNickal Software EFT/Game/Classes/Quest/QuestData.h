#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "QuestObjectiveType.h"
#include "../Vector.h"
#include <json.hpp>

namespace Quest
{
	/// <summary>
	/// Quest zone position from tarkov.dev
	/// </summary>
	struct TaskZone
	{
		std::string Id;
		std::string MapNameId;   // e.g., "bigmap" for Customs
		Vector3 Position;
	};

	/// <summary>
	/// Quest item reference from tarkov.dev
	/// </summary>
	struct QuestItem
	{
		std::string Id;          // BSG item ID
		std::string Name;
		std::string ShortName;
	};

	/// <summary>
	/// Quest objective from tarkov.dev
	/// </summary>
	struct TaskObjective
	{
		std::string Id;                      // Condition ID
		std::string Type;                    // Type string (e.g., "visit", "findItem")
		std::string Description;
		std::vector<TaskZone> Zones;         // Location zones for visit/mark/plant
		QuestItem Item;                      // For findItem objectives
		QuestItem QuestItemRef;              // For findQuestItem objectives
		QuestItem MarkerItem;                // For mark objectives
		int Count = 0;
		bool bFoundInRaid = false;

		EQuestObjectiveType GetType() const
		{
			return StringToObjectiveType(Type);
		}
	};

	/// <summary>
	/// Task definition from tarkov.dev API
	/// </summary>
	struct TaskDefinition
	{
		std::string Id;                      // BSG Task ID
		std::string Name;                    // Display name
		std::vector<TaskObjective> Objectives;
	};

	/// <summary>
	/// Static quest data loaded from tarkov.dev JSON
	/// Maps Task ID -> TaskDefinition
	/// </summary>
	class QuestDatabase
	{
	public:
		static QuestDatabase& GetInstance()
		{
			static QuestDatabase instance;
			return instance;
		}

		/// <summary>
		/// Load quest data from JSON file
		/// </summary>
		bool LoadFromFile(const std::string& filePath);

		/// <summary>
		/// Load quest data from JSON string
		/// </summary>
		bool LoadFromJson(const std::string& jsonStr);

		/// <summary>
		/// Get task definition by ID
		/// </summary>
		const TaskDefinition* GetTask(const std::string& taskId) const
		{
			auto it = m_Tasks.find(taskId);
			return (it != m_Tasks.end()) ? &it->second : nullptr;
		}

		/// <summary>
		/// Get all zones for a specific map
		/// MapNameId -> ZoneId -> Position
		/// </summary>
		const std::unordered_map<std::string, Vector3>* GetZonesForMap(const std::string& mapNameId) const
		{
			auto it = m_TaskZones.find(mapNameId);
			return (it != m_TaskZones.end()) ? &it->second : nullptr;
		}

		/// <summary>
		/// Check if an item ID is a quest item (findQuestItem objectives)
		/// </summary>
		bool IsQuestItemId(const std::string& itemId) const
		{
			return m_QuestItemIds.contains(itemId);
		}

		/// <summary>
		/// Get all task definitions
		/// </summary>
		const std::unordered_map<std::string, TaskDefinition>& GetAllTasks() const { return m_Tasks; }

		bool IsLoaded() const { return m_bLoaded; }

	private:
		QuestDatabase() = default;
		~QuestDatabase() = default;
		QuestDatabase(const QuestDatabase&) = delete;
		QuestDatabase& operator=(const QuestDatabase&) = delete;

		void ParseTasks(const nlohmann::json& tasksJson);
		void BuildZoneIndex();
		void BuildQuestItemIndex();

		std::unordered_map<std::string, TaskDefinition> m_Tasks;
		// MapNameId -> ZoneId -> Position
		std::unordered_map<std::string, std::unordered_map<std::string, Vector3>> m_TaskZones;
		// All quest item IDs (for quick lookup)
		std::unordered_set<std::string> m_QuestItemIds;
		bool m_bLoaded = false;
	};
}
