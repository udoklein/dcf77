//
//  www.blinkenlight.net
//
//  Copyright 2014 Udo Klein
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

const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = A5;  // A5 == d19
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 1;

const uint8_t dcf77_monitor_pin = A4;  // A4 == d18


uint8_t sample_input_pin() {
    const uint8_t sampled_data =
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));

    digitalWrite(dcf77_monitor_pin, sampled_data);
    return sampled_data;
}

void setup() {
    using namespace DCF77_Encoder;

    Serial.begin(9600);
    Serial.println();
    Serial.println(F("Simple DCF77 Clock V1.0"));
    Serial.println(F("(c) Udo Klein 2014"));
    Serial.println(F("www.blinkenlight.net"));
    Serial.println();
    Serial.print(F("Sample Pin:    ")); Serial.println(dcf77_sample_pin);
    Serial.print(F("Inverted Mode: ")); Serial.println(dcf77_inverted_samples);
    Serial.print(F("Analog Mode:   ")); Serial.println(dcf77_analog_samples);
    Serial.print(F("Monitor Pin:   ")); Serial.println(dcf77_monitor_pin);
    Serial.println();
    Serial.println();
    Serial.println(F("Initializing..."));

    pinMode(dcf77_monitor_pin, OUTPUT);

    pinMode(dcf77_sample_pin, INPUT);
    digitalWrite(dcf77_sample_pin, HIGH);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);


    // Wait till clock is synced, depending on the signal quality this may take
    // rather long. About 5 minutes with a good signal, 30 minutes or longer
    // with a bad signal
    for (uint8_t state = DCF77::useless;
         state == DCF77::useless || state == DCF77::dirty;
         state = DCF77_Clock::get_clock_state()) {

        // wait for next sec
        DCF77_Clock::time_t now;
        DCF77_Clock::get_current_time(now);

        // render one dot per second while initializing
        static uint8_t count = 0;
        Serial.print('.');
        ++count;
        if (count == 60) {
            count = 0;
            Serial.println();
        }
    }
}

void paddedPrint(BCD::bcd_t n) {
    Serial.print(n.digit.hi);
    Serial.print(n.digit.lo);
}

void loop() {
    DCF77_Clock::time_t now;

    DCF77_Clock::get_current_time(now);
    if (now.month.val > 0) {
        switch (DCF77_Clock::get_clock_state()) {
            case DCF77::useless: Serial.print(F("useless ")); break;
            case DCF77::dirty:   Serial.print(F("dirty:  ")); break;
            case DCF77::synced:  Serial.print(F("synced: ")); break;
            case DCF77::locked:  Serial.print(F("locked: ")); break;
        }
        Serial.print(' ');

        Serial.print(F("20"));
        paddedPrint(now.year);
        Serial.print('-');
        paddedPrint(now.month);
        Serial.print('-');
        paddedPrint(now.day);
        Serial.print(' ');

        paddedPrint(now.hour);
        Serial.print(':');
        paddedPrint(now.minute);
        Serial.print(':');
        paddedPrint(now.second);

        Serial.print("+0");
        Serial.print(now.uses_summertime? '2': '1');
        Serial.println();
    }
}
