#include "pch.h"

#include "GUI/Main Window/Main Window.h"
#include "GUI/Config/Config.h"
#include "DMA/DMA Thread.h"
#include "Makcu/MyMakcu.h"
#include "Database/Database.h"
#include "DebugMode.h"
#include "GUI/KmboxSettings/KmboxSettings.h"
#include "GUI/Fuser/Fuser.h"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <ctime>
#include <print>
#include <atomic>
#include <windows.h>
#include <vector>
#include <sstream>
#include <string>
#include <excpt.h>

std::atomic<bool> bRunning{ true };
static std::ofstream g_LogFile;

// Store original stream buffers for proper cleanup
static std::streambuf* g_OrigCout = nullptr;
static std::streambuf* g_OrigCerr = nullptr;
static std::streambuf* g_OrigClog = nullptr;

static bool TryOpenLogAt(const std::filesystem::path& path)
{
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	g_LogFile.open(path, std::ios::app);
	if (g_LogFile.is_open())
	{
		OutputDebugStringA(("Log opened: " + path.string() + "\n").c_str());
		return true;
	}
	OutputDebugStringA(("Log open failed: " + path.string() + "\n").c_str());
	return false;
}

static void LogLine(const char* msg)
{
	try
	{
		if (!msg) return;
		// Mirror to debugger
		OutputDebugStringA(msg);
		OutputDebugStringA("\n");
		if (!g_LogFile.is_open()) return;
		auto now = std::chrono::system_clock::now();
		auto t = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
		localtime_s(&tm, &t);
		char buf[32]{};
		std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
		g_LogFile << buf << " " << msg << "\n";
		g_LogFile.flush();
	}
	catch (...)
	{
	}
}

static void InitLogging()
{
	try
	{
		char exePath[MAX_PATH]{};
		GetModuleFileNameA(nullptr, exePath, MAX_PATH);
		std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
		char tempPathBuf[MAX_PATH]{};
		GetTempPathA(MAX_PATH, tempPathBuf);
		std::filesystem::path tempDir = std::filesystem::path(tempPathBuf) / "EFT_DMA";

		std::vector<std::filesystem::path> candidates{
			exeDir / "Logs" / "ui.log",
			std::filesystem::current_path() / "Logs" / "ui.log",
			tempDir / "Logs" / "ui.log"
		};

		for (const auto& p : candidates)
		{
			if (TryOpenLogAt(p))
				break;
		}

		if (g_LogFile.is_open())
		{
			auto now = std::chrono::system_clock::now();
			auto t = std::chrono::system_clock::to_time_t(now);
			std::tm tm{};
			localtime_s(&tm, &t);
			char buf[32]{};
			std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
			g_LogFile << "--- START " << buf << " ---\n";
			g_LogFile.flush();
			// Save original stream buffers before redirecting
			g_OrigCout = std::cout.rdbuf(g_LogFile.rdbuf());
			g_OrigCerr = std::cerr.rdbuf(g_LogFile.rdbuf());
			g_OrigClog = std::clog.rdbuf(g_LogFile.rdbuf());
		}
	}
	catch (...)
	{
	}
}

