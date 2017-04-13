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


#if defined(__AVR__)
    #error
    #error "You have set the target platform to Atmel AVR"
    #error "(e.g. Arduino Uno). However the Atmel controllers"
    #error "lack sufficient memory to deal with this test code."
    #error "In order to run the testys you need a target with"
    #error "more Memory (e.g. Arduino Due)."
    #error
    #error "This does not imply that the library is not"
    #error "good for Atmel AVR. It only implies that the"
    #error "test code is to big for AVR. If the tests pass"
    #error "for any platform then most of the code for the other"
    #error "platforms is also tested."
    #error
    #error "So either get a suitable target to run the tests"
    #error "or believe me that I did run them."
    #error
#endif

#include <dcf77.h>

const bool stop_on_first_error = false;
const bool verbose = false;

namespace ut {
    // templates to have a print function with multiple arguments which will be
    // output in a comma separated fashion
    template <typename T>
    void print(T v) {
        Serial.print(v);
    }

    template <typename T, typename... Args>
    void print(T first, Args... args) {
        print(first);
        print(F(", "));
        print(args...);

    }

    // required for the F macro
    typedef const __FlashStringHelper* pstr;

    void handle_error(bool ok) {
        if (!ok) {
            while (stop_on_first_error);
        }
    };

    uint32_t passed = 0;
    uint32_t failed = 0;

    void assert_internal(uint32_t line, pstr what, bool ok) {
        if (ok) {
            ++passed;
            if (verbose) {
                Serial.print(F("OK: "));
                Serial.print(line);
                Serial.print(F(": "));
                Serial.println(what);
            }
        } else {
            ++failed;
            Serial.print(F("failed: "));
            Serial.print(line);
            Serial.print(F(": "));
            Serial.println(what);
        }
    }

    void assert(uint32_t line, pstr what, bool ok) {
         assert_internal(line, what, ok);
         handle_error(ok);
    }

    // templates to implement the actual validation
    template <typename... Args>
    void assert(uint32_t line, pstr what, bool ok, Args... args) {
         assert_internal(line, what, ok);
         if (!ok) {
            Serial.print(F(" ["));
            print(args...);
            Serial.println(']');
            Serial.println();
         }
         handle_error(ok);
    }

    // we want to have the line number automatically inserted
    // notice that macros are visible outside the namespace since
    // they are processed by the preprocessor
    #define assert(...) ut::assert (__LINE__, __VA_ARGS__)

    void print_statistics() {
        Serial.println();
        Serial.print(F("passed: "));
        Serial.println(passed);
        Serial.print(F("failed: "));
        Serial.println(failed);
        Serial.println();
    }
}

namespace {
    void todo(uint32_t line) {
        Serial.print(line);
        Serial.println(F(": TODO"));
    }

    #define todo(...) todo(__LINE__)
}

void test_BCD() {
    using namespace BCD;

    {  // implicit conversion due to union structure
        bool ok = true;

        bcd_t a;
        a.val = 0;
        uint8_t i = 0;
        while (i < 100) {  // 101 because we need to test the wrap around as well
            a.digit.lo = i % 10;
            a.digit.hi = i / 10;
            if (a.val != a.digit.lo + 16*a.digit.hi) {
                ok = false;
                break;
            }
            ++i;
        }
        assert(F("properly converts lo/hi and val"), ok, i, a.val);
    }

    {  // implicit conversion due to union structure 2
        bool ok = true;

        bcd_t a;
        a.val = 0;
        uint8_t i = 0;
        uint8_t b;
        while (i < 100) {  // 101 because we need to test the wrap around as well
            a.digit.lo = i % 10;
            a.digit.hi = i / 10;
            b = a.bit.b0 + 2*(
                a.bit.b1 + 2*(
                a.bit.b2 + 2*(
                a.bit.b3 + 2*(
                a.bit.b4 + 2*(
                a.bit.b5 + 2*(
                a.bit.b6 + 2*(
                a.bit.b7 )))))));
            if (a.val != b) {
                ok = false;
                break;
            }
            ++i;
        }
        assert(F("properly converts bits and val"), ok, i, a.val, b);
    }


    {  // increment
        bool ok = true;

        bcd_t a;
        a.val = 0;
        uint8_t i = 0;
        while (i < 101) {  // 101 because we need to test the wrap around as well
            if (a.digit.lo + 10 * a.digit.hi != i % 100) {
                ok = false;
                break;
            }
            ++i;
            increment(a);
        }
        assert(F("properly increments values"), ok, i, a.val);
    }

    {  // int_to_bcd
        bool ok = true;

        uint8_t i = 0;
        bcd_t a;
        while (i < 100) {
            a = int_to_bcd(i);
            if ((a.val & 0xf) + 10 * (a.val >> 4) != i) {
                ok = false;
                break;
            }
            ++i;
            increment(a);
        }
        assert(F("properly converts int to bcd"), ok, i, a.val);
    }

    {  // bcd_to_int
        bool ok = true;

        uint8_t i = 0;
        uint8_t j = 0;
        bcd_t a;
        while (i < 100) {
            a = int_to_bcd(i);
            j = bcd_to_int(a);
            if (j != i) {
                ok = false;
                break;
            }
            ++i;
            increment(a);
        }
        assert(F("properly converts bcd_to_int"), ok, i, a.val, j);
    }

    {  // ==
        bool ok = true;

        bcd_t a;
        bcd_t b;
        uint8_t i;
        uint8_t j;
        a.val = 0;
        for (i = 0; i < 100; ++i) {
            b.val = 0;
            for (j = 0; j < 100; ++j) {
                if ((a == b) != (a.val == b.val)) {
                    ok = false;
                    break;
                }
                increment(b);
            }
            increment(a);
        }
        assert(F("properly compares == bcd values"), ok, a.val, b.val);
    }

    {  // !=
        bool ok = true;

        bcd_t a;
        bcd_t b;
        uint8_t i;
        uint8_t j;
        a.val = 0;
        for (i = 0; i < 100; ++i) {
            b.val = 0;
            for (j = 0; j < 100; ++j) {
                if ((a != b) != (a.val != b.val)) {
                    ok = false;
                    break;
                }
                increment(b);
            }
            increment(a);
        }
        assert(F("properly compares != bcd values"), ok, a.val, b.val);
    }

    {  // >=
        bool ok = true;

        bcd_t a;
        bcd_t b;
        uint8_t i;
        uint8_t j;
        a.val = 0;
        for (i = 0; i < 100; ++i) {
            b.val = 0;
            for (j = 0; j < 100; ++j) {
                if ((a >= b) != (a.val >= b.val)) {
                    ok = false;
                    break;
                }
                increment(b);
            }
            increment(a);
        }
        assert(F("properly compares >= bcd values"), ok, a.val, b.val);
    }


    {  // <=
    bool ok = true;

        bcd_t a;
        bcd_t b;
        uint8_t i;
        uint8_t j;
        a.val = 0;
        for (i = 0; i < 100; ++i) {
            b.val = 0;
            for (j = 0; j < 100; ++j) {
                if ((a <= b) != (a.val <= b.val)) {
                    ok = false;
                    break;
                }
                increment(b);
            }
            increment(a);
        }
        assert(F("properly compares <= bcd values"), ok, a.val, b.val);
    }

    {  // >
    bool ok = true;

        bcd_t a;
        bcd_t b;
        uint8_t i;
        uint8_t j;
        a.val = 0;
        for (i = 0; i < 100; ++i) {
            b.val = 0;
            for (j = 0; j < 100; ++j) {
                if ((a > b) != (a.val > b.val)) {
                    ok = false;
                    break;
                }
                increment(b);
            }
            increment(a);
        }
        assert(F("properly compares > bcd values"), ok, a.val, b.val);
    }

    {  // <
        bool ok = true;

        bcd_t a;
        bcd_t b;
        uint8_t i;
        uint8_t j;
        a.val = 0;
        for (i = 0; i < 100; ++i) {
            b.val = 0;
            for (j = 0; j < 100; ++j) {
                if ((a < b) != (a.val < b.val)) {
                    ok = false;
                    break;
                }
                increment(b);
            }
            increment(a);
        }
        assert(F("properly compares < bcd values"), ok, a.val, b.val);
    }
}

void test_TMP() {
    using namespace Internal;

    { // equal<>
        assert(F("equal types"), TMP::equal<uint8_t, uint8_t>::val == true);
        assert(F("unequal types"), TMP::equal<uint8_t, uint16_t>::val == false);
        typedef uint8_t t1;
        typedef uint8_t t2;
        typedef int8_t t3;
        assert(F("equal aliased types"), TMP::equal<t1, t2>::val == true);
        assert(F("unequal aliased types"), TMP::equal<t1, t3>::val == false);
    }


    { // uint_t<>
        assert(F(" 8 bits maps to uint8_t"),  TMP::equal<TMP::uint_t< 8>::type, uint8_t>::val  == true);
        assert(F("16 bits maps to uint16_t"), TMP::equal<TMP::uint_t<16>::type, uint16_t>::val == true);
        assert(F("32 bits maps to uint32_t"), TMP::equal<TMP::uint_t<32>::type, uint32_t>::val == true);
    }


    { // limits<>
        assert(F("uint8_t.min  == 0x00"),       TMP::limits<uint8_t>::min  == 0x00);
        assert(F("uint16_t.min == 0x00"),       TMP::limits<uint16_t>::min == 0x00);
        assert(F("uint32_t.min == 0x00"),       TMP::limits<uint32_t>::min == 0x00);
        assert(F("uint8_t.max  == 0xff"),       TMP::limits<uint8_t>::max  == 0xff);
        assert(F("uint16_t.max == 0xffff"),     TMP::limits<uint16_t>::max == 0xffff);
        assert(F("uint32_t.max == 0xffffffff"), TMP::limits<uint32_t>::max == 0xffffffff);
    }

    { // if_t<>
        assert(F("if_t<true>"),  TMP::equal<TMP::if_t<true,  uint8_t, uint16_t>::type,  uint8_t>::val == true);
        assert(F("if_t<false>"), TMP::equal<TMP::if_t<false, uint8_t, uint16_t>::type, uint16_t>::val == true);
    }
}

void test_Arithmetic_Tools() {
    using namespace Internal::Arithmetic_Tools;
    {
        // bounded_increment
        uint8_t i = 0;
        bounded_increment<0>(i);
        assert(F("incrementing by 0 does not change the value"),i == 0, 0, i);

        i = 0;
        bounded_increment<1>(i);
        assert(F("incrementing 0 by 1 delivers 1"),i == 1, 1, i);

        i = 0;
        bounded_increment<2>(i);
        assert(F("incrementing 0 by 2 delivers 2"),i == 2, 2, i);

        i = 255;
        bounded_increment<1>(i);
        assert(F("incrementing 255 by 1 delivers 255"),i == 255, 255, i);

        i = 254;
        bounded_increment<2>(i);
        assert(F("incrementing 254 by 2 delivers 255"),i == 255, 255, i);

        i = 2;
        bounded_increment<254>(i);
        assert(F("incrementing 2 by 254 delivers 255"),i == 255, 255, i);

        i = 1;
        bounded_increment<254>(i);
        assert(F("incrementing 1 by 254 delivers 255"),i == 255, 255, i);

        i = 1;
        bounded_increment<253>(i);
        assert(F("incrementing 1 by 253 delivers 254"),i == 254, 254, i);
    }

    {
        // bounded_decrement
        uint8_t i = 255;
        bounded_decrement<0>(i);
        assert(F("decrementing 255 by 0 does not change the value"),i == 255, 255, i);

        i = 255;
        bounded_decrement<1>(i);
        assert(F("decrementing 255 by 1 delivers 254"),i == 254, 254, i);

        i = 255;
        bounded_decrement<2>(i);
        assert(F("decrementing 252 by 1 delivers 253"),i == 253, 253, i);

        i = 0;
        bounded_decrement<1>(i);
        assert(F("decrementing 0 by 1 delivers 0"),i == 0, 0, i);

        i = 1;
        bounded_decrement<2>(i);
        assert(F("decrementing 1 by 2 delivers 0"),i == 0, 0, i);

        i = 254;
        bounded_decrement<255>(i);
        assert(F("decrementing 254 by 255 delivers 0"),i == 0, 0, i);

        i = 254;
        bounded_decrement<254>(i);
        assert(F("decrementing 254 by 254 delivers 0"),i == 0, 0, i);

        i = 254;
        bounded_decrement<253>(i);
        assert(F("decrementing 254 by 253 delivers 1"),i == 1, 1, i);
    }

    {
        // minimize
        {
            // uint8_t
            uint8_t i = 1;
            uint8_t j = 0;
            minimize(i, j);
            assert(F("min (1,0) == 0"), i == 0, 0, i);

            i = 0;
            j = 1;
            minimize(i, j);
            assert(F("min (0,1) == 0"), i == 0, 0, i);

            i = 1;
            minimize(i, i);
            assert(F("min (i,i) == i"), i == 1, 1, i);

            i = 1;
            j = 1;
            minimize(i, j);
            assert(F("min (1,1) == 1"), i == 1, 1, i);
        }

        {
            // uint16_t
            uint16_t i = 1;
            uint16_t j = 0;
            minimize(i, j);
            assert(F("min (1,0) == 0"), i == 0, 0, i);

            i = 0;
            j = 1;
            minimize(i, j);
            assert(F("min (0,1) == 0"), i == 0, 0, i);

            i = 1;
            minimize(i, i);
            assert(F("min (i,i) == i"), i == 1, 1, i);

            i = 1;
            j = 1;
            minimize(i, j);
            assert(F("min (1,1) == 1"), i == 1, 1, i);
        }

        {
            // int8_t
            int8_t i = 1;
            int8_t j = -1;
            minimize(i, j);
            assert(F("min (1, -1) == -1"), i == -1, -1, i);

            i = -1;
            j = 1;
            minimize(i, j);
            assert(F("min (-1,1) == -1"), i == -1, -1, i);

            i = -1;
            minimize(i, i);
            assert(F("min (i,i) == i"), i == -1, -1, i);

            i = 1;
            j = 1;
            minimize(i, j);
            assert(F("min (1,1) == 1"), i == 1, 1, i);
        }

        {
            // int16_t
            int16_t i = 1;
            int16_t j = -1;
            minimize(i, j);
            assert(F("min (1, -1) == -1"), i == -1, -1, i);

            i = -1;
            j = 1;
            minimize(i, j);
            assert(F("min (-1,1) == -1"), i == -1, -1, i);

            i = -1;
            minimize(i, i);
            assert(F("min (i,i) == i"), i == -1, -1, i);

            i = 1;
            j = 1;
            minimize(i, j);
            assert(F("min (1,1) == 1"), i == 1, 1, i);
        }
    }

    {
        // maximize
        {
            // uint8_t
            uint8_t i = 1;
            uint8_t j = 0;
            maximize(i, j);
            assert(F("max (1,0) == 1"), i == 1, 1, i);

            i = 0;
            j = 1;
            maximize(i, j);
            assert(F("max (0,1) == 1"), i == 1, 1, i);

            i = 1;
            maximize(i, i);
            assert(F("max (i,i) == i"), i == 1, 1, i);

            i = 1;
            j = 1;
            maximize(i, j);
            assert(F("max (1,1) == 1"), i == 1, 1, i);
        }

        {
            // uint16_t
            uint16_t i = 1;
            uint16_t j = 0;
            maximize(i, j);
            assert(F("max (1,0) == 1"), i == 1, 1, i);

            i = 0;
            j = 1;
            maximize(i, j);
            assert(F("max (0,1) == 1"), i == 1, 1, i);

            i = 1;
            maximize(i, i);
            assert(F("max (i,i) == i"), i == 1, 1, i);

            i = 1;
            j = 1;
            maximize(i, j);
            assert(F("max (1,1) == 1"), i == 1, 1, i);
        }

        {
            // int8_t
            int8_t i = 1;
            int8_t j = -1;
            maximize(i, j);
            assert(F("max (1, -1) == 1"), i == 1, 1, i);

            i = -1;
            j = 1;
            maximize(i, j);
            assert(F("max (-1,1) == 1"), i == 1, 1, i);

            i = -1;
            maximize(i, i);
            assert(F("max (i,i) == i"), i == -1, -1, i);

            i = 1;
            j = 1;
            maximize(i, j);
            assert(F("max (1,1) == 1"), i == 1, 1, i);
        }

        {
            // int16_t
            int16_t i = 1;
            int16_t j = -1;
            maximize(i, j);
            assert(F("max (1, -1) == 1"), i == 1, 1, i);

            i = -1;
            j = 1;
            maximize(i, j);
            assert(F("max (-1,1) == 1"), i == 1, 1, i);

            i = -1;
            maximize(i, i);
            assert(F("max (i,i) == i"), i == -1, -1, i);

            i = 1;
            j = 1;
            maximize(i, j);
            assert(F("max (1,1) == 1"), i == 1, 1, i);
        }
    }

    {
        // bounded_add
        int16_t a = 0;
        uint8_t b = 0;
        uint8_t c = 0;
        int16_t d = 0;
        bool ok = true;
        for (a=0; a<255; ++a) {
            for (b=0; b<255; ++b) {
                c = a;
                bounded_add(c,b);
                d = a+b;
                if (d>255) { d=255; }
                if (c != d) {
                    ok = false;
                    break;
                }
            }
        }
        assert(F("bounded_add"), ok, a, b, c, d);
    }

    {
        // bounded_sub
        int16_t a = 0;
        uint8_t b = 0;
        uint8_t c = 0;
        int16_t d = 0;
        bool ok = true;
        for (a=0; a<255; ++a) {
            for (b=0; b<255; ++b) {
                c = a;
                bounded_sub(c,b);
                d = a-b;
                if (d<0) { d=0; }
                if (c != d) {
                    ok = false;
                    break;
                }
            }
        }
        assert(F("bounded_sub"), ok, a, b, c, d);
    }

    {   // bit_count
        bool ok = true;
        uint8_t i;
        uint8_t c;
        // bit_count
        for (i=0; i < 255; ++i) {
            c=0;
            for (uint8_t b=0; b < 8; ++b) {
                c += (i >> b) & 1;
            }
            if  (c != bit_count(i)) {
                ok = false;
                break;
            }
        }
        assert(F("bitcount"), ok, i, c);
    }

    {   // parity
        bool ok = true;
        uint8_t i;
        uint8_t c;
        // bit_count
        for (i=0; i < 255; ++i) {
            c=0;
            for (uint8_t b=0; b < 8; ++b) {
                c += (i >> b) & 1;
                c &= 1;
            }
            if  (c != parity(i)) {
                ok = false;
                break;
            }
        }
        assert(F("parity"), ok, i, c);
    }

    {
        // set_bit
        bool ok = true;
        uint8_t i;
        uint8_t c;
        // bit_count
        for (i=0; i < 8; ++i) {
            c = set_bit(0, i, 0);
            if  (c != 0) {
                ok = false;
                break;
            }

            c = set_bit(0xff, i, 0);
            if  (c != (0xff ^ (1 << i))) {
                ok = false;
                break;
            }
        }
        assert(F("set_bit to 0"), ok, i, c);

        ok = true;
        for (i=0; i < 8; ++i) {
            c = set_bit(0, i, 1);
            if  (c != (1<<i)) {
                ok = false;
                break;
            }

            c = set_bit(0xff, i, 1);
            if  (c != 0xff) {
                ok = false;
                break;
            }
        }
        assert(F("set_bit to 1"), ok, i, c);
    }
}


