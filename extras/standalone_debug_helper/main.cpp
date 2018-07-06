//
//  www.blinkenlight.net
//
//  Copyright 2017 Udo Klein
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


#include "main.h"
#include "../../dcf77.cpp"
#include <fstream>
#include <random>
#include <getopt.h>


namespace Test_Setup {
    enum signal_source_t : uint8_t { filesystem = 1, synthesizer = 2 };
    signal_source_t signal_source = synthesizer;

    enum file_format_t : uint8_t { swiss_army_debug_helper_scope = 1, dcf77_logs_de = 2 };
    file_format_t file_format = dcf77_logs_de;
    std::string file_name = "";

    enum verbosity_level_t : uint8_t { quiet = 0, event_triggered = 1, always = 2 };

    struct verbosity_t {
        verbosity_level_t show_arguments   = quiet;
        verbosity_level_t show_debug_scope = quiet;
        verbosity_level_t show_debug_info  = quiet;
        verbosity_level_t show_synthesizer = quiet;
        //verbosity_level_t show_parser      = quiet;
    } verbosity;

    struct synthesizer_parameters_t {
        unsigned year    = 17;  // 00..99
        unsigned month   =  1;  // 01..12
        unsigned day     =  1;  // 00..29
        unsigned weekday =  7;  // Mo = 1, So = 7
        unsigned hour    = 00;  // 00..23
        unsigned minute  = 00;  // 00..59
        unsigned second  = 00;  // 00..60;
        bool uses_summertime                = false;  // false -> wintertime, true -> summertime
        bool abnormal_transmitter_operation = false;  // typically false
        bool timezone_change_scheduled      = false;
        bool leap_second_scheduled          = false;

        unsigned long synthesized_signal_length = 1000;  // seconds
    } synthesizer_parameters;


    struct signal_shaper_parameters_t {
        unsigned long startup_ms = 0;
        int startup_signal = 0;
        bool distort_startup_signal = false;

        signed int drift_pp16m = 0;  // simulates clock drift by the given value, negative values allowed

        unsigned long fade_min_ms     = 0;
        unsigned long fade_max_ms     = 0;
        unsigned long fade_min_gap_ms = 0;
        unsigned long fade_max_gap_ms = 0;
        int           faded_signal    = 0;

        unsigned int random_hf_noise_per_1000 = 0;   // How many samples out of 1000 will be randomized.
        //static const unsigned int random_hf_noise_per_1000 = 0;   // How many samples out of 1000 will be randomized.
    } signal_shaper_parameters;

    boolean high_phase_lock_resolution = false;
}

struct Configuration_lores_T {
    enum ticks_per_second_t : uint16_t { centi_seconds = 100, milli_seconds = 1000 };
    // this is the actuall sample rate
    static const ticks_per_second_t phase_lock_resolution = centi_seconds;

    enum quality_factor_sync_threshold_t : uint8_t { aggressive_sync = 1, standard_sync = 2, conservative_sync = 3 };
    static const uint8_t quality_factor_sync_threshold = quality_factor_sync_threshold_t::aggressive_sync;

    enum demodulator_quality_threshold_t : uint8_t { standard_quality = 10 };
    static const uint8_t unacceptable_demodulator_quality = demodulator_quality_threshold_t::standard_quality;

    enum controller_minute_quality_threshold_t : uint8_t { aggressive_minute_quality = 0, standard_minute_quality = 2, conservative_minute_quality = 4, paranoid_minute_quality = 6 };
    static const uint8_t unacceptable_minute_decoder_quality = controller_minute_quality_threshold_t::aggressive_minute_quality;
};

struct Configuration_hires_T {
    enum ticks_per_second_t : uint16_t { centi_seconds = 100, milli_seconds = 1000 };
    // this is the actuall sample rate
    static const ticks_per_second_t phase_lock_resolution = milli_seconds;

    enum quality_factor_sync_threshold_t : uint8_t { aggressive_sync = 1, standard_sync = 2, conservative_sync = 3 };
    static const uint8_t quality_factor_sync_threshold = quality_factor_sync_threshold_t::aggressive_sync;

    enum demodulator_quality_threshold_t : uint8_t { standard_quality = 10 };
    static const uint8_t unacceptable_demodulator_quality = demodulator_quality_threshold_t::standard_quality;

    enum controller_minute_quality_threshold_t : uint8_t { aggressive_minute_quality = 0, standard_minute_quality = 2, conservative_minute_quality = 4, paranoid_minute_quality = 6 };
    static const uint8_t unacceptable_minute_decoder_quality = controller_minute_quality_threshold_t::aggressive_minute_quality;
};

FakeSerial Serial;

namespace Statistics {
    unsigned long clock_state_count[6] = {};
    unsigned long clock_transition_count[6][6] = {};
    // actually qf should always be <= 50, 100 was put in place to be absolutely sure to never overflow
    unsigned long quality_factor_count[256] = {};
    unsigned long prediction_match_count[256] = {};

    long double square(long double x) { return x*x; }

    void indent() { print("  "); }

    void dump_histogram(unsigned long data[256]) {
        uint8_t max_n=1;
        uint8_t min_n=254;

        for (uint8_t n = 1; n < 254; ++n) {
            if (data[n] != 0) {
                if (n > max_n) { max_n = n; }
                if (n < min_n) { min_n = n; }
            }
        }
        min_n -= 1;
        max_n += 1;

        unsigned long samples = 0;
        long double avg = 0;
        long double std_deviation = 0;

        for (uint8_t n = min_n; n <= max_n; ++n) {
            indent();
            print((int) n);
            print(": ");
            println(data[n]);

            samples += data[n];
            avg += data[n] * n;
        }
        if (samples > 0) {
            avg /= samples;

            for (uint8_t n = 0; n <= max_n+1; ++n) {
                std_deviation += data[n] * square(n-avg);
            }
            std_deviation /= samples;
            std_deviation = sqrt(std_deviation);
        }

        print("average: ");
        print(avg);
        indent();
        print("standard deviation: ");
        println(std_deviation);
    }

