#pragma once
#include <string>

namespace Quest
{
	/// <summary>
	/// Quest objective types matching tarkov.dev API
	/// </summary>
	enum class EQuestObjectiveType
	{
		Unknown,
		FindQuestItem,   // "findQuestItem" - pickup quest-specific item
		GiveQuestItem,   // "giveQuestItem" - hand over quest item to trader
		PlantItem,       // "plantItem" - place marker/item at location
		PlantQuestItem,  // "plantQuestItem" - place quest-specific item
		BuildWeapon,     // "buildWeapon" - assemble weapon with parts
		FindItem,        // "findItem" - pickup regular item
		GiveItem,        // "giveItem" - hand over regular item
		Visit,           // "visit" - visit location
		Shoot,           // "shoot" - kill/shoot targets
		Skill,           // "skill" - level up skill
		Extract,         // "extract" - extract from raid
		Mark,            // "mark" - mark location with MS2000
		Experience,      // "experience" - gain XP
		UseItem,         // "useItem" - use item in raid
		SellItem,        // "sellItem" - sell item on flea
		TraderLevel,     // "traderLevel" - reach trader loyalty level
		TaskStatus,      // "taskStatus" - complete other task
		TraderStanding   // "traderStanding" - reach trader standing
	};

	/// <summary>
	/// Quest status from game memory
	/// </summary>
	enum class EQuestStatus
	{
		Locked = 0,
		AvailableForStart = 1,
		Started = 2,
		AvailableForFinish = 3,
		Success = 4,
		Fail = 5,
		FailRestartable = 6,
		MarkedAsFailed = 7,
		Expired = 8,
		AvailableAfter = 9
	};

	/// <summary>
	/// Convert string type from tarkov.dev to enum
	/// </summary>
	inline EQuestObjectiveType StringToObjectiveType(const std::string& type)
	{
		if (type == "visit") return EQuestObjectiveType::Visit;
		if (type == "mark") return EQuestObjectiveType::Mark;
		if (type == "giveItem") return EQuestObjectiveType::GiveItem;
		if (type == "shoot") return EQuestObjectiveType::Shoot;
		if (type == "extract") return EQuestObjectiveType::Extract;
		if (type == "findQuestItem") return EQuestObjectiveType::FindQuestItem;
		if (type == "giveQuestItem") return EQuestObjectiveType::GiveQuestItem;
		if (type == "findItem") return EQuestObjectiveType::FindItem;
		if (type == "buildWeapon") return EQuestObjectiveType::BuildWeapon;
		if (type == "plantItem") return EQuestObjectiveType::PlantItem;
		if (type == "plantQuestItem") return EQuestObjectiveType::PlantQuestItem;
		if (type == "traderLevel") return EQuestObjectiveType::TraderLevel;
		if (type == "traderStanding") return EQuestObjectiveType::TraderStanding;
		if (type == "skill") return EQuestObjectiveType::Skill;
		if (type == "experience") return EQuestObjectiveType::Experience;
		if (type == "useItem") return EQuestObjectiveType::UseItem;
		if (type == "sellItem") return EQuestObjectiveType::SellItem;
		if (type == "taskStatus") return EQuestObjectiveType::TaskStatus;
		return EQuestObjectiveType::Unknown;
	}

	/// <summary>
	/// Get display name for objective type
	/// </summary>
	inline const char* ObjectiveTypeToString(EQuestObjectiveType type)
	{
		switch (type)
		{
		case EQuestObjectiveType::Visit: return "Visit";
		case EQuestObjectiveType::Mark: return "Mark";
		case EQuestObjectiveType::GiveItem: return "Give Item";
		case EQuestObjectiveType::Shoot: return "Shoot";
		case EQuestObjectiveType::Extract: return "Extract";
		case EQuestObjectiveType::FindQuestItem: return "Find Quest Item";
		case EQuestObjectiveType::GiveQuestItem: return "Give Quest Item";
		case EQuestObjectiveType::FindItem: return "Find Item";
		case EQuestObjectiveType::BuildWeapon: return "Build Weapon";
		case EQuestObjectiveType::PlantItem: return "Plant Item";
		case EQuestObjectiveType::PlantQuestItem: return "Plant Quest Item";
		case EQuestObjectiveType::TraderLevel: return "Trader Level";
		case EQuestObjectiveType::TraderStanding: return "Trader Standing";
		case EQuestObjectiveType::Skill: return "Skill";
		case EQuestObjectiveType::Experience: return "Experience";
		case EQuestObjectiveType::UseItem: return "Use Item";
		case EQuestObjectiveType::SellItem: return "Sell Item";
		case EQuestObjectiveType::TaskStatus: return "Task Status";
		default: return "Unknown";
		}
	}
}
