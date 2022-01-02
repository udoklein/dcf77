//
//  www.blinkenlight.net
//
//  Copyright 2016 Udo Klein
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see http://www.gnu.org/licenses/
#include "Arduino.h"
void process_one_sample();

namespace {
    // workaround to convince the Arduino IDE to not mess with the macros
}

#if defined(__AVR__)
// pin settings for AVR based Arduino like Blinkenlighty, Uno or Nano
const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = 19;  // A5 == D19 for standard Arduinos
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 0;
// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_led = 18; // A4 == D18 for standard Arduinos

const uint8_t lower_output_led = 2;
const uint8_t upper_output_led = 17;

uint8_t ledpin(const int8_t led) {
    return led;
}

#elif defined(__STM32F1__)
const uint8_t dcf77_sample_pin = PB6;
const uint8_t dcf77_inverted_samples = 1; //output from HKW EM6 DCF 3V
const uint8_t lower_output_led = PB7;
const uint8_t upper_output_led = PB7;
const WiringPinMode dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up
const uint8_t dcf77_monitor_led = PC13;
uint8_t ledpin(const int8_t led) {
    return led;
}

#else
// Pin settings for Arduino Due
// No support for "analog samples", the "analog samples".
// is only required for the Blinkenlighty or Arduino Uno + Blinkenlight Shield.
// For the Due any free pin may be used in digital mode.

const uint8_t dcf77_sample_pin = 53;
const uint8_t dcf77_inverted_samples = 0;

// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_led = 18;

const uint8_t lower_output_led = 2;
const uint8_t upper_output_led = 17;



uint8_t ledpin(const int8_t led) {
    return led<14? led: led+(54-14);
}

// Sine the DUE has so many pins it may be convenient to power
// some pins in a way that fits some specific DCF77 module.

// As an example these are pin settings for the "Pollin Module".
// #define dedicated_module_support 1
const uint8_t gnd_pin  = 51;
const uint8_t pon_pin  = 51;  // connect pon to ground !!!
const uint8_t data_pin = dcf77_sample_pin;  // 53
const uint8_t vcc_pin  = 49;
#endif

// CRITICAL_SECTION macro definition depending on architecture
#if defined(__AVR__)
    #include <avr/eeprom.h>

    #include <util/atomic.h>
    #define CRITICAL_SECTION ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

#elif defined(__arm__)
    // Workaround as suggested by Stackoverflow user "Notlikethat"
    // http://stackoverflow.com/questions/27998059/atomic-block-for-reading-vs-arm-systicks

    static inline int __int_disable_irq(void) {
        int primask;
        asm volatile("mrs %0, PRIMASK\n" :: "r"(primask));
        asm volatile("cpsid i\n");
        return primask & 1;
    }

    static inline void __int_restore_irq(int *primask) {
        if (!(*primask)) {
            asm volatile ("" ::: "memory");
            asm volatile("cpsie i\n");
        }
    }
    // This critical section macro borrows heavily from
    // avr-libc util/atomic.h
    // --> http://www.nongnu.org/avr-libc/user-manual/atomic_8h_source.html
    #define CRITICAL_SECTION for (int primask_save __attribute__((__cleanup__(__int_restore_irq))) = __int_disable_irq(), __n = 1; __n; __n = 0)

#else
    #error Unsupported controller architecture
#endif

#ifdef __STM32F1__
    #define sprint(...)   Serial1.print(__VA_ARGS__)
    #define sprintln(...) Serial1.println(__VA_ARGS__)
#else
    #define sprint(...)   Serial.print(__VA_ARGS__)
    #define sprintln(...) Serial.println(__VA_ARGS__)
#endif

#if defined(__STM32F1__)
void setupTimer() {
    systick_attach_callback(process_one_sample);
}
#endif

uint8_t sample_input_pin() {
    const uint8_t sampled_data =
    #if defined(__AVR__)
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200):
                                                        digitalRead(dcf77_sample_pin));
    #else
        dcf77_inverted_samples ^ digitalRead(dcf77_sample_pin);
    #endif

    digitalWrite(ledpin(dcf77_monitor_led), sampled_data);
    return sampled_data;
}

void led_display_signal(const uint8_t sample) {
    static uint8_t ticks_per_cycle = 12;
    static uint8_t rolling_led = lower_output_led;
    static uint8_t counter = 0;

    digitalWrite(ledpin(rolling_led), sample);

    if (counter < ticks_per_cycle) {
        ++counter;
    } else {
        rolling_led = rolling_led < upper_output_led? rolling_led + 1: lower_output_led;
        counter = 1;
        // toggle between 12 and 13 to get 12.5 on average
        ticks_per_cycle = 25-ticks_per_cycle;
    }
}