    void dump() {
        const std::string clock_state[] = {"useless ", "dirty   ", "free    ", "unlocked", "locked  ", "synced  "};

        println("\nClock State Statistics");
        for (uint8_t i = 0; i < 6; ++i) {
            indent();
            print((int) i);
            print(' ');
            print(clock_state[i]);
            print(": ");
            println(clock_state_count[i]);
        }
        println("\nClock State Transition Statistics");
        for (uint8_t from = 0; from < 6; ++from) {
            for (uint8_t to = 0; to < 6; ++to) {
                if (clock_transition_count[from][to]) {
                    indent();
                    print(clock_state[from]);
                    print(" => ");
                    print(clock_state[to]);
                    print(": ");
                    println(clock_transition_count[from][to]);
                }
            }
        }

        println("\nQuality Factor Statistics");
        dump_histogram(quality_factor_count);

        println("\nPrediction Match Statistics");
        dump_histogram(prediction_match_count);
    }
}

namespace Internal {
    namespace Generic_1_kHz_Generator {
        void setup() {
            adjust_pp16m = 0;
            cumulated_phase_deviation = 0;
        }

        void setup(const Clock::input_provider_t input_provider) {
            setup();
        }
    }
}

namespace Debug_Clock {
    using namespace Internal;
    typedef DCF77_Clock_Controller<Configuration_lores_T, DCF77_Frequency_Control> Clock_Controller_lores;
    typedef DCF77_Clock_Controller<Configuration_hires_T, DCF77_Frequency_Control> Clock_Controller_hires;

    void setup() {
        if (Test_Setup::high_phase_lock_resolution) {
            Clock_Controller_hires::setup();
        } else {
            Clock_Controller_lores::setup();
        }
    }

    void setup(const Clock::input_provider_t input_provider, const Clock::output_handler_t output_handler) {
        if (Test_Setup::high_phase_lock_resolution) {
            Clock_Controller_hires::setup();
            Clock_Controller_hires::set_output_handler(output_handler);
        } else {
            Clock_Controller_lores::setup();
            Clock_Controller_lores::set_output_handler(output_handler);
        }
        Generic_1_kHz_Generator::setup();
    };

    void debug() {
        if (Test_Setup::high_phase_lock_resolution) {
            Clock_Controller_hires::debug();
        } else {
            Clock_Controller_lores::debug();
        }
    }

    void set_output_handler(const Clock::output_handler_t output_handler) {
        if (Test_Setup::high_phase_lock_resolution) {
            Clock_Controller_hires::set_output_handler(output_handler);
        } else {
            Clock_Controller_lores::set_output_handler(output_handler);
        }
    }

    void process_1_kHz_tick_data(const uint8_t the_data) {
        if (Test_Setup::high_phase_lock_resolution) {
            Clock_Controller_hires::process_1_kHz_tick_data(the_data);
        } else {
            Clock_Controller_lores::process_1_kHz_tick_data(the_data);
        }
    }

    void convert_time(const DCF77_Encoder &current_time, Clock::time_t &now) {
        now.second                    = BCD::int_to_bcd(current_time.second);
        now.minute                    = current_time.minute;
        now.hour                      = current_time.hour;
        now.weekday                   = current_time.weekday;
        now.day                       = current_time.day;
        now.month                     = current_time.month;
        now.year                      = current_time.year;
        now.uses_summertime           = current_time.uses_summertime;
        now.leap_second_scheduled     = current_time.leap_second_scheduled;
        now.timezone_change_scheduled = current_time.timezone_change_scheduled;
    }

    void read_current_time(Clock::time_t &now) {
        DCF77_Encoder current_time;
        if (Test_Setup::high_phase_lock_resolution) {
            Clock_Controller_hires::read_current_time(current_time);
        } else {
            Clock_Controller_lores::read_current_time(current_time);
        }
        convert_time(current_time, now);
    };

    void read_future_time(Clock::time_t &now_plus_1s) {
        DCF77_Encoder current_time;
        if (Test_Setup::high_phase_lock_resolution) {
            Clock_Controller_hires::read_current_time(current_time);
        } else {
            Clock_Controller_lores::read_current_time(current_time);
        }
        current_time.advance_second();

        convert_time(current_time, now_plus_1s);
    }

    void print(Clock::time_t time) {
        BCD::print(time.year);
        sprint('-');
        BCD::print(time.month);
        sprint('-');
        BCD::print(time.day);
        sprint(' ');
        sprint(time.weekday.val & 0xF, HEX);
        sprint(' ');
        BCD::print(time.hour);
        sprint(':');
        BCD::print(time.minute);
        sprint(':');
        BCD::print(time.second);

        if (time.uses_summertime) {
            sprint(F(" CEST "));
        } else {
            sprint(F(" CET "));
        }

        sprint(time.timezone_change_scheduled? '*': '.');
        sprint(time.leap_second_scheduled    ? 'L': '.');
    }

    uint8_t get_overall_quality_factor() {
        if (Test_Setup::high_phase_lock_resolution) {
            return Clock_Controller_hires::get_overall_quality_factor();
        } else {
            return Clock_Controller_lores::get_overall_quality_factor();
        }
    };

    Clock::clock_state_t get_clock_state() {
        if (Test_Setup::high_phase_lock_resolution) {
            return Clock_Controller_hires::get_clock_state();
        } else {
            return Clock_Controller_lores::get_clock_state();
        }
    };

    uint8_t get_prediction_match() {
        if (Test_Setup::high_phase_lock_resolution) {
            return Clock_Controller_hires::get_prediction_match();
        } else {
            return Clock_Controller_lores::get_prediction_match();
        }
    };
}

namespace Scope {
    const int characters_to_flush = 100;

    bool output_triggered = false;
    int char_count = 0 ;
    int up_to_ten_ms = 0;

    bool next_line() {
        return char_count == 0 && up_to_ten_ms == 0;
    }

