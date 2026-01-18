#include "pch.h"
#include "MonitorHelper.h"

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    auto* monitors = reinterpret_cast<std::vector<MonitorData>*>(dwData);
    MonitorData data;
    data.hMonitor = hMonitor;
    data.rcMonitor = *lprcMonitor;
    data.index = static_cast<int>(monitors->size());

    MONITORINFOEXA mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoA(hMonitor, &mi)) {
        data.rcWork = mi.rcWork;
        data.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        data.friendlyName = "Monitor " + std::to_string(data.index + 1);
        
        // Append resolution to name
        int width = abs(data.rcMonitor.right - data.rcMonitor.left);
        int height = abs(data.rcMonitor.bottom - data.rcMonitor.top);
        data.friendlyName += " (" + std::to_string(width) + "x" + std::to_string(height) + ")";
        if (data.isPrimary) data.friendlyName += " [Primary]";
    }

    monitors->push_back(data);
    return TRUE;
}

std::vector<MonitorData> MonitorHelper::GetAllMonitors() {
    std::vector<MonitorData> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

void MonitorHelper::MoveWindowToMonitor(HWND hWnd, int monitorIndex) {
    auto monitors = GetAllMonitors();
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) return;

    const auto& m = monitors[monitorIndex];
    int x = m.rcMonitor.left;
    int y = m.rcMonitor.top;
    int w = m.rcMonitor.right - m.rcMonitor.left;
    int h = m.rcMonitor.bottom - m.rcMonitor.top;

    // Restore if maximized
    ShowWindow(hWnd, SW_RESTORE);

    // Move to target monitor
    SetWindowPos(hWnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);

    // Maximize again
    ShowWindow(hWnd, SW_MAXIMIZE);
}
