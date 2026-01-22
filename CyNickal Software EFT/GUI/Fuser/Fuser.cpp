#include "pch.h"
#include "Fuser.h"
#include "Draw/Players.h"
#include "Draw/Loot.h"
#include "Draw/Exfils.h"
#include "GUI/Aimbot/Aimbot.h"
#include "../ESP/ESPSettings.h"
#include "Overlays/Ammo Count/Ammo Count.h"
#include "Game/EFT.h"
#include "Game/Camera List/Camera List.h"
#include <chrono>

#include "../Main Window/MonitorHelper.h"
#include "../Main Window/Main Window.h"

void Fuser::Initialize()
{
	// Validate and sync monitor settings from loaded config
	auto monitors = MonitorHelper::GetAllMonitors();

	// Validate selected monitor index
	if (monitors.empty())
	{
		m_SelectedMonitor = 0;
		m_ScreenSize = ImVec2(1920.0f, 1080.0f);
		return;
	}

	if (m_SelectedMonitor < 0 || m_SelectedMonitor >= static_cast<int>(monitors.size()))
	{
		m_SelectedMonitor = 0;
	}

	// Sync screen size with validated monitor
	if (!monitors.empty() && m_SelectedMonitor >= 0 && m_SelectedMonitor < static_cast<int>(monitors.size()))
	{
		const auto& m = monitors[m_SelectedMonitor];
		m_ScreenSize.x = static_cast<float>(std::abs(m.rcMonitor.right - m.rcMonitor.left));
		m_ScreenSize.y = static_cast<float>(std::abs(m.rcMonitor.bottom - m.rcMonitor.top));
	}
}

void Fuser::Render()
{
	if (!bMasterToggle) return;

	// Check if we're in raid with a valid GameWorld
	auto gameWorld = EFT::GetGameWorld();
	bool inRaid = EFT::IsInRaid() && gameWorld;

	// Only block rendering if raid requirement enabled
	if (bRequireRaidForESP && !inRaid)
		return;

	// Calculate position based on selected monitor (cached)
	static ImVec2 s_CachedWindowPos(0.0f, 0.0f);
	static int s_LastSelectedMonitor = -1;
	static std::chrono::steady_clock::time_point s_LastMonitorCheck{};
	const auto now = std::chrono::steady_clock::now();

	if (s_LastSelectedMonitor != Fuser::m_SelectedMonitor ||
		now - s_LastMonitorCheck > std::chrono::seconds(1))
	{
		auto monitors = MonitorHelper::GetAllMonitors();
		if (Fuser::m_SelectedMonitor >= 0 && Fuser::m_SelectedMonitor < static_cast<int>(monitors.size()))
		{
			s_CachedWindowPos.x = static_cast<float>(monitors[Fuser::m_SelectedMonitor].rcMonitor.left);
			s_CachedWindowPos.y = static_cast<float>(monitors[Fuser::m_SelectedMonitor].rcMonitor.top);
		}
		s_LastSelectedMonitor = Fuser::m_SelectedMonitor;
		s_LastMonitorCheck = now;
	}

	ImVec2 pos = s_CachedWindowPos;

	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(Fuser::m_ScreenSize);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::Begin("Fuser", nullptr, ImGuiWindowFlags_NoDecoration);
	auto WindowPos = ImGui::GetWindowPos();
	auto DrawList = ImGui::GetWindowDrawList();

	Aimbot::RenderFOVCircle(WindowPos, DrawList);  // Always show if aimbot enabled

	// Only render game content when in raid
	if (inRaid)
	{
		if (gameWorld->m_pLootList)
			DrawESPLoot::DrawAll(WindowPos, DrawList);

		DrawExfils::DrawAll(WindowPos, DrawList);

		if (gameWorld->m_pRegisteredPlayers)
			DrawESPPlayers::DrawAll(WindowPos, DrawList);

		AmmoCountOverlay::Render();
	}

	ImGui::End();
	ImGui::PopStyleColor();
}

