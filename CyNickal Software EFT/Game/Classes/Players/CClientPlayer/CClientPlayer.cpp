#include "pch.h"

#include "CClientPlayer.h"

#include "Game/Offsets/Offsets.h"

void CClientPlayer::PrepareRead_1(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_1(vmsh, EPlayerType::eMainPlayer);

	if (IsInvalid())
		return;

	VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CPlayer::pMovementContext, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_MovementContextAddress), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CPlayer::pProfile, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_ProfileAddress), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CPlayer::pHandsController, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_HandsControllerAddress), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CPlayer::pProceduralWeaponAnimation, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_ProceduralWeaponAnimationAddress), nullptr);
}

void CClientPlayer::PrepareRead_2(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_2(vmsh);

	if (!m_HandsControllerAddress)
		SetInvalid();

	if (IsInvalid())
		return;

	m_pHands = std::make_unique<CHeldItem>(m_HandsControllerAddress);
	m_PreviousHandsControllerAddress = m_HandsControllerAddress;
	m_pHands->PrepareRead_1(vmsh, EPlayerType::eMainPlayer);

	VMMDLL_Scatter_PrepareEx(vmsh, m_ProfileAddress + Offsets::CProfile::pProfileInfo, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_ProfileInfoAddress), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_MovementContextAddress + Offsets::CMovementContext::Rotation, sizeof(float), reinterpret_cast<BYTE*>(&m_Yaw), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_ProceduralWeaponAnimationAddress + Offsets::CProceduralWeaponAnimation::pOptics, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_OpticsAddress), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_ProceduralWeaponAnimationAddress + Offsets::CProceduralWeaponAnimation::bAiming, sizeof(std::byte), reinterpret_cast<BYTE*>(&m_AimingByte), nullptr);

	// Initialize firearm manager for aimline support
	if (m_HandsControllerAddress)
	{
		m_pFirearmManager = std::make_unique<CFirearmManager>(m_HandsControllerAddress);
		m_pFirearmManager->Update(vmsh);
	}
}

