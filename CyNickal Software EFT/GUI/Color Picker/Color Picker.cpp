#include "pch.h"

#include "Color Picker.h"

void ColorPicker::Render()
{
	if (!bMasterToggle)	return;

	ImGui::SetNextWindowSize(ImVec2(340, 420), ImGuiCond_FirstUseEver);
	ImGui::Begin("Color Picker", &bMasterToggle);
	
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Customize ESP colors for each entity type");
	ImGui::Spacing();
	
	Fuser::Render();
	Radar::Render();
	
	ImGui::End();
}

void ColorPicker::MyColorPicker(const char* label, ImColor& color)
{
	ImGui::SetNextItemWidth(200.0f);
	ImGui::ColorEdit4(label, &color.Value.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
}

void ColorPicker::Fuser::Render()
{
	if (ImGui::CollapsingHeader("Fuser Colors", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		ImGui::Spacing();
		
		ImGui::Text("Players");
		ImGui::Separator();
		MyColorPicker("PMC##Fuser", m_PMCColor);
		MyColorPicker("Scav##Fuser", m_ScavColor);
		MyColorPicker("Boss##Fuser", m_BossColor);
		MyColorPicker("Player Scav##Fuser", m_PlayerScavColor);
		
		ImGui::Spacing();
		ImGui::Text("World");
		ImGui::Separator();
		MyColorPicker("Loot##Fuser", m_LootColor);
		MyColorPicker("Container##Fuser", m_ContainerColor);
		MyColorPicker("Exfil##Fuser", m_ExfilColor);
		
		ImGui::Spacing();
		ImGui::Text("Text");
		ImGui::Separator();
		MyColorPicker("Weapon Text##Fuser", m_WeaponTextColor);
		
		ImGui::Unindent();
	}
}

void ColorPicker::Radar::Render()
{
	if (ImGui::CollapsingHeader("Radar Colors", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		ImGui::Spacing();
		
		ImGui::Text("Players");
		ImGui::Separator();
		MyColorPicker("PMC##Radar", m_PMCColor);
		MyColorPicker("Scav##Radar", m_ScavColor);
		MyColorPicker("Boss##Radar", m_BossColor);
		MyColorPicker("Player Scav##Radar", m_PlayerScavColor);
		MyColorPicker("Local Player##Radar", m_LocalPlayerColor);
		
		ImGui::Spacing();
		ImGui::Text("World");
		ImGui::Separator();
		MyColorPicker("Loot##Radar", m_LootColor);
		MyColorPicker("Container##Radar", m_ContainerColor);
		MyColorPicker("Exfil##Radar", m_ExfilColor);
		
		ImGui::Unindent();
	}
}