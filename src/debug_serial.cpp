#include "debug_serial.h"

#include "input.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace {
constexpr uint32_t kLogIntervalMs = 200;
constexpr uint8_t kRxBufferSize = 16;
constexpr uint8_t kRxBufferMask = kRxBufferSize - 1U;
uint32_t g_lastLogMs = 0;
volatile char g_rxBuffer[kRxBufferSize];
volatile uint8_t g_rxHead = 0;
volatile uint8_t g_rxTail = 0;
uint8_t g_startMatchLength = 0;

constexpr char kStartCommand[] = "START";
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
    if ((events & INPUT_START) != 0U) {
        if (needsSeparator) {
            WriteChar(',');
        }
        WriteString("START");
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
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8 data bits, no parity, 1 stop bit

    WriteString("Servo controller ready\r\n");
}

bool DebugSerial_TakeStartCommand() {
    bool commandReceived = false;

    while (g_rxTail != g_rxHead) {
        char received = g_rxBuffer[g_rxTail];
        g_rxTail = static_cast<uint8_t>((g_rxTail + 1U) & kRxBufferMask);

        if (received >= 'a' && received <= 'z') {
            received = static_cast<char>(received - ('a' - 'A'));
        }

        if (received == kStartCommand[g_startMatchLength]) {
            ++g_startMatchLength;
            if (g_startMatchLength == 5U) {
                commandReceived = true;
                g_startMatchLength = 0;
            }
        } else {
            g_startMatchLength = received == 'S' ? 1U : 0U;
        }
    }

    return commandReceived;
}

void DebugSerial_Update(uint32_t nowMs,
                        uint16_t modeAdc,
                        uint16_t speedAdc,
                        uint16_t move90Ms,
                        uint8_t selectedMode,
                        bool competitionRunning,
                        uint8_t inputEvents,
                        uint16_t targetsUpMask) {
    if (inputEvents == INPUT_NONE &&
        static_cast<uint32_t>(nowMs - g_lastLogMs) < kLogIntervalMs) {
        return;
    }
    g_lastLogMs = nowMs;

    WriteString("ADC=");
    WriteUint16(modeAdc);
    WriteString(" SPEED_ADC=");
    WriteUint16(speedAdc);
    WriteString(" MOVE90_MS=");
    WriteUint16(move90Ms);
    WriteString(" MODE=");
    WriteUint16(static_cast<uint16_t>(selectedMode + 1U));
    WriteString(" STATE=");
    WriteString(competitionRunning ? "RUNNING" : "IDLE");
    WriteString(" BTN=");
    WriteButtonEvents(inputEvents);
    WriteString(" UP=");
    WriteTargetsUp(targetsUpMask);
    WriteString("\r\n");
}

ISR(USART_RX_vect) {
    const char received = static_cast<char>(UDR0);
    const uint8_t nextHead = static_cast<uint8_t>((g_rxHead + 1U) & kRxBufferMask);
    if (nextHead != g_rxTail) {
        g_rxBuffer[g_rxHead] = received;
        g_rxHead = nextHead;
    }
}
