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
	if (!EFT::IsInRaid() || !EFT::pGameWorld) return;
	if (!EFT::pGameWorld->m_pExfilController) return;

	auto& ExfilController = EFT::GetExfilController();
	auto& PlayerList = EFT::GetRegisteredPlayers();

	std::scoped_lock Lock(ExfilController.m_ExfilMutex, PlayerList.m_Mut);

	Vector2 ScreenPos{};

	Vector3 LocalPlayerPos{};
	auto LocalPlayer = PlayerList.GetLocalPlayer();
	if (LocalPlayer && !LocalPlayer->IsInvalid())
		LocalPlayerPos = LocalPlayer->GetBonePosition(EBoneIndex::Root);

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
