#include "pch.h"
#include "Main Menu.h"

#include "GUI/Color Picker/Color Picker.h"
#include "GUI/Player Table/Player Table.h"
#include "GUI/Item Table/Item Table.h"
#include "GUI/Fuser/Fuser.h"
#include "GUI/Radar/Radar.h"
#include "GUI/Aimbot/Aimbot.h"
#include "GUI/ESP/ESPSettings.h"
#include "GUI/ESP/ESPPreview.h"
#include <string>
#include <filesystem>
#include "GUI/Keybinds/Keybinds.h"
#include "GUI/Config/Config.h"
#include "GUI/Flea Bot/Flea Bot.h"
#include "GUI/Fuser/Draw/Players.h"
#include "GUI/Fuser/Draw/Loot.h"
#include "GUI/Fuser/Draw/Exfils.h"
#include "GUI/Radar/Draw/Radar Loot.h"
#include "GUI/Radar/Draw/Radar Exfils.h"
#include "Game/Camera List/Camera List.h"
#include "DebugMode.h"
#include "GUI/KmboxSettings/KmboxSettings.h"

// ============================================================================
// UI State
// ============================================================================
namespace MainMenuState
{
	// Sidebar selection
	enum class ECategory { Combat, Visuals, Other };
	enum class ECombatItem { Aimbot };
	enum class EVisualsItem { EntityESP, ObjectsESP, Visuals, Radar };
	enum class EOtherItem { Misc, Settings };
	
	static ECategory selectedCategory = ECategory::Visuals;
	static int selectedCombatItem = 0;   // 0=Aimbot
	static int selectedVisualsItem = 2;  // 0=EntityESP, 1=ObjectsESP, 2=Visuals, 3=Radar
	static int selectedOtherItem = 0;    // 0=Misc, 1=Settings
	
	// Visuals sub-tabs
	static int visualsSubTab = 3; // 0=World, 1=Saved Locations, 2=Rename Object, 3=Misc
}

// ============================================================================
// Custom UI Components
// ============================================================================

// Custom toggle switch (red O for off, filled for on)
static bool CustomToggle(const char* label, bool* v)
{
	ImGui::PushID(label);
	
	bool changed = false;
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	
	float height = ImGui::GetFrameHeight() * 0.8f;
	float width = height * 1.8f;
	float radius = height * 0.5f;
	
	// Invisible button for interaction
	if (ImGui::InvisibleButton("##toggle", ImVec2(width, height)))
	{
		*v = !*v;
		changed = true;
	}
	
	// Colors
	ImU32 col_bg;
	ImU32 col_circle;
	
	if (*v)
	{
		// ON: Green filled circle
		col_bg = IM_COL32(30, 30, 30, 255);
		col_circle = IM_COL32(50, 200, 50, 255);
	}
	else
	{
		// OFF: Red hollow circle (O shape)
		col_bg = IM_COL32(30, 30, 30, 255);
		col_circle = IM_COL32(200, 50, 50, 255);
	}
	
	// Draw background
	draw_list->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), col_bg, radius);
	
	// Draw circle/toggle indicator
	float circle_x = *v ? (pos.x + width - radius - 2.0f) : (pos.x + radius + 2.0f);
	float circle_y = pos.y + radius;
	
	if (*v)
	{
		// Filled circle for ON
		draw_list->AddCircleFilled(ImVec2(circle_x, circle_y), radius - 3.0f, col_circle);
	}
	else
	{
		// Hollow circle (O) for OFF
		draw_list->AddCircle(ImVec2(circle_x, circle_y), radius - 3.0f, col_circle, 0, 2.0f);
	}
	
	// Label
	ImGui::SameLine();
	ImGui::Text("%s", label);
	
	ImGui::PopID();
	return changed;
}

