#pragma once
#include "Game/Classes/CBaseEntity/CBaseEntity.h"
#include "Game/Classes/CObservedLootItem/CObservedLootItem.h"
#include "Game/Classes/CLootableContainer/CLootableContainer.h"
#include "DMA/DMA.h"
#include "Game/EFT.h"

#include "CGenericLootList.h"

class CLootList : public CBaseEntity
{
public:
	CLootList(uintptr_t LootListAddress);

public:
	CGenericLootList<CLootableContainer> m_LootableContainers{};
	CGenericLootList<CObservedLootItem> m_ObservedItems{};

	// Public interface for refreshing loot
	void RefreshLoot(DMA_Connection* Conn);
	bool IsInitialized() const { return m_bInitialized; }

	// Static method to reset type address cache (call on raid exit)
	void ResetTypeCache();

private:
	void CompleteUpdate(DMA_Connection* Conn);
	void GetAndSortEntityAddresses(DMA_Connection* Conn);
	void PopulateTypeAddressCache(DMA_Connection* Conn);
	std::unordered_map<std::string, uintptr_t> ObjectTypeAddressCache{};
	std::vector<uintptr_t> m_UnsortedAddresses{};
	uintptr_t m_BaseLootListAddress{ 0 };
	uint32_t m_LootNum{ 0 };
	bool m_bInitialized{ false };
};