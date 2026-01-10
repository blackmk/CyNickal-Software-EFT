#include "pch.h"
#include "KmboxSettings.h"
#include "Input/KmboxNetController.h"
#include "GUI/Main Menu/Main Menu.h" // For BeginPanel/EndPanel if they were public, or use ImGui directly

namespace KmboxSettings
{
	bool bUseKmboxNet = false;
	std::string sKmboxNetIp = "192.168.2.4";
	int nKmboxNetPort = 8888;
	std::string sKmboxNetMac = "";
	bool bAutoConnect = false;

	// Note: We'll use the existing MainMenu utility functions if possible, 
	// but those are static in Main Menu.cpp. Let's use standard ImGui or helpers.
	
	void RenderSettings()
	{
		float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;

		ImGui::BeginChild("KmboxSettingsLeft", ImVec2(panelWidth, 0), false);
		{
			// Re-implemented simplified panel design to match style
			ImGui::TextColored(ImVec4(0.78f, 0.14f, 0.20f, 1.0f), "Connection Settings");
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Checkbox("Use KMbox NET", &bUseKmboxNet);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable to use KMbox NET device for mouse input");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// IP Address input
			static char ipBuffer[64];
			static bool ipBufferInit = false;
			if (!ipBufferInit)
			{
				strncpy_s(ipBuffer, sizeof(ipBuffer), sKmboxNetIp.c_str(), _TRUNCATE);
				ipBufferInit = true;
			}
			ImGui::Text("IP Address");
			ImGui::SetNextItemWidth(180.0f);
			if (ImGui::InputText("##IPAddress", ipBuffer, sizeof(ipBuffer)))
			{
				sKmboxNetIp = ipBuffer;
			}

			// Port input
			ImGui::Text("Port");
			ImGui::SetNextItemWidth(180.0f);
			ImGui::InputInt("##Port", &nKmboxNetPort);

			// MAC Address input
			static char macBuffer[64];
			static bool macBufferInit = false;
			if (!macBufferInit)
			{
				strncpy_s(macBuffer, sizeof(macBuffer), sKmboxNetMac.c_str(), _TRUNCATE);
				macBufferInit = true;
			}
			ImGui::Text("MAC Address");
			ImGui::SetNextItemWidth(180.0f);
			if (ImGui::InputText("##MACAddress", macBuffer, sizeof(macBuffer)))
			{
				sKmboxNetMac = macBuffer;
			}

			ImGui::Spacing();
			ImGui::Checkbox("Auto-connect on Startup", &bAutoConnect);

		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("KmboxSettingsRight", ImVec2(0, 0), false);
		{
			ImGui::TextColored(ImVec4(0.78f, 0.14f, 0.20f, 1.0f), "Status & Test");
			ImGui::Separator();
			ImGui::Spacing();

			bool isConnected = KmboxNet::KmboxNetController::IsConnected();
			if (isConnected)
			{
				ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Status: CONNECTED");
			}
			else
			{
				ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Status: DISCONNECTED");
			}

			ImGui::Spacing();

			if (!isConnected)
			{
				if (ImGui::Button("Connect", ImVec2(100, 0)))
				{
					KmboxNet::KmboxNetController::Connect(sKmboxNetIp, nKmboxNetPort, sKmboxNetMac);
				}
			}
			else
			{
				if (ImGui::Button("Disconnect", ImVec2(100, 0)))
				{
					KmboxNet::KmboxNetController::Disconnect();
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Test Movement");
			if (ImGui::Button("Test Mouse (Square)", ImVec2(180, 0)))
			{
				TestMouseMovement();
			}
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Moves mouse in a 20px square.");

		}
		ImGui::EndChild();
	}

	void TryAutoConnect()
	{
		if (bAutoConnect && bUseKmboxNet)
		{
			std::println("[Kmbox] Attempting auto-connect to {}:{}", sKmboxNetIp, nKmboxNetPort);
			KmboxNet::KmboxNetController::Connect(sKmboxNetIp, nKmboxNetPort, sKmboxNetMac);
		}
	}

	void TestMouseMovement()
	{
		if (!KmboxNet::KmboxNetController::IsConnected())
		{
			std::println("[Kmbox] Cannot test movement: Not connected.");
			return;
		}

		// Move in a small square pattern: Right, Down, Left, Up
		std::thread([]() {
			int side = 20;
			int delay = 50; // ms

			KmboxNet::KmboxNetController::Move(side, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			KmboxNet::KmboxNetController::Move(0, side);
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			KmboxNet::KmboxNetController::Move(-side, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			KmboxNet::KmboxNetController::Move(0, -side);
		}).detach();
	}
}
