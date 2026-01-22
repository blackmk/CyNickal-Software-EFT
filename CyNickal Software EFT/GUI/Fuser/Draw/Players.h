#pragma once
#include "Game/Classes/Players/CObservedPlayer/CObservedPlayer.h"
#include "Game/Classes/Players/CClientPlayer/CClientPlayer.h"
#include "Game/Classes/CPlayerSkeleton/CPlayerSkeleton.h"
#include "Game/Classes/CHeldItem/CHeldItem.h"

struct ProjectedBoneInfo
{
	Vector2 ScreenPos{};
	bool bIsOnScreen{ false };
};

class DrawESPPlayers
{
public:
	static void DrawAll(const ImVec2& WindowPos, ImDrawList* DrawList);

	// Legacy toggles for backward compatibility with code that references them
	static inline bool bOpticESP{ true };

private:
	static void DrawGenericPlayerText(const CBaseEFTPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const ImColor& Color, uint8_t& LineNumber, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones);
	static void DrawObservedPlayerHealthText(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const ImColor& Color, uint8_t& LineNumber, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones);
	static void DrawHealthBar(ImDrawList* DrawList, const ImVec2& bMin, const ImVec2& bMax, float healthPercent, const ImColor& healthColor);
	static void DrawPlayerWeapon(const CHeldItem* pHands, const ImVec2& WindowPos, ImDrawList* DrawList, uint8_t& LineNumber, const std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones);
	static void Draw(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const Vector3& LocalPlayerPos);
	static void Draw(const CClientPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const Vector3& LocalPlayerPos);
	static void DrawForOptic(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const Vector3& LocalPlayerPos);
	static void DrawForOptic(const CClientPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const Vector3& LocalPlayerPos);
	static void DrawBox(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImColor color, float thickness, int style, bool filled, ImColor fillColor);
	static void DrawSkeleton(const ImVec2& WindowPos, ImDrawList* DrawList, const std::array<ProjectedBoneInfo,SKELETON_NUMBONES>& ProjectedBones);

private:
	static inline std::array<ProjectedBoneInfo, SKELETON_NUMBONES> m_ProjectedBoneCache{};

private:
	static void DrawObservedPlayer(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones, const Vector3& LocalPlayerPos, bool bForOptic = false);
	static void DrawClientPlayer(const CClientPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones, const Vector3& LocalPlayerPos, bool bForOptic = false);
};