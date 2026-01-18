#include "pch.h"
#include "ESPPreview.h"
#include <imgui_internal.h>

namespace ESPPreview {
    // Relative positions for a standing mannequin (0.0 to 1.0)
    // X: 0.0 = left, 1.0 = right (centered at 0.5)
    // Y: 0.0 = top (head), 1.0 = bottom (feet)
    static const ImVec2 staticBones[] = {
        {0.50f, 0.03f},  // 0: Head (top)
        {0.50f, 0.10f},  // 1: Neck
        {0.50f, 0.28f},  // 2: Spine3 (chest)
        {0.50f, 0.45f},  // 3: Pelvis
        // Left Arm
        {0.42f, 0.12f},  // 4: LUpperArm (shoulder)
        {0.32f, 0.22f},  // 5: LForeArm1 (elbow area)
        {0.26f, 0.32f},  // 6: LForeArm2 (forearm)
        {0.22f, 0.40f},  // 7: LPalm (hand)
        // Right Arm
        {0.58f, 0.12f},  // 8: RUpperArm (shoulder)
        {0.68f, 0.22f},  // 9: RForeArm1 (elbow area)
        {0.74f, 0.32f},  // 10: RForeArm2 (forearm)
        {0.78f, 0.40f},  // 11: RPalm (hand)
        // Left Leg
        {0.44f, 0.48f},  // 12: LThigh1 (hip)
        {0.42f, 0.62f},  // 13: LThigh2 (thigh)
        {0.40f, 0.78f},  // 14: LCalf (knee/calf)
        {0.38f, 0.95f},  // 15: LFoot
        // Right Leg
        {0.56f, 0.48f},  // 16: RThigh1 (hip)
        {0.58f, 0.62f},  // 17: RThigh2 (thigh)
        {0.60f, 0.78f},  // 18: RCalf (knee/calf)
        {0.62f, 0.95f}   // 19: RFoot
    };

    static void ConnectBones(ImDrawList* drawList, ImVec2 posA, ImVec2 posB, ImColor color, float thickness) {
        drawList->AddLine(posA, posB, color, thickness);
    }

