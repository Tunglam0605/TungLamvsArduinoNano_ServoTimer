#include "input.h"

#include "board.h"
#include "config.h"

#include <avr/io.h>

namespace {
struct DebouncedButton {
    const GpioPin* gpio;
    bool stablePressed;
    uint16_t mismatchMs;
    uint8_t eventMask;
};

DebouncedButton g_buttons[] = {
    {&Board::kPart1Switch, false, 0, INPUT_NONE},
    {&Board::kPart2LeftButton, false, 0, INPUT_PART2_LEFT},
    {&Board::kPart2RightButton, false, 0, INPUT_PART2_RIGHT},
    {&Board::kPart2Switch, false, 0, INPUT_NONE}
};

constexpr uint8_t kPart1SwitchIndex = 0;
constexpr uint8_t kPart2SwitchIndex = 3;

uint32_t g_lastUpdateMs = 0;
uint32_t g_lastAnalogSampleMs = 0;
uint8_t g_events = INPUT_NONE;
uint16_t g_speedAdc = 0;
bool g_part3SwitchOn = false;
uint16_t g_part3SwitchMismatchMs = 0;

uint16_t ReadAdc(uint8_t channel) {
    ADMUX = static_cast<uint8_t>(_BV(REFS0) | (channel & 0x0FU));
    ADCSRA |= _BV(ADSC);
    while ((ADCSRA & _BV(ADSC)) != 0U) {
    }

    const uint8_t low = ADCL;
    const uint8_t high = ADCH;
    return static_cast<uint16_t>(low | (static_cast<uint16_t>(high) << 8U));
}

void UpdateAnalogInputs(uint32_t nowMs) {
    const uint32_t elapsed = static_cast<uint32_t>(nowMs - g_lastAnalogSampleMs);
    if (elapsed < Config::kAnalogSampleMs) {
        return;
    }
    g_lastAnalogSampleMs = nowMs;

    const uint16_t rawSpeedAdc = ReadAdc(6);
    g_speedAdc = static_cast<uint16_t>((static_cast<uint32_t>(g_speedAdc) * 7U + rawSpeedAdc) / 8U);

    const bool rawPart3On = ReadAdc(7) < Config::kPart3SwitchOnThreshold;
    if (rawPart3On == g_part3SwitchOn) {
        g_part3SwitchMismatchMs = 0;
        return;
    }

    const uint32_t accumulated = static_cast<uint32_t>(g_part3SwitchMismatchMs) + elapsed;
    g_part3SwitchMismatchMs = accumulated > 0xFFFFU ? 0xFFFFU : static_cast<uint16_t>(accumulated);
    if (g_part3SwitchMismatchMs >= Config::kButtonDebounceMs) {
        g_part3SwitchOn = rawPart3On;
        g_part3SwitchMismatchMs = 0;
    }
}
}

void Input_Init() {
    // A6 and A7 are analog-only and have no digital input buffers.
    ADMUX = _BV(REFS0); // AVcc reference; channel is selected per conversion
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // ADC clock /128

    g_speedAdc = ReadAdc(6);
    g_part3SwitchOn = ReadAdc(7) < Config::kPart3SwitchOnThreshold;

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

    UpdateAnalogInputs(nowMs);

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

uint8_t Input_GetCompetitionSwitchMask() {
    uint8_t mask = COMPETITION_SWITCH_NONE;
    if (g_buttons[kPart1SwitchIndex].stablePressed) {
        mask = static_cast<uint8_t>(mask | COMPETITION_SWITCH_PART1);
    }
    if (g_buttons[kPart2SwitchIndex].stablePressed) {
        mask = static_cast<uint8_t>(mask | COMPETITION_SWITCH_PART2);
    }
    if (g_part3SwitchOn) {
        mask = static_cast<uint8_t>(mask | COMPETITION_SWITCH_PART3);
    }
    return mask;
}

uint16_t Input_GetSpeedAdc() {
    return g_speedAdc;
}
