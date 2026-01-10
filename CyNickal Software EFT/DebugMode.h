#pragma once
#include <atomic>

namespace DebugMode
{
    extern std::atomic<bool> g_bDebugMode;
    bool IsDebugMode();
    void SetDebugMode(bool value);
}
