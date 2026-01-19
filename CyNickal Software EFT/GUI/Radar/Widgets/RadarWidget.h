#pragma once
#include <string>
#include "imgui.h"
#include <json.hpp>

using json = nlohmann::json;

// Base class for radar widgets (PlayerInfo, LootInfo, Aimview)
class RadarWidget
{
public:
	std::string title = "Widget";
	ImVec2 position = ImVec2(10, 10);
	ImVec2 size = ImVec2(300, 200);
	bool minimized = false;
	bool visible = true;

	RadarWidget() = default;
	RadarWidget(const std::string& widgetTitle, const ImVec2& pos, const ImVec2& sz)
		: title(widgetTitle), position(pos), size(sz) {}

	virtual ~RadarWidget() = default;

	// Main render function - override in derived classes
	virtual void Render() = 0;

	// Render the widget with frame (title bar, minimize, resize)
	void RenderFrame()
	{
		if (!visible)
			return;

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

		if (minimized)
		{
			flags |= ImGuiWindowFlags_NoResize;
			ImGui::SetNextWindowSize(ImVec2(size.x, 30));
		}
		else
		{
			ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
		}

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);

		// Push custom styling for widget windows
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.85f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

		std::string windowId = title + "##Widget";
		if (ImGui::Begin(windowId.c_str(), &visible, flags))
		{
			// Store current position and size
			position = ImGui::GetWindowPos();
			if (!minimized)
				size = ImGui::GetWindowSize();

			// Minimize button in title bar
			ImVec2 titleBarMin = ImGui::GetWindowPos();
			ImVec2 titleBarMax = ImVec2(titleBarMin.x + ImGui::GetWindowWidth(), titleBarMin.y + ImGui::GetFrameHeight());

			// Check for right-click to minimize
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
				ImGui::IsMouseHoveringRect(titleBarMin, titleBarMax))
			{
				minimized = !minimized;
			}

			if (!minimized)
			{
				Render();
			}
			else
			{
				ImGui::TextDisabled("(right-click to expand)");
			}
		}
		ImGui::End();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
	}

	// Serialize widget state to JSON
	virtual json Serialize() const
	{
		return json{
			{"title", title},
			{"position", {position.x, position.y}},
			{"size", {size.x, size.y}},
			{"minimized", minimized},
			{"visible", visible}
		};
	}

	// Deserialize widget state from JSON
	virtual void Deserialize(const json& j)
	{
		if (j.contains("title")) title = j["title"].get<std::string>();
		if (j.contains("position") && j["position"].is_array() && j["position"].size() == 2)
		{
			position.x = j["position"][0].get<float>();
			position.y = j["position"][1].get<float>();
		}
		if (j.contains("size") && j["size"].is_array() && j["size"].size() == 2)
		{
			size.x = j["size"][0].get<float>();
			size.y = j["size"][1].get<float>();
		}
		if (j.contains("minimized")) minimized = j["minimized"].get<bool>();
		if (j.contains("visible")) visible = j["visible"].get<bool>();
	}
};
