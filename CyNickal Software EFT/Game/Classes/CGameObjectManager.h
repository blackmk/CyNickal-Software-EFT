#pragma once
#include <cstddef>

class CGameObjectManager
{
public:
	char pad[0x20];
	uintptr_t pLastActiveNode{ 0 };
	uintptr_t pActiveNodes{ 0 };
};