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

// test unlocked / locked transitions
// test free handling
// once the clock is locked advancing the stages could be done by the local clock

namespace Debug {
    void debug_helper(char data) { Serial.print(data == 0? 'S': data == 1? '?': data - 2 + '0', 0); }

    void bcddigit(uint8_t data) {
        if (data <= 0x09) {
            Serial.print(data, HEX);
        } else {
            Serial.print('?');
        }
    }

    void bcddigits(uint8_t data) {
        bcddigit(data >>  4);
        bcddigit(data & 0xf);
    }
}

namespace BCD {
    void print(const bcd_t value) {
        Serial.print(value.val >> 4 & 0xF, HEX);
        Serial.print(value.val >> 0 & 0xF, HEX);
    }

    void increment(bcd_t &value) {
        if (value.digit.lo < 9) {
            ++value.digit.lo;
        } else {
            value.digit.lo = 0;

            if (value.digit.hi < 9) {
                ++value.digit.hi;
            } else {
                value.digit.hi = 0;
            }
        }
    }

    bcd_t int_to_bcd(const uint8_t value) {
        const uint8_t hi = value / 10;

        bcd_t result;
        result.digit.hi = hi;
        result.digit.lo = value-10*hi;

        return result;
    }

    uint8_t bcd_to_int(const bcd_t value) {
        return value.digit.lo + 10*value.digit.hi;
    }
}

namespace Arithmetic_Tools {
    template <uint8_t N> inline void bounded_increment(uint8_t &value) __attribute__((always_inline));
    template <uint8_t N>
    void bounded_increment(uint8_t &value) {
        if (value >= 255 - N) { value = 255; } else { value += N; }
    }

    template <uint8_t N> inline void bounded_decrement(uint8_t &value) __attribute__((always_inline));
    template <uint8_t N>
    void bounded_decrement(uint8_t &value) {
        if (value <= N) { value = 0; } else { value -= N; }
    }

    inline void bounded_add(uint8_t &value, const uint8_t amount) __attribute__((always_inline));
    void bounded_add(uint8_t &value, const uint8_t amount) {
        if (value >= 255-amount) { value = 255; } else { value += amount; }
    }

    inline void bounded_sub(uint8_t &value, const uint8_t amount) __attribute__((always_inline));
    void bounded_sub(uint8_t &value, const uint8_t amount) {
        if (value <= amount) { value = 0; } else { value -= amount; }
    }

    inline uint8_t bit_count(const uint8_t value) __attribute__((always_inline));
    uint8_t bit_count(const uint8_t value) {
        const uint8_t tmp1 = (value & 0b01010101) + ((value>>1) & 0b01010101);
        const uint8_t tmp2 = (tmp1  & 0b00110011) + ((tmp1>>2) & 0b00110011);
        return (tmp2 & 0x0f) + (tmp2>>4);
    }

    inline uint8_t parity(const uint8_t value) __attribute__((always_inline));
    uint8_t parity(const uint8_t value) {
        uint8_t tmp = value;

        tmp = (tmp & 0xf) ^ (tmp >> 4);
        tmp = (tmp & 0x3) ^ (tmp >> 2);
        tmp = (tmp & 0x1) ^ (tmp >> 1);

        return tmp;
    }

    void minimize(uint8_t &minimum, const uint8_t value) {
        if (value < minimum) {
            minimum = value;
        }
    }

    void maximize(uint8_t &maximum, const uint8_t value) {
        if (value > maximum) {
            maximum = value;
        }
    }

    uint8_t set_bit(const uint8_t data, const uint8_t number, const uint8_t value) {
        return value? data|(1<<number): data&~(1<<number);
    }
}

namespace Hamming {
    template <uint8_t significant_bits>
    void score (uint8_t &bin, const BCD::bcd_t input, const BCD::bcd_t candidate) {
        using namespace Arithmetic_Tools;

        const uint8_t the_score = significant_bits - bit_count(input.val ^ candidate.val);
        bounded_add(bin, the_score);
    }

    template <typename bins_t>
    void advance_tick(bins_t &bins) {
        const uint8_t number_of_bins = sizeof(bins.data) / sizeof(bins.data[0]);
        if (bins.tick < number_of_bins - 1) {
            ++bins.tick;
        } else {
            bins.tick = 0;
        }
    }

    template <typename bins_type, uint8_t significant_bits, bool with_parity>
    void hamming_binning(bins_type &bins, const BCD::bcd_t input) {
        using namespace Arithmetic_Tools;
        using namespace BCD;

        const uint8_t number_of_bins = sizeof(bins.data) / sizeof(bins.data[0]);

        if (bins.max > 255-significant_bits) {
            // If we know we can not raise the maximum any further we
            // will lower the noise floor instead.
            for (uint8_t bin_index = 0; bin_index <number_of_bins; ++bin_index) {
                bounded_decrement<significant_bits>(bins.data[bin_index]);
            }
            bins.max -= significant_bits;
            bounded_decrement<significant_bits>(bins.noise_max);
        }

        const uint8_t offset = number_of_bins-1-bins.tick;
        uint8_t bin_index = offset;
        // for minutes, hours have parity and start counting at 0
        // for days, weeks, month we have no parity and start counting at 1
        // for years and decades we have no parity and start counting at 0
        bcd_t candidate;
        candidate.val = (with_parity || number_of_bins == 10)? 0x00: 0x01;
        for (uint8_t pass=0; pass < number_of_bins; ++pass) {

            if (with_parity) {
                candidate.bit.b7 = parity(candidate.val);
                score<significant_bits>(bins.data[bin_index], input, candidate);
                candidate.bit.b7 = 0;
            } else {
                score<significant_bits>(bins.data[bin_index], input, candidate);
            }

            bin_index = bin_index < number_of_bins-1? bin_index+1: 0;
            increment(candidate);
        }
    }

    template <typename bins_t>
    void compute_max_index(bins_t &bins) {
        const uint8_t number_of_bins = sizeof(bins.data) / sizeof(bins.data[0]);

        bins.noise_max = 0;
        bins.max = 0;
        bins.max_index = 255;
        for (uint8_t index = 0; index < number_of_bins; ++index) {
            const uint8_t bin_data = bins.data[index];

            if (bin_data >= bins.max) {
                bins.noise_max = bins.max;
                bins.max = bin_data;
                bins.max_index = index;
            } else if (bin_data > bins.noise_max) {
                bins.noise_max = bin_data;
            }
        }
    }

    template <typename bins_t>
    void setup(bins_t &bins) {
        const uint8_t number_of_bins = sizeof(bins.data) / sizeof(bins.data[0]);

        for (uint8_t index = 0; index < number_of_bins; ++index) {
            bins.data[index] = 0;
        }
        bins.tick = 0;

        bins.max = 0;
        bins.max_index = 255;
        bins.noise_max = 0;
    }

    template <typename bins_t>
    BCD::bcd_t get_time_value(const bins_t &bins) {
        // there is a trade off involved here:
        //    low threshold --> lock will be detected earlier
        //    low threshold --> if lock is not clean output will be garbled
        //    a proper lock will fix the issue
        //    the question is: which start up behaviour do we prefer?
        const uint8_t threshold = 2;

        const uint8_t number_of_bins = sizeof(bins.data) / sizeof(bins.data[0]);
        const uint8_t offset = (number_of_bins == 60 || number_of_bins == 24 || number_of_bins == 10)? 0x00: 0x01;

        if (bins.max-bins.noise_max >= threshold) {
            return BCD::int_to_bcd((bins.max_index + bins.tick + 1) % number_of_bins + offset);
        } else {
            BCD::bcd_t undefined;
            undefined.val = 0xff;
            return undefined;
        }
    }

    template <typename bins_t>
    void get_quality(const bins_t bins, Hamming::lock_quality_t &lock_quality) {
        lock_quality.lock_max = bins.max;
        lock_quality.noise_max = bins.noise_max;
    }

    template <typename bins_t>
    uint8_t get_quality_factor(const bins_t bins) {
        if (bins.max <= bins.noise_max) {
            return 0;
        }

        const uint16_t delta = bins.max - bins.noise_max;
        // we define the quality factor as
        //   (delta) / ld (max + 3)

        // unfortunately this is prohibitive expensive to compute

        // --> we need some shortcuts
        // --> we will cheat a lot

        // lookup for ld(n):
        //   4 -->  2,  6 -->  2.5,   8 -->  3,  12 -->  3.5
        // above 16 --> only count the position of the leading digit

        if (bins.max >= 32-3) {
            // delta / ld(bins.max+3) ~ delta / ld(bins.max)
            uint16_t max = bins.max;
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
            return ((uint16_t)delta * multiplier) >> 8;

        } else if (bins.max >= 16-3) {
            // delta / 4
            return delta >> 2;

        } else if (bins.max >= 12-3) {
            // delta / 3.5
            // we know delta <= max < 16-3 = 13 --> delta <= 12
            return delta >= 11? 3:
                   delta >=  7? 2:
                   delta >=  4? 1:
                                0;

       } else if (bins.max >= 8-3) {
            // delta / 3
            // we know delta <= max < 12-3 = 9 --> delta <= 8
            return delta >= 6? 2:
                   delta >= 3? 1:
                               0;

        } else if (bins.max >= 6-3) {
            // delta / 2.5
            // we know delta <= max < 8-3 = 5 --> delta <= 4
            return delta >= 3? 1: 0;

        } else {  // if (bins.max >= 4-3) {
            // delta / 2
            return delta >> 1;
        }
    }

    template <typename bins_t>
    void debug (const bins_t &bins) {
        const uint8_t number_of_bins = sizeof(bins.data) / sizeof(bins.data[0]);
        const bool uses_integrals = sizeof(bins.max) == 4;

        Serial.print(get_time_value(bins).val, HEX);
        Serial.print(F(" Tick: "));
        Serial.print(bins.tick);
        Serial.print(F(" Quality: "));
        Serial.print(bins.max, DEC);
        Serial.print('-');
        Serial.print(bins.noise_max, DEC);
        Serial.print(F(" Max Index: "));
        Serial.print(bins.max_index, DEC);
        Serial.print(F(" Quality Factor: "));
        Serial.print(get_quality_factor(bins), DEC);
        Serial.print('>');

        for (uint8_t index = 0; index < number_of_bins; ++index) {
            if (index == bins.max_index ||
                (!uses_integrals && index == (bins.max_index+1) % number_of_bins) ||
                (uses_integrals && (index == (bins.max_index+10) % number_of_bins ||
                                   (index == (bins.max_index+20) % number_of_bins)))) {
                Serial.print('|');
            }
            Serial.print(bins.data[index],HEX);
        }
        Serial.println();
    }
}

