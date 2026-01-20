#include "pch.h"
#include "GOM.h"
#include "Game/EFT.h"
#include <fstream>
#include <unordered_set>
#include "Game/Offsets/Offsets.h"
#include "Game/Classes/CLinkedListEntry.h"

bool GOM::Initialize(DMA_Connection* Conn)
{
	try
	{
		ResetCache();
		auto& Proc = EFT::GetProcess();

		uintptr_t pGOMAddress = Proc.GetUnityAddress() + Offsets::pGOM;
		GameObjectManagerAddress = Proc.ReadMem<uintptr_t>(Conn, pGOMAddress);
		if (!GameObjectManagerAddress)
		{
			std::println("[EFT] GOM Address not found at 0x{:X}", pGOMAddress);
			return false;
		}

		LastActiveNode = Proc.ReadMem<uintptr_t>(Conn, GameObjectManagerAddress + Offsets::CGameObjectManager::pLastActiveNode);
		ActiveNodes = Proc.ReadMem<uintptr_t>(Conn, GameObjectManagerAddress + Offsets::CGameObjectManager::pActiveNodes);

		if (!ActiveNodes || !LastActiveNode)
		{
			std::println("[EFT] GOM Nodes pointers are null!");
			return false;
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

	// Forward traversal
	uint32_t ForwardCount = 0;
	while (CurrentActiveNode != 0)
	{
		if (ForwardCount >= MaxNodes) break;
		if (VisitedNodes.contains(CurrentActiveNode)) break;

		CLinkedListEntry NodeEntry = Proc.ReadMem<CLinkedListEntry>(Conn, CurrentActiveNode);
		if (NodeEntry.pObject == 0) break;

		VisitedNodes.insert(CurrentActiveNode);
		m_ObjectAddresses.push_back(NodeEntry.pObject);

		if (NodeEntry.pNextEntry == FirstNode || NodeEntry.pNextEntry == 0)
			break;

		CurrentActiveNode = NodeEntry.pNextEntry;
		ForwardCount++;
	}

	// Backward traversal - start from ActiveNodes and go backwards
	uint32_t BackwardCount = 0;
	CurrentActiveNode = ActiveNodes;

	CLinkedListEntry StartEntry = Proc.ReadMem<CLinkedListEntry>(Conn, CurrentActiveNode);
	if (StartEntry.pPreviousEntry != 0 && StartEntry.pPreviousEntry != FirstNode)
	{
		CurrentActiveNode = StartEntry.pPreviousEntry;
		while (CurrentActiveNode != 0)
		{
			if (BackwardCount >= MaxNodes) break;
			if (VisitedNodes.contains(CurrentActiveNode)) break;

			CLinkedListEntry NodeEntry = Proc.ReadMem<CLinkedListEntry>(Conn, CurrentActiveNode);
			if (NodeEntry.pObject == 0) break;

			VisitedNodes.insert(CurrentActiveNode);
			m_ObjectAddresses.push_back(NodeEntry.pObject);

			if (NodeEntry.pPreviousEntry == FirstNode || NodeEntry.pPreviousEntry == 0)
				break;

			CurrentActiveNode = NodeEntry.pPreviousEntry;
			BackwardCount++;
		}
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