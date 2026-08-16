#pragma once

#include <stdint.h>

enum InputEvent : uint8_t {
    INPUT_NONE = 0,
    INPUT_MODE = 1U << 0,
    INPUT_PART2_LEFT = 1U << 1,
    INPUT_PART2_RIGHT = 1U << 2,
    INPUT_START = 1U << 3
};

void Input_Init();
void Input_Update(uint32_t nowMs);
uint8_t Input_TakeEvents();
