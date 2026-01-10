#pragma once
#include <string>
#include <imgui.h>

namespace KmboxSettings
{
	// Device Status
	extern bool bUseKmboxNet;
	extern std::string sKmboxNetIp;
	extern int nKmboxNetPort;
	extern std::string sKmboxNetMac;
	extern bool bAutoConnect;

	// UI Rendering
	void RenderSettings();

	// Logic
	void TryAutoConnect();
	void TestMouseMovement();
}
