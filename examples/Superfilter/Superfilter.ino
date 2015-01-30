//
//  www.blinkenlight.net
//
//  Copyright 2015 Udo Klein
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
const uint8_t dcf77_sample_pin = 19; // A5
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 1;

const uint8_t dcf77_monitor_pin = 18; // A4

const uint8_t dcf77_filtered_pin = 12;
const uint8_t dcf77_inverse_filtered_pin = 11;
const uint8_t dcf77_second_pulse_pin = 10;

const uint8_t dcf77_diff_pin = 7;

const uint8_t dcf77_signal_good_indicator_pin = 3;

volatile uint16_t ms_counter = 0;
volatile DCF77::tick_t tick = DCF77::undefined;

uint8_t sample_input_pin() {
    const uint8_t clock_state = DCF77_Clock::get_clock_state();
    const uint8_t sampled_data =
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));

    digitalWrite(dcf77_monitor_pin, sampled_data);

    digitalWrite(dcf77_second_pulse_pin, ms_counter < 500 && clock_state>=DCF77_Clock::locked);

    const uint8_t defensive_output =
        clock_state<=DCF77_Clock::locked? sampled_data:
        tick == DCF77::long_tick        ? ms_counter < 200:
        tick == DCF77::short_tick       ? ms_counter < 100:
        tick == DCF77::sync_mark        ?  0:
                                           // tick == DCF77::undefined --> default handling
                                           // allow signal to pass for the first 200ms of each second
                                           ms_counter <=200 && sampled_data ||
                                           // if the clock has valid time data then undefined ticks
                                           // are data bits --> first 100ms of signal must be high
                                           ms_counter <100;

    digitalWrite(dcf77_filtered_pin, defensive_output);
    digitalWrite(dcf77_inverse_filtered_pin, !defensive_output);

    digitalWrite(dcf77_diff_pin, defensive_output ^ sampled_data);

    ms_counter+= (ms_counter < 1000);
    return sampled_data;
}


void output_handler(const DCF77_Clock::time_t &decoded_time) {
    // reset ms_counter for 1 Hz ticks
    ms_counter = 0;

    // status indicator --> always on if signal is good
    //                      blink 3s on 1s off if signal is poor
    //                      blink 1s on 3s off if signal is very poor
    //                      always off if signal is bad
    const uint8_t clock_state = DCF77_Clock::get_clock_state();
    digitalWrite(dcf77_signal_good_indicator_pin,
                 clock_state >= DCF77_Clock::locked  ? 1:
                 clock_state == DCF77_Clock::unlocked? (decoded_time.second.digit.lo & 0x03) != 0:
                 clock_state == DCF77_Clock::free    ? (decoded_time.second.digit.lo & 0x03) == 0:
                                                       0);
    // compute output for signal synthesis
    DCF77::time_data_t now;
    now.second                    = BCD::bcd_to_int(decoded_time.second);
    now.minute                    = decoded_time.minute;
    now.hour                      = decoded_time.hour;
    now.weekday                   = decoded_time.weekday;
    now.day                       = decoded_time.day;
    now.month                     = decoded_time.month;
    now.year                      = decoded_time.year;
    now.uses_summertime           = decoded_time.uses_summertime;
    now.leap_second_scheduled     = decoded_time.leap_second_scheduled;
    now.timezone_change_scheduled = decoded_time.timezone_change_scheduled;

    DCF77_Encoder::advance_minute(now);

    tick = DCF77_Encoder::get_current_signal(now);
}

void setup() {
    using namespace DCF77_Encoder;

    Serial.begin(115200);
    Serial.println();
    Serial.println(F("DCF77 Superfilter"));
    Serial.println(F("(c) Udo Klein 2015"));
    Serial.println(F("www.blinkenlight.net"));
    Serial.println();
    Serial.print(F("Sample Pin:          ")); Serial.println(dcf77_sample_pin);
    Serial.print(F("Inverted Mode:       ")); Serial.println(dcf77_inverted_samples);
    Serial.print(F("Analog Mode:         ")); Serial.println(dcf77_analog_samples);
    Serial.print(F("Monitor Pin:         ")); Serial.println(dcf77_monitor_pin);
    Serial.print(F("Diff Pin:            ")); Serial.println(dcf77_diff_pin);
    Serial.print(F("Filtered Pin:        ")); Serial.println(dcf77_filtered_pin);
    Serial.print(F("Inverse Filtered Pin:")); Serial.println(dcf77_inverse_filtered_pin);
    Serial.print(F("Second Pulse Pin:    ")); Serial.println(dcf77_second_pulse_pin);
    Serial.print(F("Signal Good Pin:     ")); Serial.println(dcf77_signal_good_indicator_pin);

    Serial.println();

    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.println();

    pinMode(dcf77_monitor_pin, OUTPUT);
    pinMode(dcf77_filtered_pin, OUTPUT);
    pinMode(dcf77_inverse_filtered_pin, OUTPUT);
    pinMode(dcf77_diff_pin, OUTPUT);
    pinMode(dcf77_signal_good_indicator_pin, OUTPUT);
    pinMode(dcf77_second_pulse_pin, OUTPUT);
    pinMode(dcf77_sample_pin, INPUT);
    digitalWrite(dcf77_sample_pin, HIGH);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);
    DCF77_Clock::set_output_handler(output_handler);
}

void loop() {
    DCF77_Clock::time_t now;

    DCF77_Clock::get_current_time(now);

    if (now.month.val > 0) {
        Serial.println();
        Serial.print(F("Decoded time: "));

        DCF77_Clock::print(now);
        Serial.println();
    }

    DCF77_Clock::debug();
}