// Section panel with title and optional subtitle
static void BeginPanel(const char* title, const char* subtitle = nullptr)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
	
	ImGui::BeginChild(title, ImVec2(0, 0), true);
	
	// Title with red line
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	
	// Red line before title
	float lineWidth = 40.0f;
	draw_list->AddLine(
		ImVec2(pos.x, pos.y + ImGui::GetTextLineHeight() * 0.5f),
		ImVec2(pos.x + lineWidth, pos.y + ImGui::GetTextLineHeight() * 0.5f),
		IM_COL32(180, 40, 50, 255), 1.0f
	);
	
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + lineWidth + 8.0f);
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", title);
	
	// Red line after title
	ImGui::SameLine();
	pos = ImGui::GetCursorScreenPos();
	draw_list->AddLine(
		ImVec2(pos.x + 8.0f, pos.y + ImGui::GetTextLineHeight() * 0.5f),
		ImVec2(pos.x + lineWidth + 8.0f, pos.y + ImGui::GetTextLineHeight() * 0.5f),
		IM_COL32(180, 40, 50, 255), 1.0f
	);
	
	ImGui::Dummy(ImVec2(0, 2));
	
	if (subtitle)
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", subtitle);
		ImGui::Spacing();
	}
	
	ImGui::Separator();
	ImGui::Spacing();
}

static void EndPanel()
{
	ImGui::EndChild();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

// Slider with red accent and value display
static void CustomSlider(const char* label, float* v, float min, float max, const char* format = "%.1f")
{
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.78f, 0.14f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.90f, 0.20f, 0.25f, 1.0f));
	
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat(label, v, min, max, format);
	
	ImGui::PopStyleColor(2);
}