    void second_tick_info(bool state_changed) {
        using namespace Test_Setup;
        // output more debug information

        if (Test_Setup::verbosity.show_debug_info == always ||
            state_changed && Test_Setup::verbosity.show_debug_info == event_triggered) {
            println();

            Clock::time_t now;
            Debug_Clock::read_current_time(now);

            if (now.month.val > 0) {
                Serial.print(F("Decoded time: "));

                Debug_Clock::print(now);
                Serial.println();
            }

            Debug_Clock::debug();

            //Clock_Controller::Second_Decoder.debug();
            if (output_triggered) {
                println("output triggered");
                output_triggered = false;
            }

            if (Test_Setup::high_phase_lock_resolution) {
                Debug_Clock::Clock_Controller_hires::Local_Clock.debug();
            } else {
                Debug_Clock::Clock_Controller_lores::Local_Clock.debug();
            }
            Internal::DCF77_Frequency_Control::debug();
        }
    }

    void debug_10ms(int sum) {
        static Clock::clock_state_t prev_state = Test_Setup::high_phase_lock_resolution ? Debug_Clock::Clock_Controller_hires::Local_Clock.get_state()
                                                                                        : Debug_Clock::Clock_Controller_lores::Local_Clock.get_state() ;
        static int16_t prev_adjustment = Internal::Generic_1_kHz_Generator::read_adjustment();

        static unsigned long line_count = 0;

        if (char_count == 0) { // begin of line
            ++line_count;
            if (Test_Setup::verbosity.show_debug_scope == Test_Setup::always) {
                // ensure the line_count values will be aligned to the right
                for (unsigned long val=line_count; val < 100000000; val *= 10) {
                    print(' ');
                }
                print(line_count);
                print(", ");
            }
        }

        // print one character
        if (Test_Setup::verbosity.show_debug_scope == Test_Setup::always) {
            if (sum == 0) {
                print(char_count % 10 == 0 ? '+' : '-');
            } else {
                print(sum < 10? (char) ('0'+sum): 'X');
            }
        }

        ++char_count;
        if (char_count == characters_to_flush) {
            char_count = 0;
            const Clock::clock_state_t clock_state = Test_Setup::high_phase_lock_resolution ? Debug_Clock::Clock_Controller_hires::Local_Clock.get_state()
                                                                                            : Debug_Clock::Clock_Controller_lores::Local_Clock.get_state();;
            Statistics::clock_state_count[clock_state] += 1;
            Statistics::clock_transition_count[prev_state][clock_state] += 1;
            if (Test_Setup::high_phase_lock_resolution) {
                Statistics::quality_factor_count[Debug_Clock::Clock_Controller_hires::get_overall_quality_factor()] += 1;
                Statistics::prediction_match_count[Debug_Clock::Clock_Controller_hires::get_prediction_match()] += 1;
            } else {
                Statistics::quality_factor_count[Debug_Clock::Clock_Controller_lores::get_overall_quality_factor()] += 1;
                Statistics::prediction_match_count[Debug_Clock::Clock_Controller_lores::get_prediction_match()] += 1;
            }

            const int16_t adjustment = Internal::Generic_1_kHz_Generator::read_adjustment();

            second_tick_info(clock_state != prev_state || adjustment != prev_adjustment);

            prev_state = clock_state;
            prev_adjustment = adjustment;

            if (Test_Setup::verbosity.show_debug_scope == Test_Setup::always) { println(); }
        }
    }


    void debug(int signal) {
        static int sum = 0;

        ++up_to_ten_ms;
        sum += signal;
        if (up_to_ten_ms == 10) {
            Scope::debug_10ms(sum);
            sum = 0;
            up_to_ten_ms = 0;
        }
    }
}

namespace Signal_Shaper {
    unsigned long startup_ms = 0;
    static std::mt19937_64 fade_prng(1);
    static std::uniform_int_distribution<unsigned long> distribution_fade;
    static std::uniform_int_distribution<unsigned long> distribution_gap;

    static bool is_gap = false;
    static unsigned long ms_to_go = 0;

    static std::mt19937_64 noise_prng(0);
    static std::uniform_int_distribution<int> distribution_1000(0, 999);
    static std::uniform_int_distribution<int> distribution_2(0, 1);

    void setup() {
        startup_ms = Test_Setup::signal_shaper_parameters.startup_ms;
        is_gap = false;
        ms_to_go = 0;

        fade_prng.seed(1);
        distribution_fade = std::uniform_int_distribution<unsigned long>((unsigned long) Test_Setup::signal_shaper_parameters.fade_min_ms , (unsigned long)Test_Setup::signal_shaper_parameters.fade_max_ms);
        distribution_gap = std::uniform_int_distribution<unsigned long>((unsigned long)Test_Setup::signal_shaper_parameters.fade_min_gap_ms, (unsigned long)Test_Setup::signal_shaper_parameters.fade_max_gap_ms);
        distribution_fade.reset();
        distribution_gap.reset();

        noise_prng.seed(0);
        distribution_1000.reset();
        distribution_2.reset();
    };


    void flush(const uint8_t signal) {
        Scope::debug(signal);
        Debug_Clock::process_1_kHz_tick_data(signal);
    }

    void inject_random_hf_noise(const uint8_t signal) {
        if (Test_Setup::signal_shaper_parameters.random_hf_noise_per_1000 > 0 &&
            distribution_1000(noise_prng) < Test_Setup::signal_shaper_parameters.random_hf_noise_per_1000) {

            flush(distribution_2(noise_prng));
        } else {
            flush(signal);
        }
    }


    void inject_fade(const uint8_t signal) {
        if (Test_Setup::signal_shaper_parameters.fade_max_ms == 0 && Test_Setup::signal_shaper_parameters.fade_max_gap_ms == 0) {
            // goto end
        } else {
            while (ms_to_go == 0) {
                is_gap = !is_gap;
                if (is_gap) {
                    ms_to_go = distribution_gap(fade_prng);
                } else {
                    ms_to_go = distribution_fade(fade_prng);
                }
            }
            --ms_to_go;
            if (!is_gap) {
                inject_random_hf_noise(Test_Setup::signal_shaper_parameters.faded_signal);
                return;
            }
        }
        inject_random_hf_noise(signal);
    }


