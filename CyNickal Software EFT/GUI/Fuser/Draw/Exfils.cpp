#include "pch.h"
#include "Exfils.h"
#include "Game/Camera List/Camera List.h"
#include "GUI/Color Picker/Color Picker.h"
#include "GUI/ESP/ESPSettings.h"
#include "Game/EFT.h"
#include <mutex>

void DrawExfils::DrawAll(const ImVec2& WindowPos, ImDrawList* DrawList)

{
	if (!bMasterToggle) return;

	// Must be in raid with valid GameWorld and ExfilController
	auto gameWorld = EFT::GetGameWorld();
	if (!EFT::IsInRaid() || !gameWorld) return;
	if (!gameWorld->m_pExfilController || !gameWorld->m_pRegisteredPlayers) return;

	auto& ExfilController = *gameWorld->m_pExfilController;
	auto& PlayerList = *gameWorld->m_pRegisteredPlayers;

	std::scoped_lock Lock(ExfilController.m_ExfilMutex, PlayerList.m_Mut);

	Vector2 ScreenPos{};

	// Early return if local player invalid - distance from origin is meaningless
	auto LocalPlayer = PlayerList.GetLocalPlayer();
	if (!LocalPlayer || LocalPlayer->IsInvalid())
		return;
	Vector3 LocalPlayerPos = LocalPlayer->GetBonePosition(EBoneIndex::Root);

	for (auto& Exfil : ExfilController.m_Exfils)
	{
		if (!CameraList::W2S(Exfil.m_Position, ScreenPos)) continue;

		float Distance = LocalPlayerPos.DistanceTo(Exfil.m_Position);

		// Check distance against exfil range limit
		if (Distance > ESPSettings::RenderRange::fExfilRange) continue;

		// Use friendly display name instead of raw internal name
		std::string Text = std::format("{0:s} [{1:.0f}m]", Exfil.GetDisplayName(), Distance);
		auto TextSize = ImGui::CalcTextSize(Text.c_str());
		DrawList->AddText(
			ImVec2(WindowPos.x + ScreenPos.x - (TextSize.x / 2.0f), WindowPos.y + ScreenPos.y),
			Exfil.GetFuserColor(),
			Text.c_str()
		);
	}
}
