#pragma once
#include <string>
#include <unordered_map>
#include "Game/Enums/ESpawnType.h"
#include "Game/Enums/EPlayerSide.h"

// Unified entity name dictionary
// Maps ALL spawn types to friendly display names
// Replaces the old BossNameMap.h with a comprehensive mapping

namespace SpawnTypeNames
{
	// Entity type categories for color/display purposes
	enum class EntityCategory
	{
		Boss,           // Bosses (Reshala, Killa, etc.)
		Guard,          // Boss guards/followers
		Raider,         // Raiders, Rogues, Black Division, etc.
		Cultist,        // Sectant warriors and priests
		Scav,           // Regular AI scavs
		Special,        // Special entities (Santa, Zombie, Civilian, etc.)
		PMC,            // Player PMCs
		PlayerScav,     // Player Scavs
		Unknown         // Unknown type
	};

	// Entity info structure
	struct EntityInfo
	{
		std::string displayName;
		EntityCategory category;
		std::string bossAssociation;  // For guards, which boss they're associated with
	};

	// Comprehensive spawn type name mapping
	inline const std::unordered_map<ESpawnType, EntityInfo> SpawnTypeMap = {
		// ==================== BOSSES ====================
		{ ESpawnType::Reshala,       { "Reshala",          EntityCategory::Boss,    "" }},
		{ ESpawnType::Killa,         { "Killa",            EntityCategory::Boss,    "" }},
		{ ESpawnType::Shturman,      { "Shturman",         EntityCategory::Boss,    "" }},
		{ ESpawnType::Gluhar,        { "Gluhar",           EntityCategory::Boss,    "" }},
		{ ESpawnType::Sanitar,       { "Sanitar",          EntityCategory::Boss,    "" }},
		{ ESpawnType::Tagilla,       { "Tagilla",          EntityCategory::Boss,    "" }},
		{ ESpawnType::Zryachiy,      { "Zryachiy",         EntityCategory::Boss,    "" }},
		{ ESpawnType::Knight,        { "Knight",           EntityCategory::Boss,    "" }},
		{ ESpawnType::BigPipe,       { "Big Pipe",         EntityCategory::Boss,    "" }},
		{ ESpawnType::BirdEye,       { "Birdeye",          EntityCategory::Boss,    "" }},
		{ ESpawnType::Kaban,         { "Kaban",            EntityCategory::Boss,    "" }},
		{ ESpawnType::Kolontay,      { "Kollontay",        EntityCategory::Boss,    "" }},
		{ ESpawnType::Partisan,      { "Partisan",         EntityCategory::Boss,    "" }},
		{ ESpawnType::bossBoarSniper,{ "Kaban Sniper",     EntityCategory::Boss,    "" }},
		{ ESpawnType::bossTagillaAgro, { "Tagilla (Agro)", EntityCategory::Boss,    "" }},
		{ ESpawnType::bossKillaAgro, { "Killa (Agro)",     EntityCategory::Boss,    "" }},

		// ==================== GUARDS ====================
		// Reshala's guards
		{ ESpawnType::followerBully, { "Reshala Guard",    EntityCategory::Guard,   "Reshala" }},
		
		// Shturman's guards
		{ ESpawnType::followerKojaniy, { "Shturman Guard", EntityCategory::Guard,   "Shturman" }},
		
		// Gluhar's guards (4 types)
		{ ESpawnType::followerGluharAssault,  { "Gluhar Guard",   EntityCategory::Guard,   "Gluhar" }},
		{ ESpawnType::followerGluharSecurity, { "Gluhar Guard",   EntityCategory::Guard,   "Gluhar" }},
		{ ESpawnType::followerGluharScout,    { "Gluhar Scout",   EntityCategory::Guard,   "Gluhar" }},
		{ ESpawnType::followerGluharSnipe,    { "Gluhar Sniper",  EntityCategory::Guard,   "Gluhar" }},
		
		// Sanitar's guards
		{ ESpawnType::followerSanitar, { "Sanitar Guard",  EntityCategory::Guard,   "Sanitar" }},
		
		// Tagilla's guards
		{ ESpawnType::followerTagilla,   { "Tagilla Guard",  EntityCategory::Guard,   "Tagilla" }},
		{ ESpawnType::tagillaHelperAgro, { "Tagilla Guard",  EntityCategory::Guard,   "Tagilla" }},
		
		// Zryachiy's guards
		{ ESpawnType::followerZryachiy, { "Zryachiy Guard", EntityCategory::Guard,   "Zryachiy" }},
		
		// Kaban's guards (multiple types)
		{ ESpawnType::followerBoar,       { "Kaban Guard",   EntityCategory::Guard,   "Kaban" }},
		{ ESpawnType::followerBoarClose1, { "Kaban Guard",   EntityCategory::Guard,   "Kaban" }},
		{ ESpawnType::followerBoarClose2, { "Kaban Guard",   EntityCategory::Guard,   "Kaban" }},
		
		// Kollontay's guards
		{ ESpawnType::followerKolontayAssault,  { "Kollontay Guard", EntityCategory::Guard, "Kollontay" }},
		{ ESpawnType::followerKolontaySecurity, { "Kollontay Guard", EntityCategory::Guard, "Kollontay" }},

		// ==================== RAIDERS ====================
		{ ESpawnType::exUsec,            { "Rogue",           EntityCategory::Raider, "" }},
		{ ESpawnType::pmcBot,            { "Raider",          EntityCategory::Raider, "" }},
		{ ESpawnType::pmcBEAR,           { "BEAR Bot",        EntityCategory::Raider, "" }},
		{ ESpawnType::pmcUSEC,           { "USEC Bot",        EntityCategory::Raider, "" }},
		{ ESpawnType::arenaFighter,      { "Arena Fighter",   EntityCategory::Raider, "" }},
		{ ESpawnType::arenaFighterEvent, { "Arena Fighter",   EntityCategory::Raider, "" }},
		{ ESpawnType::blackDivision,     { "Black Division",  EntityCategory::Raider, "" }},
		{ ESpawnType::vsRF,              { "VSRF",            EntityCategory::Raider, "" }},
		{ ESpawnType::vsRFSniper,        { "VSRF Sniper",     EntityCategory::Raider, "" }},
		{ ESpawnType::vsRFFight,         { "VSRF",            EntityCategory::Raider, "" }},

		// ==================== CULTISTS ====================
		{ ESpawnType::sectantWarrior,      { "Cultist",         EntityCategory::Cultist, "" }},
		{ ESpawnType::sectantPriest,       { "Priest",          EntityCategory::Cultist, "" }},
		{ ESpawnType::sectactPriestEvent,  { "Priest",          EntityCategory::Cultist, "" }},
		{ ESpawnType::sectantPredvestnik,  { "Predvestnik",     EntityCategory::Cultist, "" }},
		{ ESpawnType::sectantPrizrak,      { "Prizrak",         EntityCategory::Cultist, "" }},
		{ ESpawnType::sectantOni,          { "Oni",             EntityCategory::Cultist, "" }},

		// ==================== REGULAR SCAVS ====================
		{ ESpawnType::assault,         { "Scav",            EntityCategory::Scav, "" }},
		{ ESpawnType::marksman,        { "Sniper Scav",     EntityCategory::Scav, "" }},
		{ ESpawnType::cursedAssault,   { "Cursed Scav",     EntityCategory::Scav, "" }},
		{ ESpawnType::assaultGroup,    { "Scav",            EntityCategory::Scav, "" }},
		{ ESpawnType::crazyAssaultEvent, { "Crazy Scav",    EntityCategory::Scav, "" }},
		{ ESpawnType::assaultTutorial, { "Scav",            EntityCategory::Scav, "" }},

		// ==================== SPECIAL ENTITIES ====================
		{ ESpawnType::gifter,          { "Santa",           EntityCategory::Special, "" }},
		{ ESpawnType::sentry,          { "Sentry",          EntityCategory::Special, "" }},
		{ ESpawnType::shooterBTR,      { "BTR Gunner",      EntityCategory::Special, "" }},
		{ ESpawnType::civilian,        { "Civilian",        EntityCategory::Special, "" }},
		{ ESpawnType::peacemaker,      { "Peacemaker",      EntityCategory::Special, "" }},
		{ ESpawnType::skier,           { "Skier",           EntityCategory::Special, "" }},
		{ ESpawnType::spiritWinter,    { "Winter Spirit",   EntityCategory::Special, "" }},
		{ ESpawnType::spiritSpring,    { "Spring Spirit",   EntityCategory::Special, "" }},
		{ ESpawnType::peacefullZryachiyEvent, { "Zryachiy (Peaceful)", EntityCategory::Special, "" }},
		{ ESpawnType::ravangeZryachiyEvent,   { "Zryachiy (Revenge)",  EntityCategory::Special, "" }},

		// ==================== ZOMBIES/INFECTED ====================
		{ ESpawnType::infectedAssault,  { "Zombie",          EntityCategory::Special, "" }},
		{ ESpawnType::infectedPmc,      { "Zombie PMC",      EntityCategory::Special, "" }},
		{ ESpawnType::infectedCivil,    { "Zombie Civilian", EntityCategory::Special, "" }},
		{ ESpawnType::infectedLaborant, { "Zombie Laborant", EntityCategory::Special, "" }},
		{ ESpawnType::infectedTagilla,  { "Zombie Tagilla",  EntityCategory::Boss,    "" }},  // Still a boss

		// ==================== TEST/UNKNOWN ====================
		{ ESpawnType::bossTest,     { "Boss (Test)",     EntityCategory::Boss,    "" }},
		{ ESpawnType::followerTest, { "Guard (Test)",    EntityCategory::Guard,   "" }},
		{ ESpawnType::test,         { "Test",            EntityCategory::Unknown, "" }},
		{ ESpawnType::UNKNOWN,      { "Unknown",         EntityCategory::Unknown, "" }},
	};

