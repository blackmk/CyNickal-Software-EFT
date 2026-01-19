# Lum0s-Inspired System Overhaul - Project Tracker

This document tracks the implementation progress of the Lum0s-inspired feature overhaul.

---

## Overview

Based on the `Inspiration/Lum0s-EFT-DMA-Radar.md` reference project, we're implementing:
- **Phase 1**: Enhanced Loot Filtering System
- **Phase 2**: Radar Display + Overlay Widgets
- **Phase 3**: Quest System
- **Skipped**: Minimap feature (not needed)

---

## Phase 1: Enhanced Loot Filtering System

**Status: COMPLETE**

### Completed Tasks
- [x] Create `LootFilterEntry.h` - Per-item filter entry struct
- [x] Create `UserLootFilter.h` - Named filter preset collection
- [x] Modify `LootFilter.h` - Add dual-tier pricing, presets, new methods
- [x] Modify `LootFilter.cpp` - Implement filtering logic and UI
- [x] Add UI in Main Menu for loot filters
- [x] Add config persistence for all loot filter settings
- [x] Build and verify (0 errors)

### Features Implemented
- Dual-tier pricing (MinValueRegular: 20k, MinValueValuable: 100k)
- Price-per-slot calculation option
- Named filter presets (create/rename/delete)
- Important items list with custom colors
- Blacklist for hiding items
- Item search by name
- Quest item placeholders (for Phase 3)
- Full config serialization

### Files Created/Modified
| File | Action |
|------|--------|
| `GUI/LootFilter/LootFilterEntry.h` | Created |
| `GUI/LootFilter/UserLootFilter.h` | Created |
| `GUI/LootFilter/LootFilter.h` | Modified |
| `GUI/LootFilter/LootFilter.cpp` | Modified |
| `GUI/Main Menu/Main Menu.cpp` | Modified |
| `GUI/Config/Config.cpp` | Modified |

---

## Phase 2: Radar Display + Widgets

**Status: COMPLETE**

### Completed Tasks
- [x] Create `PlayerFocus.h` - Focus/teammate tracking
- [x] Create `RadarWidget.h` - Widget base class
- [x] Create `PlayerInfoWidget.h/cpp` - Player list widget
- [x] Create `LootInfoWidget.h/cpp` - Top loot widget
- [x] Create `AimviewWidget.h/cpp` - 3D perspective widget
- [x] Create `WidgetManager.h` - Central widget manager
- [x] Modify `Radar2D.h/cpp` - Add widget settings, focus system
- [x] Add `RenderOverlayWidgets()` implementation
- [x] Add widget config persistence
- [x] Add files to `.vcxproj`
- [x] Build and verify (0 errors)

### Features Implemented
- **Player Focus System**
  - Click to focus players
  - Cycle states: None → Focused → TempTeammate → None
  - Configurable highlight colors
- **PlayerInfoWidget**
  - Sortable player list (by distance)
  - Shows: Name, Weapon, Health, Distance
  - Color-coded by type (PMC, Scav, Boss, etc.)
  - Click to focus
- **LootInfoWidget**
  - Top valuable items list
  - Sortable by price or distance
  - Click to highlight on radar (pulse effect)
  - Shows: Name, Price, Distance
- **AimviewWidget**
  - 3D perspective view
  - FOV-based projection
  - Distance-based dot sizing
  - Crosshair and distance circles
- **Widget Framework**
  - Draggable windows
  - Minimize/restore
  - JSON serialization

### Files Created/Modified
| File | Action |
|------|--------|
| `GUI/Radar/PlayerFocus.h` | Created |
| `GUI/Radar/Widgets/RadarWidget.h` | Created |
| `GUI/Radar/Widgets/PlayerInfoWidget.h` | Created |
| `GUI/Radar/Widgets/PlayerInfoWidget.cpp` | Created |
| `GUI/Radar/Widgets/LootInfoWidget.h` | Created |
| `GUI/Radar/Widgets/LootInfoWidget.cpp` | Created |
| `GUI/Radar/Widgets/AimviewWidget.h` | Created |
| `GUI/Radar/Widgets/AimviewWidget.cpp` | Created |
| `GUI/Radar/Widgets/WidgetManager.h` | Created |
| `GUI/Radar/Radar2D.h` | Modified |
| `GUI/Radar/Radar2D.cpp` | Modified |
| `GUI/Config/Config.cpp` | Modified |
| `CyNickal Software EFT.vcxproj` | Modified |

---

## Phase 3: Quest System

**Status: COMPLETE**

