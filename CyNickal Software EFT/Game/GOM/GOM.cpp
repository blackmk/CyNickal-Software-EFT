#include "pch.h"
#include "GOM.h"
#include "Game/EFT.h"
#include <cstdint>
#include <fstream>
#include <unordered_set>
#include <vector>
#include "Game/Offsets/Offsets.h"
#include "Game/Classes/CLinkedListEntry.h"

namespace
{
	constexpr const char* kGomSignature = "48 8B 35 ? ? ? ? 48 85 F6 0F 84 ? ? ? ? 8B 46";

	uint8_t HexNibble(char value)
	{
		if (value >= '0' && value <= '9')
			return static_cast<uint8_t>(value - '0');
		if (value >= 'a' && value <= 'f')
			return static_cast<uint8_t>(10 + value - 'a');
		if (value >= 'A' && value <= 'F')
			return static_cast<uint8_t>(10 + value - 'A');
		return 0;
	}

	uint8_t HexByte(const char* hex)
	{
		return static_cast<uint8_t>((HexNibble(hex[0]) << 4) | HexNibble(hex[1]));
	}

	void BuildPattern(const char* signature, std::vector<uint8_t>& bytes, std::vector<uint8_t>& mask)
	{
		bytes.clear();
		mask.clear();
		const char* cursor = signature;
		while (*cursor != '\0')
		{
			if (*cursor == ' ')
			{
				++cursor;
				continue;
			}
			if (*cursor == '?')
			{
				bytes.push_back(0);
				mask.push_back(0);
				if (cursor[1] == '?')
					++cursor;
				++cursor;
				continue;
			}
			bytes.push_back(HexByte(cursor));
			mask.push_back(1);
			cursor += 2;
		}
	}

	uintptr_t FindSignature(DMA_Connection* Conn, const Process& Proc, uintptr_t rangeStart, uintptr_t rangeEnd, const char* signature)
	{
		if (!Conn || !Conn->IsConnected() || rangeStart >= rangeEnd)
			return 0;

		auto rangeSize = rangeEnd - rangeStart;
		if (rangeSize > static_cast<size_t>(static_cast<DWORD>(-1)))
			return 0;

		std::vector<uint8_t> buffer(rangeSize);
		DWORD bytesRead = 0;
		if (!VMMDLL_MemReadEx(Conn->GetHandle(), Proc.GetPID(), rangeStart, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, VMMDLL_FLAG_NOCACHE))
			return 0;
		if (bytesRead == 0)
			return 0;

		std::vector<uint8_t> pattern;
		std::vector<uint8_t> mask;
		BuildPattern(signature, pattern, mask);
		if (pattern.empty() || pattern.size() != mask.size() || bytesRead < pattern.size())
			return 0;

		const size_t scanSize = static_cast<size_t>(bytesRead);
		for (size_t i = 0; i + pattern.size() <= scanSize; ++i)
		{
			bool match = true;
			for (size_t j = 0; j < pattern.size(); ++j)
			{
				if (mask[j] && buffer[i + j] != pattern[j])
				{
					match = false;
					break;
				}
			}
			if (match)
				return rangeStart + i;
		}

		return 0;
	}

	bool TryGetModuleRange(DMA_Connection* Conn, const Process& Proc, const std::string& moduleName, uintptr_t& base, size_t& size)
	{
		base = 0;
		size = 0;

		PVMMDLL_MAP_MODULEENTRY moduleEntry = nullptr;
		if (!VMMDLL_Map_GetModuleFromNameU(Conn->GetHandle(), Proc.GetPID(), moduleName.c_str(), &moduleEntry, 0))
			return false;

		base = static_cast<uintptr_t>(moduleEntry->vaBase);
		size = static_cast<size_t>(moduleEntry->cbImageSize);
		VMMDLL_MemFree(moduleEntry);

		return base != 0 && size != 0;
	}

	bool TryReadPointer(const Process& Proc, DMA_Connection* Conn, uintptr_t address, uintptr_t& value)
	{
		value = 0;
		if (!address)
			return false;

		return Proc.ReadBuffer(Conn, address, reinterpret_cast<BYTE*>(&value), sizeof(value));
	}

	bool TryReadLinkedListEntry(const Process& Proc, DMA_Connection* Conn, uintptr_t address, CLinkedListEntry& entry)
	{
		return address != 0 && Proc.ReadBuffer(Conn, address, reinterpret_cast<BYTE*>(&entry), sizeof(entry));
	}

