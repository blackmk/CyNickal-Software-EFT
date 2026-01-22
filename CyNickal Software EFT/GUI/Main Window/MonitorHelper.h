#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
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
	static void RefreshMonitorCache();

private:
	static std::vector<MonitorData> EnumerateMonitorsInternal();

	static inline std::vector<MonitorData> s_CachedMonitors{};
	static inline std::mutex s_CacheMutex{};
	static inline std::chrono::steady_clock::time_point s_LastRefresh{};
	static constexpr auto CACHE_DURATION = std::chrono::seconds(5);
};
