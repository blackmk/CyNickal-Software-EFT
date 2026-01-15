#pragma once
#include "Game/Classes/CBaseEntity/CBaseEntity.h"
#include "Game/Classes/CFireportTransform/CFireportTransform.h"
#include "Game/Classes/Vector.h"
#include "Game/Ballistics/Ballistics.h"

class CFirearmManager
{
public:
	CFirearmManager(uintptr_t HandsControllerAddress);

	void Update(VMMDLL_SCATTER_HANDLE vmsh);
	void QuickUpdate(VMMDLL_SCATTER_HANDLE vmsh);
	void Finalize();
	void Reset();

	bool IsValid() const { return m_bHasFireport; }
	Vector3 GetFireportPosition() const { return m_FireportPosition; }
	Quaternion GetFireportRotation() const { return m_FireportRotation; }

	// Ammo tracking for random bone selection
	uint32_t GetCurrentAmmoCount() const { return m_CurrentAmmoCount; }
	uintptr_t GetHandsControllerAddress() const { return m_HandsControllerAddress; }
	void RefreshAmmoCount();

	// Ballistics
	void UpdateBallistics();
	BallisticsInfo GetBallistics() const { return m_CachedBallistics; }

private:
	uintptr_t m_HandsControllerAddress{ 0 };

	uintptr_t m_FireportAddress{ 0 };
	uintptr_t m_TransformInternalAddress{ 0 };
	
	std::unique_ptr<CFireportTransform> m_pFireportTransform{ nullptr };
	
	Vector3 m_FireportPosition{};
	Quaternion m_FireportRotation{};
	bool m_bHasFireport{ false };

	// Ballistics Cache
	BallisticsInfo m_CachedBallistics{};
	uintptr_t m_LastAmmoTemplate{ 0 };
	uint64_t m_LastBallisticsUpdate{ 0 }; // Timestamp

	// Ammo count for random bone tracking
	uint32_t m_CurrentAmmoCount{ 0 };
	uint64_t m_LastAmmoUpdateMs{ 0 };
};
