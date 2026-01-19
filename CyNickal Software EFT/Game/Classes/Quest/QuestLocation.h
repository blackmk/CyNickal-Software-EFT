#pragma once
#include <string>
#include "QuestObjectiveType.h"
#include "../Vector.h"

namespace Quest
{
	/// <summary>
	/// Represents a quest objective location marker on the map
	/// </summary>
	struct QuestLocation
	{
		std::string QuestId;        // Parent quest ID
		std::string QuestName;      // Parent quest display name
		std::string ObjectiveId;    // Objective condition ID
		std::string ZoneId;         // Zone ID from tarkov.dev
		EQuestObjectiveType Type = EQuestObjectiveType::Unknown;
		Vector3 Position;           // World position

		QuestLocation() = default;
		QuestLocation(const std::string& questId, const std::string& questName,
			const std::string& objectiveId, const std::string& zoneId,
			EQuestObjectiveType type, const Vector3& pos)
			: QuestId(questId), QuestName(questName), ObjectiveId(objectiveId),
			ZoneId(zoneId), Type(type), Position(pos) {}

		/// <summary>
		/// Unique key for this location (quest:objective:zone)
		/// </summary>
		std::string GetKey() const
		{
			return QuestId + ":" + ObjectiveId + ":" + ZoneId;
		}
	};
}
