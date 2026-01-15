#include "pch.h"
#include "Players.h"
#include "Game/Camera List/Camera List.h"
#include "Game/Enums/EBoneIndex.h"
#include "GUI/Color Picker/Color Picker.h"
#include "Game/EFT.h"
#include "../../ESP/ESPSettings.h"
#include <imgui_internal.h>
#include <mutex>


// Forward declaration
static void DrawTextAtPosition(ImDrawList* DrawList, const ImVec2& Position, const ImColor& Color, const std::string& Text);

// String constants
static const std::string InjuredString = "(Injured)";
static const std::string BadlyInjuredString = "(Badly Injured)";
static const std::string DyingString = "(Dying)";


void DrawESPPlayers::DrawBox(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImColor color, float thickness, int style, bool filled, ImColor fillColor) {
	if (filled) {
		drawList->AddRectFilled(min, max, fillColor);
	}

	if (style == 0) { // Full Box
		drawList->AddRect(min, max, color, 0.0f, 0, thickness);
	} else { // Corners
		float sizeX = (max.x - min.x) / 4.0f;
		float sizeY = (max.y - min.y) / 7.0f;

		// Top Left
		drawList->AddLine(min, ImVec2(min.x + sizeX, min.y), color, thickness);
		drawList->AddLine(min, ImVec2(min.x, min.y + sizeY), color, thickness);
		// Top Right
		drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x - sizeX, min.y), color, thickness);
		drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + sizeY), color, thickness);
		// Bottom Left
		drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + sizeX, max.y), color, thickness);
		drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x, max.y - sizeY), color, thickness);
		// Bottom Right
		drawList->AddLine(max, ImVec2(max.x - sizeX, max.y), color, thickness);
		drawList->AddLine(max, ImVec2(max.x, max.y - sizeY), color, thickness);
	}
}

// Helper function to get health percentage and color from ETagStatus
static std::pair<float, ImColor> GetHealthInfoFromTagStatus(uint32_t tagStatus)
{
	// Map discrete health states to approximate percentages
	if (tagStatus & static_cast<uint32_t>(ETagStatus::Dying))
		return { 0.15f, ImColor(255, 50, 50) };      // Red - critical
	if (tagStatus & static_cast<uint32_t>(ETagStatus::BadlyInjured))
		return { 0.40f, ImColor(255, 150, 50) };     // Orange - badly hurt
	if (tagStatus & static_cast<uint32_t>(ETagStatus::Injured))
		return { 0.70f, ImColor(200, 200, 50) };     // Yellow - hurt
	// Default: Healthy / unknown
	return { 1.0f, ImColor(50, 200, 50) };           // Green - full health
}

void DrawESPPlayers::DrawHealthBar(ImDrawList* DrawList, const ImVec2& bMin, const ImVec2& bMax, float healthPercent, const ImColor& healthColor)
{
	// Bar dimensions
	float barWidth = 4.0f;
	float barHeight = bMax.y - bMin.y;
	float barX = bMax.x + 4.0f;  // Position to the right of the bounding box
	
	// Background (dark gray)
	DrawList->AddRectFilled(
		ImVec2(barX, bMin.y),
		ImVec2(barX + barWidth, bMax.y),
		ImColor(40, 40, 40, 200)
	);
	
	// Health fill (from bottom up)
	float fillHeight = barHeight * healthPercent;
	DrawList->AddRectFilled(
		ImVec2(barX, bMax.y - fillHeight),
		ImVec2(barX + barWidth, bMax.y),
		healthColor
	);
	
	// Optional: border
	DrawList->AddRect(
		ImVec2(barX, bMin.y),
		ImVec2(barX + barWidth, bMax.y),
		ImColor(80, 80, 80, 255),
		0.0f, 0, 1.0f
	);
}

