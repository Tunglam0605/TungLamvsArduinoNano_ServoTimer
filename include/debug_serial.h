#pragma once

#include <stdint.h>

void DebugSerial_Init();
void DebugSerial_Update(uint32_t nowMs,
                        uint16_t speedAdc,
                        uint16_t move90Ms,
                        uint8_t competitionSwitchMask,
                        bool competitionRunning,
                        uint8_t inputEvents,
                        uint16_t targetsUpMask);