uint64_t mirror_bits(uint16_t bits, uint8_t len) {
    uint16_t result = 0;

    for (uint8_t i=0; i<len; ++i) {
        result <<= 1;
        result += bits & 1;
        bits >>= 1;
    }
    return result;
}

void assert_encoder_stream(Internal::DCF77_Encoder & encoder,
                             uint8_t  info,
                             uint8_t  minute_p,
                             uint8_t  hour_p,
                             uint8_t  day,
                             uint8_t  weekday,
                             uint8_t  month,
                             uint16_t year_p) {
    using namespace Internal;

    uint64_t bits;

    DCF77::tick_t tick;
    uint8_t expected_tick;
    bool ok = true;
    uint8_t second = 0;

    // minute marker
    tick = encoder.get_current_signal();
    expected_tick = DCF77::short_tick;
    ok = ok && (tick == expected_tick);
    encoder.advance_second();

    // weather data always undefined
    second = 1;
    while(ok && second < 15) {
        tick = encoder.get_current_signal();
        expected_tick = DCF77::undefined;
        ok = ok && (tick == expected_tick);

        ++second;
        encoder.advance_second();
    }

    bits = (mirror_bits(info,     6) << 15) |
           (mirror_bits(minute_p, 8) << 21) |
           (mirror_bits(hour_p,   7) << 29) |
           (mirror_bits(day,      6) << 36) |
           (mirror_bits(weekday,  3) << 42) |
           (mirror_bits(month,    5) << 45) |
           (mirror_bits(year_p,   9) << 50);

    if (verbose) {
        Serial.print(F("expected bit stream:  "));
        for (uint8_t i=0; i<60; ++i) {
            if (i == 15 || i == 21 || i == 29 || i == 36 || i == 42 || i == 45 || i == 50 || i == 59) { Serial.print(' '); }
            Serial.print((uint8_t) (bits >> i ) & 1);
        }
        Serial.println();

        DCF77_Encoder enc2 = encoder;
        Serial.print(F("generated bit stream: "));
        enc2.second = 0;
        for (uint8_t i=0; i<60; ++i) {
            if (i == 15 || i == 21 || i == 29 || i == 36 || i == 42 || i == 45 || i == 50 || i == 59) { Serial.print(' '); }
            Serial.print((uint8_t) enc2.get_current_signal() & 1);
            enc2.advance_second();
        }
        Serial.println();
    }

    while (ok && second < 59) {
        tick = encoder.get_current_signal();
        expected_tick = (DCF77::short_tick + ((bits >> second) & 1));
        ok = ok && (tick == expected_tick);
        ++second;
        encoder.advance_second();
    }

    if (encoder.year.val == 9 &&
        encoder.month.val == 1 &&
        encoder.day.val == 1 &&
        encoder.hour.val == 1 &&
        encoder.minute.val == 0) {
        // leap second 2009 01 01 01:00
        tick = encoder.get_current_signal();
        expected_tick = DCF77::short_tick;
        ok = ok && (tick == expected_tick);
        ++second;
        encoder.advance_second();
    }


    // sync mark
    if (ok) {
        tick = encoder.get_current_signal();
        expected_tick = DCF77::sync_mark;
        ok = ok && (tick == expected_tick);
    }

    assert(F("encoder output matches DCF77 log"), ok,
             'Y', encoder.year.val,
             'M', encoder.month.val,
             'D', encoder.day.val,
             'h', encoder.hour.val,
             'm', encoder.minute.val,
             's', encoder.second,
             second,
             tick,
             expected_tick
            );

    if (ok) {
        encoder.advance_second();
    } else {
        encoder.second = 0;
        encoder.advance_minute();
    }
}


