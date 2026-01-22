#include "pch.h"
#include "Main Menu.h"

#include "GUI/Color Picker/Color Picker.h"
#include "GUI/Player Table/Player Table.h"
#include "GUI/Item Table/Item Table.h"
#include "GUI/Fuser/Fuser.h"
#include "GUI/Radar/Radar2D.h"
#include "GUI/Aimbot/Aimbot.h"
#include "GUI/ESP/ESPSettings.h"
#include "GUI/ESP/ESPPreview.h"
#include <string>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <ctime>
#include "GUI/Keybinds/Keybinds.h"
#include "GUI/Main Window/MonitorHelper.h"
#include "GUI/Main Window/Main Window.h"
#include "GUI/Config/Config.h"
#include "GUI/Flea Bot/Flea Bot.h"
#include "GUI/Fuser/Draw/Players.h"
#include "GUI/Fuser/Draw/Loot.h"
#include "GUI/Fuser/Draw/Exfils.h"

#include "Game/Camera List/Camera List.h"
#include "DebugMode.h"
#include "GUI/KmboxSettings/KmboxSettings.h"
#include "GUI/LootFilter/LootFilter.h"
#include "Game/Classes/Quest/CQuestManager.h"

// ============================================================================
// Logging helpers
// ============================================================================
static void LogUi(const std::string& msg)
{
	using namespace std::chrono;
	try
	{
		std::filesystem::create_directories("Logs");
		auto now = system_clock::now();
		auto t = system_clock::to_time_t(now);
		std::tm tm{};
		localtime_s(&tm, &t);
		char buf[32]{};
		std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
		std::ofstream out("Logs/ui.log", std::ios::app);
		if (out)
		{
			out << buf << " " << msg << "\n";
		}
	}
	catch (...)
	{
		// swallow logging errors
	}
}

// ============================================================================
// UI State
// ============================================================================
namespace MainMenuState
{
	// Sidebar selection - Radar is the default/first item
	enum class ECategory { Radar, Combat, Visuals, Other };
	enum class ECombatItem { Aimbot };
	enum class EVisualsItem { EntityESP, WorldESP, RadarConfig };  // Merged Objects+Visuals into WorldESP
	enum class EOtherItem { FleaBot, Tools, Settings };  // FleaBot separate, Misc->Tools
	
	static ECategory selectedCategory = ECategory::Radar;  // Default to Radar
	static int selectedCombatItem = 0;   // 0=Aimbot
	static int selectedVisualsItem = 0;  // 0=EntityESP, 1=WorldESP, 2=RadarConfig
	static int selectedOtherItem = 0;    // 0=FleaBot, 1=Tools, 2=Settings
	static bool bLogInputState = false;
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
// fixedHeight > 0 = use fixed height, fixedHeight = 0 = auto-resize to content
static void BeginPanel(const char* title, const char* subtitle = nullptr, float fixedHeight = 0.0f)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
	
	if (fixedHeight > 0.0f)
		ImGui::BeginChild(title, ImVec2(0, fixedHeight), ImGuiChildFlags_Borders);
	else
		ImGui::BeginChild(title, ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
	
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
	
	// ========================================================================
	// LEFT COLUMN - Core Settings
	// ========================================================================
	ImGui::BeginChild("AimbotLeft", ImVec2(panelWidth, 0), false);
	{
		// --- Aimbot Settings Panel ---
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

		ImGui::Spacing();

		// --- Targeting Panel ---
		BeginPanel("Targeting", "Bone selection & priority");

		// Target Bone Combo
		const char* boneItems[] = { "Head", "Neck", "Chest", "Torso", "Pelvis", "Random", "Closest Visible" };
		int currentBone = (int)Aimbot::eTargetBone;
		ImGui::Text("Target Bone");
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::Combo("##TargetBone", &currentBone, boneItems, IM_ARRAYSIZE(boneItems)))
		{
			Aimbot::eTargetBone = (ETargetBone)currentBone;
		}

		// Random Bone Sliders (shown only when Random is selected)
		if (Aimbot::eTargetBone == ETargetBone::RandomBone)
		{
			ImGui::Indent();
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Random Chances:");
			bool bWeightsChanged = false;
			ImGui::Text("Head %%");
			CustomSlider("##RHead", &Aimbot::fRandomHeadChance, 0.0f, 100.0f, "%.0f%%");
			bWeightsChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::Text("Neck %%");
			CustomSlider("##RNeck", &Aimbot::fRandomNeckChance, 0.0f, 100.0f, "%.0f%%");
			bWeightsChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::Text("Chest %%");
			CustomSlider("##RChest", &Aimbot::fRandomChestChance, 0.0f, 100.0f, "%.0f%%");
			bWeightsChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::Text("Torso %%");
			CustomSlider("##RTorso", &Aimbot::fRandomTorsoChance, 0.0f, 100.0f, "%.0f%%");
			bWeightsChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::Text("Pelvis %%");
			CustomSlider("##RPelvis", &Aimbot::fRandomPelvisChance, 0.0f, 100.0f, "%.0f%%");
			bWeightsChanged |= ImGui::IsItemDeactivatedAfterEdit();

			float total = Aimbot::fRandomHeadChance + Aimbot::fRandomNeckChance + Aimbot::fRandomChestChance + Aimbot::fRandomTorsoChance + Aimbot::fRandomPelvisChance;
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Total: %.0f%%", total);

			if (bWeightsChanged)
				Aimbot::NormalizeRandomWeights();

			ImGui::Unindent();
		}

		ImGui::Spacing();

		// Targeting Mode
		const char* modeItems[] = { "Closest to Crosshair", "Closest Distance" };
		int currentMode = (int)Aimbot::eTargetingMode;
		ImGui::Text("Priority");
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::Combo("##TargetMode", &currentMode, modeItems, IM_ARRAYSIZE(modeItems)))
		{
			Aimbot::eTargetingMode = (ETargetingMode)currentMode;
		}

