#pragma once
#include "Game/Classes/CBaseEntity/CBaseEntity.h"
#include "Game/Classes/CUnityTransform/CUnityTransform.h"

class CBaseLootItem : public CBaseEntity
{
public:
	CBaseLootItem(uintptr_t EntityAddress);
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

public:
	Vector3 m_Position{};

private:
	std::unique_ptr<CUnityTransform> m_pTransform{ nullptr };
	uintptr_t m_MonoBehaviorAddress{ 0 };
	uintptr_t m_GameObjectAddress{ 0 };
	uintptr_t m_ComponentsAddress{ 0 };
	uintptr_t m_TransformAddress{ 0 };

protected:
	uintptr_t GetGameObjectAddress() const { return m_GameObjectAddress; }
};