static LONG WINAPI VectoredHandler(_In_ PEXCEPTION_POINTERS pInfo)
{
	if (pInfo && pInfo->ExceptionRecord)
	{
		std::ostringstream oss;
		oss << "SEH 0x" << std::hex << pInfo->ExceptionRecord->ExceptionCode
			<< " at 0x" << reinterpret_cast<uintptr_t>(pInfo->ExceptionRecord->ExceptionAddress)
			<< " thread " << GetCurrentThreadId();
		LogLine(oss.str().c_str());
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI UnhandledFilter(_In_ struct _EXCEPTION_POINTERS* pInfo)
{
	if (pInfo && pInfo->ExceptionRecord)
	{
		std::ostringstream oss;
		oss << "UNHANDLED 0x" << std::hex << pInfo->ExceptionRecord->ExceptionCode
			<< " at 0x" << reinterpret_cast<uintptr_t>(pInfo->ExceptionRecord->ExceptionAddress)
			<< " thread " << GetCurrentThreadId();
		LogLine(oss.str().c_str());
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

static int LogSEH(const char* prefix, EXCEPTION_POINTERS* pInfo)
{
	if (pInfo && pInfo->ExceptionRecord)
	{
		std::ostringstream oss;
		oss << prefix << " 0x" << std::hex << pInfo->ExceptionRecord->ExceptionCode
			<< " at 0x" << reinterpret_cast<uintptr_t>(pInfo->ExceptionRecord->ExceptionAddress)
			<< " thread " << GetCurrentThreadId();
		LogLine(oss.str().c_str());
	}
	else
	{
		LogLine("SEH: missing exception info");
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

static bool g_EnableVectored{ false };

static bool InstallVectoredHandler()
{
	if (!g_EnableVectored)
	{
		LogLine("Vectored handler disabled");
		return true;
	}

	PVOID handle{ nullptr };
	__try
	{
		handle = AddVectoredExceptionHandler(1, VectoredHandler);
	}
	__except (LogSEH("VECTORED INSTALL SEH", GetExceptionInformation()))
	{
		LogLine("Vectored handler install SEH raised");
		return false;
	}

	if (!handle)
	{
		LogLine("AddVectoredExceptionHandler returned null");
		return false;
	}

	LogLine("POST VectoredHandler");
	return true;
}

static void SetTerminateHandler()
{
	std::set_terminate([]()
	{
		std::exception_ptr eptr = std::current_exception();
		if (eptr)
		{
			try
			{
				std::rethrow_exception(eptr);
			}
			catch (const std::exception& e)
			{
				std::string msg = std::string("TERMINATE: ") + e.what();
				LogLine(msg.c_str());
			}
			catch (...)
			{
				LogLine("TERMINATE: unknown exception");
			}
		}
		else
		{
			LogLine("TERMINATE: no active exception");
		}
		std::abort();
	});
}

static void RunDMAThreadBody()
{
	try
	{
		DMA_Thread_Main();
	}
	catch (const std::exception& e)
	{
		std::string msg = std::string("DMA thread exception: ") + e.what();
		LogLine(msg.c_str());
	}
	catch (...)
	{
		LogLine("DMA thread exception: unknown");
	}
}

static int RunApp()
{
	LogLine("MAIN start");

	LogLine("INIT Database");
	Database::Initialize();

	LogLine("INIT Config");
	try
	{
		Config::LoadConfig("default");
	}
	catch (const std::exception& e)
	{
		std::string msg = std::string("Config load failed: ") + e.what();
		LogLine(msg.c_str());
	}
	catch (...)
	{
		LogLine("Config load failed: unknown exception");
	}
	LogLine("INIT Fuser monitor settings");
	Fuser::Initialize();
	KmboxSettings::TryAutoConnect();

	LogLine("INIT Makcu");
	MyMakcu::Initialize();

	LogLine("INSTALL handlers");
	bool vectoredOk = InstallVectoredHandler();
	if (!vectoredOk)
	{
		LogLine("Vectored handler install failed");
	}
	LogLine("SET TerminateHandler");
	SetTerminateHandler();

	LogLine("SET UnhandledFilter");
	SetUnhandledExceptionFilter(UnhandledFilter);

	LogLine("INIT DMA get instance");
	auto Conn = DMA_Connection::GetInstance();
	if (!Conn->IsConnected())
	{
		LogLine("DMA not connected, prompt user");
		int result = MessageBoxA(NULL,
			"DMA connection failed.\n\nWould you like to start in Debug Mode?\n(Settings only, no memory read/write)",
			"Connection Failed",
			MB_YESNO | MB_ICONQUESTION);

		if (result == IDYES)
		{
			DebugMode::SetDebugMode(true);
			LogLine("Debug Mode enabled by user");
		}
		else
		{
			LogLine("User declined Debug Mode; exiting");
			return 0;
		}
	}

	LogLine("START DMA thread");
	auto DMALaunch = []()
	{
		LogLine("DMA thread start");
		__try
		{
			RunDMAThreadBody();
		}
		__except (LogSEH("DMA thread SEH", GetExceptionInformation()))
		{
		}
		LogLine("DMA thread exit");
	};
	std::thread DMAThread(DMALaunch);

#ifndef DLL_FORM
	LogLine("INIT MainWindow");
	MainWindow::Initialize();
#endif

	LogLine("ENTER frame loop");
	unsigned int frameCounter = 0;
	while (bRunning)
	{
		if (GetAsyncKeyState(VK_END) & 1) bRunning = false;

#ifndef DLL_FORM
		try
		{
			if (!MainWindow::OnFrame())
			{
				LogLine("Window closed, exiting");
				bRunning = false;
			}
		}
		catch (const std::exception& e)
		{
			std::string msg = std::string("FRAME EXCEPTION (C++): ") + e.what();
			LogLine(msg.c_str());
			break;
		}
		catch (...)
		{
			LogLine("FRAME EXCEPTION (C++ unknown)");
			break;
		}
#else 
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
		if ((++frameCounter % 500) == 0)
		{
			LogLine("FRAME HEARTBEAT");
		}
	}

	LogLine("JOIN DMA thread");
	DMAThread.join();

#ifndef DLL_FORM
	LogLine("CLEANUP MainWindow");
	MainWindow::Cleanup();
#endif

	LogLine("EXIT");
	return 0;
}

int main()
{
	InitLogging();
	LogLine("POST InitLogging");
	int returnCode = 0;

	__try
	{
		returnCode = RunApp();
	}
	__except (LogSEH("MAIN SEH", GetExceptionInformation()))
	{
		returnCode = -1;
	}

	return returnCode;
}
