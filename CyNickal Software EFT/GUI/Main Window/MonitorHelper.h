#pragma once
#include <vector>
#include <string>
#include <Windows.h>

struct MonitorData {
    HMONITOR hMonitor;
    RECT rcMonitor;
    RECT rcWork;
    bool isPrimary;
    int index;
    std::string friendlyName;
};

class MonitorHelper {
public:
    static std::vector<MonitorData> GetAllMonitors();
    static void MoveWindowToMonitor(HWND hWnd, int monitorIndex);
};
