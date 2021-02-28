// //
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
#include <avr/eeprom.h>

// do not use 0 as this will interfere with the DCF77 lib's EEPROM_base
const uint16_t EEPROM_base = 0x20;

// which pin the clock module is connected to
const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = 19; // A5
// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up


const uint8_t dcf77_inverted_samples = 1;

// The Blinkenlighty requires 1 this because the input
// pins are loaded with LEDs. All others should prefer
// setting this to 0 as this reduces interrupt contention.
const uint8_t dcf77_analog_samples = 0;

const uint8_t dcf77_monitor_led = 18;

uint8_t ledpin(const uint8_t led) {
    return led;
}

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

uint8_t ledpin(const uint8_t led) {
    return led<14? led: led+(54-14);
}
#endif


using namespace Internal;
typedef DCF77_Clock_Controller<Configuration, DCF77_Frequency_Control> Clock_Controller;


namespace Phase_Drift_Analysis {
    volatile uint16_t phase = 0;
    volatile uint16_t noise = 0;
}

namespace LED_Display {
    // which led to use for monitor output
    const uint8_t dcf77_monitor_led = ::dcf77_monitor_led;

    // which leds to use for monitoring lightshow
    const uint8_t lower_output_led =  2;
    const uint8_t upper_output_led = 17;

    int8_t counter = 0;
    uint8_t rolling_led = lower_output_led;


    void reset_output_leds() {
        for (uint8_t led = lower_output_led; led <= upper_output_led; ++led) {
            digitalWrite(ledpin(led), LOW);
        }
    }

    void setup_output_leds() {
        for (uint8_t led = lower_output_led; led <= upper_output_led; ++led) {
            pinMode(ledpin(led), OUTPUT);
            digitalWrite(ledpin(led), LOW);
        }
    }

    void setup() {
        pinMode(ledpin(dcf77_monitor_led), OUTPUT);
        setup_output_leds();
    }


    volatile char mode = 't';
    void set_mode(const char c) {
        Serial.print(F("set LED mode: ")); Serial.println(c);
        if (c != mode) {
            // mode assignment must be before reset in order
            // to avoid race conditions
            mode = c;
            reset_output_leds();
        }
    }
    char get_mode() { return mode; }

    void monitor(const uint8_t sampled_data) {
        digitalWrite(ledpin(dcf77_monitor_led), sampled_data);

        switch (mode) {
            case '2':    // 200 ms
            case 't':  { // ticks
                const uint8_t ticks_per_cycle_nominator   = 25;
                const uint8_t ticks_per_cycle_denominator = 2;

                if (rolling_led <= upper_output_led) {
                    digitalWrite(ledpin(rolling_led), sampled_data);
                }

                counter += ticks_per_cycle_denominator;
                if (counter >= ticks_per_cycle_nominator) {
                    rolling_led = (rolling_led < upper_output_led ||
                                  (mode == '2' && rolling_led <= (1000 * ticks_per_cycle_denominator)/ ticks_per_cycle_nominator))
                                       ? rolling_led + 1: lower_output_led;
                    counter -= ticks_per_cycle_nominator;

                    if (mode=='2' && rolling_led <= upper_output_led) {
                        digitalWrite(ledpin(rolling_led), !sampled_data);
                    }
                }
            }
            break;

            case 'f':  { // flash
                for (uint8_t led = lower_output_led; led <= upper_output_led; ++led) {
                    digitalWrite(ledpin(led), sampled_data);
                }
            }
            break;
        }
    }

