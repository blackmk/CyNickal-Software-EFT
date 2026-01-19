# Project Patch Notes

This document tracks changes made to the codebase by AI agents and developers. Please append new entries at the top of the history section (reverse chronological order). After every successful build, add a concise patch note entry (a brief “I changed X” summary is sufficient).

## History

<!-- Add new entries below this line -->

## [Date: 2026-01-19] - Phase 3: Quest System (Lum0s-Inspired)

**Agent:** Claude Code
**Focus:** C++

### Changes
- **Modified:** `Game/Offsets/Offsets.h` - Added quest-related offsets:
  - `CProfile::pQuestsData` (0x98)
  - `CQuestsData::pId`, `Status`, `pCompletedConditions`
- **Created:** `Game/Classes/Quest/QuestObjectiveType.h` - Quest objective type enum with:
  - 18 objective types (Visit, Mark, FindItem, GiveItem, Plant, etc.)
  - EQuestStatus enum for quest state tracking
  - String conversion helpers
- **Created:** `Game/Classes/Quest/QuestEntry.h` - Active quest entry struct with:
  - Quest ID and display name
  - User toggle for enabling/disabling
- **Created:** `Game/Classes/Quest/QuestLocation.h` - Quest marker struct with:
  - Quest/objective/zone IDs
  - Objective type and world position
  - Unique key generation
- **Created:** `Game/Classes/Quest/QuestData.h/cpp` - Quest database system with:
  - TaskDefinition, TaskObjective, TaskZone structs
  - QuestDatabase singleton for loading tarkov.dev JSON format
  - Zone index by map for location lookups
  - Quest item ID tracking
- **Created:** `Game/Classes/Quest/CQuestManager.h/cpp` - Main quest manager with:
  - QuestSettings (enable flags, colors, marker size)
  - Memory reading for active quests from player profile
  - Quest item tracking for loot highlighting
  - Quest location markers by current map
  - Refresh throttling (1 second)
- **Modified:** `GUI/LootFilter/LootFilter.cpp` - Integrated IsQuestItem():
  - Checks CQuestManager for dynamic quest items
  - Falls back to ItemDatabase for static quest item flag
- **Modified:** `GUI/Radar/Radar2D.h` - Added:
  - `bShowQuestMarkers` toggle
  - `DrawQuestMarkers()` declaration
  - QuestLocation include
- **Modified:** `GUI/Radar/Radar2D.cpp` - Added:
  - `DrawQuestMarkers()` implementation with star markers
  - Quest name/distance labels with background
  - Height indicators (above/below arrows)
  - Integration in both Render() and RenderEmbedded()
  - Checkbox in RenderSettings()
- **Modified:** `GUI/Main Menu/Main Menu.cpp` - Added Quest Helper panel with:
  - Master toggle for quest helper
  - Show locations/highlight items toggles
  - Quest marker size slider
  - Color picker for quest markers
  - Active quest list with individual toggles
- **Modified:** `GUI/Config/Config.cpp` - Added serialization for:
  - All quest settings (enabled, colors, sizes)
  - Blacklisted quests (user-disabled)
  - bShowQuestMarkers for Radar2D
- **Modified:** `CyNickal Software EFT.vcxproj` - Added quest files to project

### Features
- **Quest Memory Reading:** Reads active quests from player profile, tracks status
- **Quest Item Highlighting:** Purple markers for quest items in loot
- **Quest Location Markers:** Star-shaped markers on radar for objectives
- **Quest Panel UI:** Toggle quests on/off, configure colors and sizes
- **Config Persistence:** All quest settings saved and loaded

### Verification
- [x] Code implementation complete
- [x] Files added to vcxproj
- [ ] Build succeeded (requires user to run build.bat)
- [ ] Verified in-game

---

## [Date: 2026-01-18] - Phase 2: Radar Display + Widgets (Lum0s-Inspired)

**Agent:** Claude Code
**Focus:** C++

### Changes
- **Created:** `GUI/Radar/PlayerFocus.h` - Player focus and temporary teammate tracking system with:
  - Focus state cycling (None -> Focused -> TempTeammate -> None)
  - Temp teammate set for marking friendlies
  - Configurable highlight and group line colors
