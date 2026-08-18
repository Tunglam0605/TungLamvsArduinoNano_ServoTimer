#pragma once

#include <stdint.h>

void DebugSerial_Init();
bool DebugSerial_TakeStartCommand();
void DebugSerial_Update(uint32_t nowMs,
                        uint16_t modeAdc,
                        uint8_t selectedMode,
                        bool competitionRunning,
                        uint8_t inputEvents);