    void output_handler(const Clock::time_t &decoded_time) {
        switch (mode) {
            case '2':
            case 't':  // ticks
                rolling_led = lower_output_led;
                counter = 0;
                break;

            case 's':  { // seconds
                uint8_t out = decoded_time.second.val;
                uint8_t led = lower_output_led + 3;
                for (uint8_t bit=0; bit<8; ++bit) {
                    digitalWrite(ledpin(led++), out & 0x1);
                    out >>= 1;

                    if (bit==3) {
                        ++led;
                    }
                }
                break;
            }
            case 'h': { // hours and minutes
                uint8_t led = lower_output_led;

                uint8_t out = decoded_time.minute.val;
                for (uint8_t bit=0; bit<7; ++bit) {
                    digitalWrite(ledpin(led++), out & 0x1);
                    out >>= 1;
                }
                ++led;

                out = decoded_time.hour.val;
                for (uint8_t bit=0; bit<6; ++bit) {
                    digitalWrite(ledpin(led++), out & 0x1);
                    out >>= 1;
                }
                break;
            }
            case 'm': { // months and days
                uint8_t led = lower_output_led;

                uint8_t out = decoded_time.day.val;
                for (uint8_t bit=0; bit<6; ++bit) {
                    digitalWrite(ledpin(led++), out & 0x1);
                    out >>= 1;
                }
                ++led;

                out = decoded_time.month.val;
                for (uint8_t bit=0; bit<5; ++bit) {
                    digitalWrite(ledpin(led++), out & 0x1);
                    out >>= 1;
                }
                break;
            }

            case 'a': { // analyze phase drift
                for (uint8_t bit=0; bit<10; ++bit) {
                    const uint8_t pm10 = Phase_Drift_Analysis::phase % 10;
                    digitalWrite(ledpin(lower_output_led+bit), pm10==bit);
                }
                break;
            }

            case 'c': { // calibration state + deviation
                const DCF77_Frequency_Control::calibration_state_t calibration_state = DCF77_Frequency_Control::get_calibration_state();
                int16_t deviation = abs(DCF77_Frequency_Control::get_current_deviation());
                uint8_t led = lower_output_led;

                // display calibration state, blink if running unqualified
                digitalWrite(ledpin(led++), calibration_state.qualified);
                digitalWrite(ledpin(led++), calibration_state.running && !calibration_state.qualified && (decoded_time.second.val & 1));
                digitalWrite(ledpin(led++), calibration_state.running);

                // render the absolute deviation in binary
                while (led < upper_output_led) {
                    digitalWrite(ledpin(led), deviation & 1);
                    deviation >>= 1;
                    ++led;
                }
            }
        }
    }
}

namespace Scope {
    const uint16_t samples_per_second = 1000;
    const uint8_t bins                = 100;
    const uint8_t samples_per_bin     = samples_per_second / bins;

    volatile uint8_t gbin[bins];
    volatile boolean samples_pending = false;
    volatile uint32_t count = 0;

    void process_one_sample(const uint8_t sample) {
        static uint8_t sbin[bins];

        static uint16_t ticks = 999;  // first pass will init the bins
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
}

namespace High_Resolution_Scope {
    uint16_t tick = 999;

    void print(const uint8_t sampled_data) {
        ++tick;
        if (tick == 1000) {
            tick = 0;
            Serial.println();
        }

        Serial.print(sampled_data? 'X':
                     (tick % 100)? '-':
                                   '+');
    }
}

namespace Raw {
    void print(const uint8_t sampled_data) {
        Serial.println(sampled_data);
    }
}

namespace Phase_Drift_Analysis {
    using namespace LED_Display;

    volatile uint16_t counter = 1000;
    volatile uint16_t ref_counter = 0;
    volatile uint16_t noise_detector = 0;
    volatile uint16_t noise_ticks = 0;
    volatile uint16_t phase_detector = 0;
    volatile uint8_t  phase_ticks = 0;
    volatile uint16_t sample_count = 0;

    void restart() {
        counter -= 1000;
        ref_counter = 0;
    }

    void process_one_sample(uint8_t sampled_data) {
        ++counter;
        ++ref_counter;

        if (ref_counter == 950) {
            phase_detector = 0;
            phase_ticks = 0;
        }
        if (ref_counter == 300) {
            noise_detector = 0;
            noise_ticks = 0;
        }
        if (ref_counter >= 950 || ref_counter < 50) {
            phase_detector += sampled_data;
            ++phase_ticks;
        }
        if (ref_counter == 50) {
            phase = phase_detector;
            noise = noise_detector;
        }

        if (ref_counter >= 300 && ref_counter < 900) {
            noise_detector += sampled_data;
            ++noise_ticks;
        }
    }

    void debug() {
        Serial.print(Phase_Drift_Analysis::phase);
        Serial.print('/');
        Serial.print(100);
        Serial.print('~');
        Serial.print(Phase_Drift_Analysis::noise);
        Serial.print('/');
        Serial.println(600);
    }
}

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
        }

        time.hour = BCD::int_to_bcd(hour);
    }
}

void paddedPrint(BCD::bcd_t n) {
    Serial.print(n.digit.hi);
    Serial.print(n.digit.lo);
}

char mode = 'd';
void set_mode(const char mode) {
    Serial.print(F("set mode: ")); Serial.println(mode);
    ::mode = mode;
}
char get_mode() { return mode; }