- **Created:** `GUI/Radar/Widgets/RadarWidget.h` - Base class for draggable/resizable overlay widgets with:
  - Position, size, visibility, and minimized state
  - Common frame rendering with title bar
  - JSON serialization/deserialization
- **Created:** `GUI/Radar/Widgets/PlayerInfoWidget.h/cpp` - Player list widget showing:
  - Player name, weapon, health status, distance
  - Color-coded by player type (PMC, Scav, Boss, etc.)
  - Click to focus/cycle focus state
  - Sortable by distance
- **Created:** `GUI/Radar/Widgets/LootInfoWidget.h/cpp` - Top loot widget showing:
  - Valuable items sorted by price or distance
  - Item name, price, distance columns
  - Click to highlight on radar (pulsing effect)
  - Configurable max display count
- **Created:** `GUI/Radar/Widgets/AimviewWidget.h/cpp` - 3D perspective widget showing:
  - Players relative to local player's view direction
  - Distance-based dot sizing (closer = bigger)
  - Crosshair and distance circles
  - FOV-based projection
- **Created:** `GUI/Radar/Widgets/WidgetManager.h` - Central widget manager with:
  - Widget visibility toggles
  - Settings UI for all widgets
  - Serialization/deserialization for widget states
- **Modified:** `GUI/Radar/Radar2D.h` - Added Phase 2 settings:
  - bShowGroupLines, bShowFocusHighlight, bShowHoverTooltip
  - fMaxPlayerDistance, fMaxLootDistance
  - HoveredEntityAddr for tooltip tracking
  - RenderOverlayWidgets() declaration
- **Modified:** `GUI/Radar/Radar2D.cpp` - Added:
  - RenderOverlayWidgets() implementation
  - WidgetManager and PlayerFocus settings in RenderSettings()
  - Fixed duplicate pch.h include
- **Modified:** `GUI/Config/Config.cpp` - Added serialization/deserialization for:
  - All Phase 2 radar settings
  - Widget states and positions
  - PlayerFocus colors
- **Modified:** `CyNickal Software EFT.vcxproj` - Added new source and header files to project

### Features
- **Player Focus System:** Click players to focus/track them, cycle through focus states
- **Temporary Teammates:** Mark players as friendly (hidden from aimview, different rendering)
- **PlayerInfoWidget:** Sortable player list with weapon/health/distance info
- **LootInfoWidget:** Top valuable loot list with click-to-highlight
- **AimviewWidget:** 3D perspective radar showing what's in front of you
- **Widget Framework:** Draggable, resizable, minimizable overlay widgets
- **Config Persistence:** All widget positions and settings saved

### Verification
- [x] Build succeeded (0 errors, warnings only)
- [x] Patch note recorded
- [ ] Verified in-game

---

## [Date: 2026-01-18] - Phase 1: Enhanced Loot Filtering System (Lum0s-Inspired)

**Agent:** Claude Code
**Focus:** C++

### Changes
- **Created:** `GUI/LootFilter/LootFilterEntry.h` - Per-item filter entry struct with item ID, name, type (important/blacklisted), custom color, and enabled toggle
- **Created:** `GUI/LootFilter/UserLootFilter.h` - Named filter preset collection with helper methods for managing important and blacklisted items
- **Modified:** `GUI/LootFilter/LootFilter.h` - Complete overhaul with:
  - Dual-tier pricing system (`MinValueRegular`, `MinValueValuable`)
  - Price-per-slot calculation option
  - Named filter presets management
  - Quest item integration placeholders (Phase 3)
  - Enhanced filtering checks (IsImportant, IsBlacklisted, IsValuable, IsQuestItem)
- **Modified:** `GUI/LootFilter/LootFilter.cpp` - Implemented:
  - Filter preset management (create/delete/rename)
  - Important and blacklist item management
  - Enhanced color system with priority order (Quest > Important > Valuable > Price tiers)
  - Item search UI for adding items by name
  - Full UI with preset selector, important items table, blacklist table
- **Modified:** `GUI/Config/Config.cpp` - Added full serialization/deserialization for:
  - All loot filter settings (categories, price thresholds, toggles)
  - Filter presets with entries (item lists persist across sessions)
- **Modified:** `GUI/Main Menu/Main Menu.cpp` - Integrated loot filter UI into Radar Config panel