	// PMC faction names
	inline const std::unordered_map<EPlayerSide, std::string> FactionNames = {
		{ EPlayerSide::USEC, "USEC" },
		{ EPlayerSide::BEAR, "BEAR" },
		{ EPlayerSide::SCAV, "Scav" },
	};

	// Get entity info for a spawn type
	inline const EntityInfo& GetEntityInfo(ESpawnType spawnType)
	{
		static const EntityInfo defaultInfo = { "Unknown", EntityCategory::Unknown, "" };
		auto it = SpawnTypeMap.find(spawnType);
		if (it != SpawnTypeMap.end())
			return it->second;
		return defaultInfo;
	}

	// Get display name for a spawn type
	inline const std::string& GetDisplayName(ESpawnType spawnType)
	{
		return GetEntityInfo(spawnType).displayName;
	}

	// Get category for a spawn type
	inline EntityCategory GetCategory(ESpawnType spawnType)
	{
		return GetEntityInfo(spawnType).category;
	}

	// Get faction name for player side
	inline const std::string& GetFactionName(EPlayerSide side)
	{
		static const std::string unknown = "Unknown";
		auto it = FactionNames.find(side);
		if (it != FactionNames.end())
			return it->second;
		return unknown;
	}

	// Check if spawn type is a boss
	inline bool IsBoss(ESpawnType spawnType)
	{
		return GetCategory(spawnType) == EntityCategory::Boss;
	}

	// Check if spawn type is a guard
	inline bool IsGuard(ESpawnType spawnType)
	{
		return GetCategory(spawnType) == EntityCategory::Guard;
	}

	// Check if spawn type is a raider
	inline bool IsRaider(ESpawnType spawnType)
	{
		return GetCategory(spawnType) == EntityCategory::Raider;
	}

	// Check if spawn type is a cultist
	inline bool IsCultist(ESpawnType spawnType)
	{
		return GetCategory(spawnType) == EntityCategory::Cultist;
	}

	// Check if spawn type is a regular scav
	inline bool IsScav(ESpawnType spawnType)
	{
		return GetCategory(spawnType) == EntityCategory::Scav;
	}

	// Check if spawn type is special (zombie, civilian, etc.)
	inline bool IsSpecial(ESpawnType spawnType)
	{
		return GetCategory(spawnType) == EntityCategory::Special;
	}
}