		ImGui::Spacing();
		
		ImGui::Text("Max Distance");
		CustomSlider("##MaxDist", &Aimbot::fMaxDistance, 0.0f, 1000.0f, "%.0f m");
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = Unlimited");

		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	// ========================================================================
	// RIGHT COLUMN - Behavior, Filters & Visuals
	// ========================================================================
	ImGui::BeginChild("AimbotRight", ImVec2(0, 0), false);
	{
		// --- Behavior Panel (combines Advanced + Experimental) ---
		BeginPanel("Behavior", "Targeting logic & experimental");
		
		// Fireport Experimental (collapsible, above Prediction)
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Experimental");
		
		if (ImGui::TreeNode("Fireport Aiming"))
		{
			CustomToggle("Use Fireport Aiming", &Aimbot::bUseFireportAiming);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aim based on actual barrel direction.\nCompensates for weapon sway.");
			
			if (Aimbot::bUseFireportAiming)
			{
				ImGui::Text("Projection Distance");
				CustomSlider("##FireportDist", &Aimbot::fFireportProjectionDistance, 10.0f, 500.0f, "%.0f m");
			}
			
			ImGui::Spacing();
			CustomToggle("Draw Aim Point Dot", &Aimbot::bDrawAimPointDot);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows where weapon is actually aimed.");
			
			if (Aimbot::bDrawAimPointDot)
			{
				ImGui::Text("Dot Size");
				CustomSlider("##DotSize", &Aimbot::fAimPointDotSize, 1.0f, 20.0f, "%.1f px");
				ColorPickerButton("Dot Color", Aimbot::aimPointDotColor);
			}
			
			CustomToggle("Debug Visualization", &Aimbot::bDrawDebugVisualization);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show screen center vs fireport aim point.");
			
			ImGui::TreePop();
		}
		
		if (ImGui::TreeNode("Ballistics Prediction"))
		{
			CustomToggle("Enable Prediction", &Aimbot::bEnablePrediction);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Compensate for bullet drop and travel time\nusing live ammo ballistics data.");

			if (Aimbot::bEnablePrediction)
			{
				CustomToggle("Show Trajectory", &Aimbot::bShowTrajectory);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Visualize the predicted bullet path.");
			}
			
			ImGui::TreePop();
		}

		EndPanel();

		ImGui::Spacing();

		// --- Target Filters Panel ---
		BeginPanel("Target Filters", "Select who to target");

		CustomToggle("PMC", &Aimbot::bTargetPMC);
		CustomToggle("Player Scav", &Aimbot::bTargetPlayerScav);
		CustomToggle("AI Scav", &Aimbot::bTargetAIScav);
		CustomToggle("Boss", &Aimbot::bTargetBoss);
		CustomToggle("Raider/Rogue", &Aimbot::bTargetRaider);
		CustomToggle("Guard", &Aimbot::bTargetGuard);
		
		EndPanel();

		ImGui::Spacing();

		// --- Visuals Panel (combines Smoothing + Aimline) ---
		BeginPanel("Visuals", "Smoothing & aim visualization");
		
		// Smoothing section
		ImGui::Text("Dampen Factor");
		CustomSlider("##Dampen", &Aimbot::fDampen, 0.01f, 1.0f, "%.2f");
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Lower = smoother, Higher = faster");
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		// Aimline section - grayed out if trajectory is enabled
		bool trajectoryEnabled = Aimbot::bEnablePrediction && Aimbot::bShowTrajectory;
		
		if (trajectoryEnabled)
		{
			// Don't mutate user preference - just show disabled UI
			ImGui::BeginDisabled(true);
			bool tempVal = Aimbot::bDrawAimline; // Show current state but disabled
			CustomToggle("Draw Weapon Aimline", &tempVal);
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("Disabled: Trajectory visualization already shows aim direction.");
		}
		else
		{
			CustomToggle("Draw Weapon Aimline", &Aimbot::bDrawAimline);
		}
		
		ImGui::Spacing();
		CustomToggle("Only Show on Aim", &Aimbot::bOnlyOnAim);
		
		// Show aimline settings if enabled (and not overridden by trajectory)

		if (Aimbot::bDrawAimline && !trajectoryEnabled)
		{
			ImGui::Text("Aimline Length");
			CustomSlider("##AimlineLen", &Aimbot::fAimlineLength, 1.0f, 1000.0f, "%.0f m");
			
			ImGui::Text("Aimline Thickness");
			CustomSlider("##AimlineThick", &Aimbot::fAimlineThickness, 1.0f, 10.0f, "%.1f px");

			ImGui::Spacing();
			ColorPickerButton("Aimline Color", Aimbot::aimlineColor);
		}

		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderEntityESPContent()
{
	float totalWidth = ImGui::GetContentRegionAvail().x;
	float totalHeight = ImGui::GetContentRegionAvail().y;
	float previewWidth = totalWidth * 0.35f;  // 35% for preview
	float settingsWidth = totalWidth * 0.65f; // 65% for settings
	float topRowHeight = totalHeight * 0.65f; // 65% for ESP settings

	// ========================================================================
	// LEFT SIDE - Preview Panel (always visible)
	// ========================================================================
	ImGui::BeginChild("ESPPreviewPanel", ImVec2(previewWidth, 0), false);
	{
		BeginPanel("ESP Preview", "Live visualization", totalHeight - 20.0f);

		// Master toggle at top of preview
		CustomToggle("Enable Fuser (Master)", &Fuser::bMasterToggle);
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Render the preview using remaining space
		ImVec2 previewAvail = ImGui::GetContentRegionAvail();
		if (previewAvail.x > 50.0f && previewAvail.y > 100.0f)
		{
			ESPPreview::Render(previewAvail);
		}

		EndPanel();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// ========================================================================
	// RIGHT SIDE - ESP Settings + Player Range Settings
	// ========================================================================
	ImGui::BeginChild("ESPSettingsPanel", ImVec2(0, 0), false);
	{
		// ========================================================================
		// TOP ROW: 3-Column ESP Settings
		// ========================================================================
		ImGui::BeginChild("ESPTopRow", ImVec2(0, topRowHeight), false);
		{
			float colWidth = (settingsWidth - 30.0f) / 3.0f;  // 3 equal columns with spacing

			// --- Column 1: Box ESP ---
			ImGui::BeginChild("ESPCol1", ImVec2(colWidth, 0), false);
			{
				BeginPanel("Box ESP", nullptr);

				using namespace ESPSettings::Enemy;

				CustomToggle("Enable##Box", &bBoxEnabled);

				if (bBoxEnabled)
				{
					ImGui::Spacing();

					ImGui::Text("Style");
					ImGui::SetNextItemWidth(-1);
					static const char* styles[] = { "Full", "Corners" };
					ImGui::Combo("##BoxStyle", &boxStyle, styles, IM_ARRAYSIZE(styles));

					ImGui::Spacing();
					ColorPickerButton("Color##Box", boxColor);

					ImGui::Text("Thickness");
					ImGui::SetNextItemWidth(-1);
					ImGui::SliderFloat("##BoxThick", &boxThickness, 1.0f, 5.0f, "%.1f");

					ImGui::Spacing();
					CustomToggle("Filled##Box", &bBoxFilled);
					if (bBoxFilled)
					{
						ColorPickerButton("Fill##Box", boxFillColor);
					}
				}

				EndPanel();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			// --- Column 2: Skeleton ESP ---
			ImGui::BeginChild("ESPCol2", ImVec2(colWidth, 0), false);
			{
				BeginPanel("Skeleton", nullptr);

				using namespace ESPSettings::Enemy;

				CustomToggle("Enable##Skel", &bSkeletonEnabled);

				if (bSkeletonEnabled)
				{
					ImGui::Spacing();
					ColorPickerButton("Color##Skel", skeletonColor);

					ImGui::Text("Thickness");
					ImGui::SetNextItemWidth(-1);
					ImGui::SliderFloat("##SkelThick", &skeletonThickness, 1.0f, 5.0f, "%.1f");

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::Text("Bone Groups");
					CustomToggle("Head/Neck", &bBonesHead);
					CustomToggle("Spine", &bBonesSpine);
					CustomToggle("Left Arm", &bBonesArmsL);
					CustomToggle("Right Arm", &bBonesArmsR);
					CustomToggle("Left Leg", &bBonesLegsL);
					CustomToggle("Right Leg", &bBonesLegsR);
				}

				EndPanel();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			// --- Column 3: Info ESP ---
			ImGui::BeginChild("ESPCol3", ImVec2(0, 0), false);
			{
				BeginPanel("Info ESP", nullptr);

				using namespace ESPSettings::Enemy;

				CustomToggle("Name##Info", &bNameEnabled);
				if (bNameEnabled)
				{
					ImGui::SameLine();
					float col[4] = { nameColor.Value.x, nameColor.Value.y, nameColor.Value.z, nameColor.Value.w };
					if (ImGui::ColorEdit4("##NameCol", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
						nameColor = ImColor(col[0], col[1], col[2], col[3]);
				}

				CustomToggle("Weapon##Info", &bWeaponEnabled);
				if (bWeaponEnabled)
				{
					ImGui::SameLine();
					float col[4] = { weaponColor.Value.x, weaponColor.Value.y, weaponColor.Value.z, weaponColor.Value.w };
					if (ImGui::ColorEdit4("##WeaponCol", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
						weaponColor = ImColor(col[0], col[1], col[2], col[3]);
				}

				CustomToggle("Distance##Info", &bDistanceEnabled);
				if (bDistanceEnabled)
				{
					ImGui::SameLine();
					float col[4] = { distanceColor.Value.x, distanceColor.Value.y, distanceColor.Value.z, distanceColor.Value.w };
					if (ImGui::ColorEdit4("##DistCol", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
						distanceColor = ImColor(col[0], col[1], col[2], col[3]);
				}

				CustomToggle("Health Bar (doesnt work)##Info", &bHealthEnabled);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				CustomToggle("Head Dot##Info", &bHeadDotEnabled);
				if (bHeadDotEnabled)
				{
					ColorPickerButton("Dot Color##Info", headDotColor);
					ImGui::Text("Radius");
					ImGui::SetNextItemWidth(-1);
					ImGui::SliderFloat("##DotRad", &headDotRadius, 1.0f, 10.0f, "%.1f");
				}

				EndPanel();
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();

		ImGui::Spacing();

		// ========================================================================
		// BOTTOM ROW: Player Range Settings (2 columns)
		// ========================================================================
		ImGui::BeginChild("ESPBottomRow", ImVec2(0, 0), false);
		{
			float rangeColWidth = (settingsWidth - 24.0f) / 2.0f;

			// Column 1: PMC and Player Scav
			ImGui::BeginChild("PlayerRangeCol1", ImVec2(rangeColWidth, 0), false);
			{
				BeginPanel("Player Ranges", "ESP render distance + colors");

				using namespace ESPSettings::RenderRange;

				// PMC with inline color picker
				ImGui::Text("PMC");
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
				float pmcCol[4] = { PMCColor.Value.x, PMCColor.Value.y, PMCColor.Value.z, PMCColor.Value.w };
				if (ImGui::ColorEdit4("##PMCCol", pmcCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
					PMCColor = ImColor(pmcCol[0], pmcCol[1], pmcCol[2], pmcCol[3]);
				ImGui::SetNextItemWidth(-1);
				ImGui::SliderFloat("##PMCRange", &fPMCRange, 50.0f, 1000.0f, "%.0f m");

				ImGui::Spacing();

				// Player Scav with inline color picker
				ImGui::Text("Player Scav");
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
				float pscavCol[4] = { PlayerScavColor.Value.x, PlayerScavColor.Value.y, PlayerScavColor.Value.z, PlayerScavColor.Value.w };
				if (ImGui::ColorEdit4("##PScavCol", pscavCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
					PlayerScavColor = ImColor(pscavCol[0], pscavCol[1], pscavCol[2], pscavCol[3]);
				ImGui::SetNextItemWidth(-1);
				ImGui::SliderFloat("##PScavRange", &fPlayerScavRange, 50.0f, 1000.0f, "%.0f m");

				EndPanel();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			// Column 2: Boss and AI Scav
			ImGui::BeginChild("PlayerRangeCol2", ImVec2(0, 0), false);
			{
				BeginPanel("AI Ranges", nullptr);

				using namespace ESPSettings::RenderRange;

				// Boss with inline color picker
				ImGui::Text("Boss");
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
				float bossCol[4] = { BossColor.Value.x, BossColor.Value.y, BossColor.Value.z, BossColor.Value.w };
				if (ImGui::ColorEdit4("##BossCol", bossCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
					BossColor = ImColor(bossCol[0], bossCol[1], bossCol[2], bossCol[3]);
				ImGui::SetNextItemWidth(-1);
				ImGui::SliderFloat("##BossRange", &fBossRange, 50.0f, 1000.0f, "%.0f m");

				ImGui::Spacing();

				// AI Scav with inline color picker
				ImGui::Text("AI Scav");
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
				float aiCol[4] = { AIColor.Value.x, AIColor.Value.y, AIColor.Value.z, AIColor.Value.w };
				if (ImGui::ColorEdit4("##AICol", aiCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
					AIColor = ImColor(aiCol[0], aiCol[1], aiCol[2], aiCol[3]);
				ImGui::SetNextItemWidth(-1);
				ImGui::SliderFloat("##AIRange", &fAIRange, 50.0f, 1000.0f, "%.0f m");

				EndPanel();
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}

static void RenderWorldESPContent()
{
	float totalHeight = ImGui::GetContentRegionAvail().y;
	float topRowHeight = totalHeight * 0.45f;
	float colWidth = (ImGui::GetContentRegionAvail().x - 24.0f) / 3.0f;

	// ========================================================================
	// TOP ROW: Original 3 columns
	// ========================================================================
	ImGui::BeginChild("WorldTopRow", ImVec2(0, topRowHeight), false);
	{
		// Column 1: Loot & Objects
		ImGui::BeginChild("WorldCol1", ImVec2(colWidth, 0), false);
		{
			BeginPanel("Loot & Objects", nullptr);

			CustomToggle("Show Loot (Master)", &DrawESPLoot::bMasterToggle);

			ImGui::Spacing();

			CustomToggle("Containers", &DrawESPLoot::bContainerToggle);
			CustomToggle("Items", &DrawESPLoot::bItemToggle);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			CustomToggle("Show Exfils", &DrawExfils::bMasterToggle);

			EndPanel();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Column 2: Display Settings (Fuser/Monitor)
		ImGui::BeginChild("WorldCol2", ImVec2(colWidth, 0), false);
		{
			BeginPanel("Display Settings", nullptr);

			CustomToggle("ESP Overlay", &Fuser::bMasterToggle);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Monitor selection
			static bool bMonitorsInit = false;
			static std::vector<MonitorData> monitorList;
			auto RefreshMonitors = [&]()
			{
				MonitorHelper::RefreshMonitorCache();
				monitorList = MonitorHelper::GetAllMonitors();
				if (Fuser::m_SelectedMonitor < 0 || Fuser::m_SelectedMonitor >= static_cast<int>(monitorList.size()))
				{
					Fuser::m_SelectedMonitor = monitorList.empty() ? -1 : 0;
				}
				if (!monitorList.empty() && Fuser::m_SelectedMonitor >= 0 && Fuser::m_SelectedMonitor < static_cast<int>(monitorList.size()))
				{
					const auto& m = monitorList[Fuser::m_SelectedMonitor];
					Fuser::m_ScreenSize.x = static_cast<float>(std::abs(m.rcMonitor.right - m.rcMonitor.left));
					Fuser::m_ScreenSize.y = static_cast<float>(std::abs(m.rcMonitor.bottom - m.rcMonitor.top));
				}
			};
			if (!bMonitorsInit)
			{
				RefreshMonitors();
				bMonitorsInit = true;
			}

			ImGui::Text("Monitor");
			ImGui::SameLine();
			if (ImGui::SmallButton("Refresh##Mon"))
			{
				RefreshMonitors();
			}

			if (!monitorList.empty())
			{
				std::string preview = "Select...";
				if (Fuser::m_SelectedMonitor >= 0 && Fuser::m_SelectedMonitor < static_cast<int>(monitorList.size()))
				{
					preview = monitorList[Fuser::m_SelectedMonitor].friendlyName;
				}
				ImGui::SetNextItemWidth(-1);
				if (ImGui::BeginCombo("##Monitor", preview.c_str()))
				{
					for (const auto& m : monitorList)
					{
						bool isSelected = (Fuser::m_SelectedMonitor == m.index);
						if (ImGui::Selectable(m.friendlyName.c_str(), isSelected))
						{
							Fuser::m_SelectedMonitor = m.index;
							Fuser::m_ScreenSize.x = static_cast<float>(std::abs(m.rcMonitor.right - m.rcMonitor.left));
							Fuser::m_ScreenSize.y = static_cast<float>(std::abs(m.rcMonitor.bottom - m.rcMonitor.top));
							MonitorHelper::MoveWindowToMonitor(MainWindow::g_hWnd, m.index);
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No displays");
			}

			ImGui::Spacing();
			ImGui::Text("Screen: %.0fx%.0f", Fuser::m_ScreenSize.x, Fuser::m_ScreenSize.y);

			EndPanel();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Column 3: Optic ESP & Indicators
		ImGui::BeginChild("WorldCol3", ImVec2(0, 0), false);
		{
			BeginPanel("Optic ESP", nullptr);

			CustomToggle("Enable Optic ESP", &DrawESPPlayers::bOpticESP);

			if (DrawESPPlayers::bOpticESP)
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// Auto-detection status display (read-only)
				if (CameraList::IsScoped())
				{
					ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Status: Active");
					ImGui::Text("Zoom: %.1fx", CameraList::GetPlayerScopeZoom());
					ImGui::Text("Radius: %.0f px", CameraList::GetOpticRadius());
				}
				else
				{
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Status: Not Scoped");
				}
			}

			EndPanel();

			ImGui::Spacing();

			BeginPanel("Data Tables", nullptr);

			CustomToggle("Player Table", &PlayerTable::bMasterToggle);
			CustomToggle("Item Table", &ItemTable::bMasterToggle);

			EndPanel();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();

	ImGui::Spacing();

	// ========================================================================
	// BOTTOM ROW: Item & World Ranges with Colors (3 columns)
	// ========================================================================
	ImGui::BeginChild("WorldBottomRow", ImVec2(0, 0), false);
	{
		float rangeColWidth = (ImGui::GetContentRegionAvail().x - 24.0f) / 3.0f;

		// Column 1: High & Medium Value Items
		ImGui::BeginChild("RangeCol1", ImVec2(rangeColWidth, 0), false);
		{
			BeginPanel("Item Ranges", "By value tier with colors");

			using namespace ESPSettings::RenderRange;

			// High Value with color picker
			ImGui::Text("High Value (>100k)");
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
			float highCol[4] = { ItemHighColor.Value.x, ItemHighColor.Value.y, ItemHighColor.Value.z, ItemHighColor.Value.w };
			if (ImGui::ColorEdit4("##HighCol", highCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
				ItemHighColor = ImColor(highCol[0], highCol[1], highCol[2], highCol[3]);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("##HighRange", &fItemHighRange, 10.0f, 500.0f, "%.0f m");

			ImGui::Spacing();

			// Medium Value with color picker
			ImGui::Text("Medium (50k-100k)");
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
			float medCol[4] = { ItemMediumColor.Value.x, ItemMediumColor.Value.y, ItemMediumColor.Value.z, ItemMediumColor.Value.w };
			if (ImGui::ColorEdit4("##MedCol", medCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
				ItemMediumColor = ImColor(medCol[0], medCol[1], medCol[2], medCol[3]);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("##MedRange", &fItemMediumRange, 10.0f, 300.0f, "%.0f m");

			EndPanel();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Column 2: Low & Rest Value Items
		ImGui::BeginChild("RangeCol2", ImVec2(rangeColWidth, 0), false);
		{
			BeginPanel("Low Value Items", nullptr);

			using namespace ESPSettings::RenderRange;

			// Low Value with color picker
			ImGui::Text("Low (20k-50k)");
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
			float lowCol[4] = { ItemLowColor.Value.x, ItemLowColor.Value.y, ItemLowColor.Value.z, ItemLowColor.Value.w };
			if (ImGui::ColorEdit4("##LowCol", lowCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
				ItemLowColor = ImColor(lowCol[0], lowCol[1], lowCol[2], lowCol[3]);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("##LowRange", &fItemLowRange, 10.0f, 200.0f, "%.0f m");

			ImGui::Spacing();

			// Rest Value with color picker
			ImGui::Text("Rest (<20k)");
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
			float restCol[4] = { ItemRestColor.Value.x, ItemRestColor.Value.y, ItemRestColor.Value.z, ItemRestColor.Value.w };
			if (ImGui::ColorEdit4("##RestCol", restCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
				ItemRestColor = ImColor(restCol[0], restCol[1], restCol[2], restCol[3]);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("##RestRange", &fItemRestRange, 5.0f, 100.0f, "%.0f m");

			EndPanel();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Column 3: Container/Exfil Range & Reset
		ImGui::BeginChild("RangeCol3", ImVec2(0, 0), false);
		{
			BeginPanel("World Ranges", nullptr);

			using namespace ESPSettings::RenderRange;

			// Container with color picker
			ImGui::Text("Containers");
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
			float contCol[4] = { ContainerColor.Value.x, ContainerColor.Value.y, ContainerColor.Value.z, ContainerColor.Value.w };
			if (ImGui::ColorEdit4("##ContCol", contCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
				ContainerColor = ImColor(contCol[0], contCol[1], contCol[2], contCol[3]);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("##ContainerRange", &fContainerRange, 5.0f, 100.0f, "%.0f m");

			ImGui::Spacing();

			// Exfil with color picker
			ImGui::Text("Exfils");
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
			float exfilCol[4] = { ExfilColor.Value.x, ExfilColor.Value.y, ExfilColor.Value.z, ExfilColor.Value.w };
			if (ImGui::ColorEdit4("##ExfilCol", exfilCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
				ExfilColor = ImColor(exfilCol[0], exfilCol[1], exfilCol[2], exfilCol[3]);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("##ExfilRange", &fExfilRange, 50.0f, 1000.0f, "%.0f m");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button("Reset to Defaults", ImVec2(-1, 0)))
			{
				// Player colors
				PMCColor = ImColor(255, 0, 0);
				PlayerScavColor = ImColor(255, 165, 0);
				BossColor = ImColor(255, 0, 255);
				AIColor = ImColor(255, 255, 0);
				// Player ranges
				fPMCRange = 500.0f;
				fPlayerScavRange = 400.0f;
				fBossRange = 600.0f;
				fAIRange = 300.0f;
				// Item colors
				ItemHighColor = ImColor(255, 215, 0);
				ItemMediumColor = ImColor(186, 85, 211);
				ItemLowColor = ImColor(0, 150, 150);
				ItemRestColor = ImColor(108, 150, 150);
				// Item ranges
				fItemHighRange = 100.0f;
				fItemMediumRange = 75.0f;
				fItemLowRange = 50.0f;
				fItemRestRange = 25.0f;
				// Container
				ContainerColor = ImColor(0, 255, 255);
				fContainerRange = 20.0f;
				// Exfil
				ExfilColor = ImColor(0, 255, 0);
				fExfilRange = 500.0f;
			}

			EndPanel();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}

static void RenderRadarConfigContent()
{
	float panelWidth = ImGui::GetContentRegionAvail().x * 0.48f;
	float panelHeight = ImGui::GetContentRegionAvail().y * 0.45f;

	// Top row - Radar settings
	ImGui::BeginChild("RadarLeft", ImVec2(panelWidth, panelHeight), false);
	{
		BeginPanel("Radar Settings", "2D radar minimap");

		CustomToggle("Enable Radar", &Radar2D::bEnabled);
		CustomToggle("Show Map Image", &Radar2D::bShowMapImage);
		CustomToggle("Auto Map Selection", &Radar2D::bAutoMap);
		CustomToggle("Auto Floor Switch", &Radar2D::bAutoFloorSwitch);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Display Options");
		CustomToggle("Show Local Player", &Radar2D::bShowLocalPlayer);
		CustomToggle("Show Players", &Radar2D::bShowPlayers);
		CustomToggle("Show Loot", &Radar2D::bShowLoot);
		CustomToggle("Show Exfils", &Radar2D::bShowExfils);

		EndPanel();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("RadarRight", ImVec2(0, panelHeight), false);
	{
		BeginPanel("Radar Visuals", "Display settings");

		ImGui::Text("Zoom (1=close, 200=far)");
		ImGui::SetNextItemWidth(180.0f);
		ImGui::SliderInt("##Zoom", &Radar2D::iZoom, Radar2D::MIN_ZOOM, Radar2D::MAX_ZOOM);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Icon Sizes");
		ImGui::Text("Player Icon");
		CustomSlider("##PlayerIcon", &Radar2D::fPlayerIconSize, 2.0f, 20.0f, "%.1f px");

		ImGui::Text("Loot Icon");
		CustomSlider("##LootIcon", &Radar2D::fLootIconSize, 1.0f, 15.0f, "%.1f px");

		ImGui::Text("Exfil Icon");
		CustomSlider("##ExfilIcon", &Radar2D::fExfilIconSize, 4.0f, 25.0f, "%.1f px");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Current Floor");
		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputInt("##Floor", &Radar2D::iCurrentFloor);

		EndPanel();
	}
	ImGui::EndChild();

	// Bottom row - Loot Filter and Quest Helper (side by side)
	float bottomPanelWidth = ImGui::GetContentRegionAvail().x * 0.5f;

	ImGui::BeginChild("LootFilterSection", ImVec2(bottomPanelWidth, 0), false);
	{
		LootFilter::RenderSettings();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("QuestHelperSection", ImVec2(0, 0), false);
	{
		BeginPanel("Quest Helper", "Track quest objectives");

		auto& questMgr = Quest::CQuestManager::GetInstance();

		static Quest::CQuestManager::QuestSnapshot s_CachedSnapshot{};
		static std::chrono::steady_clock::time_point s_LastUpdate{};
		auto now = std::chrono::steady_clock::now();
		if (s_LastUpdate == std::chrono::steady_clock::time_point{} ||
			now - s_LastUpdate > std::chrono::milliseconds(100))
		{
			s_CachedSnapshot = questMgr.GetSnapshot();
			s_LastUpdate = now;
		}

		auto settings = s_CachedSnapshot.Settings;
		bool settingsChanged = false;

		settingsChanged |= CustomToggle("Enable Quest Helper", &settings.bEnabled);

		if (settings.bEnabled)
		{
			ImGui::Spacing();
			settingsChanged |= CustomToggle("Show Quest Locations", &settings.bShowQuestLocations);
			settingsChanged |= CustomToggle("Highlight Quest Items", &settings.bHighlightQuestItems);
			settingsChanged |= CustomToggle("Show Quest Panel", &settings.bShowQuestPanel);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Quest Marker Size");
			float markerSize = settings.fQuestMarkerSize;
			CustomSlider("##QuestMarkerSize", &settings.fQuestMarkerSize, 4.0f, 20.0f, "%.1f px");
			settingsChanged |= (markerSize != settings.fQuestMarkerSize);

			ImGui::Spacing();

			// Quest location color picker
			float qCol[4] = { settings.QuestLocationColor.x, settings.QuestLocationColor.y,
				              settings.QuestLocationColor.z, settings.QuestLocationColor.w };
			ImGui::Text("Quest Marker Color");
			if (ImGui::ColorEdit4("##QuestColor", qCol, ImGuiColorEditFlags_NoInputs))
			{
				settings.QuestLocationColor = ImVec4(qCol[0], qCol[1], qCol[2], qCol[3]);
				settingsChanged = true;
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Active quests list (snapshot cached)
			auto& activeQuests = s_CachedSnapshot.ActiveQuests;

			ImGui::Text("Active Quests: %zu", activeQuests.size());

			if (!activeQuests.empty())
			{
				ImGui::BeginChild("QuestList", ImVec2(0, 150), true);
				for (auto& [questId, quest] : activeQuests)
				{
					bool enabled = quest.bIsEnabled;
					if (ImGui::Checkbox(("##qe_" + questId).c_str(), &enabled))
					{
						questMgr.SetQuestEnabled(questId, enabled);
						quest.bIsEnabled = enabled;
						if (enabled)
							settings.BlacklistedQuests.erase(questId);
						else
							settings.BlacklistedQuests.insert(questId);
					}
					ImGui::SameLine();
					ImGui::Text("%s", quest.Name.c_str());
				}
				ImGui::EndChild();
			}
			else
			{
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No active quests detected.");
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Quests appear when in raid.");
			}

			ImGui::Spacing();

			// Quest locations count
			const auto& questLocs = s_CachedSnapshot.QuestLocations;
			ImGui::Text("Quest Markers on Map: %zu", questLocs.size());
		}

		s_CachedSnapshot.Settings = settings;
		if (settingsChanged)
		{
			questMgr.UpdateSettings(settings);
		}

		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderRadarViewContent()
{
	// This renders the full radar map in the content area
	Radar2D::RenderEmbedded();
}

static void RenderFleaBotContent()
{
	// FleaBot initialization and thread management
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

	float colWidth = (ImGui::GetContentRegionAvail().x - 16.0f) / 2.0f;
	
	// ========================================================================
	// Left Column: Flea Bot Settings
	// ========================================================================
	ImGui::BeginChild("FleaLeft", ImVec2(colWidth, 0), false);
	{
		BeginPanel("Flea Bot", "Automatic flea market purchasing");
		
		CustomToggle("Enable Flea Bot", &FleaBot::bMasterToggle);
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		ImGui::Text("Purchase Mode");
		CustomToggle("Cycle Buy", &FleaBot::bCycleBuy);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Continuously cycle through items");
		
		CustomToggle("Limit Buy", &FleaBot::bLimitBuy);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop after reaching item count");
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		ImGui::Text("Item Count");
		ImGui::SetNextItemWidth(-1);
		ImGui::InputScalar("##ItemCount", ImGuiDataType_U32, &FleaBot::m_ItemCount);
		
		ImGui::Spacing();
		
		// Status display
		ImGui::Separator();
		ImGui::Spacing();
		if (FleaBot::bMasterToggle)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Status: Running");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Status: Stopped");
		}
		
		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	// ========================================================================
	// Right Column: Price List Info
	// ========================================================================
	ImGui::BeginChild("FleaRight", ImVec2(0, 0), false);
	{
		BeginPanel("Price List", "Loaded item prices");
		
		ImGui::Text("Items Loaded: %zu", FleaBot::m_PriceList.size());
		
		ImGui::Spacing();
		
		if (ImGui::Button("Reload Prices", ImVec2(-1, 0)))
		{
			FleaBot::m_PriceList.clear();
			FleaBot::LoadPriceList();
			FleaBot::ConstructPriceList();
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		// Scrollable price list preview
		ImGui::Text("Recent Items:");
		ImGui::BeginChild("PriceListScroll", ImVec2(0, 200), true);
		{
			int count = 0;
			for (const auto& item : FleaBot::m_PriceList)
			{
				if (count++ >= 50) break;  // Show first 50 items
				ImGui::Text("%s", item.first.c_str());
			}
			if (FleaBot::m_PriceList.empty())
			{
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No items loaded");
			}
		}
		ImGui::EndChild();
		
		EndPanel();
	}
	ImGui::EndChild();
}

static void RenderToolsContent()
{
	float colWidth = (ImGui::GetContentRegionAvail().x - 16.0f) / 2.0f;
	
	// ========================================================================
	// Left Column: Data Tables
	// ========================================================================
	ImGui::BeginChild("ToolsLeft", ImVec2(colWidth, 0), false);
	{
		BeginPanel("Data Tables", "Information displays");
		
		CustomToggle("Player Table", &PlayerTable::bMasterToggle);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show table with player information");
		
		CustomToggle("Item Table", &ItemTable::bMasterToggle);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show table with loot items");
		
		EndPanel();
	}
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	// ========================================================================
	// Right Column: Debug & Future Utilities
	// ========================================================================
	ImGui::BeginChild("ToolsRight", ImVec2(0, 0), false);
	{
		BeginPanel("Debug", "Development tools");
		
		CustomToggle("Log Input State", &MainMenuState::bLogInputState);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Log mouse/keyboard capture state");
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Additional utilities");
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "will appear here...");
		
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
			
			float colWidth = (ImGui::GetContentRegionAvail().x - 24.0f) / 3.0f;
			
			// Column 1: Fuser Player Colors
			ImGui::BeginChild("ColorsCol1", ImVec2(colWidth, 0), false);
			{
				BeginPanel("Fuser Players", nullptr);
				
				ColorPickerButton("PMC##FP", ColorPicker::Fuser::m_PMCColor);
				ColorPickerButton("Scav##FP", ColorPicker::Fuser::m_ScavColor);
				ColorPickerButton("Boss##FP", ColorPicker::Fuser::m_BossColor);
				ColorPickerButton("Player Scav##FP", ColorPicker::Fuser::m_PlayerScavColor);
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::SameLine();
			
			// Column 2: Fuser World Colors
			ImGui::BeginChild("ColorsCol2", ImVec2(colWidth, 0), false);
			{
				BeginPanel("Fuser World", nullptr);
				
				ColorPickerButton("Loot##FW", ColorPicker::Fuser::m_LootColor);
				ColorPickerButton("Container##FW", ColorPicker::Fuser::m_ContainerColor);
				ColorPickerButton("Exfil##FW", ColorPicker::Fuser::m_ExfilColor);
				
				EndPanel();
			}
			ImGui::EndChild();
			
			ImGui::SameLine();
			
			// Column 3: Radar Colors
			ImGui::BeginChild("ColorsCol3", ImVec2(0, 0), false);
			{
				BeginPanel("Radar Colors", nullptr);
				
				ImGui::Text("Players");
				ColorPickerButton("PMC##R", ColorPicker::Radar::m_PMCColor);
				ColorPickerButton("Scav##R", ColorPicker::Radar::m_ScavColor);
				ColorPickerButton("Boss##R", ColorPicker::Radar::m_BossColor);
				ColorPickerButton("Player Scav##R", ColorPicker::Radar::m_PlayerScavColor);
				ColorPickerButton("Local##R", ColorPicker::Radar::m_LocalPlayerColor);
				
				ImGui::Spacing();
				ImGui::Text("World");
				ColorPickerButton("Loot##RW", ColorPicker::Radar::m_LootColor);
				ColorPickerButton("Container##RW", ColorPicker::Radar::m_ContainerColor);
				ColorPickerButton("Exfil##RW", ColorPicker::Radar::m_ExfilColor);
				
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
			
			if (ImGui::BeginTable("KeybindsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Target PC", ImGuiTableColumnFlags_WidthFixed, 70.0f);
				ImGui::TableSetupColumn("Radar PC", ImGuiTableColumnFlags_WidthFixed, 70.0f);
				ImGui::TableSetupColumn("Toggle", ImGuiTableColumnFlags_WidthFixed, 60.0f);
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
	
	// Get main viewport - we want to fill the entire application window
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	
	// Set window to fill entire viewport
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	
	// Window flags: no title bar, no resize, no move, no collapse, no docking
	// This makes the ImGui window act as the entire application UI
	ImGuiWindowFlags windowFlags = 
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("##MainPanel", nullptr, windowFlags);
	ImGui::PopStyleVar(3);
	
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
		
		// Radar - First item, shows the radar map view
		if (SidebarCategory("Radar", true, selectedCategory == ECategory::Radar))
		{
			selectedCategory = ECategory::Radar;
		}
		
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
			if (SidebarItem("World ESP", selectedCategory == ECategory::Visuals && selectedVisualsItem == 1, true))
			{
				selectedCategory = ECategory::Visuals;
				selectedVisualsItem = 1;
			}
			if (SidebarItem("Radar Config", selectedCategory == ECategory::Visuals && selectedVisualsItem == 2, true))
			{
				selectedCategory = ECategory::Visuals;
				selectedVisualsItem = 2;
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
			if (SidebarItem("Flea Bot", selectedCategory == ECategory::Other && selectedOtherItem == 0, true))
			{
				selectedCategory = ECategory::Other;
				selectedOtherItem = 0;
			}
			if (SidebarItem("Tools", selectedCategory == ECategory::Other && selectedOtherItem == 1, true))
			{
				selectedCategory = ECategory::Other;
				selectedOtherItem = 1;
			}
			if (SidebarItem("Settings", selectedCategory == ECategory::Other && selectedOtherItem == 2, true))
			{
				selectedCategory = ECategory::Other;
				selectedOtherItem = 2;
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
		if (MainMenuState::bLogInputState)
		{
			static auto s_lastLog = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();
			if (now - s_lastLog > std::chrono::milliseconds(500))
			{
				ImGuiIO& io = ImGui::GetIO();
				LogUi(std::string("IO capture mouse=") + (io.WantCaptureMouse ? "1" : "0") + ", keyboard=" + (io.WantCaptureKeyboard ? "1" : "0"));
				s_lastLog = now;
			}
		}
		// Render appropriate content based on selection
		if (selectedCategory == ECategory::Radar)
		{
			RenderRadarViewContent();
		}
		else if (selectedCategory == ECategory::Combat)
		{
			RenderAimbotContent();
		}
		else if (selectedCategory == ECategory::Visuals)
		{
			switch (selectedVisualsItem)
			{
			case 0: RenderEntityESPContent(); break;
			case 1: RenderWorldESPContent(); break;
			case 2: RenderRadarConfigContent(); break;
			}
		}
		else if (selectedCategory == ECategory::Other)
		{
			switch (selectedOtherItem)
			{
			case 0: RenderFleaBotContent(); break;
			case 1: RenderToolsContent(); break;
			case 2: RenderSettingsContent(); break;
			}
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	
	ImGui::End();
}
