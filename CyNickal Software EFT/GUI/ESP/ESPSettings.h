#pragma once
#include <imgui.h>

namespace ESPSettings {
    namespace Enemy {
        // Box ESP
        inline bool bBoxEnabled = true;
        inline int boxStyle = 0;  // 0=Full, 1=Corners
        inline ImColor boxColor = ImColor(200, 50, 200);
        inline float boxThickness = 1.5f;
        inline bool bBoxFilled = false;
        inline ImColor boxFillColor = ImColor(0, 0, 0, 50);

        // Skeleton ESP
        inline bool bSkeletonEnabled = true;
        inline ImColor skeletonColor = ImColor(200, 50, 200);
        inline float skeletonThickness = 2.0f;

        // Individual bone group toggles
        inline bool bBonesHead = true;    // Head to neck
        inline bool bBonesSpine = true;   // Neck, spine, pelvis
        inline bool bBonesArmsL = true;   // Left arm chain
        inline bool bBonesArmsR = true;   // Right arm chain
        inline bool bBonesLegsL = true;   // Left leg chain
        inline bool bBonesLegsR = true;   // Right leg chain

        // Info ESP
        inline bool bNameEnabled = true;
        inline ImColor nameColor = ImColor(255, 255, 255);
        inline bool bDistanceEnabled = true;
        inline ImColor distanceColor = ImColor(0, 200, 255);
        inline bool bHealthEnabled = false;
        inline ImColor healthColor = ImColor(255, 100, 100);  // Light red for health status
        inline bool bWeaponEnabled = true;
        inline ImColor weaponColor = ImColor(200, 200, 0);

        // Head dot
        inline bool bHeadDotEnabled = true;
        inline ImColor headDotColor = ImColor(255, 0, 0);
        inline float headDotRadius = 3.0f;
    }

    // Render range settings for ESP elements
    namespace RenderRange {
        // Player ranges by type (in meters)
        inline float fPMCRange = 500.0f;           // PMC players (USEC/BEAR)
        inline float fPlayerScavRange = 400.0f;    // Player Scavs (real players as scav)
        inline float fBossRange = 600.0f;          // Bosses (high priority targets)
        inline float fAIRange = 300.0f;            // Regular AI scavs

        // Player colors by type
        inline ImColor PMCColor = ImColor(255, 0, 0);           // Red
        inline ImColor PlayerScavColor = ImColor(255, 165, 0);  // Orange
        inline ImColor BossColor = ImColor(255, 0, 255);        // Magenta
        inline ImColor AIColor = ImColor(255, 255, 0);          // Yellow

        // Item ranges by value tier (in meters)
        inline float fItemHighRange = 100.0f;      // Valuable items (>100k) - far
        inline float fItemMediumRange = 75.0f;     // Medium value (50k-100k)
        inline float fItemLowRange = 50.0f;        // Low value (20k-50k) - current default
        inline float fItemRestRange = 25.0f;       // Below threshold (<20k) - close only

        // Item colors by value tier (matching LootFilter price tiers)
        inline ImColor ItemHighColor = ImColor(255, 215, 0);    // Gold (>100k)
        inline ImColor ItemMediumColor = ImColor(186, 85, 211); // Purple (50k-100k)
        inline ImColor ItemLowColor = ImColor(0, 150, 150);     // Teal #009696 (20k-50k)
        inline ImColor ItemRestColor = ImColor(108, 150, 150);  // Gray-teal #6C9696 (<20k)

        // Container range + color (in meters)
        inline float fContainerRange = 20.0f;
        inline ImColor ContainerColor = ImColor(0, 255, 255);   // Cyan

        // Exfil range + color (in meters)
        inline float fExfilRange = 500.0f;
        inline ImColor ExfilColor = ImColor(0, 255, 0);         // Green
    }
}
