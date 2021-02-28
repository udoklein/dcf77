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

// which pin the clock module is connected to
const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = 19; // A5
// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_inverted_samples = 0;

// The Blinkenlighty requires 1 this because the input
// pins are loaded with LEDs. All others should prefer
// setting this to 0 as this reduces interrupt contention.
const uint8_t dcf77_analog_samples = 0;

const uint8_t dcf77_monitor_led = 18;

#define ledpin(led) (led)
#else
// different pin settings for ARM based arduino
const uint8_t dcf77_inverted_samples = 0;

// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

// Pollin module
#define POLLIN_DCF77 1
const uint8_t pon_pin  = 51; // connect pon to ground !!!
const uint8_t data_pin = 53;
const uint8_t gnd_pin  = 51;
const uint8_t vcc_pin  = 49;

const uint8_t dcf77_sample_pin = data_pin;

const uint8_t dcf77_monitor_led = 19;

#define ledpin(led) (led<14? led: led+(54-14))
#endif

const uint8_t dcf77_monitor_pin = ledpin(18); // A4

const bool provide_filtered_output = true;
const uint8_t dcf77_filtered_pin          = ledpin(12);
const uint8_t dcf77_inverted_filtered_pin = ledpin(11);
const uint8_t dcf77_filter_diff_pin       = ledpin(7);

const bool provide_semi_synthesized_output = true;
const uint8_t dcf77_semi_synthesized_pin          = ledpin(16);
const uint8_t dcf77_inverted_semi_synthesized_pin = ledpin(15);
const uint8_t dcf77_semi_synthesized_diff_pin     = ledpin(14);

const bool provide_synthesized_output = true;
const uint8_t dcf77_synthesized_pin          = ledpin(6);
const uint8_t dcf77_inverted_synthesized_pin = ledpin(5);
const uint8_t dcf77_synthesized_diff_pin     = ledpin(4);

const uint8_t dcf77_second_pulse_pin = ledpin(10);


const uint8_t dcf77_signal_good_indicator_pin = ledpin(13);

volatile uint16_t ms_counter = 0;
volatile Internal::DCF77::tick_t tick = Internal::DCF77::undefined;


namespace {
    template <bool enable, uint8_t threshold,
            uint8_t filtered_pin, uint8_t inverted_filtered_pin, uint8_t diff_pin>
    void set_output(uint8_t clock_state, uint8_t sampled_data, uint8_t synthesized_signal)
    {
        if (enable) {
            const uint8_t filtered_output = clock_state < threshold? sampled_data: synthesized_signal;
            digitalWrite(filtered_pin, filtered_output);
            digitalWrite(inverted_filtered_pin, !filtered_output);
            digitalWrite(diff_pin, filtered_output ^ sampled_data);
        }
    }

    struct Scope {
        static const uint16_t samples_per_second = 1000;
        static const uint8_t bins                = 100;
        static const uint8_t samples_per_bin     = samples_per_second / bins;

        volatile uint8_t gbin[bins];
        volatile boolean samples_pending = false;
        volatile uint32_t count = 0;

        uint8_t  sbin[bins];
        uint16_t ticks = 999;
        void process_one_sample(const uint8_t sample) {
            ++ticks;

            if (ticks == 1000) {
                ticks = 0;
                memcpy((void *)gbin, sbin, bins);
                memset(sbin, 0, bins);
                samples_pending = true;
                ++count;
            }
            sbin[ticks/samples_per_bin] += sample;
        }

        void print() {
            uint8_t lbin[bins];

            if (samples_pending) {
                noInterrupts();
                memcpy(lbin, (void *)gbin, bins);
                samples_pending = false;
                interrupts();

                // ensure the count values will be aligned to the right
                for (int32_t val=count; val < 100000000; val *= 10) {
                    Serial.print(' ');
                }
                Serial.print((int32_t)count);
                Serial.print(", ");
                for (uint8_t bin=0; bin<bins; ++bin) {
                    switch (lbin[bin]) {
                        case  0: Serial.print(bin%10? '-': '+'); break;
                        case 10: Serial.print('X');              break;
                        default: Serial.print(lbin[bin]);
                    }
                }
                Serial.println();
            }
        }
    };
}

