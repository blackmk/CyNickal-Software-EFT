# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Structure

This is a **fork** of the original CyNickal project with custom modifications.

| Remote | URL | Purpose |
|--------|-----|---------|
| `origin` | `github.com/blackmk/CyNickal-Software-EFT` | Your fork (push here) |
| `upstream` | `github.com/CyN1ckal/CyNickal-Software-EFT` | Original project |

## Inspiration Projects

Reference projects for feature ideas. See individual files in `Inspiration/` folder.

| Project | File | Key Features |
|---------|------|--------------|
| Lum0s-EFT-DMA-Radar | `Inspiration/Lum0s-EFT-DMA-Radar.md` | MemoryAim, NoRecoil, ESP overlay |

When implementing features from inspiration projects:
1. Read the corresponding MD file for project structure
2. Note that most are C# - adapt patterns to C++
3. Follow existing CyNickal coding conventions

## Git Workflow

### Branching Strategy
- **`main`** - Stable, tested code only. Merge here when features are complete and verified.
- **`dev`** - Active development branch. All work happens here first.

### Development Workflow

```bash
# 1. Start work on dev branch
git checkout dev

# 2. Make changes, build, test
build.bat
# Fix any errors...

# 3. Update PatchNotes.md (see below)

# 4. Commit to dev
git add .
git commit -m "Brief description of changes"

# 5. Push to your fork
git push origin dev
```

### Merging to Main (When Stable)

```bash
# 1. Ensure dev is up to date and builds clean
git checkout dev
build.bat  # Must have 0 errors

# 2. Switch to main and merge
git checkout main
git merge dev

# 3. Push stable version
git push origin main
```

### Syncing with Upstream (Getting Updates from Original)

```bash
# Fetch upstream changes
git fetch upstream

# Merge upstream into your dev branch
git checkout dev
git merge upstream/main

# Resolve any conflicts, then push
git push origin dev
```

### After Every Successful Build

1. **Update `PatchNotes.md`** - Add entry at the top (newest first)
2. **Commit to `dev`** - Not main, unless explicitly merging stable code
3. **Push to origin** - Keep your fork updated

### PatchNotes.md Format

Add new entries at the TOP of the History section:

```markdown
## [Date: YYYY-MM-DD] - Brief Summary

**Agent:** Claude Code (or your name)
**Focus:** C++ / C# / Docs

### Changes
- **Added:** New features
- **Modified:** Changed functionality
- **Fixed:** Bug fixes

### Verification
- [x] Build succeeded (0 errors)
- [x] Patch note recorded
- [ ] Verified in-game
```

## Build Commands

### C++ Application (Primary)
```bash
# Default build (Release x64)
build.bat

# Specific configuration
build.bat Debug x64
build.bat Release x64
build.bat DLL x64
```

Output: `x64\Release\EFT_DMA.exe`

### C# Radar (Secondary)
```bash
cd EFT-DMA-Radar-Source/src
dotnet build -c Release
```

**Critical**: After ANY code changes, run `build.bat` and fix ALL compiler errors (0 errors required).

## Architecture Overview

### Data Flow
```
[Game Memory] → [DMA Thread (MemProcFS)] → [EFT Static Facade] → [ImGui GUI]
                                                ↓
                                         [Config System (JSON)]
```

### Key Components

**DMA Layer** (`DMA/`)
- `DMA_Connection::GetInstance()` - Singleton for memory access via MemProcFS
- `DMA_Thread_Main()` - Background thread for memory reading
- Uses `LightRefresh()` (positions) and `FullRefresh()` (detailed state)

**Game State** (`Game/`)
- `EFT` static class - Central facade for all game data
- `CLocalGameWorld` - Current map/raid state
- `CRegisteredPlayers` - Player list container
- `CLootList` - Loot items
- All game classes prefixed with `C` wrap raw memory addresses

**GUI** (`GUI/`)
- ImGui with D3D11 backend
- `MainWindow` - Rendering loop
- `Main Menu/Main Menu.cpp` - ALL UI settings aggregated here via `RenderXContent()` functions

**Config** (`GUI/Config/`)
- JSON serialization via nlohmann::json
- Saved to `%APPDATA%/EFT_DMA/configs/`

### Critical Files
| File | Purpose |
|------|---------|
| `Game/Offsets/Offsets.h` | Memory offsets (sync with C#) |
| `EFT-DMA-Radar-Source/src/Tarkov/SDK.cs` | C# offsets (must match C++) |
| `GUI/Main Menu/Main Menu.cpp` | UI settings aggregation |
| `GUI/Config/Config.cpp` | Config serialization |
| `Game/EFT.h` | Game state facade |
| `DMA/DMA.h` | Memory access singleton |

## Coding Conventions

### Naming
- Classes: `CClassName` (PascalCase with C prefix)
- Members: `m_name`, booleans `m_bFlag`, floats `m_fValue`, pointers `m_pPtr`
- Static members: `s_name`
- Functions: `PascalCase()`

### Formatting
- Indentation: Tabs
- Braces: Same line for control structures, new line for functions/classes
- Headers: `#pragma once`, include `pch.h` first

### Modern C++ (C++20/23)
- Use `std::println`, `std::ranges`, `std::expected`
- Use `std::scoped_lock` for mutex protection
- Use `constexpr`/`inline` for compile-time constants

## Development Rules

### UI Changes
All new UI settings must go in `GUI/Main Menu/Main Menu.cpp` via `RenderXContent()` functions. Legacy `RenderSettings()` in individual files are deprecated.

### Config Persistence
New settings must be added to BOTH:
- `SerializeConfig()` in `GUI/Config/Config.cpp`
- `DeserializeConfig()` in `GUI/Config/Config.cpp`

### Offset Synchronization
When updating memory offsets, sync both:
- C++: `Game/Offsets/Offsets.h`
- C#: `EFT-DMA-Radar-Source/src/Tarkov/SDK.cs`

### Patch Notes
Document changes in `PatchNotes.md` after successful builds (newest first).

## Dependencies

All in `Dependencies/` directory:
- **ImGui**: UI framework (DirectX11)
- **MemProcFS**: DMA memory reading
- **nlohmann/json**: JSON parsing
- **sqlite3**: Item database
- **libcurl**: HTTP requests
- **Makcu**: Hardware input (Kmbox)
- **nanosvg**: SVG parsing

## Error Handling Pattern

```cpp
// Early return for invalid state
if (!pPlayer || pPlayer->IsInvalid())
    return;

// Debug mode fallback
if (DebugMode::IsDebugMode() || !Conn || !Conn->IsConnected())
    return T{};
```

## Thread Safety

- DMA thread reads memory; GUI thread uses results
- Use `std::scoped_lock` for mutex protection
- Data is read-only from GUI perspective
