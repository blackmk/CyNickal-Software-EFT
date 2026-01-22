#pragma once

class Fuser
{
public:
	static void Initialize();
	static void Render();
	static void RenderSettings();
	static ImVec2 GetCenterScreen();


public:
	static inline bool bSettings{ true };
	static inline bool bMasterToggle{ true };
	static inline bool bRequireRaidForESP{ false };
	static inline int m_SelectedMonitor{ 0 };
	static inline ImVec2 m_ScreenSize{ 1920.0f,1080.0f };

};