    void inject_drift(const uint8_t signal) {
        static signed long cumulated_drift = 0;

        cumulated_drift += Test_Setup::signal_shaper_parameters.drift_pp16m;
        cumulated_drift -= Internal::Generic_1_kHz_Generator::adjust_pp16m;
        if (cumulated_drift >= 16000000) {
            cumulated_drift -= 16000000;

            // clock is to fast, emulate this by skipping a sample
            return;
        }

        if (cumulated_drift <= -16000000) {
            cumulated_drift += 16000000;
            // clock is to slow, emulate this by duplicating this sample

            inject_fade(signal);
        }

        inject_fade(signal);
    }



    void process_1_kHz_tick_data(const uint8_t signal) {
        while (startup_ms > 0) {
            --startup_ms;
            if (Test_Setup::signal_shaper_parameters.distort_startup_signal) {
                inject_drift(Test_Setup::signal_shaper_parameters.startup_signal);
            } else {
                flush(Test_Setup::signal_shaper_parameters.startup_signal);
            }
        }

        inject_drift(signal);
    };
}

namespace dcf77_log_de_parser {
    void generate_signal(uint16_t ms, uint8_t data) {
        for (uint16_t count = 0; count < ms; ++count) {
            Signal_Shaper::process_1_kHz_tick_data(data);
        }
    };

    void push_gap_to_clock_controller() {
        generate_signal(1000, 0);
    }

    void push_data_to_clock_controller(char data) {
        switch (data) {
            case '0' :  // short tick
                generate_signal(100, 1);
                generate_signal(900, 0);
                break;

            case '_' :  // undefined
                generate_signal(100, 0);
                generate_signal(100, 1);
                generate_signal(800, 0);
                break;

            case '1' :  // long tick
                generate_signal(200, 1);
                generate_signal(800, 0);
                break;

            default: assert("data must be one of \"01_ \"", false, data);
        }
    }


    void parse(char c) {
        // static_parse is a co-routine for parsing.
        // That is all variables will be declared static
        // and the program counter will be stored in
        // static void * parser_state.
        // The idea is that the YIELD_NEXT_CHAR
        // macro can be used to return control to the
        // caller while keeping the internal state.
        // The caller in turn will push the
        // next available character into static_parse
        // whenever it has one character ready.
        // This is a means of cooperative multi tasking

        // For more background on what is going on you might want to
        // follow some of the two links below.
        //     https://blog.blinkenlight.net/goto-considered-helpful/
        //     https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html
        //     http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
        //

        if (c==0x0D) {
            // map carriage return to newline
            c = 0x0A;
        }

        static void * parser_state = &&l_parser_start;

        goto *parser_state;
        #define LABEL(N) label_ ## N
        #define XLABEL(N) LABEL(N)
        #define YIELD_NEXT_CHAR                                               \
            do {                                                              \
                parser_state = &&XLABEL(__LINE__); return; XLABEL(__LINE__):; \
            } while (0)

        l_parser_start: ;

        // we are searching for lines with the following format
        // ====================================================================================================================
        // 0 0010110000011 000101 10101010 1100011 100011 110 01001 000100000  Mi, 31.12.08 23:55:00, WZ
        // 0 10101000000100 000111 01000001 0000000 100000 001 10000 100100001  Do, 01.01.09 00:02:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
        // 0 11010010111000 000111 00000000 1000001 100000 001 10000 1001000010 Do, 01.01.09 01:00:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant, Weiteren Impuls erhalten (Schaltsekunde)
        // 0 01000110011101 000101 10000001 1000001 100000 001 10000 100100001  Do, 01.01.09 01:01:00, WZ
        // 0 10001111110___ ______ ________ _______ ______ ___ _____ _________  __, __.__.__ __:__:00, __ * Daten wurden unvollstaendig empfangen.

        l_search_start_separator: ;
        if (c != '=') {
            YIELD_NEXT_CHAR;
            goto l_search_start_separator;
        }

        l_consume_start_separator: ;
        if (c == '=') {
            YIELD_NEXT_CHAR;
            goto l_consume_start_separator;
        }

        l_skip_space: ;
        if (c == ' ') {
            YIELD_NEXT_CHAR;
            goto l_skip_space;
        }

        assert("newline after start separator", c == 0x0A , c);
        l_newline: ;
        YIELD_NEXT_CHAR;
        if (c == '\n') {
            goto l_newline;
        }
        if (c == '=') {
            // end separator found
            goto l_skip_till_end_of_file;
        }
        if (c == '-') {
            // comment found
            goto l_skip_till_end_of_line;
        }

        push_gap_to_clock_controller();
        push_data_to_clock_controller(c);

        YIELD_NEXT_CHAR;
        static uint8_t second = 1;

        l_parse_58_seconds: ;
        l_skip_whitespace: ;
        if (c == ' ' && second <= 58) {
            YIELD_NEXT_CHAR;
            goto l_skip_whitespace;
        }

        ++second;

        if (second <= 59) {
            push_data_to_clock_controller(c);
            YIELD_NEXT_CHAR;
            goto l_parse_58_seconds;

        } else {
            if (c != ' ') {
                push_data_to_clock_controller(c);
            }
            goto l_skip_till_end_of_line;
        }

        l_skip_till_end_of_line: ;
        YIELD_NEXT_CHAR;
        if (c == 0x0A) {
            second = 1;
            goto l_newline;
        }
        goto l_skip_till_end_of_line;

        l_skip_till_end_of_file: ;
        YIELD_NEXT_CHAR;
        goto l_skip_till_end_of_file;
    }
}

namespace swiss_army_debug_helper_scope_parser {
    void push_data_to_clock_controller(int data) {
        static int prev_signal = 0;
        int signal;

        if (prev_signal == 0) {
            // low or transition low --> high
            for (int count = 10; count > 0; --count) {
                signal = (count <= data);
                Signal_Shaper::process_1_kHz_tick_data(signal);
            }
        } else {
            // high or transition high --> low
            for (int count = 1; count <= 10; ++count) {
                signal = (count <= data);
                Signal_Shaper::process_1_kHz_tick_data(signal);
            }
        }
        prev_signal = signal;
    }

