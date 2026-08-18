#pragma once

#include <stdint.h>

enum class TargetId : uint8_t {
    P1_T1 = 0,
    P1_T2,
    P1_T3,
    P1_T4,
    P2_LEFT,
    P2_CENTER,
    P2_RIGHT,
    P3_T1,
    P3_T2,
    COUNT
};

enum class TargetState : uint8_t {
    Down = 0,
    Up
};

void Target_Init();
void Target_Set(TargetId id, TargetState state);
void Target_AllDown();
uint16_t Target_GetUpMask();