namespace DCF77_Encoder {
    using namespace DCF77;

    inline uint8_t days_per_month(const DCF77::time_data_t &now) __attribute__((always_inline));
    uint8_t days_per_month(const DCF77::time_data_t &now) {
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

    void reset(DCF77::time_data_t &now) {
        now.second      = 0;
        now.minute.val  = 0x00;
        now.hour.val    = 0x00;
        now.day.val     = 0x01;
        now.month.val   = 0x01;
        now.year.val    = 0x00;
        now.weekday.val = 0x01;
        now.uses_summertime           = false;
        now.uses_backup_antenna       = false;
        now.timezone_change_scheduled = false;
        now.leap_second_scheduled     = false;

        now.undefined_minute_output                    = false;
        now.undefined_uses_summertime_output           = false;
        now.undefined_uses_backup_antenna_output       = false;
        now.undefined_timezone_change_scheduled_output = false;
    }

    uint8_t weekday(const DCF77::time_data_t &now) {  // attention: sunday will be ==0 instead of 7
        using namespace BCD;

        if (now.day.val <= 0x31 && now.month.val <= 0x12 && now.year.val <= 0x99) {
            // This will compute the weekday for each year in 2001-2099.
            // If you really plan to use my code beyond 2099 take care of this
            // on your own. My assumption is that it is even unclear if DCF77
            // will still exist then.

            // http://de.wikipedia.org/wiki/Gau%C3%9Fsche_Wochentagsformel
            const uint8_t  d = bcd_to_int(now.day);
            const uint16_t m = now.month.val <= 0x02? now.month.val + 10:
                                                      bcd_to_int(now.month) - 2;
            const uint8_t  y = bcd_to_int(now.year) - (now.month.val <= 0x02);
            // m must be of type uint16_t otherwise this will compute crap
            uint8_t day_mod_7 = d + (26*m - 2)/10 + y + y/4;
            // We exploit 8 mod 7 = 1
            while (day_mod_7 >= 7) {
                day_mod_7 -= 7;
                day_mod_7 = (day_mod_7 >> 3) + (day_mod_7 & 7);
            }

            return day_mod_7;  // attention: sunday will be == 0 instead of 7
        } else {
            return 0xff;
        }
    }

    BCD::bcd_t bcd_weekday(const DCF77::time_data_t &now) {
        BCD::bcd_t today;

        today.val = weekday(now);
        if (today.val == 0) {
            today.val = 7;
        }

        return today;
    }

    void autoset_weekday(DCF77::time_data_t &now) {
        now.weekday = bcd_weekday(now);
    }

    void autoset_timezone(DCF77::time_data_t &now) {
        // timezone change may only happen at the last sunday of march / october
        // the last sunday is always somewhere in [25-31]

        // Wintertime --> Summertime happens at 01:00 UTC == 02:00 CET == 03:00 CEST,
        // Summertime --> Wintertime happens at 01:00 UTC == 02:00 CET == 03:00 CEST

        if (now.month.val < 0x03) {
            // January or February
            now.uses_summertime = false;
        } else
        if (now.month.val == 0x03) {
            // March
            if (now.day.val < 0x25) {
                // Last Sunday of March must be 0x25-0x31
                // Thus still to early for summertime
                now.uses_summertime = false;
            } else
            if (uint8_t wd = weekday(now)) {
                // wd != 0 --> not a Sunday
                if (now.day.val - wd < 0x25) {
                    // early March --> wintertime
                    now.uses_summertime = false;
                } else {
                    // late march summertime
                    now.uses_summertime = true;
                }
            } else {
                // last sunday of march
                // decision depends on the current hour
                now.uses_summertime = (now.hour.val > 2);
            }
        } else
        if (now.month.val < 0x10) {
            // April - September
            now.uses_summertime = true;
        } else
        if (now.month.val == 0x10) {
            // October
            if (now.day.val < 0x25) {
                // early October
                now.uses_summertime = true;
            } else
            if (uint8_t wd = weekday(now)) {
                // wd != 0 --> not a Sunday
                if (now.day.val - wd < 0x25) {
                    // early October --> summertime
                    now.uses_summertime = true;
                } else {
                    // late October --> wintertime
                    now.uses_summertime = false;
                }
            } else {  // last sunday of october
                if (now.hour.val == 2) {
                    // can not derive the flag from time data
                    // this is the only time the flag is derived
                    // from the flag vector

                } else {
                    // decision depends on the current hour
                    now.uses_summertime = (now.hour.val < 2);
                }
            }
        } else {
            // November and December
            now.uses_summertime = false;
        }
    }

    void autoset_timezone_change_scheduled(DCF77::time_data_t &now) {
        // summer/wintertime change will always happen
        // at clearly defined hours
        // http://www.gesetze-im-internet.de/sozv/__2.html

        // in doubt have a look here: http://www.dcf77logs.de/
        if (now.day.val < 0x25 || weekday(now) != 0) {
            // timezone change may only happen at the last sunday of march / october
            // the last sunday is always somewhere in [25-31]

            // notice that undefined (==0xff) day/weekday data will not cause any action
            now.timezone_change_scheduled = false;
        } else {
            if (now.month.val == 0x03) {
                if (now.uses_summertime) {
                    now.timezone_change_scheduled = (now.hour.val == 0x03 && now.minute.val == 0x00); // wintertime to summertime, preparing first minute of summertime
                } else {
                    now.timezone_change_scheduled = (now.hour.val == 0x01 && now.minute.val != 0x00); // wintertime to summertime
                }
            } else if (now.month.val == 0x10) {
                if (now.uses_summertime) {
                    now.timezone_change_scheduled = (now.hour.val == 0x02 && now.minute.val != 0x00); // summertime to wintertime
                } else {
                    now.timezone_change_scheduled = (now.hour.val == 0x02 && now.minute.val == 0x00); // summertime to wintertime, preparing first minute of wintertime
                }
            } else if (now.month.val <= 0x12) {
                now.timezone_change_scheduled = false;
            }
        }
    }

    void verify_leap_second_scheduled(DCF77::time_data_t &now) {
        // If day or month are unknown we default to "no leap second" because this is alway a very good guess.
        // If we do not know for sure we are either acquiring a lock right now --> we will easily recover from a wrong guess
        // or we have very noisy data --> the leap second bit is probably noisy as well --> we should assume the most likely case

        now.leap_second_scheduled &= (now.day.val == 0x01);

        // leap seconds will always happen at 00:00 UTC == 01:00 CET == 02:00 CEST
        if (now.month.val == 0x01) {
            now.leap_second_scheduled &= ((now.hour.val == 0x00 && now.minute.val != 0x00) ||
                                          (now.hour.val == 0x01 && now.minute.val == 0x00));
        } else if (now.month.val == 0x07 || now.month.val == 0x04 || now.month.val == 0x10) {
            now.leap_second_scheduled &= ((now.hour.val == 0x01 && now.minute.val != 0x00) ||
                                          (now.hour.val == 0x02 && now.minute.val == 0x00));
        } else {
            now.leap_second_scheduled = false;
        }
    }

    void autoset_control_bits(DCF77::time_data_t &now) {
        autoset_weekday(now);
        autoset_timezone(now);
        autoset_timezone_change_scheduled(now);
        // we can not compute leap seconds, we can only verify if they might happen
        verify_leap_second_scheduled(now);
    }

    void advance_second(DCF77::time_data_t &now) {
        // in case some value is out of range it will not be advanced
        // this is on purpose
        if (now.second < 59) {
            ++now.second;
            if (now.second == 15) {
                autoset_control_bits(now);
            }

        } else if (now.leap_second_scheduled && now.second == 59 && now.minute.val == 0x00) {
            now.second = 60;
            now.leap_second_scheduled = false;
        } else if (now.second == 59 || now.second == 60) {
            now.second = 0;
            advance_minute(now);
        }
    }

    void advance_minute(DCF77::time_data_t &now) {
        if (now.minute.val < 0x59) {
            increment(now.minute);
        } else if (now.minute.val == 0x59) {
            now.minute.val = 0x00;
            // in doubt have a look here: http://www.dcf77logs.de/
            if (now.timezone_change_scheduled && !now.uses_summertime && now.hour.val == 0x01) {
                // Wintertime --> Summertime happens at 01:00 UTC == 02:00 CET == 03:00 CEST,
                // the clock must be advanced from 01:59 CET to 03:00 CEST
                increment(now.hour);
                increment(now.hour);
                now.uses_summertime = true;
            }  else if (now.timezone_change_scheduled && now.uses_summertime && now.hour.val == 0x02) {
                // Summertime --> Wintertime happens at 01:00 UTC == 02:00 CET == 03:00,
                // the clock must be advanced from 02:59 CEST to 02:00 CET
                now.uses_summertime = false;
            } else {
                if (now.hour.val < 0x23) {
                    increment(now.hour);
                } else if (now.hour.val == 0x23) {
                    now.hour.val = 0x00;

                    if (now.weekday.val < 0x07) {
                        increment(now.weekday);
                    } else if (now.weekday.val == 0x07) {
                        now.weekday.val = 0x01;
                    }

                    if (bcd_to_int(now.day) < days_per_month(now)) {
                        increment(now.day);
                    } else if (bcd_to_int(now.day) == days_per_month(now)) {
                        now.day.val = 0x01;

                        if (now.month.val < 0x12) {
                            increment(now.month);
                        } else if (now.month.val == 0x12) {
                            now.month.val = 0x01;

                            if (now.year.val < 0x99) {
                                increment(now.year);
                            } else if (now.year.val == 0x99) {
                                now.year.val = 0x00;
                            }
                        }
                    }
                }
            }
        }
    }

    DCF77::tick_t get_current_signal(const DCF77::time_data_t &now) {
        using namespace Arithmetic_Tools;

        if (now.second >= 1 && now.second <= 14) {
            // weather data or other stuff we can not compute
            return undefined;
        }

        bool result;
        switch (now.second) {
            case 0:  // start of minute
                return short_tick;

            case 15:
                if (now.undefined_uses_backup_antenna_output) { return undefined; }
                result = now.uses_backup_antenna; break;

            case 16:
                if (now.undefined_timezone_change_scheduled_output) { return undefined; }
                result = now.timezone_change_scheduled; break;

            case 17:
                if (now.undefined_uses_summertime_output) {return undefined; }
                result = now.uses_summertime; break;

            case 18:
                if (now.undefined_uses_summertime_output) {return undefined; }
                result = !now.uses_summertime; break;

            case 19:
                result = now.leap_second_scheduled; break;

            case 20:  // start of time information
                return long_tick;

            case 21:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = now.minute.digit.lo & 0x1; break;
            case 22:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = now.minute.digit.lo & 0x2; break;
            case 23:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = now.minute.digit.lo & 0x4; break;
            case 24:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = now.minute.digit.lo & 0x8; break;

            case 25:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = now.minute.digit.hi & 0x1; break;
            case 26:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = now.minute.digit.hi & 0x2; break;
            case 27:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = now.minute.digit.hi & 0x4; break;

            case 28:
                if (now.undefined_minute_output || now.minute.val > 0x59) { return undefined; }
                result = parity(now.minute.val); break;


            case 29:
                if (now.hour.val > 0x23) { return undefined; }
                result = now.hour.digit.lo & 0x1; break;
            case 30:
                if (now.hour.val > 0x23) { return undefined; }
                result = now.hour.digit.lo & 0x2; break;
            case 31:
                if (now.hour.val > 0x23) { return undefined; }
                result = now.hour.digit.lo & 0x4; break;
            case 32:
                if (now.hour.val > 0x23) { return undefined; }
                result = now.hour.digit.lo & 0x8; break;

            case 33:
                if (now.hour.val > 0x23) { return undefined; }
                result = now.hour.digit.hi & 0x1; break;
            case 34:
                if (now.hour.val > 0x23) { return undefined; }
                result = now.hour.digit.hi & 0x2; break;

            case 35:
                if (now.hour.val > 0x23) { return undefined; }
                result = parity(now.hour.val); break;

            case 36:
                if (now.day.val > 0x31) { return undefined; }
                result = now.day.digit.lo & 0x1; break;
            case 37:
                if (now.day.val > 0x31) { return undefined; }
                result = now.day.digit.lo & 0x2; break;
            case 38:
                if (now.day.val > 0x31) { return undefined; }
                result = now.day.digit.lo & 0x4; break;
            case 39:
                if (now.day.val > 0x31) { return undefined; }
                result = now.day.digit.lo & 0x8; break;

            case 40:
                if (now.day.val > 0x31) { return undefined; }
                result = now.day.digit.hi & 0x1; break;
            case 41:
                if (now.day.val > 0x31) { return undefined; }
                result = now.day.digit.hi & 0x2; break;

            case 42:
                if (now.weekday.val > 0x7) { return undefined; }
                result = now.weekday.val & 0x1; break;
            case 43:
                if (now.weekday.val > 0x7) { return undefined; }
                result = now.weekday.val & 0x2; break;
            case 44:
                if (now.weekday.val > 0x7) { return undefined; }
                result = now.weekday.val & 0x4; break;

            case 45:
                if (now.month.val > 0x12) { return undefined; }
                result = now.month.digit.lo & 0x1; break;
            case 46:
                if (now.month.val > 0x12) { return undefined; }
                result = now.month.digit.lo & 0x2; break;
            case 47:
                if (now.month.val > 0x12) { return undefined; }
                result = now.month.digit.lo & 0x4; break;
            case 48:
                if (now.month.val > 0x12) { return undefined; }
                result = now.month.digit.lo & 0x8; break;

            case 49:
                if (now.month.val > 0x12) { return undefined; }
                result = now.month.digit.hi & 0x1; break;

            case 50:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.lo & 0x1; break;
            case 51:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.lo & 0x2; break;
            case 52:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.lo & 0x4; break;
            case 53:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.lo & 0x8; break;

            case 54:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.hi & 0x1; break;
            case 55:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.hi & 0x2; break;
            case 56:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.hi & 0x4; break;
            case 57:
                if (now.year.val > 0x99) { return undefined; }
                result = now.year.digit.hi & 0x8; break;

            case 58:
                if (now.weekday.val > 0x07 ||
                    now.day.val     > 0x31 ||
                    now.month.val   > 0x12 ||
                    now.year.val    > 0x99) { return undefined; }

                result = parity(now.day.digit.lo)   ^
                         parity(now.day.digit.hi)   ^
                         parity(now.month.digit.lo) ^
                         parity(now.month.digit.hi) ^
                         parity(now.weekday.val)    ^
                         parity(now.year.digit.lo)  ^
                         parity(now.year.digit.hi); break;

            case 59:
                // special handling for leap seconds
                if (now.leap_second_scheduled && now.minute.val == 0) { result = 0; break; }
                // standard case: fall through to "sync_mark"
            case 60:
                return sync_mark;

            default:
                return undefined;
        }

        return result? long_tick: short_tick;
    }

    void get_serialized_clock_stream(const DCF77::time_data_t &now, DCF77::serialized_clock_stream &data) {
        using namespace Arithmetic_Tools;

        // bit 16-20  // flags
        data.byte_0 = 0;
        data.byte_0 = set_bit(data.byte_0, 3, now.timezone_change_scheduled );
        data.byte_0 = set_bit(data.byte_0, 4, now.uses_summertime);
        data.byte_0 = set_bit(data.byte_0, 5, !now.uses_summertime);
        data.byte_0 = set_bit(data.byte_0, 6, now.leap_second_scheduled);
        data.byte_0 = set_bit(data.byte_0, 7, 1);

        // bit 21-28  // minutes
        data.byte_1 = set_bit(now.minute.val, 7, parity(now.minute.val));

        // bit 29-36  // hours, bit 0 of day
        data.byte_2 = set_bit(now.hour.val, 6, parity(now.hour.val));
        data.byte_2 = set_bit(data.byte_2, 7, now.day.bit.b0);


        // bit 37-44  // day + weekday
        data.byte_3 = now.day.val>>1 | now.weekday.val<<5;

        // bit 45-52  // month + bit 0-2 of year
        data.byte_4 = now.month.val | now.year.val<<5;

        const uint8_t date_parity = parity(now.day.digit.lo)   ^
                                    parity(now.day.digit.hi)   ^
                                    parity(now.month.digit.lo) ^
                                    parity(now.month.digit.hi) ^
                                    parity(now.weekday.val)    ^
                                    parity(now.year.digit.lo)  ^
                                    parity(now.year.digit.hi);
        // bit 53-58  // year + parity
        data.byte_5 = set_bit(now.year.val>>3, 5, date_parity);
    }

    void debug(const DCF77::time_data_t &clock) {
        using namespace Debug;

        Serial.print(F("  "));
        bcddigits(clock.year.val);
        Serial.print('.');
        bcddigits(clock.month.val);
        Serial.print('.');
        bcddigits(clock.day.val);
        Serial.print('(');
        bcddigit(clock.weekday.val);
        Serial.print(',');
        bcddigit(weekday(clock));
        Serial.print(')');
        bcddigits(clock.hour.val);
        Serial.print(':');
        bcddigits(clock.minute.val);
        Serial.print(':');
        if (clock.second < 10) {
            Serial.print('0');
        }
        Serial.print(clock.second, DEC);
        if (clock.uses_summertime) {
            Serial.print(F(" MESZ "));
        } else {
            Serial.print(F(" MEZ "));
        }
        if (clock.leap_second_scheduled) {
            Serial.print(F("leap second scheduled"));
        }
        if (clock.timezone_change_scheduled) {
            Serial.print(F("time zone change scheduled"));
        }
    }

    void debug(const DCF77::time_data_t &clock, const uint16_t cycles) {
        DCF77::time_data_t local_clock = clock;
        DCF77::time_data_t decoded_clock;

        Serial.print(F("M ?????????????? RAZZA S mmmmMMMP hhhhHHP ddddDD www mmmmM yyyyYYYYP S"));
        for (uint16_t second = 0; second < cycles; ++second) {
            switch (local_clock.second) {
                case  0: Serial.println(); break;
                case  1: case 15: case 20: case 21: case 29:
                case 36: case 42: case 45: case 50: case 59: Serial.print(' ');
            }

            const DCF77::tick_t tick_data = get_current_signal(local_clock);
            Debug::debug_helper(tick_data);

            DCF77_Naive_Bitstream_Decoder::set_bit(local_clock.second, tick_data, decoded_clock);

            advance_second(local_clock);

            if (local_clock.second == 0) {
                debug(decoded_clock);
            }
        }

        Serial.println();
        Serial.println();
    }
}

namespace DCF77_Naive_Bitstream_Decoder {
    using namespace DCF77;