    void parse(char c) {
        // static_parse is a co-routine for parsing.
        // That is all variables will be declared static
        // and the program counter will be stored in
        // static void * parser_state.
        // The idea is that the YIELD_NEXT_CHAR
        // macro can be used to return control to the
        // caller while keeping the internal state.
        // The caller in turn will push the
        // next available character into static_parse
        // whenever it has one character ready.
        // This is a means of cooperative multi tasking

        // For more background on what is going on you might want to
        // follow some of the two links below.
        //     https://blog.blinkenlight.net/goto-considered-helpful/
        //     https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html
        //     http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
        //

        if (c==0x0D) {
            // map carriage return to newline
            c = 0x0A;
        }

        static void * parser_state = &&l_parser_start;

        goto *parser_state;
        #define LABEL(N) label_ ## N
        #define XLABEL(N) LABEL(N)
        #define YIELD_NEXT_CHAR                                               \
            do {                                                              \
                parser_state = &&XLABEL(__LINE__); return; XLABEL(__LINE__):; \
            } while (0)

        l_parser_start: ;

        // we are searching for lines of the following format

        // any number of leading whitespace (including none at all)
        // followed by a sequence of digits
        // followed by a comma and any number of whitespace
        // a sequence of +- or digits [0-9] or X, X indicating the value 10
        // any number of whitespace (including none)

        // example:
        //      1764, +---------+---------+---------+---------+---------+------8XXXXXXXXXX6-+---------+---------+---------

        // other lines shall be ignored

        // examples:
        // TCNT2: 223
        // confirmed_precision ?? adjustment, deviation, elapsed
        // 0 pp16m @+ , 0 pp16m, -10 ticks, 25 min + 22692 ticks mod 60000
        // 69/100~0/600
        //
        // Decoded time: 16-09-01 4 23:44:52 CEST ..
        // 16.09.01(4,4)23:45:51 MEZ 0,0 5 p(7288-0:255) s(198-0:27) m(148-92:7) h(168-112:7) wd(84-56:4) D(112-84:4) M(112-84:4) Y(108-81:4) 56,28,28,50


        l_newline: ;
        if (c == 0x0A || c == ' ') {
            YIELD_NEXT_CHAR;
            goto l_newline;
        }

        l_leading_digits:
        if (c >= '0' && c <= '9') {
            YIELD_NEXT_CHAR;
            goto l_leading_digits;
        }

        if (c != ',') {
            goto l_skip_till_end_of_line;
        }

        l_skip_whitespace:
        YIELD_NEXT_CHAR;
        if (c == ' ') {
            goto l_skip_whitespace;
        }

        l_data:
        int data;
             if (c == '+' || c == '-') { data = 0; }
        else if ('0' <= c && c <= '9') { data = c - '0'; }
        else if (c == 'x' || c == 'X') { data = 10; }
        else { goto l_skip_till_end_of_line; }

        push_data_to_clock_controller(data);

        YIELD_NEXT_CHAR;
        goto l_data;

        l_skip_till_end_of_line:
        YIELD_NEXT_CHAR;
        if (c == 0x0A) {
            goto l_newline;
        }
        goto l_skip_till_end_of_line;
    }
}


void synthesize_signal() {
    Internal::DCF77_Encoder now;

    now.year    = BCD::int_to_bcd(Test_Setup::synthesizer_parameters.year);
    now.month   = BCD::int_to_bcd(Test_Setup::synthesizer_parameters.month);
    now.day     = BCD::int_to_bcd(Test_Setup::synthesizer_parameters.day);
    now.weekday = BCD::int_to_bcd(Test_Setup::synthesizer_parameters.weekday);
    now.hour    = BCD::int_to_bcd(Test_Setup::synthesizer_parameters.hour);
    now.minute  = BCD::int_to_bcd(Test_Setup::synthesizer_parameters.minute);
    now.second  =                 Test_Setup::synthesizer_parameters.second;

    now.uses_summertime =                Test_Setup::synthesizer_parameters.uses_summertime;
    now.abnormal_transmitter_operation = Test_Setup::synthesizer_parameters.abnormal_transmitter_operation;
    now.timezone_change_scheduled =      Test_Setup::synthesizer_parameters.timezone_change_scheduled;
    now.leap_second_scheduled =          Test_Setup::synthesizer_parameters.leap_second_scheduled;


    for (unsigned long i = 0; i < Test_Setup::synthesizer_parameters.synthesized_signal_length; ++i) {
        Internal::DCF77::tick_t decoded_signal = now.get_current_signal();

        for (int ms = 0; ms < 1000; ++ms) {

            int signal = 0;
            if (decoded_signal == Internal::DCF77::long_tick  && (             ms < 200)) { signal = 1; }
            if (decoded_signal == Internal::DCF77::short_tick && (             ms < 100)) { signal = 1; }
            //(if (decoded_signal == Internal::DCF77::undefined  && (100 <= ms && ms < 200)) { signal = 1; }

            if (Scope::next_line() && Test_Setup::verbosity.show_synthesizer == Test_Setup::always) {
                print("next second will be: ");
                now.debug();
                println();
            }
            Signal_Shaper::process_1_kHz_tick_data(signal);
        }
        now.advance_second();
    }
}

void output_handler(const Clock::time_t &decoded_time) {
    Scope::output_triggered = true;
}

void read_file_content(std::istream & infile) {
    using namespace Test_Setup;
    switch (file_format)  {
        case swiss_army_debug_helper_scope:
            while (!infile.eof()) {
                swiss_army_debug_helper_scope_parser::parse(infile.get());
            }
            break;

        case dcf77_logs_de:
            while (!infile.eof()) {
                dcf77_log_de_parser::parse(infile.get());
            }
            break;

        default: assert("file format has valid value", false, Test_Setup::file_format);
    }
}

void read_signal_from_file() {
    using namespace Test_Setup;
    if (file_name == "") {
        read_file_content(std::cin);
    } else {
        std::ifstream infile(file_name);
        assert("open file succeeded", infile.good(), file_name);
        read_file_content(infile);
        infile.close();
    }
}

