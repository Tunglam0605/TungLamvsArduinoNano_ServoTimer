#include "servo_engine.h"

#include "board.h"
#include "config.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

namespace {
constexpr uint16_t kTimerTicksPerUs = 2; // 16 MHz / 8 = 2 MHz
constexpr uint16_t kFrameTicks = Config::kServoFrameUs * kTimerTicksPerUs;

volatile uint16_t g_pulseTicks[Config::kServoCount];
volatile uint8_t g_channel = 0;
volatile bool g_inGap = false;
volatile uint16_t g_activePulseTicks = 0;
volatile uint16_t g_usedTicks = 0;

uint16_t AngleToTicks(uint8_t angleDeg) {
    if (angleDeg > 180U) {
        angleDeg = 180U;
    }
    const uint16_t spanUs = static_cast<uint16_t>(Config::kServoMaxUs - Config::kServoMinUs);
    const uint16_t pulseUs = static_cast<uint16_t>(Config::kServoMinUs +
        (static_cast<uint32_t>(spanUs) * angleDeg) / 180U);
    return static_cast<uint16_t>(pulseUs * kTimerTicksPerUs);
}
}

void ServoEngine_Init() {
    const uint16_t downTicks = AngleToTicks(Config::kServoDownAngle);

    for (uint8_t i = 0; i < Config::kServoCount; ++i) {
        Gpio_Output(Board::kServoPins[i]);
        Gpio_Low(Board::kServoPins[i]);
        g_pulseTicks[i] = downTicks;
    }

    TCCR1A = 0;
    TCCR1B = _BV(CS11); // Normal mode, prescaler /8
    TCNT1 = 0;

    g_channel = 0;
    g_inGap = false;
    g_usedTicks = 0;
    g_activePulseTicks = g_pulseTicks[0];

    Gpio_High(Board::kServoPins[0]);
    OCR1A = static_cast<uint16_t>(TCNT1 + g_activePulseTicks);
    TIFR1 = _BV(OCF1A);
    TIMSK1 = _BV(OCIE1A);
}

void ServoEngine_SetAngle(uint8_t channel, uint8_t angleDeg) {
    if (channel >= Config::kServoCount) {
        return;
    }
    const uint16_t ticks = AngleToTicks(angleDeg);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        g_pulseTicks[channel] = ticks;
    }
}

ISR(TIMER1_COMPA_vect) {
    if (!g_inGap) {
        Gpio_Low(Board::kServoPins[g_channel]);
        g_usedTicks = static_cast<uint16_t>(g_usedTicks + g_activePulseTicks);
        ++g_channel;

        if (g_channel < Config::kServoCount) {
            g_activePulseTicks = g_pulseTicks[g_channel];
            Gpio_High(Board::kServoPins[g_channel]);
            OCR1A = static_cast<uint16_t>(OCR1A + g_activePulseTicks);
        } else {
            g_inGap = true;
            uint16_t gapTicks = 1000U;
            if (g_usedTicks < kFrameTicks) {
                gapTicks = static_cast<uint16_t>(kFrameTicks - g_usedTicks);
            }
            OCR1A = static_cast<uint16_t>(OCR1A + gapTicks);
        }
    } else {
        g_inGap = false;
        g_channel = 0;
        g_usedTicks = 0;
        g_activePulseTicks = g_pulseTicks[0];
        Gpio_High(Board::kServoPins[0]);
        OCR1A = static_cast<uint16_t>(OCR1A + g_activePulseTicks);
    }
}
