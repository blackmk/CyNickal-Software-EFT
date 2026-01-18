#include "pch.h"
#include "EFT.h"
#include "Game/GOM/GOM.h"
#include "Game/Camera List/Camera List.h"
#include "Game/Response Data/Response Data.h"
#include "Game/Offsets/Offsets.h"
#include <algorithm>

static inline bool g_IsInRaid{ false };
static inline bool g_DiscoveryPending{ false };
static inline int g_InvalidRaidReads{ 0 };

bool EFT::Initialize(DMA_Connection* Conn)
{
	std::println("Initializing EFT module...");

	Proc.GetProcessInfo(Conn);

	MakeNewGameWorld(Conn);

	ResponseData::Initialize(Conn);

	return true;
}

const Process& EFT::GetProcess()
{
	return Proc;
}

bool EFT::TryMakeNewGameWorld(DMA_Connection* Conn)
{
	if (IsGameWorldInitialized()) return true;

	// Always ensure we're in a clean state for a new attempt
	g_DiscoveryPending = true;
	g_IsInRaid = false;
	
	// Logging to confirm we are actually trying (debugging user report of "stops looking")
	std::println("[EFT] TryMakeNewGameWorld: Attempting to find GameWorld...");

	try
	{
		// Force a fresh GOM scan every time we try
		GOM::ResetCache();
		if (!GOM::Initialize(Conn))
		{
			std::println("[EFT] TryMakeNewGameWorld: GOM::Initialize failed.");
			return false;
		}

		auto gameWorldAddr = GOM::FindGameWorldAddressFromCache(Conn);
		std::println("[EFT] TryMakeNewGameWorld: Found GameWorld at 0x{:X}", gameWorldAddr);
		
		pGameWorld = std::make_unique<CLocalGameWorld>(gameWorldAddr);
		CameraList::Initialize(Conn);
		
		g_IsInRaid = true;
		g_InvalidRaidReads = 0;
		g_DiscoveryPending = false;
		
		std::println("[EFT] TryMakeNewGameWorld: Success! Attached to raid.");
		return true;
	}
	catch (const std::exception& e)
	{
		std::println("[EFT] Discovery attempt exception: {}", e.what());
		GOM::ResetCache();
		pGameWorld.reset();
		g_IsInRaid = false;
		g_DiscoveryPending = true;
		return false;
	}
	catch (...)
	{
		std::println("[EFT] Discovery attempt unknown exception.");
		GOM::ResetCache();
		pGameWorld.reset();
		g_IsInRaid = false;
		g_DiscoveryPending = true;
		return false;
	}
}

bool EFT::IsGameWorldInitialized()
{
	return pGameWorld != nullptr;
}

bool EFT::IsInRaid()
{
	return g_IsInRaid;
}

bool EFT::IsDiscoveryPending()
{
	return g_DiscoveryPending;
}

void EFT::UpdateRaidState(DMA_Connection* Conn)
{
	if (!pGameWorld)
	{
		g_IsInRaid = false;
		g_DiscoveryPending = true;
		g_InvalidRaidReads = 0;
		return;
	}

	bool hasMainPlayer = false;
	bool hasPlayers = false;

	try
	{
		auto mainPlayer = Proc.ReadMem<uintptr_t>(Conn, pGameWorld->m_EntityAddress + Offsets::CLocalGameWorld::pMainPlayer);
		hasMainPlayer = mainPlayer != 0;
	}
	catch (...)
	{
	}

	try
	{
		auto registeredPlayers = Proc.ReadMem<uintptr_t>(Conn, pGameWorld->m_EntityAddress + Offsets::CLocalGameWorld::pRegisteredPlayers);
		if (registeredPlayers)
		{
			auto numPlayers = Proc.ReadMem<uint32_t>(Conn, registeredPlayers + Offsets::CRegisteredPlayers::NumPlayers);
			hasPlayers = numPlayers > 0;
		}
	}
	catch (...)
	{
	}

	if (hasMainPlayer && hasPlayers)
	{
		g_IsInRaid = true;
		g_DiscoveryPending = false;
		g_InvalidRaidReads = 0;
		return;
	}

	g_InvalidRaidReads = std::min(g_InvalidRaidReads + 1, 10);
	if (g_InvalidRaidReads >= 3)
	{
		if (g_IsInRaid)
		{
			std::println("[EFT] Raid ended or invalid state detected. Clearing GameWorld for next raid.");
		}
		g_IsInRaid = false;
		g_DiscoveryPending = true;
		pGameWorld.reset();
		GOM::ResetCache();
	}
}

void EFT::MakeNewGameWorld(DMA_Connection* Conn)
{
	TryMakeNewGameWorld(Conn);
}

void EFT::QuickUpdatePlayers(DMA_Connection* Conn)
{
	if (pGameWorld)
		pGameWorld->QuickUpdatePlayers(Conn);
}

void EFT::HandlePlayerAllocations(DMA_Connection* Conn)
{
	if (pGameWorld)
		pGameWorld->HandlePlayerAllocations(Conn);
}

CRegisteredPlayers& EFT::GetRegisteredPlayers()
{
	if (!pGameWorld)
		throw std::runtime_error("EFT::pGameWorld is null");

	if (!pGameWorld->m_pRegisteredPlayers)
		throw std::runtime_error("EFT::pGameWorld->m_pRegisteredPlayers is null");

	return *(pGameWorld->m_pRegisteredPlayers);
}

CLootList& EFT::GetLootList()
{
	if (!pGameWorld)
		throw std::runtime_error("EFT::pGameWorld is null");

	if (!pGameWorld->m_pLootList)
		throw std::runtime_error("EFT::pGameWorld->m_pLootList is null");

	return *(pGameWorld->m_pLootList);
}

CExfilController& EFT::GetExfilController()
{
	if (!pGameWorld)
		throw std::runtime_error("EFT::pGameWorld is null");

	if (!pGameWorld->m_pExfilController)
		throw std::runtime_error("EFT::pGameWorld->m_pRegisteredExfils is null");

	return *(pGameWorld->m_pExfilController);
}