void DrawESPPlayers::DrawObservedPlayer(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones, bool bForOptic)
{
	if (Player.IsInvalid())	return;
	if (Player.m_pSkeleton == nullptr) return;

	ProjectedBones.fill({});
	for (int i = 0; i < SKELETON_NUMBONES; i++)
		ProjectedBones[i].bIsOnScreen = bForOptic ? CameraList::OpticW2S(Player.m_pSkeleton->m_BonePositions[i], ProjectedBones[i].ScreenPos) : CameraList::W2S(Player.m_pSkeleton->m_BonePositions[i], ProjectedBones[i].ScreenPos);

	if (bForOptic
		&& ProjectedBones[Sketon_MyIndicies[EBoneIndex::Root]].ScreenPos.DistanceTo(CameraList::GetOpticCenter()) > CameraList::GetOpticRadius()
		&& ProjectedBones[Sketon_MyIndicies[EBoneIndex::Head]].ScreenPos.DistanceTo(CameraList::GetOpticCenter()) > CameraList::GetOpticRadius())
		return;

	using namespace ESPSettings::Enemy;
	uint8_t LineNumber = 0;

	// Calculate Bounding Box
	ImVec2 bMin(10000, 10000), bMax(-10000, -10000);
	bool anyOnScreen = false;
	for (const auto& bone : ProjectedBones) {
		if (bone.bIsOnScreen) {
			bMin.x = ImMin(bMin.x, bone.ScreenPos.x);
			bMin.y = ImMin(bMin.y, bone.ScreenPos.y);
			bMax.x = ImMax(bMax.x, bone.ScreenPos.x);
			bMax.y = ImMax(bMax.y, bone.ScreenPos.y);
			anyOnScreen = true;
		}
	}

	if (!anyOnScreen) return;

	// Add WindowPos
	bMin.x += WindowPos.x; bMin.y += WindowPos.y;
	bMax.x += WindowPos.x; bMax.y += WindowPos.y;

	if (bBoxEnabled) {
		DrawBox(DrawList, bMin, bMax, boxColor, boxThickness, boxStyle, bBoxFilled, boxFillColor);
	}

	if (bNameEnabled) {
		DrawGenericPlayerText(Player, WindowPos, DrawList, nameColor, LineNumber, ProjectedBones);
	}
	if (bWeaponEnabled) {
		DrawPlayerWeapon(Player.m_pHands.get(), WindowPos, DrawList, LineNumber, ProjectedBones);
	}
	if (bHealthEnabled) {
		auto [healthPercent, healthColor] = GetHealthInfoFromTagStatus(Player.m_TagStatus);
		DrawHealthBar(DrawList, bMin, bMax, healthPercent, healthColor);
	}
	if (bDistanceEnabled) {
		std::string distText = std::format("[{:.0f}m]", Player.GetBonePosition(EBoneIndex::Root).DistanceTo(m_LatestLocalPlayerPos));
		DrawTextAtPosition(DrawList, ImVec2(WindowPos.x + ProjectedBones[Sketon_MyIndicies[EBoneIndex::Root]].ScreenPos.x, bMax.y + 5.0f), distanceColor, distText);
	}

	if (bHeadDotEnabled) {
		auto& ProjectedHeadPos = ProjectedBones[Sketon_MyIndicies[EBoneIndex::Head]];
		if (ProjectedHeadPos.bIsOnScreen)
			DrawList->AddCircleFilled(ImVec2(WindowPos.x + ProjectedHeadPos.ScreenPos.x, WindowPos.y + ProjectedHeadPos.ScreenPos.y), headDotRadius, headDotColor);
	}

	if (bSkeletonEnabled)
		DrawSkeleton(WindowPos, DrawList, ProjectedBones);
}

