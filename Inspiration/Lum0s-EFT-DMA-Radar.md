# Lum0s-EFT-DMA-Radar

## Project Info
- **Repository:** https://github.com/Lum0s36/Lum0s-EFT-DMA-Radar
- **Language:** C# (.NET WPF)
- **License:** MIT
- **Note:** AI-generated commits, not always tested

## Project Structure
```
src/
├── Tarkov/
│   ├── Features/
│   │   ├── Memwrites/
│   │   │   ├── InfiniteStamina.cs
│   │   │   ├── MemoryAim.cs
│   │   │   ├── NoRecoil.cs
│   │   │   └── MemWriteConstants.cs
│   │   ├── MemWriteFeature.cs
│   │   └── MemWritesManager.cs
│   ├── GameWorld/
│   ├── Unity/
│   ├── SDK.cs
│   └── TarkovDataManager.cs
├── UI/
│   ├── ESP/
│   │   ├── ESPManager.cs
│   │   ├── ESPWindow.xaml
│   │   ├── Dx9OverlayControl.cs
│   │   └── Rendering/
│   ├── Radar/
│   ├── Loot/
│   ├── Hotkeys/
│   └── Skia/
├── DMA/
└── Config/
```

## Features of Interest
- MemoryAim (memory-based aim)
- NoRecoil
- InfiniteStamina
- ESP overlay (DirectX 9)
- Quest helper widget
- Skeleton visualization
- Loot filtering

## Key Differences from CyNickal
- C# vs C++ implementation
- WPF UI vs ImGui
- DirectX 9 overlay vs DirectX 11