    void set_bit(const uint8_t second, const uint8_t value, time_data_t &now) {
        // The naive value is a way to guess a value for unclean decoded data.
        // It is obvious that this is not necessarily a good value but better
        // than nothing.
        const uint8_t naive_value = (value == long_tick || value == undefined)? 1: 0;
        const uint8_t is_value_bad = value != long_tick && value != short_tick;

        now.second = second;

        switch (second) {
            case 15: now.uses_backup_antenna = naive_value; break;
            case 16: now.timezone_change_scheduled = naive_value; break;

            case 17:
                now.uses_summertime = naive_value;
                now.undefined_uses_summertime_output = is_value_bad;
                break;

            case 18:
                if (now.uses_summertime == naive_value) {
                    // if values of bit 17 and 18 match we will enforce a mapping
                    if (!is_value_bad) {
                        now.uses_summertime = !naive_value;
                    }
                }
                now.undefined_uses_summertime_output = false;
                break;

            case 19:
                // leap seconds are seldom, in doubt we assume none is scheduled
                now.leap_second_scheduled = naive_value && !is_value_bad;
                break;


            case 20:
                // start to decode minute data
                now.minute.val = 0x00;
                now.undefined_minute_output = false;
                break;

            case 21: now.minute.val +=      naive_value; break;
            case 22: now.minute.val +=  0x2*naive_value; break;
            case 23: now.minute.val +=  0x4*naive_value; break;
            case 24: now.minute.val +=  0x8*naive_value; break;
            case 25: now.minute.val += 0x10*naive_value; break;
            case 26: now.minute.val += 0x20*naive_value; break;
            case 27: now.minute.val += 0x40*naive_value; break;

            case 28: now.hour.val = 0; break;
            case 29: now.hour.val +=      naive_value; break;
            case 30: now.hour.val +=  0x2*naive_value; break;
            case 31: now.hour.val +=  0x4*naive_value; break;
            case 32: now.hour.val +=  0x8*naive_value; break;
            case 33: now.hour.val += 0x10*naive_value; break;
            case 34: now.hour.val += 0x20*naive_value; break;

            case 35:
                now.day.val = 0x00;
                now.month.val = 0x00;
                now.year.val = 0x00;
                now.weekday.val = 0x00;
                break;

            case 36: now.day.val +=      naive_value; break;
            case 37: now.day.val +=  0x2*naive_value; break;
            case 38: now.day.val +=  0x4*naive_value; break;
            case 39: now.day.val +=  0x8*naive_value; break;
            case 40: now.day.val += 0x10*naive_value; break;
            case 41: now.day.val += 0x20*naive_value; break;

            case 42: now.weekday.val +=     naive_value; break;
            case 43: now.weekday.val += 0x2*naive_value; break;
            case 44: now.weekday.val += 0x4*naive_value; break;

            case 45: now.month.val +=      naive_value; break;
            case 46: now.month.val +=  0x2*naive_value; break;
            case 47: now.month.val +=  0x4*naive_value; break;
            case 48: now.month.val +=  0x8*naive_value; break;
            case 49: now.month.val += 0x10*naive_value; break;

            case 50: now.year.val +=      naive_value; break;
            case 51: now.year.val +=  0x2*naive_value; break;
            case 52: now.year.val +=  0x4*naive_value; break;
            case 53: now.year.val +=  0x8*naive_value; break;
            case 54: now.year.val += 0x10*naive_value; break;
            case 55: now.year.val += 0x20*naive_value; break;
            case 56: now.year.val += 0x40*naive_value; break;
            case 57: now.year.val += 0x80*naive_value; break;
        }
    }
}

namespace DCF77_Flag_Decoder {

