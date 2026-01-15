#include "pch.h"
#include "DebugMode.h"

namespace DebugMode
{
    std::atomic<bool> g_bDebugMode{ false };

    bool IsDebugMode()
    {
        return g_bDebugMode.load();
    }

    void SetDebugMode(bool value)
    {
        g_bDebugMode.store(value);
    }
}