void test_signal_year_change() {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x07;
    encoder.month.val = 0x12;
    encoder.day.val = 0x31;
    encoder.hour.val = 0x23;
    encoder.minute.val = 0x30;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.undefined_uses_summertime_output = false;
    encoder.undefined_minute_output = false;
    assert_encoder_stream(encoder, 0b000101, 0b00001100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:30:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10001101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:31:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01001101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:32:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11001100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:33:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00101101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:34:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10101100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:35:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01101100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:36:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11101101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:37:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00011101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:38:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10011100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:39:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00000011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:40:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10000010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:41:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:42:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:43:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:44:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:45:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01100011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:46:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11100010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:47:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00010010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:48:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10010011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:49:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00001010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:50:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10001011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:51:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01001011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:52:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11001010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:53:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00101011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:54:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10101010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:55:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01101010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:56:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11101011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:57:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00011011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:58:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10011010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:59:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00000000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:00:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10000001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:01:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:02:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:03:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:04:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:05:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01100000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:06:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11100001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:07:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00010001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:08:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10010000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:09:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00001001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:10:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10001000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:11:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01001000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:12:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11001001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:13:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00101000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:14:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10101001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:15:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01101001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:16:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11101000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:17:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00011000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:18:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10011001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:19:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00000101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:20:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10000100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:21:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:22:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:23:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:24:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:25:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01100101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:26:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11100100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:27:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00010100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:28:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10010101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:29:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00001100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:30:00, WZ
}

void test_signal_winter_summertime_change()  {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x09;
    encoder.month.val = 0x03;
    encoder.day.val = 0x29;
    encoder.hour.val = 0x00;
    encoder.minute.val = 0x00;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.undefined_uses_summertime_output = false;
    encoder.undefined_minute_output = false;
    assert_encoder_stream(encoder, 0b000101, 0b00000000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:00:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10000001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:01:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:02:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:03:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:04:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:05:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01100000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:06:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11100001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:07:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00010001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:08:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10010000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:09:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00001001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:10:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10001000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:11:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01001000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:12:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11001001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:13:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00101000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:14:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10101001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:15:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01101001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:16:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11101000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:17:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00011000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:18:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10011001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:19:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00000101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:20:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10000100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:21:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:22:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:23:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:24:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:25:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01100101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:26:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11100100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:27:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00010100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:28:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10010101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:29:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00001100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:30:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10001101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:31:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01001101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:32:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11001100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:33:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00101101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:34:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10101100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:35:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01101100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:36:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11101101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:37:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00011101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:38:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10011100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:39:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00000011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:40:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10000010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:41:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:42:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:43:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:44:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:45:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01100011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:46:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11100010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:47:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00010010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:48:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10010011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:49:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00001010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:50:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10001011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:51:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01001011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:52:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11001010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:53:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00101011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:54:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10101010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:55:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01101010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:56:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11101011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:57:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00011011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:58:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10011010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:59:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00000000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:00:00, WZ
    assert_encoder_stream(encoder, 0b010101, 0b10000001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:01:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01000001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:02:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11000000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:03:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00100001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:04:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10100000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:05:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01100000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:06:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11100001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:07:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00010001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:08:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10010000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:09:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00001001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:10:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10001000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:11:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01001000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:12:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11001001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:13:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00101000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:14:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10101001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:15:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01101001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:16:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11101000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:17:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00011000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:18:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10011001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:19:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00000101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:20:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10000100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:21:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01000100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:22:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11000101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:23:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00100100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:24:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10100101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:25:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01100101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:26:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11100100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:27:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00010100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:28:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10010101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:29:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00001100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:30:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10001101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:31:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01001101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:32:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11001100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:33:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00101101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:34:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10101100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:35:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01101100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:36:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11101101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:37:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00011101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:38:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10011100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:39:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00000011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:40:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10000010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:41:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01000010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:42:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11000011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:43:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00100010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:44:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10100011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:45:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01100011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:46:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11100010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:47:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00010010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:48:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10010011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:49:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00001010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:50:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10001011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:51:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01001011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:52:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11001010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:53:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00101011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:54:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10101010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:55:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b01101010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:56:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b11101011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:57:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00011011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:58:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b10011010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:59:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00000000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:00:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b001001, 0b10000001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:01:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01000001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:02:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11000000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:03:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00100001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:04:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10100000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:05:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01100000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:06:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11100001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:07:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00010001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:08:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10010000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:09:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00001001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:10:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10001000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:11:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01001000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:12:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11001001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:13:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00101000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:14:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10101001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:15:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01101001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:16:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11101000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:17:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00011000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:18:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10011001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:19:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00000101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:20:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10000100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:21:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01000100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:22:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11000101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:23:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00100100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:24:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10100101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:25:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01100101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:26:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11100100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:27:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00010100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:28:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10010101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:29:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00001100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:30:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10001101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:31:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01001101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:32:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11001100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:33:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00101101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:34:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10101100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:35:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01101100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:36:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11101101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:37:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00011101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:38:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10011100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:39:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00000011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:40:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10000010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:41:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01000010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:42:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11000011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:43:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00100010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:44:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10100011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:45:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01100011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:46:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11100010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:47:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00010010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:48:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10010011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:49:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00001010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:50:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10001011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:51:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01001011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:52:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11001010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:53:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00101011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:54:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10101010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:55:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01101010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:56:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11101011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:57:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00011011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:58:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10011010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:59:00, SZ
}


void test_signal_summer_wintertime_change() {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x09;
    encoder.month.val = 0x10;
    encoder.day.val = 0x25;
    encoder.hour.val = 0x01;
    encoder.minute.val = 0x55;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.uses_summertime = true;
    encoder.undefined_uses_summertime_output = false;
    encoder.undefined_minute_output = false;
    assert_encoder_stream(encoder, 0b001001, 0b10101010, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:55:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b01101010, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:56:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b11101011, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:57:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00011011, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:58:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b10011010, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:59:00, SZ
    assert_encoder_stream(encoder, 0b001001, 0b00000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:00:00, SZ
    assert_encoder_stream(encoder, 0b011001, 0b10000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:01:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:02:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:03:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00100001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:04:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10100000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:05:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01100000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:06:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11100001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:07:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00010001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:08:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10010000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:09:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00001001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:10:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10001000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:11:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01001000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:12:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11001001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:13:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00101000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:14:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10101001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:15:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01101001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:16:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11101000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:17:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00011000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:18:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10011001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:19:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00000101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:20:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10000100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:21:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01000100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:22:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11000101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:23:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00100100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:24:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10100101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:25:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01100101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:26:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11100100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:27:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00010100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:28:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10010101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:29:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00001100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:30:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10001101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:31:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01001101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:32:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11001100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:33:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00101101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:34:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10101100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:35:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01101100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:36:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11101101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:37:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00011101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:38:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10011100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:39:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00000011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:40:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10000010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:41:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01000010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:42:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11000011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:43:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00100010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:44:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10100011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:45:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01100011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:46:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11100010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:47:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00010010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:48:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10010011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:49:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00001010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:50:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10001011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:51:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01001011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:52:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11001010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:53:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00101011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:54:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10101010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:55:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b01101010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:56:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b11101011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:57:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b00011011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:58:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b011001, 0b10011010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:59:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b010101, 0b00000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:00:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_encoder_stream(encoder, 0b000101, 0b10000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:01:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:02:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:03:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:04:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:05:00, WZ
}

void test_signal_leap_second() {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x08;
    encoder.month.val = 0x12;
    encoder.day.val = 0x31;
    encoder.hour.val = 0x23;
    encoder.minute.val = 0x55;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.uses_summertime = true;
    encoder.undefined_uses_summertime_output = false;
    assert_encoder_stream(encoder, 0b000101, 0b10101010, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:55:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01101010, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:56:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11101011, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:57:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00011011, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:58:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10011010, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:59:00, WZ
    encoder.leap_second_scheduled = true; // this tests if encoder properly wipes the leap second flag "close to the edge"
    assert_encoder_stream(encoder, 0b000101, 0b00000000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:00:00, WZ
    encoder.leap_second_scheduled = true; // this is required because encoder has wiped the flag before and can not compute it
    assert_encoder_stream(encoder, 0b000111, 0b10000001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:01:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01000001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:02:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11000000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:03:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00100001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:04:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10100000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:05:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01100000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:06:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11100001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:07:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00010001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:08:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10010000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:09:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00001001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:10:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10001000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:11:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01001000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:12:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11001001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:13:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00101000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:14:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10101001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:15:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01101001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:16:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11101000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:17:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00011000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:18:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10011001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:19:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00000101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:20:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10000100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:21:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01000100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:22:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11000101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:23:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00100100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:24:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10100101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:25:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01100101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:26:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11100100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:27:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00010100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:28:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10010101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:29:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00001100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:30:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10001101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:31:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01001101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:32:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11001100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:33:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00101101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:34:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10101100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:35:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01101100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:36:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11101101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:37:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00011101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:38:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10011100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:39:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00000011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:40:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10000010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:41:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01000010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:42:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11000011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:43:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00100010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:44:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10100011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:45:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01100011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:46:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11100010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:47:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00010010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:48:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10010011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:49:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00001010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:50:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10001011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:51:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01001011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:52:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11001010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:53:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00101011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:54:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10101010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:55:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b01101010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:56:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b11101011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:57:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00011011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:58:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b10011010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:59:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_encoder_stream(encoder, 0b000111, 0b00000000, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001); ///////  // Do, 01.01.09 01:00:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant, Weiteren Impuls erhalten (Schaltsekunde)
    encoder.leap_second_scheduled = true; // this tests if encoder properly wipes the leap second flag "close to the edge"
    assert_encoder_stream(encoder, 0b000101, 0b10000001, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:01:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b01000001, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:02:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b11000000, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:03:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b00100001, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:04:00, WZ
    assert_encoder_stream(encoder, 0b000101, 0b10100000, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:05:00, WZ
}

void assert_serialized_encoder_stream(Internal::DCF77_Encoder & encoder,
                             uint8_t  info,
                             uint8_t  minute_p,
                             uint8_t  hour_p,
                             uint8_t  day,
                             uint8_t  weekday,
                             uint8_t  month,
                             uint16_t year_p) {
    using namespace Internal;

    DCF77::serialized_clock_stream stream;
    encoder.autoset_control_bits();
    encoder.get_serialized_clock_stream(stream);

    uint64_t bits;
    bits = (mirror_bits(info,     6) << 15) |
           (mirror_bits(minute_p, 8) << 21) |
           (mirror_bits(hour_p,   7) << 29) |
           (mirror_bits(day,      6) << 36) |
           (mirror_bits(weekday,  3) << 42) |
           (mirror_bits(month,    5) << 45) |
           (mirror_bits(year_p,   9) << 50);

    if (verbose) {
        Serial.print(F("expected bit stream:   "));
        for (uint8_t second=16; second<59; ++second) {
            if (second == 21 || second == 29 || second == 36 || second == 42 || second == 45 || second == 50 || second == 59) { Serial.print(' '); }
            Serial.print((uint8_t) (bits >> second ) & 1);
        }
        Serial.println();

        Serial.print(F("serialized bit stream: "));
        uint8_t current_byte_index = 0;
        uint8_t current_bit_index = 3;
        uint8_t current_byte_value = stream.byte_0 >> 3;
        uint8_t second = 16;
        while (current_byte_index < 6) {
            while (current_bit_index < 8) {
                if (second == 21 || second == 29 || second == 36 || second == 42 || second == 45 || second == 50 || second == 59) { Serial.print(' '); }
                const uint8_t tick = current_byte_value & 1;
                Serial.print(tick);

                current_byte_value >>= 1;
                ++current_bit_index;
                if (current_byte_index == 5 && current_bit_index > 5) {
                    break;
                }

                ++second;
            }
            current_bit_index = 0;
            ++current_byte_index;
            current_byte_value = (&(stream.byte_0))[current_byte_index];
        }
        Serial.println();
    }

    bool ok = true;
    uint8_t tick;
    uint8_t expected_tick;
    uint8_t current_byte_index = 0;
    uint8_t current_bit_index = 3;
    uint8_t current_byte_value = stream.byte_0 >> 3;
    uint8_t second = 16;
    while (ok && current_byte_index < 6) {
        while (ok && current_bit_index < 8) {

            tick = current_byte_value & 1;
            expected_tick = (bits >> second) & 1;
            if (tick != expected_tick) {
                ok = false;
            }

            current_byte_value >>= 1;
            ++current_bit_index;
            if (current_byte_index == 5 && current_bit_index > 5) {
                break;
            }
            ++second;
        }
        current_bit_index = 0;
        ++current_byte_index;
        current_byte_value = (&(stream.byte_0))[current_byte_index];
    }

    if (encoder.year.val == 9 &&
        encoder.month.val == 1 &&
        encoder.day.val == 1 &&
        encoder.hour.val == 1 &&
        encoder.minute.val == 0) {
        // leap second 2009 01 01 01:00

        // serialized stream does not deal with leap seconds
    }

    assert(F("encoder output matches DCF77 log"), ok,
             'Y', encoder.year.val,
             'M', encoder.month.val,
             'D', encoder.day.val,
             'h', encoder.hour.val,
             'm', encoder.minute.val,
             's', encoder.second,
             second,
             tick,
             expected_tick
            );
    encoder.second = 0;
    encoder.advance_minute();

}

void test_serialized_signal_year_change() {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x07;
    encoder.month.val = 0x12;
    encoder.day.val = 0x31;
    encoder.hour.val = 0x23;
    encoder.minute.val = 0x30;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.undefined_uses_summertime_output = false;
    encoder.undefined_minute_output = false;
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00001100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:30:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10001101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:31:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01001101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:32:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11001100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:33:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00101101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:34:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10101100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:35:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01101100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:36:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11101101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:37:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00011101, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:38:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10011100, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:39:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:40:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:41:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:42:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:43:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:44:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:45:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01100011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:46:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11100010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:47:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00010010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:48:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10010011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:49:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00001010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:50:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10001011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:51:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01001011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:52:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11001010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:53:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00101011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:54:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10101010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:55:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01101010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:56:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11101011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:57:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00011011, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:58:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10011010, 0b1100011, 0b100011, 0b100, 0b01001, 0b111000001);  // Mo, 31.12.07 23:59:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:00:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:01:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:02:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:03:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:04:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:05:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01100000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:06:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11100001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:07:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00010001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:08:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10010000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:09:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00001001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:10:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10001000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:11:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01001000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:12:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11001001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:13:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00101000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:14:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10101001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:15:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01101001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:16:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11101000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:17:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00011000, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:18:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10011001, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:19:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:20:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:21:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:22:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:23:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:24:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:25:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01100101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:26:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11100100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:27:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00010100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:28:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10010101, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:29:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00001100, 0b0000000, 0b100000, 0b010, 0b10000, 0b000100000);  // Di, 01.01.08 00:30:00, WZ
}

void test_serialized_signal_winter_summertime_change()  {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x09;
    encoder.month.val = 0x03;
    encoder.day.val = 0x29;
    encoder.hour.val = 0x00;
    encoder.minute.val = 0x00;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.undefined_uses_summertime_output = false;
    encoder.undefined_minute_output = false;
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:00:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:01:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:02:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:03:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:04:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:05:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01100000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:06:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11100001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:07:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00010001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:08:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10010000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:09:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00001001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:10:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10001000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:11:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01001000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:12:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11001001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:13:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00101000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:14:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10101001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:15:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01101001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:16:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11101000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:17:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00011000, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:18:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10011001, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:19:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:20:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:21:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:22:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:23:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:24:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:25:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01100101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:26:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11100100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:27:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00010100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:28:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10010101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:29:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00001100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:30:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10001101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:31:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01001101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:32:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11001100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:33:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00101101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:34:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10101100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:35:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01101100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:36:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11101101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:37:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00011101, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:38:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10011100, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:39:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:40:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:41:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:42:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:43:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:44:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:45:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01100011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:46:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11100010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:47:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00010010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:48:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10010011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:49:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00001010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:50:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10001011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:51:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01001011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:52:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11001010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:53:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00101011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:54:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10101010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:55:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01101010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:56:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11101011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:57:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00011011, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:58:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10011010, 0b0000000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 00:59:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:00:00, WZ
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10000001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:01:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01000001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:02:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11000000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:03:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00100001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:04:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10100000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:05:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01100000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:06:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11100001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:07:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00010001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:08:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10010000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:09:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00001001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:10:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10001000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:11:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01001000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:12:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11001001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:13:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00101000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:14:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10101001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:15:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01101001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:16:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11101000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:17:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00011000, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:18:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10011001, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:19:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00000101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:20:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10000100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:21:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01000100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:22:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11000101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:23:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00100100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:24:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10100101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:25:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01100101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:26:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11100100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:27:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00010100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:28:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10010101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:29:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00001100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:30:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10001101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:31:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01001101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:32:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11001100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:33:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00101101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:34:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10101100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:35:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01101100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:36:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11101101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:37:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00011101, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:38:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10011100, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:39:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00000011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:40:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10000010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:41:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01000010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:42:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11000011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:43:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00100010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:44:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10100011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:45:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01100011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:46:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11100010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:47:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00010010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:48:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10010011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:49:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00001010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:50:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10001011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:51:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01001011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:52:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11001010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:53:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00101011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:54:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10101010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:55:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b01101010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:56:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b11101011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:57:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00011011, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:58:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b10011010, 0b1000001, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 01:59:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00000000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:00:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10000001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:01:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01000001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:02:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11000000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:03:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00100001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:04:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10100000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:05:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01100000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:06:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11100001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:07:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00010001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:08:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10010000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:09:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00001001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:10:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10001000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:11:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01001000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:12:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11001001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:13:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00101000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:14:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10101001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:15:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01101001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:16:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11101000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:17:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00011000, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:18:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10011001, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:19:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00000101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:20:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10000100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:21:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01000100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:22:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11000101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:23:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00100100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:24:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10100101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:25:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01100101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:26:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11100100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:27:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00010100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:28:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10010101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:29:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00001100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:30:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10001101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:31:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01001101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:32:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11001100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:33:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00101101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:34:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10101100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:35:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01101100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:36:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11101101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:37:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00011101, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:38:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10011100, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:39:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00000011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:40:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10000010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:41:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01000010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:42:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11000011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:43:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00100010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:44:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10100011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:45:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01100011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:46:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11100010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:47:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00010010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:48:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10010011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:49:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00001010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:50:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10001011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:51:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01001011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:52:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11001010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:53:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00101011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:54:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10101010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:55:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01101010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:56:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11101011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:57:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00011011, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:58:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10011010, 0b1100000, 0b100101, 0b111, 0b11000, 0b100100000);  // So, 29.03.09 03:59:00, SZ
}


void test_serialized_signal_summer_wintertime_change() {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x09;
    encoder.month.val = 0x10;
    encoder.day.val = 0x25;
    encoder.hour.val = 0x01;
    encoder.minute.val = 0x55;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.uses_summertime = true;
    encoder.undefined_uses_summertime_output = false;
    encoder.undefined_minute_output = false;
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10101010, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:55:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b01101010, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:56:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b11101011, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:57:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00011011, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:58:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b10011010, 0b1000001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 01:59:00, SZ
    assert_serialized_encoder_stream(encoder, 0b001001, 0b00000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:00:00, SZ
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:01:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:02:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:03:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00100001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:04:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10100000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:05:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01100000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:06:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11100001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:07:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00010001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:08:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10010000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:09:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00001001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:10:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10001000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:11:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01001000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:12:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11001001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:13:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00101000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:14:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10101001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:15:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01101001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:16:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11101000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:17:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00011000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:18:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10011001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:19:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00000101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:20:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10000100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:21:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01000100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:22:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11000101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:23:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00100100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:24:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10100101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:25:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01100101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:26:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11100100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:27:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00010100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:28:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10010101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:29:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00001100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:30:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10001101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:31:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01001101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:32:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11001100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:33:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00101101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:34:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10101100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:35:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01101100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:36:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11101101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:37:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00011101, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:38:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10011100, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:39:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00000011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:40:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10000010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:41:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01000010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:42:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11000011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:43:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00100010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:44:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10100011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:45:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01100011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:46:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11100010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:47:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00010010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:48:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10010011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:49:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00001010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:50:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10001011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:51:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01001011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:52:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11001010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:53:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00101011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:54:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10101010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:55:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b01101010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:56:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b11101011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:57:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b00011011, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:58:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b011001, 0b10011010, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:59:00, SZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b010101, 0b00000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:00:00, WZ  Bit 16 gesetzt, Zeitzonenwechsel steht bevor
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:01:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:02:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:03:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100001, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:04:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100000, 0b0100001, 0b101001, 0b111, 0b00001, 0b100100001);  // So, 25.10.09 02:05:00, WZ
}

void test_serialized_signal_leap_second() {
    using namespace Internal;

    DCF77_Encoder encoder;
    encoder.year.val = 0x08;
    encoder.month.val = 0x12;
    encoder.day.val = 0x31;
    encoder.hour.val = 0x23;
    encoder.minute.val = 0x55;
    encoder.second = 0;
    encoder.autoset_weekday();
    encoder.autoset_control_bits();
    encoder.abnormal_transmitter_operation = false;
    encoder.undefined_abnormal_transmitter_operation_output = false;
    encoder.undefined_timezone_change_scheduled_output = false;
    encoder.uses_summertime = true;
    encoder.undefined_uses_summertime_output = false;
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10101010, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:55:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01101010, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:56:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11101011, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:57:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00011011, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:58:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10011010, 0b1100011, 0b100011, 0b110, 0b01001, 0b000100000);  // Mi, 31.12.08 23:59:00, WZ
    encoder.leap_second_scheduled = true; // this tests if encoder properly wipes the leap second flag "close to the edge"
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00000000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:00:00, WZ
    encoder.leap_second_scheduled = true; // this is required because encoder has wiped the flag before and can not compute it
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10000001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:01:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01000001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:02:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11000000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:03:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00100001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:04:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10100000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:05:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01100000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:06:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11100001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:07:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00010001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:08:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10010000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:09:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00001001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:10:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10001000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:11:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01001000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:12:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11001001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:13:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00101000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:14:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10101001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:15:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01101001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:16:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11101000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:17:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00011000, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:18:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10011001, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:19:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00000101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:20:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10000100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:21:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01000100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:22:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11000101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:23:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00100100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:24:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10100101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:25:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01100101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:26:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11100100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:27:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00010100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:28:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10010101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:29:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00001100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:30:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10001101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:31:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01001101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:32:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11001100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:33:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00101101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:34:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10101100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:35:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01101100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:36:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11101101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:37:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00011101, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:38:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10011100, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:39:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00000011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:40:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10000010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:41:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01000010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:42:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11000011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:43:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00100010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:44:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10100011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:45:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01100011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:46:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11100010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:47:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00010010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:48:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10010011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:49:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00001010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:50:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10001011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:51:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01001011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:52:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11001010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:53:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00101011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:54:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10101010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:55:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b01101010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:56:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b11101011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:57:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00011011, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:58:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b10011010, 0b0000000, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 00:59:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant
    assert_serialized_encoder_stream(encoder, 0b000111, 0b00000000, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001); ///////  // Do, 01.01.09 01:00:00, WZ  Bit 19 gesetzt, einfuegen einer Schaltsekunde geplant, Weiteren Impuls erhalten (Schaltsekunde)
    encoder.leap_second_scheduled = true; // this tests if encoder properly wipes the leap second flag "close to the edge"
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10000001, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:01:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b01000001, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:02:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b11000000, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:03:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b00100001, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:04:00, WZ
    assert_serialized_encoder_stream(encoder, 0b000101, 0b10100000, 0b1000001, 0b100000, 0b001, 0b10000, 0b100100001);  // Do, 01.01.09 01:05:00, WZ
}

void test_DCF77_Encoder() {
    using namespace Internal;

    {  // days_per_month
        uint8_t days = 0;
        DCF77_Encoder encoder;
        encoder.month.val = 0x01;
        days = encoder.days_per_month();
        assert(F("January has 31 days"), days==31, days);

        days = 0;
        encoder.month.val = 0x03;
        days = encoder.days_per_month();
        assert(F("March has 31 days"), days==31, days);

        days = 0;
        encoder.month.val = 0x04;
        days = encoder.days_per_month();
        assert(F("April has 30 days"), days==30, days);

        days = 0;
        encoder.month.val = 0x05;
        days = encoder.days_per_month();
        assert(F("May has 31 days"), days==31, days);

        days = 0;
        encoder.month.val = 0x06;
        days = encoder.days_per_month();
        assert(F("June has 30 days"), days==30, days);

        days = 0;
        encoder.month.val = 0x07;
        days = encoder.days_per_month();
        assert(F("July has 31 days"), days==31, days);

        days = 0;
        encoder.month.val = 0x08;
        days = encoder.days_per_month();
        assert(F("August has 31 days"), days==31, days);

        days = 0;
        encoder.month.val = 0x09;
        days = encoder.days_per_month();
        assert(F("September has 30 days"), days==30, days);

        days = 0;
        encoder.month.val = 0x10;
        days = encoder.days_per_month();
        assert(F("October has 31 days"), days==31, days);

        days = 0;
        encoder.month.val = 0x06;
        days = encoder.days_per_month();
        assert(F("November has 30 days"), days==30, days);

        days = 0;
        encoder.month.val = 0x12;
        days = encoder.days_per_month();
        assert(F("December has 31 days"), days==31, days);

        bool ok = true;
        uint8_t year;
        encoder.month.val = 0x02;
        for (year = 0; year < 99; ++year) {
            encoder.year = BCD::int_to_bcd(year);
            days = encoder.days_per_month();
            if (days != 28 + (year % 4 == 0) - (year % 100 == 0)) {
                ok = false;
                break;
            }
        }
        assert(F("February has 29 days in leap years, 28 days otherwise"), ok, days, year);
    }

    {  // reset
        DCF77_Encoder encoder = {};

        assert(F("encoder is initialized empty"),
            encoder.second == 0 &&
            encoder.minute.val == 0 &&
            encoder.hour.val == 0 &&
            encoder.day.val == 0 &&
            encoder.month.val == 0 &&
            encoder.year.val == 0 &&
            encoder.weekday.val == 0 &&
            encoder.uses_summertime == false &&
            encoder.abnormal_transmitter_operation == false &&
            encoder.timezone_change_scheduled == false &&
            encoder.undefined_minute_output == false &&
            encoder.undefined_uses_summertime_output == false &&
            encoder.undefined_abnormal_transmitter_operation_output == false &&
            encoder.undefined_timezone_change_scheduled_output == false,
            encoder.second,
            encoder.minute.val,
            encoder.hour.val,
            encoder.day.val,
            encoder.month.val,
            encoder.year.val,
            encoder.weekday.val,
            encoder.uses_summertime,
            encoder.abnormal_transmitter_operation,
            encoder.timezone_change_scheduled,
            encoder.undefined_minute_output,
            encoder.undefined_uses_summertime_output,
            encoder.undefined_abnormal_transmitter_operation_output,
            encoder.undefined_timezone_change_scheduled_output
        );

        encoder.second = 1;
        encoder.minute.val = 1;
        encoder.hour.val = 1;
        encoder.day.val = 1;
        encoder.month.val = 1;
        encoder.year.val = 1;
        encoder.weekday.val = 1;
        encoder.uses_summertime = true;
        encoder.abnormal_transmitter_operation = true;
        encoder.timezone_change_scheduled = true;
        encoder.undefined_minute_output = true;
        encoder.undefined_uses_summertime_output = true;
        encoder.undefined_abnormal_transmitter_operation_output = true;
        encoder.undefined_timezone_change_scheduled_output = true;

        encoder.reset();
        assert(F("reset wipes encoder"),
            encoder.second == 0 &&
            encoder.minute.val == 0 &&
            encoder.hour.val == 0 &&
            encoder.day.val == 1 &&
            encoder.month.val == 1 &&
            encoder.year.val == 0 &&
            encoder.weekday.val == 1 &&
            encoder.uses_summertime == false &&
            encoder.abnormal_transmitter_operation == false &&
            encoder.timezone_change_scheduled == false &&
            encoder.undefined_minute_output == false &&
            encoder.undefined_uses_summertime_output == false &&
            encoder.undefined_abnormal_transmitter_operation_output == false &&
            encoder.undefined_timezone_change_scheduled_output == false,
            encoder.second,
            encoder.minute.val,
            encoder.hour.val,
            encoder.day.val,
            encoder.month.val,
            encoder.year.val,
            encoder.weekday.val,
            encoder.uses_summertime,
            encoder.abnormal_transmitter_operation,
            encoder.timezone_change_scheduled,
            encoder.undefined_minute_output,
            encoder.undefined_uses_summertime_output,
            encoder.undefined_abnormal_transmitter_operation_output,
            encoder.undefined_timezone_change_scheduled_output
        );
    }

    {  // get_weekday
        DCF77_Encoder encoder;

        encoder.year.val = 0x01;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2001 01 01 --> Monday"), encoder.get_weekday() == 1, encoder.get_weekday());

        encoder.year.val = 0x01;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2001 02 28 --> Wednesday"), encoder.get_weekday() == 3, encoder.get_weekday());

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2001 03 01 --> Thursday"), encoder.get_weekday() == 4, encoder.get_weekday());

        encoder.year.val = 0x01;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2002 03 01 --> Monday"), encoder.get_weekday() == 1, encoder.get_weekday());

        encoder.year.val = 0x02;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2002 01 01 --> Tuesday"), encoder.get_weekday() == 2, encoder.get_weekday());

        encoder.year.val = 0x02;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2002 02 28 --> Thursday"), encoder.get_weekday() == 4, encoder.get_weekday());

        encoder.year.val = 0x02;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2002 03 01 --> Friday"), encoder.get_weekday() == 5, encoder.get_weekday());

        encoder.year.val = 0x02;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2002 03 01 --> Tuesday"), encoder.get_weekday() == 2, encoder.get_weekday());

        encoder.year.val = 0x03;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2003 01 01 --> Wednesday"), encoder.get_weekday() == 3, encoder.get_weekday());

        encoder.year.val = 0x03;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2003 02 28 --> Saturday"), encoder.get_weekday() == 5, encoder.get_weekday());

        encoder.year.val = 0x03;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2003 03 01 --> Saturday"), encoder.get_weekday() == 6, encoder.get_weekday());

        encoder.year.val = 0x03;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2003 03 01 --> Wednesday"), encoder.get_weekday() == 3, encoder.get_weekday());

        encoder.year.val = 0x04;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2004 01 01 --> Thursday"), encoder.get_weekday() == 4, encoder.get_weekday());

        encoder.year.val = 0x04;
        encoder.month.val = 0x02;
        encoder.day.val = 0x29;
        assert(F("2004 02 29 --> Sunday"), encoder.get_weekday() == 0, encoder.get_weekday());

        encoder.year.val = 0x04;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2004 03 01 --> Monday"), encoder.get_weekday() == 1, encoder.get_weekday());

        encoder.year.val = 0x04;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2004 03 01 --> Friday"), encoder.get_weekday() == 5, encoder.get_weekday());

        encoder.year.val = 0x99;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2099 01 01 --> Thursday"), encoder.get_weekday() == 4, encoder.get_weekday());

        encoder.year.val = 0x99;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2099 02 28 --> Saturday"), encoder.get_weekday() == 6, encoder.get_weekday());

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2099 03 01 --> Sunday"), encoder.get_weekday() == 0, encoder.get_weekday());

        encoder.year.val = 0x99;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2099 12 31 --> Thursday"), encoder.get_weekday() == 4, encoder.get_weekday());
    }

    {  // get_bcd_weekday
        DCF77_Encoder encoder;

        encoder.year.val = 0x01;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2001 01 01 --> Monday"), encoder.get_bcd_weekday().val == 1, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2001 02 28 --> Wednesday"), encoder.get_bcd_weekday().val == 3, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2001 03 01 --> Thursday"), encoder.get_bcd_weekday().val == 4, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2002 03 01 --> Monday"), encoder.get_bcd_weekday().val == 1, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2002 01 01 --> Tuesday"), encoder.get_bcd_weekday().val == 2, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2002 02 28 --> Thursday"), encoder.get_bcd_weekday().val == 4, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2002 03 01 --> Friday"), encoder.get_bcd_weekday().val == 5, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2002 03 01 --> Tuesday"), encoder.get_bcd_weekday().val == 2, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2003 01 01 --> Wednesday"), encoder.get_bcd_weekday().val == 3, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2003 02 28 --> Saturday"), encoder.get_bcd_weekday().val == 5, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2003 03 01 --> Saturday"), encoder.get_bcd_weekday().val == 6, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2003 03 01 --> Wednesday"), encoder.get_bcd_weekday().val == 3, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2004 01 01 --> Thursday"), encoder.get_bcd_weekday().val == 4, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x02;
        encoder.day.val = 0x29;
        assert(F("2004 02 29 --> Sunday"), encoder.get_bcd_weekday().val == 7, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2004 03 01 --> Monday"), encoder.get_bcd_weekday().val == 1, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2004 03 01 --> Friday"), encoder.get_bcd_weekday().val == 5, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        assert(F("2099 01 01 --> Thursday"), encoder.get_bcd_weekday().val == 4, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        assert(F("2099 02 28 --> Saturday"), encoder.get_bcd_weekday().val == 6, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        assert(F("2099 03 01 --> Sunday"), encoder.get_bcd_weekday().val == 7, encoder.get_bcd_weekday().val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        assert(F("2099 12 31 --> Thursday"), encoder.get_bcd_weekday().val == 4, encoder.get_bcd_weekday().val);
    }


    {  // autoset_weekday
        DCF77_Encoder encoder;

        encoder.year.val = 0x01;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2001 01 01 --> Monday"), encoder.weekday.val == 1, encoder.weekday.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        encoder.autoset_weekday();
        assert(F("2001 02 28 --> Wednesday"), encoder.weekday.val == 3, encoder.weekday.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2001 03 01 --> Thursday"), encoder.weekday.val == 4, encoder.weekday.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        encoder.autoset_weekday();
        assert(F("2002 03 01 --> Monday"), encoder.weekday.val == 1, encoder.weekday.val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2002 01 01 --> Tuesday"), encoder.weekday.val == 2, encoder.weekday.val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        encoder.autoset_weekday();
        assert(F("2002 02 28 --> Thursday"), encoder.weekday.val == 4, encoder.weekday.val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2002 03 01 --> Friday"), encoder.weekday.val == 5, encoder.weekday.val);

        encoder.year.val = 0x02;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        encoder.autoset_weekday();
        assert(F("2002 03 01 --> Tuesday"), encoder.weekday.val == 2, encoder.weekday.val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2003 01 01 --> Wednesday"), encoder.weekday.val == 3, encoder.weekday.val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        encoder.autoset_weekday();
        assert(F("2003 02 28 --> Saturday"), encoder.weekday.val == 5, encoder.weekday.val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2003 03 01 --> Saturday"), encoder.weekday.val == 6, encoder.weekday.val);

        encoder.year.val = 0x03;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        encoder.autoset_weekday();
        assert(F("2003 03 01 --> Wednesday"), encoder.weekday.val == 3, encoder.weekday.val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2004 01 01 --> Thursday"), encoder.weekday.val == 4, encoder.weekday.val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x02;
        encoder.day.val = 0x29;
        encoder.autoset_weekday();
        assert(F("2004 02 29 --> Sunday"), encoder.weekday.val == 7, encoder.weekday.val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2004 03 01 --> Monday"), encoder.weekday.val == 1, encoder.weekday.val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        encoder.autoset_weekday();
        assert(F("2004 03 01 --> Friday"), encoder.weekday.val == 5, encoder.weekday.val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2099 01 01 --> Thursday"), encoder.weekday.val == 4, encoder.weekday.val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        encoder.autoset_weekday();
        assert(F("2099 02 28 --> Saturday"), encoder.weekday.val == 6, encoder.weekday.val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.autoset_weekday();
        assert(F("2099 03 01 --> Sunday"), encoder.weekday.val == 7, encoder.weekday.val);

        encoder.year.val = 0x99;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        encoder.autoset_weekday();
        assert(F("2099 12 31 --> Thursday"), encoder.weekday.val == 4, encoder.weekday.val);
    }

    {  // autoset_timezone
       // Timezone will be CET  before last Sunday in March 2:00
       // Timezone will be CEST after last Sunday in March 2:00
       // Timezone will be CEST till last Sunday in October 2:00 CEST
       // Timezone will be CET  starting last Sunday in October 2:00 CET
       // So except for last Sunday in Octobeer 2:00 the timezone must be autoset,
       // during the one hour is must be derived from the flag vector

       // also test: it works even if weekday.val is not yet set properly

        DCF77_Encoder encoder;

        encoder.year.val = 0x01;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 01 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x02;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 02 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 03 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 03 25 01:00 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 03 25 01:59 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 03 25 02:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 03 25 02:01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 03 25 02:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 03 25 03:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 03 25 03:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x04;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 03 25 04:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 04 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x05;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 05 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x06;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 06 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 07 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x08;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 08 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x09;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 09 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 10 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 10 28 01:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 10 28 01:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 10 28 02:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 10 28 02:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 10 28 02:00 --> Wintertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2001 10 28 01:59 --> Wintertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 10 28 03:00 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x30;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 10 30 23:59 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x11;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 11 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x01;
        encoder.month.val = 0x12;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2001 12 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);


        encoder.year.val = 0x99;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 01 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x02;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 02 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 03 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 03 29 01:00 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 03 29 01:59 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 03 29 02:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 03 29 02:01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 03 29 02:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 03 29 03:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 03 29 03:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x04;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 03 29 04:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 04 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x05;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 05 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x06;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 06 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 07 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x08;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 08 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x09;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 09 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 10 01 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 10 25 01:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);


        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 10 25 01:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 10 25 02:00 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 10 25 02:59 --> Summertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 10 25 02:00 --> Wintertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == false;
        encoder.autoset_timezone();
        assert(F("2099 10 25 01:59 --> Wintertime"), encoder.uses_summertime == true, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 10 25 03:00 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x30;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 10 30 23:59 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x11;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 11 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);

        encoder.year.val = 0x99;
        encoder.month.val = 0x12;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.uses_summertime == true;
        encoder.autoset_timezone();
        assert(F("2099 12 01 --> Wintertime"), encoder.uses_summertime == false, encoder.uses_summertime);
    }

    {  // autoset_timezone_change_scheduled_change_scheduled
       // Timezone will be CET  before last Sunday in March 2:00
       // Timezone will be CEST after last Sunday in March 2:00
       // Timezone will be CEST till last Sunday in October 2:00 CEST
       // Timezone will be CET  starting last Sunday in October 2:00 CET

        // Timezone Change must be scheduled one hour in advance

       // also test: it works even if weekday.val is not yet set properly

        DCF77_Encoder encoder;

        encoder.year.val = 0x01;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.timezone_change_scheduled == true;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 01 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x02;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 02 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 03 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 03 25 01:00 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 03 25 01:01 --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);


        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 03 25 01:59 --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 03 25 03:00 --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 03 25 03:59 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x04;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 03 25 04:00 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 04 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x05;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 05 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x06;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 06 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 07 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x08;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 08 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x09;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 09 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);


        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 28 01:59 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.uses_summertime = true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 28 02:00 CEST --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.uses_summertime = true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 28 02:01 CEST --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.uses_summertime = true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 28 02:59 CEST --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.uses_summertime = false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 28 02:00 CET --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.uses_summertime = false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 28 02:01 CET --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x28;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.uses_summertime = false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 28 03:00 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x30;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 10 30 23:59 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x11;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 11 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x01;
        encoder.month.val = 0x12;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2001 12 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);


        encoder.year.val = 0x99;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 01 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x02;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 02 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 29 01:00 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 29 01:01 --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);


        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 29 01:59 --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 29 03:00 --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 29 03:01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 29 03:59 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x03;
        encoder.day.val = 0x29;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x04;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 03 29 04:00 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 04 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x05;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 05 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x06;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 06 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 07 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x08;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 08 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x09;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 09 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);


        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 25 01:59 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.uses_summertime = true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 25 02:00 CEST --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.uses_summertime = true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 25 02:01 CEST --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.uses_summertime = true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 25 02:59 CEST --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == false;
        encoder.uses_summertime = false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 25 02:00 CET --> change scheduled"), encoder.timezone_change_scheduled == true, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.uses_summertime = false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 25 02:01 CET --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x03;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.uses_summertime = false;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 25 03:00 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x10;
        encoder.day.val = 0x30;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 10 30 23:59 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x11;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 11 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

        encoder.year.val = 0x99;
        encoder.month.val = 0x12;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.autoset_weekday();
        encoder.autoset_timezone();
        encoder.timezone_change_scheduled == true;
        encoder.autoset_timezone_change_scheduled();
        assert(F("2099 12 01 --> no change scheduled"), encoder.timezone_change_scheduled == false, encoder.timezone_change_scheduled);

    }

    {  // verify_leap_second_scheduled
        DCF77_Encoder encoder;
        bool scheduled;

        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 01 01 00:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 01 01 00:01 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 01 01 00:01 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 01 01 00:59 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 01 01 00:59 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 01 01 01:00 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 01 01 01:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 01 01 01:01 --> leap second scheduled"), scheduled == false);


        encoder.year.val = 0x09;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 04 01 01:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 04 01 01:01 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 04 01 01:01 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 04 01 01:59 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 04 01 01:59 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 04 01 02:00 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 04 01 02:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x04;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 04 01 02:01 --> leap second scheduled"), scheduled == false);


        encoder.year.val = 0x09;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 07 01 01:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 04 01 01:01 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 04 01 01:01 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 07 01 01:59 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 07 01 01:59 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 07 01 02:00 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 07 01 02:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x07;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 07 01 02:01 --> leap second scheduled"), scheduled == false);


        encoder.year.val = 0x09;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 10 01 01:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 04 01 01:01 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 04 01 01:01 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 10 01 01:59 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 10 01 01:59 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 10 01 02:00 --> leap second scheduled"), scheduled == true);
        scheduled = encoder.verify_leap_second_scheduled(false);
        assert(F("2009 10 01 02:00 --> leap second not scheduled"), scheduled == false);

        encoder.year.val = 0x09;
        encoder.month.val = 0x10;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        scheduled = encoder.verify_leap_second_scheduled(true);
        assert(F("2009 10 01 02:01 --> leap second scheduled"), scheduled == false);
    }

    {  // autoset_control_bits
        DCF77_Encoder encoder;

        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.weekday.val = 0xff;
        encoder.timezone_change_scheduled = true;
        encoder.leap_second_scheduled = true;
        encoder.uses_summertime = true;
        encoder.autoset_control_bits();
        assert(F("2009 01 01 00:00, Thursday, all flags false"),
                 encoder.weekday.val == 0x04 &&
                 encoder.timezone_change_scheduled == false &&
                 encoder.uses_summertime == false &&
                 encoder.leap_second_scheduled == false,
                 encoder.weekday.val,
                 encoder.timezone_change_scheduled,
                 encoder.uses_summertime,
                 encoder.leap_second_scheduled);


        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x30;
        encoder.second = 0;
        encoder.weekday.val = 0xff;
        encoder.timezone_change_scheduled = true;
        encoder.leap_second_scheduled = true;
        encoder.uses_summertime = true;
        encoder.autoset_control_bits();
        assert(F("2009 01 01 00:30, Thursday, all flags but leap second scheduled false"),
                 encoder.weekday.val == 0x04 &&
                 encoder.timezone_change_scheduled == false &&
                 encoder.uses_summertime == false &&
                 encoder.leap_second_scheduled == true,
                 encoder.weekday.val,
                 encoder.timezone_change_scheduled,
                 encoder.uses_summertime,
                 encoder.leap_second_scheduled);


        encoder.year.val = 0x09;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x30;
        encoder.second = 0;
        encoder.weekday.val = 0xff;
        encoder.timezone_change_scheduled = true;
        encoder.leap_second_scheduled = true;
        encoder.uses_summertime = false;
        encoder.autoset_control_bits();
        assert(F("2009 10 25 01:30, Sunday, Summertime nothing scheduled"),
                 encoder.weekday.val == 0x07 &&
                 encoder.timezone_change_scheduled == false &&
                 encoder.uses_summertime == true &&
                 encoder.leap_second_scheduled == false,
                 encoder.weekday.val,
                 encoder.timezone_change_scheduled,
                 encoder.uses_summertime,
                 encoder.leap_second_scheduled);

        encoder.year.val = 0x09;
        encoder.month.val = 0x10;
        encoder.day.val = 0x25;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x30;
        encoder.second = 0;
        encoder.weekday.val = 0xff;
        encoder.timezone_change_scheduled = false;
        encoder.leap_second_scheduled = true;
        encoder.uses_summertime = true;
        encoder.autoset_control_bits();
        assert(F("2009 10 25 02:30, Sunday, Summertime --> Wintertime scheduled"),
                 encoder.weekday.val = 0x04 &&
                 encoder.timezone_change_scheduled == true &&
                 encoder.uses_summertime == true &&
                 encoder.leap_second_scheduled == false,
                 encoder.weekday.val,
                 encoder.timezone_change_scheduled,
                 encoder.uses_summertime,
                 encoder.leap_second_scheduled);
    }


    {  // advance_minute
        DCF77_Encoder encoder;
        uint8_t i;
        bool ok;

        // properly advances all minutes of an hour
        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        ok = true;
        for (i=0; i<120; ++i) {
            if (bcd_to_int(encoder.minute) != i % 60) {
                ok = false;
                break;
            }
            encoder.advance_minute();
        }
        assert(F("advance 120 minutes"), ok, i, i % 60, encoder.minute.val);

        // properly advances hours of a day
        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.timezone_change_scheduled = false;
        ok = true;
        for (i=0; i<48; ++i) {
            if (bcd_to_int(encoder.hour) != i % 24 || encoder.minute.val != 0x00) {
                ok = false;
                break;
            }
            encoder.minute.val = 0x59;
            encoder.advance_minute();
        }
        assert(F("advance 48 hours"), ok, i, i % 24, encoder.hour.val, encoder.minute.val);

        // properly advance hour Wintertime --> Summertime
        encoder.year.val = 0x08;
        encoder.month.val = 0x03;
        encoder.day.val = 0x30;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x59;
        encoder.uses_summertime = false;
        encoder.timezone_change_scheduled = true;
        encoder.advance_minute();
        assert(F("properly advance CET --> CEST"),
                 encoder.year.val == 0x08 &&
                 encoder.month.val == 0x03 &&
                 encoder.day.val == 0x30 &&
                 encoder.hour.val == 0x03 &&
                 encoder.minute.val == 0x00 &&
                 encoder.uses_summertime == true,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val,
                 encoder.uses_summertime);

        // properly advance hour Summertime --> Wintertime
        encoder.year.val = 0x08;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.hour.val = 0x02;
        encoder.minute.val = 0x59;
        encoder.uses_summertime = true;
        encoder.timezone_change_scheduled = true;
        encoder.advance_minute();
        assert(F("properly advance CEST --> CET"),
                 encoder.year.val == 0x08 &&
                 encoder.month.val == 0x01 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x02 &&
                 encoder.minute.val == 0x00 &&
                 encoder.uses_summertime == false,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val,
                 encoder.uses_summertime);



        // properly advance day
        encoder.year.val = 0x01;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("properly advance one day"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x01 &&
                 encoder.day.val == 0x02 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        // properly advance weekday / wrap around weekday
        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x07;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        ok = true;
        for (i=0; i<14; ++i) {
            if (BCD::bcd_to_int(encoder.weekday) < 1 ||
                BCD::bcd_to_int(encoder.weekday) > 7 ||
                BCD::bcd_to_int(encoder.weekday) % 7 != i % 7) {
                ok = false;
                break;
            }
            encoder.day.val = 0x01;
            encoder.hour.val = 0x23;
            encoder.minute.val = 0x59;
            encoder.advance_minute();
        }
        assert(F("advance 7 weekdays"), ok, i, encoder.weekday.val);

        // properly advance month
        encoder.year.val = 0x00;
        encoder.month.val = 0x12;
        encoder.day.val = 0x31;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to January"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x01 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x01;
        encoder.day.val = 0x31;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to February"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x02 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to March"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x03 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x03;
        encoder.day.val = 0x31;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to April"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x04 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x04;
        encoder.day.val = 0x30;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to May"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x05 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x05;
        encoder.day.val = 0x31;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to June"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x06 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x06;
        encoder.day.val = 0x30;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to July"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x07 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x07;
        encoder.day.val = 0x31;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to August"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x08 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x08;
        encoder.day.val = 0x31;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to September"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x09 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x09;
        encoder.day.val = 0x30;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to October"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x10 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x10;
        encoder.day.val = 0x31;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to November"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x11 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x01;
        encoder.month.val = 0x11;
        encoder.day.val = 0x30;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to December"),
                 encoder.year.val == 0x01 &&
                 encoder.month.val == 0x12 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        // properly handle leap year advance month
        encoder.year.val = 0x04;
        encoder.month.val = 0x02;
        encoder.day.val = 0x28;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("do not advance minute to March on 28th February of leap year"),
                 encoder.year.val == 0x04 &&
                 encoder.month.val == 0x02 &&
                 encoder.day.val == 0x29 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        encoder.year.val = 0x04;
        encoder.month.val = 0x02;
        encoder.day.val = 0x29;
        encoder.hour.val = 0x23;
        encoder.minute.val = 0x59;
        encoder.advance_minute();
        assert(F("advance minute to March on leap year"),
                 encoder.year.val == 0x04 &&
                 encoder.month.val == 0x03 &&
                 encoder.day.val == 0x01 &&
                 encoder.hour.val == 0x00 &&
                 encoder.minute.val == 0x00,
                 encoder.year.val,
                 encoder.month.val,
                 encoder.day.val,
                 encoder.hour.val,
                 encoder.minute.val);

        // properly advance year
        encoder.year.val = 0x00;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.hour.val = 0x00;
        encoder.minute.val = 0x00;
        ok = true;
        for (i=0; i<100; ++i) {
            if (bcd_to_int(encoder.year) != i % 100 ||
                encoder.month.val != 0x01 ||
                encoder.day.val != 0x01 ||
                encoder.hour.val != 0x00 ||
                encoder.minute.val != 0x00) {

                ok = false;
                break;
            }
            encoder.year = BCD::int_to_bcd(i);
            encoder.month.val = 0x12;
            encoder.day.val = 0x31;
            encoder.hour.val = 0x23;
            encoder.minute.val = 0x59;
            encoder.advance_minute();
        }
        assert(F("advance 100 years"), ok, i, encoder.year.val);
    }

    {  // advance_second
        DCF77_Encoder encoder;
        uint8_t i;
        bool ok;

        // advances a second properly
        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.leap_second_scheduled = false;
        for (i=0, ok=true; i<120; ++i) {
            if (encoder.second != i % 60) {
                ok = false;
                break;
            }
            encoder.advance_second();
        }
        assert(F("advance 181 seconds incl. leap second"), ok, i, encoder.second);

        // advances leap second properly
        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x00;
        encoder.second = 0;
        encoder.leap_second_scheduled = true;
        for (i=0, ok=true; i<180; ++i) {
            if (i == 60) {
                if (encoder.second == 60) {
                    // step over leap second
                    encoder.advance_second();
                } else {
                    ok = false;
                    break;
                }
            }
            if (encoder.second != i % 60) {
                ok =false;
                break;
            }
            encoder.advance_second();
        }
        assert(F("advance 120 seconds"), ok, i, encoder.second);

        // autosets control bits at least once per minute
        encoder.year.val = 0x09;
        encoder.month.val = 0x01;
        encoder.day.val = 0x01;
        encoder.weekday.val = 0x04;
        encoder.hour.val = 0x01;
        encoder.minute.val = 0x01;
        encoder.second = 0;
        encoder.leap_second_scheduled = true;
        for (i=0; i<58; ++i) {
            encoder.advance_second();
        }
        assert(F("autosets control bits sufficiently before potential leap second"), encoder.leap_second_scheduled == false);
    }

    {  // get_current_signal
        test_signal_year_change();
        test_signal_winter_summertime_change();
        test_signal_summer_wintertime_change();
        test_signal_leap_second();
    }

    {  // get_serialized_clock_stream
        test_serialized_signal_year_change();
        test_serialized_signal_winter_summertime_change();
        test_serialized_signal_summer_wintertime_change();
        test_serialized_signal_leap_second();
    }
}

void test_Flag_Decoder() {
    using namespace Internal;
    {  // setup
        DCF77_Flag_Decoder decoder;

        decoder.uses_summertime = 1;
        decoder.abnormal_transmitter_operation = 1;
        decoder.timezone_change_scheduled = 1;
        decoder.leap_second_scheduled = 1;
        decoder.date_parity = 1;

        decoder.setup();

        assert(F("setup clears flag decoder"),
                 decoder.uses_summertime == 0 &&
                 decoder.abnormal_transmitter_operation == 0 &&
                 decoder.timezone_change_scheduled == 0 &&
                 decoder.leap_second_scheduled == 0 &&
                 decoder.date_parity == 0,
                 decoder.uses_summertime,
                 decoder.abnormal_transmitter_operation,
                 decoder.timezone_change_scheduled,
                 decoder.leap_second_scheduled,
                 decoder.date_parity);
    }

    {  // cummulate
        int8_t avg;

        avg = -127;
        for (int8_t n=avg; n < 127; ++n) {
            assert(F("cummulate(n, true) increments n while n < 127"), avg == n, avg);
            DCF77_Flag_Decoder::cummulate(avg, true);
        }
        DCF77_Flag_Decoder::cummulate(avg, true);
        DCF77_Flag_Decoder::cummulate(avg, true);
        assert(F("cummulate(n, true) stops at 127"), avg == 127, avg);

        avg = 127;
        for (int8_t n=avg; n > -127; --n) {
            assert(F("cummulate(n, false) decrements n while n > -127"), avg == n, avg);
            DCF77_Flag_Decoder::cummulate(avg, false);
        }
        DCF77_Flag_Decoder::cummulate(avg, false);
        DCF77_Flag_Decoder::cummulate(avg, false);
        assert(F("cummulate(n, true) stops at -127"), avg == -127, avg);
    }

    {  // process_tick
        DCF77_Flag_Decoder decoder;

        decoder.setup();

        decoder.abnormal_transmitter_operation = 1;
        decoder.process_tick(15, 0);
        assert(F("tick 15, 0"), decoder.abnormal_transmitter_operation == 0,
                 decoder.abnormal_transmitter_operation);

        decoder.process_tick(15, 0);
        assert(F("tick 15, 0 does not cummulate"), decoder.abnormal_transmitter_operation == 0,
                 decoder.abnormal_transmitter_operation);


        decoder.abnormal_transmitter_operation = 0;
        decoder.process_tick(15, 1);
        assert(F("tick 15, 1"), decoder.abnormal_transmitter_operation == 1,
                 decoder.abnormal_transmitter_operation);

        decoder.process_tick(15, 1);
        assert(F("tick 15, 1 does not cummulate"), decoder.abnormal_transmitter_operation == 1,
                 decoder.abnormal_transmitter_operation);


        decoder.timezone_change_scheduled = 127;
        decoder.process_tick(16, 0);
        assert(F("tick 16, 0"), decoder.timezone_change_scheduled == 126,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = 1;
        decoder.process_tick(16, 0);
        assert(F("tick 16, 0"), decoder.timezone_change_scheduled == 0,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = 0;
        decoder.process_tick(16, 0);
        assert(F("tick 16, 0"), decoder.timezone_change_scheduled == -1,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = -126;
        decoder.process_tick(16, 0);
        assert(F("tick 16, 0"), decoder.timezone_change_scheduled == -127,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = -127;
        decoder.process_tick(16, 0);
        assert(F("tick 16, 0"), decoder.timezone_change_scheduled == -127,
                 decoder.timezone_change_scheduled);


        decoder.timezone_change_scheduled = -127;
        decoder.process_tick(16, 1);
        assert(F("tick 16, 1"), decoder.timezone_change_scheduled == -126,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = -1;
        decoder.process_tick(16, 1);
        assert(F("tick 16, 1"), decoder.timezone_change_scheduled == 0,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = 0;
        decoder.process_tick(16, 1);
        assert(F("tick 16, 1"), decoder.timezone_change_scheduled == 1,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = 126;
        decoder.process_tick(16, 1);
        assert(F("tick 16, 1"), decoder.timezone_change_scheduled == 127,
                 decoder.timezone_change_scheduled);

        decoder.timezone_change_scheduled = 127;
        decoder.process_tick(16, 1);
        assert(F("tick 16, 1"), decoder.timezone_change_scheduled == 127,
                 decoder.timezone_change_scheduled);


        decoder.uses_summertime = 127;
        decoder.process_tick(17, 0);
        assert(F("tick 17, 0"), decoder.uses_summertime == 126,
                 decoder.uses_summertime);

        decoder.uses_summertime = 1;
        decoder.process_tick(17, 0);
        assert(F("tick 17, 0"), decoder.uses_summertime == 0,
                 decoder.uses_summertime);

        decoder.uses_summertime = 0;
        decoder.process_tick(17, 0);
        assert(F("tick 17, 0"), decoder.uses_summertime == -1,
                 decoder.uses_summertime);

        decoder.uses_summertime = -126;
        decoder.process_tick(17, 0);
        assert(F("tick 17, 0"), decoder.uses_summertime == -127,
                 decoder.uses_summertime);

        decoder.uses_summertime = -127;
        decoder.process_tick(17, 0);
        assert(F("tick 17, 0"), decoder.uses_summertime == -127,
                 decoder.uses_summertime);


        decoder.uses_summertime = -127;
        decoder.process_tick(17, 1);
        assert(F("tick 17, 1"), decoder.uses_summertime == -126,
                 decoder.uses_summertime);

        decoder.uses_summertime = -1;
        decoder.process_tick(17, 1);
        assert(F("tick 17, 1"), decoder.uses_summertime == 0,
                 decoder.uses_summertime);

        decoder.uses_summertime = 0;
        decoder.process_tick(17, 1);
        assert(F("tick 17, 1"), decoder.uses_summertime == 1,
                 decoder.uses_summertime);

        decoder.uses_summertime = 126;
        decoder.process_tick(17, 1);
        assert(F("tick 17, 1"), decoder.uses_summertime == 127,
                 decoder.uses_summertime);

        decoder.uses_summertime = 127;
        decoder.process_tick(17, 1);
        assert(F("tick 17, 1"), decoder.uses_summertime == 127,
                 decoder.uses_summertime);


        decoder.uses_summertime = 127;
        decoder.process_tick(18, 1);
        assert(F("tick 18, 1"), decoder.uses_summertime == 126,
                 decoder.uses_summertime);

        decoder.uses_summertime = 1;
        decoder.process_tick(18, 1);
        assert(F("tick 18, 1"), decoder.uses_summertime == 0,
                 decoder.uses_summertime);

        decoder.uses_summertime = 0;
        decoder.process_tick(18, 1);
        assert(F("tick 18, 1"), decoder.uses_summertime == -1,
                 decoder.uses_summertime);

        decoder.uses_summertime = -126;
        decoder.process_tick(18, 1);
        assert(F("tick 18, 1"), decoder.uses_summertime == -127,
                 decoder.uses_summertime);

        decoder.uses_summertime = -127;
        decoder.process_tick(18, 1);
        assert(F("tick 18, 1"), decoder.uses_summertime == -127,
                 decoder.uses_summertime);


        decoder.uses_summertime = -127;
        decoder.process_tick(18, 0);
        assert(F("tick 18, 0"), decoder.uses_summertime == -126,
                 decoder.uses_summertime);

        decoder.uses_summertime = -1;
        decoder.process_tick(18, 0);
        assert(F("tick 18, 0"), decoder.uses_summertime == 0,
                 decoder.uses_summertime);

        decoder.uses_summertime = 0;
        decoder.process_tick(18, 0);
        assert(F("tick 18, 0"), decoder.uses_summertime == 1,
                 decoder.uses_summertime);

        decoder.uses_summertime = 126;
        decoder.process_tick(18, 0);
        assert(F("tick 18, 0"), decoder.uses_summertime == 127,
                 decoder.uses_summertime);

        decoder.uses_summertime = 127;
        decoder.process_tick(18, 0);
        assert(F("tick 18, 0"), decoder.uses_summertime == 127,
                 decoder.uses_summertime);


        decoder.leap_second_scheduled = 127;
        decoder.process_tick(19, 0);
        assert(F("tick 19, 0"), decoder.leap_second_scheduled == 126,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = 1;
        decoder.process_tick(19, 0);
        assert(F("tick 19, 0"), decoder.leap_second_scheduled == 0,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = 0;
        decoder.process_tick(19, 0);
        assert(F("tick 19, 0"), decoder.leap_second_scheduled == -1,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = -126;
        decoder.process_tick(19, 0);
        assert(F("tick 19, 0"), decoder.leap_second_scheduled == -127,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = -127;
        decoder.process_tick(19, 0);
        assert(F("tick 19, 0"), decoder.leap_second_scheduled == -127,
                 decoder.leap_second_scheduled);


        decoder.leap_second_scheduled = -127;
        decoder.process_tick(19, 1);
        assert(F("tick 19, 1"), decoder.leap_second_scheduled == -126,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = -1;
        decoder.process_tick(19, 1);
        assert(F("tick 19, 1"), decoder.leap_second_scheduled == 0,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = 0;
        decoder.process_tick(19, 1);
        assert(F("tick 19, 1"), decoder.leap_second_scheduled == 1,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = 126;
        decoder.process_tick(19, 1);
        assert(F("tick 19, 1"), decoder.leap_second_scheduled == 127,
                 decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = 127;
        decoder.process_tick(19, 1);
        assert(F("tick 19, 1"), decoder.leap_second_scheduled == 127,
                 decoder.leap_second_scheduled);


                decoder.date_parity = 127;
        decoder.process_tick(58, 0);
        assert(F("tick 58, 0"), decoder.date_parity == 126,
                 decoder.date_parity);

        decoder.date_parity = 1;
        decoder.process_tick(58, 0);
        assert(F("tick 58, 0"), decoder.date_parity == 0,
                 decoder.date_parity);

        decoder.date_parity = 0;
        decoder.process_tick(58, 0);
        assert(F("tick 58, 0"), decoder.date_parity == -1,
                 decoder.date_parity);

        decoder.date_parity = -126;
        decoder.process_tick(58, 0);
        assert(F("tick 58, 0"), decoder.date_parity == -127,
                 decoder.date_parity);

        decoder.date_parity = -127;
        decoder.process_tick(58, 0);
        assert(F("tick 58, 0"), decoder.date_parity == -127,
                 decoder.date_parity);


        decoder.date_parity = -127;
        decoder.process_tick(58, 1);
        assert(F("tick 58, 1"), decoder.date_parity == -126,
                 decoder.date_parity);

        decoder.date_parity = -1;
        decoder.process_tick(58, 1);
        assert(F("tick 58, 1"), decoder.date_parity == 0,
                 decoder.date_parity);

        decoder.date_parity = 0;
        decoder.process_tick(58, 1);
        assert(F("tick 58, 1"), decoder.date_parity == 1,
                 decoder.date_parity);

        decoder.date_parity = 126;
        decoder.process_tick(58, 1);
        assert(F("tick 58, 1"), decoder.date_parity == 127,
                 decoder.date_parity);

        decoder.date_parity = 127;
        decoder.process_tick(58, 1);
        assert(F("tick 58, 1"), decoder.date_parity == 127,
                 decoder.date_parity);
    }

    {  // reset_after_previous_hour
        DCF77_Flag_Decoder decoder;

        decoder.uses_summertime = 10;
        decoder.timezone_change_scheduled = 11;
        decoder.reset_after_previous_hour();
        assert(F("reset_after_previous_hour timezone_change_scheduled"),
                   decoder.timezone_change_scheduled == 0,
                   decoder.timezone_change_scheduled);
        assert(F("reset_after_previous_hour timezone on timezone_change_scheduled"),
                   decoder.uses_summertime == 0,
                   decoder.uses_summertime);


        decoder.timezone_change_scheduled = -10;
        decoder.uses_summertime = 11;
        decoder.reset_after_previous_hour();
        assert(F("reset_after_previous_hour timezone_change_scheduled"),
                   decoder.timezone_change_scheduled == 0,
                   decoder.timezone_change_scheduled);
        assert(F("reset_after_previous_hour does not reset timezone on no timezone_change_scheduled"),
                   decoder.uses_summertime == 11,
                   decoder.uses_summertime);


        decoder.leap_second_scheduled = 10;
        decoder.reset_after_previous_hour();
        assert(F("reset_after_previous_hour leap_second_scheduled"),
                   decoder.leap_second_scheduled == 0,
                   decoder.leap_second_scheduled);

        decoder.leap_second_scheduled = -10;
        decoder.reset_after_previous_hour();
        assert(F("reset_after_previous_hour leap_second_scheduled"),
                   decoder.leap_second_scheduled == 0,
                   decoder.leap_second_scheduled);
    }

    {  // reset_before_new_day
        DCF77_Flag_Decoder decoder;

        decoder.date_parity = 10;
        decoder.reset_before_new_day();
        assert(F("reset_before_new_day date_parity"),
                   decoder.date_parity == 0,
                   decoder.date_parity);
    }

    {  // get_uses_summertime
        DCF77_Flag_Decoder decoder;

        decoder.uses_summertime = 1;
        assert(F("get_uses_summertime"),
                 decoder.get_uses_summertime() == true,
                 decoder.get_uses_summertime());

        decoder.uses_summertime = -1;
        assert(F("get_uses_summertime"),
                 decoder.get_uses_summertime() == false,
                 decoder.get_uses_summertime());

        decoder.uses_summertime = 0;
        assert(F("get_uses_summertime biased to true"),
                 decoder.get_uses_summertime() == true,
                 decoder.get_uses_summertime());
    }

    {  // get_abnormal_transmitter_operation
        DCF77_Flag_Decoder decoder;

        decoder.abnormal_transmitter_operation = 1;
        assert(F("get_abnormal_transmitter_operation"),
                 decoder.get_abnormal_transmitter_operation() == true,
                 decoder.get_abnormal_transmitter_operation());

        decoder.abnormal_transmitter_operation = 0;
        assert(F("get_abnormal_transmitter_operation"),
                 decoder.get_abnormal_transmitter_operation() == false,
                 decoder.get_abnormal_transmitter_operation());
    }

    {  // get_timezone_change_scheduled
        DCF77_Flag_Decoder decoder;

        decoder.timezone_change_scheduled = 1;
        assert(F("get_timezone_change_scheduled"),
                 decoder.get_timezone_change_scheduled() == true,
                 decoder.get_timezone_change_scheduled());

        decoder.timezone_change_scheduled = -1;
        assert(F("get_timezone_change_scheduled"),
                 decoder.get_timezone_change_scheduled() == false,
                 decoder.get_timezone_change_scheduled());

        decoder.timezone_change_scheduled = 0;
        assert(F("get_timezone_change_scheduled biased to false"),
                 decoder.get_timezone_change_scheduled() == false,
                 decoder.get_timezone_change_scheduled());
    }

    {  // get_leap_second_scheduled
        DCF77_Flag_Decoder decoder;

        decoder.leap_second_scheduled = 1;
        assert(F("get_leap_second_scheduled"),
                 decoder.get_leap_second_scheduled() == true,
                 decoder.get_leap_second_scheduled());

        decoder.leap_second_scheduled = -1;
        assert(F("get_leap_second_scheduled"),
                 decoder.get_leap_second_scheduled() == false,
                 decoder.get_leap_second_scheduled());

        decoder.leap_second_scheduled = 0;
        assert(F("get_leap_second_scheduled biased to false"),
                 decoder.get_leap_second_scheduled() == false,
                 decoder.get_leap_second_scheduled());
    }

    {  // get_date_parity
        DCF77_Flag_Decoder decoder;

        decoder.date_parity = 1;
        assert(F("get_date_parity"),
                 decoder.get_date_parity() == true,
                 decoder.get_date_parity());

        decoder.date_parity = -1;
        assert(F("get_date_parity"),
                 decoder.get_date_parity() == false,
                 decoder.get_date_parity());

        decoder.date_parity = 0;
        // do not check any bias, it does not really matter
    }

    {  // get_quality
        DCF77_Flag_Decoder decoder;
        uint8_t uses_summertime_quality;
        uint8_t timezone_change_scheduled_quality;
        uint8_t leapsecond_change_scheduled_quality;

        decoder.uses_summertime = 10;
        decoder.timezone_change_scheduled = 11;
        decoder.leap_second_scheduled = 12;
        decoder.get_quality(uses_summertime_quality,
                            timezone_change_scheduled_quality,
                            leapsecond_change_scheduled_quality);
        assert(F("decoder quality delivers absolute values"),
                 uses_summertime_quality == 10 &&
                 timezone_change_scheduled_quality == 11 &&
                 leapsecond_change_scheduled_quality == 12,
                 uses_summertime_quality,
                 timezone_change_scheduled_quality,
                 leapsecond_change_scheduled_quality);

        decoder.uses_summertime = -10;
        decoder.timezone_change_scheduled = -11;
        decoder.leap_second_scheduled = -12;
        decoder.get_quality(uses_summertime_quality,
                            timezone_change_scheduled_quality,
                            leapsecond_change_scheduled_quality);
        assert(F("decoder quality delivers absolute values"),
                 uses_summertime_quality == 10 &&
                 timezone_change_scheduled_quality == 11 &&
                 leapsecond_change_scheduled_quality == 12,
                 uses_summertime_quality,
                 timezone_change_scheduled_quality,
                 leapsecond_change_scheduled_quality);
    }
}


struct modulator {
    // this is bascially the start time INPUT,
    // since the modulator keeps its state here, it is also
    // the OUTPUT
    Internal::DCF77_Encoder encoder;
    uint16_t phase = 0;

    // these are parameters that control the modulator behaviour
    // if necessary they can be modified to simulate a poor signal
    uint16_t short_tick_duration = 100;
    uint16_t long_tick_duration = 200;
    int8_t  drift = 0;

    // this is the modulator output
    Internal::DCF77::tick_t output_tick;
    uint8_t signal;

    // this is the interal state of the modulator
    uint8_t tick_duration;
    int32_t cummulated_drift = 0;


    void compute_next_ms() {
        using namespace Internal;

        if (phase == 0) {
            output_tick = encoder.get_current_signal();
            tick_duration = output_tick == DCF77::long_tick ? long_tick_duration:
                            output_tick == DCF77::short_tick? short_tick_duration:
                            output_tick == DCF77::undefined ? short_tick_duration:
                                                                                0;
        }

        // push output
        signal = (phase < tick_duration);

        ++phase;

        cummulated_drift += drift;
        if (cummulated_drift < -1000000) {
            --phase;
            cummulated_drift += 1000000;
        }

        if (cummulated_drift > 1000000) {
            ++phase;
            cummulated_drift -= 1000000;
        }

        if (phase >= 1000) {
            encoder.advance_second();
            phase -= 1000;
        }
    }
};

template <boolean want_high_resolution>
struct Configuration_T {
    static const boolean want_high_phase_lock_resolution = want_high_resolution;
    //const boolean want_high_phase_lock_resolution = false;

    // end of configuration section, the stuff below
    // will compute the implications of the desired configuration,
    // ready for the compiler to consume
    #if defined(__arm__)
    static const boolean has_lots_of_memory = true;
    #else
    static const boolean has_lots_of_memory = false;
    #endif

    static const boolean high_phase_lock_resolution = want_high_phase_lock_resolution &&
                                                      has_lots_of_memory;

    enum ticks_per_second_t : uint16_t { centi_seconds = 100, milli_seconds = 1000 };
    // this is the actuall sample rate
    static const ticks_per_second_t phase_lock_resolution = high_phase_lock_resolution ? milli_seconds
                                                                                       : centi_seconds;
};


template <typename Configuration_T>
struct Controller_T {
    typedef Configuration_T Configuration;
    typedef void *Controller_process_single_tick_handler(Internal::DCF77::tick_t);

    static Internal::DCF77::tick_t last_tick;

    static void process_single_tick_data(Internal::DCF77::tick_t tick) {
        last_tick = tick;
    }
};
template <typename Configuration_T> Internal::DCF77::tick_t Controller_T<Configuration_T>::last_tick;

template <typename Configuration_T>
void test_Demodulator_internal() {
    using namespace Internal;
    typedef Controller_T<Configuration_T> controller_t;
    typedef DCF77_Demodulator<Controller_T<Configuration_T> > Demodulator_t;

    const uint16_t bin_count = Demodulator_t::bin_count;
    const uint16_t bins_per_10ms  = Demodulator_t::bins_per_10ms;
    const uint16_t bins_per_50ms  = Demodulator_t::bins_per_50ms;
    const uint16_t bins_per_100ms = Demodulator_t::bins_per_100ms;
    const uint16_t samples_per_second = 1000;
    const uint16_t samples_per_bin = samples_per_second / bin_count;

    const bool hires = (bin_count > 100);

    {  // static attributes
        assert(F("bin_count vs. resolution"),
               bin_count == hires? 1000: 100,
               Demodulator_t::bins_per_10ms  == (hires? 10 : 1)    &&
               Demodulator_t::bins_per_50ms  ==  5 * bins_per_10ms &&
               Demodulator_t::bins_per_60ms  ==  6 * bins_per_10ms &&
               Demodulator_t::bins_per_100ms == 10 * bins_per_10ms &&
               Demodulator_t::bins_per_200ms == 20 * bins_per_10ms &&
               Demodulator_t::bins_per_400ms == 40 * bins_per_10ms &&
               Demodulator_t::bins_per_500ms == 50 * bins_per_10ms &&
               Demodulator_t::bins_per_600ms == 60 * bins_per_10ms,
               bin_count,
               hires,
               Demodulator_t::bins_per_10ms,
               Demodulator_t::bins_per_50ms,
               Demodulator_t::bins_per_60ms,
               Demodulator_t::bins_per_100ms,
               Demodulator_t::bins_per_200ms,
               Demodulator_t::bins_per_400ms,
               Demodulator_t::bins_per_500ms,
               Demodulator_t::bins_per_600ms);

        assert(F("samples_per_bin vs. resolution"), samples_per_bin == hires? 1: 10,
               samples_per_bin, hires);

        // proper types
        assert(F("proper index_t"), sizeof(Demodulator_t::bin_count) == (bin_count > 255 ? 2: 1),
               sizeof(Demodulator_t::bin_count), bin_count);
    }

    {  // wrap
        uint16_t value;

        value = Demodulator_t::wrap(0);
        assert(F("0 never wraps"), value == 0, value, bin_count);

        value = Demodulator_t::wrap(bin_count);
        assert(F("wrap to 0"), value == 0, value, bin_count);

        value = Demodulator_t::wrap(bin_count+1);
        assert(F("wrap to 1"), value == 1, value, bin_count);

        value = Demodulator_t::wrap(1000);
        assert(F("1000 wraps to 0"), value == 1000 % bin_count, value, bin_count);

        value = Demodulator_t::wrap(54021);
        assert(F("54021 wraps to 21"), value == 54021 % bin_count, value, bin_count);
    }

    {  // set_has_tuned_clock
        assert(F("tuned clock assumed to drift 10 times less than untuned clock"),
               Demodulator_t::ticks_to_drift_one_tick * 10 <= Demodulator_t::tuned_ticks_to_drift_one_tick,
               Demodulator_t::ticks_to_drift_one_tick,
               Demodulator_t::tuned_ticks_to_drift_one_tick);

        Demodulator_t decoder;
        uint16_t n0 = decoder.N;
        decoder.set_has_tuned_clock();
        uint16_t n1 = decoder.N;

        assert(F("tuning increases filter constant"), n1 == 10 * n0,
               n1, n0);

        assert(F("filter constant increases according to drift improvement"),
               n1 / n0 == Demodulator_t::tuned_ticks_to_drift_one_tick / Demodulator_t::ticks_to_drift_one_tick,
               n1, n0);
    }

    {  // phase_binning
        // test how a clean short tick is detected
        Demodulator_t decoder;
        for (uint16_t offset = 0; offset < bin_count && offset < bin_count; ++offset) {
            decoder.setup();

            for (uint16_t bin = 0; bin < 40*bin_count; ++bin) {
                const uint8_t data = ((bin+offset) % bin_count) < bins_per_100ms;
                decoder.phase_binning(data);
            }

            const uint16_t expected_index = (bin_count + 1 - offset) % bin_count;
            assert(F("proper phase lock on a clean short tick"), decoder.running_max_index == expected_index,
                   decoder.bin_count,
                   decoder.running_max_index,
                   expected_index);
        }

        // test how a clean long tick is detected
        for (uint16_t offset = 0; offset < bin_count && offset < bin_count; ++offset) {
            decoder.setup();

            for (uint16_t bin = 0; bin < 40*bin_count; ++bin) {
                const uint8_t data = ((bin+offset) % bin_count) < 2*bins_per_100ms;
                decoder.phase_binning(data);
            }

            const uint16_t expected_index = (bin_count + 1 - offset) % bin_count;
            assert(F("proper phase lock on a clean long tick"), decoder.running_max_index == expected_index,
                   decoder.bin_count,
                   decoder.running_max_index,
                   expected_index);
        }

        // no tests with noise because we will test this together with the rest of the decoder
    }

    {  // decode_200ms
        Demodulator_t decoder;
        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go > 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(0, bins_to_go);
        }
        assert(F("clock controller not triggered after decoding ++200 ms"),
               controller_t::last_tick == (DCF77::tick_t) 0xFF,
               controller_t::last_tick,
               hires);
        decoder.decode_200ms(0, 0);
        assert(F("clock controller triggered after decoding ++++200 ms"),
               controller_t::last_tick != (DCF77::tick_t) 0xFF,
               controller_t::last_tick,
               hires);
        assert(F("decode all 0 as sync marc"),
               controller_t::last_tick == DCF77::sync_mark,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone < bins_per_50ms, bins_to_go);
        }
        assert(F("decode 50ms 1 as sync marc"),
               controller_t::last_tick == DCF77::sync_mark,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone <= bins_per_50ms, bins_to_go);
        }
        assert(F("decode ++50ms 1 as short tick"),
               controller_t::last_tick == DCF77::short_tick,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone <= 3*bins_per_50ms, bins_to_go);
        }
        assert(F("decode ++150ms as short tick"),
               controller_t::last_tick == DCF77::short_tick,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone <= 3*bins_per_50ms+1, bins_to_go);
        }
        assert(F("decode ++++150ms as long tick"),
               controller_t::last_tick == DCF77::long_tick,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(1, bins_to_go);
        }
        assert(F("decode all 1 as long tick"),
               controller_t::last_tick == DCF77::long_tick,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone <= bins_per_100ms ||
                                 bins_gone > bins_per_100ms + bins_per_50ms +1 ,
                                 bins_to_go);
        }
        assert(F("decode ++100ms 1 followed by ++50ms 0 and 50ms 1 as short tick"),
               controller_t::last_tick == DCF77::short_tick,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone <= bins_per_100ms+1 ||
                                 bins_gone > bins_per_100ms+1 + bins_per_50ms,
                                 bins_to_go);
        }
        assert(F("decode ++++100ms 1 followed by 50ms 0 and 50ms 1 as long tick"),
               controller_t::last_tick == DCF77::long_tick,
               controller_t::last_tick,
               hires);

        controller_t::last_tick = (DCF77::tick_t) 0xFF;
        decoder.decoded_data = 0;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone > decoder.bins_per_50ms &&
                                 bins_gone <= 3*decoder.bins_per_50ms,
                                 bins_to_go);
        }
        assert(F("decode ++50ms 0 followed by 100ms 1 as sync marc"),
               controller_t::last_tick == DCF77::sync_mark,
               controller_t::last_tick,
               hires);

                controller_t::last_tick = (DCF77::tick_t) 0xFF;
        for (int16_t bins_to_go = decoder.bins_per_200ms+1, bins_gone = 0; bins_to_go >= 0; --bins_to_go, ++bins_gone) {
            decoder.decode_200ms(bins_gone > decoder.bins_per_50ms-1 && bins_gone <= 3*decoder.bins_per_50ms, bins_to_go);
        }
        assert(F("decode 50ms 0 followed ++100ms 1 as short tick"),
               controller_t::last_tick == DCF77::short_tick,
               controller_t::last_tick,
               hires);
    }

    {  // stage_with_averages
        if (samples_per_bin > 1) {
            typename Demodulator_t::stage_with_averages stage;

            stage.sample_count = 1;
            stage.sum = 1;
            stage.reset();
            assert(F("reset stage"), stage.sample_count == 0 && stage.sum == 0,
                   stage.sample_count, stage.sum);

            stage.reduce(1);
            assert(F("reduce 1"), stage.sample_count == 1 && stage.sum == 1,
                   stage.sample_count, stage.sum);

            stage.reduce(4);
            assert(F("reduce 4 more"), stage.sample_count == 2 && stage.sum == 5,
                   stage.sample_count, stage.sum);

            stage.reduce(0);
            stage.reduce(0);
            stage.reduce(0);
            stage.reduce(0);
            stage.reduce(0);
            stage.reduce(0);
            stage.reduce(0);
            assert(F("reduce 7 zeroes more"), stage.sample_count == 9 && stage.sum == 5,
                   stage.sample_count, stage.sum);

            assert(F("data not ready after 9 samples"), stage.data_ready() == false,
                   stage.data_ready());
            assert(F("cummulated data below threshhold"), stage.avg() == 0,
                   stage.avg());

            stage.reduce(1);
            assert(F("data ready after 10 samples"), stage.data_ready() == true,
                   stage.data_ready());
            assert(F("cummulated data above threshhold"), stage.avg() == 1,
                   stage.avg(), stage.sum);
        }
    }

    {  // detector
        // The detector will behave differently depending on the number of bins.
        // If there are 1000 bins (== one bin per sample) it will just call phase_binning.
        // If therea are 100 bins (== 10 samples per bin) it will average 10 samples
        // and then dispatch to phase_binning.

todo();
    }
}

void test_Demodulator() {
    // Attention: depending on the resolution (100 or 1000 samples) the implemented
    //            logic is different --> both must be tested

    using namespace Internal;

    test_Demodulator_internal<Configuration_T<false> >();
    test_Demodulator_internal<Configuration_T<true> >();
}

void test_Binning() {
    using namespace Internal;
    const uint8_t number_of_bins = 8;

    {   // setup
        Binning::bins_t<uint8_t, uint8_t, number_of_bins> bins;
        bins.setup();
        for (uint8_t idx=0; idx < number_of_bins; ++idx) {
            assert(F("setup wipes data"), bins.data[idx] == 0, bins.data[idx]);
        }
        assert(F("setup clears max"), bins.signal_max == 0, bins.signal_max);
        assert(F("setup clears max_index"), bins.signal_max_index == number_of_bins+1, bins.signal_max_index);
        assert(F("setup clears noise max"), bins.noise_max == 0, bins.noise_max);
    }

    {   // advance_ticks
        Binning::bins_t<uint8_t, uint8_t, number_of_bins> bins;
        bins.setup();

        bins.tick = 0;
        bins.advance_tick();
        assert(F("advance_tick 0"), bins.tick == 1, bins.tick);

        bins.tick = number_of_bins-2;
        bins.advance_tick();
        assert(F("advance_tick number_of_bins-2"), bins.tick == number_of_bins-1, bins.tick);

        bins.tick = number_of_bins-1;
        bins.advance_tick();
        assert(F("advance_tick number_of_bins-1"), bins.tick == 0, bins.tick);
    }

    {    // compute_max_index
        Binning::Decoder<uint8_t, number_of_bins> bins;
        bins.setup();

        bins.data[0] = 1;
        bins.compute_max_index();
        assert(F("compute max 0"), bins.signal_max == 1 && bins.noise_max == 0 && bins.signal_max_index == 0,
                 bins.signal_max, bins.signal_max, bins.noise_max, bins.signal_max_index);

        bins.data[number_of_bins-1] = 2;
        bins.compute_max_index();
        assert(F("compute max 1"), bins.signal_max == 2 && bins.noise_max == 1 && bins.signal_max_index == number_of_bins-1,
                 bins.signal_max, bins.signal_max, bins.noise_max, bins.signal_max_index);

        bins.data[1] = 3;
        bins.compute_max_index();
        assert(F("compute max 2"), bins.signal_max == 3 && bins.noise_max == 2 && bins.signal_max_index == 1,
                 bins.signal_max, bins.signal_max, bins.noise_max, bins.signal_max_index);
    }

    {   // get_time_value
        const uint8_t number_of_bins = 10;
        Binning::Decoder<uint8_t, number_of_bins> bins;
        bins.setup();
        BCD::bcd_t time;

        bins.data[0] = 1;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F("below threshhold time is invalid"), time.val == (uint8_t)-1, time.val) ;

        bins.data[0] = 2;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F("max in first bin"), time.val == 1, time.val);

        bins.data[number_of_bins-1] = 20;
        bins.compute_max_index();
        assert(F("max in last bin"), time.val == 1, time.val);

        bins.data[number_of_bins-2] = 40;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F("max in last bin-1"),time.val == number_of_bins-1), time.val;

        bins.data[1] = 50;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F("max in second bin"), time.val == 2, time.val);

        bins.advance_tick();
        time = bins.get_time_value();
        assert(F("max in second bin plus advanced tick"), time.val == 3, time.val);
    }

    {   // get_time_value
        const uint8_t number_of_bins = 60;
        Binning::Decoder<uint8_t, number_of_bins> bins;
        BCD::bcd_t time;

        bins.setup();

        bins.data[number_of_bins-2] = 40;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F("max in last bin-1, proper BCD conversion"), time.val == 0x59, time.val);
    }

    {   // get_time_value with offset
    const uint8_t number_of_bins = 100;
        Binning::Decoder<uint8_t, number_of_bins> bins;
        bins.setup();
        BCD::bcd_t time;

        bins.data[0] = 1;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F(".. below threshhold time is invalid"), time.val == (uint8_t)-1, time.val);

        bins.data[0] = 2;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F(".. max in first bin"), time.val == 2, time.val);

        bins.data[number_of_bins-1] = 20;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F(".. max in last bin"), time.val == 1, time.val);

        bins.data[number_of_bins-2] = 40;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F(".. max in last bin-1"), time.val == 0xa0, time.val);

        bins.data[1] = 50;
        bins.compute_max_index();
        time = bins.get_time_value();
        assert(F(".. max in second bin"), time.val == 3, time.val);

        bins.advance_tick();
        time = bins.get_time_value();
        assert(F(".. max in second bin plus advanced tick"), time.val == 4, time.val);
    }

    {   // get_quality
        Binning::Decoder<uint8_t, number_of_bins> bins;
        bins.setup();
        Binning::lock_quality_tt<uint8_t> lq;

        bins.compute_max_index();
        bins.get_quality(lq);
        assert(F("lock quality 0 0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        bins.data[0] = 1;
        bins.compute_max_index();
        bins.get_quality(lq);
        assert(F("lock quality 1 0"), lq.lock_max == 1 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        bins.data[1] = 2;
        bins.compute_max_index();
        bins.get_quality(lq);
        assert(F("lock quality 2 1"), lq.lock_max == 2 && lq.noise_max == 1,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // get_quality_factor
        Binning::Decoder<uint8_t, number_of_bins> bins;
        bins.setup();

        bins.data[0] = 10;
        bins.data[1] = 255;
        bins.compute_max_index();
        assert(F("quality_factor 255-10 --> 34"), bins.get_quality_factor() == 34, bins.get_quality_factor());
    }

    {   // debug
        // no test as this produces only side effects
    }
}

void test_Minute_Decoder() {
    using namespace Internal;
    {   // test all 0
        DCF77_Minute_Decoder Minute_Decoder;

        Minute_Decoder.setup();

        assert(F("defaults to -1"), Minute_Decoder.get_time_value().val == 0xff,
                 Minute_Decoder.get_time_value().val);

        for (uint8_t second = 0; second < 60; ++second) {
            Minute_Decoder.process_tick(second, 0);
        }

        assert(F("locks to minute 0"), Minute_Decoder.get_time_value().val == 0,
                 Minute_Decoder.get_time_value().val);

        Binning::lock_quality_tt<uint8_t> lq;
        Minute_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Minute_Decoder.advance_tick();
        assert(F("advances to minute 1"), Minute_Decoder.get_time_value().val == 1,
                 Minute_Decoder.get_time_value().val);

        for (uint8_t minute = 1; minute < 59; ++minute) {
            Minute_Decoder.advance_tick();
        }
        assert(F("advances to minute 59"), Minute_Decoder.get_time_value().val == 0x59,
                 Minute_Decoder.get_time_value().val);
        Minute_Decoder.advance_tick();
        assert(F("advances to minute 0"), Minute_Decoder.get_time_value().val == 0,
                 Minute_Decoder.get_time_value().val);


        Minute_Decoder.setup();
        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Minute_Decoder.process_tick(second, 0);
            }
            Minute_Decoder.advance_tick();
        }
        Minute_Decoder.get_quality(lq);
        assert(F("quality 20-18"), lq.lock_max == 20 && lq.noise_max == 18,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        for (uint8_t second = 0; second < 60; ++second) {
            Minute_Decoder.process_tick(second, 0);
        }
        Minute_Decoder.get_quality(lq);
        assert(F("quality 22-20"), lq.lock_max == 22 && lq.noise_max == 20,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Minute_Decoder.setup();
        for (int8_t minute = 0; minute < 60; ++minute) {
            for (uint8_t second = 0; second < 60; ++second) {
                Minute_Decoder.process_tick(second, 0);
            }
            Minute_Decoder.advance_tick();
        }
        assert(F("60 minutes all 0"), Minute_Decoder.get_time_value().val == (uint8_t)-1,
                 Minute_Decoder.get_time_value().val);
        Minute_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 1
        DCF77_Minute_Decoder Minute_Decoder;
        Minute_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Minute_Decoder.process_tick(second, 1);
        }

        assert(F("fails to lock on all 1"), Minute_Decoder.get_time_value().val == (uint8_t)-1,
                 Minute_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Minute_Decoder.get_quality(lq);
        assert(F("quality 6-6"), lq.lock_max == 6 && lq.noise_max == 6,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        Minute_Decoder.setup();
        for (int8_t minute = 0; minute < 60; ++minute) {
            for (uint8_t second = 0; second < 60; ++second) {
                Minute_Decoder.process_tick(second, 1);
            }
            Minute_Decoder.advance_tick();
        }
        assert(F("60 minutes all 1"), Minute_Decoder.get_time_value().val == (uint8_t)-1,
                 Minute_Decoder.get_time_value().val);
        Minute_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 50:50
        DCF77_Minute_Decoder Minute_Decoder;
        Minute_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Minute_Decoder.process_tick(second, 0);
        }
        for (uint8_t second = 0; second < 60; ++second) {
            Minute_Decoder.process_tick(second, 1);
        }

        bool ok = true;
        for (uint8_t second = 0; second < 60; ++second) {
            ok = ok && Minute_Decoder.data[second] == 0;
        }

        assert(F("50:50 fills bins with 0"), ok);
        assert(F("fails to lock on 50:50"), Minute_Decoder.get_time_value().val == (uint8_t)-1,
                 Minute_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Minute_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

    }

    {   // test even bits
        DCF77_Minute_Decoder Minute_Decoder;
        Minute_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Minute_Decoder.process_tick(second, 1-(second & 1));
        }

        assert(F("fails to lock on even bits"), Minute_Decoder.get_time_value().val == (uint8_t)-1);
        Binning::lock_quality_tt<uint8_t> lq;
        Minute_Decoder.get_quality(lq);
        assert(F("quality 6-6"), lq.lock_max == 6 && lq.noise_max == 6,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test odd bits
        DCF77_Minute_Decoder Minute_Decoder;
        Minute_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Minute_Decoder.process_tick(second, second & 1);
        }

        assert(F("lock to minute 55 on odd bits"), Minute_Decoder.get_time_value().val == (uint8_t)0x55,
                 Minute_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Minute_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // 10 times 13 followed by 10 times 14
        DCF77_Minute_Decoder Minute_Decoder;
        Minute_Decoder.setup();

        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Minute_Decoder.process_tick(second,
                                            second == 21 ? 1 :
                                            second == 22 ? 1 :
                                            second == 23 ? 0 :
                                            second == 24 ? 0 :
                                            second == 25 ? 1 :
                                            second == 26 ? 0 :
                                            second == 27 ? 0 :
                                            second == 28 ? 1 :
                                            0);
            }
        }
        assert(F("properly lock to 13"), Minute_Decoder.get_time_value().val == (uint8_t)0x13,
                 Minute_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Minute_Decoder.get_quality(lq);
        assert(F("quality 60-40"), lq.lock_max == 60 && lq.noise_max == 40,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        for (uint8_t pass = 0; pass < 11; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Minute_Decoder.process_tick(second,
                                            second == 21 ? 0 :
                                            second == 22 ? 0 :
                                            second == 23 ? 1 :
                                            second == 24 ? 0 :
                                            second == 25 ? 1 :
                                            second == 26 ? 0 :
                                            second == 27 ? 0 :
                                            second == 28 ? 0 :
                                            0);
            }
        }
        assert(F("properly lock to 14 after 13"), Minute_Decoder.get_time_value().val == (uint8_t)0x14,
                 Minute_Decoder.get_time_value().val);
        Minute_Decoder.get_quality(lq);
        assert(F("quality 66-64"), lq.lock_max == 66 && lq.noise_max == 64,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }
}

void test_Hour_Decoder() {
    using namespace Internal;
    {   // test all 0
        DCF77_Hour_Decoder Hour_Decoder;

        Hour_Decoder.setup();

        assert(F("defaults to -1"), Hour_Decoder.get_time_value().val == 0xff,
                 Hour_Decoder.get_time_value().val);

        for (uint8_t second = 0; second < 60; ++second) {
            Hour_Decoder.process_tick(second, 0);
        }

        assert(F("locks to hour 0"), Hour_Decoder.get_time_value().val == 0,
                 Hour_Decoder.get_time_value().val);

        Binning::lock_quality_tt<uint8_t> lq;
        Hour_Decoder.get_quality(lq);
        assert(F("quality 4-2"), lq.lock_max == 4 && lq.noise_max == 2,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Hour_Decoder.advance_tick();
        assert(F("advances to hour 1"), Hour_Decoder.get_time_value().val == 1,
                 Hour_Decoder.get_time_value().val);

        for (uint8_t hour = 1; hour < 23; ++hour) {
            Hour_Decoder.advance_tick();
        }
        assert(F("advances to hour 23"), Hour_Decoder.get_time_value().val == 0x23,
                 Hour_Decoder.get_time_value().val);
        Hour_Decoder.advance_tick();
        assert(F("advances to hour 0"), Hour_Decoder.get_time_value().val == 0,
                 Hour_Decoder.get_time_value().val);


        Hour_Decoder.setup();
        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Hour_Decoder.process_tick(second, 0);
            }
            Hour_Decoder.advance_tick();
        }
        Hour_Decoder.get_quality(lq);
        assert(F("quality 10-10"), lq.lock_max == 10 && lq.noise_max == 10,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        for (uint8_t second = 0; second < 60; ++second) {
            Hour_Decoder.process_tick(second, 0);
        }
        Hour_Decoder.get_quality(lq);
        assert(F("quality 12-12"), lq.lock_max == 12 && lq.noise_max == 12,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Hour_Decoder.setup();
        for (int8_t hour = 0; hour < 24; ++hour) {
            for (uint8_t second = 0; second < 60; ++second) {
                Hour_Decoder.process_tick(second, 0);
            }
            Hour_Decoder.advance_tick();
        }
        assert(F("24 hours all 0"), Hour_Decoder.get_time_value().val == (uint8_t)-1,
                 Hour_Decoder.get_time_value().val);
        Hour_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

    }

    {   // test all 1
        DCF77_Hour_Decoder Hour_Decoder;
        Hour_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Hour_Decoder.process_tick(second, 1);
        }
        assert(F("fails to lock on all 1"), Hour_Decoder.get_time_value().val == (uint8_t)-1,
                 Hour_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Hour_Decoder.get_quality(lq);
        assert(F("quality 4-4"), lq.lock_max == 4 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        Hour_Decoder.setup();
        for (int8_t hour = 0; hour < 24; ++hour) {
            for (uint8_t second = 0; second < 60; ++second) {
                Hour_Decoder.process_tick(second, 1);
            }
            Hour_Decoder.advance_tick();
        }

        assert(F("24 hours all 1"), Hour_Decoder.get_time_value().val == (uint8_t)-1,
                 Hour_Decoder.get_time_value().val);
        Hour_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 50:50
        DCF77_Hour_Decoder Hour_Decoder;
        Hour_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Hour_Decoder.process_tick(second, 0);
        }
        for (uint8_t second = 0; second < 60; ++second) {
            Hour_Decoder.process_tick(second, 1);
        }

        bool ok = true;
        for (uint8_t hour = 0; hour < 24; ++hour) {
            ok = ok && Hour_Decoder.data[hour] == 0;
        }
        assert(F("50:50 fills bins with 0"), ok);
        assert(F("fails to lock on 50:50"), Hour_Decoder.get_time_value().val == (uint8_t)-1,
                 Hour_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Hour_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test even bits
        DCF77_Hour_Decoder Hour_Decoder;
        Hour_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Hour_Decoder.process_tick(second, 1-(second & 1));
        }

        assert(F("lock to hour 22 on even bits"), Hour_Decoder.get_time_value().val == 0x22,
                 Hour_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Hour_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test odd bits
        DCF77_Hour_Decoder Hour_Decoder;
        Hour_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Hour_Decoder.process_tick(second, second & 1);
        }
        assert(F("lock to hour 15 on odd bits"), Hour_Decoder.get_time_value().val == (uint8_t)0x15,
                 Hour_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Hour_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }
}

void test_Day_Decoder() {
    using namespace Internal;
    {   // test all 0
        DCF77_Day_Decoder Day_Decoder;

        Day_Decoder.setup();

        assert(F("defaults to -1"), Day_Decoder.get_time_value().val == 0xff,
                 Day_Decoder.get_time_value().val);

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Day_Decoder.process_tick(second, second == 36);
            }
        }

        assert(F("locks to day 1"), Day_Decoder.get_time_value().val == 1,
                 Day_Decoder.get_time_value().val);

        Binning::lock_quality_tt<uint8_t> lq;
        Day_Decoder.get_quality(lq);
        assert(F("quality 8-6"), lq.lock_max == 8 && lq.noise_max == 6,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Day_Decoder.advance_tick();
        assert(F("advances to day 2"), Day_Decoder.get_time_value().val == 2,
                 Day_Decoder.get_time_value().val);

        for (uint8_t day = 2; day < 31; ++day) {
            Day_Decoder.advance_tick();
        }

        assert(F("advances to day 31"), Day_Decoder.get_time_value().val == 0x31,
                 Day_Decoder.get_time_value().val);
        Day_Decoder.advance_tick();
        assert(F("advances to day 1"), Day_Decoder.get_time_value().val == 1,
                 Day_Decoder.get_time_value().val);


        Day_Decoder.setup();
        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Day_Decoder.process_tick(second, 0);
            }
            Day_Decoder.advance_tick();
        }
        Day_Decoder.get_quality(lq);
        assert(F("quality 11-10"), lq.lock_max == 11 && lq.noise_max == 10,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        for (uint8_t second = 0; second < 60; ++second) {
            Day_Decoder.process_tick(second, 0);
        }
        Day_Decoder.get_quality(lq);
        assert(F("quality 11-10"), lq.lock_max == 11 && lq.noise_max == 10,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Day_Decoder.setup();
        for (int8_t day = 0; day < 31; ++day) {
            for (uint8_t second = 0; second < 60; ++second) {
                Day_Decoder.process_tick(second, 0);
            }
            Day_Decoder.advance_tick();
        }
        assert(F("31 days all 0"), Day_Decoder.get_time_value().val == (uint8_t)-1,
                 Day_Decoder.get_time_value().val);
        Day_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 1
        DCF77_Day_Decoder Day_Decoder;
        Day_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Day_Decoder.process_tick(second, 1);
        }
        assert(F("fails to lock on all 1"), Day_Decoder.get_time_value().val == (uint8_t)-1,
                 Day_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Day_Decoder.get_quality(lq);
        assert(F("quality 3-3"), lq.lock_max == 3 && lq.noise_max == 3,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        Day_Decoder.setup();
        for (int8_t day = 0; day < 31; ++day) {
            for (uint8_t second = 0; second < 60; ++second) {
                Day_Decoder.process_tick(second, 1);
            }
            Day_Decoder.advance_tick();
        }

        assert(F("31 days all 1"), Day_Decoder.get_time_value().val == (uint8_t)-1,
                 Day_Decoder.get_time_value().val);
        Day_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 50:50
        DCF77_Day_Decoder Day_Decoder;
        Day_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Day_Decoder.process_tick(second, 0);
        }
        for (uint8_t second = 0; second < 60; ++second) {
            Day_Decoder.process_tick(second, 1);
        }

        bool ok = true;
        for (uint8_t day = 0; day < 24; ++day) {
            ok = ok && Day_Decoder.data[day] == 0;
        }
        assert(F("50:50 fills bins with 0"), ok);
        assert(F("fails to lock on 50:50"), Day_Decoder.get_time_value().val == (uint8_t)-1,
                 Day_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Day_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test even bits
        DCF77_Day_Decoder Day_Decoder;
        Day_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Day_Decoder.process_tick(second, 1-(second & 1));
            }
        }
        assert(F("lock to day 15 on even bits"), Day_Decoder.get_time_value().val == 0x15,
                 Day_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Day_Decoder.get_quality(lq);
        assert(F("quality 10-8"), lq.lock_max == 10 && lq.noise_max == 8,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test odd bits
        DCF77_Day_Decoder Day_Decoder;
        Day_Decoder.setup();
        for (uint8_t second = 0; second < 60; ++second) {
            Day_Decoder.process_tick(second, second & 1);
        }
        assert(F("fail to lock on odd bits"), Day_Decoder.get_time_value().val == (uint8_t)-1,
                 Day_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Day_Decoder.get_quality(lq);
        assert(F("quality 5-5"), lq.lock_max == 5 && lq.noise_max == 5,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }
}

void test_Month_Decoder() {
    using namespace Internal;
    {   // test all 0
        DCF77_Month_Decoder Month_Decoder;

        Month_Decoder.setup();

        assert(F("defaults to -1"), Month_Decoder.get_time_value().val == 0xff,
                 Month_Decoder.get_time_value().val);

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Month_Decoder.process_tick(second, second == 45);
            }
        }

        assert(F("locks to month 1"), Month_Decoder.get_time_value().val == 1,
                 Month_Decoder.get_time_value().val);

        Binning::lock_quality_tt<uint8_t> lq;
        Month_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Month_Decoder.advance_tick();
        assert(F("advances to month 2"), Month_Decoder.get_time_value().val == 2,
                 Month_Decoder.get_time_value().val);

        for (uint8_t month = 2; month < 12; ++month) {
            Month_Decoder.advance_tick();
        }

        assert(F("advances to month 12"), Month_Decoder.get_time_value().val == 0x12,
                 Month_Decoder.get_time_value().val);
        Month_Decoder.advance_tick();
        assert(F("advances to month 1"), Month_Decoder.get_time_value().val == 1,
                 Month_Decoder.get_time_value().val);


        Month_Decoder.setup();
        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Month_Decoder.process_tick(second, 0);
            }
            Month_Decoder.advance_tick();
        }
        Month_Decoder.get_quality(lq);
        assert(F("quality 3-2"), lq.lock_max == 3 && lq.noise_max == 2,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        for (uint8_t second = 0; second < 60; ++second) {
            Month_Decoder.process_tick(second, 0);
        }
        Month_Decoder.get_quality(lq);
        assert(F("quality 2-1"), lq.lock_max == 2 && lq.noise_max == 1,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Month_Decoder.setup();
        for (int8_t month = 0; month < 12; ++month) {
            for (uint8_t second = 0; second < 60; ++second) {
                Month_Decoder.process_tick(second, 0);
            }
            Month_Decoder.advance_tick();
        }
        assert(F("12 months all 0"), Month_Decoder.get_time_value().val == (uint8_t)-1,
                 Month_Decoder.get_time_value().val);
        Month_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 1
        DCF77_Month_Decoder Month_Decoder;
        Month_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Month_Decoder.process_tick(second, 1);
            }
        }
        assert(F("fails to lock on all 1"), Month_Decoder.get_time_value().val == (uint8_t)7,
                 Month_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Month_Decoder.get_quality(lq);
        assert(F("quality 4-2"), lq.lock_max == 4 && lq.noise_max == 2,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        Month_Decoder.setup();
        for (int8_t month = 0; month < 12; ++month) {
            for (uint8_t second = 0; second < 60; ++second) {
                Month_Decoder.process_tick(second, 1);
            }
            Month_Decoder.advance_tick();
        }

        assert(F("12 months all 1"), Month_Decoder.get_time_value().val == (uint8_t)-1,
                 Month_Decoder.get_time_value().val);
        Month_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 50:50
        DCF77_Month_Decoder Month_Decoder;
        Month_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Month_Decoder.process_tick(second, 0);
        }
        for (uint8_t second = 0; second < 60; ++second) {
            Month_Decoder.process_tick(second, 1);
        }

        bool ok = true;
        for (uint8_t month = 0; month < 12; ++month) {
            ok = ok && Month_Decoder.data[month] == 0;
        }
        assert(F("50:50 fills bins with 0"), ok);
        assert(F("fails to lock on 50:50"), Month_Decoder.get_time_value().val == (uint8_t)-1,
                 Month_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Month_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test even bits
        DCF77_Month_Decoder Month_Decoder;
        Month_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Month_Decoder.process_tick(second, 1-(second & 1));
            }
        }
        assert(F("fails to lock on even bits"), Month_Decoder.get_time_value().val == (uint8_t)-1,
                 Month_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Month_Decoder.get_quality(lq);
        assert(F("quality 6-6"), lq.lock_max == 6 && lq.noise_max == 6,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

    }

    {   // test odd bits
        DCF77_Month_Decoder Month_Decoder;
        Month_Decoder.setup();
        for (uint8_t second = 0; second < 60; ++second) {
            Month_Decoder.process_tick(second, second & 1);
        }
        assert(F("fail to lock on odd bits"), Month_Decoder.get_time_value().val == (uint8_t)-1,
                 Month_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Month_Decoder.get_quality(lq);
        assert(F("quality 3-3"), lq.lock_max == 3 && lq.noise_max == 3,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }
}


void test_Year_Decoder() {
    using namespace Internal;
    {   // test all 0
        DCF77_Year_Decoder Year_Decoder;

        Year_Decoder.setup();

        assert(F("defaults to -1"), Year_Decoder.get_time_value().val == 0xff,
                 Year_Decoder.get_time_value().val);

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Year_Decoder.process_tick(second, 0);
            }
        }

        assert(F("locks to year 0"), Year_Decoder.get_time_value().val == 0,
                 Year_Decoder.get_time_value().val);

        Binning::lock_quality_tt<uint8_t> lq;
        Year_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Year_Decoder.advance_tick();
        assert(F("advances to year 1"), Year_Decoder.get_time_value().val == 1,
                 Year_Decoder.get_time_value().val);

        for (uint8_t year = 1; year < 99; ++year) {
            Year_Decoder.advance_tick();
        }

        assert(F("advances to year 99"), Year_Decoder.get_time_value().val == 0x99,
                 Year_Decoder.get_time_value().val);
        Year_Decoder.advance_tick();
        assert(F("advances to year 0"), Year_Decoder.get_time_value().val == 0,
                 Year_Decoder.get_time_value().val);


        Year_Decoder.setup();
        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Year_Decoder.process_tick(second, 0);
            }
            Year_Decoder.advance_tick();
        }
        Year_Decoder.get_quality(lq);
        assert(F("quality 30-30"), lq.lock_max == 30 && lq.noise_max == 30,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        for (uint8_t second = 0; second < 60; ++second) {
            Year_Decoder.process_tick(second, 0);
        }

        Year_Decoder.get_quality(lq);
        assert(F("quality 33-32"), lq.lock_max == 33 && lq.noise_max == 32,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Year_Decoder.setup();
        for (int8_t year = 0; year < 100; ++year) {
            for (uint8_t second = 0; second < 60; ++second) {
                Year_Decoder.process_tick(second, 0);
            }
            Year_Decoder.advance_tick();
        }
        assert(F("100 years all 0"), Year_Decoder.get_time_value().val == (uint8_t)-1,
                 Year_Decoder.get_time_value().val);
        Year_Decoder.get_quality(lq);
        assert(F("quality 248-248"), lq.lock_max == 248 && lq.noise_max == 248,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 1
        DCF77_Year_Decoder Year_Decoder;
        Year_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Year_Decoder.process_tick(second, 1);
            }
        }
        assert(F("locks to 77 on  all 1"), Year_Decoder.get_time_value().val == 0x77,
                 Year_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Year_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        Year_Decoder.setup();
        for (int8_t year = 0; year < 100; ++year) {
            for (uint8_t second = 0; second < 60; ++second) {
                Year_Decoder.process_tick(second, 1);
            }
            Year_Decoder.advance_tick();
        }

        assert(F("100 years all 1"), Year_Decoder.get_time_value().val == (uint8_t)-1,
                 Year_Decoder.get_time_value().val);
        Year_Decoder.get_quality(lq);
        assert(F("quality 248-248"), lq.lock_max == 248 && lq.noise_max == 248,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 50:50
        DCF77_Year_Decoder Year_Decoder;
        Year_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Year_Decoder.process_tick(second, 0);
        }
        for (uint8_t second = 0; second < 60; ++second) {
            Year_Decoder.process_tick(second, 1);
        }

        bool ok = true;
        for (uint8_t year = 0; year < 10; ++year) {
            ok = ok && Year_Decoder.data[year] == 0;
        }
        for (uint8_t decade = 0; decade < 10; ++decade) {
            ok = ok && Year_Decoder.Decade_Decoder.data[decade] == 0;
        }
        assert(F("50:50 fills bins with 0"), ok);
        assert(F("fails to lock on 50:50"), Year_Decoder.get_time_value().val == (uint8_t)-1,
                 Year_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Year_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test even bits
        DCF77_Year_Decoder Year_Decoder;
        Year_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Year_Decoder.process_tick(second, 1-(second & 1));
            }
        }
        assert(F("lock to 55 on even bits"), Year_Decoder.get_time_value().val == 0x55,
                 Year_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Year_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test odd bits
        DCF77_Year_Decoder Year_Decoder;
        Year_Decoder.setup();
        for (uint8_t second = 0; second < 60; ++second) {
            Year_Decoder.process_tick(second, second & 1);
        }
        assert(F("fail to lock on odd bits"), Year_Decoder.get_time_value().val == (uint8_t)-1,
                 Year_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Year_Decoder.get_quality(lq);
        assert(F("quality 3-3"), lq.lock_max == 3 && lq.noise_max == 3,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }
}


void test_Decade_Decoder() {
    using namespace Internal;
    {   // test all 0
        DCF77_Decade_Decoder Decade_Decoder;

        Decade_Decoder.setup();

        assert(F("defaults to -1"), Decade_Decoder.get_time_value().val == 0xff,
                 Decade_Decoder.get_time_value().val);

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Decade_Decoder.process_tick(second, 0);
            }
        }

        assert(F("locks to decade 0"), Decade_Decoder.get_time_value().val == 0,
                 Decade_Decoder.get_time_value().val);

        Binning::lock_quality_tt<uint8_t> lq;
        Decade_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Decade_Decoder.advance_tick();
        assert(F("advances to decade 1"), Decade_Decoder.get_time_value().val == 1,
                 Decade_Decoder.get_time_value().val);

        for (uint8_t decade = 1; decade < 9; ++decade) {
            Decade_Decoder.advance_tick();
        }

        assert(F("advances to decade 9"), Decade_Decoder.get_time_value().val == 9,
                 Decade_Decoder.get_time_value().val);
        Decade_Decoder.advance_tick();
        assert(F("advances to decade 0"), Decade_Decoder.get_time_value().val == 0,
                 (uint8_t) Decade_Decoder.get_time_value().val);


        Decade_Decoder.setup();
        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Decade_Decoder.process_tick(second, 0);
            }
            Decade_Decoder.advance_tick();
        }
        Decade_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        for (uint8_t second = 0; second < 60; ++second) {
            Decade_Decoder.process_tick(second, 0);
        }

        Decade_Decoder.get_quality(lq);
        assert(F("quality 3-2"), lq.lock_max == 3 && lq.noise_max == 2,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Decade_Decoder.setup();
        for (int8_t decade = 0; decade < 10; ++decade) {
            for (uint8_t second = 0; second < 60; ++second) {
                Decade_Decoder.process_tick(second, 0);
            }
            Decade_Decoder.advance_tick();
        }
        assert(F("10 decades all 0"), Decade_Decoder.get_time_value().val == (uint8_t)-1,
                 Decade_Decoder.get_time_value().val);
        Decade_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 1
        DCF77_Decade_Decoder Decade_Decoder;
        Decade_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Decade_Decoder.process_tick(second, 1);
            }
        }
        assert(F("locks to 7 on  all 1"), Decade_Decoder.get_time_value().val == 7,
                 Decade_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Decade_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        Decade_Decoder.setup();
        for (int8_t decade = 0; decade < 10; ++decade) {
            for (uint8_t second = 0; second < 60; ++second) {
                Decade_Decoder.process_tick(second, 1);
            }
            Decade_Decoder.advance_tick();
        }

        assert(F("10 decades all 1"), Decade_Decoder.get_time_value().val == (uint8_t)-1,
                 Decade_Decoder.get_time_value().val);
        Decade_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 50:50
        DCF77_Decade_Decoder Decade_Decoder;
        Decade_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Decade_Decoder.process_tick(second, 0);
        }
        for (uint8_t second = 0; second < 60; ++second) {
            Decade_Decoder.process_tick(second, 1);
        }

        bool ok = true;
        for (uint8_t decade = 0; decade < 10; ++decade) {
            ok = ok && Decade_Decoder.data[decade] == 0;
        }
        assert(F("50:50 fills bins with 0"), ok);
        assert(F("fails to lock on 50:50"), Decade_Decoder.get_time_value().val == (uint8_t)-1,
                 Decade_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Decade_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test even bits
        DCF77_Decade_Decoder Decade_Decoder;
        Decade_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Decade_Decoder.process_tick(second, 1-(second & 1));
            }
        }
        assert(F("lock to 5 on even bits"), Decade_Decoder.get_time_value().val == 5,
                 Decade_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Decade_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test odd bits
        DCF77_Decade_Decoder Decade_Decoder;
        Decade_Decoder.setup();
        for (uint8_t second = 0; second < 60; ++second) {
            Decade_Decoder.process_tick(second, second & 1);
        }
        assert(F("fail to lock on odd bits"), Decade_Decoder.get_time_value().val == (uint8_t)-1,
                 Decade_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Decade_Decoder.get_quality(lq);
        assert(F("quality 3-3"), lq.lock_max == 3 && lq.noise_max == 3,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }
}


void test_Weekday_Decoder() {
    using namespace Internal;
    {   // test all 0
        DCF77_Weekday_Decoder Weekday_Decoder;

        Weekday_Decoder.setup();

        assert(F("defaults to -1"), Weekday_Decoder.get_time_value().val == 0xff,
                 Weekday_Decoder.get_time_value().val);

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Weekday_Decoder.process_tick(second, second == 42);
            }
        }
        assert(F("locks to weekday 1"), Weekday_Decoder.get_time_value().val == 1,
                 Weekday_Decoder.get_time_value().val);

        Binning::lock_quality_tt<uint8_t> lq;
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Weekday_Decoder.advance_tick();
        assert(F("advances to weekday 2"), Weekday_Decoder.get_time_value().val == 2,
                 Weekday_Decoder.get_time_value().val);

        for (uint8_t weekday = 2; weekday < 7; ++weekday) {
            Weekday_Decoder.advance_tick();
        }
        assert(F("advances to weekday 7"), Weekday_Decoder.get_time_value().val == 7,
                 Weekday_Decoder.get_time_value().val);
        Weekday_Decoder.advance_tick();
        assert(F("advances to weekday 1"), Weekday_Decoder.get_time_value().val == 1,
                 Weekday_Decoder.get_time_value().val);


        Weekday_Decoder.setup();
        for (uint8_t pass = 0; pass < 10; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Weekday_Decoder.process_tick(second, 0);
            }
            Weekday_Decoder.advance_tick();
        }
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 3-3"), lq.lock_max == 3 && lq.noise_max == 3,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        for (uint8_t second = 0; second < 60; ++second) {
            Weekday_Decoder.process_tick(second, 0);
        }

        Weekday_Decoder.get_quality(lq);
        assert(F("quality 3-2"), lq.lock_max == 3 && lq.noise_max == 2,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);

        Weekday_Decoder.setup();
        for (int8_t weekday = 0; weekday < 7; ++weekday) {
            for (uint8_t second = 0; second < 60; ++second) {
                Weekday_Decoder.process_tick(second, 0);
            }
            Weekday_Decoder.advance_tick();
        }
        assert(F("7 days all 0"), Weekday_Decoder.get_time_value().val == (uint8_t)-1,
                 Weekday_Decoder.get_time_value().val);
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 1
        DCF77_Weekday_Decoder Weekday_Decoder;
        Weekday_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Weekday_Decoder.process_tick(second, 1);
            }
        }
        assert(F("locks to 7 on  all 1"), Weekday_Decoder.get_time_value().val == 7,
                 Weekday_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 4-2"), lq.lock_max == 4 && lq.noise_max == 2,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);


        Weekday_Decoder.setup();
        for (int8_t weekday = 0; weekday < 7; ++weekday) {
            for (uint8_t second = 0; second < 60; ++second) {
                Weekday_Decoder.process_tick(second, 1);
            }
            Weekday_Decoder.advance_tick();
        }

        assert(F("7 weekdays all 1"), Weekday_Decoder.get_time_value().val == (uint8_t)-1,
                 Weekday_Decoder.get_time_value().val);
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test all 50:50
        DCF77_Weekday_Decoder Weekday_Decoder;
        Weekday_Decoder.setup();

        for (uint8_t second = 0; second < 60; ++second) {
            Weekday_Decoder.process_tick(second, 0);
        }
        for (uint8_t second = 0; second < 60; ++second) {
            Weekday_Decoder.process_tick(second, 1);
        }

        bool ok = true;
        for (uint8_t weekday = 0; weekday < 7; ++weekday) {
            ok = ok && Weekday_Decoder.data[weekday] == 0;
        }
        assert(F("50:50 fills bins with 0"), ok);
        assert(F("fails to lock on 50:50"), Weekday_Decoder.get_time_value().val == (uint8_t)-1,
                 Weekday_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 0-0"), lq.lock_max == 0 && lq.noise_max == 0,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test even bits
        DCF77_Weekday_Decoder Weekday_Decoder;
        Weekday_Decoder.setup();

        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Weekday_Decoder.process_tick(second, 1-(second & 1));
            }
        }
        assert(F("lock to 5 on even bits"), Weekday_Decoder.get_time_value().val == 5,
                 Weekday_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }

    {   // test odd bits
        DCF77_Weekday_Decoder Weekday_Decoder;
        Weekday_Decoder.setup();
        for (uint8_t pass = 0; pass < 2; ++pass) {
            for (uint8_t second = 0; second < 60; ++second) {
                Weekday_Decoder.process_tick(second, second & 1);
            }
        }
        assert(F("lock to 2 on odd bits"), Weekday_Decoder.get_time_value().val == 2,
                 Weekday_Decoder.get_time_value().val);
        Binning::lock_quality_tt<uint8_t> lq;
        Weekday_Decoder.get_quality(lq);
        assert(F("quality 6-4"), lq.lock_max == 6 && lq.noise_max == 4,
                 lq.lock_max, lq.noise_max, lq.lock_max - lq.noise_max);
    }
}

void boilerplate() {
    Serial.println();
    Serial.print(F("Test compiled: "));
    Serial.println(F(__TIMESTAMP__));
}

void disable_tick_interrupts() {
    #if defined(__AVR__)
    TIMSK2 = 0;
    #else
    SysTick->CTRL = 0;
    #endif
}

void setup() {
    disable_tick_interrupts();
    Serial.begin(115000);
    boilerplate();

    test_BCD();
    todo(); //test_Clock()
    todo(); //test_DCF77_Clock()
    test_TMP();
    test_Arithmetic_Tools();

    test_DCF77_Encoder();
    todo(); //test_Convoluter() // todo, not urgent as implicitly tested with Demodulator
    todo(); //test_Binner() // todo, not urgent as implicitly tested with most decoders
    test_Demodulator();
    test_Flag_Decoder();
    test_Binning();

    test_Minute_Decoder();
    test_Hour_Decoder();
    test_Day_Decoder();
    test_Month_Decoder();
    test_Year_Decoder();
    test_Decade_Decoder();
    test_Weekday_Decoder();

    todo(); //test_Local_Clock()
    todo(); //test_Frequency_Control()
    todo(); //test_Clock_Controller()
    todo(); //test_Generic_1_kHz_Generator()

    ut::print_statistics();
}
void loop() {}
