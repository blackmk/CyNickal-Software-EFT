#pragma once
#include <unordered_set>
#include <cstdint>

// Player focus and temporary teammate tracking for radar interactions
class PlayerFocus
{
public:
	// === Focused Player (click to track) ===
	static inline uintptr_t FocusedPlayerAddr = 0;

	static bool HasFocusedPlayer() { return FocusedPlayerAddr != 0; }

	static void SetFocusedPlayer(uintptr_t addr) { FocusedPlayerAddr = addr; }

	static void ClearFocus() { FocusedPlayerAddr = 0; }

	// === Temporary Teammates (mark players as friendly for this session) ===
	static inline std::unordered_set<uintptr_t> TempTeammates;

	static bool IsTempTeammate(uintptr_t addr)
	{
		return TempTeammates.find(addr) != TempTeammates.end();
	}

	static void AddTempTeammate(uintptr_t addr)
	{
		TempTeammates.insert(addr);
	}

	static void RemoveTempTeammate(uintptr_t addr)
	{
		TempTeammates.erase(addr);
	}

	static void ClearTempTeammates()
	{
		TempTeammates.clear();
	}

	// === Focus State Cycling ===
	// Cycles: unfocused -> focused -> temp teammate -> unfocused
	enum class FocusState
	{
		None,
		Focused,
		TempTeammate
	};

	static FocusState GetPlayerFocusState(uintptr_t addr)
	{
		if (IsTempTeammate(addr))
			return FocusState::TempTeammate;
		if (FocusedPlayerAddr == addr)
			return FocusState::Focused;
		return FocusState::None;
	}

	static void CycleFocusState(uintptr_t playerAddr)
	{
		FocusState currentState = GetPlayerFocusState(playerAddr);

		switch (currentState)
		{
		case FocusState::None:
			// Clear previous focus and set new
			FocusedPlayerAddr = playerAddr;
			break;

		case FocusState::Focused:
			// Promote to temp teammate
			FocusedPlayerAddr = 0;
			AddTempTeammate(playerAddr);
			break;

		case FocusState::TempTeammate:
			// Remove from temp teammates
			RemoveTempTeammate(playerAddr);
			break;
		}
	}

	// === Group Visualization Settings ===
	static inline bool bShowGroupLines = true;        // Draw lines between group members
	static inline bool bShowFocusHighlight = true;    // Highlight focused player
	static inline float fFocusHighlightRadius = 15.0f; // Radius of focus highlight circle

	// === Colors ===
	static inline ImU32 FocusHighlightColor = IM_COL32(255, 255, 0, 200);     // Yellow highlight
	static inline ImU32 TempTeammateColor = IM_COL32(0, 255, 0, 255);         // Green for temp teammates
	static inline ImU32 GroupLineColor = IM_COL32(100, 100, 255, 150);        // Blue group lines
};
