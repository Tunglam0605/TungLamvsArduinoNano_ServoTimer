#pragma once

#include <stdint.h>

void ServoEngine_Init();
void ServoEngine_SetAngle(uint8_t channel, uint8_t angleDeg);
void ServoEngine_SetSpeedFromAdc(uint16_t adcValue);
uint16_t ServoEngine_GetMove90Ms();