	bool TryLoadGomFromPointerAddress(
		DMA_Connection* Conn,
		const Process& Proc,
		uintptr_t gomPointerAddress,
		uintptr_t& gomAddress,
		uintptr_t& activeNodes,
		uintptr_t& lastActiveNode)
	{
		gomAddress = 0;
		activeNodes = 0;
		lastActiveNode = 0;
		if (!TryReadPointer(Proc, Conn, gomPointerAddress, gomAddress) || !gomAddress)
			return false;

		if (!TryReadPointer(Proc, Conn, gomAddress + Offsets::CGameObjectManager::pLastActiveNode, lastActiveNode))
			return false;
		if (!TryReadPointer(Proc, Conn, gomAddress + Offsets::CGameObjectManager::pActiveNodes, activeNodes))
			return false;
		if (!activeNodes || !lastActiveNode)
			return false;

		CLinkedListEntry activeEntry{};
		CLinkedListEntry lastEntry{};
		if (!TryReadLinkedListEntry(Proc, Conn, activeNodes, activeEntry))
			return false;
		if (!TryReadLinkedListEntry(Proc, Conn, lastActiveNode, lastEntry))
			return false;
		if (!activeEntry.pObject || !lastEntry.pObject)
			return false;

		return true;
	}

	uintptr_t FindGomPointerAddressFromSignature(DMA_Connection* Conn, const Process& Proc)
	{
		uintptr_t unityBase = 0;
		size_t unitySize = 0;
		if (!TryGetModuleRange(Conn, Proc, ConstStrings::Unity, unityBase, unitySize))
			return 0;

		auto signatureAddress = FindSignature(Conn, Proc, unityBase, unityBase + unitySize, kGomSignature);
		if (!signatureAddress)
			return 0;

		auto rva = Proc.ReadMem<int32_t>(Conn, signatureAddress + 3);
		return static_cast<uintptr_t>(signatureAddress + 7 + rva);
	}
}

bool GOM::Initialize(DMA_Connection* Conn)
{
	try
	{
		ResetCache();
		auto& Proc = EFT::GetProcess();

		static DWORD s_GomCachePid = 0;
		static uintptr_t s_GomPointerAddress = 0;
		static bool s_GomPointerFromSignature = false;
		auto TryLoadGom = [&](uintptr_t gomPointerAddress) -> bool
		{
			uintptr_t gomAddress = 0;
			uintptr_t activeNodes = 0;
			uintptr_t lastActiveNode = 0;
			if (!TryLoadGomFromPointerAddress(Conn, Proc, gomPointerAddress, gomAddress, activeNodes, lastActiveNode))
				return false;
			GameObjectManagerAddress = gomAddress;
			ActiveNodes = activeNodes;
			LastActiveNode = lastActiveNode;
			return true;
		};

		bool gomReady = false;
		if (s_GomCachePid == Proc.GetPID() && s_GomPointerAddress)
		{
			gomReady = TryLoadGom(s_GomPointerAddress);
			if (!gomReady)
				s_GomPointerAddress = 0;
		}

		if (!gomReady)
		{
			uintptr_t gomPointerAddress = Proc.GetUnityAddress() + Offsets::pGOM;
			gomReady = TryLoadGom(gomPointerAddress);
			if (gomReady)
			{
				s_GomCachePid = Proc.GetPID();
				s_GomPointerAddress = gomPointerAddress;
				s_GomPointerFromSignature = false;
			}
		}

		if (!gomReady)
		{
			std::println("[EFT] GOM read failed at hardcoded offset. Trying signature scan.");
			auto signaturePointerAddress = FindGomPointerAddressFromSignature(Conn, Proc);
			if (!signaturePointerAddress)
			{
				std::println("[EFT] GOM signature scan failed.");
				return false;
			}
			gomReady = TryLoadGom(signaturePointerAddress);
			if (!gomReady)
			{
				std::println("[EFT] GOM signature pointer invalid at 0x{:X}", signaturePointerAddress);
				return false;
			}
			s_GomCachePid = Proc.GetPID();
			s_GomPointerAddress = signaturePointerAddress;
			s_GomPointerFromSignature = true;
			std::println("[EFT] GOM located via signature at 0x{:X}", signaturePointerAddress);
		}
		else if (s_GomPointerFromSignature)
		{
			std::println("[EFT] GOM using cached signature address 0x{:X}", s_GomPointerAddress);
		}

		GetObjectAddresses(Conn, 40000);
		if (m_ObjectAddresses.empty())
		{
			std::println("[EFT] GOM scan returned no objects");
			return false;
		}

		PopulateObjectInfoListFromAddresses(Conn);

		return true;
	}
	catch (const std::exception& ex)
	{
		std::println("[EFT] GOM Initialize error: {}", ex.what());
		ResetCache();
		return false;
	}
	catch (...)
	{
		ResetCache();
		return false;
	}
}