void DrawESPPlayers::DrawClientPlayer(const CClientPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones, bool bForOptic)
{
	if (Player.IsInvalid())	return;
	if (Player.IsLocalPlayer())	return;
	if (Player.m_pSkeleton == nullptr) return;

	ProjectedBones.fill({});
	for (int i = 0; i < SKELETON_NUMBONES; i++)
		ProjectedBones[i].bIsOnScreen = (bForOptic) ? CameraList::OpticW2S(Player.m_pSkeleton->m_BonePositions[i], ProjectedBones[i].ScreenPos) : CameraList::W2S(Player.m_pSkeleton->m_BonePositions[i], ProjectedBones[i].ScreenPos);

	if (bForOptic
		&& ProjectedBones[Sketon_MyIndicies[EBoneIndex::Root]].ScreenPos.DistanceTo(CameraList::GetOpticCenter()) > CameraList::GetOpticRadius()
		&& ProjectedBones[Sketon_MyIndicies[EBoneIndex::Head]].ScreenPos.DistanceTo(CameraList::GetOpticCenter()) > CameraList::GetOpticRadius())
		return;

	using namespace ESPSettings::Enemy;
	uint8_t LineNumber = 0;

	// Calculate Bounding Box
	ImVec2 bMin(10000, 10000), bMax(-10000, -10000);
	bool anyOnScreen = false;
	for (const auto& bone : ProjectedBones) {
		if (bone.bIsOnScreen) {
			bMin.x = ImMin(bMin.x, bone.ScreenPos.x);
			bMin.y = ImMin(bMin.y, bone.ScreenPos.y);
			bMax.x = ImMax(bMax.x, bone.ScreenPos.x);
			bMax.y = ImMax(bMax.y, bone.ScreenPos.y);
			anyOnScreen = true;
		}
	}
	if (!anyOnScreen) return;

	bMin.x += WindowPos.x; bMin.y += WindowPos.y;
	bMax.x += WindowPos.x; bMax.y += WindowPos.y;

	if (bBoxEnabled) {
		DrawBox(DrawList, bMin, bMax, boxColor, boxThickness, boxStyle, bBoxFilled, boxFillColor);
	}

	if (bNameEnabled) {
		DrawGenericPlayerText(Player, WindowPos, DrawList, nameColor, LineNumber, ProjectedBones);
	}
	if (bWeaponEnabled) {
		DrawPlayerWeapon(Player.m_pHands.get(), WindowPos, DrawList, LineNumber, ProjectedBones);
	}

	if (bHealthEnabled) {
		auto [healthPercent, healthColor] = GetHealthInfoFromTagStatus(0);
		DrawHealthBar(DrawList, bMin, bMax, healthPercent, healthColor);
	}

	if (bDistanceEnabled) {
		std::string distText = std::format("[{:.0f}m]", Player.GetBonePosition(EBoneIndex::Root).DistanceTo(m_LatestLocalPlayerPos));
		DrawTextAtPosition(DrawList, ImVec2(WindowPos.x + ProjectedBones[Sketon_MyIndicies[EBoneIndex::Root]].ScreenPos.x, bMax.y + 5.0f), distanceColor, distText);
	}

	if (bHeadDotEnabled) {
		auto& ProjectedHeadPos = ProjectedBones[Sketon_MyIndicies[EBoneIndex::Head]];
		if (ProjectedHeadPos.bIsOnScreen)
			DrawList->AddCircleFilled(ImVec2(WindowPos.x + ProjectedHeadPos.ScreenPos.x, WindowPos.y + ProjectedHeadPos.ScreenPos.y), headDotRadius, headDotColor);
	}

	if (bSkeletonEnabled)
		DrawSkeleton(WindowPos, DrawList, ProjectedBones);
}

