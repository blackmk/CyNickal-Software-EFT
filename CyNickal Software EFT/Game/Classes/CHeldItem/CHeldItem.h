#pragma once
#include "Game/Classes/CBaseEntity/CBaseEntity.h"
#include "Game/Classes/CItem/CItem.h"
#include "Game/Classes/CMagazine/CMagazine.h"
#include "Game/Enums/EPlayerType.h"

class CHeldItem : public CBaseEntity
{
public:
	CHeldItem(uintptr_t EntityAddress);
	void PrepareRead_1(VMMDLL_SCATTER_HANDLE vmsh, EPlayerType PlayerType);
	void PrepareRead_2(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_3(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_4(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_5(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_6(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_7(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_8(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_9(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_10(VMMDLL_SCATTER_HANDLE vmsh);
	void QuickRead(VMMDLL_SCATTER_HANDLE vmsh, EPlayerType PlayerType);
	void Finalize();
	void QuickFinalize();
	void CompleteUpdate(EPlayerType PlayerType);

public:
	std::unique_ptr <CItem> m_pHeldItem{ nullptr };
	std::unique_ptr <CMagazine> m_pMagazine{ nullptr };

private:
	uintptr_t m_HeldItemAddress{ 0 };
	uintptr_t m_PreviousHeldItemAddress{ 0 };
	uintptr_t m_MagazineAddress{ 0 };
};