#pragma once
#include "DMA/DMA.h"
#include "Game/Enums/EBoneIndex.h"
#include <string>
#include <cstdint>


// Target bone selection for aimbot
enum class ETargetBone : uint8_t
{
	Head = 0,
	Neck,
	Chest,      // Spine3
	Torso,      // Spine2
	Pelvis,
	RandomBone,
	ClosestVisible
};

// Targeting priority mode
enum class ETargetingMode : uint8_t
{
	ClosestToCrosshair = 0,  // Target closest to aim point (default)
	ClosestDistance          // Target closest by world distance
};

class Aimbot
{
public:
	static void RenderSettings();
	static void RenderFOVCircle(const ImVec2& WindowPos, ImDrawList* DrawList);
	static void OnDMAFrame(DMA_Connection* Conn);

	// Device-agnostic mouse movement - routes to Makcu or KMbox/Net based on settings
	static void SendDeviceMove(int dx, int dy);
	static void NormalizeRandomWeights();

public:

	// General Aimbot Settings
	static inline bool bSettings{ false };
	static inline bool bMasterToggle{ false };
	static inline bool bDrawFOV{ true };
	static inline float fDampen{ 0.95f };
	static inline float fPixelFOV{ 75.0f };
	static inline float fDeadzoneFov{ 2.0f };
	static inline bool bDrawAimline{ false };
	static inline float fAimlineLength{ 100.0f };
	static inline float fAimlineThickness{ 2.0f };
	static inline ImColor aimlineColor{ 255, 255, 255, 255 };

	// Experimental: Fireport-based aiming to compensate for weapon sway
	static inline bool bUseFireportAiming{ false };
	static inline float fFireportProjectionDistance{ 100.0f };

	// Aim point dot visualization (shows where fireport is actually aimed)
	static inline bool bDrawAimPointDot{ false };
	static inline float fAimPointDotSize{ 4.0f };
	static inline ImColor aimPointDotColor{ 0, 255, 255, 255 };  // Cyan

	// Debug visualization (shows both screen center and fireport aim point)
	static inline bool bDrawDebugVisualization{ false };

	// === TARGET BONE SETTINGS ===
	static inline ETargetBone eTargetBone{ ETargetBone::Head };
	
	// Random bone chances (used when eTargetBone == RandomBone)
	static inline float fRandomHeadChance{ 40.0f };    // % chance to aim at head
	static inline float fRandomNeckChance{ 10.0f };    // % chance to aim at neck
	static inline float fRandomChestChance{ 25.0f };   // % chance to aim at chest
	static inline float fRandomTorsoChance{ 15.0f };   // % chance to aim at torso
	static inline float fRandomPelvisChance{ 10.0f };  // % chance to aim at pelvis

	static inline EBoneIndex s_CurrentRandomBone{ EBoneIndex::Head };
	static inline uintptr_t s_LastRandomHandsController{ 0 };
	static inline uint32_t s_LastRandomAmmoCount{ 0 };
	static inline bool bRandomBoneInitialized{ false };

	// === TARGET FILTER SETTINGS ===

	static inline bool bTargetPMC{ true };
	static inline bool bTargetPlayerScav{ true };
	static inline bool bTargetAIScav{ false };
	static inline bool bTargetBoss{ true };
	static inline bool bTargetRaider{ true };
	static inline bool bTargetGuard{ true };

	// === ADVANCED SETTINGS ===

	// Max targeting distance in meters (0 = unlimited)
	static inline float fMaxDistance{ 0.0f };
	static inline bool bOnlyOnAim{ false };

	// Targeting mode: closest to crosshair vs closest by distance
	static inline ETargetingMode eTargetingMode{ ETargetingMode::ClosestToCrosshair };


	// Ballistics prediction for moving targets
	static inline bool bEnablePrediction{ false };
	static inline bool bShowTrajectory{ false };

	// KMbox/Net Device Settings (Managed in KmboxSettings module)
	static inline bool bUseKmboxNet{ false };
	static inline std::string sKmboxNetIp{ "192.168.2.4" };
	static inline int nKmboxNetPort{ 8888 };
	static inline std::string sKmboxNetMac{ "" };

private:
	static ImVec2 GetAimDeltaToTarget(uintptr_t TargetAddress, const ImVec2& AimOrigin);
	static uintptr_t FindBestTarget(const ImVec2& AimOrigin);
};