void CClientPlayer::PrepareRead_3(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_3(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_2(vmsh);

	VMMDLL_Scatter_PrepareEx(vmsh, m_ProfileInfoAddress + Offsets::CProfileInfo::Side, sizeof(uint32_t), reinterpret_cast<BYTE*>(&m_Side), nullptr);
}

void CClientPlayer::PrepareRead_4(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_4(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_3(vmsh);
}

void CClientPlayer::PrepareRead_5(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_5(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_4(vmsh);
}

void CClientPlayer::PrepareRead_6(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_6(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_5(vmsh);
}

void CClientPlayer::PrepareRead_7(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_7(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_6(vmsh);
}

void CClientPlayer::PrepareRead_8(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_8(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_7(vmsh);
}

void CClientPlayer::PrepareRead_9(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_9(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_8(vmsh);
}

void CClientPlayer::PrepareRead_10(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::PrepareRead_10(vmsh);

	if (IsInvalid())
		return;

	m_pHands->PrepareRead_9(vmsh);
}

void CClientPlayer::PrepareRead_11(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (IsInvalid())
		return;

	m_pHands->PrepareRead_10(vmsh);
}

/* Empty methods to keep interface same as CObservedPlayer */
void CClientPlayer::PrepareRead_12(VMMDLL_SCATTER_HANDLE vmsh)
{
}

void CClientPlayer::PrepareRead_13(VMMDLL_SCATTER_HANDLE vmsh)
{
}

void CClientPlayer::PrepareRead_14(VMMDLL_SCATTER_HANDLE vmsh)
{
}

void CClientPlayer::QuickRead(VMMDLL_SCATTER_HANDLE vmsh)
{
	CBaseEFTPlayer::QuickRead(vmsh, EPlayerType::eMainPlayer);

	if (IsInvalid())
		return;

	m_bScoped = false;

	if (m_pHands)
		m_pHands->QuickRead(vmsh, EPlayerType::eMainPlayer);

	VMMDLL_Scatter_PrepareEx(vmsh, m_MovementContextAddress + Offsets::CMovementContext::Rotation, sizeof(float), reinterpret_cast<BYTE*>(&m_Yaw), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_EntityAddress + Offsets::CPlayer::pHandsController, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_HandsControllerAddress), nullptr);
	VMMDLL_Scatter_PrepareEx(vmsh, m_ProceduralWeaponAnimationAddress + Offsets::CProceduralWeaponAnimation::bAiming, sizeof(std::byte), reinterpret_cast<BYTE*>(&m_AimingByte), nullptr);

	if (m_ProceduralWeaponAnimationAddress)
	{
		VMMDLL_Scatter_PrepareEx(vmsh, m_ProceduralWeaponAnimationAddress + Offsets::CProceduralWeaponAnimation::pOptics, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_OpticsAddress), nullptr);
	}

	if (m_OpticsAddress)
	{
		VMMDLL_Scatter_PrepareEx(vmsh, m_OpticsAddress + Offsets::CUnityList::ArrayBase, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_OpticItemsArray), nullptr);
		VMMDLL_Scatter_PrepareEx(vmsh, m_OpticsAddress + Offsets::CUnityList::Count, sizeof(int32_t), reinterpret_cast<BYTE*>(&m_OpticCount), nullptr);
	}

	if (m_OpticItemsArray)
		VMMDLL_Scatter_PrepareEx(vmsh, m_OpticItemsArray + 0x20, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_FirstSightNBone), nullptr);

	if (m_FirstSightNBone)
		VMMDLL_Scatter_PrepareEx(vmsh, m_FirstSightNBone + Offsets::CSightNBone::pMod, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_FirstSightComponent), nullptr);

	if (m_FirstSightComponent)
		VMMDLL_Scatter_PrepareEx(vmsh, m_FirstSightComponent + Offsets::CSightComponent::ScopeZoomValue, sizeof(float), reinterpret_cast<BYTE*>(&m_ScopeZoomValue), nullptr);

	// Variable zoom scope support - read additional data for jagged array traversal
	// These reads use values from the previous frame (1-frame lag is acceptable for zoom changes)
	if (m_FirstSightComponent)
	{
		// Read SelectedScope (which optic index is active) and ScopesSelectedModes pointer
		VMMDLL_Scatter_PrepareEx(vmsh, m_FirstSightComponent + Offsets::CSightComponent::SelectedScope, sizeof(int32_t), reinterpret_cast<BYTE*>(&m_SelectedScope), nullptr);
		VMMDLL_Scatter_PrepareEx(vmsh, m_FirstSightComponent + Offsets::CSightComponent::ScopesSelectedModes, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_ScopesSelectedModesPtr), nullptr);
		VMMDLL_Scatter_PrepareEx(vmsh, m_FirstSightComponent + Offsets::CSightComponent::pTemplate, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_SightTemplatePtr), nullptr);
	}

	// Read Zooms array pointer from SightInterface (uses previous frame's SightTemplate)
	if (m_SightTemplatePtr)
	{
		VMMDLL_Scatter_PrepareEx(vmsh, m_SightTemplatePtr + Offsets::CSightInterface::Zooms, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_ZoomsArrayPtr), nullptr);
	}

	// Read the selected mode from ScopesSelectedModes[SelectedScope] (uses previous frame's values)
	if (m_ScopesSelectedModesPtr && m_SelectedScope >= 0 && m_SelectedScope < 10)
	{
		// ScopesSelectedModes is an int32[] - read the mode index for the selected scope
		// Array structure: [ArrayBase + 0x20] + index * sizeof(int32_t)
		uintptr_t modeAddr = m_ScopesSelectedModesPtr + 0x20 + (static_cast<uint32_t>(m_SelectedScope) * sizeof(int32_t));
		VMMDLL_Scatter_PrepareEx(vmsh, modeAddr, sizeof(int32_t), reinterpret_cast<BYTE*>(&m_SelectedScopeMode), nullptr);
	}

	// Read the scope's zoom array pointer from Zooms[SelectedScope] (uses previous frame's values)
	if (m_ZoomsArrayPtr && m_SelectedScope >= 0 && m_SelectedScope < 10)
	{
		// Zooms is a float[][] (jagged array) - first get the inner array pointer
		// Structure: [ArrayBase + 0x20] + index * sizeof(uintptr_t) -> float[]
		uintptr_t scopeZoomArrayAddr = m_ZoomsArrayPtr + 0x20 + (static_cast<uint32_t>(m_SelectedScope) * sizeof(uintptr_t));
		VMMDLL_Scatter_PrepareEx(vmsh, scopeZoomArrayAddr, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&m_ZoomsScopeArray), nullptr);
	}

	// Read the actual zoom value from Zooms[SelectedScope][SelectedScopeMode] (uses previous frame's values)
	if (m_ZoomsScopeArray && m_SelectedScopeMode >= 0 && m_SelectedScopeMode < 10)
	{
		// Inner array is float[] - read the zoom value at the selected mode index
		uintptr_t zoomValueAddr = m_ZoomsScopeArray + 0x20 + (static_cast<uint32_t>(m_SelectedScopeMode) * sizeof(float));
		VMMDLL_Scatter_PrepareEx(vmsh, zoomValueAddr, sizeof(float), reinterpret_cast<BYTE*>(&m_FallbackZoomValue), nullptr);
	}

	// Update firearm manager for aimline
	if (m_pFirearmManager)
	{
		m_pFirearmManager->Update(vmsh);
		m_pFirearmManager->QuickUpdate(vmsh);
	}
}


