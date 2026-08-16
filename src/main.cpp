#include "competition.h"
#include "input.h"
#include "servo_engine.h"
#include "system_tick.h"
#include "target.h"

#include <avr/interrupt.h>
#include <avr/io.h>

int main() {
    cli();

    // Timer0 is a free-running entropy source for Part 2 random selection.
    // No Timer0 interrupt and no Arduino millis()/delay() are used.
    TCCR0A = 0;
    TCCR0B = _BV(CS01) | _BV(CS00); // Prescaler /64
    TIMSK0 = 0;
    TCNT0 = 0;

    Target_Init();
    Input_Init();
    Competition_Init();
    SystemTick_Init();
    ServoEngine_Init();

    sei();

    while (true) {
        const uint32_t nowMs = SystemTick_NowMs();
        Input_Update(nowMs);
        const uint8_t events = Input_TakeEvents();
        Competition_Update(nowMs, events);
    }
}
