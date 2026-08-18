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
    {&Board::kPart2LeftButton, false, 0, INPUT_PART2_LEFT},
    {&Board::kPart2RightButton, false, 0, INPUT_PART2_RIGHT},
    {&Board::kStartButton, false, 0, INPUT_START}
};

uint32_t g_lastUpdateMs = 0;
uint32_t g_lastPotSampleMs = 0;
uint32_t g_modeCandidateStartMs = 0;
uint8_t g_events = INPUT_NONE;
uint8_t g_modeSelection = 0;
uint8_t g_modeCandidate = 0;

constexpr uint16_t kMode1UpperBoundary = 341;
constexpr uint16_t kMode2UpperBoundary = 683;

uint16_t ReadModePotAdc() {
    ADCSRA |= _BV(ADSC);
    while ((ADCSRA & _BV(ADSC)) != 0U) {
    }

    const uint8_t low = ADCL;
    const uint8_t high = ADCH;
    return static_cast<uint16_t>(low | (static_cast<uint16_t>(high) << 8U));
}

uint8_t ModeFromAdc(uint16_t adcValue) {
    if (adcValue < kMode1UpperBoundary) {
        return 0;
    }
    if (adcValue < kMode2UpperBoundary) {
        return 1;
    }
    return 2;
}

uint8_t ModeWithHysteresis(uint16_t adcValue) {
    const uint16_t hysteresis = Config::kModePotHysteresis;
    switch (g_modeSelection) {
        case 0:
            if (adcValue >= static_cast<uint16_t>(kMode2UpperBoundary + hysteresis)) {
                return 2;
            }
            return adcValue >= static_cast<uint16_t>(kMode1UpperBoundary + hysteresis) ? 1 : 0;
        case 1:
            if (adcValue <= static_cast<uint16_t>(kMode1UpperBoundary - hysteresis)) {
                return 0;
            }
            return adcValue >= static_cast<uint16_t>(kMode2UpperBoundary + hysteresis) ? 2 : 1;
        default:
            if (adcValue <= static_cast<uint16_t>(kMode1UpperBoundary - hysteresis)) {
                return 0;
            }
            return adcValue <= static_cast<uint16_t>(kMode2UpperBoundary - hysteresis) ? 1 : 2;
    }
}

void UpdateModePot(uint32_t nowMs) {
    if (static_cast<uint32_t>(nowMs - g_lastPotSampleMs) < Config::kModePotSampleMs) {
        return;
    }
    g_lastPotSampleMs = nowMs;

    const uint8_t newCandidate = ModeWithHysteresis(ReadModePotAdc());
    if (newCandidate == g_modeSelection) {
        g_modeCandidate = g_modeSelection;
        g_modeCandidateStartMs = nowMs;
        return;
    }

    if (newCandidate != g_modeCandidate) {
        g_modeCandidate = newCandidate;
        g_modeCandidateStartMs = nowMs;
        return;
    }

    if (static_cast<uint32_t>(nowMs - g_modeCandidateStartMs) >= Config::kModePotStableMs) {
        g_modeSelection = g_modeCandidate;
    }
}
}

void Input_Init() {
    Gpio_InputFloating(Board::kModePotentiometer);
    DIDR0 |= _BV(ADC2D);
    ADMUX = _BV(REFS0) | 2U; // AVcc reference, ADC2/A2 channel
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // ADC clock /128

    g_modeSelection = ModeFromAdc(ReadModePotAdc());
    g_modeCandidate = g_modeSelection;

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

    UpdateModePot(nowMs);

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

uint8_t Input_GetModeSelection() {
    return g_modeSelection;
}