void boilerplate(int argc, char** argv) {
    using namespace Test_Setup;

    for (int i = 0; i < argc; ++i) {
        print(argv[i]);
        print(' ');
    }
    println();

    println(F(__FILE__));
    print(F("  compiled: "));
    println(F(__TIMESTAMP__));
    print(F("  architecture:          "));
    println(F(GCC_ARCHITECTURE));
    print(F("  compiler version:      "));
    println(F(__VERSION__));
    print(F("  dcf77 library version: "));
    println(F(DCF77_VERSION_STRING));

    println();
    println(F("Configuration"));
    print(F("  high_phase_lock_resolution:          "));
    println(Test_Setup::high_phase_lock_resolution);
    print(F("  phase_lock_resolution:               "));
    println(Configuration::phase_lock_resolution);
    print(F("  quality_factor_sync_threshold:       "));
    println((int)Configuration::quality_factor_sync_threshold);
    print(F("  unacceptable_demodulator_quality:    "));
    println((int)Configuration::unacceptable_demodulator_quality);
    print(F("  unacceptable_minute_decoder_quality: "));
    println((int)Configuration::unacceptable_minute_decoder_quality);
    print(F("  has_stable_ambient_temperature:      "));
    println(Configuration::has_stable_ambient_temperature);
    print(F("Maximum frequency adjustment [pp16m]:  "));
    print((int)Configuration::maximum_total_frequency_adjustment);

    println();
    println(F("synthesizer parameters"));
    print(F("  YY-MM-DD: "));
    print(synthesizer_parameters.year);
    print('-');
    print(synthesizer_parameters.month);
    print('-');
    println(synthesizer_parameters.day);
    print(F("  weekday:  "));
    println(synthesizer_parameters.weekday);
    print(F("  hh:mm:ss: "));
    print(synthesizer_parameters.hour);
    print(':');
    print(synthesizer_parameters.minute);
    print(':');
    println(synthesizer_parameters.second);

    println();
    print(F("  uses_summertime:                "));
    println(synthesizer_parameters.uses_summertime);
    print(F("  abnormal_transmitter_operation: "));
    println(synthesizer_parameters.abnormal_transmitter_operation);

    print(F("  timezone_change_scheduled:      "));
    println(synthesizer_parameters.timezone_change_scheduled);

    print(F("  leap_second_scheduled:          "));
    println(synthesizer_parameters.leap_second_scheduled);

    println();
    print(F("  synthesized_signal_length: "));
    println(synthesizer_parameters.synthesized_signal_length);

    println();
    println(F("signal shaper parameters"));
    print(F("  startup_ms:             "));
    println(signal_shaper_parameters.startup_ms);
    print(F("  startup_signal:         "));
    println(signal_shaper_parameters.startup_signal);
    print(F("  distort_startup_signal: "));
    println(signal_shaper_parameters.distort_startup_signal);
    println();
    print(F("  drift ppm:   "));
    println(signal_shaper_parameters.drift_pp16m * 1.0 / 16);
    print(F("  drift_pp16m: "));
    println(signal_shaper_parameters.drift_pp16m);
    println();
    print(F("  fade_min_ms    : "));
    println(signal_shaper_parameters.fade_min_ms);
    print(F("  fade_max_ms    : "));
    println(signal_shaper_parameters.fade_max_ms);
    print(F("  fade_min_gap_ms: "));
    println(signal_shaper_parameters.fade_min_gap_ms);
    print(F("  fade_max_gap_ms: "));
    println(signal_shaper_parameters.fade_max_gap_ms);
    print(F("  faded_signal   : "));
    println(signal_shaper_parameters.faded_signal);
    println();
    print(F("  random_hf_noise_per_1000: "));
    println(signal_shaper_parameters.random_hf_noise_per_1000);

    println();
    println(F("filter parameters"));
    print(F("  millisecond_samples: "));
    println(high_phase_lock_resolution);
    println();
}


void tombstone() {
    Test_Setup::verbosity.show_debug_info = Test_Setup::always;
    println("\nFinal Clock State");
    Scope::second_tick_info(true);

    Statistics::dump();
    println();
}


void run(int argc, char **argv) {
    boilerplate(argc, argv);

    Debug_Clock::setup();
    Debug_Clock::set_output_handler(&output_handler);

    Signal_Shaper::setup();

    switch (Test_Setup::signal_source) {
        case Test_Setup::filesystem  : read_signal_from_file(); break;
        case Test_Setup::synthesizer : synthesize_signal();     break;
        default: assert("signal source has valid value", false, Test_Setup::signal_source);
    }

    tombstone();
}


unsigned long parse_unsigned(const std::string token_name, short int min_len, short int max_len, short int base, const std::string input, char sign=' ') {
   if (input.length() >= min_len && input.length() <= max_len) {
        unsigned long l_result = 0;
        for (std::string::size_type pos = 0; pos < input.length(); ++pos) {
            if ('0' <= input[pos] && input[pos] < '0'+base) {
                l_result = 10 * l_result + (input[pos]-'0');
            } else {
                print("invalid character ");
                print(input[pos]);
                print(" in token ");
                println(token_name);
                exit(4);
            }
        }
        if (Test_Setup::verbosity.show_arguments == Test_Setup::always) { print(token_name); print(':'); print(sign); println(l_result); }
        return l_result;
   } else {
        print("invalid input length for token ");
        println(token_name);
        exit(4);
   }
}

std::string tail(const std::string source) {
    return source.size() ? source.substr(1) : "";
}

signed parse_signed(const std::string token_name, short int min_len, short int max_len, short int base, const std::string input) {
    switch (input[0]) {
        case '-': return -parse_unsigned(token_name, min_len, max_len, base, tail(input), '-');
        case '+': return  parse_unsigned(token_name, min_len, max_len, base, tail(input), '+');
        default : return  parse_unsigned(token_name, min_len, max_len, base,      input);
    }
}

bool parse_boolean(const std::string token_name, const std::string c) {
    const char input = c[0];
    if (input < '0' || input > '1' || c[1] != 0) {
        print("Argument for option ");
        print(token_name);
        println(" must be 0 or 1.");
        exit(4);
    }
    const bool output = input == '1';
    if (Test_Setup::verbosity.show_arguments == Test_Setup::always) { print(token_name); print(": "); println((int) output); }
    return output;
}