    bool uses_backup_antenna;
    int8_t timezone_change_scheduled;
    int8_t uses_summertime;
    int8_t leap_second_scheduled;
    int8_t date_parity;

    void setup() {
        uses_summertime = 0;
        uses_backup_antenna = 0;
        timezone_change_scheduled = 0;
        leap_second_scheduled = 0;
        date_parity = 0;
    }

    void cummulate(int8_t &average, bool count_up) {
        if (count_up) {
            average += (average < 127);
        } else {
            average -= (average > -127);
        }
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 15: uses_backup_antenna = tick_value; break;
            case 16: cummulate(timezone_change_scheduled, tick_value); break;
            case 17: cummulate(uses_summertime, tick_value); break;
            case 18: cummulate(uses_summertime, 1-tick_value); break;
            case 19: cummulate(leap_second_scheduled, tick_value); break;
            case 58: cummulate(date_parity, tick_value); break;
        }
    }

    void reset_after_previous_hour() {
        // HH := hh+1
        // timezone_change_scheduled will be set from hh:01 to HH:00
        // leap_second_scheduled will be set from hh:01 to HH:00

        if (timezone_change_scheduled) {
            timezone_change_scheduled = 0;
            uses_summertime -= uses_summertime;
        }
        leap_second_scheduled = 0;
    }

    void reset_before_new_day() {
        // date_parity will stay the same 00:00-23:59
        date_parity = 0;
    }

    bool get_uses_summertime() {
        return uses_summertime > 0;
    }

    bool get_uses_backup_antenna() {
        return uses_backup_antenna;
    }

    bool get_timezone_change_scheduled() {
        return timezone_change_scheduled > 0;
    }

    bool get_leap_second_scheduled() {
        return leap_second_scheduled > 0;
    }


    void get_quality(uint8_t &uses_summertime_quality,
                     uint8_t &timezone_change_scheduled_quality,
                     uint8_t &leap_second_scheduled_quality) {
        uses_summertime_quality = abs(uses_summertime);
        timezone_change_scheduled_quality = abs(timezone_change_scheduled);
        leap_second_scheduled_quality = abs(leap_second_scheduled);
    }

    void debug() {
        Serial.print(F("Backup Antenna, TZ change, TZ, Leap scheduled, Date parity: "));
        Serial.print(uses_backup_antenna, BIN);
        Serial.print(',');
        Serial.print(timezone_change_scheduled, DEC);
        Serial.print(',');
        Serial.print(uses_summertime, DEC);
        Serial.print(',');
        Serial.print(leap_second_scheduled, DEC);
        Serial.print(',');
        Serial.println(date_parity, DEC);
    }
}

namespace DCF77_Decade_Decoder {
    const uint8_t decades_per_century = 10;

    typedef struct {
        uint8_t data[decades_per_century];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } decade_bins;

    decade_bins bins;


    void advance_decade() {
        Hamming::advance_tick(bins);
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        using namespace Hamming;

        static BCD::bcd_t decade_data;

        switch (current_second) {
            case 54: decade_data.val +=      tick_value; break;
            case 55: decade_data.val += 0x02*tick_value; break;
            case 56: decade_data.val += 0x04*tick_value; break;
            case 57: decade_data.val += 0x08*tick_value;
            hamming_binning<decade_bins, 4, false>(bins, decade_data); break;

            case 58: compute_max_index(bins);
            // fall through on purpose
            default: decade_data.val = 0;
        }
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);
    }

    uint8_t get_quality_factor() {
        return Hamming::get_quality_factor(bins);
    }

    BCD::bcd_t get_decade() {
        return Hamming::get_time_value(bins);
    }

    void setup() {
        Hamming::setup(bins);
    }

    void debug() {
        Serial.print(F("Decade: "));
        Hamming::debug(bins);
    }
}


namespace DCF77_Year_Decoder {
    const uint8_t years_per_century = 10;

    typedef struct {
        uint8_t data[years_per_century];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } year_bins;

    year_bins bins;


    void advance_year() {
        Hamming::advance_tick(bins);
        if (Hamming::get_time_value(bins).val == 0) {
            DCF77_Decade_Decoder::advance_decade();
        }
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        using namespace Hamming;

        static BCD::bcd_t year_data;

        switch (current_second) {
            case 50: year_data.val +=      tick_value; break;
            case 51: year_data.val +=  0x2*tick_value; break;
            case 52: year_data.val +=  0x4*tick_value; break;
            case 53: year_data.val +=  0x8*tick_value;
                     hamming_binning<year_bins, 4, false>(bins, year_data); break;

            case 54: compute_max_index(bins);
            // fall through on purpose
            default: year_data.val = 0;
        }

        DCF77_Decade_Decoder::process_tick(current_second, tick_value);
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);

        Hamming::lock_quality_t decade_lock_quality;
        DCF77_Decade_Decoder::get_quality(decade_lock_quality);

        Arithmetic_Tools::minimize(lock_quality.lock_max, decade_lock_quality.lock_max);
        Arithmetic_Tools::maximize(lock_quality.noise_max, decade_lock_quality.noise_max);
    }

    uint8_t get_quality_factor() {
        const uint8_t qf_years = Hamming::get_quality_factor(bins);
        const uint8_t qf_decades = DCF77_Decade_Decoder::get_quality_factor();
        return min(qf_years, qf_decades);
    }

    BCD::bcd_t get_year() {
        BCD::bcd_t year = Hamming::get_time_value(bins);
        BCD::bcd_t decade = DCF77_Decade_Decoder::get_decade();

        if (year.val == 0xff || decade.val == 0xff) {
            // undefined handling
            year.val = 0xff;
        } else {
            year.val += decade.val << 4;
        }
        return year;
    }

    void setup() {
        Hamming::setup(bins);
        DCF77_Decade_Decoder::setup();
    }

    void debug() {
        Serial.print(F("Year: "));
        Hamming::debug(bins);
        DCF77_Decade_Decoder::debug();
    }
}

namespace DCF77_Month_Decoder {
    const uint8_t months_per_year = 12;

    typedef struct {
        uint8_t data[months_per_year];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } month_bins;

    month_bins bins;


    void advance_month() {
        Hamming::advance_tick(bins);
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        using namespace Hamming;

        static BCD::bcd_t month_data;

        switch (current_second) {
            case 45: month_data.val +=      tick_value; break;
            case 46: month_data.val +=  0x2*tick_value; break;
            case 47: month_data.val +=  0x4*tick_value; break;
            case 48: month_data.val +=  0x8*tick_value; break;
            case 49: month_data.val += 0x10*tick_value;
                    hamming_binning<month_bins, 5, false>(bins, month_data); break;

            case 50: compute_max_index(bins);
            // fall through on purpose
            default: month_data.val = 0;
        }
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);
    }

    uint8_t get_quality_factor() {
        return Hamming::get_quality_factor(bins);
    }

    BCD::bcd_t get_month() {
        return Hamming::get_time_value(bins);
    }

    void setup() {
        Hamming::setup(bins);
    }

    void debug() {
        Serial.print(F("Month: "));
        Hamming::debug(bins);
    }
}

namespace DCF77_Weekday_Decoder {
    const uint8_t weekdays_per_week = 7;

    typedef struct {
        uint8_t data[weekdays_per_week];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } weekday_bins;

    weekday_bins bins;


    void advance_weekday() {
        Hamming::advance_tick(bins);
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        using namespace Hamming;

        static BCD::bcd_t weekday_data;

        switch (current_second) {
            case 42: weekday_data.val +=      tick_value; break;
            case 43: weekday_data.val +=  0x2*tick_value; break;
            case 44: weekday_data.val +=  0x4*tick_value;
                    hamming_binning<weekday_bins, 3, false>(bins, weekday_data); break;
            case 45: compute_max_index(bins);
            // fall through on purpose
            default: weekday_data.val = 0;
        }
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);
    }

    uint8_t get_quality_factor() {
        return Hamming::get_quality_factor(bins);
    }

    BCD::bcd_t get_weekday() {
        return Hamming::get_time_value(bins);
    }

    void setup() {
        Hamming::setup(bins);
    }

    void debug() {
        Serial.print(F("Weekday: "));
        Hamming::debug(bins);
    }
}

namespace DCF77_Day_Decoder {
    const uint8_t days_per_month = 31;

    typedef struct {
        uint8_t data[days_per_month];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } day_bins;

    day_bins bins;


