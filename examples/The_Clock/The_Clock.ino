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
const uint8_t dcf77_sample_pin = 19; // A5
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 1;

const uint8_t dcf77_monitor_pin = 18;  // A4

const uint8_t lower_output_pin = 2;
const uint8_t upper_output_pin = 17;

const uint8_t dcf77_rolling_monitor_lower_pin = 4;
const uint8_t dcf77_rolling_monitor_upper_pin = 13;


uint8_t counter = 0;
uint8_t rolling_pin = dcf77_rolling_monitor_lower_pin;


void reset_output_pins() {
    for (uint8_t pin = lower_output_pin; pin <= upper_output_pin; ++pin) {
        digitalWrite(pin, LOW);
    }
}

void setup_output_pins() {
    for (uint8_t pin = lower_output_pin; pin <= upper_output_pin; ++pin) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
}

uint8_t sample_input_pin() {
    const uint8_t sampled_data =
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));

    digitalWrite(dcf77_monitor_pin, sampled_data);
    return sampled_data;
}

namespace MaxTicks {
    void output_handler(const DCF77_Clock::time_t &decoded_time) {
        rolling_pin = lower_output_pin;
        counter = 0;
    }
    
    uint8_t sample_input_pin() {
        static uint8_t ticks_per_cycle = 12;

        // reuse the sample code from the global namespace
        uint8_t sampled_data = ::sample_input_pin();
        
        // additional code for the tick visualization
        digitalWrite(rolling_pin, sampled_data);
        
        if (counter < ticks_per_cycle) {
            ++counter;
        } else {
            rolling_pin = rolling_pin < upper_output_pin? rolling_pin + 1: lower_output_pin;
            counter = 1;
            // toggle between 12 and 13 to get 12.5 on average
            ticks_per_cycle = 25-ticks_per_cycle;
        }
        
        return sampled_data;
    }
    
    void setup() {
        reset_output_pins();
        DCF77_Clock::set_input_provider(sample_input_pin);
        DCF77_Clock::set_output_handler(output_handler);
    }
}

namespace Ticks {
    void output_handler(const DCF77_Clock::time_t &decoded_time) {
        rolling_pin = dcf77_rolling_monitor_lower_pin;
        counter = 0;
    }
    
    uint8_t sample_input_pin() {
        // reuse the sample code from the global namespace
        uint8_t sampled_data = ::sample_input_pin();
        
        // additional code for the tick visualization
        digitalWrite(rolling_pin, sampled_data);
        
        if (counter < 20) {
            ++counter;
        } else {
            rolling_pin = rolling_pin < dcf77_rolling_monitor_upper_pin? rolling_pin + 1: dcf77_rolling_monitor_lower_pin;
            counter = 1;
        }
        
        return sampled_data;
    }
    
    void setup() {
        reset_output_pins();
        DCF77_Clock::set_input_provider(sample_input_pin);
        DCF77_Clock::set_output_handler(output_handler);    
    }
}

namespace Seconds {
    void output_handler(const DCF77_Clock::time_t &decoded_time) {
        uint8_t out = decoded_time.second.val;
        uint8_t pin = lower_output_pin + 3;
        for (uint8_t bit=0; bit<8; ++bit) {
            digitalWrite(pin++, out & 0x1);
            out >>= 1;
            
            if (bit==3) {
                ++pin;
            }
        }
    }
    
    void setup() {
        reset_output_pins();
        DCF77_Clock::set_input_provider(sample_input_pin);
        DCF77_Clock::set_output_handler(output_handler);
    }
}

namespace Hours_and_Minutes {
    void output_handler(const DCF77_Clock::time_t &decoded_time) {
        uint8_t pin = lower_output_pin;
        
        uint8_t out = decoded_time.minute.val;
        for (uint8_t bit=0; bit<7; ++bit) {
            digitalWrite(pin++, out & 0x1);
            out >>= 1;
        }
        ++pin;
        
        out = decoded_time.hour.val;
        for (uint8_t bit=0; bit<6; ++bit) {
            digitalWrite(pin++, out & 0x1);
            out >>= 1;
        }
    }
    
    void setup() {
        reset_output_pins();
        DCF77_Clock::set_input_provider(sample_input_pin);
        DCF77_Clock::set_output_handler(output_handler);
    }
}

namespace Months_and_Days {
    void output_handler(const DCF77_Clock::time_t &decoded_time) {
        uint8_t pin = lower_output_pin;
        
        uint8_t out = decoded_time.day.val;
        for (uint8_t bit=0; bit<6; ++bit) {
            digitalWrite(pin++, out & 0x1);
            out >>= 1;
        }
        ++pin;
        
        out = decoded_time.month.val;
        for (uint8_t bit=0; bit<5; ++bit) {
            digitalWrite(pin++, out & 0x1);
            out >>= 1;
        }
    }
    
    void setup() {
        reset_output_pins();
        DCF77_Clock::set_input_provider(sample_input_pin);
        DCF77_Clock::set_output_handler(output_handler);
    }
}


void help() {
    Serial.println();
    Serial.println(F("use serial interface to alter display settings"));
    Serial.println(F("  t --> 10 ticks"));
    Serial.println(F("  T --> 16 ticks"));
    Serial.println(F("  s --> BCD seconds"));
    Serial.println(F("  h --> BCD hours and minutes"));
    Serial.println(F("  m --> BCD months and days"));
    Serial.println();
}

void setup() {
    using namespace DCF77_Encoder;
    
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("DCF77 Clock V1.0"));
    Serial.println(F("(c) Udo Klein 2014"));
    Serial.println(F("www.blinkenlight.net"));
    Serial.println();
    Serial.print(F("Sample Pin:    ")); Serial.println(dcf77_sample_pin);
    Serial.print(F("Inverted Mode: ")); Serial.println(dcf77_inverted_samples);
    Serial.print(F("Analog Mode:   ")); Serial.println(dcf77_analog_samples);
    Serial.print(F("Monitor Pin:   ")); Serial.println(dcf77_monitor_pin);
    Serial.println();
    help();
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.println();
    
    pinMode(dcf77_monitor_pin, OUTPUT);
    
    pinMode(dcf77_sample_pin, INPUT);
    digitalWrite(dcf77_sample_pin, HIGH);
    
    setup_output_pins();
    
    DCF77_Clock::setup();
    MaxTicks::setup();
    
}

void loop() {
    if (Serial.available()) {
        switch (Serial.read()) {
            case 't': Ticks::setup(); break;
            case 'T': MaxTicks::setup(); break;
            case 's': Seconds::setup(); break;
            case 'h': Hours_and_Minutes::setup(); break;
            case 'm': Months_and_Days::setup(); break;
            
            default: help();
        }
    }
    
    DCF77_Clock::time_t now;
    
    DCF77_Clock::get_current_time(now);
    
    if (now.month.val > 0) {
        Serial.println();
        Serial.print(F("Decoded time: "));
        
        DCF77_Clock::print(now);
        Serial.println();
    }
    
    DCF77_Clock::debug();
    //DCF77_Second_Decoder::debug();
    DCF77_Local_Clock::debug();
}
