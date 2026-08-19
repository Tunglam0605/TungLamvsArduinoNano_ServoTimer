#include "competition.h"
#include "debug_serial.h"
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
    ServoEngine_SetSpeedFromAdc(Input_GetSpeedAdc());
    DebugSerial_Init();

    sei();

    uint16_t appliedSpeedAdc = Input_GetSpeedAdc();
    uint16_t move90Ms = ServoEngine_GetMove90Ms();

    while (true) {
        const uint32_t nowMs = SystemTick_NowMs();
        Input_Update(nowMs);
        const uint16_t speedAdc = Input_GetSpeedAdc();
        if (speedAdc != appliedSpeedAdc) {
            ServoEngine_SetSpeedFromAdc(speedAdc);
            appliedSpeedAdc = speedAdc;
            move90Ms = ServoEngine_GetMove90Ms();
        }
        Competition_Update(nowMs);
        DebugSerial_Update(nowMs,
                           speedAdc,
                           move90Ms,
                           Input_GetCompetitionSwitchMask(),
                           static_cast<uint8_t>(Competition_GetMode()),
                           Competition_IsRunning(),
                           Competition_IsWaitingForAllOff(),
                           Target_GetUpMask());
    }
}
