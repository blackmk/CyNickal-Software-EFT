#pragma once
#include "GUI/Radar/Widgets/PlayerInfoWidget.h"
#include "GUI/Radar/Widgets/LootInfoWidget.h"
#include "GUI/Radar/Widgets/AimviewWidget.h"

// Central manager for radar overlay widgets
class WidgetManager
{
public:
	// Widget visibility toggles
	static inline bool bShowPlayerInfoWidget = true;
	static inline bool bShowLootInfoWidget = true;
	static inline bool bShowAimviewWidget = false;  // Off by default

	// Render all visible widgets
	static void RenderWidgets()
	{
		if (bShowPlayerInfoWidget)
		{
			PlayerInfoWidget::GetInstance().RenderFrame();
		}

		if (bShowLootInfoWidget)
		{
			LootInfoWidget::GetInstance().RenderFrame();
		}

		if (bShowAimviewWidget)
		{
			AimviewWidget::GetInstance().RenderFrame();
		}
	}

	// Render widget settings UI
	static void RenderSettings()
	{
		if (ImGui::CollapsingHeader("Overlay Widgets"))
		{
			ImGui::Indent();

			ImGui::Checkbox("Player Info Widget##WM", &bShowPlayerInfoWidget);
			if (bShowPlayerInfoWidget)
			{
				ImGui::Indent();
				ImGui::SliderInt("Max Players##PIW", &PlayerInfoWidget::MaxDisplayPlayers, 5, 50);
				ImGui::Checkbox("Sort by Distance##PIW", &PlayerInfoWidget::SortByDistance);
				ImGui::Checkbox("Show Weapon##PIW", &PlayerInfoWidget::ShowWeapon);
				ImGui::Checkbox("Show Health##PIW", &PlayerInfoWidget::ShowHealth);
				ImGui::Checkbox("Show Distance##PIW", &PlayerInfoWidget::ShowDistance);
				ImGui::Unindent();
			}

			ImGui::Spacing();

			ImGui::Checkbox("Loot Info Widget##WM", &bShowLootInfoWidget);
			if (bShowLootInfoWidget)
			{
				ImGui::Indent();
				ImGui::SliderInt("Max Items##LIW", &LootInfoWidget::MaxDisplayItems, 5, 50);
				ImGui::Checkbox("Sort by Value##LIW", &LootInfoWidget::SortByValue);
				ImGui::Checkbox("Show Price##LIW", &LootInfoWidget::ShowPrice);
				ImGui::Checkbox("Show Distance##LIW", &LootInfoWidget::ShowDistance);
				ImGui::Unindent();
			}

			ImGui::Spacing();

			ImGui::Checkbox("Aimview Widget##WM", &bShowAimviewWidget);
			if (bShowAimviewWidget)
			{
				ImGui::Indent();
				ImGui::SliderFloat("FOV##AVW", &AimviewWidget::FOV, 30.0f, 120.0f, "%.0f deg");
				ImGui::SliderFloat("Max Distance##AVW", &AimviewWidget::MaxDistance, 50.0f, 500.0f, "%.0fm");
				ImGui::Checkbox("Show Crosshair##AVW", &AimviewWidget::ShowCrosshair);
				ImGui::Checkbox("Show Distance Circles##AVW", &AimviewWidget::ShowDistanceCircles);
				ImGui::Unindent();
			}

			ImGui::Unindent();
		}
	}

	// Serialize all widget states
	static json SerializeWidgets()
	{
		json j;
		j["bShowPlayerInfoWidget"] = bShowPlayerInfoWidget;
		j["bShowLootInfoWidget"] = bShowLootInfoWidget;
		j["bShowAimviewWidget"] = bShowAimviewWidget;
		j["PlayerInfoWidget"] = PlayerInfoWidget::GetInstance().Serialize();
		j["LootInfoWidget"] = LootInfoWidget::GetInstance().Serialize();
		j["AimviewWidget"] = AimviewWidget::GetInstance().Serialize();
		return j;
	}

	// Deserialize all widget states
	static void DeserializeWidgets(const json& j)
	{
		if (j.contains("bShowPlayerInfoWidget")) bShowPlayerInfoWidget = j["bShowPlayerInfoWidget"].get<bool>();
		if (j.contains("bShowLootInfoWidget")) bShowLootInfoWidget = j["bShowLootInfoWidget"].get<bool>();
		if (j.contains("bShowAimviewWidget")) bShowAimviewWidget = j["bShowAimviewWidget"].get<bool>();
		if (j.contains("PlayerInfoWidget")) PlayerInfoWidget::GetInstance().Deserialize(j["PlayerInfoWidget"]);
		if (j.contains("LootInfoWidget")) LootInfoWidget::GetInstance().Deserialize(j["LootInfoWidget"]);
		if (j.contains("AimviewWidget")) AimviewWidget::GetInstance().Deserialize(j["AimviewWidget"]);
	}
};