void DrawESPPlayers::DrawAll(const ImVec2& WindowPos, ImDrawList* DrawList)
{
	auto& PlayerList = EFT::GetRegisteredPlayers();

	std::scoped_lock PlayerLock(PlayerList.m_Mut);

	auto LocalPlayer = PlayerList.GetLocalPlayer();
	if (LocalPlayer == nullptr || LocalPlayer->IsInvalid()) return;

	m_LatestLocalPlayerPos = LocalPlayer->GetBonePosition(EBoneIndex::Root);

	auto bDrawOpticESP = DrawESPPlayers::bOpticESP && CameraList::IsScoped();
	auto WindowSize = ImGui::GetWindowSize();
	auto OpticFOV = CameraList::GetOpticRadius();

	if (PlayerList.m_Players.empty()) return;

	for (auto& Player : PlayerList.m_Players)
		std::visit([WindowPos, DrawList](auto& Player) { DrawESPPlayers::Draw(Player, WindowPos, DrawList); }, Player);

	if (bDrawOpticESP)
	{
		ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(WindowPos.x + (WindowSize.x * 0.5f), WindowPos.y + (WindowSize.y * 0.5f)), OpticFOV, ImColor(0, 0, 0), 33);
		ImGui::GetWindowDrawList()->AddCircle(ImVec2(WindowPos.x + (WindowSize.x * 0.5f), WindowPos.y + (WindowSize.y * 0.5f)), OpticFOV + 2.0f, ImColor(255, 255, 255), 33, 2.f);

		for (auto& Player : PlayerList.m_Players)
			std::visit([WindowPos, DrawList](auto& Player) { DrawESPPlayers::DrawForOptic(Player, WindowPos, DrawList); }, Player);
	}
}

void DrawTextAtPosition(ImDrawList* DrawList, const ImVec2& Position, const ImColor& Color, const std::string& Text)

{
	auto TextSize = ImGui::CalcTextSize(Text.c_str());
	DrawList->AddText(
		ImVec2(Position.x - (TextSize.x / 2.0f), Position.y),
		Color,
		Text.c_str()
	);
}

void DrawESPPlayers::DrawGenericPlayerText(const CBaseEFTPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const ImColor& Color, uint8_t& LineNumber, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones)
{
	std::string Text = Player.GetBaseName();
	auto& ProjectedHead = ProjectedBones[Sketon_MyIndicies[EBoneIndex::Head]];
	DrawTextAtPosition(DrawList, ImVec2(WindowPos.x + ProjectedHead.ScreenPos.x, WindowPos.y + ProjectedHead.ScreenPos.y - 15.0f - (ImGui::GetTextLineHeight() * LineNumber)), Color, Text);
	LineNumber++;
}

void DrawESPPlayers::DrawObservedPlayerHealthText(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList, const ImColor& Color, uint8_t& LineNumber, std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones)
{
	const char* DataPtr = nullptr;
	if (Player.IsInCondition(ETagStatus::Injured))
		DataPtr = InjuredString.data();
	else if (Player.IsInCondition(ETagStatus::BadlyInjured))
		DataPtr = BadlyInjuredString.data();
	else if (Player.IsInCondition(ETagStatus::Dying))
		DataPtr = DyingString.data();

	if (DataPtr == nullptr) return;

	auto& ProjectedRootPos = ProjectedBones[Sketon_MyIndicies[EBoneIndex::Root]];
	DrawTextAtPosition(DrawList, ImVec2(WindowPos.x + ProjectedRootPos.ScreenPos.x, WindowPos.y + ProjectedRootPos.ScreenPos.y + (ImGui::GetTextLineHeight() * LineNumber)), Color, DataPtr);
	LineNumber++;
}