    void advance_day() {
        Hamming::advance_tick(bins);
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        using namespace Hamming;

        static BCD::bcd_t day_data;

        switch (current_second) {
            case 36: day_data.val +=      tick_value; break;
            case 37: day_data.val +=  0x2*tick_value; break;
            case 38: day_data.val +=  0x4*tick_value; break;
            case 39: day_data.val +=  0x8*tick_value; break;
            case 40: day_data.val += 0x10*tick_value; break;
            case 41: day_data.val += 0x20*tick_value;
                    hamming_binning<day_bins, 6, false>(bins, day_data); break;
            case 42: compute_max_index(bins);
            // fall through on purpose
            default: day_data.val = 0;
        }
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);
    }

    uint8_t get_quality_factor() {
        return Hamming::get_quality_factor(bins);
    }

    BCD::bcd_t get_day() {
        return Hamming::get_time_value(bins);
    }

    void setup() {
        Hamming::setup(bins);
    }

    void debug() {
        Serial.print(F("Day: "));
        Hamming::debug(bins);
    }
}

namespace DCF77_Hour_Decoder {
    const uint8_t hours_per_day = 24;

    typedef struct {
        uint8_t data[hours_per_day];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } hour_bins;

    hour_bins bins;


    void advance_hour() {
        Hamming::advance_tick(bins);
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        using namespace Hamming;

        static BCD::bcd_t hour_data;

        switch (current_second) {
            case 29: hour_data.val +=      tick_value; break;
            case 30: hour_data.val +=  0x2*tick_value; break;
            case 31: hour_data.val +=  0x4*tick_value; break;
            case 32: hour_data.val +=  0x8*tick_value; break;
            case 33: hour_data.val += 0x10*tick_value; break;
            case 34: hour_data.val += 0x20*tick_value; break;
            case 35: hour_data.val += 0x80*tick_value;        // Parity !!!
                    hamming_binning<hour_bins, 7, true>(bins, hour_data); break;

            case 36: compute_max_index(bins);
                    // fall through on purpose
            default: hour_data.val = 0;
        }
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);
    }

    uint8_t get_quality_factor() {
        return Hamming::get_quality_factor(bins);
    }

    BCD::bcd_t get_hour() {
        return Hamming::get_time_value(bins);
    }

    void setup() {
        Hamming::setup(bins);
    }

    void debug() {
        Serial.print(F("Hour: "));
        Hamming::debug(bins);
    }
}

namespace DCF77_Minute_Decoder {
    const uint8_t minutes_per_hour = 60;

    typedef struct {
        uint8_t data[minutes_per_hour];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } minute_bins;

    minute_bins bins;

    void advance_minute() {
        Hamming::advance_tick(bins);
    }

    void process_tick(const uint8_t current_second, const uint8_t tick_value) {
        using namespace Hamming;

        static BCD::bcd_t minute_data;

        switch (current_second) {
            case 21: minute_data.val +=      tick_value; break;
            case 22: minute_data.val +=  0x2*tick_value; break;
            case 23: minute_data.val +=  0x4*tick_value; break;
            case 24: minute_data.val +=  0x8*tick_value; break;
            case 25: minute_data.val += 0x10*tick_value; break;
            case 26: minute_data.val += 0x20*tick_value; break;
            case 27: minute_data.val += 0x40*tick_value; break;
            case 28: minute_data.val += 0x80*tick_value;        // Parity !!!
                    hamming_binning<minute_bins, 8, true>(bins, minute_data); break;
            case 29: compute_max_index(bins);
            // fall through on purpose
            default: minute_data.val = 0;
        }
    }

    void setup() {
        Hamming::setup(bins);
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);
    }

    uint8_t get_quality_factor() {
        return Hamming::get_quality_factor(bins);
    }

    BCD::bcd_t get_minute() {
        return Hamming::get_time_value(bins);
    }

    void debug() {
        Serial.print(F("Minute: "));
        Hamming::debug(bins);
    }
}

namespace DCF77_Second_Decoder {
    using namespace DCF77;

    const uint8_t seconds_per_minute = 60;
    // this is a trick threshold
    //    lower it to get a faster second lock
    //    but then risk to garble the successive stages during startup
    //    --> to low and total startup time will increase
    const uint8_t lock_threshold = 12;

    typedef struct {
        uint8_t data[seconds_per_minute];
        uint8_t tick;

        uint8_t noise_max;
        uint8_t max;
        uint8_t max_index;
    } sync_bins;

    sync_bins bins;

    serialized_clock_stream convolution_kernel;
    // used to determine how many of the predicted bits are actually observed,
    // also used to indicate if convolution is already applied
    const uint8_t convolution_binning_not_ready = 0xff;
    uint8_t prediction_match = convolution_binning_not_ready;
    uint8_t buffered_match = convolution_binning_not_ready;

    uint8_t get_prediction_match() {
        return buffered_match;
    };

    void set_convolution_time(const DCF77::time_data_t &now) {
        DCF77::time_data_t convolution_clock = now;

        // we are always decoding the data for the NEXT minute
        DCF77_Encoder::advance_minute(convolution_clock);

        // the convolution kernel shall have proper flag settings
        DCF77_Encoder::autoset_control_bits(convolution_clock);

        DCF77_Encoder::get_serialized_clock_stream(convolution_clock, convolution_kernel);
        prediction_match = 0;
    }

    void convolution_binning(const uint8_t tick_data) {
        using namespace Arithmetic_Tools;

        // determine sync lock
        if (bins.max - bins.noise_max <= lock_threshold || get_second() == 3) {
            // after a lock is acquired this happens only once per minute and it is
            // reasonable cheap to process,
            //
            // that is: after we have a "lock" this will be processed whenever
            // the sync mark was detected

            Hamming::compute_max_index(bins);

            const uint8_t convolution_weight = 50;
            if (bins.max > 255-convolution_weight) {
                // If we know we can not raise the maximum any further we
                // will lower the noise floor instead.
                for (uint8_t bin_index = 0; bin_index < seconds_per_minute; ++bin_index) {
                    bounded_decrement<convolution_weight>(bins.data[bin_index]);
                }
                bins.max -= convolution_weight;
                bounded_decrement<convolution_weight>(bins.noise_max);
            }

            buffered_match = prediction_match;
        }

        if (tick_data == sync_mark) {
            bounded_increment<6>(bins.data[bins.tick]);
            if (bins.tick == bins.max_index) {
                prediction_match += 6;
            }
        } else if (tick_data == short_tick || tick_data == long_tick) {
            uint8_t decoded_bit = (tick_data == long_tick);

            // bit 0 always 0
            uint8_t bin = bins.tick>0? bins.tick-1: seconds_per_minute-1;
            const bool is_match = (decoded_bit == 0);
            bins.data[bin] += is_match;
            if (bin == bins.max_index) {
                prediction_match += is_match;
            }

            // bit 16 is where the convolution kernel starts
            bin = bin>15? bin-16: bin + seconds_per_minute-16;
            uint8_t current_byte_index = 0;
            uint8_t current_bit_index = 3;
            uint8_t current_byte_value = convolution_kernel.byte_0 >> 3;
            while (current_byte_index < 6) {
                while (current_bit_index < 8) {
                    const uint8_t current_bit_value = current_byte_value & 1;
                    const bool is_match = (decoded_bit == current_bit_value);

                    bins.data[bin] += is_match;
                    if (bin == bins.max_index) {
                        prediction_match += is_match;
                    }

                    bin = bin>0? bin-1: seconds_per_minute-1;

                    current_byte_value >>= 1;
                    ++current_bit_index;

                    if (current_byte_index == 5 && current_bit_index > 5) {
                        break;
                    }
                }
                current_bit_index = 0;
                ++current_byte_index;
                current_byte_value = (&(convolution_kernel.byte_0))[current_byte_index];
            }
        }

        bins.tick = bins.tick<seconds_per_minute-1? bins.tick+1: 0;
    }

    void sync_mark_binning(const uint8_t tick_data) {
        // We use a binning approach to find out the proper phase.
        // The goal is to localize the sync_mark. Due to noise
        // there may be wrong marks of course. The idea is to not
        // only look at the statistics of the marks but to exploit
        // additional data properties:

        // Bit position  0 after a proper sync is a 0.
        // Bit position 20 after a proper sync is a 1.

        // The binning will work as follows:

        //   1) A sync mark will score +6 points for the current bin
        //      it will also score -2 points for the previous bin
        //                         -2 points for the following bin
        //                     and -2 points 20 bins later
        //  In total this will ensure that a completely lost signal
        //  will not alter the buffer state (on average)

        //   2) A 0 will score +1 point for the previous bin
        //      it also scores -2 point 20 bins back
        //                 and -2 points for the current bin

        //   3) A 1 will score +1 point 20 bins back
        //      it will also score -2 point for the previous bin
        //                     and -2 points for the current bin

        //   4) An undefined value will score -2 point for the current bin
        //                                    -2 point for the previous bin
        //                                    -2 point 20 bins back

        //   5) Scores have an upper limit of 255 and a lower limit of 0.

        // Summary: sync mark earns 6 points, a 0 in position 0 and a 1 in position 20 earn 1 bonus point
        //          anything that allows to infer that any of the "connected" positions is not a sync will remove 2 points

        // It follows that the score of a sync mark (during good reception)
        // may move up/down the whole scale in slightly below 64 minutes.
        // If the receiver should glitch for whatever reason this implies
        // that the clock will take about 33 minutes to recover the proper
        // phase (during phases of good reception). During bad reception things
        // are more tricky.
        using namespace Arithmetic_Tools;

        const uint8_t previous_tick = bins.tick>0? bins.tick-1: seconds_per_minute-1;
        const uint8_t previous_21_tick = bins.tick>20? bins.tick-21: bins.tick + seconds_per_minute-21;

        switch (tick_data) {
            case sync_mark:
                bounded_increment<6>(bins.data[bins.tick]);

                bounded_decrement<2>(bins.data[previous_tick]);
                bounded_decrement<2>(bins.data[previous_21_tick]);

                { const uint8_t next_tick = bins.tick< seconds_per_minute-1? bins.tick+1: 0;
                bounded_decrement<2>(bins.data[next_tick]); }
                break;

            case short_tick:
                bounded_increment<1>(bins.data[previous_tick]);

                bounded_decrement<2>(bins.data[bins.tick]);
                bounded_decrement<2>(bins.data[previous_21_tick]);
                break;

            case long_tick:
                bounded_increment<1>(bins.data[previous_21_tick]);

                bounded_decrement<2>(bins.data[bins.tick]);
                bounded_decrement<2>(bins.data[previous_tick]);
                break;

            case undefined:
            default:
                bounded_decrement<2>(bins.data[bins.tick]);
                bounded_decrement<2>(bins.data[previous_tick]);
                bounded_decrement<2>(bins.data[previous_21_tick]);
        }
        bins.tick = bins.tick<seconds_per_minute-1? bins.tick+1: 0;

        // determine sync lock
        if (bins.max - bins.noise_max <=lock_threshold ||
            get_second() == 3) {
            // after a lock is acquired this happens only once per minute and it is
            // reasonable cheap to process,
            //
            // that is: after we have a "lock" this will be processed whenever
            // the sync mark was detected

            Hamming::compute_max_index(bins);
        }
    }

