#pragma once
#include "Game/Classes/Players/CBaseEFTPlayer/CBaseEFTPlayer.h"
#include "Game/Classes/CFirearmManager/CFirearmManager.h"

class CClientPlayer : public CBaseEFTPlayer
{
private:
	uintptr_t m_MovementContextAddress{ 0 };
	uintptr_t m_ProfileAddress{ 0 };
	uintptr_t m_ProfileInfoAddress{ 0 };
	uintptr_t m_HandsControllerAddress{ 0 };
	uintptr_t m_PreviousHandsControllerAddress{ 0 };
	uintptr_t m_ProceduralWeaponAnimationAddress{ 0 };
	uintptr_t m_OpticsAddress{ 0 };
	uintptr_t m_OpticItemsArray{ 0 };
	uintptr_t m_FirstSightNBone{ 0 };
	uintptr_t m_FirstSightComponent{ 0 };
	int32_t m_OpticCount{ 0 };
	float m_ScopeZoomValue{ 1.0f };
	bool m_bScoped{ false };
	std::byte m_AimingByte{ 0 };
	std::unique_ptr<CFirearmManager> m_pFirearmManager{ nullptr };


public:
	CClientPlayer(uintptr_t EntityAddress) : CBaseEFTPlayer(EntityAddress) {}
	void PrepareRead_1(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_2(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_3(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_4(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_5(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_6(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_7(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_8(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_9(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_10(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_11(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_12(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_13(VMMDLL_SCATTER_HANDLE vmsh);
	void PrepareRead_14(VMMDLL_SCATTER_HANDLE vmsh);
	void QuickRead(VMMDLL_SCATTER_HANDLE vmsh);
	void Finalize();
	void QuickFinalize();
	bool IsAiming() const { return m_AimingByte != std::byte{ 0 }; }
	bool IsScoped() const { return m_bScoped; }
	float GetScopeZoom() const { return m_ScopeZoomValue; }
	CFirearmManager* GetFirearmManager() const { return m_pFirearmManager.get(); }
};