    static void DrawBox(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImColor color, float thickness, int style, bool filled, ImColor fillColor) {
        if (filled) {
            drawList->AddRectFilled(min, max, fillColor);
        }

        if (style == 0) { // Full Box
            drawList->AddRect(min, max, color, 0.0f, 0, thickness);
        } else { // Corners
            float sizeX = (max.x - min.x) / 4.0f;
            float sizeY = (max.y - min.y) / 6.0f;

            // Top Left
            drawList->AddLine(min, ImVec2(min.x + sizeX, min.y), color, thickness);
            drawList->AddLine(min, ImVec2(min.x, min.y + sizeY), color, thickness);
            // Top Right
            drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x - sizeX, min.y), color, thickness);
            drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + sizeY), color, thickness);
            // Bottom Left
            drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + sizeX, max.y), color, thickness);
            drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x, max.y - sizeY), color, thickness);
            // Bottom Right
            drawList->AddLine(max, ImVec2(max.x - sizeX, max.y), color, thickness);
            drawList->AddLine(max, ImVec2(max.x, max.y - sizeY), color, thickness);
        }
    }

    void Render(ImVec2 availableSize) {
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Use full available size with small margin
        float margin = 10.0f;
        float previewWidth = availableSize.x - margin * 2.0f;
        float previewHeight = availableSize.y - margin * 2.0f;
        
        // Ensure minimum size
        if (previewWidth < 100.0f) previewWidth = 100.0f;
        if (previewHeight < 150.0f) previewHeight = 150.0f;

        // Draw preview background
        ImVec2 bgMin = canvasPos;
        ImVec2 bgMax = ImVec2(canvasPos.x + previewWidth, canvasPos.y + previewHeight);
        drawList->AddRectFilled(bgMin, bgMax, ImColor(20, 20, 30, 255), 4.0f);
        drawList->AddRect(bgMin, bgMax, ImColor(60, 60, 70, 255), 4.0f);

        // Define draw area with padding inside the preview box
        float padding = 25.0f;
        ImVec2 drawAreaMin = {canvasPos.x + padding, canvasPos.y + padding + 25.0f};  // Extra top for text
        ImVec2 drawAreaMax = {canvasPos.x + previewWidth - padding, canvasPos.y + previewHeight - padding - 20.0f};  // Extra bottom for distance text
        float drawWidth = drawAreaMax.x - drawAreaMin.x;
        float drawHeight = drawAreaMax.y - drawAreaMin.y;

        // Ensure valid draw area
        if (drawWidth < 50.0f || drawHeight < 80.0f) {
            ImGui::Dummy(availableSize);
            return;
        }

        // Map relative bone positions to preview area
        // The mannequin is designed to fit in the center with proper proportions
        ImVec2 bones[20];
        for (int i = 0; i < 20; i++) {
            bones[i].x = drawAreaMin.x + staticBones[i].x * drawWidth;
            bones[i].y = drawAreaMin.y + staticBones[i].y * drawHeight;
        }

        // Calculate scale factor for thickness/radius based on preview size
        float scaleFactor = (drawHeight / 400.0f);  // Base scale on height
        if (scaleFactor < 0.5f) scaleFactor = 0.5f;
        if (scaleFactor > 2.0f) scaleFactor = 2.0f;

        using namespace ESPSettings::Enemy;

        // Draw Skeleton
        if (bSkeletonEnabled) {
            float thick = skeletonThickness * scaleFactor;
            ImColor col = skeletonColor;

            if (bBonesHead) {
                ConnectBones(drawList, bones[0], bones[1], col, thick); // Head to Neck
            }
            if (bBonesSpine) {
                ConnectBones(drawList, bones[1], bones[2], col, thick); // Neck to Spine
                ConnectBones(drawList, bones[2], bones[3], col, thick); // Spine to Pelvis
            }
            if (bBonesArmsL) {
                ConnectBones(drawList, bones[1], bones[4], col, thick); // Neck to LUpperArm
                ConnectBones(drawList, bones[4], bones[5], col, thick);
                ConnectBones(drawList, bones[5], bones[6], col, thick);
                ConnectBones(drawList, bones[6], bones[7], col, thick);
            }
            if (bBonesArmsR) {
                ConnectBones(drawList, bones[1], bones[8], col, thick); // Neck to RUpperArm
                ConnectBones(drawList, bones[8], bones[9], col, thick);
                ConnectBones(drawList, bones[9], bones[10], col, thick);
                ConnectBones(drawList, bones[10], bones[11], col, thick);
            }
            if (bBonesLegsL) {
                ConnectBones(drawList, bones[3], bones[12], col, thick); // Pelvis to LThigh
                ConnectBones(drawList, bones[12], bones[13], col, thick);
                ConnectBones(drawList, bones[13], bones[14], col, thick);
                ConnectBones(drawList, bones[14], bones[15], col, thick);
            }
            if (bBonesLegsR) {
                ConnectBones(drawList, bones[3], bones[16], col, thick); // Pelvis to RThigh
                ConnectBones(drawList, bones[16], bones[17], col, thick);
                ConnectBones(drawList, bones[17], bones[18], col, thick);
                ConnectBones(drawList, bones[18], bones[19], col, thick);
            }
        }

        // Draw Head Dot
        if (bHeadDotEnabled) {
            float radius = headDotRadius * scaleFactor;
            if (radius < 2.0f) radius = 2.0f;
            drawList->AddCircleFilled(bones[0], radius, headDotColor);
        }

        // Calculate bounding box for the mannequin body (not hands/feet extremes)
        // Use a tighter box around the main body
        float bodyLeft = bones[7].x;   // Left hand
        float bodyRight = bones[11].x; // Right hand
        float bodyTop = bones[0].y - 8.0f * scaleFactor;    // Above head
        float bodyBottom = bones[15].y; // Feet
        
        // Expand box slightly for padding
        float boxPadding = 8.0f * scaleFactor;
        ImVec2 bMin = ImVec2(bodyLeft - boxPadding, bodyTop);
        ImVec2 bMax = ImVec2(bodyRight + boxPadding, bodyBottom + boxPadding * 0.5f);

        // Draw Box
        if (bBoxEnabled) {
            float thick = boxThickness * scaleFactor;
            DrawBox(drawList, bMin, bMax, boxColor, thick, boxStyle, bBoxFilled, boxFillColor);
        }

        // Draw Info Text (scaled font would be ideal, but we'll use positioning)
        float textCenterX = (bMin.x + bMax.x) * 0.5f;
        
        // Name above box
        if (bNameEnabled) {
            const char* name = "PlayerName";
            ImVec2 tSize = ImGui::CalcTextSize(name);
            float textY = bMin.y - tSize.y - 2.0f;
            if (bWeaponEnabled) textY -= tSize.y + 2.0f;  // Make room for weapon
            drawList->AddText(ImVec2(textCenterX - tSize.x * 0.5f, textY), nameColor, name);
        }
        
        // Weapon above box (below name)
        if (bWeaponEnabled) {
            const char* weapon = "AK-74M";
            ImVec2 tSize = ImGui::CalcTextSize(weapon);
            float textY = bMin.y - tSize.y - 2.0f;
            drawList->AddText(ImVec2(textCenterX - tSize.x * 0.5f, textY), weaponColor, weapon);
        }

        // Distance below box
        if (bDistanceEnabled) {
            const char* dist = "125m";
            ImVec2 tSize = ImGui::CalcTextSize(dist);
            drawList->AddText(ImVec2(textCenterX - tSize.x * 0.5f, bMax.y + 4.0f), distanceColor, dist);
        }
        
        // Health bar on right side of box
        if (bHealthEnabled) {
            float barWidth = 4.0f * scaleFactor;
            float barHeight = bMax.y - bMin.y;
            float healthPercent = 0.75f;  // Example: 75% health
            
            // Background
            drawList->AddRectFilled(
                ImVec2(bMax.x + 4.0f, bMin.y), 
                ImVec2(bMax.x + 4.0f + barWidth, bMax.y), 
                ImColor(40, 40, 40, 200)
            );
            // Health fill (from bottom)
            float fillHeight = barHeight * healthPercent;
            drawList->AddRectFilled(
                ImVec2(bMax.x + 4.0f, bMax.y - fillHeight), 
                ImVec2(bMax.x + 4.0f + barWidth, bMax.y), 
                ImColor(50, 200, 50, 255)
            );
        }

        // Reserve space for the preview
        ImGui::Dummy(ImVec2(previewWidth, previewHeight));
    }
}