const uint16_t samples_per_second = 1000;
const uint8_t bins                = 100;
const uint8_t samples_per_bin     = samples_per_second / bins;

volatile uint8_t gbin[bins];
volatile boolean samples_pending = false;

void process_one_sample() {
    static uint8_t sbin[bins];

    const uint8_t sample = sample_input_pin();
    led_display_signal(sample);

    static uint16_t ticks = 999;  // first pass will init the bins
    ++ticks;

    if (ticks == 1000) {
        ticks = 0;
        memcpy((void *)gbin, sbin, bins);
        memset(sbin, 0, bins);
        samples_pending = true;
    }

    sbin[ticks/samples_per_bin] += sample;
}

void setup() {
    #ifdef __STM32F1__
    Serial1.begin(115200);
    #else
    Serial.begin(115200);
    #endif
    sprintln();

    #if defined(dedicated_module_support)
    pinMode(gnd_pin, OUTPUT);
    digitalWrite(gnd_pin, LOW);
    pinMode(pon_pin, OUTPUT);
    digitalWrite(pon_pin, LOW);
    pinMode(vcc_pin, OUTPUT);
    digitalWrite(vcc_pin, HIGH);
    #endif

    pinMode(dcf77_sample_pin, dcf77_pin_mode);

    pinMode(dcf77_monitor_led, OUTPUT);
    for (uint8_t led = lower_output_led; led <= upper_output_led; ++led) {
        pinMode(ledpin(led), OUTPUT);
        digitalWrite(ledpin(led), LOW);
    }

    setupTimer();
}

void loop() {
    static int64_t count = 0;
    uint8_t lbin[bins];

    if (samples_pending) {
        CRITICAL_SECTION {
            memcpy(lbin, (void *)gbin, bins);
            samples_pending = false;
        }

        ++count;
        // ensure the count values will be aligned to the right
        for (int32_t val=count; val < 100000000; val *= 10) {
            sprint(' ');
        }
        sprint((int32_t)count);
        sprint(", ");
        for (uint8_t bin=0; bin<bins; ++bin) {
            switch (lbin[bin]) {
                case  0: sprint(bin%10? '-': '+'); break;
                case 10: sprint('X'); break;
                default: sprint(lbin[bin]);
            }
        }
        sprintln();
     }
}

// timer handling depending on architecture
#if defined(__AVR_ATmega168__)  || \
    defined(__AVR_ATmega48__)   || \
    defined(__AVR_ATmega88__)   || \
    defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega1280__) || \
    defined(__AVR_ATmega2560__) || \
    defined(__AVR_AT90USB646__) || \
    defined(__AVR_AT90USB1286__)
ISR(TIMER2_COMPA_vect) {
    process_one_sample();
}

void stopTimer0() {
    // ensure that the standard timer interrupts will not
    // mess with msTimer2
    TIMSK0 = 0;
}

void initTimer2() {
    // Timer 2 CTC mode, prescaler 64
    TCCR2B = (0<<WGM22) | (1<<CS22);
    TCCR2A = (1<<WGM21) | (0<<WGM20);

    // 16 MHz: 249 + 1 == 250 == 250 000 / 1000 =  (16 000 000 / 64) / 1000
    //  8 MHz: 124 + 1 == 125 == 125 000 / 1000 =  ( 8 000 000 / 64) / 1000
    OCR2A = (F_CPU / 64 / 1000) - 1;

    // enable Timer 2 interrupts
    TIMSK2 = (1<<OCIE2A);
}

void setupTimer() {
    initTimer2();
    stopTimer0();
}
#endif

// timer handling depending on architecture
#if defined(__AVR_ATmega32U4__)
ISR(TIMER3_COMPA_vect) {
    process_one_sample();
}

void stopTimer0() {
    // ensure that the standard timer interrupts will not
    // mess with msTimer2
    TIMSK0 = 0;
}

void initTimer3() {
    // Timer 3 CTC mode, prescaler 64
    TCCR3B = (0<<WGM33) | (1<<WGM32) | (1<<CS31) | (1<<CS30);
    TCCR3A = (0<<WGM31) | (0<<WGM30);

    // 249 + 1 == 250 == 250 000 / 1000 =  (16 000 000 / 64) / 1000
    OCR3A = 249;

    // enable Timer 3 interrupts
    TIMSK3 = (1<<OCIE3A);
}

void setupTimer() {
    initTimer3();
    stopTimer0();
}
#endif

#if defined(__SAM3X8E__)
// Systick hook implementation moved to systick_hook.cpp
// in order to fix compiler issue.
void setupTimer() {}
#endif
