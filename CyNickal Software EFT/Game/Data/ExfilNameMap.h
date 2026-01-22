#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>

// Exfil name dictionary: maps internal game names to friendly display names
// Organized by map ID (lowercase) -> internal name -> display name
// Based on reference from C# EFT-DMA-Radar-Source project

namespace ExfilNames
{
	// Case-insensitive string hasher for the map lookups
	struct CaseInsensitiveHash
	{
		size_t operator()(const std::string& str) const
		{
			std::string lower = str;
			std::transform(lower.begin(), lower.end(), lower.begin(),
				[](unsigned char c) { return std::tolower(c); });
			size_t hash = 0;
			for (char c : lower)
				hash = hash * 31 + c;
			return hash;
		}
	};

	struct CaseInsensitiveEqual
	{
		bool operator()(const std::string& lhs, const std::string& rhs) const
		{
			if (lhs.size() != rhs.size()) return false;
			for (size_t i = 0; i < lhs.size(); ++i)
			{
				if (std::tolower(static_cast<unsigned char>(lhs[i])) != 
					std::tolower(static_cast<unsigned char>(rhs[i])))
					return false;
			}
			return true;
		}
	};

	using ExfilMap = std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;
	using MapExfilMap = std::unordered_map<std::string, ExfilMap, CaseInsensitiveHash, CaseInsensitiveEqual>;

