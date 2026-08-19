#include "debug_serial.h"

#include "input.h"

#include <avr/io.h>

namespace {
constexpr uint32_t kLogIntervalMs = 200;
uint32_t g_lastLogMs = 0;
constexpr const char* kTargetNames[] = {
    "P1T1", "P1T2", "P1T3", "P1T4",
    "P2L", "P2C", "P2R",
    "P3T1", "P3T2"
};

void WriteChar(char value) {
    while ((UCSR0A & _BV(UDRE0)) == 0U) {
    }
    UDR0 = static_cast<uint8_t>(value);
}

void WriteString(const char* text) {
    while (*text != '\0') {
        WriteChar(*text);
        ++text;
    }
}

void WriteUint16(uint16_t value) {
    char digits[5];
    uint8_t count = 0;

    do {
        digits[count] = static_cast<char>('0' + (value % 10U));
        value = static_cast<uint16_t>(value / 10U);
        ++count;
    } while (value != 0U);

    while (count > 0U) {
        --count;
        WriteChar(digits[count]);
    }
}

void WriteButtonEvents(uint8_t events) {
    if (events == INPUT_NONE) {
        WriteChar('-');
        return;
    }

    bool needsSeparator = false;
    if ((events & INPUT_PART2_LEFT) != 0U) {
        WriteString("LEFT");
        needsSeparator = true;
    }
    if ((events & INPUT_PART2_RIGHT) != 0U) {
        if (needsSeparator) {
            WriteChar(',');
        }
        WriteString("RIGHT");
        needsSeparator = true;
    }
}

void WriteCompetitionSwitches(uint8_t switchMask) {
    WriteChar((switchMask & COMPETITION_SWITCH_PART1) != 0U ? '1' : '0');
    WriteChar((switchMask & COMPETITION_SWITCH_PART2) != 0U ? '1' : '0');
    WriteChar((switchMask & COMPETITION_SWITCH_PART3) != 0U ? '1' : '0');
}

void WriteSelection(uint8_t switchMask) {
    if (switchMask == COMPETITION_SWITCH_PART1) {
        WriteChar('1');
    } else if (switchMask == COMPETITION_SWITCH_PART2) {
        WriteChar('2');
    } else if (switchMask == COMPETITION_SWITCH_PART3) {
        WriteChar('3');
    } else if (switchMask == COMPETITION_SWITCH_NONE) {
        WriteString("NONE");
    } else {
        WriteString("INVALID");
    }
}

void WriteTargetsUp(uint16_t targetsUpMask) {
    if (targetsUpMask == 0U) {
        WriteChar('-');
        return;
    }

    bool needsSeparator = false;
    for (uint8_t target = 0; target < 9U; ++target) {
        const uint16_t targetBit = static_cast<uint16_t>(1U << target);
        if ((targetsUpMask & targetBit) == 0U) {
            continue;
        }
        if (needsSeparator) {
            WriteChar(',');
        }
        WriteString(kTargetNames[target]);
        needsSeparator = true;
    }
}
}

void DebugSerial_Init() {
    // 16 MHz, double-speed mode, UBRR0=16: approximately 115200 baud.
    UCSR0A = _BV(U2X0);
    UBRR0H = 0;
    UBRR0L = 16;
    UCSR0B = _BV(TXEN0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8 data bits, no parity, 1 stop bit

    WriteString("Servo controller ready\r\n");
}

void DebugSerial_Update(uint32_t nowMs,
                        uint16_t speedAdc,
                        uint16_t move90Ms,
                        uint8_t competitionSwitchMask,
                        bool competitionRunning,
                        uint8_t inputEvents,
                        uint16_t targetsUpMask) {
    if (inputEvents == INPUT_NONE &&
        static_cast<uint32_t>(nowMs - g_lastLogMs) < kLogIntervalMs) {
        return;
    }
    g_lastLogMs = nowMs;

    WriteString("SPEED_ADC=");
    WriteUint16(speedAdc);
    WriteString(" MOVE90_MS=");
    WriteUint16(move90Ms);
    WriteString(" SW=");
    WriteCompetitionSwitches(competitionSwitchMask);
    WriteString(" SELECT=");
    WriteSelection(competitionSwitchMask);
    WriteString(" STATE=");
    WriteString(competitionRunning ? "RUNNING" : "IDLE");
    WriteString(" BTN=");
    WriteButtonEvents(inputEvents);
    WriteString(" UP=");
    WriteTargetsUp(targetsUpMask);
    WriteString("\r\n");
}
