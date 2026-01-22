#include "pch.h"
#include "CFirearmManager.h"
#include "Game/Offsets/Offsets.h"
#include "Game/EFT.h"
#include <cmath>
#include <chrono>

CFirearmManager::CFirearmManager(uintptr_t HandsControllerAddress)
	: m_HandsControllerAddress(HandsControllerAddress)
{
	// Initialize with default ballistics (e.g. 5.45 BP) until read
	m_CachedBallistics.BulletSpeed = 850.f;
	m_CachedBallistics.BulletMassGrams = 3.4f;
	m_CachedBallistics.BulletDiameterMillimeters = 5.45f;
	m_CachedBallistics.BallisticCoefficient = 0.32f;
}

void CFirearmManager::Update(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (!m_HandsControllerAddress)
	{
		Reset();
		return;
	}

	// Read fireport address synchronously to avoid scatter pointer lifetime issues
	auto Conn = DMA_Connection::GetInstance();
	auto PID = EFT::GetProcess().GetPID();
	
	uintptr_t tempAddress = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, m_HandsControllerAddress + Offsets::CFirearmController::Fireport,
		reinterpret_cast<PBYTE>(&tempAddress), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
	
	if (tempAddress != 0)
		m_FireportAddress = tempAddress;

	UpdateBallistics();
}

