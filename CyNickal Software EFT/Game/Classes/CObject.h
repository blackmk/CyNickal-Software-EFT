#pragma once
#include <cstddef>

class CObject
{
public:
	char pad[0x88]{ 0 };
	uintptr_t pName{ 0 };
};