Scope scope_1;
Scope scope_2;

uint8_t sample_input_pin() {
    const uint8_t clock_state = DCF77_Clock::get_clock_state();
    const uint8_t sampled_data =
        #if defined(__AVR__)
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));
        #else
        dcf77_inverted_samples ^ digitalRead(dcf77_sample_pin);
        #endif

    digitalWrite(dcf77_monitor_pin, sampled_data);
    digitalWrite(dcf77_second_pulse_pin, ms_counter < 500 && clock_state >= Clock::locked);

    const uint8_t synthesized_signal =
        tick == Internal::DCF77::long_tick  ? ms_counter < 200:
        tick == Internal::DCF77::short_tick ? ms_counter < 100:
        tick == Internal::DCF77::sync_mark  ? 0:
                                              // tick == DCF77::undefined --> default handling
                                              // allow signal to pass for the first 200ms of each second
                                              (ms_counter <=200 && sampled_data) ||
                                              // if the clock has valid time data then undefined ticks
                                              // are data bits --> first 100ms of signal must be high
                                              ms_counter <100;

    set_output<provide_filtered_output, Clock::locked,
              dcf77_filtered_pin, dcf77_inverted_filtered_pin, dcf77_filter_diff_pin>
              (clock_state, sampled_data, synthesized_signal);

    set_output<provide_semi_synthesized_output, Clock::unlocked,
               dcf77_semi_synthesized_pin, dcf77_inverted_semi_synthesized_pin, dcf77_semi_synthesized_diff_pin>
               (clock_state, sampled_data, synthesized_signal);

    set_output<provide_synthesized_output, Clock::free,
               dcf77_synthesized_pin, dcf77_inverted_synthesized_pin, dcf77_synthesized_diff_pin>
               (clock_state, sampled_data, synthesized_signal);

    ms_counter+= (ms_counter < 1000);

    scope_1.process_one_sample(sampled_data);
    scope_2.process_one_sample(digitalRead(dcf77_synthesized_pin));

    return sampled_data;
}



void output_handler(const Clock::time_t &decoded_time) {
    // reset ms_counter for 1 Hz ticks
    ms_counter = 0;

    // status indicator --> always on if signal is good
    //                      blink 3s on 1s off if signal is poor
    //                      blink 1s on 3s off if signal is very poor
    //                      always off if signal is bad
    const uint8_t clock_state = DCF77_Clock::get_clock_state();
    digitalWrite(dcf77_signal_good_indicator_pin,
                 clock_state >= Clock::locked  ? 1:
                 clock_state == Clock::unlocked? (decoded_time.second.digit.lo & 0x03) != 0:
                 clock_state == Clock::free    ? (decoded_time.second.digit.lo & 0x03) == 0:
                                                  0);
    // compute output for signal synthesis
    Internal::DCF77_Encoder now;
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

    now.undefined_minute_output                         = false;
    now.undefined_uses_summertime_output                = false;
    now.undefined_abnormal_transmitter_operation_output = false;
    now.undefined_timezone_change_scheduled_output      = false;

    now.advance_minute();
    tick = now.get_current_signal();
}

void setup_serial() {
    Serial.begin(115200);
}

