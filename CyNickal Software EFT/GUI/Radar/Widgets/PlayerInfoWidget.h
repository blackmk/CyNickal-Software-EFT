#pragma once
#include "GUI/Radar/Widgets/RadarWidget.h"
#include "Game/Classes/Vector.h"

// Forward declarations
class CClientPlayer;
class CObservedPlayer;

// Widget displaying a table of all alive hostile players
class PlayerInfoWidget : public RadarWidget
{
public:
	// Singleton instance
	static PlayerInfoWidget& GetInstance()
	{
		static PlayerInfoWidget instance;
		return instance;
	}

	// Settings
	static inline int MaxDisplayPlayers = 20;
	static inline bool SortByDistance = true;
	static inline bool ShowWeapon = true;
	static inline bool ShowDistance = true;
	static inline bool ShowHealth = true;

	void Render() override;

	// Serialize/Deserialize with additional settings
	json Serialize() const override;
	void Deserialize(const json& j) override;

private:
	PlayerInfoWidget() : RadarWidget("Players", ImVec2(10, 10), ImVec2(350, 250)) {}

	// Render a single player row
	void RenderPlayerRow(const CObservedPlayer& player, const Vector3& localPos, int index);
	void RenderPlayerRow(const CClientPlayer& player, const Vector3& localPos, int index);

	// Helper to get player type color
	static ImU32 GetPlayerTypeColor(const CObservedPlayer& player);
	static ImU32 GetPlayerTypeColor(const CClientPlayer& player);

	// Helper to get distance string
	static std::string GetDistanceString(float distance);
};