### Features
- **Dual-tier pricing:** Regular threshold (default 20k) and Valuable threshold (default 100k) for better loot highlighting
- **Price-per-slot:** Optional normalization by inventory size for comparing item value efficiency
- **Named filter presets:** Create, rename, delete custom filter presets (default preset protected)
- **Important items list:** Mark specific items to always highlight with custom colors
- **Blacklist:** Mark items to hide completely from radar/ESP
- **Item search:** Search database to add items to important/blacklist by name
- **Quest item support:** Infrastructure ready for Phase 3 quest system integration
- **Config persistence:** All settings and filter presets saved to config files

### Color Priority System
1. Quest Items -> Purple (when enabled)
2. Important (user-marked) -> Turquoise or custom color
3. Blacklisted -> Hidden (not rendered)
4. Valuable (>MinValueValuable) -> Gold
5. High Value -> Purple
6. Medium Value -> Blue
7. Low Value -> Green
8. Normal -> White
9. Below Threshold -> Gray

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded
- [ ] Verified in-game

---

## [Date: 2026-01-18] - Fixed Random Bone State Desync

**Agent:** Claude Code
**Focus:** C++

### Changes
- **Fixed:** Random bone feature breaking after some shots and stopping to lock correctly
- **Root Cause:** Three bugs in `GUI/Aimbot/Aimbot.cpp` caused state desync:
  1. **Dual initialization paths** - `GetTargetBoneIndex()` had redundant initialization that didn't update `s_LastRandomAmmoCount`, causing state desync when `pFirearm` was temporarily invalid
  2. **Partial state reset** - When player/firearm was invalid, only `bRandomBoneInitialized` was reset, leaving stale `s_LastRandomHandsController` and `s_LastRandomAmmoCount` values
  3. **Unreliable ammo count** - Zero or garbage ammo reads could trigger spurious shot detection (e.g., 25 bone re-rolls in one frame)
- **Fix 1:** Removed duplicate initialization from `GetTargetBoneIndex()` - now just returns `s_CurrentRandomBone` since `UpdateRandomBoneState()` handles all initialization
- **Fix 2:** Full state reset (all 3 variables) when player or firearm becomes invalid
- **Fix 3:** Added ammo validation (ignore 0 or >200) and max shots-per-frame limit (cap at 10, default to 1 if exceeded)

### Technical Details
The bug manifested when:
1. `pFirearm` became invalid momentarily (reload/animation)
2. `UpdateRandomBoneState()` set `bRandomBoneInitialized = false` and returned early
3. `GetTargetBoneIndex()` re-initialized the bone without updating `s_LastRandomAmmoCount`
4. Next frame, ammo tracking was out of sync → spurious shot detection → rapid bone re-rolls → broken aim

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded
- [ ] Verified in-game

---

## [Date: 2026-01-15] - Health Bar Label Warning

**Agent:** opencode
**Focus:** C++

### Changes
- **Modified:** ESP settings UI now labels the Health Bar toggle as "doesnt work" to set expectations

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Health Bar Visibility + Exfil Name Source Fix

**Agent:** opencode
**Focus:** C++

### Changes
- **Fixed:** Health bar now renders for client-player entities by drawing a default full-health bar when tag status isn’t available
- **Modified:** Health bar rendering uses tag-status flags directly for observed players, with discrete percentage mapping (Healthy/Injured/Badly Injured/Dying)
- **Fixed:** Exfil names now read from `ExitTriggerSettings.Name` instead of GameObject names
- **Added:** New offsets for `CExfiltrationPoint::pSettings` (0x98) and `CExitTriggerSettings::pName` (0x18)
- **Modified:** Exfil name translation now uses the internal map ID from `LocalGameWorld.LocationId` (0xC8) with a display-name fallback

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Health Bar ESP Visualization

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** Visual health bar in ESP (matches preview style) instead of text labels
  - Bar appears to the right of player bounding box
  - Color-coded based on health state:
    - Green (100%): Healthy
    - Yellow (70%): Injured
    - Orange (40%): Badly Injured
    - Red (15%): Dying
  - Dark gray background with subtle border
