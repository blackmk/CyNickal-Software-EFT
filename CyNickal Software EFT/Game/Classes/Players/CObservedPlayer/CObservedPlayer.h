#pragma once
#include "Game/Classes/Players/CBaseEFTPlayer/CBaseEFTPlayer.h"

// Bitmask flags for player tag status - values must match EFT's ETagStatus enum
enum class ETagStatus : uint32_t
{
	Unaware = 1,
	Aware = 2,
	Combat = 4,
	Solo = 8,
	Coop = 16,
	Bear = 32,
	Usec = 64,
	Scav = 128,
	TargetSolo = 256,
	TargetMultiple = 512,
	Healthy = 1024,
	Injured = 2048,
	BadlyInjured = 4096,
	Dying = 8192,
	Birdeye = 16384,
	Knight = 32768,
	BigPipe = 65536,
	BlackDivision = 131072,
	VSRF = 262144,
};

// Helper function to get human-readable health status string
inline const char* GetHealthStatusString(uint32_t tagStatus)
{
	if (tagStatus & static_cast<uint32_t>(ETagStatus::Dying))
		return "Dying";
	if (tagStatus & static_cast<uint32_t>(ETagStatus::BadlyInjured))
		return "Badly Injured";
	if (tagStatus & static_cast<uint32_t>(ETagStatus::Injured))
		return "Injured";
	if (tagStatus & static_cast<uint32_t>(ETagStatus::Healthy))
		return "Healthy";
	return "Unknown";
}

class CObservedPlayer : public CBaseEFTPlayer
{
private:
	uintptr_t m_PlayerControllerAddress{ 0 };
	uintptr_t m_MovementControllerAddress{ 0 };
	uintptr_t m_ObservedMovementStateAddress{ 0 };
	uintptr_t m_ObservedPlayerHandsControllerAddress{ 0 };
	uintptr_t m_HealthControllerAddress{ 0 };
	uintptr_t m_VoiceAddress{ 0 };
	uintptr_t m_ObservedHandsControllerAddress{ 0 };
	uintptr_t m_PreviousHandsControllerAddress{ 0 };
	wchar_t m_wVoice[32]{ 0 };

public:
	uint32_t m_TagStatus{ std::numeric_limits<uint32_t>::max() };
	char m_Voice[32]{ 0 };

public:
	CObservedPlayer(uintptr_t EntityAddress) : CBaseEFTPlayer(EntityAddress) {}
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
	void Finalize();
	void QuickFinalize();
	void QuickRead(VMMDLL_SCATTER_HANDLE vmsh);
	const bool IsInCondition(const ETagStatus status) const;
	const bool IsInCriticalHealthStatus() const;

private:
	void SetSpawnTypeFromVoice();
};