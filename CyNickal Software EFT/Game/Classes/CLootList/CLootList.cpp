#include "pch.h"
#include "CLootList.h"
#include "Game/Offsets/Offsets.h"

CLootList::CLootList(uintptr_t LootListAddress) : CBaseEntity(LootListAddress)
{
	std::println("[CLootList] Constructed CLootList with 0x{:X} - awaiting first refresh", LootListAddress);
	// Don't load immediately - let the DMA thread handle the first load after a delay
	// This prevents race conditions where loot list isn't fully populated yet
}

void CLootList::CompleteUpdate(DMA_Connection* Conn)
{
	GetAndSortEntityAddresses(Conn);

	m_LootableContainers.CompleteUpdate(Conn);
	m_ObservedItems.CompleteUpdate(Conn);
}

std::vector<uintptr_t> DerefPointerVec(DMA_Connection* Conn, std::vector<uintptr_t>& Pointers)
{
	std::vector<uintptr_t> Return{};
	Return.resize(Pointers.size());

	auto vmsh = VMMDLL_Scatter_Initialize(Conn->GetHandle(), EFT::GetProcess().GetPID(), VMMDLL_FLAG_NOCACHE);
	for (auto&& [Index, Addr] : std::views::enumerate(Pointers))
	{
		if (Addr == 0)
			continue;

		VMMDLL_Scatter_PrepareEx(vmsh, Addr, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&Return[Index]), nullptr);
	}
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_CloseHandle(vmsh);

	return Return;
}

void CLootList::GetAndSortEntityAddresses(DMA_Connection* Conn)
{
	auto& Proc = EFT::GetProcess();

	m_BaseLootListAddress = Proc.ReadMem<uintptr_t>(Conn, m_EntityAddress + 0x10);
	m_LootNum = Proc.ReadMem<uint32_t>(Conn, m_EntityAddress + 0x18);
	m_UnsortedAddresses = Proc.ReadVec<uintptr_t>(Conn, m_BaseLootListAddress + 0x20, m_LootNum);

	if (ObjectTypeAddressCache.empty())
		PopulateTypeAddressCache(Conn);

	uintptr_t ObservedLootTypeAddress = ObjectTypeAddressCache["ObservedLootItem"];
	uintptr_t LootableContainerTypeAddress = ObjectTypeAddressCache["LootableContainer"];

	auto DerefLootAddresses = DerefPointerVec(Conn, m_UnsortedAddresses);

	m_ObservedItems.m_EntityAddresses.clear();
	m_LootableContainers.m_EntityAddresses.clear();
	for (auto&& [Index, Addr] : std::views::enumerate(DerefLootAddresses))
	{
		if (Addr == 0)
			continue;

		if (Addr == ObservedLootTypeAddress)
			m_ObservedItems.m_EntityAddresses.push_back(m_UnsortedAddresses[Index]);
		else if (Addr == LootableContainerTypeAddress)
			m_LootableContainers.m_EntityAddresses.push_back(m_UnsortedAddresses[Index]);
	}

	std::println("[CLootList] Found {} ObservedLootItems and {} LootableContainers", m_ObservedItems.m_EntityAddresses.size(), m_LootableContainers.m_EntityAddresses.size());
}

void CLootList::PopulateTypeAddressCache(DMA_Connection* Conn)
{
	ObjectTypeAddressCache.clear();

	auto& Proc = EFT::GetProcess();

	auto DerefLootAddresses = DerefPointerVec(Conn, m_UnsortedAddresses);

	std::ranges::sort(DerefLootAddresses);
	auto ret = std::unique(DerefLootAddresses.begin(), DerefLootAddresses.end());
	DerefLootAddresses.erase(ret, DerefLootAddresses.end());

	std::vector<std::array<char, 64>> TypeNameBuffers{};
	TypeNameBuffers.resize(DerefLootAddresses.size());

	for (auto&& [Index, Addr] : std::views::enumerate(DerefLootAddresses))
	{
		if (Addr == 0)
			continue;

		uintptr_t NameAddr = Proc.ReadMem<uintptr_t>(Conn, Addr + 0x10);
		TypeNameBuffers[Index] = Proc.ReadMem<std::array<char, 64>>(Conn, NameAddr);
		std::string TypeNameStr = std::string(TypeNameBuffers[Index].begin(), TypeNameBuffers[Index].end());
		ObjectTypeAddressCache[TypeNameStr.c_str()] = Addr;
		std::println("[CLootList] Cached Type: {0} at {1:X}", TypeNameStr.c_str(), Addr);
	}
}

void CLootList::RefreshLoot(DMA_Connection* Conn)
{
	try
	{
		CompleteUpdate(Conn);
		m_bInitialized = true;
		std::println("[CLootList] Loot refreshed: {} items, {} containers", 
			m_ObservedItems.m_Entities.size(), m_LootableContainers.m_Entities.size());
	}
	catch (const std::exception& e)
	{
		std::println("[CLootList] RefreshLoot exception: {}", e.what());
	}
	catch (...)
	{
		std::println("[CLootList] RefreshLoot unknown exception");
	}
}

void CLootList::ResetTypeCache()
{
	ObjectTypeAddressCache.clear();
	m_bInitialized = false;
	std::println("[CLootList] Type cache reset for new raid");
}