- **Added:** `DrawHealthBar()` function in `Players.cpp` for bar rendering
- **Added:** `GetHealthInfo()` helper function to map ETagStatus to percentage and color
- **Modified:** `DrawObservedPlayer()` now calls `DrawHealthBar()` instead of text function

### Technical Details
Since EFT's ETagStatus only provides discrete health states (not exact percentages), the bar maps these to approximate visual percentages for intuitive display.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Fixed Health Status Display for Observed Players

**Agent:** opencode
**Focus:** C++

### Changes
- **Fixed:** `ETagStatus` enum values in `CObservedPlayer.h` - Changed from sequential values (11, 12, 13, 14) to correct bitmask flags matching the game's ETagStatus (1024, 2048, 4096, 8192)
- **Fixed:** `IsInCondition()` function in `CObservedPlayer.cpp` - Changed from bitset bit position testing to proper bitwise AND operation
- **Added:** `GetHealthStatusString(uint32_t tagStatus)` helper function to convert raw tag status to human-readable strings ("Healthy", "Injured", "Badly Injured", "Dying")
- **Added:** `healthColor` setting in `ESPSettings.h` - Dedicated color for health status text in ESP (default: light red)
- **Modified:** `Player Table.cpp` - Now shows human-readable health status instead of raw hex value
- **Modified:** `Players.cpp` - ESP health text now uses the new `healthColor` setting instead of player's fuser color

### Technical Details
The C# reference project uses bitmask flags where each status is a power of 2 (Healthy=1024, Injured=2048, etc.). The C++ code was incorrectly using sequential values (11, 12, 13, 14), causing `IsInCondition()` to test wrong bit positions and health status to never display correctly.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Comprehensive Naming System for Entities and Exfils

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** `Game/Data/ExfilNameMap.h` - Dictionary with 130+ exfil name translations organized by map ID
  - Maps internal names (e.g., `"EXFIL_ZB013"`) to friendly display names (e.g., `"ZB-013"`)
  - Covers all maps: Woods, Shoreline, Reserve, Labs, Interchange, Factory, Customs, Lighthouse, Streets, Ground Zero
  - Case-insensitive lookup with custom hash/equal functors
- **Added:** `Game/Data/SpawnTypeNameMap.h` - Unified entity naming system replacing `BossNameMap.h`
  - All bosses: Reshala, Killa, Gluhar, Sanitar, Tagilla, Shturman, Zryachiy, Knight, Big Pipe, Birdeye, Kaban, Kollontay, Partisan
  - Raiders: Rogue, Raider, USEC Bot, BEAR Bot, Arena Fighter, Black Division, VSRF
  - Guards: Named with boss association (e.g., "Reshala Guard", "Gluhar Scout")
  - Cultists: Cultist, Priest, Predvestnik, Prizrak, Oni
  - Special: Santa, Zombie, Civilian, Sentry, BTR Gunner, Scav variants
  - `EntityCategory` enum and helper functions (`IsBoss`, `IsRaider`, `IsGuard`, `IsCultist`, etc.)
  - `GetFactionName(EPlayerSide)` for USEC/BEAR display
- **Modified:** `CExfilPoint.cpp` - Uses `ExfilNameMap` to translate exfil names in `Finalize()`
- **Modified:** `CExfilPoint.h` - Added `m_DisplayName`, `GetDisplayName()`, `GetInternalName()` methods
- **Modified:** `CBaseEFTPlayer.cpp` - Rewrote `GetBaseName()`, `GetBossName()`, `IsBoss()`, `IsRaider()`, `IsGuard()` to use unified map
- **Modified:** `Radar2D.cpp` - Exfil labels now use `GetDisplayName()` instead of internal `m_Name`
- **Modified:** `GUI/Fuser/Draw/Exfils.cpp` - ESP text uses `GetDisplayName()`
- **Modified:** `Player Table.cpp` - Shows faction name (USEC/BEAR/Scav) and spawn type display name instead of raw integers

### Technical Details
- Ported naming conventions from C# reference project (`EFT-DMA-Radar-Source`)
- Map ID translation handles display names ("Customs") vs internal names ("bigmap")
- Single source of truth for all entity categorization instead of scattered switch statements

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Fixed DMA Thread Crash on Raid Exit (Re-attach Now Works)

**Agent:** opencode
**Focus:** C++

