#pragma once
#include "DMA/DMA.h"
#include "DMA/Process.h"
#include "Classes/CLocalGameWorld/CLocalGameWorld.h"
#include "Classes/CRegisteredPlayers/CRegisteredPlayers.h"
#include "Classes/CLootList/CLootList.h"
#include "Classes/CExfilController/CExfilController.h"
#include <memory>
#include <shared_mutex>

class EFT
{
public:
	static bool Initialize(DMA_Connection* Conn);
	static const Process& GetProcess();

	static bool TryMakeNewGameWorld(DMA_Connection* Conn);
	static bool IsGameWorldInitialized();
	static bool IsInRaid();
	static void UpdateRaidState(DMA_Connection* Conn);
	static std::shared_ptr<class CLocalGameWorld> GetGameWorld();
private:
	static inline Process Proc{};
	static void MakeNewGameWorld(DMA_Connection* Conn);

public:
	static void QuickUpdatePlayers(DMA_Connection* Conn);
	static void HandlePlayerAllocations(DMA_Connection* Conn);
	static void HandleLootUpdates(DMA_Connection* Conn);

public:
	static class CRegisteredPlayers& GetRegisteredPlayers();
	static class CLootList& GetLootList();
	static class CExfilController& GetExfilController();

private:
	static inline std::shared_ptr<class CLocalGameWorld> pGameWorld{ nullptr };
	static inline std::shared_mutex s_GameWorldMutex{};
};
