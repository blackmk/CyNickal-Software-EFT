#include "pch.h"
#include "CHeldItem.h"
#include "Game/Offsets/Offsets.h"
#include "DMA/DMA.h"
#include "Game/EFT.h"

CHeldItem::CHeldItem(uintptr_t EntityAddress) : CBaseEntity(EntityAddress) {}

void CHeldItem::PrepareRead_1(VMMDLL_SCATTER_HANDLE vmsh, EPlayerType PlayerType)
{
	if (PlayerType == EPlayerType::eMainPlayer)
		VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CHandsController::pItem, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_HeldItemAddress), reinterpret_cast<DWORD*>(&m_BytesRead));
	else if (PlayerType == EPlayerType::eObservedPlayer)
		VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CObservedPlayerHands::pItem, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_HeldItemAddress), reinterpret_cast<DWORD*>(&m_BytesRead));
}

void CHeldItem::PrepareRead_2(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (m_BytesRead != sizeof(uintptr_t))
		SetInvalid();

	if (!m_HeldItemAddress)
		SetInvalid();

	if (IsInvalid()) return;

	m_pHeldItem = std::make_unique<CItem>(m_HeldItemAddress);
	m_PreviousHeldItemAddress = m_HeldItemAddress;
	m_pHeldItem->PrepareRead_1(vmsh);

	VMMDLL_Scatter_PrepareEx(vmsh, m_HeldItemAddress + Offsets::CItem::pMagslot, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_MagazineAddress), reinterpret_cast<DWORD*>(&m_BytesRead));
}

void CHeldItem::PrepareRead_3(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (m_BytesRead != sizeof(uintptr_t))
		SetInvalid();

	if (IsInvalid()) return;

	if (m_MagazineAddress)
	{
		m_pMagazine = std::make_unique<CMagazine>(m_MagazineAddress);
		m_pMagazine->PrepareRead_1(vmsh);
	}

	m_pHeldItem->PrepareRead_2(vmsh);
}

void CHeldItem::PrepareRead_4(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->PrepareRead_2(vmsh);

	m_pHeldItem->PrepareRead_3(vmsh);
}

void CHeldItem::PrepareRead_5(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->PrepareRead_3(vmsh);
}

void CHeldItem::PrepareRead_6(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->PrepareRead_4(vmsh);
}

void CHeldItem::PrepareRead_7(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->PrepareRead_5(vmsh);
}

void CHeldItem::PrepareRead_8(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->PrepareRead_6(vmsh);
}

void CHeldItem::PrepareRead_9(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->PrepareRead_7(vmsh);
}

void CHeldItem::PrepareRead_10(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->PrepareRead_8(vmsh);
}

void CHeldItem::QuickRead(VMMDLL_SCATTER_HANDLE vmsh, EPlayerType PlayerType)
{
	if (IsInvalid()) return;

	if (PlayerType == EPlayerType::eMainPlayer)
		VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CHandsController::pItem, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_HeldItemAddress), reinterpret_cast<DWORD*>(&m_BytesRead));
	else if (PlayerType == EPlayerType::eObservedPlayer)
		VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CObservedPlayerHands::pItem, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_HeldItemAddress), reinterpret_cast<DWORD*>(&m_BytesRead));

	if(m_pMagazine)
		m_pMagazine->QuickRead(vmsh);
}

void CHeldItem::Finalize()
{
	if (!m_pHeldItem || m_pHeldItem->IsInvalid())
		SetInvalid();

	if (m_pMagazine && m_pMagazine->IsInvalid())
		m_pMagazine = nullptr;

	if (IsInvalid()) return;

	if (m_pMagazine)
		m_pMagazine->Finalize();

	m_pHeldItem->Finalize();
}

void CHeldItem::QuickFinalize()
{
	if (m_BytesRead != sizeof(uintptr_t))
		SetInvalid();

	if (IsInvalid()) return;

	if (m_HeldItemAddress == m_PreviousHeldItemAddress)
		return;

	if (m_HeldItemAddress == 0)
	{
		m_pHeldItem = nullptr;
		m_PreviousHeldItemAddress = 0;
		return;
	}

	m_PreviousHeldItemAddress = m_HeldItemAddress;

	m_pHeldItem = std::make_unique<CItem>(m_HeldItemAddress);
	m_pHeldItem->CompleteUpdate();
}

void CHeldItem::CompleteUpdate(EPlayerType PlayerType)
{
	auto Conn = DMA_Connection::GetInstance();

	auto PID = EFT::GetProcess().GetPID();

	auto vmsh = VMMDLL_Scatter_Initialize(Conn->GetHandle(), PID, VMMDLL_FLAG_NOCACHE);
	PrepareRead_1(vmsh, PlayerType);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_2(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_3(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_4(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_5(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_6(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_7(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_8(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_9(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	PrepareRead_10(vmsh);
	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_Clear(vmsh, PID, VMMDLL_FLAG_NOCACHE);

	VMMDLL_Scatter_CloseHandle(vmsh);

	Finalize();
}