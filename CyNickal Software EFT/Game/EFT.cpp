#include "pch.h"
#include "EFT.h"
#include "Game/GOM/GOM.h"
#include "Game/Camera List/Camera List.h"
#include "Game/Response Data/Response Data.h"
#include "Game/Offsets/Offsets.h"
#include <algorithm>
#include <atomic>
#include <shared_mutex>

static std::atomic<bool> g_IsInRaid{ false };
static std::atomic<int> g_InvalidRaidReads{ 0 };

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
	if (IsGameWorldInitialized())
	{
		if (IsInRaid())
			return true;

		{
			std::unique_lock lock(s_GameWorldMutex);
			pGameWorld.reset();
		}
		GOM::ResetCache();
	}

	// Ensure clean state for new attempt
	g_IsInRaid = false;

	try
	{
		// Force a fresh GOM scan every time we try
		GOM::ResetCache();
		if (!GOM::Initialize(Conn))
			return false;

		auto gameWorldAddr = GOM::FindGameWorldAddressFromCache(Conn);
		
		auto newWorld = std::make_shared<CLocalGameWorld>(gameWorldAddr);
		{
			std::unique_lock lock(s_GameWorldMutex);
			pGameWorld = std::move(newWorld);
		}
		CameraList::Initialize(Conn);
		
		g_IsInRaid = true;
		g_InvalidRaidReads = 0;
		
		return true;
	}
	catch (...)
	{
		// Discovery failed - player likely not in raid yet
		std::println("[EFT] GameWorld discovery failed. Will retry.");
		GOM::ResetCache();
		{
			std::unique_lock lock(s_GameWorldMutex);
			pGameWorld.reset();
		}
		g_IsInRaid = false;
		return false;
	}
}

bool EFT::IsGameWorldInitialized()
{
	return GetGameWorld() != nullptr;
}

bool EFT::IsInRaid()
{
	return g_IsInRaid;
}

std::shared_ptr<CLocalGameWorld> EFT::GetGameWorld()
{
	std::shared_ptr<CLocalGameWorld> result;
	{
		std::shared_lock lock(s_GameWorldMutex);
		result = pGameWorld;
	}
	return result;
}

void EFT::UpdateRaidState(DMA_Connection* Conn)
{
	auto gameWorld = GetGameWorld();
	if (!gameWorld)
	{
		g_IsInRaid = false;
		g_InvalidRaidReads = 0;
		return;
	}

	bool hasMainPlayer = false;
	bool hasPlayers = false;

	try
	{
		auto mainPlayer = Proc.ReadMem<uintptr_t>(Conn, gameWorld->m_EntityAddress + Offsets::CLocalGameWorld::pMainPlayer);
		hasMainPlayer = mainPlayer != 0;
	}
	catch (...)
	{
	}

	try
	{
		auto registeredPlayers = Proc.ReadMem<uintptr_t>(Conn, gameWorld->m_EntityAddress + Offsets::CLocalGameWorld::pRegisteredPlayers);
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
		g_InvalidRaidReads = 0;
		return;
	}

	g_InvalidRaidReads = std::min(g_InvalidRaidReads + 1, 10);
	if (g_InvalidRaidReads >= 3)
	{
		if (g_IsInRaid)
			std::println("[EFT] Raid ended. Clearing GameWorld for next raid.");
		
		g_IsInRaid = false;
		{
			std::unique_lock lock(s_GameWorldMutex);
			pGameWorld.reset();
		}
		GOM::ResetCache();
	}
}

void EFT::MakeNewGameWorld(DMA_Connection* Conn)
{
	TryMakeNewGameWorld(Conn);
}

void EFT::QuickUpdatePlayers(DMA_Connection* Conn)
{
	auto gameWorld = GetGameWorld();
	if (gameWorld)
		gameWorld->QuickUpdatePlayers(Conn);
}

void EFT::HandlePlayerAllocations(DMA_Connection* Conn)
{
	auto gameWorld = GetGameWorld();
	if (gameWorld)
		gameWorld->HandlePlayerAllocations(Conn);
}

CRegisteredPlayers& EFT::GetRegisteredPlayers()
{
	static thread_local std::shared_ptr<CLocalGameWorld> s_GameWorldSnapshot{};
	s_GameWorldSnapshot = GetGameWorld();

	if (!s_GameWorldSnapshot)
		throw std::runtime_error("EFT::pGameWorld is null");

	if (!s_GameWorldSnapshot->m_pRegisteredPlayers)
		throw std::runtime_error("EFT::pGameWorld->m_pRegisteredPlayers is null");

	return *(s_GameWorldSnapshot->m_pRegisteredPlayers);
}

CLootList& EFT::GetLootList()
{
	static thread_local std::shared_ptr<CLocalGameWorld> s_GameWorldSnapshot{};
	s_GameWorldSnapshot = GetGameWorld();

	if (!s_GameWorldSnapshot)
		throw std::runtime_error("EFT::pGameWorld is null");

	if (!s_GameWorldSnapshot->m_pLootList)
		throw std::runtime_error("EFT::pGameWorld->m_pLootList is null");

	return *(s_GameWorldSnapshot->m_pLootList);
}

CExfilController& EFT::GetExfilController()
{
	static thread_local std::shared_ptr<CLocalGameWorld> s_GameWorldSnapshot{};
	s_GameWorldSnapshot = GetGameWorld();

	if (!s_GameWorldSnapshot)
		throw std::runtime_error("EFT::pGameWorld is null");

	if (!s_GameWorldSnapshot->m_pExfilController)
		throw std::runtime_error("EFT::pGameWorld->m_pExfilController is null");

	return *(s_GameWorldSnapshot->m_pExfilController);
}

void EFT::HandleLootUpdates(DMA_Connection* Conn)
{
	auto gameWorld = GetGameWorld();
	if (!gameWorld || !gameWorld->m_pLootList)
		return;

	gameWorld->RefreshLoot(Conn);
}