### Changes
- **Fixed:** DMA thread no longer crashes when exiting a raid. The app now properly re-attaches when entering a new raid without requiring a restart.
- **Root Cause:** `CameraList::QuickUpdateNecessaryCameras()` called `EFT::GetRegisteredPlayers()` without checking if GameWorld was valid. When raid ends and `pGameWorld` becomes null, the function threw an unhandled exception that crashed the entire DMA thread's main loop.
- **Added:** Guard check `if (!EFT::IsGameWorldInitialized()) return;` at the start of `CameraList::QuickUpdateNecessaryCameras()`.
- **Added:** Guard check in `Keybinds::OnDMAFrame()` before calling `GetRegisteredPlayers().FullUpdate()`.
- **Added:** Try-catch wrappers around all high-frequency timer callbacks in `DMA Thread.cpp` as a safety net to prevent any future unhandled exceptions from crashing the loop.

### Technical Details
The crash sequence was:
1. `UpdateRaidState()` detects raid exit → resets `pGameWorld` to null
2. `Camera_UpdateViewMatrix` timer fires (every 2ms) → calls `CameraList::QuickUpdateNecessaryCameras()`
3. That calls `EFT::GetRegisteredPlayers()` which throws because `pGameWorld` is null
4. Unhandled exception kills the DMA thread → no more timer callbacks run → no retry loop

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Fixed Random Bone Magazine Ammo Detection

**Agent:** opencode
**Focus:** C++

### Changes
- **Fixed:** Random bone feature now correctly detects ammo count changes to trigger per-shot bone re-rolls.
- **Technical Details:** `CFirearmManager::RefreshAmmoCount()` was reading `CUnityList::Count` (offset 0x18) which returns the number of ammo stacks (~1) instead of the actual ammo count. Fixed by following the same memory traversal pattern as `CMagazine.cpp`:
  - `itemsList + 0x10` -> `listAddress`
  - `listAddress + 0x20` -> `ammoItem`
  - `ammoItem + CItem::StackCount (0x24)` -> actual ammo count

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Fixed Raid Discovery Retry Loop

**Agent:** opencode
**Focus:** C++

### Changes
- **Fixed:** CTimer clock type mismatch in `DMA Thread.h`. The `Tick()` parameter was `steady_clock::time_point` but caller passed `high_resolution_clock::time_point` and stored member was also `high_resolution_clock`. Now consistently uses `high_resolution_clock` throughout.
- **Added:** Try-catch wrapper around entire `GameWorld_Retry` callback to prevent silent thread crashes from unhandled exceptions.
- **Added:** Periodic tick logging (every 10 seconds) to confirm the retry loop is actually running.
- **Fixed:** Removed duplicate `#include "pch.h"` in `EFT.cpp`.
- **Technical Details:** The clock mismatch could cause undefined behavior when comparing time points, potentially breaking timer execution. This explains why "no retry logs appear after initial failure".

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-15] - Fixed Aimline/Trajectory Visibility Logic

**Agent:** opencode
**Focus:** C++

### Changes
- **Fixed:** Aimline and trajectory now display correctly based on `bOnlyOnAim` setting.
  - Previously, all visuals were blocked by an early return (`if (!bAimKeyActive) return;`) before the `shouldDraw` check could be evaluated.
  - Moved `shouldDrawVisuals` calculation before the early return block.
  - Aimline/trajectory now show permanently when `bOnlyOnAim` is OFF, and only when scoped/aiming when ON.
- **Modified:** Aim point dot and debug visualization still require aim key held (intentional behavior).
- **Technical Details:** Restructured `Aimbot::Render()` logic in `Aimbot.cpp:449-540` to properly gate each visual element.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Aimbot Visuals Logic & Settings

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** "Only Show on Aim" setting (`bOnlyOnAim`) to hide aimline and trajectory when not scoped/aiming.
- **Modified:** Aimline toggle is now forcibly disabled and uncheckable when Trajectory visualization is active, preventing overlapping visuals.
- **Modified:** Added serialization for the new "Only Show on Aim" setting in `Config.cpp`.
- **Fixed:** Render logic now respects the "Only on Aim" flag for both aimlines and trajectory prediction.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Removed Frame Log Spam

