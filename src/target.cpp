#include "target.h"

#include "board.h"
#include "config.h"
#include "servo_engine.h"

namespace {
uint16_t g_upMask = 0;

void SetSideLed(TargetId id, TargetState state) {
    if (id == TargetId::P2_LEFT) {
        state == TargetState::Up ? Gpio_High(Board::kPart2LeftLed) : Gpio_Low(Board::kPart2LeftLed);
    } else if (id == TargetId::P2_RIGHT) {
        state == TargetState::Up ? Gpio_High(Board::kPart2RightLed) : Gpio_Low(Board::kPart2RightLed);
    }
}
}

void Target_Init() {
    Gpio_Output(Board::kPart2LeftLed);
    Gpio_Output(Board::kPart2RightLed);
    Gpio_Low(Board::kPart2LeftLed);
    Gpio_Low(Board::kPart2RightLed);
    Target_AllDown();
}

void Target_Set(TargetId id, TargetState state) {
    const uint8_t channel = static_cast<uint8_t>(id);
    if (channel >= static_cast<uint8_t>(TargetId::COUNT)) {
        return;
    }

    const uint8_t angle = (state == TargetState::Up) ? Config::kServoUpAngle : Config::kServoDownAngle;
    ServoEngine_SetAngle(channel, angle);
    SetSideLed(id, state);

    const uint16_t targetBit = static_cast<uint16_t>(1U << channel);
    if (state == TargetState::Up) {
        g_upMask = static_cast<uint16_t>(g_upMask | targetBit);
    } else {
        g_upMask = static_cast<uint16_t>(g_upMask & static_cast<uint16_t>(~targetBit));
    }
}

void Target_AllDown() {
    for (uint8_t i = 0; i < static_cast<uint8_t>(TargetId::COUNT); ++i) {
        Target_Set(static_cast<TargetId>(i), TargetState::Down);
    }
}

uint16_t Target_GetUpMask() {
    return g_upMask;
}
