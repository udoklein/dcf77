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


#define GCC_VERSION (__GNUC__ * 10000 \
    + __GNUC_MINOR__ * 100 \
    + __GNUC_PATCHLEVEL__)

#define ERROR_MESSAGE(major, minor, patchlevel) compiler_version__GCC_ ## major ## _ ## minor ## _ ## patchlevel ## __ ;
#define OUTDATED_COMPILER_ERROR(major, minor, patchlevel) ERROR_MESSAGE(major, minor, patchlevel)

#if GCC_VERSION < 40503
    // Arduino 1.0.0 - 1.0.6 come with an outdated version of avr-gcc.
    // Arduino 1.5.8 comes with a ***much*** better avr-gcc. The library
    // will compile but fail to execute properly if compiled with an
    // outdated avr-gcc. So here we stop here if the compiler is outdated.
    //
    // You may find out your compiler version by executing 'avr-gcc --version'

    // Visit the compatibility section here:
    //     http://blog.blinkenlight.net/experiments/dcf77/dcf77-library/
    // for more details.
    #error Outdated compiler version < 4.5.3
    #error Absolute minimum recommended version is avr-gcc 4.5.3.
    #error Use 'avr-gcc --version' from the command line to verify your compiler version.
    #error Arduino 1.0.0 - 1.0.6 ship with outdated compilers.
    #error Arduino 1.5.8 (avr-gcc 4.8.1) and above are recommended.

    OUTDATED_COMPILER_ERROR(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#endif

#ifndef dcf77_h
#define dcf77_h

#include <stdint.h>

#ifndef STANDALONE
    #include "Arduino.h"
#endif

// Constants refering to data in time signal
#ifndef MSF60
    // DCF77 Offsets
    #define OFFSET_MINUTE_1 (21)
    #define OFFSET_MINUTE_2 (22)
    #define OFFSET_MINUTE_4 (23)
    #define OFFSET_MINUTE_8 (24)
    #define OFFSET_MINUTE_10 (25)
    #define OFFSET_MINUTE_20 (26)
    #define OFFSET_MINUTE_40 (27)
    #define OFFSET_MINUTE_PARITY (28)
    #define OFFSET_MINUTE_PROCESS (29)

    #define OFFSET_HOUR_1 (29)
    #define OFFSET_HOUR_2 (30)
    #define OFFSET_HOUR_4 (31)
    #define OFFSET_HOUR_8 (32)
    #define OFFSET_HOUR_10 (33)
    #define OFFSET_HOUR_20 (34)
    #define OFFSET_HOUR_PARITY (35)
    #define OFFSET_HOUR_PROCESS (36)

    #define OFFSET_DAY_1 (36)
    #define OFFSET_DAY_2 (37)
    #define OFFSET_DAY_4 (38)
    #define OFFSET_DAY_8 (39)
    #define OFFSET_DAY_10 (40)
    #define OFFSET_DAY_20 (41)
    #define OFFSET_DAY_PROCESS (42)

    #define OFFSET_WEEKDAY_1 (42)
    #define OFFSET_WEEKDAY_2 (43)
    #define OFFSET_WEEKDAY_4 (44)
    #define OFFSET_WEEKDAY_PROCESS (45)

    #define OFFSET_MONTH_1 (45)
    #define OFFSET_MONTH_2 (46)
    #define OFFSET_MONTH_4 (47)
    #define OFFSET_MONTH_8 (48)
    #define OFFSET_MONTH_10 (49)
    #define OFFSET_MONTH_PROCESS (50)

    #define OFFSET_YEAR_1 (50)
    #define OFFSET_YEAR_2 (51)
    #define OFFSET_YEAR_4 (52)
    #define OFFSET_YEAR_8 (53)
    #define OFFSET_YEAR_PROCESS (54)

    #define OFFSET_DECADE_1 (54)
    #define OFFSET_DECADE_2 (55)
    #define OFFSET_DECADE_4 (56)
    #define OFFSET_DECADE_8 (57)
    #define OFFSET_DECADE_PROCESS (58)
#else
    // MSF60 Offsets
    #define OFFSET_MINUTE_1 (51)
    #define OFFSET_MINUTE_2 (50)
    #define OFFSET_MINUTE_4 (49)
    #define OFFSET_MINUTE_8 (48)
    #define OFFSET_MINUTE_10 (47)
    #define OFFSET_MINUTE_20 (46)
    #define OFFSET_MINUTE_40 (45)
    #define OFFSET_MINUTE_PARITY (255)
    #define OFFSET_MINUTE_PROCESS (52)

    #define OFFSET_HOUR_1 (44)
    #define OFFSET_HOUR_2 (43)
    #define OFFSET_HOUR_4 (42)
    #define OFFSET_HOUR_8 (41)
    #define OFFSET_HOUR_10 (40)
    #define OFFSET_HOUR_20 (39)
    #define OFFSET_HOUR_PARITY (254)
    #define OFFSET_HOUR_PROCESS (45)

    #define OFFSET_DAY_1 (35)
    #define OFFSET_DAY_2 (34)
    #define OFFSET_DAY_4 (33)
    #define OFFSET_DAY_8 (32)
    #define OFFSET_DAY_10 (31)
    #define OFFSET_DAY_20 (30)
    #define OFFSET_DAY_PROCESS (36)

    #define OFFSET_WEEKDAY_1 (38)
    #define OFFSET_WEEKDAY_2 (37)
    #define OFFSET_WEEKDAY_4 (36)
    #define OFFSET_WEEKDAY_PROCESS (39)

    #define OFFSET_MONTH_1 (29)
    #define OFFSET_MONTH_2 (28)
    #define OFFSET_MONTH_4 (27)
    #define OFFSET_MONTH_8 (26)
    #define OFFSET_MONTH_10 (25)
    #define OFFSET_MONTH_PROCESS (30)

    #define OFFSET_YEAR_1 (24)
    #define OFFSET_YEAR_2 (23)
    #define OFFSET_YEAR_4 (22)
    #define OFFSET_YEAR_8 (21)
    #define OFFSET_YEAR_PROCESS (25)

    #define OFFSET_DECADE_1 (20)
    #define OFFSET_DECADE_2 (19)
    #define OFFSET_DECADE_4 (18)
    #define OFFSET_DECADE_8 (17)
    #define OFFSET_DECADE_PROCESS (21)
#endif

namespace Configuration {
    // The Configuration namespace holds the configuration of the clock library.

    // The resolution defines if the clock will average 10 milli second ticks and
    // then sample or if it will directly sample each millisecond.

    // The impact of this is:
    //    - If resolution is set to centi seconds:
    //        -the RAM footprint will be significantly lower
    //        - clock jitter occurs less often but in 10 ms leaps
    //        - auto tune will settle 10 times slower
    //        - filter bandwidth of the demodulator stage is reduced by 10 times
    //            - noise tolerance of this stage is higher

    //    - If resolution is set to milliseconds:
    //        - the RAM footprint will be significantly higher
    //        - clock jitter occurs ten times more often but in 1 ms leaps
    //        - auto tune will settle 10 times faster
    //        - filter bandwidth of the demodulator stage is increased by 10 times
    //            - noise tolerance of this stage is lower

    // As a consequency on Atmega 328 (e.g. "Arduino Uno") the resolution must
    // be centi_seconds. On ARM (e.g. "Arduino Due") the resolution may be set
    // to milli_seconds.

    // If your application does not need milli second accuracy then the low
    // memory footprint (centi_seconds) option is recommended.
    // If you are in a (electrically) noisy environment the low memory footprint
    // option is recommended.

    // 10 ms vs 1 ms sample resolution
    // setting this to true will change the sample resolution to 1 ms for ARM
    // for AVR there is not enough memory, so for AVR it will default to false

    // the constant(s) below are assumed to be configured by the user of the library

    const boolean want_high_phase_lock_resolution = true;
    //const boolean want_high_phase_lock_resolution = false;


    // end of configuration section, the stuff below
    // will compute the implications of the desired configuration,
    // ready for the compiler to consume

    #if defined(__arm__)
    const boolean has_lots_of_memory = true;
    #else
    const boolean has_lots_of_memory = false;
    #endif

    const boolean high_phase_lock_resolution = want_high_phase_lock_resolution && has_lots_of_memory;

    enum ticks_per_second_t : uint16_t { centi_seconds = 100, milli_seconds = 1000 };
    // this is the actuall sample rate
    const ticks_per_second_t phase_lock_resolution = high_phase_lock_resolution ? milli_seconds
                                                                                : centi_seconds;
}


namespace BCD {
    typedef union {
        struct {
            uint8_t lo:4;
            uint8_t hi:4;
        } digit;

        struct {
            uint8_t b0:1;
            uint8_t b1:1;
            uint8_t b2:1;
            uint8_t b3:1;
            uint8_t b4:1;
            uint8_t b5:1;
            uint8_t b6:1;
            uint8_t b7:1;
        } bit;

        uint8_t val;
    } bcd_t;

    bool operator == (const bcd_t a, const bcd_t b);
    bool operator != (const bcd_t a, const bcd_t b);
    bool operator >= (const bcd_t a, const bcd_t b);
    bool operator <= (const bcd_t a, const bcd_t b);
    bool operator >  (const bcd_t a, const bcd_t b);
    bool operator <  (const bcd_t a, const bcd_t b);

    void increment(bcd_t &value);

    bcd_t int_to_bcd(const uint8_t value);
    uint8_t bcd_to_int(const bcd_t value);
}

namespace Clock {
    typedef struct {
        BCD::bcd_t year;     // 0..99
        BCD::bcd_t month;    // 1..12
        BCD::bcd_t day;      // 1..31
        BCD::bcd_t weekday;  // Mo = 1, So = 7
        BCD::bcd_t hour;     // 0..23
        BCD::bcd_t minute;   // 0..59
        BCD::bcd_t second;   // 0..60
        bool uses_summertime;
        bool timezone_change_scheduled;
        bool leap_second_scheduled;
    } time_t;

    // Once the clock has locked to the DCF77 signal
    // and has decoded a reliable time signal it will
    // call output handler once per second
    typedef void (*output_handler_t)(const time_t &decoded_time);

    // input provider will be called each millisecond and must
    // provide the input of the raw DCF77 signal
    typedef uint8_t (*input_provider_t)(void);

    typedef enum {
        useless  = 0,  // waiting for good enough signal
        dirty    = 1,  // time data available but unreliable
        free     = 2,  // clock was once synced but now may deviate more than 200 ms, must not re-lock if valid phase is detected
        unlocked = 3,  // lock was once synced, inaccuracy below 200 ms, may re-lock if a valid phase is detected
        locked   = 4,  // clock driven by accurate phase, time is accurate but not all decoder stages have sufficient quality for sync
        synced   = 5   // best possible quality, clock is 100% synced
    } clock_state_t;
}

namespace DCF77_Clock {
    void setup();
    void setup(const Clock::input_provider_t input_provider, const Clock::output_handler_t output_handler);

    void set_input_provider(const Clock::input_provider_t);
    void set_output_handler(const Clock::output_handler_t output_handler);

    // blocking till start of next second
    void get_current_time(Clock::time_t &now);
    // non-blocking, reads current second
    void read_current_time(Clock::time_t &now);
    // non-blocking, reads current second+1
    void read_future_time(Clock::time_t &now_plus_1s);

    #if defined(__AVR__)
    void auto_persist();  // this is slow and messes with the interrupt flag, do not call during interrupt handling
    #endif
    void print(Clock::time_t time);

    void debug();

    // determine quality of the DCF77 signal lock
    uint8_t get_overall_quality_factor();

    // determine the internal clock state
    Clock::clock_state_t get_clock_state();

    // determine the short term signal quality
    // 0xff = not available
    // 0..25 = extraordinary poor
    // 25.5 would be the expected value for 100% noise
    // 26 = very poor
    // 50 = best possible, every signal bit matches with the local clock
    uint8_t get_prediction_match();
}

//////////////////////////////////////////////////////////////////////////
// Everything below this comment are internals of the library.
// The interfaces are public to ease debugging.
// There is no guarantee that it will be kept stable.
// I did not separate this into several header files due to
// Arduino's poor build system.
// Unless you know what you are doing and unless you accept that
// future release of the library may break your code do not use
// anything below in your code. You have been warned.
//////////////////////////////////////////////////////////////////////////

namespace Internal {
    namespace Debug {
        void debug_helper(char data);
        void bcddigit(uint8_t data);
        void bcddigits(uint8_t data);
        void hexdump(uint8_t data);
    }

    namespace DCF77 {

#ifndef MSF60
        typedef enum {
            long_tick  = 3,
            short_tick = 2,
            undefined  = 1,
            sync_mark  = 0
        } tick_t;
#else
        typedef enum {
            min_marker = 5,
            undefined  = 4,
            A1_B1      = 3,
            A1_B0      = 2,
            A0_B1      = 1,
            A0_B0      = 0
        } tick_t;
#endif

        // This six byte structure used for MSF60 and DCF77 bit stream
        // comments for fields reflect DCF77 definitions

        typedef struct {
            uint8_t byte_0;  // bit 16-20  // flags
            uint8_t byte_1;  // bit 21-28  // minutes
            uint8_t byte_2;  // bit 29-36  // hours, bit 0 of day
            uint8_t byte_3;  // bit 37-44  // day + weekday
            uint8_t byte_4;  // bit 45-52  // month + bit 0-2 of year
            uint8_t byte_5;  // bit 52-58  // year + parity
        } serialized_clock_stream;

#ifdef MSF60
        typedef struct {
            serialized_clock_stream A;
            serialized_clock_stream B;
        } serialized_clock_stream_pair;
#endif

    }

    namespace TMP {
        // determine type depending on number of bits
        template <uint8_t typesize> struct uint_t {};
        template <> struct uint_t<8>  { typedef uint8_t  type; };
        template <> struct uint_t<16> { typedef uint16_t type; };
        template <> struct uint_t<32> { typedef uint32_t type; };

        // determine type depending on limits
        template <uint32_t max> struct uval_t {
            typedef typename uint_t< max <   256?  8:
                                     max < 65536? 16:
                                                  32>::type type;
        };

        // template to compare types
        template <typename x, typename y> struct equal       { enum { val = false }; };
        template <typename x>             struct equal<x, x> { enum { val = true  }; };

        template <typename T> struct limits { };
        template <>           struct limits<uint8_t>  { enum { min = 0, max = 0xff}; };
        template <>           struct limits<uint16_t> { enum { min = 0, max = 0xffff}; };
        template <>           struct limits<uint32_t> { enum { min = 0, max = 0xffffffff}; };

        template <bool condition, typename x, typename y> struct if_t             { typedef y type; };
        template <                typename x, typename y> struct if_t<true, x, y> { typedef x type; };
    }

    namespace Arithmetic_Tools {
        template <uint8_t N> inline void bounded_increment(uint8_t &value) __attribute__((always_inline));
        template <uint8_t N>
        void bounded_increment(uint8_t &value) {
            const uint8_t bound = TMP::limits<uint8_t>::max;
            if (value >= bound - N) { value = bound; } else { value += N; }
        }

        template <uint8_t N> inline void bounded_decrement(uint8_t &value) __attribute__((always_inline));
        template <uint8_t N>
        void bounded_decrement(uint8_t &value) {
            const uint8_t bound = TMP::limits<uint8_t>::min;
            if (value <= bound + N) { value = bound; } else { value -= N; }
        }

        template <typename t>
        void minimize(t& minimum, t const value) {
            if (value < minimum) {
                minimum = value;
            }
        }
        template <typename t>
        void maximize(t& maximum, t const value) {
            if (value > maximum) {
                maximum = value;
            }
        }

        inline void bounded_add(uint8_t &value, const uint8_t amount) __attribute__((always_inline));
        inline void bounded_sub(uint8_t &value, const uint8_t amount) __attribute__((always_inline));
        inline uint8_t bit_count(const uint8_t value) __attribute__((always_inline));
        inline uint8_t parity(const uint8_t value) __attribute__((always_inline));

        uint8_t set_bit(const uint8_t data, const uint8_t number, const uint8_t value);
    }

    struct DCF77_Encoder {
        BCD::bcd_t year;     // 0..99
        BCD::bcd_t month;    // 1..12
        BCD::bcd_t day;      // 1..31
        BCD::bcd_t weekday;  // Mo = 1, So = 7
        BCD::bcd_t hour;     // 0..23
        BCD::bcd_t minute;   // 0..59
        uint8_t second;      // 0..60
        bool uses_summertime                : 1;  // false -> wintertime, true, summertime
        bool abnormal_transmitter_operation : 1;  // typically false
        bool timezone_change_scheduled      : 1;
        bool leap_second_scheduled          : 1;

        bool undefined_minute_output                        : 1;
        bool undefined_uses_summertime_output               : 1;
        bool undefined_abnormal_transmitter_operation_output: 1;
        bool undefined_timezone_change_scheduled_output     : 1;

#ifdef MSF60
        bool undefined_year_output          : 1;
        bool undefined_day_output           : 1;
        bool undefined_weekday_output       : 1;
        bool undefined_time_output          : 1;
#endif

        // What *** exactly *** is the semantics of the "Encoder"?
        // It only *** encodes *** whatever time is set
        // It does never attempt to verify the data

        uint8_t days_per_month() const;
        void reset();
        uint8_t get_weekday() const;
        BCD::bcd_t get_bcd_weekday() const;

        // This will set the weekday by evaluating the date.
        void autoset_weekday();
        void autoset_timezone();
        void autoset_timezone_change_scheduled();
        bool verify_leap_second_scheduled(const bool assume_leap_second) const;
        void autoset_control_bits();

        // This will advance the minute. It will consider the control
        // bits while doing so. It will NOT try to properly set the
        // control bits. If this is desired "autoset" must be called in
        // advance.
        void advance_minute();

        // This will advance the second. It will consider the control
        // bits while doing so. It will NOT try to properly set the
        // control bits. If this is desired "autoset" must be called in
        // advance.
        void advance_second();
        DCF77::tick_t get_current_signal() const;

#ifndef MSF60
        void get_serialized_clock_stream(DCF77::serialized_clock_stream &data) const;
#else
        void get_serialized_clock_stream(DCF77::serialized_clock_stream_pair &data) const;
#endif
        void debug() const;
        void debug(const uint16_t cycles) const;

        // Bit      Bezeichnung     Wert    Pegel   Bedeutung
        // 0        M                       0       Minutenanfang (

        // 1..14    n/a                             reserviert

        // 15       R                               Reserveantenne aktiv (0 inaktiv, 1 aktiv)
        // 16       A1                              Ankündigung Zeitzonenwechsel (1 Stunde vor dem Wechsel für 1 Stunde, d.h ab Minute 1)
        // 17       Z1               2              Zeitzonenbit Sommerzeit (MEZ = 0, MESZ = 1); also Zeitzone = UTC + 2*Z1 + Z2
        // 18       Z2               1              Zeitzonenbit Winterzeit (MEZ = 1, MESZ = 0); also Zeitzone = UTC + 2*Z1 + Z2
        // 19       A2                              Ankündigung einer Schaltsekunde (1 Stunde vor der Schaltsekunde für 1 Stunde, d.h. ab Minute 1)

        // 20       S                       1       Startbit für Zeitinformation

        // 21                        1              Minuten  1er
        // 22                        2              Minuten  2er
        // 23                        4              Minuten  4er
        // 24                        8              Minuten  8er
        // 25                       10              Minuten 10er
        // 26                       20              Minuten 20er
        // 27                       40              Minuten 40er
        // 28       P1                              Prüfbit 1 (gerade Parität)

        // 29                        1              Stunden  1er
        // 30                        2              Stunden  2er
        // 31                        4              Stunden  4er
        // 32                        8              Stunden  8er
        // 33                       10              Stunden 10er
        // 34                       20              Stunden 20er
        // 35       P2                              Prüfbit 2 (gerade Parität)

        // 36                        1              Tag  1er
        // 37                        2              Tag  2er
        // 38                        4              Tag  4er
        // 39                        8              Tag  8er
        // 40                       10              Tag 10er
        // 41                       20              Tag 20er

        // 42                        1              Wochentag 1er (Mo = 1, Di = 2, Mi = 3,
        // 43                        2              Wochentag 2er (Do = 4, Fr = 5, Sa = 6,
        // 44                        4              Wochentag 4er (So = 7)

        // 45                        1              Monat  1er
        // 46                        2              Monat  2er
        // 47                        4              Monat  4er
        // 48                        8              Monat  8er
        // 49                       10              Monat 10er

        // 50                        1              Jahr  1er
        // 51                        2              Jahr  2er
        // 52                        4              Jahr  4er
        // 53                        8              Jahr  8er
        // 54                       10              Jahr 10er
        // 55                       20              Jahr 20er
        // 56                       40              Jahr 40er
        // 57                       80              Jahr 80er

        // 58       P3                              Prüftbit 3 (gerade Parität)

        // 59       sync                            Sync Marke, kein Impuls (übliches Minutenende)
        // 59                               0       Schaltsekunde (sehr selten, nur nach Ankündigung)
        // 60       sync                            Sync Marke, kein Impuls (nur nach Schaltsekunde)

        // Falls eine Schaltsekunde eingefügt wird, wird bei Bit 59 eine Sekundenmarke gesendet.
        // Der Syncimpuls erfolgt dann in Sekunde 60 statt 59. Üblicherweise wird eine 0 als Bit 59 gesendet

        // Üblicherweise springt die Uhr beim Wechsel Winterzeit nach Sommerzeit von 1:59:59 auf 3:00:00
        //                               beim Wechsel Sommerzeit nach Winterzeit von 2:59:59 auf 2:00:00

        // Die Zeitinformation wird immer 1 Minute im Vorraus übertragen. D.h. nach der Syncmarke hat
        // man die aktuelle Zeit

        // http://www.dcf77logs.de/SpecialFiles.aspx

        // Schaltsekunden werden in Deutschland von der Physikalisch-Technischen Bundesanstalt festgelegt,
        // die allerdings dazu nur die international vom International Earth Rotation and Reference Systems
        // Service (IERS) festgelegten Schaltsekunden übernimmt. Im Mittel sind Schaltsekunden etwa alle 18
        // Monate nötig und werden vorrangig am 31. Dezember oder 30. Juni, nachrangig am 31. März oder
        // 30. September nach 23:59:59 UTC (also vor 1:00 MEZ bzw. 2:00 MESZ) eingefügt. Seit der Einführung
        // des Systems 1972 wurden ausschließlich die Zeitpunkte im Dezember und Juni benutzt.
    };

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

    #elif defined(STANDALONE)
    #else
        #error Unsupported controller architecture
    #endif


    #define sprint(...)   Serial.print(__VA_ARGS__)
    #define sprintln(...) Serial.println(__VA_ARGS__)

    namespace Binning {
        template <typename uint_t>
        struct lock_quality_tt {
            uint_t lock_max;
            uint_t noise_max;
        };

        template <uint8_t significant_bits>
        void score (uint8_t& data, const BCD::bcd_t input, const BCD::bcd_t candidate) {
            using namespace Arithmetic_Tools;
            const uint8_t the_score = significant_bits - bit_count(input.val ^ candidate.val);
            bounded_add(data, the_score);
        }

        template <typename data_t, typename signal_t, signal_t signal_max>
        void score(data_t &value, const signal_t signal, const bool bit) {
            // it is assumed that the caller takes care of potential overflow condition
            // before calling the score function
            if (bit) {
                value += signal;
            } else {
                value += signal_max - signal;
            }
        }

        template <typename data_type, typename noise_type, uint32_t number_of_bins>
        struct bins_t {
            typedef data_type data_t;
            typedef typename TMP::uval_t<number_of_bins>::type index_t;
            typedef lock_quality_tt<noise_type> lock_quality_t;

            data_t  data[number_of_bins];
            index_t tick;

            noise_type  noise_max;
            noise_type  max;
            index_t     max_index;

            void setup() {
                for (index_t index = 0; index < number_of_bins; ++index) {
                    data[index] = 0;
                }
                tick = 0;

                max = 0;
                max_index = number_of_bins + 1;
                noise_max = 0;
            }

            void advance_tick() {
                if (tick < number_of_bins - 1) {
                    ++tick;
                } else {
                    tick = 0;
                }
            }

            void get_quality(lock_quality_t & lock_quality) {
                CRITICAL_SECTION {
                    lock_quality.lock_max  = max;
                    lock_quality.noise_max = noise_max;
                }
            }

            uint8_t get_quality_factor() {
                noise_type max;
                noise_type delta;

                CRITICAL_SECTION {
                    max = this->max;

                    if (max <= this->noise_max) {
                        return 0;
                    }
                    delta = max - this->noise_max;
                }

                if (TMP::equal<noise_type, uint32_t>::val) {
                    // noise_type equals uint32_t --> typically convolution and other integration style stuff
                    uint8_t log2_plus_1 = 0;
                    while (max) {
                        max >>= 1;
                        ++log2_plus_1;
                    }

                    // crude approximation for delta/log2(max)
                    while (log2_plus_1) {
                        log2_plus_1 >>= 1;
                        delta >>= 1;
                    }

                    return delta<256? delta: 255;
                }

                if (TMP::equal<noise_type, uint8_t>::val) {
                    // noise_type equals uint8_t --> typically standard binners

                    // we define the quality factor as
                    //   (delta) / ld (max + 3)

                    // unfortunately this is prohibitive expensive to compute

                    // --> we need some shortcuts
                    // --> we will cheat a lot

                    // lookup for ld(n):
                    //   4 -->  2,  6 -->  2.5,   8 -->  3,  12 -->  3.5
                    // above 16 --> only count the position of the leading digit

                    uint8_t quality_factor;
                    if (max >= 32-3) {
                        // delta / ld(max+3) ~ delta / ld(max)
                        uint8_t log2 = 0;
                        while (max > 0) {
                            max >>= 1;
                            ++log2;
                        }
                        log2 -= 1;
                        // now 15 >= log2 >= 5
                        // multiply by 256/log2 and divide by 256
                        const uint16_t multiplier =
                            log2 > 12? log2 > 13? log2 > 14? 256/15
                                                           : 256/14
                                                           : 256/13
                                     : log2 > 8 ? log2 > 10? log2 > 11? 256/12
                                                                      : 256/11
                                                           : log2 >  9? 256/10
                                                                      : 256/ 9
                                                : log2 >  6? log2 >  7? 256/ 8
                                                                      : 256/ 7
                                                           : log2 >  5? 256/ 6
                                                                      : 256/ 5;
                        quality_factor = ((uint16_t)delta * multiplier) >> 8;

                    } else if (max >= 16-3) {
                        // delta / 4
                        quality_factor = delta >> 2;

                    } else if (max >= 12-3) {
                        // delta / 3.5
                        // we know delta <= max < 16-3 = 13 --> delta <= 12
                        quality_factor = delta >= 11? 3:
                        delta >=  7? 2:
                        delta >=  4? 1:
                        0;

                    } else if (max >= 8-3) {
                        // delta / 3
                        // we know delta <= max < 12-3 = 9 --> delta <= 8
                        quality_factor = delta >= 6? 2:
                        delta >= 3? 1:
                        0;

                    } else if (max >= 6-3) {
                        // delta / 2.5
                        // we know delta <= max < 8-3 = 5 --> delta <= 4
                        quality_factor = delta >= 3? 1: 0;

                    } else {  // if (max >= 4-3) {
                        // delta / 2
                        quality_factor = delta >> 1;
                    }
                    return quality_factor;
                }

                // create compile time error if we encounter an unexpectedt type
                typedef bool assert_type_is_known[TMP::equal<noise_type, uint8_t>::val ||
                                                  TMP::equal<noise_type, uint32_t>::val ? 0: -1];
            }

            void debug() {
                sprint(this->get_time_value().val, HEX);
                sprint(F(" Tick: "));
                sprint(this->tick);
                sprint(F(" Quality: "));
                sprint(this->max, DEC);
                sprint('-');
                sprint(this->noise_max, DEC);
                sprint(F(" Max Index: "));
                sprint(this->max_index, DEC);
                sprint(F(" Quality Factor: "));
                sprintln(this->get_quality_factor(), DEC);
                sprint('>');
            }

            void dump() {
                for (index_t index = 0; index < number_of_bins-1; ++index) {
                    sprint(data[index]);
                    sprint(',');
                }
                sprintln(data[number_of_bins-1]);
            }
        };


        template <typename data_type, uint32_t number_of_bins>
        struct Decoder : bins_t<data_type, data_type, number_of_bins> {
            typedef typename bins_t<data_type, data_type, number_of_bins>::index_t index_t;
            typedef data_type data_t;

            void compute_max_index() {
                this->noise_max = 0;
                this->max = 0;
                this->max_index = number_of_bins + 1;
                for (index_t index = 0; index < number_of_bins; ++index) {
                    const data_t bin_data = this->data[index];

                    if (bin_data >= this->max) {
                        this->noise_max = this->max;
                        this->max = bin_data;
                        this->max_index = index;
                    } else if (bin_data > this->noise_max) {
                        this->noise_max = bin_data;
                    }
                }
            }

            BCD::bcd_t get_time_value() {
                // there is a trade off involved here:
                //    low threshold --> lock will be detected earlier
                //    low threshold --> if lock is not clean output will be garbled
                //    a proper lock will fix the issue
                //    the question is: which start up behaviour do we prefer?
                const uint8_t threshold = 2;

                const index_t offset = (number_of_bins == 60 ||
                                        number_of_bins == 24 ||
                                        number_of_bins == 10)? 0x00: 0x01;

                if (this->max - this->noise_max >= threshold) {
                    return BCD::int_to_bcd((this->max_index + this->tick + 1) % number_of_bins + offset);
                } else {
                    BCD::bcd_t undefined;
                    undefined.val = 0xff;
                    return undefined;
                }
            }

            void debug() {
                this->debug();

                for (index_t index = 0; index < number_of_bins; ++index) {
                    sprint((index == this->max_index ||
                            index == ((this->max_index+1) % number_of_bins))
                           ? '|': ',');
                    sprint(this->data[index], HEX);
                }
                sprintln();
            }

            template <uint8_t significant_bits, bool with_parity>
            void hamming_binning(const BCD::bcd_t input) {
                using namespace Arithmetic_Tools;
                using namespace BCD;

                if (this->max > 255-significant_bits) {
                    // If we know we can not raise the maximum any further we
                    // will lower the noise floor instead.
                    for (index_t bin_index = 0; bin_index <number_of_bins; ++bin_index) {
                        bounded_decrement<significant_bits>(this->data[bin_index]);
                    }
                    this->max -= significant_bits;
                    bounded_decrement<significant_bits>(this->noise_max);
                }

                const index_t offset = number_of_bins-1-this->tick;
                index_t bin_index = offset;
                // for minutes, hours have parity and start counting at 0
                // for days, weeks, month we have no parity and start counting at 1
                // for years and decades we have no parity and start counting at 0
                bcd_t candidate;
                candidate.val = (with_parity || number_of_bins == 10)? 0x00: 0x01;
                for (index_t pass=0; pass < number_of_bins; ++pass) {
                    if (with_parity) {
                        candidate.bit.b7 = parity(candidate.val);
                        score<significant_bits>(this->data[bin_index], input, candidate);
                        candidate.bit.b7 = 0;
                    } else {
                        score<significant_bits>(this->data[bin_index], input, candidate);
                    }

                    bin_index = bin_index < number_of_bins-1? bin_index+1: 0;
                    increment(candidate);
                }
            }

            template <typename signal_t, signal_t signal_max, uint8_t signal_bitno_offset, uint8_t significant_bits, bool with_parity,
                    bool zeroBased, bool lsbFirst>
            void BCD_binning(const uint8_t bitno_with_offset, const signal_t signal) {
                using namespace Arithmetic_Tools;
                using namespace BCD;

                if (bitno_with_offset < signal_bitno_offset) { return; }
                uint8_t bitno = bitno_with_offset - signal_bitno_offset;

                const uint8_t number_of_bits = significant_bits + with_parity;
                if (bitno > number_of_bits) { return; }
                if (bitno == number_of_bits) { compute_max_index(); return; }

                const bool isParityBit = with_parity && (bitno==significant_bits);

                // For transmissions that receive msb first we must reverse the order, with the
                // exception of the parity bit which may come last (although no example yet)
                if (!lsbFirst) {
                    // Parity always goes at the end
                    if (!isParityBit) {
                        // Leave as is since this is the final parity bit
                        bitno = (significant_bits-1)-bitno;
                    }
                }

                const data_t upper_bin_bound = TMP::equal<data_t, uint8_t>::val? 255 : (60 * number_of_bits * signal_max) / 2;

                // for minutes, hours have parity and start counting at 0
                // for days, weeks, month we have no parity and start counting at 1
                // for years and decades we have no parity and start counting at 0

                bcd_t candidate;
                candidate.val = zeroBased ? 0x00: 0x01;

                data_t min = upper_bin_bound;
                data_t max = 0;

                const index_t offset = number_of_bins-1-this->tick;
                index_t bin_index = offset;
                for (index_t pass=0; pass < number_of_bins; ++pass) {
                    data_t& current_bin = this->data[bin_index];
                    if (bitno < significant_bits) {
                        // score vs. bcd value determined by pass
                        score<data_t, signal_t, signal_max>(current_bin, signal, (candidate.val >> bitno) & 1);
                    } else
                    if (isParityBit) {
                        // score vs. parity bit
                        score<data_t, signal_t, signal_max>(current_bin, signal, parity(candidate.val));
                    }

                    maximize(max, current_bin);
                    minimize(min, current_bin);

                    bin_index = bin_index < number_of_bins-1? bin_index+1: 0;
                    increment(candidate);
                }

                if (max - min <= upper_bin_bound) {
                    // enforce min == 0, upper bound will be fine anyway
                    for (index_t pass=0; pass < number_of_bins; ++pass) {
                        this->data[pass] -= min;
                    }
                }

                if (max >= upper_bin_bound - number_of_bits) {
                    // runs at most once per minute

                    // enforce max <= upper_bin_bound
                    for (index_t pass=0; pass < number_of_bins; ++pass) {
                        data_t & current_bin = this->data[pass];

                        bounded_decrement<number_of_bits>(current_bin);
                        maximize(current_bin, (data_t) 0);
                    }
                }
            }
        };


        template <typename data_type, uint32_t number_of_bins>
        struct Convoluter : bins_t<data_type, uint32_t, number_of_bins> {
            typedef typename bins_t<data_type, data_type, number_of_bins>::index_t index_t;
            typedef data_type data_t;

            void debug() {
                for (index_t index = 0; index < number_of_bins; ++index) {
                    sprint((
                        index == this->max_index                         ||
                        index == ((this->max_index+10) % number_of_bins) ||
                        index == ((this->max_index+20) % number_of_bins))
                        ? '|': ',');
                        sprint(this->data[index], HEX);
                }
                sprintln();
            }
        };

        template <typename data_t, uint32_t number_of_bins>
        struct Binner {
            typedef typename TMP::uval_t<number_of_bins>::type index_t;

            typedef uint8_t noise_type;
            typedef lock_quality_tt<uint8_t> lock_quality_t;

            struct bins_t {
                data_t  data[number_of_bins];
                index_t tick;

                data_t  noise_max;
                data_t  max;
                index_t max_index;
            };
            bins_t bins;

            void setup();
            void advance_tick();
            void get_quality(lock_quality_tt<uint8_t>  &lock_quality);
            uint8_t get_quality_factor();

            void compute_max_index();
            BCD::bcd_t get_time_value();
            void debug();

            template <uint8_t significant_bits, bool with_parity>
            void hamming_binning(const BCD::bcd_t input);

            template <typename signal_t, signal_t signal_max, uint8_t significant_bits, bool with_parity>
            void BCD_binning(const uint8_t bitno, const signal_t signal);
        };
    }

    template <typename Clock_Controller>
    struct DCF77_Demodulator : Binning::Convoluter<uint16_t, Configuration::phase_lock_resolution> {
        static const index_t bin_count = Configuration::phase_lock_resolution;
        static const uint16_t samples_per_second = 1000;

        static const uint16_t samples_per_bin = samples_per_second / bin_count;
        static const uint16_t bins_per_10ms  = bin_count / 100;
        static const uint16_t bins_per_50ms  =  5 * bins_per_10ms;
        static const uint16_t bins_per_60ms  =  6 * bins_per_10ms;
        static const uint16_t bins_per_100ms = 10 * bins_per_10ms;
        static const uint16_t bins_per_200ms = 20 * bins_per_10ms;
        static const uint16_t bins_per_400ms = 40 * bins_per_10ms;
        static const uint16_t bins_per_500ms = 50 * bins_per_10ms;
        static const uint16_t bins_per_600ms = 60 * bins_per_10ms;

        uint16_t wrap(const uint16_t value) {
            // faster modulo function which avoids division
            uint16_t result = value;
            while (result >= bin_count) {
                result-= bin_count;
            }
            return result;
        }


        const uint32_t ticks_to_drift_one_tick       =  30000UL;
        const uint32_t tuned_ticks_to_drift_one_tick = 300000UL;

        // N times the clock precision shall be smaller than 1/resolution
        // how many seconds may be cummulated
        // this controls how slow the filter may be to follow a phase drift
        // N times the clock precision shall be smaller 1/bin_count
        // untuned clock ~30 ppm, 100 bins => N < 300
        // tuned clock ~2 ppm,    100 bins => N < 3600
        uint16_t N = ticks_to_drift_one_tick / bin_count;
        void set_has_tuned_clock() {
            // will be called once crystal is tuned to better than 1 ppm.
            N = tuned_ticks_to_drift_one_tick / bin_count;
        }

        int32_t integral = 0;
        int32_t running_max = 0;
        index_t running_max_index = 0;
        int32_t running_noise_max = 0;
        uint8_t phase_binning(const uint8_t input)
                __attribute__((always_inline)) {
            Binning::Convoluter<uint16_t, Configuration::phase_lock_resolution>::advance_tick();
            const index_t tick = this->tick;

            data_t & data = this->data[tick];

            if (data > N) {
                data = N;
            }

            if (input) {
                if (data < N) {
                    ++data;
                }
            } else {
                if (data > 0) {
                    --data;
                }
            }

            {
                // ck = convolution_kernel
                const index_t ck_start_tick  = wrap(tick+((10-2)*bins_per_100ms));
                const index_t ck_middle_tick = wrap(tick+((10-1)*bins_per_100ms));

                if (integral > running_max) {
                    running_max = integral;
                    running_max_index = ck_start_tick;
                }
                if (tick == wrap(this->max_index + 2*bins_per_200ms)) {
                    running_noise_max = integral;
                }

                if (tick == 0) {
                    // one period has passed, flush the result

                    this->max = running_max;
                    this->max_index = running_max_index;
                    this->noise_max = running_noise_max;

                    // reset running_max for next period
                    running_max = 0;

                    // no need to reset the noise_max as this is
                    // not computed by comparision against the integral
                    // but by index lookup
                }

                integral -= (int32_t)this->data[ck_start_tick]*2;
                integral += (int32_t)this->data[ck_middle_tick];
                integral += (int32_t)this->data[tick];
            }

            return tick;
        }

        TMP::uval_t<bin_count>::type count = 0;
        uint8_t decoded_data = 0;

#ifndef MSF60
        void decode_200ms(const uint8_t input, const uint8_t bins_to_go)
        __attribute__((always_inline)) {
            // will be called for each bin during the "interesting" 200 ms
            count += input;
            if (bins_to_go > bins_per_100ms) {
                if (bins_to_go == bins_per_100ms + 1) {
                    decoded_data = count > bins_per_50ms? 2: 0;
                    count = 0;
                }
            } else {
                if (bins_to_go == 0) {
                    decoded_data += count > bins_per_50ms? 1: 0;
                    count = 0;
                    // pass control further
                    // decoded_data: 3 --> 1
                    //               2 --> 0,
                    //               1 --> undefined,
                    //               0 --> sync_mark
                    Clock_Controller::process_single_tick_data((DCF77::tick_t) decoded_data);
                }
            }
        }
#else
        void decode_500ms(const uint8_t input, const uint8_t bins_to_go)
            __attribute__((always_inline)) {
            // will be called for each bin during the "interesting" 200 ms

            static bool initial = false;
            static bool trail = false;
            static bool bitA = false;
            static bool bitB = false;

            // pass control further
            // decoded_data: 0 --> A0_B0,
            //               1 --> A0_B1,
            //               2 --> A1_B0,
            //               3 --> A1_B1,
            //               4 --> undefined,
            //               5 --> min_marker
            count += input;

            switch (bins_to_go) {

                // Second marker 100ms in length (1ms - 100ms after start of second)
                case 40:
                    initial = (count > 5);
                    count=0;
                    break;

                case 35:
                    count=0;
                    break;

                    // Bit A 100ms in length after second marker (101ms - 200ms after start of second)
                case 30:
                    bitA = (count > 3);
                    count = 0;
                    break;

                case 25:
                    count=0;
                    break;

                    // Bit B 100ms in length after bit A (201ms - 300ms after start of second)
                case 20:
                    bitB = (count > 3);
                    count = 0;
                    break;

                case 16:
                    count=0;
                    break;

                    // this case reads 301ms - 400ms after start of second
                case 0:
                    trail = (count > 8);

                    // If all five sections are High then must be MM
                    if ((bitA && bitB)&&(initial && trail)) {
                        decoded_data=5;
                    } else if ((!initial)||(trail)) {
                        decoded_data = 4;
                    } else
                    {
                        decoded_data = (((uint8_t)bitA)<<1) + ((uint8_t)bitB);
                    }

                    Clock_Controller::process_single_tick_data((DCF77::tick_t) decoded_data);
                    count = 0;
                    break;
            }
        }
#endif

#ifndef MSF60
        TMP::uval_t<bins_per_200ms+1>::type bins_to_go = 0;
#else
        TMP::uval_t<bins_per_500ms+1>::type bins_to_go = 0;
#endif
        void detector_stage_2(const uint8_t input)
              __attribute__((always_inline)) {
            const index_t current_bin = this->tick;

            if (bins_to_go == 0) {
                if (wrap((bin_count + current_bin - this->max_index)) <= bins_per_100ms ||   // current_bin at most 100ms after phase_bin
                    wrap((bin_count + this->max_index - current_bin)) <= bins_per_10ms  ) {  // current bin at 1 tick before phase_bin
                    // if phase bin varies to much during one period we will always be screwed in may ways...
                    // last tick of current second
                    Clock_Controller::flush();
                    // start processing of bins
#ifndef MSF60
                    bins_to_go = bins_per_200ms + 2*bins_per_10ms;
#else
                    bins_to_go = bins_per_500ms;
#endif
                }
            }

            if (bins_to_go > 0) {
                --bins_to_go;

                // this will be called for each bin in the "interesting" 200ms for DCF
                // or "interesting" 500mS for MSF
                // this is also a good place for a "monitoring hook"
#ifndef MSF60
                decode_200ms(input, bins_to_go);
#else
                decode_500ms(input, bins_to_go);
#endif
            }
        }

        // The struct below would better capture the whole code of detector_stage_1 for the_score
        // "with averages" case. However the straighforward approach leads to lots of nasty
        // complications with template scoping. Hence detector_stage_1 is still a function
        // and detector dispatches.
        struct stage_with_averages {
            uint8_t sample_count = 0;
            uint8_t average = 0;

            void reset() __attribute__ ((always_inline)) {
                sample_count = 0;
                average = 0;
            }
            void reduce(const uint8_t sampled_data) __attribute__ ((always_inline)){
                average += sampled_data;
                ++sample_count;
            }
            bool data_ready() const __attribute__ ((always_inline)) {
                return sample_count >= samples_per_bin;
            }
            uint8_t avg() const __attribute__ ((always_inline)) {
                return average > samples_per_bin / 2;
            }
        };

        struct dummy_stage {
            void    reset()                            const {}
            void    reduce(const uint8_t sampled_data) const {}
            bool    data_ready()                       const {}
            uint8_t avg()                              const {}
        };

        static const bool requires_averages = samples_per_bin > 1;
        typename TMP::if_t<requires_averages, stage_with_averages, dummy_stage>::type stage_1;

        void detector_stage_1(const uint8_t sampled_data)
             __attribute__((always_inline)) {

            stage_1.reduce(sampled_data);
            if (stage_1.data_ready()) {
                // once all samples for the current bin are captured the bin gets updated
                // that is each 10ms control is passed to stage 2
                const uint8_t input = stage_1.avg();

                phase_binning(input);
                detector_stage_2(input);

                stage_1.reset();
            }
        }

        void detector(const uint8_t sampled_data) {
            if (samples_per_bin > 1) {
                // average samples
                detector_stage_1(sampled_data);
            } else {
                // no averaging required
                phase_binning(sampled_data);
                detector_stage_2(sampled_data);
            }
        }

        void debug() {
            sprint(F("Phase: "));
            Binning::Convoluter<uint16_t, Configuration::phase_lock_resolution>::debug();
        }

        void debug_verbose() {
            // attention: debug_verbose is not really thread save
            //            thus the output may contain unexpected artifacts
            //            do not rely on the output of one debug cycle
            debug();

            sprintln(F("max_index, max, index, integral"));

            uint32_t integral = 0;
            for (index_t bin = 0; bin < bins_per_100ms; ++bin) {
                integral += ((uint32_t)this->data[bin])<<1;
            }
            for (index_t bin = bins_per_100ms; bin < bins_per_200ms; ++bin) {
                integral += (uint32_t)this->data[bin];
            }

            uint32_t max = 0;
            index_t max_index = 0;
            for (index_t bin = 0; bin < bin_count; ++bin) {
                if (integral > max) {
                    max = integral;
                    max_index = bin;
                }

                integral -= (uint32_t)this->data[bin]<<1;
                integral += (uint32_t)(this->data[wrap(bin + bins_per_100ms)] +
                this->data[wrap(bin + bins_per_200ms)]);

                sprint(max_index);
                sprint(F(", "));
                sprint(max);
                sprint(F(", "));
                sprint(bin);
                sprint(F(", "));
                sprintln(integral);
            }

            // max_index indicates the position of the 200ms second signal window.
            // Now how can we estimate the noise level? This is very tricky because
            // averaging has already happened to some extend.

            // The issue is that most of the undesired noise happens around the signal,
            // especially after high->low transitions. So as an approximation of the
            // noise I test with a phase shift of 200ms.
            uint32_t noise_max = 0;

            const index_t noise_index = wrap(max_index + bins_per_200ms);
            for (index_t bin = 0; bin < bins_per_100ms; ++bin) {
                noise_max += ((uint32_t)this->data[wrap(noise_index + bin)])<<1;
            }
            for (index_t bin = bins_per_100ms; bin < bins_per_200ms; ++bin) {
                noise_max += (uint32_t)this->data[wrap(noise_index + bin)];
            }

            sprint(F("noise_index, noise_max: "));
            sprint(noise_index);
            sprint(F(", "));
            sprintln(noise_max);
        }
    };

    struct DCF77_Flag_Decoder {
        bool abnormal_transmitter_operation;
        int8_t timezone_change_scheduled;
        int8_t uses_summertime;
        int8_t leap_second_scheduled;
        int8_t date_parity;

#ifdef MSF60
        int8_t year_parity;     // 54B = Decade + Year 17A-24A
        int8_t weekday_parity;  // 56B = Day-of-week 36A-38A
        int8_t time_parity;     // 57B = Hour + Minute 39A-51A
#endif

        void setup();

        void cummulate(int8_t &average, bool count_up);

        void process_tick(const uint8_t current_second, const uint8_t tick_value);

        void reset_after_previous_hour();
        void reset_before_new_day();

        bool get_uses_summertime();
        bool get_abnormal_transmitter_operation();
        bool get_timezone_change_scheduled();
        bool get_leap_second_scheduled();

        void get_quality(uint8_t &uses_summertime_quality,
                        uint8_t &timezone_change_scheduled_quality,
                        uint8_t &leap_second_scheduled_quality);

        void debug();
    };

    struct DCF77_Decade_Decoder : public Binning::Decoder<uint8_t, 10> {
        BCD::bcd_t decade_data;

        void process_tick(const uint8_t current_second, const uint8_t tick_value);
        void debug();
    };

    struct DCF77_Year_Decoder : public Binning::Decoder<uint8_t, 10> {
        DCF77_Decade_Decoder Decade_Decoder;
        BCD::bcd_t year_data;

        void advance_tick();
        void process_tick(const uint8_t current_second, const uint8_t tick_value);
        void get_quality(lock_quality_t &lock_quality);
        uint8_t get_quality_factor();
        BCD::bcd_t get_time_value();
        void setup();

        void debug();
    };

    struct DCF77_Month_Decoder : public Binning::Decoder<uint8_t, 12> {
        BCD::bcd_t month_data;

        void process_tick(const uint8_t current_second, const uint8_t tick_value);
        void debug();
    };

    struct DCF77_Weekday_Decoder : public Binning::Decoder<uint8_t, 7> {
        BCD::bcd_t weekday_data;

        void process_tick(const uint8_t current_second, const uint8_t tick_value);
        void debug();
    };

    struct DCF77_Day_Decoder : public Binning::Decoder<uint8_t, 31> {
        BCD::bcd_t day_data;

        void process_tick(const uint8_t current_second, const uint8_t tick_value);
        void debug();
    };

    struct DCF77_Hour_Decoder : public Binning::Decoder<uint8_t, 24> {
        BCD::bcd_t hour_data;

        void process_tick(const uint8_t current_second, const uint8_t tick_value);
        void debug();
    };

    struct DCF77_Minute_Decoder : public Binning::Decoder<uint8_t, 60> {
//        BCD::bcd_t minute_data;

        void process_tick(const uint8_t current_second, const uint8_t tick_value);
        void debug();
    };

    struct DCF77_Second_Decoder : public Binning::Decoder<uint8_t, 60> {
        static const uint8_t seconds_per_minute = 60;

        // threshold:
        //    lower it to get a faster second lock
        //    but then risk to garble the successive stages during startup
        //    --> to low and total startup time will increase
        static const uint8_t lock_threshold = 12;

#ifndef MSF60
        DCF77::serialized_clock_stream convolution_kernel;
#else
        DCF77::serialized_clock_stream_pair convolution_kernel;
#endif


        // used to determine how many of the predicted bits are actually observed,
        // also used to indicate if convolution is already applied
        static const uint8_t convolution_binning_not_ready = 0xff;
        uint8_t prediction_match;
        uint8_t buffered_match;

        void setup();
        uint8_t get_prediction_match();
        void set_convolution_time(const DCF77_Encoder &now);
        void convolution_binning(const uint8_t tick_data);
#ifdef MSF60
        uint8_t get_previous_n_tick(const uint8_t n);
#endif
        void sync_mark_binning(const uint8_t tick_data);
        uint8_t get_time_value();
        void binning(const DCF77::tick_t tick_data);
        void debug();
    };

    template <typename Clock_Controller>
    struct DCF77_Local_Clock {
        Clock::clock_state_t clock_state;
        DCF77_Encoder local_clock_time;
        volatile bool second_toggle;
        uint16_t tick;

        // This will take more than 100 years to overflow.
        // An overflow would indicate that the clock is
        // running for >100 years without a sync.
        // --> It is pointless to handle this.
        uint32_t unlocked_seconds;

        void setup() {
            clock_state = Clock::useless;
            tick = 0;
            unlocked_seconds = 0;
            max_unlocked_seconds = 3000;

            local_clock_time.reset();
        }

        void process_1_Hz_tick(const DCF77_Encoder &decoded_time) {
            uint8_t quality_factor = Clock_Controller::get_overall_quality_factor();

            if (quality_factor > 1) {
                if (clock_state != Clock::synced) {
                    Clock_Controller::sync_achieved_event_handler();
                    clock_state = Clock::synced;
                }
            } else if (clock_state == Clock::synced) {
                Clock_Controller::sync_lost_event_handler();
                clock_state = Clock::locked;
            }

            while (true) {
                switch (clock_state) {
                    case Clock::useless: {
                        if (quality_factor > 0) {
                            clock_state = Clock::dirty;
                            break;  // goto dirty state
                        } else {
                            second_toggle = !second_toggle;
                            return;
                        }
                    }

                    case Clock::dirty: {
                        if (quality_factor == 0) {
                            clock_state = Clock::useless;
                            second_toggle = !second_toggle;
                            local_clock_time.reset();
                            return;
                        } else {
                            tick = 0;
                            local_clock_time = decoded_time;
                            Clock_Controller::local_clock_flush(decoded_time);
                            second_toggle = !second_toggle;
                            return;
                        }
                    }

                    case Clock::synced: {
                        tick = 0;
                        local_clock_time = decoded_time;
                        Clock_Controller::local_clock_flush(decoded_time);
                        second_toggle = !second_toggle;
                        return;
                    }

                    case Clock::locked: {
                        if (Clock_Controller::get_demodulator_quality_factor() > 10) {
                            // If we are not sure about leap seconds we will skip
                            // them. Worst case is that we miss a leap second due
                            // to noisy reception. This may happen at most once a
                            // year.
                            local_clock_time.leap_second_scheduled = false;

                            // autoset_control_bits is not required because
                            // advance_second will call this internally anyway
                            //local_clock_time.autoset_control_bits();
                            local_clock_time.advance_second();
                            Clock_Controller::local_clock_flush(local_clock_time);
                            tick = 0;
                            second_toggle = !second_toggle;
                            return;
                        } else {
                            clock_state = Clock::unlocked;
                            Clock_Controller::phase_lost_event_handler();
                            unlocked_seconds = 0;
                            return;
                        }
                    }

                    case Clock::unlocked: {
                        if (Clock_Controller::get_demodulator_quality_factor() > 10) {
                            // Quality is somewhat reasonable again, check
                            // if the phase offset is in reasonable bounds.
                            if (200 < tick && tick < 800) {
                                // Deviation of local phase vs. decoded phase exceeds 200 ms.
                                // So something is not OK. We can not relock.
                                // On the other hand we are still below max_unlocked_seconds.
                                // --> Stay in unlocked mode.
                                return;
                            } else {
                                // Phase drift was below 200 ms and clock was not unlocked
                                // for max_unlocked_seconds. So we know that we are close
                                // enough to the proper time. Only exception:
                                //     missed leap seconds.
                                // We ignore this issue as it is not worse than running in
                                // free mode.
                                clock_state = Clock::locked;
                                if (tick < 200) {
                                    // time output was handled at most 200 ms before
                                    tick = 0;
                                    return;
                                } else {
                                    break;  // goto locked state
                                }
                            }
                        } else {
                            // quality is still poor, we stay in unlocked mode
                            return;
                        }
                    }

                    case Clock::free: {
                        return;
                    }
                }
            }
        }

        void process_1_kHz_tick() {
            ++tick;

            if (clock_state == Clock::synced || clock_state == Clock::locked) {
                // the important part is 150 < 200,
                // otherwise it will fall through to free immediately after changing to unlocked
                if (tick >= 1150) {
                    // The 1 Hz pulse was locked but now
                    // it is definitely out of phase.
                    unlocked_seconds = 1;

                    // 1 Hz tick missing for more than 1200ms
                    clock_state = Clock::unlocked;
                    Clock_Controller::phase_lost_event_handler();
                }
            }

            if (clock_state == Clock::unlocked || clock_state == Clock::free) {
                if (tick >= 1000) {
                    tick -= 1000;

                    // If we are not sure about leap seconds we will skip
                    // them. Worst case is that we miss a leap second due
                    // to noisy reception. This may happen at most once a
                    // year.
                    local_clock_time.leap_second_scheduled = false;

                    // autoset_control_bits is not required because
                    // advance_second will call this internally anyway
                    //local_clock_time.autoset_control_bits();
                    local_clock_time.advance_second();
                    Clock_Controller::local_clock_flush(local_clock_time);
                    second_toggle = !second_toggle;

                    ++unlocked_seconds;
                    if (unlocked_seconds > max_unlocked_seconds) {
                        clock_state = Clock::free;
                    }
                }
            }
        }

        uint32_t max_unlocked_seconds;
        void set_has_tuned_clock() {
            max_unlocked_seconds = 30000;
        }

        Clock::clock_state_t get_state() {
            return clock_state;
        }

        // non-blocking, reads current second
        void read_current_time(DCF77_Encoder &now) {
            CRITICAL_SECTION {
                now = local_clock_time;
            }
        }

        // blocking till start of next second
        void get_current_time(DCF77_Encoder &now) {
            for (bool stopper = second_toggle; stopper == second_toggle; ) {
                // wait for second_toggle to toggle
                // that is wait for decoded time to be ready
            }
            read_current_time(now);
        }

        void debug() {
            sprint(F("Clock state: "));
            switch (clock_state) {
                case Clock::useless:  sprintln(F("useless"));  break;
                case Clock::dirty:    sprintln(F("dirty"));    break;
                case Clock::free:     sprintln(F("free"));     break;
                case Clock::unlocked: sprintln(F("unlocked")); break;
                case Clock::locked:   sprintln(F("locked"));   break;
                case Clock::synced:   sprintln(F("synced"));   break;
                default:       sprintln(F("undefined"));
            }
            sprint(F("Tick: "));
            sprintln(tick);
        }
    };

    struct DCF77_Frequency_Control {
        // Precision at tau min is 8 Hz == 0.5 ppm or better
        // This is because 340 m = 334 * 60 * 100 centiseconds = 2 004 000 centiseconds
        //                  34 m = 34 * 60 * 1000 milliseconds = 2 040 000 ms

        // Do not decrease this value!
        static const uint16_t tau_min_minutes = 2000000uL / (60uL * Configuration::phase_lock_resolution) + 1;

        // Precision at tau_max would be 0.5 Hz
        // This may be decreased if desired. Do not decrease below 2*tau_min.
        // 5334 * 6000 = 32 004 000 // 534 * 60000 = 32 040 000
        static const uint16_t tau_max_minutes = 32000000uL / (60uL * Configuration::phase_lock_resolution) + 1;

        // 1600 Hz@16MHz = 100 ppm = 16 pp16m
        // Theoretically higher values would be possible.
        // However if a tuning beyond 100 ppm is necessary then there is something
        // fundamentally wrong with the oscillator.
        // Unit of measure is parts per 16 Million [pp16m]
        static const int16_t max_total_adjust = 1600;

        static volatile int8_t confirmed_precision;

        #if defined(_AVR_EEPROM_H_)
        // indicator if data may be persisted to EEPROM
        static volatile boolean data_pending;
        #endif

        // 2*tau_max = 32 004 000 ticks = 5333 minutes or 533 minutes depending on the resolution
        //  60 000 centi seconds = 10 minutes
        // 600 000 milli seconds =  1 minute
        // maximum drift in 32 004 000 ticks @ 900 ppm would result
        // in a drift of +/- 28800 ticks
        // thus it is uniquely measured if we know it mod 60 000
        // However in the centi seconds case we will require slighty more
        // complicated logic due to the additional divider
        //template <Configuration::ticks_per_second_t phase_lock_resolution>
        struct generic_deviation_tracker_t {
            volatile uint16_t elapsed_minutes;
            volatile uint16_t elapsed_ticks_mod_60000;

            void start(const uint8_t minute_mod_10) {
                elapsed_ticks_mod_60000 = 0;
                elapsed_minutes = 0;
            }

            void process_tick() {
                if (elapsed_ticks_mod_60000 < 59999) {
                    ++elapsed_ticks_mod_60000;
                } else {
                    elapsed_ticks_mod_60000 = 0;
                    ++elapsed_minutes;
                }
            }

            int16_t compute_phase_deviation(uint8_t current_second, uint8_t current_minute_mod_10) {
                int32_t deviation =
                        ((int32_t) elapsed_ticks_mod_60000) -
                        ((int32_t) current_second - (int32_t) calibration_second) * 1000;

                // ensure we are between 30000 and -29999
                while (deviation >  30000) { deviation -= 60000; }
                while (deviation <=-30000) { deviation += 60000; }

                return deviation;
            }

            bool timeout() {
                return elapsed_minutes >= tau_max_minutes;
            }

            bool good_enough() {
                return elapsed_minutes >= tau_min_minutes;
            }
        };

        struct averaging_deviation_tracker_t : generic_deviation_tracker_t {
            uint8_t  start_minute_mod_10;
            uint8_t  divider = 0;

            void start(const uint8_t minute_mod_10) {
                generic_deviation_tracker_t::start(minute_mod_10);
                start_minute_mod_10 = minute_mod_10;
            }

            void process_tick() {
                if (divider < 9) {
                    ++divider;
                }  else {
                    divider = 0;

                    // in this
                    generic_deviation_tracker_t::process_tick();

                    if (elapsed_ticks_mod_60000 % 6000 == 0) {
                        ++elapsed_minutes;
                    }
                }
            }

            int16_t compute_phase_deviation(uint8_t current_second, uint8_t current_minute_mod_10) {
                int32_t deviation =
                        ((int32_t) elapsed_ticks_mod_60000) -
                        ((int32_t) current_second        - (int32_t) calibration_second)  * 100 -
                        ((int32_t) current_minute_mod_10 - (int32_t) start_minute_mod_10) * 6000;

                // ensure we are between 30000 and -29999
                while (deviation >  30000) { deviation -= 60000; }
                while (deviation <=-30000) { deviation += 60000; }

                return deviation;
            }
        };

        typedef TMP::if_t<Configuration::high_phase_lock_resolution,
                         generic_deviation_tracker_t,
                         averaging_deviation_tracker_t>::type deviation_tracker_t;
        static deviation_tracker_t deviation_tracker;

        // Seconds 0 and 15 already receive more computation than
        // other seconds thus calibration will run in second 5.
        static const int8_t calibration_second = 5;


        typedef struct {
            bool qualified : 1;
            bool running   : 1;
        } calibration_state_t;

        static volatile calibration_state_t calibration_state;
        static volatile int16_t deviation;

        static void restart_measurement();
        static void debug();
        static bool increase_tau();
        static bool decrease_tau();
        static void adjust();
        static void process_1_Hz_tick(const DCF77_Encoder &decoded_time);
        static void process_1_kHz_tick();

        static void qualify_calibration();
        static void unqualify_calibration();
// TODO msres: how to deal with lower vs. higher resolution, different arguments needed
        static int16_t compute_phase_deviation(uint8_t current_second, uint8_t current_minute_mod_10);

        static calibration_state_t get_calibration_state();
        // The phase deviation is only meaningful if calibration is running.
        static int16_t get_current_deviation();

        static void setup();


        #if defined(__AVR__)
        // Offset for writing to EEPROM / reading from EEPROM
        // this is necesarry if other libraries also want to
        // use EEPROM.
        // This library will use 8 bytes of EEPROM
        // 2 bytes for an identifier and 3 bytes for storing the
        // data redundantantly.
        static const uint16_t eeprom_base = 0x00;
        static void persist_to_eeprom(const int8_t adjust_steps, const int16_t adjust);  // this is slow, do not call during interrupt handling

        static void read_from_eeprom(int8_t &precision, int16_t &adjust);
        static void auto_persist();  // this is slow and messes with the interrupt flag, do not call during interrupt handling
        #endif
        // get the adjust step that was used for the last adjustment
        //   if there was no adjustment or if the frequency adjustment was poor it will return 0
        //   if the adjustment was from eeprom it will return the negative value of the persisted adjust step
        static int8_t get_confirmed_precision();
    };

    // Dummy class to parametrize a clock controller without frequency control
    struct DCF77_No_Frequency_Control {
        static void process_1_Hz_tick(const DCF77_Encoder &decoded_time);
        static void process_1_kHz_tick();

        static void qualify_calibration();
        static void unqualify_calibration();

        static void setup();

        #if defined(__AVR__)
        static void auto_persist();  // this is slow and messes with the interrupt flag, do not call during interrupt handling
        #endif
    };

    namespace Debug {
        void debug_helper(char data);
        void bcddigit(uint8_t data);
        void bcddigits(uint8_t data);
    }

    namespace Debug {
        void debug_helper(char data);
        void bcddigit(uint8_t data);
        void bcddigits(uint8_t data);
    }

    namespace DCF77_Naive_Bitstream_Decoder {
        void set_bit(const uint8_t second, const uint8_t value, DCF77_Encoder &now);
    }

    template <typename Frequency_Control>
    struct DCF77_Clock_Controller {
        static DCF77_Second_Decoder  Second_Decoder;
        static DCF77_Minute_Decoder  Minute_Decoder;
        static DCF77_Hour_Decoder    Hour_Decoder;
        static DCF77_Weekday_Decoder Weekday_Decoder;
        static DCF77_Day_Decoder     Day_Decoder;
        static DCF77_Month_Decoder   Month_Decoder;
        static DCF77_Year_Decoder    Year_Decoder;
        static DCF77_Flag_Decoder    Flag_Decoder;

        // blocking, will unblock at the start of the second
        static void get_current_time(DCF77_Encoder &now) {
            #if defined(_AVR_EEPROM_H_)
            auto_persist();
            #endif
            Local_Clock.get_current_time(now);
        }

        static void set_DCF77_Encoder(DCF77_Encoder &now) {
            now.second  = Second_Decoder.get_time_value();
            now.minute  = Minute_Decoder.get_time_value();
            now.hour    = Hour_Decoder.get_time_value();
            now.weekday = Weekday_Decoder.get_time_value();
            now.day     = Day_Decoder.get_time_value();
            now.month   = Month_Decoder.get_time_value();
            now.year    = Year_Decoder.get_time_value();

            now.abnormal_transmitter_operation = Flag_Decoder.get_abnormal_transmitter_operation();
            now.timezone_change_scheduled      = Flag_Decoder.get_timezone_change_scheduled();
            now.uses_summertime                = Flag_Decoder.get_uses_summertime();
            now.leap_second_scheduled          = Flag_Decoder.get_leap_second_scheduled();
        }

        static uint8_t leap_second;
        static DCF77_Encoder decoded_time;
        static void flush() {
            // This is called "at the end of each second / before the next second begins."
            // The call is triggered by the decoder stages. Thus it flushes the current
            // decoded time. If the decoders are out of sync this may not be
            // called at all.

            DCF77_Encoder now;
            DCF77_Encoder now_1;

            set_DCF77_Encoder(now);
            now_1 = now;

            // leap_second offset to compensate for the skipped second tick
            now.second += leap_second > 0;

            now.advance_second();
            now.autoset_control_bits();

            decoded_time.second = now.second;
            if (now.second == 0) {
                // the decoder will always decode the data for the NEXT minute
                // thus we have to keep the data of the previous minute
                decoded_time = now_1;
                decoded_time.second = 0;

                if (now.minute.val == 0x01) {
                    // We are at the last moment of the "old" hour.
                    // The data for the first minute of the new hour is now complete.
                    // The point is that we can reset the flags only now.
                    Flag_Decoder.reset_after_previous_hour();

                    now.uses_summertime = Flag_Decoder.get_uses_summertime();
                    now.timezone_change_scheduled = Flag_Decoder.get_timezone_change_scheduled();
                    now.leap_second_scheduled = Flag_Decoder.get_leap_second_scheduled();

                    now.autoset_control_bits();
                    decoded_time.uses_summertime                = now.uses_summertime;
                    decoded_time.timezone_change_scheduled      = now.timezone_change_scheduled;
                    decoded_time.leap_second_scheduled          = now.leap_second_scheduled;
                    decoded_time.abnormal_transmitter_operation = Flag_Decoder.get_abnormal_transmitter_operation();
                }

                if (now.hour.val == 0x23 && now.minute.val == 0x59) {
                    // We are now starting to process the data for the
                    // new day. Thus the parity flag is of little use anymore.

                    // We could be smarter though and merge synthetic parity information with measured historic values.

                    // that is: advance(now), see if parity changed, if not so --> fine, otherwise change sign of flag
                    Flag_Decoder.reset_before_new_day();
                }

                // reset leap second
                leap_second &= leap_second < 2;
            }

            // pass control to local clock
            Local_Clock.process_1_Hz_tick(decoded_time);
        }

        static Clock::output_handler_t output_handler; //= 0;
        static void set_output_handler(const Clock::output_handler_t new_output_handler) {
            output_handler = new_output_handler;
        }

        static void local_clock_flush(const DCF77_Encoder &decoded_time) {
            // This is the callback for the "local clock".
            // It will be called once per second.

            // It ensures that the local clock is decoupled
            // from things like "output handling".

            // frequency control must be handled before output handling, otherwise
            // output handling might introduce undesirable jitter to frequency control
            Frequency_Control::process_1_Hz_tick(decoded_time);

            if (output_handler) {
                Clock::time_t time;

                time.second                    = BCD::int_to_bcd(decoded_time.second);
                time.minute                    = decoded_time.minute;
                time.hour                      = decoded_time.hour;
                time.weekday                   = decoded_time.weekday;
                time.day                       = decoded_time.day;
                time.month                     = decoded_time.month;
                time.year                      = decoded_time.year;
                time.uses_summertime           = decoded_time.uses_summertime;
                time.leap_second_scheduled     = decoded_time.leap_second_scheduled;
                time.timezone_change_scheduled = decoded_time.timezone_change_scheduled;
                output_handler(time);
            }

            if (decoded_time.second == 15 && Local_Clock.clock_state != Clock::useless
                                          && Local_Clock.clock_state != Clock::dirty
            ) {
                Second_Decoder.set_convolution_time(decoded_time);
            }
        }

        // called by the 1 kHz generator after handling the input provider
        // the idea is that the input provider and the 1 kHz generator
        // both basically belong to "the hardware". Thus the clock
        // controller will not care to much about them.
        static void process_1_kHz_tick_data(const uint8_t sampled_data) {
            Demodulator.detector(sampled_data);
            Local_Clock.process_1_kHz_tick();
            Frequency_Control::process_1_kHz_tick();
        }

        // This is the callback of the Demodulator stage. The clock controller
        // assumes that this is called more or less once per second by the demodulator.
        // However it is understood that this may jitter depending on the signal quality.
#ifndef MSF60
        static void process_single_tick_data(const DCF77::tick_t tick_data) {
            using namespace DCF77;

            DCF77_Encoder now;
            set_DCF77_Encoder(now);
            now.second += leap_second;

            now.advance_second();

            // leap_second == 2 indicates that we are in the 2nd second of the leap second handling
            leap_second <<= 1;
            // leap_second == 1 indicates that we now process second 59 but not the expected sync mark
            leap_second += (now.second == 59 && now.get_current_signal() != sync_mark);

            if (leap_second != 1) {
                Second_Decoder.binning(tick_data);

                if (now.second == 0) {
                    Minute_Decoder.advance_tick();
                    if (now.minute.val == 0x00) {

                        // "while" takes automatically care of timezone change
                        while (Hour_Decoder.get_time_value().val <= 0x23 &&
                            Hour_Decoder.get_time_value().val != now.hour.val) {
                            Hour_Decoder.advance_tick();
                        }

                        if (now.hour.val == 0x00) {
                            if (Weekday_Decoder.get_time_value().val <= 0x07) {
                                Weekday_Decoder.advance_tick();
                            }

                            // "while" takes automatically care of different month lengths
                            while (Day_Decoder.get_time_value().val <= 0x31 &&
                                Day_Decoder.get_time_value().val != now.day.val) {
                                Day_Decoder.advance_tick();
                            }

                            if (now.day.val == 0x01) {
                                if (Month_Decoder.get_time_value().val <= 0x12) {
                                    Month_Decoder.advance_tick();
                                }
                                if (now.month.val == 0x01) {
                                    if (now.year.val <= 0x99) { Year_Decoder.advance_tick(); }
                                }
                            }
                        }
                    }
                }

                const uint8_t tick_value = (tick_data == long_tick || tick_data == undefined)? 1: 0;
                Flag_Decoder.process_tick(now.second, tick_value);
                Minute_Decoder.process_tick(now.second, tick_value);
                Hour_Decoder.process_tick(now.second, tick_value);
                Weekday_Decoder.process_tick(now.second, tick_value);
                Day_Decoder.process_tick(now.second, tick_value);
                Month_Decoder.process_tick(now.second, tick_value);
                Year_Decoder.process_tick(now.second, tick_value);
            }
        }
#else
        static void process_single_tick_data(const DCF77::tick_t tick_data) {
            using namespace DCF77;

            DCF77_Encoder now;
            set_DCF77_Encoder(now);

            now.advance_second();

            Second_Decoder.binning(tick_data);

            if (now.second == 0) {
                Minute_Decoder.advance_tick();
                if (now.minute.val == 0x00) {

                    // "while" takes automatically care of timezone change
                    while (Hour_Decoder.get_time_value().val <= 0x23 &&
                           Hour_Decoder.get_time_value().val != now.hour.val) {
                        Hour_Decoder.advance_tick();
                    }

                    if (now.hour.val == 0x00) {
                        if (Weekday_Decoder.get_time_value().val <= 0x07) {
                            Weekday_Decoder.advance_tick();
                        }

                        // "while" takes automatically care of different month lengths
                        while (Day_Decoder.get_time_value().val <= 0x31 &&
                               Day_Decoder.get_time_value().val != now.day.val) {
                            Day_Decoder.advance_tick();
                        }

                        if (now.day.val == 0x01) {
                            if (Month_Decoder.get_time_value().val <= 0x12) {
                                Month_Decoder.advance_tick();
                            }
                            if (now.month.val == 0x01) {
                                if (now.year.val <= 0x99) { Year_Decoder.advance_tick(); }
                            }
                        }
                    }
                }
            }

            const bool tick_value_A = (tick_data == A1_B0 || tick_data == A1_B1 || tick_data == undefined) ? 1 : 0;
            const bool tick_value_B = (tick_data == A0_B1 || tick_data == A1_B1 || tick_data == undefined) ? 1 : 0;

            Flag_Decoder.process_tick(now.second, tick_value_B);
            Minute_Decoder.process_tick(now.second, tick_value_A);
            Hour_Decoder.process_tick(now.second, tick_value_A);
            Weekday_Decoder.process_tick(now.second, tick_value_A);
            Day_Decoder.process_tick(now.second, tick_value_A);
            Month_Decoder.process_tick(now.second, tick_value_A);
            Year_Decoder.process_tick(now.second, tick_value_A);

        }
#endif

        typedef Binning::lock_quality_tt<uint8_t> lock_quality_t;
        typedef struct {
            Clock::clock_state_t clock_state;
            uint8_t prediction_match;

            Binning::lock_quality_tt<uint32_t> phase;

            lock_quality_t second;
            lock_quality_t minute;
            lock_quality_t hour;
            lock_quality_t weekday;
            lock_quality_t day;
            lock_quality_t month;
            lock_quality_t year;

            uint8_t uses_summertime_quality;
            uint8_t timezone_change_scheduled_quality;
            uint8_t leap_second_scheduled_quality;
        } clock_quality_t;

        static void get_quality(clock_quality_t &clock_quality) {
            Demodulator.get_quality(clock_quality.phase);
            Second_Decoder.get_quality(clock_quality.second);
            Minute_Decoder.get_quality(clock_quality.minute);
            Hour_Decoder.get_quality(clock_quality.hour);
            Day_Decoder.get_quality(clock_quality.day);
            Weekday_Decoder.get_quality(clock_quality.weekday);
            Month_Decoder.get_quality(clock_quality.month);
            Year_Decoder.get_quality(clock_quality.year);

            Flag_Decoder.get_quality(clock_quality.uses_summertime_quality,
                                     clock_quality.timezone_change_scheduled_quality,
                                     clock_quality.leap_second_scheduled_quality);
        }

        typedef struct {
            uint8_t phase;
            uint8_t second;
            uint8_t minute;
            uint8_t hour;
            uint8_t weekday;
            uint8_t day;
            uint8_t month;
            uint8_t year;
        } clock_quality_factor_t;

        static void get_quality_factor(clock_quality_factor_t &clock_quality_factor) {
            clock_quality_factor.phase   = Demodulator.get_quality_factor();
            clock_quality_factor.second  = Second_Decoder.get_quality_factor();
            clock_quality_factor.minute  = Minute_Decoder.get_quality_factor();
            clock_quality_factor.hour    = Hour_Decoder.get_quality_factor();
            clock_quality_factor.day     = Day_Decoder.get_quality_factor();
            clock_quality_factor.weekday = Weekday_Decoder.get_quality_factor();
            clock_quality_factor.month   = Month_Decoder.get_quality_factor();
            clock_quality_factor.year    = Year_Decoder.get_quality_factor();
        }

        static uint8_t get_overall_quality_factor() {
            using namespace Arithmetic_Tools;

            uint8_t quality_factor = Demodulator.get_quality_factor();
            minimize(quality_factor, Second_Decoder.get_quality_factor());
            minimize(quality_factor, Minute_Decoder.get_quality_factor());
            minimize(quality_factor, Hour_Decoder.get_quality_factor());

            uint8_t date_quality_factor = Day_Decoder.get_quality_factor();
            minimize(date_quality_factor, Month_Decoder.get_quality_factor());
            minimize(date_quality_factor, Year_Decoder.get_quality_factor());

            const uint8_t weekday_quality_factor = Weekday_Decoder.get_quality_factor();
            if (date_quality_factor > 0 && weekday_quality_factor > 0) {

                DCF77_Encoder now;
                now.second  = Second_Decoder.get_time_value();
                now.minute  = Minute_Decoder.get_time_value();
                now.hour    = Hour_Decoder.get_time_value();
                now.weekday = Weekday_Decoder.get_time_value();
                now.day     = Day_Decoder.get_time_value();
                now.month   = Month_Decoder.get_time_value();
                now.year    = Year_Decoder.get_time_value();

                BCD::bcd_t weekday = now.get_bcd_weekday();
                if (weekday.val == 0) {
                    weekday.val = 7;
                }
                if (now.weekday.val == weekday.val) {
                    date_quality_factor += 1;
                } else if (date_quality_factor <= weekday_quality_factor) {
                    date_quality_factor = 0;
                }
            }

            minimize(quality_factor, date_quality_factor);
            return quality_factor;
        };

        static Clock::clock_state_t get_clock_state() {
            return Local_Clock.get_state();
        }

        static uint8_t get_prediction_match() {
            return Second_Decoder.get_prediction_match();
        }

        static void on_tuned_clock() {
            Demodulator.set_has_tuned_clock();
            Local_Clock.set_has_tuned_clock();
        };

        static void phase_lost_event_handler() {
            // do not reset frequency control as a reset would also reset
            // the current value for the measurement period length
            Second_Decoder.setup();
            Minute_Decoder.setup();
            Hour_Decoder.setup();
            Day_Decoder.setup();
            Weekday_Decoder.setup();
            Month_Decoder.setup();
            Year_Decoder.setup();
        }

        static void sync_achieved_event_handler() {
            // It can be argued if phase events instead of sync events
            // should be used. In theory it would be sufficient to have a
            // reasonable phase at the start and end of a calibration measurement
            // interval.
            // On the other hand a clean signal will provide a better calibration.
            // Since it is sufficient if the calibration happens only once in a
            // while we are satisfied with hooking at the sync events.
            Frequency_Control::qualify_calibration();
        }

        static void sync_lost_event_handler() {
            Frequency_Control::unqualify_calibration();

            bool reset_successors = (Demodulator.get_quality_factor() == 0);
            if (reset_successors) {
                Second_Decoder.setup();
            }

            reset_successors |= (Second_Decoder.get_quality_factor() == 0);
            if (reset_successors) {
                Minute_Decoder.setup();
            }

            reset_successors |= (Minute_Decoder.get_quality_factor() == 0);
            if (reset_successors) {
                Hour_Decoder.setup();
            }

            reset_successors |= (Hour_Decoder.get_quality_factor() == 0);
            if (reset_successors) {
                Weekday_Decoder.setup();
                Day_Decoder.setup();
            }

            reset_successors |= (Day_Decoder.get_quality_factor() == 0);
            if (reset_successors) {
                Month_Decoder.setup();
            }

            reset_successors |= (Month_Decoder.get_quality_factor() == 0);
            if (reset_successors) {
                Year_Decoder.setup();
            }
        }

        static DCF77_Demodulator<DCF77_Clock_Controller> Demodulator;
        static uint8_t get_demodulator_quality_factor() {
            return Demodulator.get_quality_factor();
        }

        static DCF77_Local_Clock<DCF77_Clock_Controller> Local_Clock;

        // non-blocking, reads current second
        static void read_current_time(DCF77_Encoder &now) {
            Local_Clock.read_current_time(now);
        }

        #if defined(_AVR_EEPROM_H_)
        // this is slow and messes with the interrupt flag, do not call during interrupt handling
        static void auto_persist() {
            Frequency_Control::auto_persist();
        }
        #endif

        static void setup() {
            Demodulator.setup();
            phase_lost_event_handler();
            Frequency_Control::setup();
        }

        static void debug() {
            DCF77_Encoder now;
            now.second  = Second_Decoder.get_time_value();
            now.minute  = Minute_Decoder.get_time_value();
            now.hour    = Hour_Decoder.get_time_value();
            now.weekday = Weekday_Decoder.get_time_value();
            now.day     = Day_Decoder.get_time_value();
            now.month   = Month_Decoder.get_time_value();
            now.year    = Year_Decoder.get_time_value();
            now.debug();

            clock_quality_t clock_quality;
            get_quality(clock_quality);

            clock_quality_factor_t clock_quality_factor;
            get_quality_factor(clock_quality_factor);

            sprint(get_overall_quality_factor(), DEC);
            sprint(F(" p("));
            sprint(clock_quality.phase.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.phase.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.phase, DEC);

            sprint(F(") s("));

            sprint(clock_quality.second.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.second.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.second, DEC);

            sprint(F(") m("));

            sprint(clock_quality.minute.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.minute.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.minute, DEC);

            sprint(F(") h("));

            sprint(clock_quality.hour.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.hour.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.hour, DEC);

            sprint(F(") wd("));

            sprint(clock_quality.weekday.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.weekday.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.weekday, DEC);

            sprint(F(") D("));

            sprint(clock_quality.day.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.day.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.day, DEC);

            sprint(F(") M("));

            sprint(clock_quality.month.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.month.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.month, DEC);

            sprint(F(") Y("));

            sprint(clock_quality.year.lock_max, DEC);
            sprint('-');
            sprint(clock_quality.year.noise_max, DEC);
            sprint(':');
            sprint(clock_quality_factor.year, DEC);
            sprint(F(") "));

            sprint(clock_quality.uses_summertime_quality, DEC);
            sprint(',');
            sprint(clock_quality.timezone_change_scheduled_quality, DEC);
            sprint(',');
            sprint(clock_quality.leap_second_scheduled_quality, DEC);

            sprint(',');
            sprintln(get_prediction_match(), DEC);
        }
    };

    namespace Generic_1_kHz_Generator {
        // This is the only remaining dependency to the DCF77 clock.
        // The implementation of the generator is otherwise completely generic.
        typedef DCF77_Clock_Controller<DCF77_Frequency_Control> Clock_Controller;

        void setup(const Clock::input_provider_t input_provider);
        uint8_t zero_provider();
        // positive_value --> increase frequency
        // pp16m = parts per 16 million = 1 Hz @ 16 Mhz
        void adjust(const int16_t pp16m);
        int16_t read_adjustment();
        void isr_handler();
    }
}
#endif
