#include "pch.h"
#include "CLocalGameWorld.h"
#include "Game/Offsets/Offsets.h"
#include "DMA/DMA.h"
#include "Game/EFT.h"

CLocalGameWorld::CLocalGameWorld(uintptr_t GameWorldAddress) : CBaseEntity(GameWorldAddress)
{
	std::println("[CLocalGameWorld] Created CLocalGameWorld at address: 0x{:X}", GameWorldAddress);

	auto Conn = DMA_Connection::GetInstance();
	auto& Proc = EFT::GetProcess();

	uintptr_t LootListAddress = Proc.ReadMem<uintptr_t>(Conn, GameWorldAddress + Offsets::CLocalGameWorld::pLootList);
	m_pLootList = std::make_unique<CLootList>(LootListAddress);

	uintptr_t RegisteredPlayersAddress = Proc.ReadMem<uintptr_t>(Conn, GameWorldAddress + Offsets::CLocalGameWorld::pRegisteredPlayers);
	m_pRegisteredPlayers = std::make_unique<CRegisteredPlayers>(RegisteredPlayersAddress);

	uintptr_t ExfiltrationControllerAddress = Proc.ReadMem<uintptr_t>(Conn, GameWorldAddress + Offsets::CLocalGameWorld::pExfiltrationController);
	m_pExfilController = std::make_unique<CExfilController>(ExfiltrationControllerAddress);
}

void CLocalGameWorld::QuickUpdatePlayers(DMA_Connection* Conn)
{
	if(m_pRegisteredPlayers)
		m_pRegisteredPlayers->QuickUpdate(Conn);
}

void CLocalGameWorld::HandlePlayerAllocations(DMA_Connection* Conn)
{
	if (m_pRegisteredPlayers == nullptr) return;

	m_pRegisteredPlayers->UpdateBaseAddresses(Conn);
	m_pRegisteredPlayers->HandlePlayerAllocations(Conn);
}