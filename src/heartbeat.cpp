#include "heartbeat.h"

#include "board.h"
#include "config.h"

namespace {
uint32_t g_lastToggleMs = 0;
}

void Heartbeat_Init() {
    Gpio_Output(Board::kSystemLed);
    Gpio_Low(Board::kSystemLed);
    g_lastToggleMs = 0;
}

void Heartbeat_Update(uint32_t nowMs) {
    if (static_cast<uint32_t>(nowMs - g_lastToggleMs) < Config::kSystemLedToggleMs) {
        return;
    }

    g_lastToggleMs = nowMs;
    Gpio_Toggle(Board::kSystemLed);
}
