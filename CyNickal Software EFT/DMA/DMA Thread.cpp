#include "pch.h"
#include "DMA Thread.h"
#include "Input Manager.h"
#include "Game/EFT.h"
#include "Game/Response Data/Response Data.h"

#include "Game/GOM/GOM.h"
#include "Game/Camera List/Camera List.h"
#include "GUI/Aimbot/Aimbot.h"
#include "GUI/Keybinds/Keybinds.h"
#include "DebugMode.h"

extern std::atomic<bool> bRunning;

void DMA_Thread_Main()
{
	std::println("[DMA Thread] DMA Thread started.");

	DMA_Connection* Conn = DMA_Connection::GetInstance();

	if (DebugMode::IsDebugMode())
	{
		std::println("[DMA Thread] Debug mode active - DMA operations disabled.");
		while (bRunning)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		return;
	}

	c_keys::InitKeyboard(Conn);

	if (!EFT::Initialize(Conn))
	{
		std::println("[DMA Thread] EFT Initialization failed, requesting exit.");
		bRunning = false;
		return;
	}

	CTimer LightRefresh(std::chrono::seconds(5), [&Conn]() { Conn->LightRefresh(); });
	CTimer ResponseData(std::chrono::milliseconds(25), [&Conn]() { 
		try { ResponseData::OnDMAFrame(Conn); } 
		catch (...) { /* Not in raid or data unavailable */ }
	});
	CTimer Player_Quick(std::chrono::milliseconds(25), [&Conn]() { 
		try { EFT::QuickUpdatePlayers(Conn); } 
		catch (...) { /* Not in raid */ }
	});
	CTimer Player_Allocations(std::chrono::seconds(5), [&Conn]() { 
		try { EFT::HandlePlayerAllocations(Conn); } 
		catch (...) { /* Not in raid */ }
	});
	CTimer Camera_UpdateViewMatrix(std::chrono::milliseconds(2), [&Conn]() { 
		try { CameraList::QuickUpdateNecessaryCameras(Conn); } 
		catch (...) { /* Not in raid */ }
	});
	CTimer Keybinds(std::chrono::milliseconds(50), [&Conn]() { 
		try { Keybinds::OnDMAFrame(Conn); } 
		catch (...) { /* Keybind error */ }
	});
	CTimer RaidState(std::chrono::seconds(1), [&Conn]() { 
		try { EFT::UpdateRaidState(Conn); } 
		catch (...) { /* State check failed */ }
	});

	// Loot update timer - refreshes loot every 5 seconds
	// Has a 3-second initial delay after GameWorld creation before first load
	static std::chrono::steady_clock::time_point s_GameWorldCreatedTime{};
	static bool s_bInitialLootLoadDone = false;
	CTimer Loot_Update(std::chrono::seconds(5), [&Conn]() {
		try
		{
			if (!EFT::IsInRaid() || !EFT::pGameWorld || !EFT::pGameWorld->m_pLootList)
				return;

			// Check if this is the first load after entering a raid
			if (!s_bInitialLootLoadDone)
			{
				// Wait 3 seconds after GameWorld creation for game to fully populate loot
				auto now = std::chrono::steady_clock::now();
				auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - s_GameWorldCreatedTime);
				if (elapsed.count() < 3)
					return;
				s_bInitialLootLoadDone = true;
			}

			EFT::HandleLootUpdates(Conn);
		}
		catch (...)
		{
			// Loot update failed, will retry
		}
	});

	CTimer GameWorld_Retry(std::chrono::seconds(2), [&Conn]() {
		try
		{
			if (EFT::IsGameWorldInitialized())
				return;

			if (EFT::TryMakeNewGameWorld(Conn))
			{
				std::println("[DMA Thread] LocalGameWorld initialized successfully!");
				// Set timestamp for loot initial delay and reset the flag
				s_GameWorldCreatedTime = std::chrono::steady_clock::now();
				s_bInitialLootLoadDone = false;
			}
		}
		catch (...)
		{
			// Discovery failed, will retry on next tick
		}
	});

	while (bRunning)
	{
		auto TimeNow = std::chrono::high_resolution_clock::now();
		LightRefresh.Tick(TimeNow);
		ResponseData.Tick(TimeNow);
		Player_Quick.Tick(TimeNow);
		Player_Allocations.Tick(TimeNow);
		Camera_UpdateViewMatrix.Tick(TimeNow);
		Keybinds.Tick(TimeNow);
		RaidState.Tick(TimeNow);
		Loot_Update.Tick(TimeNow);
		GameWorld_Retry.Tick(TimeNow);
	}


	Conn->EndConnection();
}