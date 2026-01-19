#pragma once
#include "GUI/Radar/Widgets/RadarWidget.h"
#include "Game/Classes/Vector.h"

// Forward declarations
class CObservedPlayer;
class CClientPlayer;

// Widget showing 3D perspective view of players relative to local player's view direction
class AimviewWidget : public RadarWidget
{
public:
	// Singleton instance
	static AimviewWidget& GetInstance()
	{
		static AimviewWidget instance;
		return instance;
	}

	// Settings
	static inline float FOV = 60.0f;            // Field of view angle (degrees)
	static inline float MaxDistance = 200.0f;   // Max distance to show players
	static inline bool ShowCrosshair = true;
	static inline bool ShowDistanceCircles = true;
	static inline float CrosshairSize = 8.0f;

	void Render() override;

	// Serialize/Deserialize with additional settings
	json Serialize() const override;
	void Deserialize(const json& j) override;

private:
	AimviewWidget() : RadarWidget("Aimview", ImVec2(370, 10), ImVec2(250, 250))
	{
		visible = false;  // Off by default
	}

	// Project world position to aimview coordinates
	// Returns screen position within widget, and whether it's in front of player
	bool WorldToAimview(const Vector3& worldPos, const Vector3& localPos,
		float localYaw, float localPitch, ImVec2& outPos, float& outDistance);

	// Draw a player dot with depth-based size
	void DrawPlayerDot(ImDrawList* drawList, const ImVec2& center, const ImVec2& pos,
		float distance, ImU32 color, bool isFocused);

	// Draw the crosshair
	void DrawCrosshair(ImDrawList* drawList, const ImVec2& center);

	// Draw distance reference circles
	void DrawDistanceCircles(ImDrawList* drawList, const ImVec2& center, float radius);

	// Get player type color
	static ImU32 GetPlayerColor(const CObservedPlayer& player);
	static ImU32 GetPlayerColor(const CClientPlayer& player);
};