void CClientPlayer::Finalize()
{
	CBaseEFTPlayer::Finalize();

	if (IsInvalid())
		return;
}

void CClientPlayer::QuickFinalize()
{
	CBaseEFTPlayer::QuickFinalize();

	if (IsInvalid())
		return;

	if (!IsAiming() || m_OpticsAddress == 0)
	{
		m_OpticCount = 0;
		m_FirstSightComponent = 0;
		m_ScopeZoomValue = 1.0f;
		m_FallbackZoomValue = 1.0f;
		m_bScoped = false;
		// Reset zoom chain pointers
		m_SightTemplatePtr = 0;
		m_ZoomsArrayPtr = 0;
		m_ScopesSelectedModesPtr = 0;
		m_ZoomsScopeArray = 0;
		m_SelectedScope = 0;
		m_SelectedScopeMode = 0;
	}

	if (m_OpticCount <= 0 || m_FirstSightComponent == 0)
	{
		m_ScopeZoomValue = 1.0f;
		m_FallbackZoomValue = 1.0f;
	}

	// Use fallback zoom from jagged array if ScopeZoomValue is 0 or invalid
	// This handles variable zoom scopes (VUDU, Razor, etc.) that don't update ScopeZoomValue
	if (m_ScopeZoomValue <= 0.0f || m_ScopeZoomValue > 100.0f)
	{
		// ScopeZoomValue is invalid, use fallback if available
		if (m_FallbackZoomValue > 0.0f && m_FallbackZoomValue < 100.0f)
			m_ScopeZoomValue = m_FallbackZoomValue;
		else
			m_ScopeZoomValue = 1.0f;
	}

	// Trigger scoped state for ANY optic (including 1x red dots/holos)
	// The optic radius calculation will handle zoom scaling appropriately
	m_bScoped = IsAiming() && m_OpticCount > 0 && m_FirstSightComponent != 0;

	// Finalize firearm manager for aimline
	if (m_pFirearmManager)
		m_pFirearmManager->Finalize();

	if (m_HandsControllerAddress == m_PreviousHandsControllerAddress) return;


	m_PreviousHandsControllerAddress = m_HandsControllerAddress;
	m_pHands = std::make_unique<CHeldItem>(m_HandsControllerAddress);
	m_pHands->CompleteUpdate(EPlayerType::eMainPlayer);

	// Recreate firearm manager when hands controller changes
	if (m_HandsControllerAddress)
		m_pFirearmManager = std::make_unique<CFirearmManager>(m_HandsControllerAddress);
}