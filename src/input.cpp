#include "input.h"

#include "board.h"
#include "config.h"

namespace {
struct DebouncedButton {
    const GpioPin* gpio;
    bool stablePressed;
    uint16_t mismatchMs;
    uint8_t eventMask;
};

DebouncedButton g_buttons[] = {
    {&Board::kModeButton, false, 0, INPUT_MODE},
    {&Board::kPart2LeftButton, false, 0, INPUT_PART2_LEFT},
    {&Board::kPart2RightButton, false, 0, INPUT_PART2_RIGHT},
    {&Board::kStartButton, false, 0, INPUT_START}
};

uint32_t g_lastUpdateMs = 0;
uint8_t g_events = INPUT_NONE;
}

void Input_Init() {
    for (auto& button : g_buttons) {
        Gpio_InputPullup(*button.gpio);
        button.stablePressed = Gpio_IsPressedActiveLow(*button.gpio);
        button.mismatchMs = 0;
    }
}

void Input_Update(uint32_t nowMs) {
    const uint32_t elapsed = static_cast<uint32_t>(nowMs - g_lastUpdateMs);
    if (elapsed == 0U) {
        return;
    }
    g_lastUpdateMs = nowMs;

    for (auto& button : g_buttons) {
        const bool rawPressed = Gpio_IsPressedActiveLow(*button.gpio);
        if (rawPressed == button.stablePressed) {
            button.mismatchMs = 0;
            continue;
        }

        const uint32_t accumulated = static_cast<uint32_t>(button.mismatchMs) + elapsed;
        button.mismatchMs = accumulated > 0xFFFFU ? 0xFFFFU : static_cast<uint16_t>(accumulated);

        if (button.mismatchMs >= Config::kButtonDebounceMs) {
            button.stablePressed = rawPressed;
            button.mismatchMs = 0;
            if (rawPressed) {
                g_events |= button.eventMask;
            }
        }
    }
}

uint8_t Input_TakeEvents() {
    const uint8_t events = g_events;
    g_events = INPUT_NONE;
    return events;
}
