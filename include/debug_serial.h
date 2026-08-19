#pragma once

#include <stdint.h>

void DebugSerial_Init();
void DebugSerial_Update(uint32_t nowMs,
                        uint16_t speedAdc,
                        uint16_t move90Ms,
                        uint8_t competitionSwitchMask,
                        uint8_t lockedMode,
                        bool competitionRunning,
                        bool waitingForAllOff,
                        uint16_t targetsUpMask);