void parse_start_time(unsigned &p0,
                      unsigned &p1,
                      unsigned &p2,
                      unsigned &p3,
                      unsigned &p4,
                      unsigned &p5,
                      std::string input) {

    const std::string subtoken[] = {"YY", "MM", "DD", "hh", "mm", "ss"};
    const uint8_t range_max[] = {99, 12, 31, 23, 59, 60};
    const uint8_t range_min[] = { 0,  1,  1,  0,  0,  0};
    const char delimiter[] = {'.', '.', '@', ':', ':' };
    unsigned l_p[6];
    size_t pos = -1;
    unsigned short token_no = 0;
    std::string token;

    while (token_no < 6) {
        input = input.substr(pos + 1);
        if (token_no < 5) {
            pos = input.find(delimiter[token_no]);
            if (pos == std::string::npos && token_no != 5) {
                print("missing token ");
                println(subtoken[token_no+1]);
                exit(4);
            }
            token = input.substr(0, pos);
        } else {
            token = input;
        }

        l_p[token_no] = parse_unsigned(subtoken[token_no], 1, 2, 10, token);
        if (l_p[token_no] > range_max[token_no] || l_p[token_no] < range_min[token_no]) {
            print("value ");
            print(l_p[token_no]);
            print(" out of range for token ");
            println(subtoken[token_no]);
            exit(4);
        }

        ++token_no;
    }

    p0 = l_p[0];
    p1 = l_p[1];
    p2 = l_p[2];
    p3 = l_p[3];
    p4 = l_p[4];
    p5 = l_p[5];
}

void parse_fade_parameters(long unsigned &p0,
                           long unsigned &p1,
                           long unsigned &p2,
                           long unsigned &p3,
                           int &p4,
                           std::string input) {

    const std::string subtoken[] = {"a", "b", "c", "d", "e"};
    const char delimiter = ',';
    unsigned l_p[5];
    size_t pos = -1;
    unsigned short token_no = 0;
    std::string token;

    while (token_no < 5) {
        input = input.substr(pos + 1);
        if (token_no < 4) {
            pos = input.find(delimiter);
            if (pos == std::string::npos && token_no != 5) {
                print("missing token ");
                println(subtoken[token_no+1]);
                exit(4);
            }
            token = input.substr(0, pos);
        } else {
            token = input;
        }

        if (token_no < 4) {
            l_p[token_no] = parse_unsigned(subtoken[token_no], 1, 9, 10, token);
        } else {
            l_p[token_no] = parse_boolean(subtoken[token_no], token);
        }

        ++token_no;
    }

    p0 = l_p[0];
    p1 = l_p[1];
    p2 = l_p[2];
    p3 = l_p[3];
    p4 = l_p[4];
}

void parse_verbosity(std::string options) {
    using namespace Test_Setup;
    for (uint8_t i=0; i< 255 && i < options.length(); ++i) {
        switch (options[i]) {
            case 'a': verbosity.show_arguments   =                                                         always; break;
            case 'd': verbosity.show_debug_info  = verbosity.show_debug_info  == quiet ? event_triggered : always; break;
            case 's': verbosity.show_debug_scope =                                                         always; break;
            case 'y': verbosity.show_synthesizer =                                                         always; break;
//            case 'p': verbosity.show_parser      =                                                         always; break;
//            default: assert("option must be one of adsyp", false, options[i]);
            default: assert("option must be one of adsy", false, options[i]);
        }
    }
    if (verbosity.show_arguments) {
        const std::string verbosity_level[] = {"quiet", "event triggered", "always"};
        println("Verbosity options:");
        print("  arguments:          "); println(verbosity_level[verbosity.show_arguments]);
        print("  debug output:       "); println(verbosity_level[verbosity.show_debug_info]);
        print("  debug scope output: "); println(verbosity_level[verbosity.show_debug_scope]);
        print("  synthesizer state:  "); println(verbosity_level[verbosity.show_synthesizer]);
//        print("  parser state:       "); println(verbosity_level[verbosity.show_parser]);
        println();
    }
}

