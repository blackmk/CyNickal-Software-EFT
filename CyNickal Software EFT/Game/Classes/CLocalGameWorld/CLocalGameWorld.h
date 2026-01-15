#pragma once
#include "Game/Classes/CBaseEntity/CBaseEntity.h"
#include "Game/Classes/CLootList/CLootList.h"
#include "Game/Classes/CRegisteredPlayers/CRegisteredPlayers.h"
#include "Game/Classes/CExfilController/CExfilController.h"

class CLocalGameWorld : public CBaseEntity
{
public:
	CLocalGameWorld(uintptr_t GameWorldAddress);

public:
	void QuickUpdatePlayers(DMA_Connection* Conn);
	void HandlePlayerAllocations(DMA_Connection* Conn);

public:
	std::unique_ptr<class CLootList> m_pLootList{ nullptr };
	std::unique_ptr<class CRegisteredPlayers> m_pRegisteredPlayers{ nullptr };
	std::unique_ptr<class CExfilController> m_pExfilController{ nullptr };
};