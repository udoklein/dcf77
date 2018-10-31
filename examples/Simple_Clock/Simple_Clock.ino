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

#include <dcf77.h>

#if defined(__AVR__)
const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = A5;       // A5 == d19
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 1;
// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_led = 18;  // A4 == d18

uint8_t ledpin(const uint8_t led) {
    return led;
}
#elif defined(__STM32F1__)
const uint8_t dcf77_sample_pin = PB6;
const uint8_t dcf77_inverted_samples = 1; //output from HKW EM6 DCF 3V
const WiringPinMode dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up
const uint8_t dcf77_monitor_led = PC13;
uint8_t ledpin(const int8_t led) {
    return led;
}
#else
const uint8_t dcf77_sample_pin = 53;
const uint8_t dcf77_inverted_samples = 0;

// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_led = 19;

uint8_t ledpin(const uint8_t led) {
    return led<14? led: led+(54-14);
}
#endif

uint8_t sample_input_pin() {
    const uint8_t sampled_data =
        #if defined(__AVR__)
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));
        #else
        dcf77_inverted_samples ^ digitalRead(dcf77_sample_pin);
        #endif

    digitalWrite(ledpin(dcf77_monitor_led), sampled_data);
    return sampled_data;
}

void setup() {
    using namespace Clock;

    #if defined(__STM32F1__)
    Serial1.begin(115200);
    #else
    Serial.begin(9600);
    #endif
    sprintln();
    sprintln(F("Simple DCF77 Clock V3.1.1"));
    sprintln(F("(c) Udo Klein 2016"));
    sprintln(F("www.blinkenlight.net"));
    sprintln();
    sprint(F("Sample Pin:      ")); sprintln(dcf77_sample_pin);
    sprint(F("Sample Pin Mode: ")); sprintln(dcf77_pin_mode);
    sprint(F("Inverted Mode:   ")); sprintln(dcf77_inverted_samples);
    #if defined(__AVR__)
    sprint(F("Analog Mode:     ")); sprintln(dcf77_analog_samples);
    #endif
    sprint(F("Monitor Pin:     ")); sprintln(ledpin(dcf77_monitor_led));
    sprintln();
    sprintln();
    sprintln(F("Initializing..."));

    pinMode(ledpin(dcf77_monitor_led), OUTPUT);
    pinMode(dcf77_sample_pin, dcf77_pin_mode);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);


    // Wait till clock is synced, depending on the signal quality this may take
    // rather long. About 5 minutes with a good signal, 30 minutes or longer
    // with a bad signal
    for (uint8_t state = Clock::useless;
        state == Clock::useless || state == Clock::dirty;
        state = DCF77_Clock::get_clock_state()) {

        // wait for next sec
        Clock::time_t now;
        DCF77_Clock::get_current_time(now);

        // render one dot per second while initializing
        static uint8_t count = 0;
        sprint('.');
        ++count;
        if (count == 60) {
            count = 0;
            sprintln();
        }
    }
}

void paddedPrint(BCD::bcd_t n) {
    sprint(n.digit.hi);
    sprint(n.digit.lo);
}

void loop() {
    Clock::time_t now;

    DCF77_Clock::get_current_time(now);
    if (now.month.val > 0) {
        switch (DCF77_Clock::get_clock_state()) {
            case Clock::useless: sprint(F("useless ")); break;
            case Clock::dirty:   sprint(F("dirty:  ")); break;
            case Clock::synced:  sprint(F("synced: ")); break;
            case Clock::locked:  sprint(F("locked: ")); break;
        }
        sprint(' ');

        sprint(F("20"));
        paddedPrint(now.year);
        sprint('-');
        paddedPrint(now.month);
        sprint('-');
        paddedPrint(now.day);
        sprint(' ');

        paddedPrint(now.hour);
        sprint(':');
        paddedPrint(now.minute);
        sprint(':');
        paddedPrint(now.second);

        sprint("+0");
        sprint(now.uses_summertime? '2': '1');
        sprintln();
    }
}