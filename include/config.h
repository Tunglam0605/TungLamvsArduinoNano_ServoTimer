#pragma once

#include <stdint.h>

namespace Config {
constexpr uint8_t kServoCount = 9;
constexpr uint8_t kServoDownAngle = 0;
constexpr uint8_t kServoUpAngle = 90;

constexpr uint16_t kServoFrameUs = 20000;
// Calibrated for the tested SG90 units: this maps logical 0°..180° to the
// usable mechanical travel. Reduce either endpoint if a particular servo
// reaches its stop or buzzes.
constexpr uint16_t kServoMinUs = 600;
constexpr uint16_t kServoMaxUs = 2400;
// The A6 speed potentiometer maps this slew range to approximately
// 2000 ms (slow) through 200 ms (fast) for a 0-to-90-degree move.
constexpr uint16_t kServoMinSlewStepUsPerFrame = 9;
constexpr uint16_t kServoMaxSlewStepUsPerFrame = 90;

constexpr uint16_t kButtonDebounceMs = 20;
constexpr uint16_t kAnalogSampleMs = 10;
constexpr uint16_t kModeCompleteBlinkMs = 500;

constexpr uint32_t kPart1VisibleMs = 8000UL;
constexpr uint32_t kPart1HiddenMs = 10000UL;
constexpr uint8_t kPart1RoundCount = 6;

constexpr uint32_t kPart2CenterVisibleMs = 25000UL;

constexpr uint32_t kPart3Target1VisibleMs = 8000UL;
constexpr uint32_t kPart3Target1HiddenMs = 7000UL;
constexpr uint8_t kPart3Target1RepeatCount = 3;
constexpr uint32_t kPart3Target2VisibleMs = 15000UL;
}
