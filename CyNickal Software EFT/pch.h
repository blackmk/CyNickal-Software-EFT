#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Prevent Winsock 1.1 inclusion
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

// Enable Winsock 2.0 explicitly if needed elsewhere, 
// but for now we block headers to avoid conflicts.

#include <print>
#include <array>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <utility>
#include <mutex>
#include <variant>
#include <bitset>
#include <expected>
#include <ranges>
#include <algorithm>
#include <fstream>
#include <numbers>
#include <filesystem>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

// Define NTSTATUS properly to prevent vmmdll.h from doing it incorrectly
#ifndef _NTSTATUS_
#define _NTSTATUS_
typedef long NTSTATUS;
#endif

#include "vmmdll.h"
#pragma comment(lib, "leechcore.lib")
#pragma comment(lib, "vmm.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"