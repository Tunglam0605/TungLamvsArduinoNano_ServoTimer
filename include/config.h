#pragma once

#include <stdint.h>

namespace Config {
constexpr uint8_t kServoCount = 9;
constexpr uint8_t kServoDownAngle = 0;
constexpr uint8_t kServoUpAngle = 90;

constexpr uint16_t kServoFrameUs = 20000;
constexpr uint16_t kServoMinUs = 1000;
constexpr uint16_t kServoMaxUs = 2000;

constexpr uint16_t kButtonDebounceMs = 20;

constexpr uint32_t kPart1VisibleMs = 8000UL;
constexpr uint32_t kPart1HiddenMs = 10000UL;
constexpr uint8_t kPart1RoundCount = 6;

constexpr uint32_t kPart2CenterVisibleMs = 25000UL;

constexpr uint32_t kPart3Target1VisibleMs = 8000UL;
constexpr uint32_t kPart3Target1HiddenMs = 7000UL;
constexpr uint8_t kPart3Target1RepeatCount = 3;
constexpr uint32_t kPart3Target2VisibleMs = 15000UL;
}