void Fuser::RenderSettings()
{
	if (!bSettings) return;

	ImGui::SetNextWindowSize(ImVec2(320, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("Fuser Settings", &bSettings);

	// Optic ESP Section
	if (ImGui::CollapsingHeader("Optic ESP", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		ImGui::Checkbox("Enable Optic ESP", &DrawESPPlayers::bOpticESP);
		
		if (DrawESPPlayers::bOpticESP)
		{
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Optic camera auto-detected");
			ImGui::Text("Auto Radius: %.0f", CameraList::GetOpticRadius());
		}
		ImGui::Unindent();
	}


	// Player Visuals Section
	if (ImGui::CollapsingHeader("Player Visuals", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		ImGui::Checkbox("Player Names", &ESPSettings::Enemy::bNameEnabled);
		ImGui::Checkbox("Player Skeletons", &ESPSettings::Enemy::bSkeletonEnabled);
		ImGui::Checkbox("Head Dots", &ESPSettings::Enemy::bHeadDotEnabled);
		ImGui::Unindent();
	}

	// Loot Section
	if (ImGui::CollapsingHeader("Loot Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		DrawESPLoot::DrawSettings();
		ImGui::Unindent();
	}

	// World Section
	if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		ImGui::Checkbox("Show Exfils", &DrawExfils::bMasterToggle);
		ImGui::Checkbox("Require Raid for ESP", &Fuser::bRequireRaidForESP);
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("When enabled, ESP overlay only shows during raids.\nWhen disabled, overlay window always visible.");
		}
		ImGui::Unindent();
	}

	// Screen Settings Section
	if (ImGui::CollapsingHeader("Screen Settings"))
	{
		ImGui::Indent();
		
		// Cache monitor list, refresh every 2 seconds
		static std::vector<MonitorData> s_MonitorListCache;
		static std::chrono::steady_clock::time_point s_LastCacheUpdate{};
		auto cacheNow = std::chrono::steady_clock::now();
		if (s_MonitorListCache.empty() || cacheNow - s_LastCacheUpdate > std::chrono::seconds(2))
		{
			s_MonitorListCache = MonitorHelper::GetAllMonitors();
			s_LastCacheUpdate = cacheNow;
		}

		const auto& monitors = s_MonitorListCache;
		std::string preview = "Select Monitor...";
		if (Fuser::m_SelectedMonitor >= 0 && Fuser::m_SelectedMonitor < monitors.size())
			preview = monitors[Fuser::m_SelectedMonitor].friendlyName;

		if (ImGui::BeginCombo("Target Monitor", preview.c_str()))
		{
			for (const auto& m : monitors)
			{
				bool isSelected = (Fuser::m_SelectedMonitor == m.index);
				if (ImGui::Selectable(m.friendlyName.c_str(), isSelected))
				{
					Fuser::m_SelectedMonitor = m.index;
					Fuser::m_ScreenSize.x = static_cast<float>(abs(m.rcMonitor.right - m.rcMonitor.left));
					Fuser::m_ScreenSize.y = static_cast<float>(abs(m.rcMonitor.bottom - m.rcMonitor.top));
					MonitorHelper::MoveWindowToMonitor(MainWindow::g_hWnd, m.index);
				}
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Keep manual inputs as fallback/info
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputFloat("Screen Width", &Fuser::m_ScreenSize.x, 1.0f, 10.0f, "%.0f", ImGuiInputTextFlags_ReadOnly);
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputFloat("Screen Height", &Fuser::m_ScreenSize.y, 1.0f, 10.0f, "%.0f", ImGuiInputTextFlags_ReadOnly);
		
		ImGui::Unindent();
	}

	ImGui::End();
}

ImVec2 Fuser::GetCenterScreen()
{
	return { m_ScreenSize.x * .5f, m_ScreenSize.y * .5f };
}