void GOM::ResetCache()
{
	GameObjectManagerAddress = 0;
	LastActiveNode = 0;
	ActiveNodes = 0;
	m_MainPlayerAddress = 0;
	m_ObjectAddresses.clear();
	m_ObjectInfo.clear();
}


void GOM::GetObjectAddresses(DMA_Connection* Conn, uint32_t MaxNodes)
{
	auto& Proc = EFT::GetProcess();

	m_ObjectAddresses.clear();

	auto StartTime = std::chrono::high_resolution_clock::now();
	uint32_t NodeCount = 0;
	uintptr_t CurrentActiveNode = ActiveNodes;
	uintptr_t FirstNode = ActiveNodes;

	// Track visited nodes to avoid duplicates when doing bidirectional traversal
	std::unordered_set<uintptr_t> VisitedNodes;
	bool loggedInvalidNode = false;
	auto LogNodeFailure = [&](const char* direction, uintptr_t nodeAddress)
	{
		if (!loggedInvalidNode)
		{
			std::println("[EFT] GOM {} traversal failed at node 0x{:X}", direction, nodeAddress);
			loggedInvalidNode = true;
		}
	};

	// Forward traversal
	uint32_t ForwardCount = 0;
	while (CurrentActiveNode != 0)
	{
		if (ForwardCount >= MaxNodes) break;
		if (VisitedNodes.contains(CurrentActiveNode)) break;

		CLinkedListEntry NodeEntry{};
		if (!TryReadLinkedListEntry(Proc, Conn, CurrentActiveNode, NodeEntry))
		{
			LogNodeFailure("forward", CurrentActiveNode);
			break;
		}
		if (NodeEntry.pObject == 0) break;

		VisitedNodes.insert(CurrentActiveNode);
		m_ObjectAddresses.push_back(NodeEntry.pObject);

		if (CurrentActiveNode == LastActiveNode || NodeEntry.pNextEntry == FirstNode || NodeEntry.pNextEntry == 0)
			break;

		CurrentActiveNode = NodeEntry.pNextEntry;
		ForwardCount++;
	}

	// Backward traversal - start from LastActiveNode and go backwards
	uint32_t BackwardCount = 0;
	CurrentActiveNode = LastActiveNode;
	while (CurrentActiveNode != 0)
	{
		if (BackwardCount >= MaxNodes) break;
		if (VisitedNodes.contains(CurrentActiveNode)) break;

		CLinkedListEntry NodeEntry{};
		if (!TryReadLinkedListEntry(Proc, Conn, CurrentActiveNode, NodeEntry))
		{
			LogNodeFailure("backward", CurrentActiveNode);
			break;
		}
		if (NodeEntry.pObject == 0) break;

		VisitedNodes.insert(CurrentActiveNode);
		m_ObjectAddresses.push_back(NodeEntry.pObject);

		if (CurrentActiveNode == FirstNode || NodeEntry.pPreviousEntry == 0)
			break;

		CurrentActiveNode = NodeEntry.pPreviousEntry;
		BackwardCount++;
	}

	NodeCount = (uint32_t)m_ObjectAddresses.size();
}

std::vector<uintptr_t> GOM::GetGameWorldAddresses(DMA_Connection* Conn)
{
	std::vector<uintptr_t> GameWorldAddresses{};

	for (auto& ObjInfo : m_ObjectInfo)
	{
		if (ObjInfo.m_ObjectName == "GameWorld")
		{
			// std::println("[EFT] GameWorld Address: 0x{:X}", ObjInfo.m_ObjectAddress);
			GameWorldAddresses.push_back(ObjInfo.m_ObjectAddress);
		}
	}

	return GameWorldAddresses;
}

