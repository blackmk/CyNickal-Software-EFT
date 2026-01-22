#include "pch.h"
#include "GUI/Fuser/Draw/Loot.h"
#include "Game/Camera List/Camera List.h"
#include "GUI/Color Picker/Color Picker.h"
#include "GUI/ESP/ESPSettings.h"
#include "GUI/LootFilter/LootFilter.h"
#include "Game/EFT.h"
#include <mutex>

// Helper function to get item range based on value tier
static float GetItemRangeByValue(int32_t price)
{
	using namespace ESPSettings::RenderRange;

	// Use LootFilter thresholds for consistency
	if (price >= LootFilter::MinValueValuable)         return fItemHighRange;    // >100k (default)
	if (price >= LootFilter::MinValueValuable / 2)     return fItemMediumRange;  // >50k
	if (price >= LootFilter::MinValueRegular)          return fItemLowRange;     // >20k
	return fItemRestRange;                                                       // <20k
}

void DrawESPLoot::DrawAll(const ImVec2& WindowPos, ImDrawList* DrawList)

{
	if (!bMasterToggle) return;

	// Must be in raid with valid GameWorld and LootList
	auto gameWorld = EFT::GetGameWorld();
	if (!EFT::IsInRaid() || !gameWorld) return;
	if (!gameWorld->m_pLootList || !gameWorld->m_pRegisteredPlayers) return;
	if (!gameWorld->m_pLootList->IsInitialized()) return;

	auto& PlayerList = *gameWorld->m_pRegisteredPlayers;

	std::scoped_lock PlayerLock(PlayerList.m_Mut);

	Vector3 LocalPlayerPos{};
	auto LocalPlayer = PlayerList.GetLocalPlayer();
	if (LocalPlayer && !LocalPlayer->IsInvalid())
		LocalPlayerPos = LocalPlayer->GetBonePosition(EBoneIndex::Root);

	DrawAllContainers(WindowPos, DrawList, LocalPlayerPos);
	DrawAllItems(WindowPos, DrawList, LocalPlayerPos);
}


void DrawESPLoot::DrawSettings()
{
	ImGui::Checkbox("Containers", &bContainerToggle);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputFloat("##LootContainerMaxDistance", &fMaxContainerDistance);

	ImGui::Checkbox("Items", &bItemToggle);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputFloat("##LootItemMaxDistance", &fMaxItemDistance);
	ImGui::InputScalar("Min Item Price", ImGuiDataType_S32, &m_MinItemPrice);
}

void DrawESPLoot::DrawAllItems(const ImVec2& WindowPos, ImDrawList* DrawList, const Vector3& LocalPlayerPos)
{
	if (!bItemToggle) return;

	auto gameWorld = EFT::GetGameWorld();
	if (!gameWorld || !gameWorld->m_pLootList) return;

	auto& LootList = *gameWorld->m_pLootList;
	auto& ObservedItems = LootList.m_ObservedItems;

	std::scoped_lock LootLock(ObservedItems.m_Mut);

	for (auto& Loot : ObservedItems.m_Entities)
		DrawItem(Loot, DrawList, WindowPos, LocalPlayerPos);
}

void DrawESPLoot::DrawAllContainers(const ImVec2& WindowPos, ImDrawList* DrawList, const Vector3& LocalPlayerPos)
{
	if (!bContainerToggle) return;

	auto gameWorld = EFT::GetGameWorld();
	if (!gameWorld || !gameWorld->m_pLootList) return;

	auto& LootList = *gameWorld->m_pLootList;
	auto& LootableContainers = LootList.m_LootableContainers;

	std::scoped_lock ContainerLock(LootableContainers.m_Mut);

	for (auto& Container : LootableContainers.m_Entities)
		DrawContainer(Container, DrawList, WindowPos, LocalPlayerPos);
}


void DrawESPLoot::DrawItem(CObservedLootItem& Item, ImDrawList* DrawList, ImVec2 WindowPos, const Vector3& LocalPlayerPos)
{
	if (Item.IsInvalid()) return;

	// Use GetEffectivePrice() for consistency with LootFilter color system (handles price-per-slot)
	int32_t itemPrice = LootFilter::GetEffectivePrice(Item);

	if (m_MinItemPrice > 0 && itemPrice < m_MinItemPrice)
		return;

	Vector2 ScreenPos{};
	if (!CameraList::W2S(Item.m_Position, ScreenPos))	return;

	auto Distance = LocalPlayerPos.DistanceTo(Item.m_Position);

	// Use tier-based distance filtering based on item value
	float maxRange = GetItemRangeByValue(itemPrice);
	if (Distance > maxRange)
		return;

	std::string DisplayString = std::format("{0:s} ({1:d}) [{2:.0f}m]", Item.GetName().c_str(), itemPrice, Distance);

	auto TextSize = ImGui::CalcTextSize(DisplayString.c_str());

	// Use LootFilter's tier-based color system instead of static color
	ImU32 itemColor = LootFilter::GetItemColor(Item);

	DrawList->AddText(
		ImVec2(WindowPos.x + ScreenPos.x - (TextSize.x / 2.0f), WindowPos.y + ScreenPos.y - 10.0f - TextSize.y),
		itemColor,
		DisplayString.c_str()
	);
}

void DrawESPLoot::DrawContainer(CLootableContainer& Container, ImDrawList* DrawList, ImVec2 WindowPos, const Vector3& LocalPlayerPos)
{
	if (Container.IsInvalid()) return;

	Vector2 ScreenPos{};
	if (!CameraList::W2S(Container.m_Position, ScreenPos))	return;

	auto Distance = LocalPlayerPos.DistanceTo(Container.m_Position);

	// Use configurable container range from ESPSettings
	if (Distance > ESPSettings::RenderRange::fContainerRange)
		return;

	std::string DisplayString = std::format("{0:s} [{1:.0f}m]", Container.GetName().c_str(), Distance);

	auto TextSize = ImGui::CalcTextSize(DisplayString.c_str());

	DrawList->AddText(
		ImVec2(WindowPos.x + ScreenPos.x - (TextSize.x / 2.0f), WindowPos.y + ScreenPos.y - 10.0f - TextSize.y),
		Container.GetFuserColor(),
		DisplayString.c_str()
	);
}