uint8_t sample_input_pin() {
    const uint8_t sampled_data =
    #if defined(__AVR__)
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));
    #else
        dcf77_inverted_samples ^ digitalRead(dcf77_sample_pin);
    #endif
    // computations must be before display code
    Scope::process_one_sample(sampled_data);
    Phase_Drift_Analysis::process_one_sample(sampled_data);

    LED_Display::monitor(sampled_data);


    if (mode == 'r') {
        Raw::print(sampled_data);
    } else
    if (mode == 'S') {
        High_Resolution_Scope::print(sampled_data);
    }

    return sampled_data;
}

void output_handler(const Clock::time_t &decoded_time) {
    Phase_Drift_Analysis::restart();
    LED_Display::output_handler(decoded_time);
}

/*
void free_dump() {

    uint8_t *heapptr;
    uint8_t *stackptr;

    stackptr = (uint8_t *)malloc(4);   // use stackptr temporarily
    heapptr = stackptr;                // save value of heap pointer
    free(stackptr);                    // free up the memory again (sets stackptr to 0)
    stackptr =  (uint8_t *)(SP);       // save value of stack pointer


    // print("HP: ");
    Serial.print(F("HP: "));
    Serial.println((int) heapptr, HEX);

    // print("SP: ");
    Serial.print(F("SP: "));
    Serial.println((int) stackptr, HEX);

    // print("Free: ");
    Serial.print(F("Free: "));
    Serial.println((int) stackptr - (int) heapptr, HEX);
    Serial.println();
}
*/

namespace Parser {
    #if defined(_AVR_EEPROM_H_)
    // ID constants to see if EEPROM has already something stored
    const char ID_u = 'u';
    const char ID_k = 'k';

    void persist_to_EEPROM() {
        uint16_t eeprom = EEPROM_base;
        eeprom_write_byte((uint8_t *)(eeprom++), ID_u);
        eeprom_write_byte((uint8_t *)(eeprom++), ID_k);
        eeprom_write_byte((uint8_t *)(eeprom++), ::get_mode());
        eeprom_write_byte((uint8_t *)(eeprom++), LED_Display::get_mode());
        Serial.println(F("modes persisted to eeprom"));
    }

    void restore_from_EEPROM() {
        uint16_t eeprom = EEPROM_base;
        if (eeprom_read_byte((const uint8_t *)(eeprom++)) == ID_u &&
            eeprom_read_byte((const uint8_t *)(eeprom++)) == ID_k) {
            ::set_mode(eeprom_read_byte((const uint8_t *)(eeprom++)));
            LED_Display::set_mode(eeprom_read_byte((const uint8_t *)(eeprom++)));
            Serial.println(F("modes restored from eeprom"));
        }
    }
    #endif

    void help() {
        Serial.println();
        Serial.println(F("use serial interface to alter settings"));
        Serial.println(F("  L: led output modes"));
        Serial.println(F("    q: quiet"));
        Serial.println(F("    f: flash"));
        Serial.println(F("    t: ticks"));
        Serial.println(F("    2: 200 ms of the signal"));
        Serial.println(F("    s: BCD seconds"));
        Serial.println(F("    h: BCD hours and minutes"));
        Serial.println(F("    m: BCD months and days"));
        Serial.println(F("    a: analyze phase drift"));
        Serial.println(F("    c: calibration state + deviation"));
        Serial.println(F("  D: debug modes"));
        Serial.println(F("    q: quiet"));
        Serial.println(F("    d: debug quality factors"));
        Serial.println(F("    s: scope"));
        Serial.println(F("    S: scope high resolution"));
        Serial.println(F("    m: multi mode debug + scope"));
        Serial.println(F("    a: analyze frequency"));
        Serial.println(F("    A: Analyze phase drift, more details"));
        Serial.println(F("    b: demodulator bins"));
        Serial.println(F("    B: detailed demodulator bins"));
        Serial.println(F("    r: raw output"));
        Serial.println(F("    c: CET/CEST"));
        Serial.println(F("    u: UTC"));
        #if defined(_AVR_EEPROM_H_)
        Serial.println(F("  *: persist current modes to EEPROM"));
        Serial.println(F("  ~: restore modes from EEPROM"));
        #endif
        Serial.println();
    }

    void help_on_none_space(const char c) {
        if (c!=' ' && c!='\n' && c!='\r') {
            help();
        }
    }