### Completed Tasks
- [x] Add quest offsets to `Game/Offsets/Offsets.h`
- [x] Create `Game/Classes/Quest/QuestObjectiveType.h` - Enum for quest types
- [x] Create `Game/Classes/Quest/QuestEntry.h` - Quest entry struct
- [x] Create `Game/Classes/Quest/QuestLocation.h` - Quest marker struct
- [x] Create `Game/Classes/Quest/QuestData.h/cpp` - Quest database from JSON
- [x] Create `Game/Classes/Quest/CQuestManager.h/cpp` - Main quest manager
- [x] Integrate quest items with LootFilter (`IsQuestItem()`)
- [x] Add quest markers to Radar2D (star markers, configurable colors)
- [x] Add quest panel UI in Main Menu (Quest Helper section)
- [x] Add config persistence for quest settings
- [x] Add files to `.vcxproj`
- [x] Build and verify (0 errors, 1 pre-existing warning)

### Features Implemented
- **Quest Memory Reading**
  - Reads active quests from player profile memory
  - Tracks quest status (Started, Completed, etc.)
  - Gets quest item requirements via QuestDatabase
- **Quest Item Integration**
  - Highlights quest items in loot (purple by default)
  - CQuestManager tracks which items are needed
  - Integrates with LootFilter::IsQuestItem()
- **Quest Locations on Radar**
  - Star-shaped quest markers
  - Configurable color and size
  - Height indicators (above/below arrows)
  - Quest name and distance labels
- **Quest Panel UI**
  - List of active quests with toggles
  - Master toggle for quest helper
  - Options for locations/items highlighting
  - Quest marker color picker

### Offsets Added
```cpp
namespace CProfile {
    inline constexpr std::ptrdiff_t pQuestsData{ 0x98 };
}

namespace CQuestsData {
    inline constexpr std::ptrdiff_t pId{ 0x10 };
    inline constexpr std::ptrdiff_t Status{ 0x1C };
    inline constexpr std::ptrdiff_t pCompletedConditions{ 0x28 };
}
```

### Files Created/Modified
| File | Action |
|------|--------|
| `Game/Offsets/Offsets.h` | Modified - Added quest offsets |
| `Game/Classes/Quest/QuestObjectiveType.h` | Created |
| `Game/Classes/Quest/QuestEntry.h` | Created |
| `Game/Classes/Quest/QuestLocation.h` | Created |
| `Game/Classes/Quest/QuestData.h` | Created |
| `Game/Classes/Quest/QuestData.cpp` | Created |
| `Game/Classes/Quest/CQuestManager.h` | Created |
| `Game/Classes/Quest/CQuestManager.cpp` | Created |
| `GUI/LootFilter/LootFilter.cpp` | Modified - IsQuestItem integration |
| `GUI/Radar/Radar2D.h` | Modified - bShowQuestMarkers setting |
| `GUI/Radar/Radar2D.cpp` | Modified - DrawQuestMarkers() |
| `GUI/Main Menu/Main Menu.cpp` | Modified - Quest Helper panel |
| `GUI/Config/Config.cpp` | Modified - Quest settings persistence |
| `CyNickal Software EFT.vcxproj` | Modified - Added quest files |

### Note on Quest Database
The quest system requires a `quests.json` file with tarkov.dev format data.
The QuestDatabase class can load from file via `Initialize(path)`.
Without the JSON file, quest locations won't display but item highlighting still works
based on items the game marks as quest items.

---

## Skipped Features

### Minimap
- **Reason**: Not needed per user request
- **Description**: Would have been a small radar overlay on game screen

---

## Future Considerations

### Potential Improvements
- [ ] Group lines between same-group players on radar
- [ ] Hover tooltips for entities on radar
- [ ] Click handling on radar for player focus
- [ ] ESP integration with widgets
- [ ] Hotkey support for widget toggles

### Known Issues
- C4244 warnings in widget files (wchar_t to char conversion in GetTemplateId)
  - This is a pre-existing issue in CObservedLootItem, not introduced by Phase 2

---

## Build Status

| Phase | Status | Errors | Warnings |
|-------|--------|--------|----------|
| Phase 1 | Complete | 0 | Some |
| Phase 2 | Complete | 0 | Some (pre-existing) |
| Phase 3 | Complete | 0 | 1 (pre-existing C4244) |

**Last Successful Build**: 2026-01-19 (Phase 3 - Quest System)

---

## Quick Reference

### How to Enable Widgets
1. Open the main menu
2. Go to Radar Config section
3. Expand "Overlay Widgets" header
4. Toggle desired widgets on/off
5. Configure individual widget settings

### How to Use Player Focus
1. In PlayerInfoWidget, click a player row to focus
2. Click again to mark as temporary teammate
3. Click again to clear
4. Or use "Clear Focus" / "Clear All Temp Teammates" buttons in settings

### How to Use Loot Filters
1. Open Radar Config section
2. Expand "Loot Filters" header
3. Select or create a filter preset
4. Add items to Important or Blacklist
5. Adjust price thresholds as needed

### How to Use Quest Helper
1. Open the main menu
2. Go to "Quest Helper" section
3. Toggle "Enable Quest Helper" on
4. Enable/disable individual quests as needed
5. Configure quest marker colors and item highlighting
6. Quest locations appear as star markers on radar
7. Quest items are highlighted in loot display
