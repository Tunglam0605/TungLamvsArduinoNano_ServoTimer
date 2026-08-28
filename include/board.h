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

// Maintained mode switches: A0..A2, active-low with internal pull-ups.
static const GpioPin kPart1Switch = {&DDRC, &PORTC, &PINC, _BV(PC0)}; // A0
static const GpioPin kPart2Switch = {&DDRC, &PORTC, &PINC, _BV(PC1)}; // A1
static const GpioPin kPart3Switch = {&DDRC, &PORTC, &PINC, _BV(PC2)}; // A2

// Mode LEDs: A3..A5.
static const GpioPin kModeLedPins[3] = {
    {&DDRC, &PORTC, &PINC, _BV(PC3)}, // A3
    {&DDRC, &PORTC, &PINC, _BV(PC4)}, // A4
    {&DDRC, &PORTC, &PINC, _BV(PC5)}  // A5
};

// Part-2 side LEDs: D11, D12. D13 remains free as a system/status LED.
static const GpioPin kPart2LeftLed  = {&DDRB, &PORTB, &PINB, _BV(PB3)}; // D11
static const GpioPin kPart2RightLed = {&DDRB, &PORTB, &PINB, _BV(PB4)}; // D12
static const GpioPin kSystemLed     = {&DDRB, &PORTB, &PINB, _BV(PB5)}; // D13
}

inline void Gpio_Output(const GpioPin& gpio) {
    *gpio.ddr |= gpio.mask;
}

inline void Gpio_InputPullup(const GpioPin& gpio) {
    *gpio.ddr &= static_cast<uint8_t>(~gpio.mask);
    *gpio.port |= gpio.mask;
}

inline void Gpio_InputFloating(const GpioPin& gpio) {
    *gpio.ddr &= static_cast<uint8_t>(~gpio.mask);
    *gpio.port &= static_cast<uint8_t>(~gpio.mask);
}

inline void Gpio_High(const GpioPin& gpio) {
    *gpio.port |= gpio.mask;
}

inline void Gpio_Low(const GpioPin& gpio) {
    *gpio.port &= static_cast<uint8_t>(~gpio.mask);
}

inline void Gpio_Toggle(const GpioPin& gpio) {
    // On ATmega328P, writing a one to PINx toggles only the selected PORTx bit.
    // This avoids a read-modify-write race with servo outputs on the same port.
    *gpio.pin = gpio.mask;
}

inline bool Gpio_IsPressedActiveLow(const GpioPin& gpio) {
    return ((*gpio.pin & gpio.mask) == 0U);
}