void DrawESPPlayers::DrawPlayerWeapon(const CHeldItem* pHands, const ImVec2& WindowPos, ImDrawList* DrawList, uint8_t& LineNumber, const std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones)
{
	if (!pHands || pHands->IsInvalid()) return;

	auto ItemName = pHands->m_pHeldItem->GetItemName();
	auto& ProjectedHead = ProjectedBones[Sketon_MyIndicies[EBoneIndex::Head]];
	
	DrawTextAtPosition(DrawList, ImVec2(WindowPos.x + ProjectedHead.ScreenPos.x, WindowPos.y + ProjectedHead.ScreenPos.y - 15.0f - (ImGui::GetTextLineHeight() * LineNumber)), ESPSettings::Enemy::weaponColor, ItemName);
	LineNumber++;

	auto& Magazine = pHands->m_pMagazine;
	if (Magazine == nullptr) return;

	std::string MagText = std::format("{0:d} {1:s}", Magazine->m_CurrentCartridges, Magazine->GetAmmoName().c_str());
	DrawTextAtPosition(DrawList, ImVec2(WindowPos.x + ProjectedHead.ScreenPos.x, WindowPos.y + ProjectedHead.ScreenPos.y - 15.0f - (ImGui::GetTextLineHeight() * LineNumber)), ESPSettings::Enemy::weaponColor, MagText);
	LineNumber++;
}

void DrawESPPlayers::Draw(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList)
{
	DrawObservedPlayer(Player, WindowPos, DrawList, m_ProjectedBoneCache, false);
}

void DrawESPPlayers::Draw(const CClientPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList)
{
	DrawClientPlayer(Player, WindowPos, DrawList, m_ProjectedBoneCache, false);
}

void DrawESPPlayers::DrawForOptic(const CObservedPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList)
{
	DrawObservedPlayer(Player, WindowPos, DrawList, m_ProjectedBoneCache, true);
}

void DrawESPPlayers::DrawForOptic(const CClientPlayer& Player, const ImVec2& WindowPos, ImDrawList* DrawList)
{
	DrawClientPlayer(Player, WindowPos, DrawList, m_ProjectedBoneCache, true);
}

void ConnectBones(const ProjectedBoneInfo& BoneA, const ProjectedBoneInfo& BoneB, const ImVec2& WindowPos, ImDrawList* DrawList, const ImColor& Color, float Thickness)
{
	if (BoneA.bIsOnScreen == false || BoneB.bIsOnScreen == false)
		return;

	DrawList->AddLine(
		{ WindowPos.x + BoneA.ScreenPos.x, WindowPos.y + BoneA.ScreenPos.y },
		{ WindowPos.x + BoneB.ScreenPos.x, WindowPos.y + BoneB.ScreenPos.y },
		Color,
		Thickness
	);
}

void DrawESPPlayers::DrawSkeleton(const ImVec2& WindowPos, ImDrawList* DrawList, const std::array<ProjectedBoneInfo, SKELETON_NUMBONES>& ProjectedBones)
{
	using namespace ESPSettings::Enemy;
	ImColor col = skeletonColor;
	float thick = skeletonThickness;

	if (bBonesHead) ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::Head]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::Neck]], WindowPos, DrawList, col, thick);
	
	if (bBonesSpine) {
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::Neck]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::Spine3]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::Spine3]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::Pelvis]], WindowPos, DrawList, col, thick);
	}

	if (bBonesLegsL) {
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::Pelvis]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LThigh1]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::LThigh1]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LThigh2]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::LThigh2]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LCalf]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::LCalf]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LFoot]], WindowPos, DrawList, col, thick);
	}

	if (bBonesLegsR) {
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::Pelvis]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RThigh1]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::RThigh1]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RThigh2]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::RThigh2]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RCalf]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::RCalf]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RFoot]], WindowPos, DrawList, col, thick);
	}

	if (bBonesArmsR) {
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::Spine3]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RUpperArm]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::RUpperArm]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RForeArm1]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::RForeArm1]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RForeArm2]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::RForeArm2]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::RPalm]], WindowPos, DrawList, col, thick);
	}

	if (bBonesArmsL) {
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::Spine3]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LUpperArm]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::LUpperArm]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LForeArm1]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::LForeArm1]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LForeArm2]], WindowPos, DrawList, col, thick);
		ConnectBones(ProjectedBones[Sketon_MyIndicies[EBoneIndex::LForeArm2]], ProjectedBones[Sketon_MyIndicies[EBoneIndex::LPalm]], WindowPos, DrawList, col, thick);
	}
}