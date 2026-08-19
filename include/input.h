#pragma once

#include <stdint.h>

enum CompetitionSwitch : uint8_t {
    COMPETITION_SWITCH_NONE = 0,
    COMPETITION_SWITCH_PART1 = 1U << 0,
    COMPETITION_SWITCH_PART2 = 1U << 1,
    COMPETITION_SWITCH_PART3 = 1U << 2
};

void Input_Init();
void Input_Update(uint32_t nowMs);
uint8_t Input_GetCompetitionSwitchMask();
uint16_t Input_GetSpeedAdc();