**Agent:** opencode
**Focus:** C++

### Changes
- **Fixed:** Removed "OnFrame begin" and "OnFrame end" log lines from the main loop that were causing excessive log spam.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Verbose Discovery Debugging

**Agent:** opencode
**Focus:** C++

### Changes
- **Modified:** `EFT::TryMakeNewGameWorld` now logs detailed status messages (start, success, failure) to the console to confirm the discovery loop is active.
- **Modified:** Explicitly resets `g_DiscoveryPending` and clears GOM cache at the start of every attempt to prevent stale state from blocking retries.
- **Fixed:** Build failure caused by locked executable (process killed via PowerShell before rebuild).

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Reliable build script + docs


**Agent:** opencode
**Focus:** C++ / Docs

### Changes
- **Modified:** `build.bat` now bootstraps the VS dev environment, builds the `.vcxproj`, and passes `SolutionDir` so dependency includes resolve.
- **Modified:** `AGENTS.md` now documents the reliable build flow and the VS fallback command.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Remove broken Safe Lock

**Agent:** opencode
**Focus:** C++

### Changes
- **Removed:** Safe Lock toggle, config persistence, and target-lock state.
- **Modified:** Target selection and aimline logic now operate without lock preference.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - ESP Lag Mitigation (Random Bone)


**Agent:** opencode
**Focus:** C++

### Changes
- **Modified:** Removed per-frame ammo count reads from firearm QuickUpdate; ammo refresh now throttled and only triggered by random-bone aimbot logic.
- **Modified:** Random-bone state now refreshes ammo on-demand with weapon-swap detection instead of constant polling.
- **Technical Details:** Added `RefreshAmmoCount()` with a 100ms throttle and cached hands controller to avoid ESP stalls from synchronous DMA reads.

### Verification
- [x] Build succeeded (0 errors)
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Discovery Retry Logging

**Agent:** opencode
**Focus:** C++

### Changes
- **Modified:** Discovery retry loop now logs periodically while pending to confirm continuous attempts.
- **Unchanged:** GOM cache resets and indefinite retry cadence remain in place for clean reattach.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Endless GOM Discovery Retry

**Agent:** opencode
**Focus:** C++

### Changes
- **Modified:** GameWorld discovery now retries indefinitely via the DMA thread timer (2s cadence) instead of a fixed attempt cap.
- **Modified:** Discovery pending flag stays set until a successful attach; failures reset GOM cache but keep retries running.
- **Unchanged:** Raid-state reset still clears GOM cache to ensure clean reattach for the next raid.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Aimbot performance safeguards

**Agent:** ChatGPT
**Focus:** C++

### Changes
- **Modified:** Gated aimline, prediction, trajectory, and debug visuals on aim-key state and feature toggles to avoid per-frame heavy work when disabled.
- **Modified:** Random-bone selection now updates only while aiming in random mode and resets when toggled off.
- **Modified:** Removed hot-path logging and reuse aim origin to cut redundant world-to-screen and player scans.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - GOM Discovery Resilience

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** Multi-attempt GameObjectManager discovery with cache reset to avoid getting stuck on first failure.
- **Added:** Discovery-pending flag exposed alongside raid-state tracking for downstream logic.
- **Modified:** Raid-state reset now clears GOM cache so the next raid can reattach cleanly.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Stabilize ESP rendering locks

**Agent:** ChatGPT
**Focus:** C++

### Changes
- **Added:** None
- **Modified:** Player, loot, and exfil ESP draws now use scoped locks instead of try-lock skips to keep overlays stable.
- **Fixed:** Flickering ESP caused by skipped frames when render mutexes were busy.
- **Technical Details:** Local player data is fetched under a blocking lock before rendering, avoiding missed draw calls while keeping the existing optic ESP gating.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Raid Waiting State & Reset

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** Raid state tracker that validates main player and player count each second to determine active raids.
- **Modified:** DMA thread clears stale GameWorld and resets raid state so next raid reattaches cleanly.
- **Modified:** Radar UI now uses the raid flag to show "Waiting for raid" when out of raid and resets cached positions.

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Patch Note Rule Enforcement

**Agent:** opencode
**Focus:** Docs