	// All exfil names organized by map
	inline const MapExfilMap AllExfilNames = {
		// ==================== WOODS ====================
		{ "woods", {
			// PMC
			{ "Factory Gate", "Friendship Bridge (Co-Op)" },
			{ "RUAF Gate", "RUAF Gate" },
			{ "ZB-016", "ZB-016" },
			{ "ZB-014", "ZB-014" },
			{ "UN Roadblock", "UN Roadblock" },
			{ "South V-Ex", "Bridge V-Ex" },
			{ "Outskirts", "Outskirts" },
			{ "un-sec", "Northern UN Roadblock" },
			{ "wood_sniper_exit", "Power Line Passage (Flare)" },
			{ "woods_secret_minefield", "Railway Bridge to Tarkov (Secret)" },
			// SCAV
			{ "Friendship Bridge (Co-Op)", "Friendship Bridge (Co-Op)" },
			{ "Outskirts Water", "Scav Bridge" },
			{ "Dead Man's Place", "Dead Man's Place" },
			{ "The Boat", "Boat" },
			{ "Scav House", "Scav House" },
			{ "East Gate", "Scav Bunker" },
			{ "Mountain Stash", "Mountain Stash" },
			{ "West Border", "Eastern Rocks" },
			{ "Old Station", "Old Railway Depot" },
			{ "RUAF Roadblock", "RUAF Roadblock" },
		}},

		// ==================== SHORELINE ====================
		{ "shoreline", {
			// PMC
			{ "Shorl_V-Ex", "Road to North V-Ex" },
			{ "Road to Customs", "Road to Customs" },
			{ "Road_at_railbridge", "Railway Bridge" },
			{ "Tunnel", "Tunnel" },
			{ "Lighthouse_pass", "Path to Lighthouse" },
			{ "Smugglers_Trail_coop", "Smuggler's Path (Co-op)" },
			{ "Pier Boat", "Pier Boat" },
			{ "RedRebel_alp", "Climber's Trail" },
			{ "shoreline_secret_heartbeat", "Mountain Bunker (Secret)" },
			// SCAV
			{ "Scav Road to Customs", "Road to Customs" },
			{ "Lighthouse", "Lighthouse" },
			{ "Wrecked Road", "Ruined Road" },
			{ "South Fence Passage", "Old Bunker" },
			{ "RWing Gym Entrance", "East Wing Gym Entrance" },
			{ "Adm Basement", "Admin Basement" },
			{ "Smuggler's Path (Co-op)", "Smuggler's Path (Co-op)" },
		}},

		// ==================== RESERVE ====================
		{ "rezervbase", {
			// PMC
			{ "EXFIL_Bunker_D2", "D-2" },
			{ "EXFIL_Bunker", "Bunker Hermetic Door" },
			{ "Alpinist", "Cliff Descent" },
			{ "EXFIL_ScavCooperation", "Scav Lands (Co-op)" },
			{ "EXFIL_vent", "Sewer Manhole" },
			{ "EXFIL_Train", "Armored Train" },
			{ "reserve_secret_minefield", "Exit to Woods (Secret)" },
			// SCAV
			{ "Bunker Hermetic Door", "Depot Hermetic Door" },
			{ "Scav Lands (Co-Op)", "Scav Lands (Co-Op)" },
			{ "Sewer Manhole", "Sewer Manhole" },
			{ "Exit1", "Hole in the Wall by the Mountains" },
			{ "Exit2", "Heating Pipe" },
			{ "Exit3", "??" },
			{ "Exit4", "Checkpoint Fence" },
			{ "Armored Train", "Armored Train" },
		}},

		// ==================== LABYRINTH ====================
		{ "labyrinth", {
			{ "labir_exit", "The Way Up" },
			{ "labyrinth_secret_tagilla_key", "Ariadne's Path (Secret)" },
		}},

		// ==================== LABS ====================
		{ "laboratory", {
			// PMC (no Scav exfils)
			{ "lab_Elevator_Cargo", "Cargo Elevator" },
			{ "lab_Elevator_Main", "Main Elevator" },
			{ "lab_Vent", "Ventilation Shaft" },
			{ "lab_Elevator_Med", "Medical Block Elevator" },
			{ "lab_Under_Storage_Collector", "Sewage Conduit" },
			{ "lab_Parking_Gate", "Parking Gate" },
			{ "lab_Hangar_Gate", "Hangar Gate" },
		}},

		// ==================== INTERCHANGE ====================
		{ "interchange", {
			// PMC
			{ "SE Exfil", "Emercom Checkpoint" },
			{ "NW Exfil", "Railway Exfil" },
			{ "PP Exfil", "Power Station V-Ex" },
			{ "Interchange Cooperation", "Scav Camp (Co-OP)" },
			{ "Hole Exfill", "Hole in the Fence" },
			{ "Saferoom Exfil", "Saferoom Exfil" },
			{ "shopping_sniper_exit", "Path to River (Flare)" },
			// SCAV
			{ "Emercom Checkpoint", "Emercom Checkpoint" },
			{ "Railway Exfil", "Railway Exfil" },
			{ "Scav Camp (Co-Op)", "Scav Camp (Co-OP)" },
		}},

		// ==================== FACTORY (DAY) ====================
		{ "factory4_day", {
			// PMC
			{ "Cellars", "Cellars" },
			{ "Gate 3", "Gate 3" },
			{ "Gate 0", "Gate 0" },
			{ "Gate m", "Med Tent Gate" },
			{ "Gate_o", "Courtyard Gate" },
			{ "factory_secret_ark", "Smugglers' Passage (Secret)" },
			// SCAV
			{ "Camera Bunker Door", "Camera Bunker Door" },
			{ "Office Window", "Office Window" },
		}},

		// ==================== FACTORY (NIGHT) ====================
		{ "factory4_night", {
			// PMC
			{ "Cellars", "Cellars" },
			{ "Gate 3", "Gate 3" },
			{ "Gate 0", "Gate 0" },
			{ "Gate m", "Med Tent Gate" },
			{ "Gate_o", "Courtyard Gate" },
			{ "factory_secret_ark", "Smugglers' Passage (Secret)" },
			// SCAV
			{ "Camera Bunker Door", "Camera Bunker Door" },
			{ "Office Window", "Office Window" },
		}},

		// ==================== CUSTOMS (bigmap) ====================
		{ "bigmap", {
			// PMC
			{ "EXFIL_ZB013", "ZB-013" },
			{ "Dorms V-Ex", "Dorms V-Ex" },
			{ "ZB-1011", "ZB-1011" },
			{ "Crossroads", "Crossroads" },
			{ "Old Gas Station", "Old Gas Station" },
			{ "Trailer Park", "Trailer Park" },
			{ "RUAF Roadblock", "RUAF Roadblock" },
			{ "Smuggler's Boat", "Smuggler's Boat" },
			{ "ZB-1012", "ZB-1012" },
			{ "customs_secret_voron_boat", "Smugglers' Boat (Secret)" },
			{ "customs_secret_voron_bunker", "Smugglers' Bunker (ZB-1012) (Secret)" },
			{ "Custom_scav_pmc", "Boiler Room Basement (Co-op)" },
			{ "customs_sniper_exit", "Railroad Passage (Flare)" },
			// SCAV
			{ "Shack", "Military Base CP" },
			{ "Beyond Fuel Tank", "Passage Between Rocks" },
			{ "Railroad To Military Base", "Railroad to Military Base" },
			{ "Old Road Gate", "Old Road Gate" },
			{ "Sniper Roadblock", "Sniper Roadblock" },
			{ "Railroad To Port", "Railroad To Port" },
			{ "Trailer Park Workers Shack", "Trailer Park Workers Shack" },
			{ "Railroad To Tarkov", "Railroad To Tarkov" },
			{ "RUAF Roadblock_scav", "RUAF Roadblock" },
			{ "Warehouse 17", "Warehouse 17" },
			{ "Factory Shacks", "Factory Shacks" },
			{ "Warehouse 4", "Warehouse 4" },
			{ "Old Azs Gate", "Old Gas Station" },
			{ "Factory Far Corner", "Factory Far Corner" },
			{ "Administration Gate", "Administration Gate" },
			{ "Military Checkpoint", "Scav Checkpoint" },
			{ "Customs_scav_pmc", "Boiler Room Basement (Co-op)" },
		}},

		// ==================== LIGHTHOUSE ====================
		{ "lighthouse", {
			// PMC
			{ "V-Ex_light", "Road to Military Base V-Ex" },
			{ "tunnel_shared", "Side Tunnel (Co-OP)" },
			{ "Alpinist_light", "Mountain Pass" },
			{ "Shorl_free", "Path to Shoreline" },
			{ "Nothern_Checkpoint", "Northern Checkpoint" },
			{ "Coastal_South_Road", "Southern Road" },
			{ "EXFIL_Train", "Armored Train" },
			{ "lighthouse_secret_minefield", "Passage by the Lake (Secret)" },
			// SCAV
			{ "Side Tunnel (Co-Op)", "Side Tunnel (Co-OP)" },
			{ "Shorl_free_scav", "Path to Shoreline" },
			{ "Scav_Coastal_South", "Southern Road" },
			{ "Scav_Underboat_Hideout", "Hideout Under the Landing Stage" },
			{ "Scav_Hideout_at_the_grotto", "Scav Hideout at the Grotto" },
			{ "Scav_Industrial_zone", "Industrial Zone Gates" },
			{ "Armored Train", "Armored Train" },
		}},

		// ==================== STREETS ====================
		{ "tarkovstreets", {
			// PMC
			{ "E8_yard", "Courtyard" },
			{ "E7_car", "Primorsky Ave Taxi V-Ex" },
			{ "E1", "Stylobate Building Elevator" },
			{ "E4", "Crash Site" },
			{ "E2", "Sewer River" },
			{ "E3", "Damaged House" },
			{ "E5", "Collapsed Crane" },
			{ "E6", "??" },
			{ "E9_sniper", "Klimov Street" },
			{ "Exit_E10_coop", "Pinewood Basement (Co-OP)" },
			{ "E7", "Expo Checkpoint" },
			{ "streets_secret_onyx", "Smugglers' Basement (Secret)" },
			// SCAV
			{ "scav_e1", "Basement Descent" },
			{ "scav_e2", "Entrance to Catacombs" },
			{ "scav_e3", "Ventilation Shaft" },
			{ "scav_e4", "Sewer Manhole" },
			{ "scav_e5", "Near Kamchatskaya Arch" },
			{ "scav_e7", "Cardinal Apartment Complex Parking" },
			{ "scav_e8", "Klimov Shopping Mall Exfil" },
			{ "scav_e6", "Pinewood Basement (Co-OP)" },
		}},

		// ==================== GROUND ZERO ====================
		{ "sandbox", {
			// PMC
			{ "Sandbox_VExit", "Police Cordon V-Ex" },
			{ "Unity_free_exit", "Emercom Checkpoint" },
			{ "Scav_coop_exit", "Scav Checkpoint (Co-OP)" },
			{ "Nakatani_stairs_free_exit", "Nakatani Basement Stairs" },
			{ "Sniper_exit", "Mira Ave" },
			{ "groundzero_secret_adaptation", "Tartowers Sales Office (Secret)" },
			// SCAV
			{ "Scav Checkpoint (Co-Op)", "Scav Checkpoint (Co-OP)" },
			{ "Emercom Checkpoint", "Emercom Checkpoint" },
			{ "Nakatani Basement Stairs", "Nakatani Basement Stairs" },
		}},

		// ==================== GROUND ZERO HIGH ====================
		{ "sandbox_high", {
			// PMC
			{ "Sandbox_VExit", "Police Cordon V-Ex" },
			{ "Unity_free_exit", "Emercom Checkpoint" },
			{ "Scav_coop_exit", "Scav Checkpoint (Co-OP)" },
			{ "Nakatani_stairs_free_exit", "Nakatani Basement Stairs" },
			{ "Sniper_exit", "Mira Ave" },
			{ "groundzero_secret_adaptation", "Tartowers Sales Office (Secret)" },
			// SCAV
			{ "Scav Checkpoint (Co-Op)", "Scav Checkpoint (Co-OP)" },
			{ "Emercom Checkpoint", "Emercom Checkpoint" },
			{ "Nakatani Basement Stairs", "Nakatani Basement Stairs" },
		}},
	};

	// Get friendly display name for an exfil
	// Returns the original name if no translation found
	inline std::string GetDisplayName(const std::string& mapId, const std::string& internalName)
	{
		auto mapIt = AllExfilNames.find(mapId);
		if (mapIt != AllExfilNames.end())
		{
			auto exfilIt = mapIt->second.find(internalName);
			if (exfilIt != mapIt->second.end())
			{
				return exfilIt->second;
			}
		}
		// Return original name if not found
		return internalName;
	}
}
