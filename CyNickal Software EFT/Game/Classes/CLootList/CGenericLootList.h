#pragma once
#include "Game/EFT.h"

class EFT;
template <typename T>
class CGenericLootList
{
public:
	std::mutex m_Mut{};
	std::vector<typename T> m_Entities{};
	std::vector<uintptr_t> m_EntityAddresses{};

public:
	void CompleteUpdate(DMA_Connection* Conn)
	{
		std::scoped_lock lk(m_Mut);
		m_Entities.clear();

		for (auto& Addr : m_EntityAddresses)
			m_Entities.emplace_back(T(Addr));

		ExecuteFullReads(Conn);
	}
	void ExecuteFullReads(DMA_Connection* Conn)
	{
		auto& Proc = EFT::GetProcess();
		auto ProcID = Proc.GetPID();

		auto vmsh = VMMDLL_Scatter_Initialize(Conn->GetHandle(), ProcID, VMMDLL_FLAG_NOCACHE);
		for (auto& Ent : m_Entities)
			Ent.PrepareRead_1(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_Clear(vmsh, ProcID, VMMDLL_FLAG_NOCACHE);

		for (auto& Ent : m_Entities)
			Ent.PrepareRead_2(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_Clear(vmsh, ProcID, VMMDLL_FLAG_NOCACHE);

		for (auto& Ent : m_Entities)
			Ent.PrepareRead_3(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_Clear(vmsh, ProcID, VMMDLL_FLAG_NOCACHE);

		for (auto& Ent : m_Entities)
			Ent.PrepareRead_4(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_Clear(vmsh, ProcID, VMMDLL_FLAG_NOCACHE);

		for (auto& Ent : m_Entities)
			Ent.PrepareRead_5(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_Clear(vmsh, ProcID, VMMDLL_FLAG_NOCACHE);

		for (auto& Ent : m_Entities)
			Ent.PrepareRead_6(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_Clear(vmsh, ProcID, VMMDLL_FLAG_NOCACHE);

		for (auto& Ent : m_Entities)
			Ent.PrepareRead_7(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_Clear(vmsh, ProcID, VMMDLL_FLAG_NOCACHE);

		for (auto& Ent : m_Entities)
			Ent.PrepareRead_8(vmsh);
		VMMDLL_Scatter_Execute(vmsh);
		VMMDLL_Scatter_CloseHandle(vmsh);

		for (auto& Ent : m_Entities)
			Ent.Finalize();
	}
};