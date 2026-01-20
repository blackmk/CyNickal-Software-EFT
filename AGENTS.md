# AGENTS.md - CyNickal Software EFT

Guidelines for AI coding agents working in this repository.

## Project Overview

DMA radar application for Escape From Tarkov with two components:
- **C++ Native Application** (`CyNickal Software EFT/`) - Win32/DirectX11 ImGui app
- **C#/.NET 9 WPF Application** (`EFT-DMA-Radar-Source/`) - SkiaSharp-based radar

Both use DMA (Direct Memory Access) via MemProcFS/vmmdll for memory reading.

---

## Inspiration Projects

Reference projects for feature ideas. See individual files in `Inspiration/` folder.

| Project | File | Key Features |
|---------|------|--------------|
| Lum0s-EFT-DMA-Radar | `Inspiration/Lum0s-EFT-DMA-Radar.md` | MemoryAim, NoRecoil, ESP overlay |

When implementing features from inspiration projects:
1. Read the corresponding MD file for project structure
2. Note that most are C# - adapt patterns to C++
3. Follow existing CyNickal coding conventions

---

## Build Commands

### C++ (Main Application)
```bash
build.bat                    # Reliable build (Release x64 default)
build.bat Debug x64          # Debug x64
build.bat Release x64        # Release x64
build.bat Release Win32      # Release Win32
# Output: x64\Release\EFT_DMA.exe
```

**Reliable build notes**
- `build.bat` bootstraps the VS dev environment and builds the `.vcxproj` directly; run it from repo root.
- **Always invoke `build.bat` directly** (not via `cmd.exe /c`) - direct invocation captures output correctly.
- Manual fallback (Developer PowerShell for VS 2022): `msbuild "CyNickal Software EFT\CyNickal Software EFT.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:minimal`


### C#/.NET Application
```bash
cd EFT-DMA-Radar-Source/src
dotnet build                 # Debug build
dotnet build -c Release      # Release build
dotnet run                   # Run application
```

### CRITICAL: Mandatory Build Verification

