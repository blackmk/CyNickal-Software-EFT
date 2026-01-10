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

void Fuser::Render()
{
	if (!bMasterToggle) return;

	if (!EFT::pGameWorld)
		return;

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(Fuser::m_ScreenSize);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::Begin("Fuser", nullptr, ImGuiWindowFlags_NoDecoration);
	auto WindowPos = ImGui::GetWindowPos();
	auto DrawList = ImGui::GetWindowDrawList();

	Aimbot::RenderFOVCircle(WindowPos, DrawList);

	if (EFT::pGameWorld->m_pLootList)
		DrawESPLoot::DrawAll(WindowPos, DrawList);

	DrawExfils::DrawAll(WindowPos, DrawList);

	if (EFT::pGameWorld->m_pRegisteredPlayers)
		DrawESPPlayers::DrawAll(WindowPos, DrawList);

	AmmoCountOverlay::Render();

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
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputScalarN("Optic Index", ImGuiDataType_U32, &CameraList::m_OpticIndex, 1);
			
			static float fNewRadius{ 300.0f };
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::SliderFloat("Optic Radius", &fNewRadius, 100.0f, 500.0f, "%.0f")) {
				CameraList::SetOpticRadius(fNewRadius);
			}
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
		ImGui::Unindent();
	}

	// Screen Settings Section
	if (ImGui::CollapsingHeader("Screen Settings"))
	{
		ImGui::Indent();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputFloat("Screen Width", &Fuser::m_ScreenSize.x, 1.0f, 10.0f, "%.0f");
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputFloat("Screen Height", &Fuser::m_ScreenSize.y, 1.0f, 10.0f, "%.0f");
		ImGui::Unindent();
	}

	ImGui::End();
}

ImVec2 Fuser::GetCenterScreen()
{
	return { m_ScreenSize.x * .5f, m_ScreenSize.y * .5f };
}