#pragma once
#include <string>

namespace Quest
{
	/// <summary>
	/// Represents an active quest from the player's profile
	/// </summary>
	struct QuestEntry
	{
		std::string Id;           // BSG Task ID (e.g., "5936d90786f7742b1420ba5b")
		std::string Name;         // Display name (e.g., "Debut")
		bool bIsEnabled = true;   // User toggle to show/hide this quest's objectives

		QuestEntry() = default;
		QuestEntry(const std::string& id, const std::string& name)
			: Id(id), Name(name), bIsEnabled(true) {}

		bool operator==(const QuestEntry& other) const { return Id == other.Id; }
		bool operator!=(const QuestEntry& other) const { return Id != other.Id; }
	};
}
