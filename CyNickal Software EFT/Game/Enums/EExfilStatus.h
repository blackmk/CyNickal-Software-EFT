#pragma once
#include <cstdint>
#include <limits>

enum class EExfilStatus : uint32_t
{
	NotPresent = 1,
	UncompleteRequirements = 2,
	Countdown = 3,
	Regular = 4,
	Pending = 5,
	AwaitingActivation = 6,
	Hidden = 7,
	UNKNOWN = std::numeric_limits<uint32_t>::max()
};