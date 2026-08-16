#include "target.h"

#include "board.h"
#include "config.h"
#include "servo_engine.h"

namespace {
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
}

void Target_AllDown() {
    for (uint8_t i = 0; i < static_cast<uint8_t>(TargetId::COUNT); ++i) {
        Target_Set(static_cast<TargetId>(i), TargetState::Down);
    }
}