uintptr_t GOM::FindGameWorldAddressFromCache(DMA_Connection* Conn)
{
	auto GameWorldAddrs = GetGameWorldAddresses(Conn);
	if (GameWorldAddrs.empty())
	{
		throw std::runtime_error("No 'GameWorld' objects found in GOM scan.");
	}

	auto& Proc = EFT::GetProcess();

	for (auto& GameWorldAddr : GameWorldAddrs)
	{
		auto Deref1 = Proc.ReadMem<uintptr_t>(Conn, GameWorldAddr + Offsets::CGameObject::pComponents);
		if (!Deref1) continue;

		auto Deref2 = Proc.ReadMem<uintptr_t>(Conn, Deref1 + 0x18);
		if (!Deref2) continue;

		auto LocalWorldAddr = Proc.ReadMem<uintptr_t>(Conn, Deref2 + Offsets::CComponent::pObjectClass);
		if (!LocalWorldAddr) continue;

		auto MainPlayerAddr = Proc.ReadMem<uintptr_t>(Conn, LocalWorldAddr + Offsets::CLocalGameWorld::pMainPlayer);

		if (MainPlayerAddr)
		{
			m_MainPlayerAddress = MainPlayerAddr;
			return LocalWorldAddr;
		}
	}

	throw std::runtime_error("Found 'GameWorld' object(s), but none have a valid MainPlayer (Player not in raid yet?)");
}

void GOM::DumpAllObjectsToFile(const std::string& FileName)
{
	std::ofstream OutFile(FileName, std::ios::out | std::ios::trunc);
	if (!OutFile.is_open())
	{
		std::println("[EFT] DumpAllObjectsToFile; Failed to open file: {}", FileName);
		return;
	}

	for (int i = 0; i < m_ObjectInfo.size(); i++)
	{
		auto& ObjInfo = m_ObjectInfo[i];
		OutFile << std::format("Entity #{0:d} @ {1:X} named `{2:s}`", i, ObjInfo.m_ObjectAddress, ObjInfo.m_ObjectName.c_str()) << std::endl;
	}

	OutFile.close();
}

std::vector<std::pair<uintptr_t, std::array<std::byte, 32>>> ObjectDataBuffers{};
void GOM::PopulateObjectInfoListFromAddresses(DMA_Connection* Conn)
{
	auto& Proc = EFT::GetProcess();

	ObjectDataBuffers.resize(m_ObjectAddresses.size());

	auto vmsh = VMMDLL_Scatter_Initialize(Conn->GetHandle(), Proc.GetPID(), VMMDLL_FLAG_NOCACHE);

	for (int i = 0; i < m_ObjectAddresses.size(); i++)
	{
		auto& ObjAddr = m_ObjectAddresses[i];
		uintptr_t NameAddress = ObjAddr + Offsets::CGameObject::pName;
		VMMDLL_Scatter_PrepareEx(vmsh, NameAddress, sizeof(uintptr_t), reinterpret_cast<BYTE*>(&ObjectDataBuffers[i].first), nullptr);
	}

	VMMDLL_Scatter_Execute(vmsh);

	VMMDLL_Scatter_Clear(vmsh, Proc.GetPID(), VMMDLL_FLAG_NOCACHE);

	for (int i = 0; i < m_ObjectAddresses.size(); i++)
	{
		auto& [NameAddress, DataBuffer] = ObjectDataBuffers[i];
		VMMDLL_Scatter_PrepareEx(vmsh, NameAddress, DataBuffer.size(), reinterpret_cast<BYTE*>(DataBuffer.data()), nullptr);
	}

	VMMDLL_Scatter_Execute(vmsh);

	VMMDLL_Scatter_CloseHandle(vmsh);

	m_ObjectInfo.clear();
	for (int i = 0; i < m_ObjectAddresses.size(); i++)
	{
		auto& [NameAddress, DataBuffer] = ObjectDataBuffers[i];
		std::string Name(reinterpret_cast<char*>(DataBuffer.data()), strnlen_s(reinterpret_cast<char*>(DataBuffer.data()), DataBuffer.size()));
		CObjectInfo ObjInfo{};
		ObjInfo.m_ObjectAddress = m_ObjectAddresses[i];
		ObjInfo.m_ObjectName = Name;
		m_ObjectInfo.push_back(ObjInfo);
	}
}
