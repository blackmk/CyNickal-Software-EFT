#pragma once
#include "Game/Classes/CBaseEntity/CBaseEntity.h"
#include "Game/Classes/CItemTemplate/CItemTemplate.h"

class CMagazine : public CBaseEntity
{
public:
	CMagazine(uintptr_t MagazineSlotAddress);
	void PrepareRead_1(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_2(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_3(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_4(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_5(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_6(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_7(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_8(VMMDLL_SCATTER_HANDLE vmsh);
	void Finalize();
	void QuickRead(VMMDLL_SCATTER_HANDLE vmsh);
	void QuickFinalize();

	const std::string& GetAmmoName() const;

public:
	uint32_t m_MaxCartridges{ 0 };
	uint32_t m_CurrentCartridges{ 0 };

	std::unique_ptr<CItemTemplate> m_pMagazineItemTemplate{ nullptr };
	std::unique_ptr<CItemTemplate> m_pAmmoItemTemplate{ nullptr };

private:
	std::string m_AmmoName{ "" };
	uintptr_t m_ContainedItemAddress{ 0 };
	uintptr_t m_MagazineCartridgesAddress{ 0 };
	uintptr_t m_StackItemsAddress{ 0 };
	uintptr_t m_ListAddress{ 0 };
	uintptr_t m_AmmoItemAddress{ 0 };
	uintptr_t m_AmmoTemplateAddress{ 0 };
	uintptr_t m_MagazineTemplateAddress{ 0 };
};