    // The parser will deliver output in two different ways
    //     1) synchronous as a return value
    //     2) as a side effect to the LED display
    // We are lazy with the command mapping, that is the parser will
    // not map anything. The commands are fed directly from the parser
    // to the consumers. This of course increases coupling.
    void parse() {
        enum mode { waiting=0, led_display_command, debug_output_command};

        static mode parser_mode = waiting;

        if (Serial.available()) {
            const char c = Serial.read();

            switch(c) {
                #if defined(_AVR_EEPROM_H_)
                case '*': persist_to_EEPROM();
                    return;
                case '~': restore_from_EEPROM();
                    return;
                #endif
                case 'D':
                    parser_mode = debug_output_command;
                    return;
                case 'L':
                    parser_mode = led_display_command;
                    return;
                default:
                    switch (parser_mode) {
                        case led_display_command: {
                            switch (c) {
                                case 'q':  // quiet
                                case 'f':  // flash
                                case 't':  // ticks
                                case '2':  // 200 ms of the signal
                                case 's':  // seconds
                                case 'h':  // hours and minutes
                                case 'm':  // months and days
                                case 'a':  // analyze phase drift
                                case 'c':  // calibration state + deviation
                                    LED_Display::set_mode(c);
                                    return;
                            }
                        }
                        case debug_output_command: {
                            switch(c) {
                                case 'q':  // quiet
                                case 'd':  // debug
                                case 's':  // scope
                                case 'm':  // multi mode debug + scope
                                case 'S':  // hight resolution scope
                                case 'a':  // analyze phase drift
                                case 'A':  // Analyze phase drift, more details
                                case 'b':  // demodulator bins
                                case 'B':  // more on demodulator bins
                                case 'r':  // raw
                                case 'c':  // CET/CEST
                                case 'u':  // UTC
                                    ::set_mode(c);
                                    return;
                            }
                        }
                    }
            }
            help_on_none_space(c);
        }
    }
}

void print_clock_state() {
    switch (DCF77_Clock::get_clock_state()) {
        case Clock::useless:  Serial.print(F("useless:  ")); break;
        case Clock::dirty:    Serial.print(F("dirty:    ")); break;
        case Clock::free:     Serial.print(F("free:     ")); break;
        case Clock::synced:   Serial.print(F("synced:   ")); break;
        case Clock::locked:   Serial.print(F("locked:   ")); break;
        case Clock::unlocked: Serial.print(F("unlocked: ")); break;
    }
}

void sprintlnpp16m(int16_t pp16m) {
    Debug::sprintpp16m(pp16m);
    sprintln();
}

void setup() {
    //using namespace DCF77_Encoder;

    Serial.begin(115200);

    pinMode(dcf77_sample_pin, dcf77_pin_mode);

#if defined(POLLIN_DCF77)
    pinMode(gnd_pin, OUTPUT);
    digitalWrite(gnd_pin, LOW);
    pinMode(pon_pin, OUTPUT);
    digitalWrite(pon_pin, LOW);
    pinMode(vcc_pin, OUTPUT);
    digitalWrite(vcc_pin, HIGH);
#endif

    LED_Display::setup();

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);
    DCF77_Clock::set_output_handler(output_handler);

    Serial.println();
    Serial.print(F("DCF77 Clock V"));
    Serial.println(F(DCF77_VERSION_STRING));
    Serial.println(F("(c) Udo Klein 2017"));
    Serial.println(F("www.blinkenlight.net"));
    Serial.println();
    Serial.println(F("Documentation:  https://blog.blinkenlight.net/experiments/dcf77/"));
    Serial.println(F("Git Repository: https://github.com/udoklein/dcf77/releases/"));

    Serial.println();
    Serial.println(F(__FILE__));
    Serial.print(F("Compiled:              "));
    Serial.println(F(__TIMESTAMP__));
    Serial.print(F("Architecture:          "));
    Serial.println(F(GCC_ARCHITECTURE));
    Serial.print(F("Compiler Version:      "));
    Serial.println(F(__VERSION__));
    Serial.print(F("DCF77 Library Version: "));
    Serial.println(F(DCF77_VERSION_STRING));
    Serial.print(F("CPU Frequency:         "));
    Serial.println(F_CPU);

    Serial.println();
    Serial.print(F("Phase_lock_resolution [ticks per second]: "));
    Serial.println(Configuration::phase_lock_resolution);
    Serial.print(F("Quality Factor Sync Threshold:            "));
    Serial.println((int)Configuration::quality_factor_sync_threshold);
    Serial.print(F("Unacceptable Demodulator Quality:         "));
    Serial.println((int)Configuration::unacceptable_demodulator_quality);
    Serial.print(F("Unacceptable Minute Decoder Quality:      "));
    Serial.println((int)Configuration::unacceptable_minute_decoder_quality);
    Serial.print(F("Has stable ambient temperature:           "));
    Serial.println(Configuration::has_stable_ambient_temperature);
    Serial.print(F("Maximum frequency adjustment [pp16m]:     "));
    Serial.println((int)Configuration::maximum_total_frequency_adjustment);



    Serial.println();
    Serial.print(F("Sample Pin:      ")); Serial.println(dcf77_sample_pin);
    Serial.print(F("Sample Pin Mode: ")); Serial.println(dcf77_pin_mode);
    Serial.print(F("Inverted Mode:   ")); Serial.println(dcf77_inverted_samples);
    #if defined(__AVR__)
    Serial.print(F("Analog Mode:     ")); Serial.println(dcf77_analog_samples);
    #endif
    Serial.print(F("Monitor Led:     ")); Serial.println(LED_Display::dcf77_monitor_led);

    Serial.println();
    Serial.print(F("Freq. Adjust:    ")); sprintlnpp16m(Generic_1_kHz_Generator::read_adjustment());

    Serial.println();

    Parser::help();

    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.println();

    #if defined(_AVR_EEPROM_H_)
    Parser::restore_from_EEPROM();
    #endif
}

