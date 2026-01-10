#include "pch.h"
#include "ESPPreview.h"
#include <imgui_internal.h>

namespace ESPPreview {
    // Relative positions for a standing mannequin (0.0 to 1.0)
    // Centered horizontally (0.5)
    static const ImVec2 staticBones[] = {
        {0.50f, 0.05f}, // Head (0)
        {0.50f, 0.12f}, // Neck (1)
        {0.50f, 0.30f}, // Spine3 (2)
        {0.50f, 0.50f}, // Pelvis (3)
        // Arms
        {0.45f, 0.15f}, // LUpperArm (4)
        {0.35f, 0.25f}, // LForeArm1 (5)
        {0.30f, 0.35f}, // LForeArm2 (6)
        {0.25f, 0.40f}, // LPalm (7)
        {0.55f, 0.15f}, // RUpperArm (8)
        {0.65f, 0.25f}, // RForeArm1 (9)
        {0.70f, 0.35f}, // RForeArm2 (10)
        {0.75f, 0.40f}, // RPalm (11)
        // Legs
        {0.45f, 0.55f}, // LThigh1 (12)
        {0.43f, 0.70f}, // LThigh2 (13)
        {0.41f, 0.85f}, // LCalf (14)
        {0.40f, 0.95f}, // LFoot (15)
        {0.55f, 0.55f}, // RThigh1 (16)
        {0.57f, 0.70f}, // RThigh2 (17)
        {0.59f, 0.85f}, // RCalf (18)
        {0.60f, 0.95f}  // RFoot (19)
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
            float sizeY = (max.y - min.y) / 7.0f; // Taller corners for vertical box

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

    void Render(ImVec2 previewSize) {
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Draw preview background
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + previewSize.x, canvasPos.y + previewSize.y), ImColor(25, 25, 35, 255), 5.0f);
        drawList->AddRect(canvasPos, ImVec2(canvasPos.x + previewSize.x, canvasPos.y + previewSize.y), ImColor(50, 50, 60, 255), 5.0f);

        // Define a "padding" for the mannequin within the preview box
        float padding = 40.0f;
        ImVec2 drawAreaMin = {canvasPos.x + padding, canvasPos.y + padding + 20.0f};
        ImVec2 drawAreaMax = {canvasPos.x + previewSize.x - padding, canvasPos.y + previewSize.y - padding - 20.0f};
        float drawWidth = drawAreaMax.x - drawAreaMin.x;
        float drawHeight = drawAreaMax.y - drawAreaMin.y;

        // Map relative bone positions to preview area
        ImVec2 bones[20];
        for (int i = 0; i < 20; i++) {
            bones[i].x = drawAreaMin.x + (staticBones[i].x - 0.25f) * (drawWidth / 0.5f); // Centering and scaling
            bones[i].y = drawAreaMin.y + staticBones[i].y * drawHeight;
        }

        using namespace ESPSettings::Enemy;

        // Draw Skeleton
        if (bSkeletonEnabled) {
            float thick = skeletonThickness;
            ImColor col = skeletonColor;

            if (bBonesHead) ConnectBones(drawList, bones[0], bones[1], col, thick); // Head to Neck
            if (bBonesSpine) {
                ConnectBones(drawList, bones[1], bones[2], col, thick); // Neck to Spine
                ConnectBones(drawList, bones[2], bones[3], col, thick); // Spine to Pelvis
            }
            if (bBonesArmsL) {
                ConnectBones(drawList, bones[2], bones[4], col, thick); // Spine to LUpperArm
                ConnectBones(drawList, bones[4], bones[5], col, thick);
                ConnectBones(drawList, bones[5], bones[6], col, thick);
                ConnectBones(drawList, bones[6], bones[7], col, thick);
            }
            if (bBonesArmsR) {
                ConnectBones(drawList, bones[2], bones[8], col, thick); // Spine to RUpperArm
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
            drawList->AddCircleFilled(bones[0], headDotRadius, headDotColor);
        }

        // Calculate bounding box for the mannequin
        ImVec2 bMin = bones[0];
        ImVec2 bMax = bones[0];
        for (int i = 0; i < 20; i++) {
            bMin.x = ImMin(bMin.x, bones[i].x);
            bMin.y = ImMin(bMin.y, bones[i].y);
            bMax.x = ImMax(bMax.x, bones[i].x);
            bMax.y = ImMax(bMax.y, bones[i].y);
        }
        // Add slight padding to the box
        bMin.x -= 5.0f; bMin.y -= 5.0f;
        bMax.x += 5.0f; bMax.y += 5.0f;

        // Draw Box
        if (bBoxEnabled) {
            DrawBox(drawList, bMin, bMax, boxColor, boxThickness, boxStyle, bBoxFilled, boxFillColor);
        }

        // Draw Info Text
        float textYOffset = 0.0f;
        if (bNameEnabled) {
            const char* name = "Name";
            ImVec2 tSize = ImGui::CalcTextSize(name);
            drawList->AddText(ImVec2(bMin.x + (bMax.x - bMin.x) / 2.0f - tSize.x / 2.0f, bMin.y - 15.0f - textYOffset), nameColor, name);
            textYOffset += 15.0f;
        }
        if (bWeaponEnabled) {
            const char* weapon = "FN SCAR-L";
            ImVec2 tSize = ImGui::CalcTextSize(weapon);
            drawList->AddText(ImVec2(bMin.x + (bMax.x - bMin.x) / 2.0f - tSize.x / 2.0f, bMin.y - 15.0f - textYOffset), weaponColor, weapon);
        }

        if (bDistanceEnabled) {
            const char* dist = "500M";
            ImVec2 tSize = ImGui::CalcTextSize(dist);
            drawList->AddText(ImVec2(bMin.x + (bMax.x - bMin.x) / 2.0f - tSize.x / 2.0f, bMax.y + 5.0f), distanceColor, dist);
        }
        
        // Example health bar if needed (not in current plan but good for reference)
        if (bHealthEnabled) {
            drawList->AddRectFilled(ImVec2(bMax.x + 5.0f, bMin.y), ImVec2(bMax.x + 8.0f, bMax.y), ImColor(0, 0, 0, 100));
            drawList->AddRectFilled(ImVec2(bMax.x + 5.0f, bMin.y + (bMax.y - bMin.y) * 0.2f), ImVec2(bMax.x + 8.0f, bMax.y), ImColor(0, 255, 0, 255));
        }

        ImGui::Dummy(previewSize);
    }
}