**After completing ANY coding task, you MUST:**
1. Run build command (`build.bat` for C++, `dotnet build` for C#)
2. Fix ALL compiler errors before considering work complete
3. Re-run build after each fix until 0 errors
4. Warnings are acceptable; errors are NOT

---

## Code Style Guidelines

### C++ Naming Conventions
| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase, `C` prefix for game structs | `CBaseEntity`, `CLocalGameWorld` |
| Member variables | `m_` prefix | `m_VMMHandle`, `m_bConnected` |
| Static members | `s_` prefix | `s_initialized`, `s_items` |
| Booleans | `b` prefix | `bRunning`, `bMasterToggle` |
| Floats | `f` prefix | `fPixelFOV`, `fDampen` |
| Pointers | `m_p` prefix | `m_pPlayerBody` |
| Constants | `k` prefix or SCREAMING_SNAKE | `kMaxPlayers`, `MAX_ITEMS` |
| Functions/Methods | PascalCase | `GetInstance()`, `ReadMem()` |
| Namespaces | PascalCase | `Offsets::CPlayer` |

### C++ Formatting
- Tabs for indentation (not spaces)
- Opening braces: same line for control structures, new line for functions/classes
- Use `#pragma once` for header guards

### C++ Includes (Order Matters)
```cpp
#include "pch.h"              // Always first (precompiled header)
#include "LocalHeader.h"      // Project headers with quotes
#include <standard_header>    // System headers with angle brackets
```

### C++ Error Handling
```cpp
if (!pPlayer || pPlayer->IsInvalid())
    return;  // Early return for invalid state

if (DebugMode::IsDebugMode() || !Conn || !Conn->IsConnected())
    return T{};  // Handle debug mode gracefully
```

### C++ Modern Features
- Use C++20/23 features (`std::println`, `std::expected`, `std::ranges`)
- Use `constexpr` and `inline` for compile-time constants
- Use `std::scoped_lock` for mutex protection

### C# Naming Conventions
| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `LocalGameWorld`, `CameraManager` |
| Private fields | `_camelCase` | `_vmm`, `_processId` |
| Properties | PascalCase | `UnityBase`, `InRaid` |
| Interfaces | `I` prefix | `IWorldEntity` |

### C# Null Safety & Error Handling
```csharp
public static bool InRaid => Game?.InRaid ?? false;  // Null-conditional

try { /* risky op */ }
catch (Exception ex) { DebugLogger.LogDebug($"[Module] Error: {ex.Message}"); }
```

---

## Project Structure

```
CyNickal-Software-EFT-master/
├── CyNickal Software EFT/     # C++ main application
│   ├── DMA/                   # DMA connection, memory reading
│   ├── Game/Classes/          # Game object wrappers
│   ├── Game/Offsets/          # Memory offsets (Offsets.h)
│   ├── GUI/                   # ImGui interface
│   └── main.cpp               # Entry point
├── EFT-DMA-Radar-Source/src/  # C# .NET application
│   ├── Tarkov/SDK.cs          # C# offsets
│   └── UI/                    # WPF/SkiaSharp interface
├── Dependencies/              # Third-party libs (ImGui, MemProcFS, nlohmann)
├── Inspiration/               # Reference projects for feature ideas
└── build.bat                  # C++ build script
```

---

## Architecture Overview

### Data Flow
```
[Game Memory] → [DMA Thread (MemProcFS)] → [EFT Static Facade] → [ImGui GUI]
                                                ↓
                                         [Config System (JSON)]
```

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| DMA Layer | `DMA/` | Memory access via MemProcFS singleton |
| Game State | `Game/` | `EFT` facade, `CLocalGameWorld`, `CRegisteredPlayers` |
| GUI | `GUI/` | ImGui/D3D11, `Main Menu.cpp` aggregates all settings |
| Config | `GUI/Config/` | JSON serialization to `%APPDATA%/EFT_DMA/configs/` |

### Critical Files

| File | Purpose |
|------|---------|
| `Game/Offsets/Offsets.h` | Memory offsets (sync with C#) |
| `EFT-DMA-Radar-Source/src/Tarkov/SDK.cs` | C# offsets (must match C++) |
| `GUI/Main Menu/Main Menu.cpp` | UI settings aggregation |
| `GUI/Config/Config.cpp` | Config serialization |
| `Game/EFT.h` | Game state facade |
| `DMA/DMA.h` | Memory access singleton |

---

## Key Rules

### UI Changes (C++)
Add all UI settings to `GUI/Main Menu/Main Menu.cpp` via `RenderXContent()` functions.
Legacy `RenderSettings()` functions in individual files are NOT used.

### Config Persistence
New settings MUST be added to BOTH `SerializeConfig()` AND `DeserializeConfig()` in `GUI/Config/Config.cpp`.

### Offsets Sync
Keep C++ offsets (`Game/Offsets/Offsets.h`) and C# offsets (`Tarkov/SDK.cs`) in sync.

---

## Testing

No automated tests. Manual workflow:
1. Build with `build.bat` or `dotnet build`
2. Connect DMA device (or use debug mode for UI testing)
3. Test features in-game

---

## Git Workflow

See `CLAUDE.md` for full Git workflow documentation.

### Quick Reference
- **Work on `dev` branch** - All development happens here
- **Merge to `main`** - Only when code is stable and tested
- **After successful build**: Update `PatchNotes.md`, commit to `dev`, push to origin

### Remotes
- `origin` - Your fork (github.com/blackmk/CyNickal-Software-EFT)
- `upstream` - Original project (github.com/CyN1ckal/CyNickal-Software-EFT)

---

## Patch Notes Protocol

Document all changes in `PatchNotes.md` (repository root).

### Rules
1. Update at the end of every significant task
2. Newest entries first (reverse chronological)
3. Use the template below
4. After every successful build, add a brief patch note entry summarizing what changed (a concise "I changed X" is acceptable)
5. **Commit to `dev` branch**, not `main`

### Template
```markdown
## [Date: YYYY-MM-DD] - [Brief Summary of Task]

**Agent:** [Agent Name/ID]
**Focus:** [C++ / C# / Shared / Docs]

### Changes
- **Added:** [New features or files]
- **Modified:** [Existing functionality changed]
- **Fixed:** [Bugs or errors resolved]
- **Technical Details:** [Brief explanation if complex]

### Verification
- [ ] Build succeeded (0 errors)
- [ ] Patch note recorded after the successful build
- [ ] Verified in Debug mode / In-game
```

---

## Warnings

- **Never commit sensitive data** (IP addresses, MAC addresses, credentials)
- **Offsets change with game updates** - verify before PRs
- **DMA hardware required** for full functionality
- Keep `Dependencies/` libraries in sync with project settings

---

## Dependencies

All in `Dependencies/` directory:
- **ImGui**: UI framework (DirectX11)
- **MemProcFS**: DMA memory reading
- **nlohmann/json**: JSON parsing
- **sqlite3**: Item database
- **libcurl**: HTTP requests
- **Makcu**: Hardware input (Kmbox)
- **nanosvg**: SVG parsing

---

## Thread Safety

- DMA thread reads memory; GUI thread uses results
- Use `std::scoped_lock` for mutex protection
- Data is read-only from GUI perspective

---

## Code Review with CodeRabbit

CodeRabbit CLI is available in WSL (Ubuntu) for AI-powered code reviews.

### Setup
- **Binary:** `/root/.local/bin/coderabbit` (WSL Ubuntu)
- **Version:** 0.3.5
- **API Key:** Set via `CODERABBIT_API_KEY` environment variable

### Quick Commands

```bash
# Review all changes (from Windows terminal)
wsl -d Ubuntu -- bash -c "cd /mnt/c/path/to/repo && export PATH=$PATH:/root/.local/bin && export CODERABBIT_API_KEY=<YOUR_API_KEY> && coderabbit review --plain"

# Review uncommitted changes only
coderabbit review --type uncommitted --plain

# Token-efficient output (for AI agents)
coderabbit review --prompt-only
```

### Review Options

| Flag | Purpose |
|------|---------|
| `--plain` | Detailed analysis with fix suggestions |
| `--prompt-only` | Minimal output for token efficiency |
| `--type uncommitted` | Only uncommitted changes |
| `--type committed` | Only committed changes |
| `--base <branch>` | Compare against a specific branch |

### Integration Notes
- Run from the git repository root directory
- WSL Ubuntu must have the API key exported in the environment
- Use `--plain` for human-readable output, `--prompt-only` for AI agent consumption