void loop() {
    Parser::parse();

    switch (mode) {
        case 'q': break;
        case 'A':
        case 'a': {
            Clock::time_t now;
            DCF77_Clock::get_current_time(now);

            if (mode == 'A') {
                Serial.println();
                Scope::print();
                #if defined(__AVR__)
                Serial.println();
                #if !defined(__AVR_ATmega32U4__)
                Serial.print(F("TCNT2: "));
                Serial.println(TCNT2);
                #else
                Serial.print(F("TCNT3: "));
                Serial.println(TCNT3);
                #endif
                #endif
            }
            DCF77_Frequency_Control::debug();
            Phase_Drift_Analysis::debug();
            //DCF77_Demodulator::debug();
            if (mode == 'A') {
                if (now.month.val > 0) {
                    Serial.println();
                    Serial.print(F("Decoded time: "));

                    DCF77_Clock::print(now);
                    Serial.println();
                }

                DCF77_Clock::debug();
            }

            break;
        }
        case 'b': {
            Clock::time_t now;
            DCF77_Clock::get_current_time(now);

            Clock_Controller::Demodulator.debug();
            break;
        }

        case 'B': {
            Clock::time_t now;
            DCF77_Clock::get_current_time(now);

            print_clock_state();
            Serial.println();

            Clock_Controller::Demodulator.debug_verbose();
            Serial.println();
            break;
        }


        case 's':
            Scope::print();
            break;
        case 'S': break;
        case 'r': break;

        case 'c': // render CET/CEST
        case 'u': // render UTC
            {
                Clock::time_t now;
                DCF77_Clock::get_current_time(now);

                if (now.month.val > 0) {
                    print_clock_state();
                    Serial.print(' ');

                    const int8_t target_timezone_offset =
                        mode == 'c' ?         0:
                        now.uses_summertime? -2:
                                             -1;
                    Timezone::adjust(now, target_timezone_offset);

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

                    Serial.print(' ');
                    if (mode == 'c') {
                        if (now.uses_summertime) {
                            Serial.println(F("CEST (UTC+2)"));
                        } else {
                            Serial.println(F("CET (UTC+1)"));
                        }
                    } else {
                        Serial.println(F("UTC"));
                    }
                }
                break;
            }

        case 'm':  // multi mode scope + fall through to debug
            Serial.println();
            Scope::print();

        default: {
            Clock::time_t now;
            DCF77_Clock::get_current_time(now);

            if (now.month.val > 0) {
                Serial.println();
                Serial.print(F("Decoded time: "));

                DCF77_Clock::print(now);
                Serial.println();
            }

            DCF77_Clock::debug();

            //Clock_Controller::Second_Decoder.debug();
            Clock_Controller::Local_Clock.debug();
        }
    }
    //free_dump();
}
