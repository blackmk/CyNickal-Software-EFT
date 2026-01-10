#pragma once
#include "DMA/DMA.h"
#include <string>

class Aimbot
{
public:
	static void RenderSettings();
	static void RenderFOVCircle(const ImVec2& WindowPos, ImDrawList* DrawList);
	static void OnDMAFrame(DMA_Connection* Conn);

	// Device-agnostic mouse movement - routes to Makcu or KMbox/Net based on settings
	static void SendDeviceMove(int dx, int dy);

public:
	// General Aimbot Settings
	static inline bool bSettings{ false };
	static inline bool bMasterToggle{ false };
	static inline bool bDrawFOV{ true };
	static inline float fDampen{ 0.95f };
	static inline float fPixelFOV{ 75.0f };
	static inline float fDeadzoneFov{ 2.0f };

	// KMbox/Net Device Settings (Managed in KmboxSettings module)
	static inline bool bUseKmboxNet{ false };
	static inline std::string sKmboxNetIp{ "192.168.2.4" };
	static inline int nKmboxNetPort{ 8888 };
	static inline std::string sKmboxNetMac{ "" };

private:
	static ImVec2 GetAimDeltaToTarget(uintptr_t TargetAddress);
	static uintptr_t FindBestTarget();
};