#pragma once

#include <stdint.h>

enum class CompetitionMode : uint8_t {
    Part1 = 0,
    Part2,
    Part3,
    Count
};

void Competition_Init();
void Competition_Update(uint32_t nowMs);
CompetitionMode Competition_GetMode();
bool Competition_IsRunning();
bool Competition_IsWaitingForAllOff();
