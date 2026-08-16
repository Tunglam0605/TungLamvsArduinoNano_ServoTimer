#pragma once

#include <avr/io.h>
#include <stdint.h>

struct GpioPin {
    volatile uint8_t* ddr;
    volatile uint8_t* port;
    volatile uint8_t* pin;
    uint8_t mask;
};

namespace Board {
// Servo channels: D2..D10
static const GpioPin kServoPins[9] = {
    {&DDRD, &PORTD, &PIND, _BV(PD2)},
    {&DDRD, &PORTD, &PIND, _BV(PD3)},
    {&DDRD, &PORTD, &PIND, _BV(PD4)},
    {&DDRD, &PORTD, &PIND, _BV(PD5)},
    {&DDRD, &PORTD, &PIND, _BV(PD6)},
    {&DDRD, &PORTD, &PIND, _BV(PD7)},
    {&DDRB, &PORTB, &PINB, _BV(PB0)}, // D8
    {&DDRB, &PORTB, &PINB, _BV(PB1)}, // D9
    {&DDRB, &PORTB, &PINB, _BV(PB2)}  // D10
};

// Mode LEDs: D11, D12, D13
static const GpioPin kModeLedPins[3] = {
    {&DDRB, &PORTB, &PINB, _BV(PB3)},
    {&DDRB, &PORTB, &PINB, _BV(PB4)},
    {&DDRB, &PORTB, &PINB, _BV(PB5)}
};

// Part-2 side LEDs: A0, A1
static const GpioPin kPart2LeftLed  = {&DDRC, &PORTC, &PINC, _BV(PC0)};
static const GpioPin kPart2RightLed = {&DDRC, &PORTC, &PINC, _BV(PC1)};

// Buttons, active-low with internal pull-ups: A2..A5
static const GpioPin kModeButton       = {&DDRC, &PORTC, &PINC, _BV(PC2)};
static const GpioPin kPart2LeftButton  = {&DDRC, &PORTC, &PINC, _BV(PC3)};
static const GpioPin kPart2RightButton = {&DDRC, &PORTC, &PINC, _BV(PC4)};
static const GpioPin kStartButton      = {&DDRC, &PORTC, &PINC, _BV(PC5)};
}

inline void Gpio_Output(const GpioPin& gpio) {
    *gpio.ddr |= gpio.mask;
}

inline void Gpio_InputPullup(const GpioPin& gpio) {
    *gpio.ddr &= static_cast<uint8_t>(~gpio.mask);
    *gpio.port |= gpio.mask;
}

inline void Gpio_High(const GpioPin& gpio) {
    *gpio.port |= gpio.mask;
}

inline void Gpio_Low(const GpioPin& gpio) {
    *gpio.port &= static_cast<uint8_t>(~gpio.mask);
}

inline bool Gpio_IsPressedActiveLow(const GpioPin& gpio) {
    return ((*gpio.pin & gpio.mask) == 0U);
}
