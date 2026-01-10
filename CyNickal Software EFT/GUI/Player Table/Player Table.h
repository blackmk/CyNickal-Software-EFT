#pragma once
#include "Game/Classes/Players/CClientPlayer/CClientPlayer.h"
#include "Game/Classes/Players/CObservedPlayer/CObservedPlayer.h"

class PlayerTable
{
public:
	static void Render();

public:
	static inline bool bMasterToggle{ true };

private:
	static void AddRow(const CClientPlayer& Player);
	static void AddRow(const CObservedPlayer& Player);
};