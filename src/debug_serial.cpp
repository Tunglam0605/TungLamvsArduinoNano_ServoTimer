#include "debug_serial.h"

#include "input.h"

#include <avr/io.h>

namespace {
constexpr uint32_t kLogIntervalMs = 200;
uint32_t g_lastLogMs = 0;

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
    if ((events & INPUT_START) != 0U) {
        if (needsSeparator) {
            WriteChar(',');
        }
        WriteString("START");
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
                        uint16_t modeAdc,
                        uint8_t selectedMode,
                        bool competitionRunning,
                        uint8_t inputEvents) {
    if (inputEvents == INPUT_NONE &&
        static_cast<uint32_t>(nowMs - g_lastLogMs) < kLogIntervalMs) {
        return;
    }
    g_lastLogMs = nowMs;

    WriteString("ADC=");
    WriteUint16(modeAdc);
    WriteString(" MODE=");
    WriteUint16(static_cast<uint16_t>(selectedMode + 1U));
    WriteString(" STATE=");
    WriteString(competitionRunning ? "RUNNING" : "IDLE");
    WriteString(" BTN=");
    WriteButtonEvents(inputEvents);
    WriteString("\r\n");
}
