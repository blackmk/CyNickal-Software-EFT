#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <array>
#include "ESPSettings.h"

namespace ESPPreview {
    void Render(ImVec2 size);
    
    // Internal helper structures
    struct PreviewBone {
        ImVec2 pos;
        bool bIsOnScreen = true;
    };
}
