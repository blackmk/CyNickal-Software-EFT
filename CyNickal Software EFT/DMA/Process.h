#pragma once

#include "DMA/DMA.h"
#include "DebugMode.h"

namespace ConstStrings
{
	const std::string Game = "EscapeFromTarkov.exe";
	const std::string Unity = "UnityPlayer.dll";
	const std::string GameAssembly = "GameAssembly.dll";
}

class Process
{
private:
	DWORD m_PID = 0;
	std::unordered_map<std::string, uintptr_t> m_Modules;

public:
	bool GetProcessInfo(DMA_Connection* Conn, int maxRetries = 60);
	const uintptr_t GetBaseAddress() const;
	const uintptr_t GetUnityAddress() const;
	const uintptr_t GetAssemblyBase() const;
	const DWORD GetPID() const;
	const uintptr_t GetModuleAddress(const std::string& ModuleName);

private:
	bool PopulateModules(DMA_Connection* Conn, int maxRetries = 60);

public:
	template<typename T> inline T ReadMem(DMA_Connection* Conn, uintptr_t Address) const
	{
		if (DebugMode::IsDebugMode() || !Conn || !Conn->IsConnected())
			return T{};

		VMMDLL_SCATTER_HANDLE vmsh = VMMDLL_Scatter_Initialize(Conn->GetHandle(), m_PID, VMMDLL_FLAG_NOCACHE);
		DWORD BytesRead{ 0 };
		T Buffer{};

		VMMDLL_Scatter_PrepareEx(vmsh, Address, sizeof(T), reinterpret_cast<BYTE*>(&Buffer), &BytesRead);

		VMMDLL_Scatter_Execute(vmsh);

		VMMDLL_Scatter_CloseHandle(vmsh);

		if (BytesRead != sizeof(T))
			std::println("Incomplete read: {}/{}", BytesRead, sizeof(T));

		return Buffer;
	}
	template<typename T> inline std::vector<T> ReadVec(DMA_Connection* Conn, uintptr_t Address, size_t Num) const
	{
		if (DebugMode::IsDebugMode() || !Conn || !Conn->IsConnected())
			return std::vector<T>(Num);

		VMMDLL_SCATTER_HANDLE vmsh = VMMDLL_Scatter_Initialize(Conn->GetHandle(), m_PID, VMMDLL_FLAG_NOCACHE);
		DWORD BytesRead{ 0 };

		std::vector<T> Buffer(Num);

		VMMDLL_Scatter_PrepareEx(vmsh, Address, sizeof(T) * Num, reinterpret_cast<BYTE*>(Buffer.data()), &BytesRead);

		VMMDLL_Scatter_Execute(vmsh);

		VMMDLL_Scatter_CloseHandle(vmsh);

		if (BytesRead != sizeof(T) * Num)
			std::println("Incomplete read: {}/{}", BytesRead, sizeof(T));

		return Buffer;
	}
	inline uintptr_t ReadChain(DMA_Connection* Conn, uintptr_t Base, std::vector<std::ptrdiff_t> Offsets) const
	{
		if (DebugMode::IsDebugMode() || !Conn || !Conn->IsConnected())
			return 0;

		uintptr_t PreviousAddress = Base;
		for (auto& Offset : Offsets)
			PreviousAddress = ReadMem<uintptr_t>(Conn, PreviousAddress + Offset);

		return PreviousAddress;
	}
	inline bool ReadBuffer(DMA_Connection* Conn, uintptr_t Address, BYTE* Buffer, size_t Size) const
	{
		if (DebugMode::IsDebugMode() || !Conn || !Conn->IsConnected())
			return false;

		VMMDLL_SCATTER_HANDLE vmsh = VMMDLL_Scatter_Initialize(Conn->GetHandle(), m_PID, VMMDLL_FLAG_NOCACHE);
		DWORD BytesRead{ 0 };
		VMMDLL_Scatter_PrepareEx(vmsh, Address, static_cast<DWORD>(Size), Buffer, &BytesRead);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_CloseHandle(vmsh);

		return BytesRead == Size;
	}
};