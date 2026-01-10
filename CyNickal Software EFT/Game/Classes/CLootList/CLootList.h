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

private:
	void CompleteUpdate(DMA_Connection* Conn);
	void GetAndSortEntityAddresses(DMA_Connection* Conn);
	void PopulateTypeAddressCache(DMA_Connection* Conn);
	std::unordered_map<std::string, uintptr_t> ObjectTypeAddressCache{};
	std::vector<uintptr_t> m_UnsortedAddresses{};
	uintptr_t m_BaseLootListAddress{ 0 };
	uint32_t m_LootNum{ 0 };
};