// Color picker button with hex display
static void ColorPickerButton(const char* label, ImColor& color)
{
	float col[4] = { color.Value.x, color.Value.y, color.Value.z, color.Value.w };
	
	ImGui::Text("%s", label);
	ImGui::SameLine();
	
	// Hex color text
	char hexBuf[16];
	snprintf(hexBuf, sizeof(hexBuf), "#%02X%02X%02X", 
		(int)(col[0] * 255), (int)(col[1] * 255), (int)(col[2] * 255));
	ImGui::TextColored(ImVec4(0.78f, 0.14f, 0.20f, 1.0f), "%s", hexBuf);
	
	ImGui::SameLine();
	
	// Color button
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
	if (ImGui::ColorEdit4(("##" + std::string(label)).c_str(), col, 
		ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
	{
		color = ImColor(col[0], col[1], col[2], col[3]);
	}
	ImGui::PopStyleVar();
}

// Sidebar category header
static bool SidebarCategory(const char* label, bool isExpanded, bool isActive)
{
	ImGui::PushStyleColor(ImGuiCol_Header, isActive ? ImVec4(0.55f, 0.10f, 0.14f, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.08f, 0.10f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.14f, 0.20f, 1.0f));
	
	bool clicked = ImGui::Selectable(label, isActive, ImGuiSelectableFlags_None);
	
	ImGui::PopStyleColor(3);
	return clicked;
}

// Sidebar item
static bool SidebarItem(const char* label, bool isActive, bool hasMarker = false)
{
	ImGui::PushStyleColor(ImGuiCol_Header, isActive ? ImVec4(0.15f, 0.15f, 0.15f, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
	
	// Indent for sub-items
	ImGui::Indent(8.0f);
	
	// Marker
	if (hasMarker && isActive)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ">");
		ImGui::SameLine();
	}
	
	bool clicked = ImGui::Selectable(label, isActive, ImGuiSelectableFlags_None);
	
	ImGui::Unindent(8.0f);
	ImGui::PopStyleColor(2);
	return clicked;
}

// ============================================================================
// Content Panels
// ============================================================================

static void RenderAimbotContent()
{
	float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
	
	ImGui::BeginChild("AimbotLeft", ImVec2(panelWidth, 0), false);
	{
		BeginPanel("Aimbot Settings", "Hardware-level aim assistance");
		
		CustomToggle("Enable Aimbot", &Aimbot::bMasterToggle);
		ImGui::Spacing();
		CustomToggle("Draw FOV Circle", &Aimbot::bDrawFOV);
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		ImGui::Text("FOV Radius");
		CustomSlider("##FOV", &Aimbot::fPixelFOV, 10.0f, 300.0f, "%.0f px");
		
		ImGui::Text("Deadzone");
		CustomSlider("##Deadzone", &Aimbot::fDeadzoneFov, 1.0f, 20.0f, "%.1f px");
		
		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	ImGui::BeginChild("AimbotRight", ImVec2(0, 0), false);
	{
		BeginPanel("Smoothing", "Aim speed control");
		
		ImGui::Text("Dampen Factor");
		CustomSlider("##Dampen", &Aimbot::fDampen, 0.01f, 1.0f, "%.2f");
		
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Lower = smoother but slower");
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Higher = faster but obvious");
		
		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderEntityESPContent()
{
	float panelWidth = ImGui::GetContentRegionAvail().x * 0.45f;
	
	ImGui::BeginChild("ESPSettingsLeft", ImVec2(panelWidth, 0), false);
	{
		BeginPanel("Enemy ESP", "Visual configuration");
		
		CustomToggle("Enable Fuser (Master)", &Fuser::bMasterToggle);
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		using namespace ESPSettings::Enemy;
		
		CustomToggle("Box ESP##ESP", &bBoxEnabled);
		if (bBoxEnabled) {
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			static const char* styles[] = { "Full", "Corners" };
			ImGui::Combo("##BoxStyleESP", &boxStyle, styles, IM_ARRAYSIZE(styles));
			
			ColorPickerButton("Box Color##ESP", boxColor);
			CustomToggle("Filled Box##ESP", &bBoxFilled);
			if (bBoxFilled) ColorPickerButton("Fill Color##ESP", boxFillColor);
			CustomSlider("##BoxThick", &boxThickness, 1.0f, 5.0f, "%.1f px");
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		CustomToggle("Skeleton##ESP", &bSkeletonEnabled);
		if (bSkeletonEnabled) {
			ColorPickerButton("Skeleton Color##ESP", skeletonColor);
			CustomSlider("##SkeletonThick", &skeletonThickness, 1.0f, 5.0f, "%.1f px");
			
			if (ImGui::TreeNode("Bone Groups##ESP")) {
				CustomToggle("Head/Neck##ESP", &bBonesHead);
				CustomToggle("Spine/Pelvis##ESP", &bBonesSpine);
				CustomToggle("Left Arm##ESP", &bBonesArmsL);
				CustomToggle("Right Arm##ESP", &bBonesArmsR);
				CustomToggle("Left Leg##ESP", &bBonesLegsL);
				CustomToggle("Right Leg##ESP", &bBonesLegsR);
				ImGui::TreePop();
			}
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		CustomToggle("Name##ESP", &bNameEnabled);
		if (bNameEnabled) ColorPickerButton("Name Color##ESP", nameColor);
		
		CustomToggle("Weapon##ESP", &bWeaponEnabled);
		if (bWeaponEnabled) ColorPickerButton("Weapon Color##ESP", weaponColor);
		
		CustomToggle("Distance##ESP", &bDistanceEnabled);
		if (bDistanceEnabled) ColorPickerButton("Distance Color##ESP", distanceColor);
		
		CustomToggle("Head Dot##ESP", &bHeadDotEnabled);
		if (bHeadDotEnabled) {
			ColorPickerButton("Dot Color##ESP", headDotColor);
			CustomSlider("##DotRadius", &headDotRadius, 1.0f, 10.0f, "%.1f px");
		}
		
		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	ImGui::BeginChild("ESPSettingsRight", ImVec2(0, 0), false);
	{
		BeginPanel("Preview", "Live mannequin visual");
		
		ImVec2 avail = ImGui::GetContentRegionAvail();
		ESPPreview::Render(ImVec2(avail.x, avail.y - 10.0f));
		
		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderObjectsESPContent()
{
	float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
	
	ImGui::BeginChild("LootLeft", ImVec2(panelWidth, 0), false);
	{
		BeginPanel("Loot ESP", "Item and container display");
		
		CustomToggle("Show Loot", &DrawESPLoot::bMasterToggle);
		CustomToggle("Show Containers", &DrawESPLoot::bContainerToggle);
		CustomToggle("Show Items", &DrawESPLoot::bItemToggle);
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		CustomToggle("Show Exfils", &DrawExfils::bMasterToggle);
		
		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	ImGui::BeginChild("LootRight", ImVec2(0, 0), false);
	{
		BeginPanel("World Colors", "Loot and world colors");
		
		ColorPickerButton("Loot", ColorPicker::Fuser::m_LootColor);
		ColorPickerButton("Container", ColorPicker::Fuser::m_ContainerColor);
		ColorPickerButton("Exfil", ColorPicker::Fuser::m_ExfilColor);
		
		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderVisualsTabContent()
{
	using namespace MainMenuState;
	
	// Sub-tabs matching reference (World, Saved Locations, Rename Object, Misc)
	if (ImGui::BeginTabBar("VisualsSubTabs"))
	{
		if (ImGui::BeginTabItem("World"))
		{
			visualsSubTab = 0;
			ImGui::Spacing();
			
			float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
			
			ImGui::BeginChild("WorldLeft", ImVec2(panelWidth, 0), false);
			{
				BeginPanel("Fuser Settings", "3D ESP screen settings");
				
				ImGui::Text("Screen Width");
				CustomSlider("##ScreenW", &Fuser::m_ScreenSize.x, 800.0f, 3840.0f, "%.0f");
				
				ImGui::Text("Screen Height");
				CustomSlider("##ScreenH", &Fuser::m_ScreenSize.y, 600.0f, 2160.0f, "%.0f");
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::SameLine();
			
			ImGui::BeginChild("WorldRight", ImVec2(0, 0), false);
			{
				BeginPanel("Optic ESP", "Scope vision settings");
				
				CustomToggle("Enable Optic ESP", &DrawESPPlayers::bOpticESP);
				
				if (DrawESPPlayers::bOpticESP)
				{
					ImGui::Text("Optic Index");
					ImGui::SetNextItemWidth(100.0f);
					ImGui::InputScalarN("##OpticIdx", ImGuiDataType_U32, &CameraList::m_OpticIndex, 1);
					
					static float fNewRadius{ 300.0f };
					ImGui::Text("Optic Radius");
					if (ImGui::SliderFloat("##OpticRad", &fNewRadius, 100.0f, 500.0f, "%.0f"))
					{
						CameraList::SetOpticRadius(fNewRadius);
					}
				}
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Saved Locations"))
		{
			visualsSubTab = 1;
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Saved locations feature coming soon...");
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Rename Object"))
		{
			visualsSubTab = 2;
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Object renaming feature coming soon...");
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Misc"))
		{
			visualsSubTab = 3;
			ImGui::Spacing();
			
			float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
			
			ImGui::BeginChild("MiscIndicators", ImVec2(panelWidth, 200), false);
			{
				BeginPanel("Indicators", "Visual indicators/alerts");
				
				CustomToggle("Player Table", &PlayerTable::bMasterToggle);
				CustomToggle("Item Table", &ItemTable::bMasterToggle);
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::SameLine();
			
			ImGui::BeginChild("MiscEnhancements", ImVec2(0, 200), false);
			{
				BeginPanel("Visual Enhancements", "Game visual changes");
				
				// Placeholder for potential future FOV/visual settings
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Additional visual enhancements");
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "will appear here...");
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::EndTabItem();
		}
		
		ImGui::EndTabBar();
	}
}

static void RenderRadarContent()
{
	float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
	
	ImGui::BeginChild("RadarLeft", ImVec2(panelWidth, 0), false);
	{
		BeginPanel("Radar Settings", "2D radar minimap");
		
		CustomToggle("Enable Radar", &Radar::bMasterToggle);
		CustomToggle("Local View Ray", &Radar::bLocalViewRay);
		CustomToggle("Other View Rays", &Radar::bOtherPlayerViewRays);
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		CustomToggle("Radar Loot", &DrawRadarLoot::bMasterToggle);
		CustomToggle("Radar Exfils", &DrawRadarExfils::bMasterToggle);
		
		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	ImGui::BeginChild("RadarRight", ImVec2(0, 0), false);
	{
		BeginPanel("Radar Visuals", "Display settings");
		
		ImGui::Text("Scale");
		CustomSlider("##Scale", &Radar::fScale, 0.1f, 5.0f, "%.1f");
		
		ImGui::Text("Local View Ray Length");
		CustomSlider("##LocalRay", &Radar::fLocalViewRayLength, 10.0f, 500.0f, "%.0f");
		
		ImGui::Text("Other View Ray Length");
		CustomSlider("##OtherRay", &Radar::fOtherViewRayLength, 10.0f, 500.0f, "%.0f");
		
		ImGui::Text("Entity Radius");
		CustomSlider("##EntRadius", &Radar::fEntityRadius, 1.0f, 20.0f, "%.1f");
		
		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderMiscContent()
{
	// FleaBot initialization and thread management (moved from FleaBot::Render)
	if (FleaBot::m_PriceList.empty())
	{
		FleaBot::LoadPriceList();
		FleaBot::ConstructPriceList();
	}

	if (FleaBot::pInputThread && !FleaBot::bMasterToggle && FleaBot::bInputThreadDone)
	{
		FleaBot::pInputThread->join();
		FleaBot::pInputThread = nullptr;
	}

	if (!FleaBot::pInputThread && FleaBot::bMasterToggle)
	{
		FleaBot::bInputThreadDone = false;
		FleaBot::pInputThread = std::make_unique<std::thread>(&FleaBot::InputThread);
	}

	float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
	
	ImGui::BeginChild("MiscLeft", ImVec2(panelWidth, 0), false);
	{
		BeginPanel("Flea Bot", "Automatic flea market");
		
		CustomToggle("Enable Flea Bot", &FleaBot::bMasterToggle);
		
		ImGui::Spacing();
		
		// Basic Flea Bot settings
		CustomToggle("Cycle Buy", &FleaBot::bCycleBuy);
		CustomToggle("Limit Buy", &FleaBot::bLimitBuy);
		
		ImGui::Spacing();
		ImGui::Text("Item Count");
		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputScalar("##ItemCount", ImGuiDataType_U32, &FleaBot::m_ItemCount);
		
		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	ImGui::BeginChild("MiscRight", ImVec2(0, 0), false);
	{
		BeginPanel("Data Tables", "Information displays");
		
		CustomToggle("Player Table", &PlayerTable::bMasterToggle);
		CustomToggle("Item Table", &ItemTable::bMasterToggle);
		
		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderSettingsContent()
{
	// Tabs for Config, Colors, Keybinds
	if (ImGui::BeginTabBar("SettingsTabs"))
	{
		if (ImGui::BeginTabItem("Config"))
		{
			ImGui::Spacing();
			
			static char configNameBuf[64] = "default";
			static std::vector<std::string> configFiles;
			static bool bFirstRun = true;
			static int selectedConfig = -1;
			
			if (bFirstRun) {
				Config::RefreshConfigFilesList(configFiles);
				bFirstRun = false;
			}
			
			float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
			
			ImGui::BeginChild("ConfigLeft", ImVec2(panelWidth, 0), false);
			{
				BeginPanel("Save/Load", "Configuration management");
				
				ImGui::Text("Config Name");
				ImGui::SetNextItemWidth(180.0f);
				ImGui::InputText("##ConfigName", configNameBuf, IM_ARRAYSIZE(configNameBuf));
				
				ImGui::Spacing();
				
				if (ImGui::Button("Save", ImVec2(80, 0)))
				{
					Config::SaveConfig(configNameBuf);
					Config::RefreshConfigFilesList(configFiles);
				}
				ImGui::SameLine();
				if (ImGui::Button("Load", ImVec2(80, 0)))
				{
					Config::LoadConfig(configNameBuf);
				}
				ImGui::SameLine();
				if (ImGui::Button("Delete", ImVec2(80, 0)))
				{
					if (selectedConfig >= 0 && selectedConfig < static_cast<int>(configFiles.size())) {
						std::string fileToDelete = Config::getConfigPath(configFiles[selectedConfig]);
						if (std::filesystem::exists(fileToDelete)) {
							std::filesystem::remove(fileToDelete);
							selectedConfig = -1;
							strncpy_s(configNameBuf, sizeof(configNameBuf), "default", _TRUNCATE);
							Config::RefreshConfigFilesList(configFiles);
						}
					}
				}
				
				ImGui::Spacing();
				
				if (ImGui::Button("Refresh List", ImVec2(100, 0)))
				{
					Config::RefreshConfigFilesList(configFiles);
				}
				ImGui::SameLine();
				if (ImGui::Button("Open Folder", ImVec2(100, 0)))
				{
					ShellExecuteA(nullptr, "open", Config::getConfigDir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
				}
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::SameLine();
			
			ImGui::BeginChild("ConfigRight", ImVec2(0, 0), false);
			{
				BeginPanel("Saved Configs", "Available configuration files");
				
				ImGui::BeginChild("ConfigList", ImVec2(0, 150), true);
				for (size_t i = 0; i < configFiles.size(); ++i) {
					if (ImGui::Selectable(configFiles[i].c_str(), selectedConfig == static_cast<int>(i))) {
						selectedConfig = static_cast<int>(i);
						strncpy_s(configNameBuf, sizeof(configNameBuf), configFiles[i].c_str(), _TRUNCATE);
					}
				}
				ImGui::EndChild();
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Colors"))
		{
			ImGui::Spacing();
			
			float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
			
			ImGui::BeginChild("ColorsLeft", ImVec2(panelWidth, 0), false);
			{
				BeginPanel("Fuser Colors", "3D ESP colors");
				
				ImGui::Text("Players");
				ImGui::Separator();
				ColorPickerButton("PMC##F", ColorPicker::Fuser::m_PMCColor);
				ColorPickerButton("Scav##F", ColorPicker::Fuser::m_ScavColor);
				ColorPickerButton("Boss##F", ColorPicker::Fuser::m_BossColor);
				ColorPickerButton("Player Scav##F", ColorPicker::Fuser::m_PlayerScavColor);
				
				ImGui::Spacing();
				ImGui::Text("World");
				ImGui::Separator();
				ColorPickerButton("Loot##F", ColorPicker::Fuser::m_LootColor);
				ColorPickerButton("Container##F", ColorPicker::Fuser::m_ContainerColor);
				ColorPickerButton("Exfil##F", ColorPicker::Fuser::m_ExfilColor);
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::SameLine();
			
			ImGui::BeginChild("ColorsRight", ImVec2(0, 0), false);
			{
				BeginPanel("Radar Colors", "2D radar colors");
				
				ImGui::Text("Players");
				ImGui::Separator();
				ColorPickerButton("PMC##R", ColorPicker::Radar::m_PMCColor);
				ColorPickerButton("Scav##R", ColorPicker::Radar::m_ScavColor);
				ColorPickerButton("Boss##R", ColorPicker::Radar::m_BossColor);
				ColorPickerButton("Player Scav##R", ColorPicker::Radar::m_PlayerScavColor);
				ColorPickerButton("Local Player##R", ColorPicker::Radar::m_LocalPlayerColor);
				
				ImGui::Spacing();
				ImGui::Text("World");
				ImGui::Separator();
				ColorPickerButton("Loot##R", ColorPicker::Radar::m_LootColor);
				ColorPickerButton("Container##R", ColorPicker::Radar::m_ContainerColor);
				ColorPickerButton("Exfil##R", ColorPicker::Radar::m_ExfilColor);
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Keybinds"))
		{
			ImGui::Spacing();
			
			BeginPanel("Hotkeys", "Configure keyboard shortcuts");
			
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Click a key button to rebind. Target = Game PC, Radar = This PC");
			ImGui::Spacing();
			
			if (ImGui::BeginTable("KeybindsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Target PC", ImGuiTableColumnFlags_WidthFixed, 70.0f);
				ImGui::TableSetupColumn("Radar PC", ImGuiTableColumnFlags_WidthFixed, 70.0f);
				ImGui::TableHeadersRow();
				
				Keybinds::DMARefresh.RenderTableRow();
				Keybinds::PlayerRefresh.RenderTableRow();
				Keybinds::Aimbot.RenderTableRow();
				Keybinds::FleaBot.RenderTableRow();
				Keybinds::OpticESP.RenderTableRow();
				
				ImGui::EndTable();
			}
			
			EndPanel();
			
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("External Aim Device"))
		{
			ImGui::Spacing();
			KmboxSettings::RenderSettings();
			ImGui::EndTabItem();
		}
		
		ImGui::EndTabBar();
	}
}

// ============================================================================
// Main Render Function
// ============================================================================

void MainMenu::Render()
{
	using namespace MainMenuState;
	
	// Main window setup - larger size for unified panel
	ImGui::SetNextWindowSize(ImVec2(900, 550), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("CyNickal Software", nullptr, ImGuiWindowFlags_NoCollapse);
	ImGui::PopStyleVar();
	
	// ========== SIDEBAR ==========
	float sidebarWidth = 140.0f;
	
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
	ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), true);
	{
		// Logo area
		ImGui::Spacing();
		ImGui::SetCursorPosX((sidebarWidth - ImGui::CalcTextSize(">>").x) * 0.3f);
		ImGui::TextColored(ImVec4(0.78f, 0.14f, 0.20f, 1.0f), ">>");
		ImGui::SetCursorPosX((sidebarWidth - ImGui::CalcTextSize("CyNickal").x) * 0.5f);
		ImGui::TextColored(ImVec4(0.78f, 0.14f, 0.20f, 1.0f), "CyNickal");
		ImGui::Spacing();

		if (DebugMode::IsDebugMode())
		{
			ImGui::SetCursorPosX((sidebarWidth - ImGui::CalcTextSize("DEBUG MODE").x) * 0.5f);
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DEBUG MODE");
			ImGui::SetCursorPosX((sidebarWidth - ImGui::CalcTextSize("NO CONNECTION").x) * 0.5f);
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "NO CONNECTION");
			ImGui::Spacing();
		}

		ImGui::Separator();
		ImGui::Spacing();
		
		// Combat category
		if (SidebarCategory("Combat", true, selectedCategory == ECategory::Combat))
		{
			selectedCategory = ECategory::Combat;
		}
		if (selectedCategory == ECategory::Combat || true) // Always show items for now
		{
			if (SidebarItem("Aimbot", selectedCategory == ECategory::Combat && selectedCombatItem == 0, true))
			{
				selectedCategory = ECategory::Combat;
				selectedCombatItem = 0;
			}
		}
		
		ImGui::Spacing();
		
		// Visuals category
		if (SidebarCategory("Visuals", true, selectedCategory == ECategory::Visuals))
		{
			selectedCategory = ECategory::Visuals;
		}
		if (selectedCategory == ECategory::Visuals || true)
		{
			if (SidebarItem("Entity ESP", selectedCategory == ECategory::Visuals && selectedVisualsItem == 0, true))
			{
				selectedCategory = ECategory::Visuals;
				selectedVisualsItem = 0;
			}
			if (SidebarItem("Objects ESP", selectedCategory == ECategory::Visuals && selectedVisualsItem == 1, true))
			{
				selectedCategory = ECategory::Visuals;
				selectedVisualsItem = 1;
			}
			if (SidebarItem("Visuals##VisualsSubItem", selectedCategory == ECategory::Visuals && selectedVisualsItem == 2, true))
			{
				selectedCategory = ECategory::Visuals;
				selectedVisualsItem = 2;
			}
			if (SidebarItem("Radar", selectedCategory == ECategory::Visuals && selectedVisualsItem == 3, true))
			{
				selectedCategory = ECategory::Visuals;
				selectedVisualsItem = 3;
			}
		}
		
		ImGui::Spacing();
		
		// Other category
		if (SidebarCategory("Other", true, selectedCategory == ECategory::Other))
		{
			selectedCategory = ECategory::Other;
		}
		if (selectedCategory == ECategory::Other || true)
		{
			if (SidebarItem("Misc", selectedCategory == ECategory::Other && selectedOtherItem == 0, true))
			{
				selectedCategory = ECategory::Other;
				selectedOtherItem = 0;
			}
			if (SidebarItem("Settings", selectedCategory == ECategory::Other && selectedOtherItem == 1, true))
			{
				selectedCategory = ECategory::Other;
				selectedOtherItem = 1;
			}
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
	
	// ========== MAIN CONTENT ==========
	ImGui::SameLine();
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
	ImGui::BeginChild("MainContent", ImVec2(0, 0), false);
	{
		// Render appropriate content based on selection
		if (selectedCategory == ECategory::Combat)
		{
			RenderAimbotContent();
		}
		else if (selectedCategory == ECategory::Visuals)
		{
			switch (selectedVisualsItem)
			{
			case 0: RenderEntityESPContent(); break;
			case 1: RenderObjectsESPContent(); break;
			case 2: RenderVisualsTabContent(); break;
			case 3: RenderRadarContent(); break;
			}
		}
		else if (selectedCategory == ECategory::Other)
		{
			switch (selectedOtherItem)
			{
			case 0: RenderMiscContent(); break;
			case 1: RenderSettingsContent(); break;
			}
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	
	ImGui::End();
}