    void get_quality(Hamming::lock_quality_t &lock_quality) {
        Hamming::get_quality(bins, lock_quality);
    }

    uint8_t get_quality_factor() {
        return Hamming::get_quality_factor(bins);
    }

    uint8_t get_second() {
        if (bins.max - bins.noise_max >= lock_threshold) {
            // at least one sync mark and a 0 and a 1 seen
            // the threshold is tricky:
            //   higher --> takes longer to acquire an initial lock, but higher probability of an accurate lock
            //
            //   lower  --> higher probability that the lock will oscillate at the beginning
            //              and thus spoil the downstream stages

            // we have to subtract 2 seconds
            //   1 because the seconds already advanced by 1 tick
            //   1 because the sync mark is not second 0 but second 59

            uint8_t second = 2*seconds_per_minute + bins.tick - 2 - bins.max_index;
            while (second >= seconds_per_minute) { second-= seconds_per_minute; }

            return second;
        } else {
            return 0xff;
        }
    }

    void process_single_tick_data(const DCF77::tick_t tick_data) {
        if (prediction_match == convolution_binning_not_ready) {
            sync_mark_binning(tick_data);
        } else {
            convolution_binning(tick_data);
        }
    }

    void setup() {
        Hamming::setup(bins);
    }

    void debug() {
        static uint8_t prev_tick;

        if (prev_tick == bins.tick) {
            return;
        } else {
            prev_tick = bins.tick;

            Serial.print(F("second: "));
            Serial.print(get_second(), DEC);
            Serial.print(F(" Sync mark index "));
            Hamming::debug(bins);
            Serial.print(F("Prediction Match: "));
            Serial.println(prediction_match, DEC);
            Serial.println();
        }
    }
}

namespace DCF77_Local_Clock {

    clock_state_t clock_state = useless;
    DCF77::output_handler_t output_handler = 0;
    DCF77::time_data_t local_clock_time;
    volatile bool second_toggle;
    uint16_t tick = 0;

    // This will take more than 100 years to overflow.
    // An overflow would indicate that the clock is
    // running for >100 years without a sync.
    // --> It is pointless to handle this.
    uint32_t unlocked_seconds = 0;

    void setup() {
        DCF77_Encoder::reset(local_clock_time);
    }

    void get_current_time(DCF77::time_data_t &now) {
        for (bool stopper = second_toggle; stopper == second_toggle; ) {
            // wait for second_toggle to toggle
            // that is wait for decoded time to be ready
        }
        now = local_clock_time;
    }

    void read_current_time(DCF77::time_data_t &now) {
        now = local_clock_time;
    }

    void process_1_Hz_tick(const DCF77::time_data_t &decoded_time) {
        uint8_t quality_factor = DCF77_Clock_Controller::get_overall_quality_factor();

        if (quality_factor > 1) {
            clock_state = synced;
        } else if (clock_state == synced) {
            DCF77_Clock_Controller::sync_lost_event_handler();
            clock_state = locked;
        }

        while (true) {
            switch (clock_state) {
                case useless: {
                    if (quality_factor > 0) {
                        clock_state = dirty;
                        break;  // goto dirty state
                    } else {
                        second_toggle = !second_toggle;
                        return;
                    }
                }

                case dirty: {
                    if (quality_factor == 0) {
                        clock_state = useless;
                        second_toggle = !second_toggle;
                        DCF77_Encoder::reset(local_clock_time);
                        return;
                    } else {
                        tick = 0;
                        local_clock_time = decoded_time;
                        DCF77_Clock_Controller::flush(decoded_time);
                        second_toggle = !second_toggle;
                        return;
                    }
                }

                case synced: {
                    tick = 0;
                    local_clock_time = decoded_time;
                    DCF77_Clock_Controller::flush(decoded_time);
                    second_toggle = !second_toggle;
                    return;
                }

                case locked: {
                    if (DCF77_Demodulator::get_quality_factor() > 1) {
                        // If we are not sure about leap seconds we will skip
                        // them. Worst case is that we miss a leap second due
                        // to noisy reception. This may happen at most once a
                        // year.
                        local_clock_time.leap_second_scheduled = false;

                        // autoset_control_bits is not required because
                        // advance_second will call this internally anyway
                        //DCF77_Encoder::autoset_control_bits(local_clock_time);
                        DCF77_Encoder::advance_second(local_clock_time);
                        DCF77_Clock_Controller::flush(local_clock_time);
                        tick = 0;
                        second_toggle = !second_toggle;
                        return;
                    } else {
                        clock_state = unlocked;
                        DCF77_Clock_Controller::phase_lost_event_handler();
                        unlocked_seconds = 0;
                        return;
                    }
                }

                case unlocked: {
                    if (200 < tick && tick < 800) {
                        clock_state = free;
                        return;
                    } else if (DCF77_Demodulator::get_quality_factor() > 1) {
                        clock_state = locked;
                        if (tick < 200) {
                            // time output was handled at most 200 ms before
                            tick = 0;
                            return;
                        } else {
                            break;  // goto locked state
                        }
                    }
                }

                case free: {
                    return;
                }
            }
        }
    }

    void process_1_kHz_tick() {
        ++tick;

        if (clock_state == synced || clock_state == locked) {
            // the important part is 150 < 200,
            // otherwise it will fall through to free immediately after changing to unlocked
            if (tick >= 1150) {
                // The 1 Hz pulse was locked but now
                // it is definitely out of phase.
                unlocked_seconds = 1;

                // 1 Hz tick missing for more than 1200ms
                clock_state = unlocked;
                DCF77_Clock_Controller::phase_lost_event_handler();
            }
        }

        if (clock_state == unlocked || clock_state == free) {
            if (tick >= 1000) {
                tick -= 1000;

                // If we are not sure about leap seconds we will skip
                // them. Worst case is that we miss a leap second due
                // to noisy reception. This may happen at most once a
                // year.
                local_clock_time.leap_second_scheduled = false;

                // autoset_control_bits is not required because
                // advance_second will call this internally anyway
                //DCF77_Encoder::autoset_control_bits(local_clock_time);
                DCF77_Encoder::advance_second(local_clock_time);
                DCF77_Clock_Controller::flush(local_clock_time);
                second_toggle = !second_toggle;

                ++unlocked_seconds;
                if (unlocked_seconds > 3000) {
                    clock_state = free;
                }
            }
        }
    }


    void set_output_handler(const DCF77::output_handler_t new_output_handler) {
        output_handler = new_output_handler;
    }

    clock_state_t get_state() {
        return clock_state;
    }

    void debug() {
        Serial.print(F("Clock state: "));
        switch (clock_state) {
            case useless:  Serial.println(F("useless"));  break;
            case dirty:    Serial.println(F("dirty"));    break;
            case free:     Serial.println(F("free"));     break;
            case unlocked: Serial.println(F("unlocked")); break;
            case locked:   Serial.println(F("locked"));   break;
            case synced:   Serial.println(F("synced"));   break;
            default:       Serial.println(F("undefined"));
        }
        Serial.print(F("Tick: "));
        Serial.println(tick);
    }
}

namespace DCF77_Clock_Controller {
    uint8_t leap_second = 0;
    DCF77_Clock::output_handler_t output_handler = 0;
    DCF77::time_data_t decoded_time;

    void get_current_time(DCF77::time_data_t &now) {
        DCF77_Local_Clock::get_current_time(now);
    }

    void read_current_time(DCF77::time_data_t &now) {
        DCF77_Local_Clock::get_current_time(now);
    }

    void set_DCF77_encoder(DCF77::time_data_t &now) {
        using namespace DCF77_Second_Decoder;
        using namespace DCF77_Minute_Decoder;
        using namespace DCF77_Hour_Decoder;
        using namespace DCF77_Weekday_Decoder;
        using namespace DCF77_Day_Decoder;
        using namespace DCF77_Month_Decoder;
        using namespace DCF77_Year_Decoder;
        using namespace DCF77_Flag_Decoder;

        now.second  = get_second();
        now.minute  = get_minute();
        now.hour    = get_hour();
        now.weekday = get_weekday();
        now.day     = get_day();
        now.month   = get_month();
        now.year    = get_year();

        now.uses_backup_antenna       = get_uses_backup_antenna();
        now.timezone_change_scheduled = get_timezone_change_scheduled();
        now.uses_summertime           = get_uses_summertime();
        now.leap_second_scheduled     = get_leap_second_scheduled();
    }

    void flush() {
        // This is called "at the end of each second / before the next second begins."
        // The call is triggered by the decoder stages. Thus it flushes the current
        // decoded time. If the decoders are out of sync this may not be
        // called at all.

        DCF77::time_data_t now;
        DCF77::time_data_t now_1;

        set_DCF77_encoder(now);
        now_1 = now;

        // leap_second offset to compensate for the skipped second tick
        now.second += leap_second > 0;

        DCF77_Encoder::advance_second(now);
        DCF77_Encoder::autoset_control_bits(now);

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
                DCF77_Flag_Decoder::reset_after_previous_hour();

                now.uses_summertime = DCF77_Flag_Decoder::get_uses_summertime();
                now.timezone_change_scheduled = DCF77_Flag_Decoder::get_timezone_change_scheduled();
                now.leap_second_scheduled = DCF77_Flag_Decoder::get_leap_second_scheduled();

                DCF77_Encoder::autoset_control_bits(now);
                decoded_time.uses_summertime = now.uses_summertime;
                decoded_time.timezone_change_scheduled = now.timezone_change_scheduled;
                decoded_time.leap_second_scheduled = now.leap_second_scheduled;
                decoded_time.uses_backup_antenna = DCF77_Flag_Decoder::get_uses_backup_antenna();
            }

            if (now.hour.val == 0x23 && now.minute.val == 0x59) {
                // We are now starting to process the data for the
                // new day. Thus the parity flag is of little use anymore.

                // We could be smarter though and merge synthetic parity information with measured historic values.

                // that is: advance(now), see if parity changed, if not so --> fine, otherwise change sign of flag
                DCF77_Flag_Decoder::reset_before_new_day();
            }

