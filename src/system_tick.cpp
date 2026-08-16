#include "system_tick.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

static volatile uint32_t g_tickMs = 0;

void SystemTick_Init() {
    TCCR2A = _BV(WGM21);                 // CTC mode
    TCCR2B = _BV(CS22);                  // Prescaler /64
    OCR2A = 249;                         // 16 MHz / 64 / 250 = 1 kHz
    TCNT2 = 0;
    TIFR2 = _BV(OCF2A);
    TIMSK2 = _BV(OCIE2A);
}

uint32_t SystemTick_NowMs() {
    uint32_t value;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        value = g_tickMs;
    }
    return value;
}

ISR(TIMER2_COMPA_vect) {
    ++g_tickMs;
}
