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


// Resources:
//     http://www.obbl-net.de/dcf77.html
//     http://home.arcor.de/armin.diehl/dcf77usb/
//     http://www.meinbergglobal.com/english/info/ntp.htm#cfg
//     http://www.meinbergglobal.com/download/ntp/docs/ntp_cheat_sheet.pdf
//     http://www.meinberg.de/german/specs/timestr.htm

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

    //Serial.begin(9600, SERIAL_7N2);
    Serial.begin(9600);
    Serial.println();
    Serial.println(F("Meinberg Emulator"));
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
    Serial.println();

    pinMode(dcf77_monitor_pin, OUTPUT);

    pinMode(dcf77_sample_pin, INPUT);
    digitalWrite(dcf77_sample_pin, HIGH);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);
}

void paddedPrint(BCD::bcd_t n) {
    Serial.print(n.digit.hi);
    Serial.print(n.digit.lo);
}

void loop() {
    const char STX = 2;
    const char ETX = 3;

    DCF77_Clock::time_t now;

    DCF77_Clock::get_current_time(now);
    if (now.month.val > 0) {

        //Serial.println();
        Serial.print(STX);

        Serial.print("D:");
        paddedPrint(now.day);
        Serial.print('.');
        paddedPrint(now.month);
        Serial.print('.');
        paddedPrint(now.year);
        Serial.print(';');

        Serial.print("T:");
        Serial.print(now.weekday.digit.lo);
        Serial.print(';');

        Serial.print("U:");
        paddedPrint(now.hour);
        Serial.print('.');
        paddedPrint(now.minute);
        Serial.print('.');
        paddedPrint(now.second);
        Serial.print(';');

        uint8_t state = DCF77_Clock::get_clock_state();
        Serial.print(
            state == DCF77::useless || state == DCF77::dirty? '#'  // not synced
                                                            : ' '  // good
        );

        Serial.print(
            state == DCF77::synced || state == DCF77::locked? ' '  // DCF77
                                                            : '*'  // crystal clock
        );

        Serial.print(now.uses_summertime? 'S': ' ');
        Serial.print(
            now.timezone_change_scheduled? '!':
            now.leap_second_scheduled?     'A':
                                           ' '
        );

        Serial.print(ETX);
    }
}