            // reset leap second
            leap_second &= leap_second < 2;
        }

        // pass control to local clock
        DCF77_Local_Clock::process_1_Hz_tick(decoded_time);
    }

    void flush(const DCF77::time_data_t &decoded_time) {
        // This is the callback for the "local clock".
        // It will be called once per second.

        // It ensures that the local clock is decoupled
        // from things like "output handling".
        if (output_handler) {
            DCF77_Clock::time_t time;

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

        if (decoded_time.second == 15 && DCF77_Local_Clock::clock_state != DCF77_Local_Clock::useless
                                      && DCF77_Local_Clock::clock_state != DCF77_Local_Clock::dirty
        ) {
            DCF77_Second_Decoder::set_convolution_time(decoded_time);
        }
    };

    void process_1_kHz_tick_data(const uint8_t sampled_data) {
        DCF77_Demodulator::detector(sampled_data);
        DCF77_Local_Clock::process_1_kHz_tick();
    }

    void set_output_handler(const DCF77_Clock::output_handler_t new_output_handler) {
        output_handler = new_output_handler;
    }

    void get_quality(clock_quality_t &clock_quality) {
        DCF77_Demodulator::get_quality(clock_quality.phase.lock_max, clock_quality.phase.noise_max);
        DCF77_Second_Decoder::get_quality(clock_quality.second);
        DCF77_Minute_Decoder::get_quality(clock_quality.minute);
        DCF77_Hour_Decoder::get_quality(clock_quality.hour);
        DCF77_Day_Decoder::get_quality(clock_quality.day);
        DCF77_Weekday_Decoder::get_quality(clock_quality.weekday);
        DCF77_Month_Decoder::get_quality(clock_quality.month);
        DCF77_Year_Decoder::get_quality(clock_quality.year);

        DCF77_Flag_Decoder::get_quality(clock_quality.uses_summertime_quality,
                                        clock_quality.timezone_change_scheduled_quality,
                                        clock_quality.leap_second_scheduled_quality);
    }


    void get_quality_factor(clock_quality_factor_t &clock_quality_factor) {
        clock_quality_factor.phase   = DCF77_Demodulator::get_quality_factor();
        clock_quality_factor.second  = DCF77_Second_Decoder::get_quality_factor();
        clock_quality_factor.minute  = DCF77_Minute_Decoder::get_quality_factor();
        clock_quality_factor.hour    = DCF77_Hour_Decoder::get_quality_factor();
        clock_quality_factor.day     = DCF77_Day_Decoder::get_quality_factor();
        clock_quality_factor.weekday = DCF77_Weekday_Decoder::get_quality_factor();
        clock_quality_factor.month   = DCF77_Month_Decoder::get_quality_factor();
        clock_quality_factor.year    = DCF77_Year_Decoder::get_quality_factor();
    }

    uint8_t get_overall_quality_factor() {
        using namespace Arithmetic_Tools;

        uint8_t quality_factor = DCF77_Demodulator::get_quality_factor();
        minimize(quality_factor, DCF77_Second_Decoder::get_quality_factor());
        minimize(quality_factor, DCF77_Minute_Decoder::get_quality_factor());
        minimize(quality_factor, DCF77_Hour_Decoder::get_quality_factor());

        uint8_t date_quality_factor = DCF77_Day_Decoder::get_quality_factor();
        minimize(date_quality_factor, DCF77_Month_Decoder::get_quality_factor());
        minimize(date_quality_factor, DCF77_Year_Decoder::get_quality_factor());

        const uint8_t weekday_quality_factor = DCF77_Weekday_Decoder::get_quality_factor();
        if (date_quality_factor > 0 && weekday_quality_factor > 0) {

            DCF77::time_data_t now;
            now.second  = DCF77_Second_Decoder::get_second();
            now.minute  = DCF77_Minute_Decoder::get_minute();
            now.hour    = DCF77_Hour_Decoder::get_hour();
            now.weekday = DCF77_Weekday_Decoder::get_weekday();
            now.day     = DCF77_Day_Decoder::get_day();
            now.month   = DCF77_Month_Decoder::get_month();
            now.year    = DCF77_Year_Decoder::get_year();

            BCD::bcd_t weekday = DCF77_Encoder::bcd_weekday(now);
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

    uint8_t get_clock_state() {
        return DCF77_Local_Clock::get_state();
    }

    uint8_t get_prediction_match() {
        return DCF77_Second_Decoder::get_prediction_match();
    }

    void debug() {
        clock_quality_t clock_quality;
        get_quality(clock_quality);

        clock_quality_factor_t clock_quality_factor;
        get_quality_factor(clock_quality_factor);

        Serial.print(F("Quality (p,s,m,h,wd,d,m,y,st,tz,ls,pm): "));
        Serial.print(get_overall_quality_factor(), DEC);
        Serial.print(F(" ("));
        Serial.print(clock_quality.phase.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.phase.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.phase, DEC);
        Serial.print(')');

        Serial.print('(');
        Serial.print(clock_quality.second.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.second.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.second, DEC);
        Serial.print(')');

        Serial.print('(');
        Serial.print(clock_quality.minute.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.minute.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.minute, DEC);
        Serial.print(')');

        Serial.print('(');
        Serial.print(clock_quality.hour.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.hour.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.hour, DEC);
        Serial.print(')');

        Serial.print('(');
        Serial.print(clock_quality.weekday.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.weekday.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.weekday, DEC);
        Serial.print(')');

        Serial.print('(');
        Serial.print(clock_quality.day.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.day.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.day, DEC);
        Serial.print(')');

        Serial.print('(');
        Serial.print(clock_quality.month.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.month.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.month, DEC);
        Serial.print(')');

        Serial.print('(');
        Serial.print(clock_quality.year.lock_max, DEC);
        Serial.print('-');
        Serial.print(clock_quality.year.noise_max, DEC);
        Serial.print(':');
        Serial.print(clock_quality_factor.year, DEC);
        Serial.print(')');

        Serial.print(clock_quality.uses_summertime_quality, DEC);
        Serial.print(',');
        Serial.print(clock_quality.timezone_change_scheduled_quality, DEC);
        Serial.print(',');
        Serial.print(clock_quality.leap_second_scheduled_quality, DEC);

        Serial.print(',');
        Serial.println(get_prediction_match(), DEC);
    }

    void phase_lost_event_handler() {
        DCF77_Second_Decoder::setup();
        DCF77_Minute_Decoder::setup();
        DCF77_Hour_Decoder::setup();
        DCF77_Day_Decoder::setup();
        DCF77_Weekday_Decoder::setup();
        DCF77_Month_Decoder::setup();
        DCF77_Year_Decoder::setup();
    }

    void sync_lost_event_handler() {
        bool reset_successors = false;

        reset_successors = (DCF77_Demodulator::get_quality_factor() == 0);
        if (reset_successors) {
            DCF77_Second_Decoder::setup();
        }

        reset_successors |= (DCF77_Second_Decoder::get_quality_factor() == 0);
        if (reset_successors) {
            DCF77_Minute_Decoder::setup();
        }

        reset_successors |= (DCF77_Minute_Decoder::get_quality_factor() == 0);
        if (reset_successors) {
            DCF77_Hour_Decoder::setup();
        }

        reset_successors |= (DCF77_Hour_Decoder::get_quality_factor() == 0);
        if (reset_successors) {
            DCF77_Weekday_Decoder::setup();
            DCF77_Day_Decoder::setup();
        }

        reset_successors |= (DCF77_Hour_Decoder::get_quality_factor() == 0);
        if (reset_successors) {
            DCF77_Month_Decoder::setup();
        }

        reset_successors |= (DCF77_Month_Decoder::get_quality_factor() == 0);
        if (reset_successors) {
            DCF77_Year_Decoder::setup();
        }
    }

    void setup() {
        DCF77_Demodulator::setup();
        phase_lost_event_handler();
    }

    void process_single_tick_data(const DCF77::tick_t tick_data) {
        using namespace DCF77;
        using namespace DCF77_Second_Decoder;
        using namespace DCF77_Minute_Decoder;
        using namespace DCF77_Hour_Decoder;
        using namespace DCF77_Weekday_Decoder;
        using namespace DCF77_Day_Decoder;
        using namespace DCF77_Month_Decoder;
        using namespace DCF77_Year_Decoder;
        using namespace DCF77_Flag_Decoder;

        time_data_t now;
        set_DCF77_encoder(now);
        now.second += leap_second;

        DCF77_Encoder::advance_second(now);

        // leap_second == 2 indicates that we are in the 2nd second of the leap second handling
        leap_second <<= 1;
        // leap_second == 1 indicates that we now process second 59 but not the expected sync mark
        leap_second += (now.second == 59 && DCF77_Encoder::get_current_signal(now) != sync_mark);

        if (leap_second != 1) {
            DCF77_Second_Decoder::process_single_tick_data(tick_data);

            if (now.second == 0) {
                DCF77_Minute_Decoder::advance_minute();
                if (now.minute.val == 0x00) {

                    // "while" takes automatically care of timezone change
                    while (get_hour().val <= 0x23 && get_hour().val != now.hour.val) { advance_hour(); }

                    if (now.hour.val == 0x00) {
                        if (get_weekday().val <= 0x07) { advance_weekday(); }

                        // "while" takes automatically care of different month lengths
                        while (get_day().val <= 0x31 && get_day().val != now.day.val) { advance_day(); }

                        if (now.day.val == 0x01) {
                            if (get_month().val <= 0x12) { advance_month(); }
                            if (now.month.val == 0x01) {
                                if (now.year.val <= 0x99) { advance_year(); }
                            }
                        }
                    }
                }
            }
            const uint8_t tick_value = (tick_data == long_tick || tick_data == undefined)? 1: 0;
            DCF77_Flag_Decoder::process_tick(now.second, tick_value);
            DCF77_Minute_Decoder::process_tick(now.second, tick_value);
            DCF77_Hour_Decoder::process_tick(now.second, tick_value);
            DCF77_Weekday_Decoder::process_tick(now.second, tick_value);
            DCF77_Day_Decoder::process_tick(now.second, tick_value);
            DCF77_Month_Decoder::process_tick(now.second, tick_value);
            DCF77_Year_Decoder::process_tick(now.second, tick_value);
        }
    }
}

namespace DCF77_Demodulator {
    using namespace DCF77;

    const uint8_t bin_count = 100;

    typedef struct {
        uint16_t data[bin_count];
        uint8_t tick;

        uint32_t noise_max;
        uint32_t max;
        uint8_t max_index;
    } phase_bins;

    phase_bins bins;

    const uint16_t samples_per_second = 1000;

    const uint16_t samples_per_bin = samples_per_second / bin_count;
    const uint16_t bins_per_10ms  = bin_count / 100;
    const uint16_t bins_per_50ms  =  5 * bins_per_10ms;
    const uint16_t bins_per_60ms  =  6 * bins_per_10ms;
    const uint16_t bins_per_100ms = 10 * bins_per_10ms;
    const uint16_t bins_per_200ms = 20 * bins_per_10ms;
    const uint16_t bins_per_500ms = 50 * bins_per_10ms;

    void setup() {
        Hamming::setup(bins);
    }

    void decode_220ms(const uint8_t input, const uint8_t bins_to_go) {
        // will be called for each bin during the "interesting" 220ms

        static uint8_t count = 0;
        static uint8_t decoded_data = 0;

        count += input;
        if (bins_to_go >= bins_per_100ms + bins_per_10ms) {
            if (bins_to_go == bins_per_100ms + bins_per_10ms) {
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
                DCF77_Clock_Controller::process_single_tick_data((DCF77::tick_t)decoded_data);
            }
        }
    }

    uint16_t wrap(const uint16_t value) {
        // faster modulo function which avoids division
        uint16_t result = value;
        while (result >= bin_count) {
            result-= bin_count;
        }
        return result;
    }

    void phase_detection() {
        // We will compute the integrals over 200ms.
        // The integrals is used to find the window of maximum signal strength.
        uint32_t integral = 0;

        for (uint16_t bin = 0; bin < bins_per_100ms; ++bin)  {
            integral += ((uint32_t)bins.data[bin])<<1;
        }

        for (uint16_t bin = bins_per_100ms; bin < bins_per_200ms; ++bin)  {
            integral += (uint32_t)bins.data[bin];
        }

        bins.max = 0;
        bins.max_index = 0;
        for (uint16_t bin = 0; bin < bin_count; ++bin) {
            if (integral > bins.max) {
                bins.max = integral;
                bins.max_index = bin;
            }

            integral -= (uint32_t)bins.data[bin]<<1;
            integral += (uint32_t)(bins.data[wrap(bin + bins_per_100ms)] + bins.data[wrap(bin + bins_per_200ms)]);
        }

        // max_index indicates the position of the 200ms second signal window.
        // Now how can we estimate the noise level? This is very tricky because
        // averaging has already happened to some extend.

        // The issue is that most of the undesired noise happens around the signal,
        // especially after high->low transitions. So as an approximation of the
        // noise I test with a phase shift of 200ms.
        bins.noise_max = 0;
        const uint16_t noise_index = wrap(bins.max_index + bins_per_200ms);

        for (uint16_t bin = 0; bin < bins_per_100ms; ++bin)  {
            bins.noise_max += ((uint32_t)bins.data[wrap(noise_index + bin)])<<1;
        }

        for (uint16_t bin = bins_per_100ms; bin < bins_per_200ms; ++bin)  {
            bins.noise_max += (uint32_t)bins.data[wrap(noise_index + bin)];
        }
    }

    uint8_t phase_binning(const uint8_t input) {
        // how many seconds may be cummulated
        // this controls how slow the filter may be to follow a phase drift
        // N times the clock precision shall be smaller 1
        // clock 30 ppm => N < 300
        const uint16_t N = 300;

        Hamming::advance_tick(bins);

        if (input) {
            if (bins.data[bins.tick] < N) {
                ++bins.data[bins.tick];
            }
        } else {
            if (bins.data[bins.tick] > 0) {
                --bins.data[bins.tick];
            }
        }
        return bins.tick;
    }

    void detector_stage_2(const uint8_t input) {
        const uint8_t current_bin = bins.tick;

        const uint8_t threshold = 30;

        if (bins.max-bins.noise_max < threshold ||
            wrap(bin_count + current_bin - bins.max_index) == 53) {
            // Phase detection far enough out of phase from anything that
            // might consume runtime otherwise.
            phase_detection();
        }

        static uint8_t bins_to_process = 0;

        if (bins_to_process == 0) {
            if (wrap((bin_count + current_bin - bins.max_index)) <= bins_per_100ms ||   // current_bin at most 100ms after phase_bin
                wrap((bin_count + bins.max_index - current_bin)) <= bins_per_10ms ) {   // current bin at most 10ms before phase_bin
                // if phase bin varies to much during one period we will always be screwed in may ways...

                // last 10ms of current second
                DCF77_Clock_Controller::flush();

                // start processing of bins
                bins_to_process = bins_per_200ms + 2*bins_per_10ms;
            }
        }

        if (bins_to_process > 0) {
            --bins_to_process;

            // this will be called for each bin in the "interesting" 220ms
            // this is also a good place for a "monitoring hook"
            decode_220ms(input, bins_to_process);
        }
    }

    void detector(const uint8_t sampled_data) {
        static uint8_t current_sample = 0;
        static uint8_t average = 0;

        // detector stage 0: average 10 samples (per bin)
        average += sampled_data;

        if (++current_sample >= samples_per_bin) {
            // once all samples for the current bin are captured the bin gets updated
            // that is each 10ms control is passed to stage 1
            const uint8_t input = (average> samples_per_bin/2);

            phase_binning(input);

            detector_stage_2(input);

            average = 0;
            current_sample = 0;
        }
    }

    void get_quality(uint32_t &lock_max, uint32_t &noise_max) {
        lock_max = bins.max;
        noise_max = bins.noise_max;
    }

    uint8_t get_quality_factor() {
        uint32_t delta = bins.max - bins.noise_max;

        uint8_t log2_plus_1 = 0;
        uint32_t max = bins.max;
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
    };

    void debug() {
        Serial.print(F("Phase: "));
        Hamming::debug(bins);
    }
}

namespace DCF77_Clock {
    typedef void (*output_handler_t)(const time_t &decoded_time);
    typedef uint8_t (*input_provider_t)(void);

    void setup() {
        DCF77_Clock_Controller::setup();
    }

    void setup(const input_provider_t input_provider, const output_handler_t output_handler) {
        DCF77_Clock_Controller::setup();
        DCF77_Clock_Controller::set_output_handler(output_handler);
        DCF77_1_Khz_Generator::setup(input_provider);
    };

    void debug() {
        DCF77_Clock_Controller::debug();
    }

    void set_input_provider(const input_provider_t input_provider) {
        DCF77_1_Khz_Generator::setup(input_provider);
    }

    void set_output_handler(const output_handler_t output_handler) {
        DCF77_Clock_Controller::set_output_handler(output_handler);
    }

    void convert_time(const DCF77::time_data_t &current_time, time_t &now) {
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

    void get_current_time(time_t &now) {
        DCF77::time_data_t current_time;
        DCF77_Clock_Controller::get_current_time(current_time);

        convert_time(current_time, now);
    };

    void read_current_time(time_t &now) {
        DCF77::time_data_t current_time;
        DCF77_Clock_Controller::read_current_time(current_time);

        convert_time(current_time, now);
    };

    void print(time_t time) {
        BCD::print(time.year);
        Serial.print('-');
        BCD::print(time.month);
        Serial.print('-');
        BCD::print(time.day);
        Serial.print(' ');
        Serial.print(time.weekday.val & 0xF, HEX);
        Serial.print(' ');
        BCD::print(time.hour);
        Serial.print(':');
        BCD::print(time.minute);
        Serial.print(':');
        BCD::print(time.second);

        if (time.uses_summertime) {
            Serial.print(F(" MESZ "));
        } else {
            Serial.print(F(" MEZ "));
        }

        Serial.print(time.timezone_change_scheduled? '*': '.');
        Serial.print(time.leap_second_scheduled    ? 'L': '.');
    }

    uint8_t get_overall_quality_factor() {
        return DCF77_Clock_Controller::get_overall_quality_factor();
    };

    uint8_t get_clock_state() {
        return DCF77_Clock_Controller::get_clock_state();
    };

    uint8_t get_prediction_match() {
        return DCF77_Clock_Controller::get_prediction_match();
    };
}

namespace DCF77_1_Khz_Generator {
    uint8_t zero_provider() {
        return 0;
    }
    
    static DCF77_Clock::input_provider_t the_input_provider = zero_provider;
    static int16_t adjust_pp16m = 0;
    static int32_t cumulated_drift = 0;
    
    void adjust(const int16_t pp16m) {
        const uint8_t prev_SREG = SREG;
        cli();
        // positive_value --> increase frequency
        adjust_pp16m = pp16m;
        SREG = prev_SREG;
    }

    int16_t read_adjustment() {
        // positive_value --> increase frequency
        const uint8_t prev_SREG = SREG;
        cli();
        const int16_t pp16m = adjust_pp16m;
        SREG = prev_SREG;
        return pp16m;
    }
    
    void init_timer_2() {
        // Timer 2 CTC mode, prescaler 64
        TCCR2B = (1<<WGM22) | (1<<CS22);
        TCCR2A = (1<<WGM21);
        
        // 249 + 1 == 250 == 250 000 / 1000 =  (16 000 000 / 64) / 1000
        OCR2A = 249;
        
        // enable Timer 2 interrupts
        TIMSK2 = (1<<OCIE2A);
    }
    
    void stop_timer_0() {
        // ensure that the standard timer interrupts will not
        // mess with msTimer2
        TIMSK0 = 0;
    }
    
    void setup(const DCF77_Clock::input_provider_t input_provider) {
        init_timer_2();
        stop_timer_0();
        the_input_provider = input_provider;
    }
    
    void isr_handler() {
        cumulated_drift += adjust_pp16m;
        // 1 / 250 / 64000 = 1 / 16 000 000
        if (cumulated_drift >= 64000) {
            cumulated_drift -= 64000;
            // cumulated drift exceeds 1 timer step (4 microseconds)
            // drop one timer step to realign
            OCR2A = 248;
        } else
            if (cumulated_drift <= -64000) {
                // cumulated drift exceeds 1 timer step (4 microseconds)
                // insert one timer step to realign
                cumulated_drift += 64000;
                OCR2A = 250;
            } else {
                // 249 + 1 == 250 == 250 000 / 1000 =  (16 000 000 / 64) / 1000
                OCR2A = 249;
            }
            DCF77_Clock_Controller::process_1_kHz_tick_data(the_input_provider());
    }
}

ISR(TIMER2_COMPA_vect) {
    DCF77_1_Khz_Generator::isr_handler();
}
