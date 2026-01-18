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
}
