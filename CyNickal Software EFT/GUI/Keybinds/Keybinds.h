#pragma once
#include "DMA/DMA.h"

class CKeybind
{
public:
	std::string m_Name{};
	uint32_t m_Key{ 0 };
	bool m_bTargetPC{ false };
	bool m_bRadarPC{ false };
	bool m_bWaitingForKey{ false };
	bool m_bToggle{ false };
	bool m_bState{ false };
	bool m_bLastKeyStatus{ false };

public:
	void Render();
	void RenderTableRow();
	bool IsActive(DMA_Connection* Conn, bool bHoldMode = false);
	bool GetState() const { return m_bState; }
	const char* GetKeyName(uint32_t vkCode);
};


class Keybinds
{
public:
	static void Render();
	static void OnDMAFrame(DMA_Connection* Conn);

public:
	static inline bool bSettings{ true };
	static inline CKeybind DMARefresh = { "DMA Refresh", VK_INSERT, true, true };
	static inline CKeybind PlayerRefresh = { "Player Refresh", VK_HOME, true, true };
	static inline CKeybind Aimbot = { "Aimbot", VK_XBUTTON2, true, false };
	static inline CKeybind FleaBot = { "Flea Bot", VK_PRIOR, true, true };
	static inline CKeybind OpticESP = { "Optic ESP", VK_NEXT, true, true };
};