int main (int argc, char **argv) {
    using namespace Test_Setup;

    int  c;
    int  digit_optind = 0;
    char *copt = 0;

    const struct option long_options[] = {
        {"help",                           no_argument,       0, 'h'},
        {"verbose",                        no_argument,       0, 'v'},
        {"input",                          required_argument, 0, 'i'},
        {"Infile",                         required_argument, 0, 'I'},
        {"time",                           required_argument, 0, 't'},
        {"summertime",                     required_argument, 0, 'S'},
        {"Summertime",                     required_argument, 0, 'S'},
        {"abnormal_transmitter_operation", required_argument, 0, 'a'},
        {"timezone_change_scheduled",      required_argument, 0, 'c'},
        {"leap_second_scheduled",          required_argument, 0, 'l'},
        {"signal_length_s",                required_argument, 0, 's'},
        {"startup_ms",                     required_argument, 0, 'd'},
        {"startup_signal",                 required_argument, 0, 'u'},
        {"distort_startup_signal",         required_argument, 0, 'D'},
        {"Distort_startup_signal",         required_argument, 0, 'D'},
        {"drift_pp16m",                    required_argument, 0, 'p'},
        {"drift_ppm",                      required_argument, 0, 'P'},
        {"fade",                           required_argument, 0, 'f'},
        {"random_hf_noise_per_1000",       required_argument, 0, 'r'},
        {"millisecond_samples",            required_argument, 0, 'm'},
        {NULL, 0, NULL, 0}
    };

    int option_index = 0;
    while ((c = getopt_long(argc, argv, "hv:t:S:a:c:l:s:d:u:D:i:I:p:P:f:r:m:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print("Usage: ");
                print(argv[0]);
                println(" [options]");
                println(
                    "Standalone dcf77 library tester\n"
                    "\n"
                    "  -h, --help                                 display this help text\n"
                    "\n"
//                    "  -v, --view=[adsyp]                         verbose output depending on option set\n"
                    "  -v, --view=[adsy]                          verbose output depending on option set\n"
                    "                                               use option d twice to get more output\n"
                    "                                               a: show command line argument parser arguments\n"
                    "                                               d: show debug output\n"
                    "                                               s: show debug scope output\n"
                    "                                               y: show synthesizer state\n"
//                    "                                               p: show parser state\n"
                    "\n"
                    "  -i, --input=[0|1|2]                        define signal source\n"
                    "                                               use signal synthesizer [0],\n"
                    "                                               read from stdin in swiss army debug helper \"scope (Ds)\" format [1],\n"
                    "                                               read from stdin in http://www.dcf77logs.de/ format [2]\n"
                    "                                               default: 0\n"
                    "  -I, --Infile=<filename>                    name of input file\n"
                    "                                               default: "" (= read from stdin)\n"
                    "\n"
                    "                                             Synthesizer options (applicable for --input=0 only):\n"
                    "  -t, --time=YY.MM.DD@hh:mm:ss                 set start date and time, default: <<<TBD>>>\n"
                    "  -S, --summertime=[0|1]                       set summertime [1] or wintertime [0], default: 0\n"
                    "  -a, --abnormal_transmitter_operation=[0|1]   set abnormal_transmitter_operation flag, default: 0\n"
                    "  -c, --timezone_change_scheduled=[0|1]        set timezone_change_scheduled flag, default: 0\n"
                    "  -l, --leap_second_scheduled=[0|1]            set leap_second_scheduled flag, default: 0\n"
                    "  -s, --signal_length_s=n                      generate test signal data for n seconds, default: 1000\n"
                    "\n"
                    "                                             Signal distortion options (applicable for --input=[0|1|2])\n"
                    "  -d, --startup_ms=n                           prepend test signal with constant value for n milliseconds, default: 0\n"
                    "  -u, --startup_signal=[0|1]                   value of the startup signal, default: 0\n"
                    "  -D, --distort_startup_signal=[0|1]           distortion shall already be applied in the startup phase, default: 0\n"
                    "\n"
                    "  -p, --drift_pp16m=[+|-]n                     (signed) drift in parts per 16 million, default = 0\n"
                    "  -P, --drift_ppm=[+|-]n                       (signed) drift in parts per million, default = 0\n"
                    "  -f, --fade=a,b,c,d,[0,1]                     inject random signal fade\n"
                    "                                                 with random duration in [a,b] milliseconds with\n"
                    "                                                 random gap length in [c,d] milliseconds with\n"
                    "                                                 signal value e,\n"
                    "                                                 default value: 0,0,0,0,0\n"
                    "  -r, --random_hf_noise_per_1000=n             replace samples with probability n/1000 by a random value, default: 0\n"
                    "\n"
                    "                                             Filter parameter options\n"
                    "  -m, -millisecond_samples=[0|1]               aggregate 10 samples before filtering [0],\n"
                    "                                               process millisecond samples directly [1]\n"
                    "\n"
                    "Exit status:\n"
                    "  0:  OK\n"
                    "  4:  Invalid option or parameter\n"
                );
                exit(0);

            case 'v':
                parse_verbosity(optarg);
                break;
            case 'i': {
                const uint8_t inp = parse_signed("input", 1, 1, 3, optarg);
                if (verbosity.show_arguments) { print("input: "); println((int)inp); }
                signal_source = inp == 0 ? synthesizer : filesystem;
                file_format   = inp == 1 ? swiss_army_debug_helper_scope : dcf77_logs_de;
                break;
            }
            case 'I':
                if (verbosity.show_arguments) { print("Infile: "); println(optarg); }
                file_name = optarg;
                break;
            case 't':
                parse_start_time(
                    synthesizer_parameters.year,
                    synthesizer_parameters.month,
                    synthesizer_parameters.day,
                    synthesizer_parameters.hour,
                    synthesizer_parameters.minute,
                    synthesizer_parameters.second,
                    optarg);
                break;
            case 'S': synthesizer_parameters.uses_summertime                = parse_boolean("summertime", optarg);                     break;
            case 'a': synthesizer_parameters.abnormal_transmitter_operation = parse_boolean("abnormal_transmitter_operation", optarg); break;
            case 'c': synthesizer_parameters.timezone_change_scheduled      = parse_boolean("timezone_change_scheduled", optarg);      break;
            case 'l': synthesizer_parameters.leap_second_scheduled          = parse_boolean("leap_second_scheduled", optarg);          break;
            case 's': synthesizer_parameters.synthesized_signal_length      = parse_unsigned("signal_length", 1, 9, 10, optarg);       break;

            case 'd': signal_shaper_parameters.startup_ms             = parse_unsigned("startup_ms", 1, 9, 10, optarg);  break;
            case 'u': signal_shaper_parameters.startup_signal         = parse_boolean("startup_signal", optarg);         break;
            case 'D': signal_shaper_parameters.distort_startup_signal = parse_boolean("distort_startup_signal", optarg); break;

            case 'p': signal_shaper_parameters.drift_pp16m =    parse_signed("drift_pp16m", 1, 7, 10, optarg); break;
            case 'P': signal_shaper_parameters.drift_pp16m = 16*parse_signed("drift_ppm", 1, 6, 10, optarg);   break;

            case 'f':
                parse_fade_parameters(
                    signal_shaper_parameters.fade_min_ms,
                    signal_shaper_parameters.fade_max_ms,
                    signal_shaper_parameters.fade_min_gap_ms,
                    signal_shaper_parameters.fade_max_gap_ms,
                    signal_shaper_parameters.faded_signal,
                    optarg);
                break;
            case 'r': signal_shaper_parameters.random_hf_noise_per_1000 = parse_signed("input", 1, 3, 10, optarg);
                      if (signal_shaper_parameters.random_hf_noise_per_1000 > 1000) { signal_shaper_parameters.random_hf_noise_per_1000 = 1000; }
                      break;
            case 'm': high_phase_lock_resolution = parse_boolean("millisecond_samples", optarg); break;
            default:
                printf("?? getopt returned character code 0%o ??\n", c);
                exit(4);
        }
    }
    if (optind < argc) {
        println("ignoring non-option ARGV-elements: ");
        while (optind < argc) {
            print(argv[optind++]);
            print(' ');
        }
        println();
        exit(4);
    }

    run(argc, argv);

    exit (0);
}