### Changes
- **Modified:** Added mandatory post-build patch note rule to `AGENTS.md`.
- **Modified:** Updated the patch note template with a checkbox to confirm notes after successful builds.
- **Modified:** Clarified `PatchNotes.md` intro that every successful build needs a concise entry.

### Verification
- [ ] Build succeeded (N/A - docs-only change)
- [ ] Patch note recorded after the successful build (N/A - no build run)
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Config Persistence Coverage

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** Full ESP enemy serialization (boxes, skeleton, info, head dot) to saved configs.
- **Added:** Optic ESP state, optic index/radius, radar floor, data table toggles, and Flea Bot runtime settings to config files.
- **Added:** Keybind persistence for Flea Bot and Optic ESP; extended UI tables save/load.
- **Fixed:** Missing radar floor and world overlay flags now load correctly from configs.

### Verification
- [x] Build succeeded (0 errors)
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Random Bone Per-Shot Fix

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** Ammo count accessor in `CFirearmManager` to expose current magazine stack count for aimbot logic.
- **Modified:** Random-bone selection now tracks hands controller + ammo deltas to reroll per shot instead of time-based or invalid hands pointers.
- **Fixed:** Removed broken `m_pHands`/`m_pMagazine` references by using `GetLocalPlayer()` and firearm manager data.
- **Technical Details:** Rolls a new bone on weapon swap and each detected shot via ammo count decrease; weights are normalized before rolls.

### Verification
- [x] Build succeeded (0 errors)
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Single Window Consolidation

**Agent:** opencode
**Focus:** C++

### Changes
- **Modified:** Win32 window title changed from "EFT DMA" to "CyNickal Software"
- **Modified:** Removed `ImGui::DockSpaceOverViewport()` - no longer needed with fullscreen UI
- **Modified:** `MainMenu::Render()` now fills entire viewport with pinned flags:
  - `ImGuiWindowFlags_NoTitleBar` - no ImGui title bar (Win32 title bar remains)
  - `ImGuiWindowFlags_NoResize` / `NoMove` - cannot be resized/moved inside viewport
  - `ImGuiWindowFlags_NoDocking` - cannot be docked/undocked
  - `ImGuiWindowFlags_NoBringToFrontOnFocus` / `NoNavFocus` - stays in background layer
- **Technical Details:** The application now shows as a single "CyNickal Software" window. The ImGui panel fills 100% of the window and cannot be detached. Fuser 3D ESP overlay still works via viewport pop-out for multi-monitor setups.

### Verification
- [x] Build succeeded (0 errors)
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Single Window UI Redesign (Phase 1-2)

**Agent:** opencode
**Focus:** C++

### Changes
- **Added:** `Radar2D::RenderEmbedded()` function to render radar directly in ImGui content area
- **Added:** `Radar2D::GetCurrentFloorName()` helper for status bar display
- **Added:** `Radar2D::GetCurrentMapId()` and `Radar2D::GetLocalPlayerPos()` accessors
- **Added:** New "Radar" category as first item in sidebar (shows radar map by default)
- **Added:** `RenderRadarViewContent()` function in Main Menu to host embedded radar
- **Modified:** `MainMenuState::ECategory` enum - added `Radar` as first category
- **Modified:** `EVisualsItem::Radar` renamed to `EVisualsItem::RadarConfig` for radar settings
- **Modified:** `RenderRadarContent()` renamed to `RenderRadarConfigContent()`
- **Modified:** `Main Window.cpp` - removed separate `Radar2D::Render()` call (radar now embedded)
- **Modified:** Sidebar navigation - "Radar Config" moved under Visuals for settings only
- **Technical Details:** Radar is now embedded in the main window content area when "Radar" sidebar item is selected. The radar canvas renders with full input handling (scroll-to-zoom, middle-click pan) and status bar showing Map/Zoom/Position.

### Verification
- [x] Build succeeded (0 errors)
- [ ] Verified in Debug mode / In-game

---

## [Date: 2026-01-14] - Added Patch Notes Protocol

**Agent:** opencode
**Focus:** Docs

### Changes
- **Added:** `PatchNotes.md` to track project history.
- **Modified:** `AGENTS.md` to include the Patch Notes Protocol requirements.

### Verification
- [x] Build succeeded (N/A - docs only)
- [x] Verified file creation