void output_splash_screen() {
    Serial.println();
    Serial.println(F("DCF77 Superfilter 3.1.1"));
    Serial.println(F("(c) 2016 Udo Klein"));
    Serial.println(F("www.blinkenlight.net"));
    Serial.println();
    Serial.print(F("Sample Pin:               ")); Serial.println(dcf77_sample_pin);
    Serial.print(F("Sample Pin Mode:          ")); Serial.println(dcf77_pin_mode);
    Serial.print(F("Inverted Mode:            ")); Serial.println(dcf77_inverted_samples);
    #if defined(__AVR__)
    Serial.print(F("Analog Mode:              ")); Serial.println(dcf77_analog_samples);
    #endif
    Serial.print(F("Monitor Pin:              ")); Serial.println(dcf77_monitor_pin);
    Serial.println();

    if (provide_filtered_output) {
        Serial.println(F("Filtered Output"));
        Serial.print(F("    Filtered Pin:         ")); Serial.println(dcf77_filtered_pin);
        Serial.print(F("    Diff Pin:             ")); Serial.println(dcf77_filter_diff_pin);
        Serial.print(F("    Inverse Filtered Pin: ")); Serial.println(dcf77_inverted_filtered_pin);
        Serial.println();
    }

    if (provide_semi_synthesized_output) {
        Serial.println(F("Semi Synthesized Output"));
        Serial.print(F("    Filtered Pin:         ")); Serial.println(dcf77_semi_synthesized_pin);
        Serial.print(F("    Diff Pin:             ")); Serial.println(dcf77_semi_synthesized_diff_pin);
        Serial.print(F("    Inverse Filtered Pin: ")); Serial.println(dcf77_inverted_semi_synthesized_pin);
        Serial.println();
    }

    if (provide_synthesized_output) {
        Serial.println(F("Synthesized Output"));
        Serial.print(F("    Filtered Pin:         ")); Serial.println(dcf77_synthesized_pin);
        Serial.print(F("    Diff Pin:             ")); Serial.println(dcf77_synthesized_diff_pin);
        Serial.print(F("    Inverse Filtered Pin: ")); Serial.println(dcf77_inverted_synthesized_pin);
        Serial.println();
    }

    Serial.print(F("Second Pulse Pin:         ")); Serial.println(dcf77_second_pulse_pin);
    Serial.print(F("Signal Good Pin:          ")); Serial.println(dcf77_signal_good_indicator_pin);

    Serial.println();

    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.println();
};

void setup_pins() {
    if (provide_filtered_output) {
        pinMode(dcf77_filtered_pin, OUTPUT);
        pinMode(dcf77_filter_diff_pin, OUTPUT);
        pinMode(dcf77_inverted_filtered_pin, OUTPUT);
    }

    if (provide_semi_synthesized_output) {
        pinMode(dcf77_semi_synthesized_pin, OUTPUT);
        pinMode(dcf77_semi_synthesized_diff_pin, OUTPUT);
        pinMode(dcf77_inverted_semi_synthesized_pin, OUTPUT);
    }

    if (provide_synthesized_output) {
        pinMode(dcf77_synthesized_pin, OUTPUT);
        pinMode(dcf77_synthesized_diff_pin, OUTPUT);
        pinMode(dcf77_inverted_synthesized_pin, OUTPUT);
    }

    pinMode(dcf77_monitor_pin, OUTPUT);
    pinMode(dcf77_signal_good_indicator_pin, OUTPUT);
    pinMode(dcf77_second_pulse_pin, OUTPUT);
    pinMode(dcf77_sample_pin, dcf77_pin_mode);
}

void setup_clock() {
    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);
    DCF77_Clock::set_output_handler(output_handler);
}

void setup() {
    setup_serial();
    output_splash_screen();
    setup_pins();
    setup_clock();

#if defined(POLLIN_DCF77)
    pinMode(gnd_pin, OUTPUT);
    digitalWrite(gnd_pin, LOW);
    pinMode(pon_pin, OUTPUT);
    digitalWrite(pon_pin, LOW);
    pinMode(vcc_pin, OUTPUT);
    digitalWrite(vcc_pin, HIGH);
#endif
}

void print_clock_state() {
    switch (DCF77_Clock::get_clock_state()) {
        case Clock::useless:  Serial.print(F("useless:  ")); break;
        case Clock::dirty:    Serial.print(F("dirty:    ")); break;
        case Clock::free:     Serial.print(F("free:     ")); break;
        case Clock::unlocked: Serial.print(F("unlocked: ")); break;
        case Clock::locked:   Serial.print(F("locked:   ")); break;
        case Clock::synced:   Serial.print(F("synced:   ")); break;
    }
}

void loop() {
    Clock::time_t now;
    DCF77_Clock::get_current_time(now);

    if (now.month.val > 0) {
        Serial.println();
        Serial.print(F("Decoded time: "));

        DCF77_Clock::print(now);
        Serial.println();
    }

    print_clock_state();
    Serial.print(' ');
    DCF77_Clock::debug();

    Serial.print(F("[1] ")); scope_1.print();
    Serial.print(F("[2] ")); scope_2.print();
}