void CFirearmManager::UpdateBallistics()
{
	// Limit update rate (e.g. once every 2 seconds)
	auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if (m_LastBallisticsUpdate != 0 && (now - m_LastBallisticsUpdate < 2000))
		return;

	m_LastBallisticsUpdate = now;

	if (!m_HandsControllerAddress) return;

	auto Conn = DMA_Connection::GetInstance();
	auto PID = EFT::GetProcess().GetPID();

	// 1. Get Weapon Item
	uintptr_t weaponItem = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, m_HandsControllerAddress + Offsets::CHandsController::pItem,
		reinterpret_cast<PBYTE>(&weaponItem), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (!weaponItem) return;

	// 2. Try to find Ammo Template
	uintptr_t ammoTemplate = 0;

	// Check Chambers
	uintptr_t chambersPtr = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, weaponItem + Offsets::CLootItemWeapon::Chambers,
		reinterpret_cast<PBYTE>(&chambersPtr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (chambersPtr)
	{
		int count = 0;
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, chambersPtr + Offsets::CUnityList::Count,
			reinterpret_cast<PBYTE>(&count), sizeof(int), nullptr, VMMDLL_FLAG_NOCACHE);

		if (count > 0)
		{
			// Read array
			uintptr_t arrayBase = chambersPtr + 0x20; // Unity Array items start at 0x20

			for (int i = 0; i < count; i++)
			{
				uintptr_t slotPtr = 0;
				VMMDLL_MemReadEx(Conn->GetHandle(), PID, arrayBase + i * sizeof(uintptr_t),
					reinterpret_cast<PBYTE>(&slotPtr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

				if (slotPtr)
				{
					uintptr_t containedItem = 0;
					VMMDLL_MemReadEx(Conn->GetHandle(), PID, slotPtr + Offsets::CSlot::pContainedItem,
						reinterpret_cast<PBYTE>(&containedItem), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

					if (containedItem)
					{
						VMMDLL_MemReadEx(Conn->GetHandle(), PID, containedItem + Offsets::CItem::pTemplate,
							reinterpret_cast<PBYTE>(&ammoTemplate), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

						if (ammoTemplate) break; // Found round in chamber
					}
				}
			}
		}
	}

	// Check Magazine if no round in chamber
	if (!ammoTemplate)
	{
		uintptr_t magSlot = 0;
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, weaponItem + Offsets::CLootItemWeapon::MagSlotCache,
			reinterpret_cast<PBYTE>(&magSlot), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

		if (magSlot)
		{
			uintptr_t magItem = 0;
			VMMDLL_MemReadEx(Conn->GetHandle(), PID, magSlot + Offsets::CSlot::pContainedItem,
				reinterpret_cast<PBYTE>(&magItem), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

			if (magItem)
			{
				// Check Cartridges (StackSlot)
				uintptr_t cartridges = 0;
				VMMDLL_MemReadEx(Conn->GetHandle(), PID, magItem + Offsets::CMagazineItem::Cartridges,
					reinterpret_cast<PBYTE>(&cartridges), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE); 

				if (cartridges) 
				{
					// StackSlot has _items (List)
					uintptr_t itemsList = 0;
					VMMDLL_MemReadEx(Conn->GetHandle(), PID, cartridges + Offsets::CStackSlot::pItems,
						reinterpret_cast<PBYTE>(&itemsList), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

					if (itemsList)
					{
						// Unity List: 0x10 -> Array, 0x18 -> Count
						int count = 0;
						VMMDLL_MemReadEx(Conn->GetHandle(), PID, itemsList + Offsets::CUnityList::Count,
							reinterpret_cast<PBYTE>(&count), sizeof(int), nullptr, VMMDLL_FLAG_NOCACHE);

						if (count > 0)
						{
							uintptr_t arrayPtr = 0;
							VMMDLL_MemReadEx(Conn->GetHandle(), PID, itemsList + Offsets::CUnityList::ArrayBase,
								reinterpret_cast<PBYTE>(&arrayPtr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

							if (arrayPtr)
							{
								uintptr_t firstStack = 0;
								VMMDLL_MemReadEx(Conn->GetHandle(), PID, arrayPtr + 0x20, // First item
									reinterpret_cast<PBYTE>(&firstStack), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

								if (firstStack)
								{
									VMMDLL_MemReadEx(Conn->GetHandle(), PID, firstStack + Offsets::CItem::pTemplate,
										reinterpret_cast<PBYTE>(&ammoTemplate), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
								}
							}
						}
					}
				}
			}
		}
	}

	// If we found a template, read ballistics
	if (ammoTemplate && ammoTemplate != m_LastAmmoTemplate)
	{
		m_LastAmmoTemplate = ammoTemplate;

		float mass = 0, diam = 0, bc = 0, speed = 0;

		VMMDLL_MemReadEx(Conn->GetHandle(), PID, ammoTemplate + Offsets::CAmmoTemplate::BulletMassGram,
			reinterpret_cast<PBYTE>(&mass), sizeof(float), nullptr, VMMDLL_FLAG_NOCACHE);
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, ammoTemplate + Offsets::CAmmoTemplate::BulletDiameterMillimeters,
			reinterpret_cast<PBYTE>(&diam), sizeof(float), nullptr, VMMDLL_FLAG_NOCACHE);
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, ammoTemplate + Offsets::CAmmoTemplate::BallisticCoefficient,
			reinterpret_cast<PBYTE>(&bc), sizeof(float), nullptr, VMMDLL_FLAG_NOCACHE);
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, ammoTemplate + Offsets::CAmmoTemplate::InitialSpeed,
			reinterpret_cast<PBYTE>(&speed), sizeof(float), nullptr, VMMDLL_FLAG_NOCACHE);

		if (mass > 0 && speed > 0)
		{
			m_CachedBallistics.BulletMassGrams = mass;
			m_CachedBallistics.BulletDiameterMillimeters = diam;
			m_CachedBallistics.BallisticCoefficient = bc;
			m_CachedBallistics.BulletSpeed = speed;

			std::println("[Firearm] Updated Ballistics: Speed={:.1f} Mass={:.1f} BC={:.3f}", speed, mass, bc);
		}
	}
}

void CFirearmManager::QuickUpdate(VMMDLL_SCATTER_HANDLE vmsh)
{
	if (!m_FireportAddress)
	{
		Reset();
		return;
	}

	// If we don't have a transform yet, try to create one
	if (!m_pFireportTransform)
	{
		// Read TransformInternal: Fireport -> 0x10 -> 0x10
		auto Conn = DMA_Connection::GetInstance();
		auto PID = EFT::GetProcess().GetPID();

		uintptr_t bifacial = 0;
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, m_FireportAddress + 0x10, 
			reinterpret_cast<PBYTE>(&bifacial), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

		if (!bifacial)
		{
			Reset();
			return;
		}

		uintptr_t transformInternal = 0;
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, bifacial + 0x10, 
			reinterpret_cast<PBYTE>(&transformInternal), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

		if (!transformInternal)
		{
			Reset();
			return;
		}

		m_TransformInternalAddress = transformInternal;
		
		// Create the new CFireportTransform - it does all initialization synchronously in constructor
		m_pFireportTransform = std::make_unique<CFireportTransform>(transformInternal);
		
		if (!m_pFireportTransform->IsValid())
		{
			Reset();
			return;
		}
	}

	// Update vertices each frame
	if (m_pFireportTransform)
	{
		if (!m_pFireportTransform->UpdateVertices())
		{
			// If update fails, reset and try again next frame
			Reset();
		}
	}
}

void CFirearmManager::Finalize()
{

	if (m_pFireportTransform && m_pFireportTransform->IsValid())
	{
		m_FireportPosition = m_pFireportTransform->GetPosition();
		m_FireportRotation = m_pFireportTransform->GetRotation();
		
		// Validate - check for NaN values
		if (std::isnan(m_FireportPosition.x) || std::isnan(m_FireportPosition.y) || std::isnan(m_FireportPosition.z) ||
			std::isnan(m_FireportRotation.x) || std::isnan(m_FireportRotation.y) || 
			std::isnan(m_FireportRotation.z) || std::isnan(m_FireportRotation.w))
		{
			Reset();
			return;
		}
		
		m_bHasFireport = true;
	}
}

void CFirearmManager::Reset()
{
	m_FireportAddress = 0;
	m_TransformInternalAddress = 0;
	m_pFireportTransform = nullptr;
	m_FireportPosition = {};
	m_FireportRotation = {};
	m_bHasFireport = false;
	m_CurrentAmmoCount = 0;
	m_LastAmmoUpdateMs = 0;
	m_bAmmoCountValid = false;
}

bool CFirearmManager::RefreshAmmoCount()
{
	if (!m_HandsControllerAddress)
	{
		m_CurrentAmmoCount = 0;
		m_LastAmmoUpdateMs = 0;
		m_bAmmoCountValid = false;
		return false;
	}

	auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if (m_LastAmmoUpdateMs != 0 && (now - m_LastAmmoUpdateMs) < 100)
		return m_bAmmoCountValid;

	m_LastAmmoUpdateMs = now;

	auto Conn = DMA_Connection::GetInstance();
	auto PID = EFT::GetProcess().GetPID();
	m_bAmmoCountValid = false;

	// Read weapon item: HandsController + 0x70
	uintptr_t weaponItem = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, m_HandsControllerAddress + Offsets::CHandsController::pItem,
		reinterpret_cast<PBYTE>(&weaponItem), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (!weaponItem)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	uint32_t chamberCount = 0;
	uintptr_t chambersPtr = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, weaponItem + Offsets::CLootItemWeapon::Chambers,
		reinterpret_cast<PBYTE>(&chambersPtr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (chambersPtr)
	{
		int chamberSlots = 0;
		bool chamberCountReadOk = VMMDLL_MemReadEx(Conn->GetHandle(), PID, chambersPtr + Offsets::CUnityList::Count,
			reinterpret_cast<PBYTE>(&chamberSlots), sizeof(int), nullptr, VMMDLL_FLAG_NOCACHE);

		constexpr int MAX_CHAMBERS = 8;
		if (chamberCountReadOk && chamberSlots > 0 && chamberSlots <= MAX_CHAMBERS)
		{
			uintptr_t arrayBase = chambersPtr + 0x20;
			for (int i = 0; i < chamberSlots; ++i)
			{
				uintptr_t slotPtr = 0;
				VMMDLL_MemReadEx(Conn->GetHandle(), PID, arrayBase + (static_cast<uintptr_t>(i) * sizeof(uintptr_t)),
					reinterpret_cast<PBYTE>(&slotPtr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

				if (!slotPtr)
					continue;

				uintptr_t containedItem = 0;
				VMMDLL_MemReadEx(Conn->GetHandle(), PID, slotPtr + Offsets::CSlot::pContainedItem,
					reinterpret_cast<PBYTE>(&containedItem), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

				if (containedItem)
					++chamberCount;
			}
		}
	}

	// Read magazine slot: Weapon + 0xC8
	uintptr_t magSlot = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, weaponItem + Offsets::CLootItemWeapon::MagSlotCache,
		reinterpret_cast<PBYTE>(&magSlot), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (!magSlot)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	// Read magazine item: Slot + 0x48
	uintptr_t magItem = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, magSlot + Offsets::CSlot::pContainedItem,
		reinterpret_cast<PBYTE>(&magItem), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (!magItem)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	// Read cartridges (StackSlot): Magazine + 0xA8
	uintptr_t cartridges = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, magItem + Offsets::CMagazineItem::Cartridges,
		reinterpret_cast<PBYTE>(&cartridges), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (!cartridges)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	// Read items list: StackSlot + 0x18
	uintptr_t itemsList = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, cartridges + Offsets::CStackSlot::pItems,
		reinterpret_cast<PBYTE>(&itemsList), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (!itemsList)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	int itemCount = 0;
	bool countReadOk = VMMDLL_MemReadEx(Conn->GetHandle(), PID, itemsList + Offsets::CUnityList::Count,
		reinterpret_cast<PBYTE>(&itemCount), sizeof(int), nullptr, VMMDLL_FLAG_NOCACHE);
	if (!countReadOk)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	constexpr int MAX_AMMO_STACKS = 64;
	if (itemCount <= 0)
	{
		m_CurrentAmmoCount = chamberCount;
		m_bAmmoCountValid = true;
		return true;
	}

	if (itemCount > MAX_AMMO_STACKS)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	uintptr_t arrayPtr = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, itemsList + Offsets::CUnityList::ArrayBase,
		reinterpret_cast<PBYTE>(&arrayPtr), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

	if (!arrayPtr)
	{
		m_CurrentAmmoCount = 0;
		return false;
	}

	uint32_t totalStack = 0;
	bool foundAmmo = false;
	for (int i = 0; i < itemCount; ++i)
	{
		uintptr_t ammoItem = 0;
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, arrayPtr + 0x20 + (static_cast<uintptr_t>(i) * sizeof(uintptr_t)),
			reinterpret_cast<PBYTE>(&ammoItem), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);

		if (!ammoItem)
			continue;

		foundAmmo = true;

		uint32_t stackCount = 0;
		VMMDLL_MemReadEx(Conn->GetHandle(), PID, ammoItem + Offsets::CItem::StackCount,
			reinterpret_cast<PBYTE>(&stackCount), sizeof(uint32_t), nullptr, VMMDLL_FLAG_NOCACHE);

		totalStack += stackCount;
	}

	if (!foundAmmo)
	{
		m_CurrentAmmoCount = chamberCount;
		if (chamberCount == 0)
			return false;
		m_bAmmoCountValid = true;
		return true;
	}

	m_CurrentAmmoCount = totalStack + chamberCount;
	m_bAmmoCountValid = true;
	return true;
}
