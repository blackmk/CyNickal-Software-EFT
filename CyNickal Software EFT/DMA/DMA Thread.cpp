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
	CTimer GameWorld_Retry(std::chrono::seconds(2), [&Conn]() {
		try
		{
			static bool bLoggedWaiting = false;
			static auto LastPendingLog = std::chrono::steady_clock::now() - std::chrono::seconds(15);
			static uint64_t tickCount = 0;
			tickCount++;

			// Periodic tick logging to confirm loop is running
			if (tickCount % 5 == 0)
				std::println("[DMA Thread] GameWorld_Retry tick #{}", tickCount);

			if (EFT::IsGameWorldInitialized())
			{
				bLoggedWaiting = false;
				return;
			}

			if (!bLoggedWaiting)
			{
				std::println("[DMA Thread] Waiting for GameWorld... (Player not in raid?)");
				bLoggedWaiting = true;
			}

			if (EFT::TryMakeNewGameWorld(Conn))
			{
				std::println("[DMA Thread] LocalGameWorld initialized successfully!");
				bLoggedWaiting = false;
				return;
			}

			if (EFT::IsDiscoveryPending())
			{
				auto now = std::chrono::steady_clock::now();
				if (now - LastPendingLog >= std::chrono::seconds(10))
				{
					std::println("[DMA Thread] Discovery still pending, will keep retrying...");
					LastPendingLog = now;
				}
			}
		}
		catch (const std::exception& e)
		{
			std::println("[DMA Thread] GameWorld_Retry exception: {}", e.what());
		}
		catch (...)
		{
			std::println("[DMA Thread] GameWorld_Retry unknown exception");
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
		GameWorld_Retry.Tick(TimeNow);
	}


	Conn->EndConnection();
}