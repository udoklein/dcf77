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

const int8_t timezone_offset = -1;  // GB is one hour behind CET/CEST


namespace Timezone {
    uint8_t days_per_month(const Clock::time_t &now) {
        switch (now.month.val) {
            case 0x02:
                // valid till 31.12.2399
                // notice year mod 4 == year & 0x03
                return 28 + ((now.year.val != 0) && ((bcd_to_int(now.year) & 0x03) == 0)? 1: 0);
            case 0x01: case 0x03: case 0x05: case 0x07: case 0x08: case 0x10: case 0x12: return 31;
            case 0x04: case 0x06: case 0x09: case 0x11:                                  return 30;
            default: return 0;
        }
    }

    void adjust(Clock::time_t &time, const int8_t offset) {
        // attention: maximum supported offset is +/- 23h

        int8_t hour = BCD::bcd_to_int(time.hour) + offset;

        if (hour > 23) {
            hour -= 24;
            uint8_t day = BCD::bcd_to_int(time.day) + 1;
            if (day > days_per_month(time)) {
                day = 1;
                uint8_t month = BCD::bcd_to_int(time.month);
                ++month;
                if (month > 12) {
                    month = 1;
                    uint8_t year = BCD::bcd_to_int(time.year);
                    ++year;
                    if (year > 99) {
                        year = 0;
                    }
                    time.year = BCD::int_to_bcd(year);
                }
                time.month = BCD::int_to_bcd(month);
            }
            time.day = BCD::int_to_bcd(day);
            time.weekday.val = time.weekday.val<7 ? time.weekday.val+1 : time.weekday.val-6;
        }

        if (hour < 0) {
            hour += 24;
            uint8_t day = BCD::bcd_to_int(time.day) - 1;
            if (day < 1) {
                uint8_t month = BCD::bcd_to_int(time.month);
                --month;
                if (month < 1) {
                    month = 12;
                    int8_t year = BCD::bcd_to_int(time.year);
                    --year;
                    if (year < 0) {
                        year = 99;
                    }
                    time.year = BCD::int_to_bcd(year);
                }
                time.month = BCD::int_to_bcd(month);
                day = days_per_month(time);
            }
            time.day = BCD::int_to_bcd(day);
            time.weekday.val = time.weekday.val>1 ? time.weekday.val-1 : time.weekday.val+6;
        }

        time.hour = BCD::int_to_bcd(hour);
    }
}

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
    Serial.begin(9600);
    Serial.println();
    Serial.println(F("Simple DCF77 Clock V3.1.1"));
    Serial.println(F("(c) Udo Klein 2016"));
    Serial.println(F("www.blinkenlight.net"));
    Serial.println();
    Serial.print(F("Sample Pin:      ")); Serial.println(dcf77_sample_pin);
    Serial.print(F("Sample Pin Mode: ")); Serial.println(dcf77_pin_mode);
    Serial.print(F("Inverted Mode:   ")); Serial.println(dcf77_inverted_samples);
    #if defined(__AVR__)
    Serial.print(F("Analog Mode:   ")); Serial.println(dcf77_analog_samples);
    #endif
    Serial.print(F("Monitor Pin:   ")); Serial.println(ledpin(dcf77_monitor_led));
    Serial.print(F("Timezone Offset: ")); Serial.println(timezone_offset);
    Serial.println();
    Serial.println();
    Serial.println(F("Initializing..."));

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
        Serial.print('.');
        ++count;
        if (count == 60) {
            count = 0;
            Serial.println();
        }
    }
    Serial.println();
}

void paddedPrint(BCD::bcd_t n) {
    Serial.print(n.digit.hi);
    Serial.print(n.digit.lo);
}

void loop() {
    Clock::time_t now;

    DCF77_Clock::get_current_time(now);
    Timezone::adjust(now, timezone_offset);

    if (now.month.val > 0) {
        switch (DCF77_Clock::get_clock_state()) {
            case Clock::useless: Serial.print(F("useless ")); break;
            case Clock::dirty:   Serial.print(F("dirty:  ")); break;
            case Clock::synced:  Serial.print(F("synced: ")); break;
            case Clock::locked:  Serial.print(F("locked: ")); break;
        }

        switch (now.weekday.val) {
            case 1: Serial.print(F("Monday   ")); break;
            case 2: Serial.print(F("Tuesday  ")); break;
            case 3: Serial.print(F("Wednesday")); break;
            case 4: Serial.print(F("Thursday ")); break;
            case 5: Serial.print(F("Friday   ")); break;
            case 6: Serial.print(F("Saturday ")); break;
            case 7: Serial.print(F("Sunday   ")); break;
        }
        Serial.print(F(" 20"));
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

        const int8_t offset_to_utc = timezone_offset + (now.uses_summertime? 2: 1);
        Serial.print(F(" UTC"));
        Serial.print(offset_to_utc<0? '-':'+');
        if (abs(offset_to_utc) < 10) {
            Serial.print('0');
        }
        Serial.println(abs(offset_to_utc));
    }
}
