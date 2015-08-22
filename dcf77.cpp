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

#ifdef STANDALONE
#include "standalone/standalone.h"
#endif

#include "dcf77.h"


namespace Internal { namespace Debug {
    void debug_helper(char data) { sprint(data == 0? 'S': data == 1? '?': data - 2 + '0', 0); }

    void bcddigit(uint8_t data) {
        if (data <= 0x09) {
            sprint(data, HEX);
        } else {
            sprint('?');
        }
    }

    void bcddigits(uint8_t data) {
        bcddigit(data >>  4);
        bcddigit(data & 0xf);
    }

    void hexdump(uint8_t data) {
        if (data < 0x10) { sprint('0'); }
        sprint(data, HEX);
    }
}}

namespace BCD {
    void print(const bcd_t value) {
        sprint(value.val >> 4 & 0xF, HEX);
        sprint(value.val >> 0 & 0xF, HEX);
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

    bool operator == (const bcd_t a, const bcd_t b) {
        return a.val == b.val;
    }
    bool operator != (const bcd_t a, const bcd_t b) {
        return a.val != b.val;
    }
    bool operator >= (const bcd_t a, const bcd_t b) {
        return a.val >= b.val;
    }
    bool operator <= (const bcd_t a, const bcd_t b) {
        return a.val <= b.val;
    }
    bool operator > (const bcd_t a, const bcd_t b) {
        return a.val > b.val;
    }
    bool operator < (const bcd_t a, const bcd_t b) {
        return a.val < b.val;
    }
}

namespace Internal { namespace Arithmetic_Tools {
    void bounded_add(uint8_t &value, const uint8_t amount) {
        if (value >= 255-amount) { value = 255; } else { value += amount; }
    }

    void bounded_sub(uint8_t &value, const uint8_t amount) {
        if (value <= amount) { value = 0; } else { value -= amount; }
    }

    uint8_t bit_count(const uint8_t value) {
        const uint8_t tmp1 = (value & 0b01010101) + ((value>>1) & 0b01010101);
        const uint8_t tmp2 = (tmp1  & 0b00110011) + ((tmp1>>2) & 0b00110011);
        return (tmp2 & 0x0f) + (tmp2>>4);
    }

    uint8_t parity(const uint8_t value) {
        uint8_t tmp = value;

        tmp = (tmp & 0xf) ^ (tmp >> 4);
        tmp = (tmp & 0x3) ^ (tmp >> 2);
        tmp = (tmp & 0x1) ^ (tmp >> 1);

        return tmp;
    }

    uint8_t set_bit(const uint8_t data, const uint8_t number, const uint8_t value) {
        return value? data|(1<<number): data & ~(1<<number);
    }
}}

namespace Internal {  // DCF77_Flag_Decoder
    void DCF77_Flag_Decoder::setup() {
        uses_summertime = 0;
        abnormal_transmitter_operation = 0;
        timezone_change_scheduled = 0;
        leap_second_scheduled = 0;
        date_parity = 0;
#ifdef MSF60
        year_parity = 0;
        weekday_parity = 0;
        time_parity = 0;
#endif
    }

    void DCF77_Flag_Decoder::cummulate(int8_t &average, bool count_up) {
        if (count_up) {
            average += (average < 127);
        } else {
            average -= (average > -127);
        }
    }

    void DCF77_Flag_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {

#ifndef MSF60
            case 15: abnormal_transmitter_operation = tick_value;      break;
            case 16: cummulate(timezone_change_scheduled, tick_value); break;
            case 17: cummulate(uses_summertime, tick_value);           break;
            case 18: cummulate(uses_summertime, 1-tick_value);         break;
            case 19: cummulate(leap_second_scheduled, tick_value);     break;
            case 58: cummulate(date_parity, tick_value);               break;
#else
            // For MSF, tick_value is Bit B from Signal, so 53B...58B
            case 53: cummulate(timezone_change_scheduled, tick_value); break;
            case 54: cummulate(year_parity, tick_value); break;
            case 55: cummulate(date_parity, tick_value); break;
            case 56: cummulate(weekday_parity, tick_value); break;
            case 57: cummulate(time_parity, tick_value); break;
            case 58: cummulate(uses_summertime, tick_value); break;
#endif
        }
    }

    void DCF77_Flag_Decoder::reset_after_previous_hour() {
        // HH := hh+1
        // timezone_change_scheduled will be set from hh:01 to HH:00
        // leap_second_scheduled will be set from hh:01 to HH:00

        if (get_timezone_change_scheduled()) {
            timezone_change_scheduled = 0;
            uses_summertime = 0;
        }
        leap_second_scheduled = 0;
    }

    void DCF77_Flag_Decoder::reset_before_new_day() {
        // date_parity will stay the same 00:00-23:59
        date_parity = 0;
#ifdef MSF60
        weekday_parity = 0;
#endif
    }

    bool DCF77_Flag_Decoder::get_uses_summertime() {
        return uses_summertime > 0;
    }

    bool DCF77_Flag_Decoder::get_abnormal_transmitter_operation() {
        return abnormal_transmitter_operation;
    }

    bool DCF77_Flag_Decoder::get_timezone_change_scheduled() {
        return timezone_change_scheduled > 0;
    }

    // Beware that MSF60 doesn't seem to warn of upcoming leap second so we are likely to just lose sync
    // Possibly add additional processing around potential leap second changes? But they are very rare?
    bool DCF77_Flag_Decoder::get_leap_second_scheduled() {
        return leap_second_scheduled > 0;
    }

    void DCF77_Flag_Decoder::get_quality(uint8_t &uses_summertime_quality,
                                         uint8_t &timezone_change_scheduled_quality,
                                         uint8_t &leap_second_scheduled_quality) {
        uses_summertime_quality           = abs(uses_summertime);
        timezone_change_scheduled_quality = abs(timezone_change_scheduled);
        leap_second_scheduled_quality     = abs(leap_second_scheduled);
    }

    void DCF77_Flag_Decoder::debug() {
        sprint(F("Backup Antenna, TZ change, TZ, Leap scheduled, Date parity: "));
        sprint(abnormal_transmitter_operation, BIN);
        sprint(',');
        sprint(timezone_change_scheduled, DEC);
        sprint(',');
        sprint(uses_summertime, DEC);
        sprint(',');
        sprint(leap_second_scheduled, DEC);
        sprint(',');
        sprintln(date_parity, DEC);
    }
}

namespace Internal {  // DCF77_Decade_Decoder
    /*
    void DCF77_Decade_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 54: decade_data.val +=      tick_value; break;
            case 55: decade_data.val += 0x02*tick_value; break;
            case 56: decade_data.val += 0x04*tick_value; break;
            case 57: decade_data.val += 0x08*tick_value;
            hamming_binning<4, false>(decade_data); break;

            case 58: compute_max_index();
            // fall through on purpose
            default: decade_data.val = 0;
        }
    }
    */

    void DCF77_Decade_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
#ifndef MSF60
        BCD_binning<uint8_t, 1, 54, 4, false, true, true>(current_second, tick_value);
#else
        BCD_binning<uint8_t, 1, 17, 4, false, true, false>(current_second, tick_value);
#endif
    }

    void DCF77_Decade_Decoder::debug() {
        sprint(F("Decade: "));
        this->Binning::Decoder<uint8_t, 10>::debug();
    }
}

namespace Internal { // DCF77_Year_Decoder
    void DCF77_Year_Decoder::advance_tick() {
        Binning::Decoder<uint8_t, 10>::advance_tick();
        if (get_time_value().val == 0) {
            Decade_Decoder.advance_tick();
        }
    }

/*
    void DCF77_Year_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 50: year_data.val +=      tick_value; break;
            case 51: year_data.val +=  0x2*tick_value; break;
            case 52: year_data.val +=  0x4*tick_value; break;
            case 53: year_data.val +=  0x8*tick_value;
            hamming_binning<4, false>(year_data); break;

            case 54: compute_max_index();
            // fall through on purpose
            default: year_data.val = 0;
        }

        Decade_Decoder.process_tick(current_second, tick_value);
    }
*/
    void DCF77_Year_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {

#ifndef MSF60
        BCD_binning<uint8_t, 1, 50, 4, false, true, true>(current_second, tick_value);
#else
        BCD_binning<uint8_t, 1, 21, 4, false, true, false>(current_second, tick_value);
#endif

        Decade_Decoder.process_tick(current_second, tick_value);
    }

    void DCF77_Year_Decoder::get_quality(lock_quality_t &lock_quality) {
        Binning::Decoder<uint8_t, 10>::get_quality(lock_quality);

        lock_quality_t decade_lock_quality;
        Decade_Decoder.get_quality(decade_lock_quality);

        Arithmetic_Tools::minimize(lock_quality.lock_max, decade_lock_quality.lock_max);
        Arithmetic_Tools::maximize(lock_quality.noise_max, decade_lock_quality.noise_max);
    }

    uint8_t DCF77_Year_Decoder::get_quality_factor() {
        const uint8_t qf_years = Binning::Decoder<uint8_t, 10>::get_quality_factor();
        const uint8_t qf_decades = Decade_Decoder.get_quality_factor();
        return min(qf_years, qf_decades);
    }

    BCD::bcd_t DCF77_Year_Decoder::get_time_value() {
        BCD::bcd_t year = Binning::Decoder<uint8_t, 10>::get_time_value();
        BCD::bcd_t decade = Decade_Decoder.get_time_value();

        if (year.val == 0xff || decade.val == 0xff) {
            // undefined handling
            year.val = 0xff;
        } else {
            year.val += decade.val << 4;
        }

        return year;
    }

    void DCF77_Year_Decoder::setup() {
        Binning::Decoder<uint8_t, 10>::setup();
        Decade_Decoder.setup();
    }

    void DCF77_Year_Decoder::debug() {
        sprint(F("Year: "));
        Binning::Decoder<uint8_t, 10>::debug();
        Decade_Decoder.debug();
    }
}

namespace Internal {  // DCF77_Month_Decoder
/*
    void DCF77_Month_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 45: month_data.val +=      tick_value; break;
            case 46: month_data.val +=  0x2*tick_value; break;
            case 47: month_data.val +=  0x4*tick_value; break;
            case 48: month_data.val +=  0x8*tick_value; break;
            case 49: month_data.val += 0x10*tick_value;
            hamming_binning<5, false>(month_data); break;

            case 50: compute_max_index();
            // fall through on purpose
            default: month_data.val = 0;
        }
    }
*/
    void DCF77_Month_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {

#ifndef MSF60
        BCD_binning<uint8_t, 1, 45, 5, false, false, true>(current_second, tick_value);
#else
        BCD_binning<uint8_t, 1, 25, 5, false, false, false>(current_second, tick_value);
#endif

    }

    void DCF77_Month_Decoder::debug() {
        sprint(F("Month: "));
        Binning::Decoder<uint8_t, 12>::debug();
    }
}

namespace Internal {  // DCF77_Weekday_Decoder
/*
    void DCF77_Weekday_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 42: weekday_data.val +=      tick_value; break;
            case 43: weekday_data.val +=  0x2*tick_value; break;
            case 44: weekday_data.val +=  0x4*tick_value;
            hamming_binning<3, false>(weekday_data); break;
            case 45: compute_max_index();
            // fall through on purpose
            default: weekday_data.val = 0;
        }
    }
*/

    void DCF77_Weekday_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {

#ifndef MSF60
        BCD_binning<uint8_t, 1, 42, 3, false, false, true>(current_second, tick_value);
#else
        BCD_binning<uint8_t, 1, 36, 3, false, false, false>(current_second, tick_value);
#endif

}

    void DCF77_Weekday_Decoder::debug() {
        sprint(F("Weekday: "));
        Binning::Decoder<uint8_t, 7>::debug();
    }
}

namespace Internal {  // DCF77_Day_Decoder
/*
    void DCF77_Day_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 36: day_data.val +=      tick_value; break;
            case 37: day_data.val +=  0x2*tick_value; break;
            case 38: day_data.val +=  0x4*tick_value; break;
            case 39: day_data.val +=  0x8*tick_value; break;
            case 40: day_data.val += 0x10*tick_value; break;
            case 41: day_data.val += 0x20*tick_value;
            hamming_binning<6, false>(day_data); break;
            case 42: compute_max_index();
            // fall through on purpose
            default: day_data.val = 0;
        }
    }
*/

    void DCF77_Day_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {

#ifndef MSF60
        BCD_binning<uint8_t, 1, 36, 6, false, false, true>(current_second, tick_value);
#else
        BCD_binning<uint8_t, 1, 30, 6, false, false, false>(current_second, tick_value);
#endif

    }

    void DCF77_Day_Decoder::debug() {
        sprint(F("Day: "));
        Binning::Decoder<uint8_t, 31> ::debug();
    }
}

namespace Internal {  // DCF77_Hour_Decoder
/*
    void DCF77_Hour_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 29: hour_data.val +=      tick_value; break;
            case 30: hour_data.val +=  0x2*tick_value; break;
            case 31: hour_data.val +=  0x4*tick_value; break;
            case 32: hour_data.val +=  0x8*tick_value; break;
            case 33: hour_data.val += 0x10*tick_value; break;
            case 34: hour_data.val += 0x20*tick_value; break;
            case 35: hour_data.val += 0x80*tick_value;        // Parity !!!
                    hamming_binning<7, true>(hour_data); break;

            case 36: compute_max_index();
            // fall through on purpose
            default: hour_data.val = 0;
        }
    }
*/
    void DCF77_Hour_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {

#ifndef MSF60
        BCD_binning<uint8_t, 1, 29, 6, true, true, true>(current_second, tick_value);
#else
        BCD_binning<uint8_t, 1, 39, 6, false, true, false>(current_second, tick_value);
#endif

    }

    void DCF77_Hour_Decoder::debug() {
        sprint(F("Hour: "));
        Binning::Decoder<uint8_t, 24>::debug();
    }
}

namespace Internal {  // DCF77_Minute_Decoder
/*
    void DCF77_Minute_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {
        switch (current_second) {
            case 21: minute_data.val +=      tick_value; break;
            case 22: minute_data.val +=  0x2*tick_value; break;
            case 23: minute_data.val +=  0x4*tick_value; break;
            case 24: minute_data.val +=  0x8*tick_value; break;
            case 25: minute_data.val += 0x10*tick_value; break;
            case 26: minute_data.val += 0x20*tick_value; break;
            case 27: minute_data.val += 0x40*tick_value; break;
            case 28: minute_data.val += 0x80*tick_value;        // Parity !!!
                        hamming_binning<8, true>(minute_data);
                        break;
            case 29: compute_max_index();
            // fall through on purpose
            default: minute_data.val = 0;
        }
    }
*/

    void DCF77_Minute_Decoder::process_tick(const uint8_t current_second, const uint8_t tick_value) {

#ifndef MSF60
        BCD_binning<uint8_t, 1, 21, 7, true, true, true>(current_second, tick_value);
#else
        BCD_binning<uint8_t, 1, 45, 7, false, true, false>(current_second, tick_value);
#endif

    }

    void DCF77_Minute_Decoder::debug() {
        sprint(F("Minute: "));
        Binning::Decoder<uint8_t, 60>::debug();
    }
}

namespace Internal {  // DCF77_Second_Decoder
    void DCF77_Second_Decoder::setup() {
        Binning::Decoder<uint8_t, 60>::setup();
        prediction_match = convolution_binning_not_ready;
        buffered_match = convolution_binning_not_ready;
    }

    uint8_t DCF77_Second_Decoder::get_prediction_match() {
        return buffered_match;
    };

    void DCF77_Second_Decoder::set_convolution_time(const DCF77_Encoder &now) {
        DCF77_Encoder convolution_clock = now;

        // we are always decoding the data for the NEXT minute
        convolution_clock.advance_minute();

        // the convolution kernel shall have proper flag settings
        convolution_clock.autoset_control_bits();

        convolution_clock.get_serialized_clock_stream(convolution_kernel);
        prediction_match = 0;
    }

#ifndef MSF60
    void DCF77_Second_Decoder::convolution_binning(const uint8_t tick_data) {
        using namespace Arithmetic_Tools;

        // determine sync lock
        if (this->max - this->noise_max <= lock_threshold || get_time_value() == 3) {
            // after a lock is acquired this happens only once per minute and it is
            // reasonable cheap to process,
            //
            // that is: after we have a "lock" this will be processed whenever
            // the sync mark was detected

            compute_max_index();

            const uint8_t convolution_weight = 50;
            if (this->max > 255-convolution_weight) {
                // If we know we can not raise the maximum any further we
                // will lower the noise floor instead.
                for (uint8_t bin_index = 0; bin_index < seconds_per_minute; ++bin_index) {
                    bounded_decrement<convolution_weight>(this->data[bin_index]);
                }
                this->max -= convolution_weight;
                bounded_decrement<convolution_weight>(this->noise_max);
            }

            buffered_match = prediction_match;
        }

        if (tick_data == DCF77::sync_mark) {
            bounded_increment<6>(this->data[this->tick]);
            if (this->tick == this->max_index) {
                prediction_match += 6;
            }
        } else if (tick_data == DCF77::short_tick || tick_data == DCF77::long_tick) {
            uint8_t decoded_bit = (tick_data == DCF77::long_tick);

            // bit 0 always 0
            uint8_t bin = this->tick>0? this->tick-1: seconds_per_minute-1;
            const bool is_match = (decoded_bit == 0);
            this->data[bin] += is_match;
            if (bin == this->max_index) {
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

                    this->data[bin] += is_match;
                    if (bin == this->max_index) {
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

        this->tick = this->tick<seconds_per_minute-1? this->tick+1: 0;
    }
#else
    void DCF77_Second_Decoder::convolution_binning(const uint8_t tick_data) {
        using namespace Arithmetic_Tools;

        // determine sync lock
        if (this->max - this->noise_max <= lock_threshold || get_time_value() == 3) {
            // after a lock is acquired this happens only once per minute and it is
            // reasonable cheap to process,
            //
            // that is: after we have a "lock" this will be processed whenever
            // the sync mark was detected

            compute_max_index();

            const uint8_t convolution_weight = 50;
            if (this->max > 255-convolution_weight) {
                // If we know we can not raise the maximum any further we
                // will lower the noise floor instead.
                for (uint8_t bin_index = 0; bin_index < seconds_per_minute; ++bin_index) {
                    bounded_decrement<convolution_weight>(this->data[bin_index]);
                }
                this->max -= convolution_weight;
                bounded_decrement<convolution_weight>(this->noise_max);
            }

            buffered_match = prediction_match;
        }

        if (tick_data == DCF77::min_marker) {
            bounded_increment<50>(this->data[this->tick]);
            if (this->tick == this->max_index) {
                prediction_match += 50;
            }
        } else if (tick_data == DCF77::A0_B0 || tick_data == DCF77::A0_B1 || tick_data == DCF77::A1_B0 || tick_data == DCF77::A1_B1) {

            // bit 17 is where the convolution kernel starts
            uint8_t bin = this->tick;
            if (bin<17) bin += seconds_per_minute;
            bin -= 17;

            for (uint8_t current_byte_index = 0; current_byte_index < 6; current_byte_index++) {

                uint8_t current_byte_A_value = (&(convolution_kernel.A.byte_0))[current_byte_index];
                uint8_t current_byte_B_value = (&(convolution_kernel.B.byte_0))[current_byte_index];

                for (uint8_t current_bit_index = 0;
                     current_bit_index < 8 && !(current_byte_index == 5 && current_bit_index > 2);
                     current_bit_index++) {

                    const bool is_match = (tick_data == (((current_byte_A_value & 1)<< 1)  | (current_byte_B_value & 1)));

                    this->data[bin] += is_match;
                    if (bin == this->max_index) {
                        prediction_match += is_match;
                    }

                    current_byte_A_value >>= 1;
                    current_byte_B_value >>= 1;

                    if (bin<1) bin += seconds_per_minute;
                    bin -= 1;
                }
            }
        }

        this->tick = this->tick<seconds_per_minute-1? this->tick+1: 0;
    }
#endif

#ifndef MSF60
    void DCF77_Second_Decoder::sync_mark_binning(const uint8_t tick_data) {
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

        const uint8_t previous_tick = this->tick>0? this->tick-1: seconds_per_minute-1;
        const uint8_t previous_21_tick = this->tick>20? this->tick-21: this->tick + seconds_per_minute-21;

        switch (tick_data) {
            case DCF77::sync_mark:
                bounded_increment<6>(this->data[this->tick]);

                bounded_decrement<2>(this->data[previous_tick]);
                bounded_decrement<2>(this->data[previous_21_tick]);

                { const uint8_t next_tick = this->tick< seconds_per_minute-1? this->tick+1: 0;
                bounded_decrement<2>(this->data[next_tick]); }
                break;

            case DCF77::short_tick:
                bounded_increment<1>(this->data[previous_tick]);

                bounded_decrement<2>(this->data[this->tick]);
                bounded_decrement<2>(this->data[previous_21_tick]);
                break;

            case DCF77::long_tick:
                bounded_increment<1>(this->data[previous_21_tick]);

                bounded_decrement<2>(this->data[this->tick]);
                bounded_decrement<2>(this->data[previous_tick]);
                break;

            case DCF77::undefined:
            default:
                bounded_decrement<2>(this->data[this->tick]);
                bounded_decrement<2>(this->data[previous_tick]);
                bounded_decrement<2>(this->data[previous_21_tick]);
        }
        this->tick = this->tick<seconds_per_minute-1? this->tick+1: 0;

        // determine sync lock
        if (this->max - this->noise_max <=lock_threshold ||
            get_time_value() == 3) {
            // after a lock is acquired this happens only once per minute and it is
            // reasonable cheap to process,
            //
            // that is: after we have a "lock" this will be processed whenever
            // the sync mark was detected

            compute_max_index();
        }
    }
#else

    uint8_t DCF77_Second_Decoder::get_previous_n_tick(const uint8_t n) {
        return (tick >= n ? tick - n: tick + seconds_per_minute - n);
    }

    void DCF77_Second_Decoder::sync_mark_binning(const uint8_t tick_data) {
        // We use a binning approach to find out the proper phase.
        // The goal is to localize the sync_mark. Due to noise
        // there may be wrong marks of course. The idea is to not
        // only look at the statistics of the marks but to exploit
        // additional data properties:
        //
        // Bit A and B position 52 after a proper sync are both 0.
        // Bit A and B position 59 after a proper sync are both 0.
        // Bit A positions 1-16 after a proper sync are all 0.
        // Bit B positions 17-52 after a proper sync are all 0.
        //
        // The binning will work as follows:
        //   1) A sync mark will score +10 points for the current bin
        //   2) A "1" in bit A will score +1 points 53-58 bins back
        //  2b) A "0" in bit A will score +1 points for bins 1-16 back as well as 52 and 59 back
        //   3) A "0" in bit B will score +1 points for bins 17-52 and 59.
        //   4) An undefined value will score -2 point for the current bin
        //   5) Scores have an upper limit of 255 and a lower limit of 0.
        //
        // Summary: sync mark earns 10 points, a 0 in position 52 and 59 earn 1 bonus point
        //          anything that allows to infer that any of the "connected" positions is not a sync will remove 2 points
        //
        // It follows that the score of a sync mark (during good reception)
        // may move up/down the whole scale in slightly below 64 minutes.
        // If the receiver should glitch for whatever reason this implies
        // that the clock will take about 33 minutes to recover the proper
        // phase (during phases of good reception). During bad reception things
        // are more tricky.

        using namespace Arithmetic_Tools;

        switch (tick_data) {
            case DCF77::min_marker:
                bounded_increment<10>(data[tick]);
                break;

            case DCF77::A0_B0:
                for (uint8_t i=1; i<=51; ++i) {
                    bounded_increment<1>(this->data[get_previous_n_tick(i)]);
                }
                bounded_increment<2>(data[get_previous_n_tick(52)]);
                bounded_increment<2>(data[get_previous_n_tick(59)]);
                bounded_decrement<60>(data[tick]);
                break;

            case DCF77::A0_B1:
                for (uint8_t i=1; i<=16; ++i) {
                    bounded_increment<1>(data[get_previous_n_tick(i)]);
                }
                bounded_increment<1>(data[get_previous_n_tick(52)]);
                bounded_increment<1>(data[get_previous_n_tick(59)]);
                bounded_decrement<60>(data[tick]);
                break;

            case DCF77::A1_B0:
                for (uint8_t i=17; i<=59; ++i) {
                    bounded_increment<1>(data[get_previous_n_tick(i)]);
                }
                bounded_decrement<60>(data[tick]);
                break;

            case DCF77::A1_B1:
                for (uint8_t i=53; i<=58; ++i) {
                    bounded_increment<1>(data[get_previous_n_tick(i)]);
                }
                bounded_decrement<60>(data[tick]);
                break;

            case DCF77::undefined:
                bounded_decrement<55>(data[tick]);
                break;

            default:
                bounded_decrement<60>(data[tick]);
        }

        tick = tick<seconds_per_minute-1 ? tick+1: 0;

        // determine sync lock
        if (this->max - this->noise_max <=lock_threshold ||
            get_time_value() == 3) {
            // after a lock is acquired this happens only once per minute and it is
            // reasonable cheap to process,
            //
            // that is: after we have a "lock" this will be processed whenever
            // the sync mark was detected

            compute_max_index();
        }
    }
#endif

    uint8_t DCF77_Second_Decoder::get_time_value() {
        if (this->max - this->noise_max >= lock_threshold) {
            // at least one sync mark and a 0 and a 1 seen
            // the threshold is tricky:
            //   higher --> takes longer to acquire an initial lock, but higher probability of an accurate lock
            //
            //   lower  --> higher probability that the lock will oscillate at the beginning
            //              and thus spoil the downstream stages

            // we have to subtract 2 seconds
            //   1 because the seconds already advanced by 1 tick
            //   1 because the sync mark is not second 0 but second 59

#ifndef MSF60
    #define TICK_RETARD 2
#else
    #define TICK_RETARD 1
#endif

            uint8_t second = 2*seconds_per_minute + this->tick - TICK_RETARD - this->max_index;
            while (second >= seconds_per_minute) { second-= seconds_per_minute; }

            return second;
        } else {
            return 0xff;
        }
    }

    void DCF77_Second_Decoder::binning(const DCF77::tick_t tick_data) {
        if (prediction_match == convolution_binning_not_ready) {
            sync_mark_binning(tick_data);
        } else {
            convolution_binning(tick_data);
        }
    }

    void DCF77_Second_Decoder::debug() {
        static uint8_t prev_tick;

        if (prev_tick == this->tick) {
            return;
        } else {
            prev_tick = this->tick;

            sprint(F("second: "));
            sprint(get_time_value(), DEC);
            sprint(F(" Sync mark index "));
            Binning::Decoder<uint8_t, 60>::debug();
            sprint(F("Prediction Match: "));
            sprintln(prediction_match, DEC);
            sprintln();
        }
    }
}

namespace Internal {  // DCF77_Encoder
    uint8_t DCF77_Encoder::days_per_month() const {
        switch (month.val) {
            case 0x02:
                // valid till 31.12.2399
                // notice year mod 4 == year & 0x03
                return 28 + ((year.val != 0) && ((bcd_to_int(year) & 0x03) == 0)? 1: 0);
            case 0x01: case 0x03: case 0x05: case 0x07: case 0x08: case 0x10: case 0x12: return 31;
            case 0x04: case 0x06: case 0x09: case 0x11:                                  return 30;
            default: return 0;
        }
    }

    void DCF77_Encoder::reset() {
        second      = 0;
        minute.val  = 0x00;
        hour.val    = 0x00;
        day.val     = 0x01;
        month.val   = 0x01;
        year.val    = 0x00;
        weekday.val = 0x01;
        uses_summertime                = false;
        abnormal_transmitter_operation = false;
        timezone_change_scheduled      = false;
        leap_second_scheduled          = false;

        undefined_minute_output                         = false;
        undefined_uses_summertime_output                = false;
        undefined_abnormal_transmitter_operation_output = false;
        undefined_timezone_change_scheduled_output      = false;
    }

    uint8_t DCF77_Encoder::get_weekday() const {
        // attention: sunday will be ==0 instead of 7
        using namespace BCD;

        if (day.val <= 0x31 && month.val <= 0x12 && year.val <= 0x99) {
            // This will compute the weekday for each year in 2001-2099.
            // If you really plan to use my code beyond 2099 take care of this
            // on your own. My assumption is that it is even unclear if DCF77
            // will still exist then.

            // http://de.wikipedia.org/wiki/Gau%C3%9Fsche_Wochentagsformel
            const uint8_t  d = bcd_to_int(day);
            const uint16_t m = month.val <= 0x02? month.val + 10:
            bcd_to_int(month) - 2;
            const uint8_t  y = bcd_to_int(year) - (month.val <= 0x02);
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

    BCD::bcd_t DCF77_Encoder::get_bcd_weekday() const {
        BCD::bcd_t today;

        today.val = get_weekday();
        if (today.val == 0) {
            today.val = 7;
        }

        return today;
    }

    void DCF77_Encoder::autoset_weekday() {
        weekday = get_bcd_weekday();
    }

    void DCF77_Encoder::autoset_timezone() {
        // timezone change may only happen at the last sunday of march / october
        // the last sunday is always somewhere in [25-31]

        // Wintertime --> Summertime happens at 01:00 UTC == 02:00 CET == 03:00 CEST,
        // Summertime --> Wintertime happens at 01:00 UTC == 02:00 CET == 03:00 CEST

        if (month.val < 0x03) {
            // January or February
            uses_summertime = false;
        } else
        if (month.val == 0x03) {
            // March
            if (day.val < 0x25) {
                // Last Sunday of March must be 0x25-0x31
                // Thus still to early for summertime
                uses_summertime = false;
            } else
            if (uint8_t wd = get_weekday()) {
                // wd != 0 --> not a Sunday
                if (day.val - wd < 0x25) {
                    // early March --> wintertime
                    uses_summertime = false;
                } else {
                    // late march summertime
                    uses_summertime = true;
                }
            } else {
                // last sunday of march
                // decision depends on the current hour
                uses_summertime = (hour.val > 2);
            }
        } else
            if (month.val < 0x10) {
                // April - September
                uses_summertime = true;
            } else
            if (month.val == 0x10) {
                // October
                if (day.val < 0x25) {
                    // early October
                    uses_summertime = true;
                } else
                if (uint8_t wd = get_weekday()) {
                    // wd != 0 --> not a Sunday
                    if (day.val - wd < 0x25) {
                        // early October --> summertime
                        uses_summertime = true;
                    } else {
                        // late October --> wintertime
                        uses_summertime = false;
                    }
                } else {  // last sunday of october
                if (hour.val == 2) {
                    // can not derive the flag from time data
                    // this is the only time the flag is derived
                    // from the flag vector
                } else {
                    // decision depends on the current hour
                    uses_summertime = (hour.val < 2);
                }
            }
        } else {
            // November and December
            uses_summertime = false;
        }
    }

    void DCF77_Encoder::autoset_timezone_change_scheduled() {
        // summer/wintertime change will always happen
        // at clearly defined hours
        // http://www.gesetze-im-internet.de/sozv/__2.html

        // in doubt have a look here: http://www.dcf77logs.de/
        if (day.val < 0x25 || get_weekday() != 0) {
            // timezone change may only happen at the last sunday of march / october
            // the last sunday is always somewhere in [25-31]

            // notice that undefined (==0xff) day/weekday data will not cause any action
            timezone_change_scheduled = false;
        } else {
            if (month.val == 0x03) {
                if (uses_summertime) {
                    timezone_change_scheduled = (hour.val == 0x03 && minute.val == 0x00); // wintertime to summertime, preparing first minute of summertime
                } else {
                    timezone_change_scheduled = (hour.val == 0x01 && minute.val != 0x00); // wintertime to summertime
                }
            } else if (month.val == 0x10) {
                if (uses_summertime) {
                    timezone_change_scheduled = (hour.val == 0x02 && minute.val != 0x00); // summertime to wintertime
                } else {
                    timezone_change_scheduled = (hour.val == 0x02 && minute.val == 0x00); // summertime to wintertime, preparing first minute of wintertime
                }
            } else if (month.val <= 0x12) {
                timezone_change_scheduled = false;
            }
        }
    }

    bool DCF77_Encoder::verify_leap_second_scheduled(const bool assume_leap_second) const {
        // If day or month are unknown we default to "no leap second" because this is alway a very good guess.
        // If we do not know for sure we are either acquiring a lock right now --> we will easily recover from a wrong guess
        // or we have very noisy data --> the leap second bit is probably noisy as well --> we should assume the most likely case

        bool leap_second_scheduled = day.val == 0x01 && (assume_leap_second || leap_second_scheduled);

        // leap seconds will always happen
        // after 23:59:59 UTC and before 00:00 UTC == 01:00 CET == 02:00 CEST
        if (month.val == 0x01) {
            leap_second_scheduled &= ((hour.val == 0x00 && minute.val != 0x00) ||
                                    (hour.val == 0x01 && minute.val == 0x00));
        } else if (month.val == 0x07 || month.val == 0x04 || month.val == 0x10) {
            leap_second_scheduled &= ((hour.val == 0x01 && minute.val != 0x00) ||
                                    (hour.val == 0x02 && minute.val == 0x00));
        } else {
            leap_second_scheduled = false;
        }

        return leap_second_scheduled;
    }

    void DCF77_Encoder::autoset_control_bits() {
        autoset_weekday();
        autoset_timezone();
        autoset_timezone_change_scheduled();
        // we can not compute leap seconds, we can only verify if they might happen
        leap_second_scheduled = verify_leap_second_scheduled(false);
    }

    void DCF77_Encoder::advance_minute() {
        if (minute.val < 0x59) {
            increment(minute);
        } else if (minute.val == 0x59) {
            minute.val = 0x00;
            // in doubt have a look here: http://www.dcf77logs.de/
            if (timezone_change_scheduled && !uses_summertime && hour.val == 0x01) {
                // Wintertime --> Summertime happens at 01:00 UTC == 02:00 CET == 03:00 CEST,
                // the clock must be advanced from 01:59 CET to 03:00 CEST
                increment(hour);
                increment(hour);
                uses_summertime = true;
            }  else if (timezone_change_scheduled && uses_summertime && hour.val == 0x02) {
                // Summertime --> Wintertime happens at 01:00 UTC == 02:00 CET == 03:00,
                // the clock must be advanced from 02:59 CEST to 02:00 CET
                uses_summertime = false;
            } else {
                if (hour.val < 0x23) {
                    increment(hour);
                } else if (hour.val == 0x23) {
                    hour.val = 0x00;

                    if (weekday.val < 0x07) {
                        increment(weekday);
                    } else if (weekday.val == 0x07) {
                        weekday.val = 0x01;
                    }

                    if (bcd_to_int(day) < days_per_month()) {
                        increment(day);
                    } else if (bcd_to_int(day) == days_per_month()) {
                        day.val = 0x01;

                        if (month.val < 0x12) {
                            increment(month);
                        } else if (month.val == 0x12) {
                            month.val = 0x01;

                            if (year.val < 0x99) {
                                increment(year);
                            } else if (year.val == 0x99) {
                                year.val = 0x00;
                            }
                        }
                    }
                }
            }
        }
    }

    void DCF77_Encoder::advance_second() {
        // in case some value is out of range it will not be advanced
        // this is on purpose
        if (second < 59) {
            ++second;
            if (second == 15) {
                autoset_control_bits();
            }

        } else if (leap_second_scheduled && second == 59 && minute.val == 0x00) {
            second = 60;
            leap_second_scheduled = false;
        } else if (second == 59 || second == 60) {
            second = 0;
            advance_minute();
        }
    }

#ifndef MSF60
    DCF77::tick_t DCF77_Encoder::get_current_signal() const {
        using namespace DCF77;
        using namespace Arithmetic_Tools;

        if (second >= 1 && second <= 14) {
            // weather data or other stuff we can not compute
            return undefined;
        }

        bool result;
        switch (second) {
            case 0:  // start of minute
                return short_tick;

            case 15:
                if (undefined_abnormal_transmitter_operation_output) { return undefined; }
                result = abnormal_transmitter_operation; break;

            case 16:
                if (undefined_timezone_change_scheduled_output) { return undefined; }
                result = timezone_change_scheduled; break;

            case 17:
                if (undefined_uses_summertime_output) {return undefined; }
                result = uses_summertime; break;

            case 18:
                if (undefined_uses_summertime_output) {return undefined; }
                result = !uses_summertime; break;

            case 19:
                result = leap_second_scheduled; break;

            case 20:  // start of time information
                return long_tick;

            case 21:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = minute.digit.lo & 0x1; break;
            case 22:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = minute.digit.lo & 0x2; break;
            case 23:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = minute.digit.lo & 0x4; break;
            case 24:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = minute.digit.lo & 0x8; break;

            case 25:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = minute.digit.hi & 0x1; break;
            case 26:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = minute.digit.hi & 0x2; break;
            case 27:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = minute.digit.hi & 0x4; break;

            case 28:
                if (undefined_minute_output || minute.val > 0x59) { return undefined; }
                result = parity(minute.val); break;


            case 29:
                if (hour.val > 0x23) { return undefined; }
                result = hour.digit.lo & 0x1; break;
            case 30:
                if (hour.val > 0x23) { return undefined; }
                result = hour.digit.lo & 0x2; break;
            case 31:
                if (hour.val > 0x23) { return undefined; }
                result = hour.digit.lo & 0x4; break;
            case 32:
                if (hour.val > 0x23) { return undefined; }
                result = hour.digit.lo & 0x8; break;

            case 33:
                if (hour.val > 0x23) { return undefined; }
                result = hour.digit.hi & 0x1; break;
            case 34:
                if (hour.val > 0x23) { return undefined; }
                result = hour.digit.hi & 0x2; break;

            case 35:
                if (hour.val > 0x23) { return undefined; }
                result = parity(hour.val); break;

            case 36:
                if (day.val > 0x31) { return undefined; }
                result = day.digit.lo & 0x1; break;
            case 37:
                if (day.val > 0x31) { return undefined; }
                result = day.digit.lo & 0x2; break;
            case 38:
                if (day.val > 0x31) { return undefined; }
                result = day.digit.lo & 0x4; break;
            case 39:
                if (day.val > 0x31) { return undefined; }
                result = day.digit.lo & 0x8; break;

            case 40:
                if (day.val > 0x31) { return undefined; }
                result = day.digit.hi & 0x1; break;
            case 41:
                if (day.val > 0x31) { return undefined; }
                result = day.digit.hi & 0x2; break;

            case 42:
                if (weekday.val > 0x7) { return undefined; }
                result = weekday.val & 0x1; break;
            case 43:
                if (weekday.val > 0x7) { return undefined; }
                result = weekday.val & 0x2; break;
            case 44:
                if (weekday.val > 0x7) { return undefined; }
                result = weekday.val & 0x4; break;

            case 45:
                if (month.val > 0x12) { return undefined; }
                result = month.digit.lo & 0x1; break;
            case 46:
                if (month.val > 0x12) { return undefined; }
                result = month.digit.lo & 0x2; break;
            case 47:
                if (month.val > 0x12) { return undefined; }
                result = month.digit.lo & 0x4; break;
            case 48:
                if (month.val > 0x12) { return undefined; }
                result = month.digit.lo & 0x8; break;

            case 49:
                if (month.val > 0x12) { return undefined; }
                result = month.digit.hi & 0x1; break;

            case 50:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.lo & 0x1; break;
            case 51:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.lo & 0x2; break;
            case 52:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.lo & 0x4; break;
            case 53:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.lo & 0x8; break;

            case 54:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.hi & 0x1; break;
            case 55:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.hi & 0x2; break;
            case 56:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.hi & 0x4; break;
            case 57:
                if (year.val > 0x99) { return undefined; }
                result = year.digit.hi & 0x8; break;

            case 58:
                if (weekday.val > 0x07 ||
                    day.val     > 0x31 ||
                    month.val   > 0x12 ||
                    year.val    > 0x99) { return undefined; }

                    result = parity(day.digit.lo)   ^
                            parity(day.digit.hi)   ^
                            parity(month.digit.lo) ^
                            parity(month.digit.hi) ^
                            parity(weekday.val)    ^
                            parity(year.digit.lo)  ^
                            parity(year.digit.hi); break;

            case 59:
                // special handling for leap seconds
                if (leap_second_scheduled && minute.val == 0) { result = 0; break; }
                // standard case: fall through to "sync_mark"
            case 60:
                return sync_mark;

            default:
                return undefined;
        }

        return result? long_tick: short_tick;
    }
#else
    DCF77::tick_t DCF77_Encoder::get_current_signal() const {
        using namespace DCF77;
        using namespace Arithmetic_Tools;

        // With MSF signal two data bits are transmitted per second
        bool result_A;
        bool result_B;

        switch (second) {
            case 0:  // start of minute
                return min_marker; break;

            case 1 ... 16:  // DUT1 data we can not compute
                return undefined; break;

            case OFFSET_DECADE_8:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.hi & 0x8;
                result_B = 0; break;
            case OFFSET_DECADE_4:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.hi & 0x4;
                result_B = 0; break;
            case OFFSET_DECADE_2:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.hi & 0x2;
                result_B = 0; break;
            case OFFSET_DECADE_1:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.hi & 0x1;
                result_B = 0; break;
            case OFFSET_YEAR_8:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.lo & 0x8;
                result_B = 0; break;
            case OFFSET_YEAR_4:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.lo & 0x4;
                result_B = 0; break;
            case OFFSET_YEAR_2:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.lo & 0x2;
                result_B = 0; break;
            case OFFSET_YEAR_1:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = year.digit.lo & 0x1;
                result_B = 0; break;

            case OFFSET_MONTH_10:
                if (undefined_day_output || month.val > 0x12) { return undefined; }
                result_A = month.digit.hi & 0x1;
                result_B = 0; break;
            case OFFSET_MONTH_8:
                if (undefined_day_output || month.val > 0x12) { return undefined; }
                result_A = month.digit.lo & 0x8;
                result_B = 0; break;
            case OFFSET_MONTH_4:
                if (undefined_day_output || month.val > 0x12) { return undefined; }
                result_A = month.digit.lo & 0x4;
                result_B = 0; break;
            case OFFSET_MONTH_2:
                if (undefined_day_output || month.val > 0x12) { return undefined; }
                result_A = month.digit.lo & 0x2;
                result_B = 0; break;
            case OFFSET_MONTH_1:
                if (undefined_day_output || month.val > 0x12) { return undefined; }
                result_A = month.digit.lo & 0x1;
                result_B = 0; break;

            case OFFSET_DAY_20:
                if (undefined_day_output || day.val > 0x31) { return undefined; }
                result_A = day.digit.hi & 0x2; break;
                result_B = 0;
            case OFFSET_DAY_10:
                if (undefined_day_output || day.val > 0x31) { return undefined; }
                result_A = day.digit.hi & 0x1;
                result_B = 0; break;
            case OFFSET_DAY_8:
                if (undefined_day_output || day.val > 0x31) { return undefined; }
                result_A = day.digit.lo & 0x8;
                result_B = 0; break;
            case OFFSET_DAY_4:
                if (undefined_day_output || day.val > 0x31) { return undefined; }
                result_A = day.digit.lo & 0x4;
                result_B = 0; break;
            case OFFSET_DAY_2:
                if (undefined_day_output || day.val > 0x31) { return undefined; }
                result_A = day.digit.lo & 0x2;;
                result_B = 0; break;
            case OFFSET_DAY_1:
                if (undefined_day_output || day.val > 0x31) { return undefined; }
                result_A = day.digit.lo & 0x1;
                result_B = 0; break;

            case OFFSET_WEEKDAY_4:
                if (undefined_weekday_output || weekday.val > 0x7) { return undefined; }
                result_A = weekday.val & 0x4;
                result_B = 0; break;
            case OFFSET_WEEKDAY_2:
                if (undefined_weekday_output || weekday.val > 0x7) { return undefined; }
                result_A = weekday.val & 0x2;
                result_B = 0; break;
            case OFFSET_WEEKDAY_1:
                if (undefined_weekday_output || weekday.val > 0x7) { return undefined; }
                result_A = weekday.val & 0x1;
                result_B = 0; break;

            case OFFSET_HOUR_20:
                if (undefined_time_output || hour.val > 0x23) { return undefined; }
                result_A = hour.digit.hi & 0x2;
                result_B = 0; break;
            case OFFSET_HOUR_10:
                if (undefined_time_output || hour.val > 0x23) { return undefined; }
                result_A = hour.digit.hi & 0x1;
                result_B = 0; break;
            case OFFSET_HOUR_8:
                if (undefined_time_output || hour.val > 0x23) { return undefined; }
                result_A = hour.digit.lo & 0x8;
                result_B = 0; break;
            case OFFSET_HOUR_4:
                if (undefined_time_output || hour.val > 0x23) { return undefined; }
                result_A = hour.digit.lo & 0x4;
                result_B = 0; break;
            case OFFSET_HOUR_2:
                if (undefined_time_output || hour.val > 0x23) { return undefined; }
                result_A = hour.digit.lo & 0x2;
                result_B = 0; break;
            case OFFSET_HOUR_1:
                if (undefined_time_output || hour.val > 0x23) { return undefined; }
                result_A = hour.digit.lo & 0x1;
                result_B = 0; break;

            case OFFSET_MINUTE_40:
                if (undefined_time_output || minute.val > 0x59) { return undefined; }
                result_A = minute.digit.hi & 0x4;
                result_B = 0; break;
            case OFFSET_MINUTE_20:
                if (undefined_time_output || minute.val > 0x59) { return undefined; }
                result_A = minute.digit.hi & 0x2;
                result_B = 0; break;
            case OFFSET_MINUTE_10:
                if (undefined_time_output || minute.val > 0x59) { return undefined; }
                result_A = minute.digit.hi & 0x1;
                result_B = 0; break;
            case OFFSET_MINUTE_8:
                if (undefined_time_output || minute.val > 0x59) { return undefined; }
                result_A = minute.digit.lo & 0x8;
                result_B = 0; break;
            case OFFSET_MINUTE_4:
                if (undefined_time_output || minute.val > 0x59) { return undefined; }
                result_A = minute.digit.lo & 0x4;
                result_B = 0; break;
            case OFFSET_MINUTE_2:
                if (undefined_time_output || minute.val > 0x59) { return undefined; }
                result_A = minute.digit.lo & 0x2;
                result_B = 0; break;
            case OFFSET_MINUTE_1:
                if (undefined_time_output || minute.val > 0x59) { return undefined; }
                result_A = minute.digit.lo & 0x1;
                result_B = 0; break;

            case 52:
                result_A = 0; result_B = 0; break;

            case 53:
                if (undefined_timezone_change_scheduled_output) { return undefined; }
                result_A = 1;
                result_B = timezone_change_scheduled; break;

            case 54:
                if (undefined_year_output || year.val > 0x99) { return undefined; }
                result_A = 1;
                result_B = !parity(year.val); break;
            case 55:
                if (undefined_day_output || month.val > 0x12 || day.val > 0x31) { return undefined; }
                result_A = 1;
                result_B = !(parity(month.digit.hi) ^
                             parity(month.digit.lo) ^
                             parity(day.digit.hi) ^
                             parity(day.digit.lo)); break;
            case 56:
                if (undefined_weekday_output || weekday.val > 0x07) { return undefined; }
                result_A = 1;
                result_B = !parity(weekday.val); break;
            case 57:
                if (undefined_time_output || hour.val > 0x23 || minute.val > 0x59) { return undefined; }
                result_A = 1;
                result_B = !(parity(hour.digit.hi) ^
                             parity(hour.digit.lo) ^
                             parity(minute.digit.hi) ^
                             parity(minute.digit.lo)); break;

            case 58:
                if (undefined_uses_summertime_output) {return undefined; }
                result_A = 1;
                result_B = uses_summertime; break;

            case 59:
                result_A = 0; result_B = 0; break;

            default:
                return undefined;
        }

        return ((DCF77::tick_t)(result_A<<1 + result_B));
    }
#endif

#ifndef MSF60
    void DCF77_Encoder::get_serialized_clock_stream( DCF77::serialized_clock_stream &data) const {
        using namespace Arithmetic_Tools;

        // bit 16-20  // flags
        data.byte_0 = 0;
        data.byte_0 = set_bit(data.byte_0, 3, timezone_change_scheduled );
        data.byte_0 = set_bit(data.byte_0, 4, uses_summertime);
        data.byte_0 = set_bit(data.byte_0, 5, !uses_summertime);
        data.byte_0 = set_bit(data.byte_0, 6, leap_second_scheduled);
        data.byte_0 = set_bit(data.byte_0, 7, 1);

        // bit 21-28  // minutes
        data.byte_1 = set_bit(minute.val, 7, parity(minute.val));

        // bit 29-36  // hours, bit 0 of day
        data.byte_2 = set_bit(hour.val, 6, parity(hour.val));
        data.byte_2 = set_bit(data.byte_2, 7, day.bit.b0);


        // bit 37-44  // day + weekday
        data.byte_3 = day.val>>1 | weekday.val<<5;

        // bit 45-52  // month + bit 0-2 of year
        data.byte_4 = month.val | year.val<<5;

        const uint8_t date_parity = parity(day.digit.lo)   ^
                                    parity(day.digit.hi)   ^
                                    parity(month.digit.lo) ^
                                    parity(month.digit.hi) ^
                                    parity(weekday.val)    ^
                                    parity(year.digit.lo)  ^
                                    parity(year.digit.hi);
        // bit 53-58  // year + parity
        data.byte_5 = set_bit(year.val>>3, 5, date_parity);
    }
#else
    void DCF77_Encoder::get_serialized_clock_stream( DCF77::serialized_clock_stream_pair &data) const {
        using namespace Arithmetic_Tools;

        const uint8_t reverse_day = reverse(day.val);
        const uint8_t reverse_hour = reverse(hour.val);
        const uint8_t reverse_min = reverse(minute.val);

        // byte_0A:	bit 17-24	// year
        // byte_1A:	bit 25-32	// month + bit 0-2 day
        // byte_2A:	bit 33-40	// bit 3-5 day + weekday + bit 0-1 hour
        // byte_3A:	bit 41-48	// bit 2-5 hour + bit 0-3 minute
        // byte_4A:	bit 49-56	// bit 4-6 minute + 11110
        // byte_5A:	bit 57-59	// 011

        // bit A 17-24
        data.A.byte_0 = reverse(year.val);
        // bit A 25-32
        data.A.byte_1 = reverse(month.val)>>3 | reverse_day<<3;
        // bit A 33-40
        data.A.byte_2 = reverse_day>>5 | reverse(weekday.val)>>2 | reverse_hour<<4;
        // bit A 41-48
        data.A.byte_3 = reverse_hour>>4 | reverse_min<<3;
        // bit A 49-56
        data.A.byte_4 = reverse_min>>5 | 0b01111110<<3;
        // bit A 57-59
        data.A.byte_5 = 0b01111110>>5;

        // byte_0B:	bit 17-24   // 0
        // byte_1B:	bit 25-32	// 0
        // byte_2B:	bit 33-40	// 0
        // byte_3B:	bit 41-48	// 0
        // byte_4B:	bit 49-56	// 0000 + flags + parity
        // byte_5B:	bit 57-59	// flags + parity + 0

        data.B.byte_0 = 0;
        data.B.byte_1 = 0;
        data.B.byte_2 = 0;
        data.B.byte_3 = 0;

        // bit B 53-58 // flags and parity
        data.B.byte_4 = set_bit(0, 4, timezone_change_scheduled);
        data.B.byte_4 = set_bit(data.B.byte_4, 5, !parity(year.val));
        data.B.byte_4 = set_bit(data.B.byte_4, 6, !(parity(month.val)^parity(day.val)));
        data.B.byte_4 = set_bit(data.B.byte_4, 7, !parity(weekday.val));
        const uint8_t time_parity =	!(parity(hour.digit.hi) ^
                                         parity(hour.digit.lo) ^
                                         parity(minute.digit.hi) ^
                                         parity(minute.digit.lo));
        data.B.byte_5 = set_bit(0, 0, time_parity);
        data.B.byte_5 = set_bit(data.B.byte_5, 1, uses_summertime);
    }
#endif

    void DCF77_Encoder::debug() const {
        using namespace Debug;

        sprint(F("  "));
        bcddigits(year.val);
        sprint('.');
        bcddigits(month.val);
        sprint('.');
        bcddigits(day.val);
        sprint('(');
        bcddigit(weekday.val);
        sprint(',');
        bcddigit(get_weekday());
        sprint(')');
        bcddigits(hour.val);
        sprint(':');
        bcddigits(minute.val);
        sprint(':');
        if (second < 10) {
            sprint('0');
        }
        sprint(second, DEC);
        if (uses_summertime) {
            sprint(F(" MESZ "));
        } else {
            sprint(F(" MEZ "));
        }
        sprint(leap_second_scheduled);
        sprint(',');
        sprint(timezone_change_scheduled);
        sprint(' ');
    }

    void DCF77_Encoder::debug(const uint16_t cycles) const {
        DCF77_Encoder local_clock = *this;
        DCF77_Encoder decoded_clock;

        sprint(F("M ?????????????? RAZZA S mmmmMMMP hhhhHHP ddddDD www mmmmM yyyyYYYYP S"));
        for (uint16_t second = 0; second < cycles; ++second) {
            switch (local_clock.second) {
                case  0: sprintln(); break;
                case  1: case 15: case 20: case 21: case 29:
                case 36: case 42: case 45: case 50: case 59: sprint(' ');
            }

            const DCF77::tick_t tick_data = local_clock.get_current_signal();
            Debug::debug_helper(tick_data);

            DCF77_Naive_Bitstream_Decoder::set_bit(local_clock.second, tick_data, decoded_clock);

            local_clock.advance_second();

            if (local_clock.second == 0) {
                decoded_clock.debug();
            }
        }

        sprintln();
        sprintln();
    }
}

namespace Internal {
    namespace DCF77_Naive_Bitstream_Decoder {
        using namespace DCF77;

#ifndef MSF60
        void set_bit(const uint8_t second, const uint8_t value, DCF77_Encoder &now) {
            // The naive value is a way to guess a value for unclean decoded data.
            // It is obvious that this is not necessarily a good value but better
            // than nothing.
            const uint8_t naive_value = (value == long_tick || value == undefined)? 1: 0;
            const uint8_t is_value_bad = value != long_tick && value != short_tick;

            now.second = second;

            switch (second) {
                case 15: now.abnormal_transmitter_operation = naive_value; break;
                case 16: now.timezone_change_scheduled      = naive_value; break;

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
#else

        void set_bit(const uint8_t second, const uint8_t value, DCF77_Encoder &now) {
            // The naive value is a way to guess a value for unclean decoded data.
            // It is obvious that this is not necessarily a good value but better
            // than nothing.
            const bool naive_value = (value == A1_B0 || value == A1_B1 || value == undefined)? 1: 0;
            const bool naive_value_B = (value == A0_B1 || value == A1_B1 || value == undefined)? 1: 0;
            const bool is_value_bad = value != A0_B1 && value != A1_B1 && value != A0_B0 && value != A0_B1;

            now.second = second;

            switch (second) {
                case 16: now.year.val = 0x00; now.undefined_year_output = false; break;

                case OFFSET_DECADE_8: now.year.val += 0x80*naive_value; break;
                case OFFSET_DECADE_4: now.year.val += 0x40*naive_value; break;
                case OFFSET_DECADE_2: now.year.val += 0x20*naive_value; break;
                case OFFSET_DECADE_1: now.year.val += 0x10*naive_value; break;

                case OFFSET_YEAR_8: now.year.val +=  0x8*naive_value; break;
                case OFFSET_YEAR_4: now.year.val +=  0x4*naive_value; break;
                case OFFSET_YEAR_2: now.year.val +=  0x2*naive_value; break;
                case OFFSET_YEAR_1: now.year.val +=      naive_value;
                    now.month.val = 0x00; now.undefined_day_output = false; break;

                case OFFSET_MONTH_10: now.month.val += 0x10*naive_value; break;
                case OFFSET_MONTH_8: now.month.val +=  0x8*naive_value; break;
                case OFFSET_MONTH_4: now.month.val +=  0x4*naive_value; break;
                case OFFSET_MONTH_2: now.month.val +=  0x2*naive_value; break;
                case OFFSET_MONTH_1: now.month.val +=      naive_value;
                    now.day.val = 0x00; break;

                case OFFSET_DAY_20: now.day.val += 0x20*naive_value; break;
                case OFFSET_DAY_10: now.day.val += 0x10*naive_value; break;
                case OFFSET_DAY_8: now.day.val +=  0x8*naive_value; break;
                case OFFSET_DAY_4: now.day.val +=  0x4*naive_value; break;
                case OFFSET_DAY_2: now.day.val +=  0x2*naive_value; break;
                case OFFSET_DAY_1: now.day.val +=      naive_value;
                    now.weekday.val = 0x00; now.undefined_weekday_output = false; break;

                case OFFSET_WEEKDAY_4: now.weekday.val += 0x4*naive_value; break;
                case OFFSET_WEEKDAY_2: now.weekday.val += 0x2*naive_value; break;
                case OFFSET_WEEKDAY_1: now.weekday.val +=     naive_value;
                    now.hour.val = 0; now.undefined_time_output = false; break;

                case OFFSET_HOUR_20: now.hour.val += 0x20*naive_value; break;
                case OFFSET_HOUR_10: now.hour.val += 0x10*naive_value; break;
                case OFFSET_HOUR_8: now.hour.val +=  0x8*naive_value; break;
                case OFFSET_HOUR_4: now.hour.val +=  0x4*naive_value; break;
                case OFFSET_HOUR_2: now.hour.val +=  0x2*naive_value; break;
                case OFFSET_HOUR_1: now.hour.val +=      naive_value;
                    now.minute.val = 0x00; break;

                case OFFSET_MINUTE_40: now.minute.val += 0x40*naive_value; break;
                case OFFSET_MINUTE_20: now.minute.val += 0x20*naive_value; break;
                case OFFSET_MINUTE_10: now.minute.val += 0x10*naive_value; break;
                case OFFSET_MINUTE_8: now.minute.val +=  0x8*naive_value; break;
                case OFFSET_MINUTE_4: now.minute.val +=  0x4*naive_value; break;
                case OFFSET_MINUTE_2: now.minute.val +=  0x2*naive_value; break;
                case OFFSET_MINUTE_1: now.minute.val +=      naive_value; break;

                case 53: now.timezone_change_scheduled = naive_value_B;
                    now.undefined_timezone_change_scheduled_output = is_value_bad; break;

                case 58: now.uses_summertime = naive_value_B;
                    now.undefined_uses_summertime_output = is_value_bad; break;
            }
        }
#endif

    }
}

namespace Internal {  // DCF77_Clock_Controller<Frequency_Control>
    // static member definitions for DCF77_Clock_Controller<Frequency_Control>
    template <typename Frequency_Control> Clock::output_handler_t DCF77_Clock_Controller<Frequency_Control>::output_handler = 0;

    template <typename Frequency_Control> DCF77_Second_Decoder  DCF77_Clock_Controller<Frequency_Control>::Second_Decoder;
    template <typename Frequency_Control> DCF77_Minute_Decoder  DCF77_Clock_Controller<Frequency_Control>::Minute_Decoder;
    template <typename Frequency_Control> DCF77_Hour_Decoder    DCF77_Clock_Controller<Frequency_Control>::Hour_Decoder;
    template <typename Frequency_Control> DCF77_Weekday_Decoder DCF77_Clock_Controller<Frequency_Control>::Weekday_Decoder;
    template <typename Frequency_Control> DCF77_Day_Decoder     DCF77_Clock_Controller<Frequency_Control>::Day_Decoder;
    template <typename Frequency_Control> DCF77_Month_Decoder   DCF77_Clock_Controller<Frequency_Control>::Month_Decoder;
    template <typename Frequency_Control> DCF77_Year_Decoder    DCF77_Clock_Controller<Frequency_Control>::Year_Decoder;
    template <typename Frequency_Control> DCF77_Flag_Decoder    DCF77_Clock_Controller<Frequency_Control>::Flag_Decoder;

    template <typename Frequency_Control> uint8_t       DCF77_Clock_Controller<Frequency_Control>::leap_second;
    template <typename Frequency_Control> DCF77_Encoder DCF77_Clock_Controller<Frequency_Control>::decoded_time;

    template <typename Frequency_Control> DCF77_Local_Clock<DCF77_Clock_Controller<Frequency_Control> >
        DCF77_Clock_Controller<Frequency_Control>::Local_Clock;

    template <typename Frequency_Control> DCF77_Demodulator<DCF77_Clock_Controller<Frequency_Control> >
        DCF77_Clock_Controller<Frequency_Control>::Demodulator;
}

namespace DCF77_Clock {
    using namespace Internal;

    typedef DCF77_Clock_Controller<DCF77_Frequency_Control> Clock_Controller;

    void setup() {
        Clock_Controller::setup();
    }

    void setup(const Clock::input_provider_t input_provider, const Clock::output_handler_t output_handler) {
        Clock_Controller::setup();
        Clock_Controller::set_output_handler(output_handler);
        Generic_1_kHz_Generator::setup(input_provider);
    };

    void debug() {
        Clock_Controller::debug();
    }

    void set_input_provider(const Clock::input_provider_t input_provider) {
        Generic_1_kHz_Generator::setup(input_provider);
    }

    void set_output_handler(const Clock::output_handler_t output_handler) {
        Clock_Controller::set_output_handler(output_handler);
    }

    #if defined(__AVR__)
    void auto_persist() {
        Clock_Controller::auto_persist();
    }
    #endif

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

    void get_current_time(Clock::time_t &now) {
        DCF77_Encoder current_time;
        Clock_Controller::get_current_time(current_time);

        convert_time(current_time, now);
    };

    void read_current_time(Clock::time_t &now) {
        DCF77_Encoder current_time;
        Clock_Controller::read_current_time(current_time);

        convert_time(current_time, now);
    };

    void read_future_time(Clock::time_t &now_plus_1s) {
        DCF77_Encoder current_time;
        Clock_Controller::read_current_time(current_time);
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
        return Clock_Controller::get_overall_quality_factor();
    };

    Clock::clock_state_t get_clock_state() {
        return Clock_Controller::get_clock_state();
    };

    uint8_t get_prediction_match() {
        return Clock_Controller::get_prediction_match();
    };
}

namespace Internal {  // DCF77_Frequency_Control
    volatile int8_t DCF77_Frequency_Control::confirmed_precision = 0;
    // get the adjust step that was used for the last adjustment
    //   if there was no adjustment or if the phase drift was poor it will return 0
    //   if the adjustment was from eeprom it will return the negative value of the
    //   persisted adjust step
    int8_t DCF77_Frequency_Control::get_confirmed_precision() {
        return confirmed_precision;
    }

    void DCF77_Frequency_Control::qualify_calibration() {
        calibration_state.qualified = true;
    };

    void DCF77_Frequency_Control::unqualify_calibration() {
        calibration_state.qualified = false;
    };

    DCF77_Frequency_Control::deviation_tracker_t DCF77_Frequency_Control::deviation_tracker;
    int16_t DCF77_Frequency_Control::compute_phase_deviation(uint8_t current_second, uint8_t current_minute_mod_10) {
        return deviation_tracker.compute_phase_deviation(current_second, current_minute_mod_10);
    }

    volatile DCF77_Frequency_Control::calibration_state_t DCF77_Frequency_Control::calibration_state = {false ,false};
    DCF77_Frequency_Control::calibration_state_t DCF77_Frequency_Control::get_calibration_state() {
        return *(calibration_state_t *)&calibration_state;
    }

    volatile int16_t DCF77_Frequency_Control::deviation;
    int16_t DCF77_Frequency_Control::get_current_deviation() {
        return deviation;
    }

    void DCF77_Frequency_Control::adjust() {
        int16_t total_adjust = Generic_1_kHz_Generator::read_adjustment();
        // The proper formula would be
        //     int32_t adjust == (16 000 000 / (elapsed_minutes * 60 * phase_lock_resolution)) * new_deviation;
        // The total error of the formula below is ~ 1/(3*elapsed_minutes)
        //     which is  ~ 1/1000 (for centisecond resolution) and ~1/100 for millisecond resolution
        // Also notice that 2667*deviation will not overflow even if the
        // local clock would deviate by more than 400 ppm or 6 kHz
        // from its nominal frequency.
        // Finally notice that the frequency_offset will always be rounded towards zero
        // while the confirmed_precision is rounded away from zero. The first should
        // be considered a kind of relaxation while the second should be considered
        // a defensive computation.
        const int16_t minutes_per_16000000_ticks = 16000000uL / 60 / Configuration::phase_lock_resolution;
        const int16_t frequency_offset = ((minutes_per_16000000_ticks * (int32_t)deviation) /
                                           deviation_tracker.elapsed_minutes);
        // In doubt confirmed precision will be slightly larger than the true value
        confirmed_precision = ((minutes_per_16000000_ticks - 1) + deviation_tracker.elapsed_minutes) /
                                deviation_tracker.elapsed_minutes;
        if (confirmed_precision == 0) { confirmed_precision = 1; }

        total_adjust -= frequency_offset;

        if (total_adjust >  max_total_adjust) { total_adjust =  max_total_adjust; }
        if (total_adjust < -max_total_adjust) { total_adjust = -max_total_adjust; }

        Generic_1_kHz_Generator::adjust(total_adjust);
    }

    void DCF77_Frequency_Control::process_1_Hz_tick(const DCF77_Encoder &decoded_time) {
        const int16_t deviation_to_trigger_readjust = 5;

        deviation = compute_phase_deviation(decoded_time.second, decoded_time.minute.digit.lo);

        if (decoded_time.second == calibration_second) {
            // We might be in an unqualified state and thus the leap second information
            // we have might be wrong.
            // However if we fail to detect an actual leap second, calibration will be wrong
            // by 1 second.
            // Therefore we assume a leap second and verify_leap_second_scheduled()
            // will tell us if our assumption could actually be a leap second.
            if (decoded_time.verify_leap_second_scheduled(true)) {
                // Leap seconds will mess up our frequency computations.
                // Handling them properly would be slightly more complicated.
                // Since leap seconds may only happen every 3 months we just
                // stop calibration for leap seconds and do nothing else.
                calibration_state.running = false;
            }

            if (calibration_state.running) {
                if (calibration_state.qualified) {
                    if ((deviation_tracker.good_enough() && abs(deviation) >= deviation_to_trigger_readjust) ||
                         deviation_tracker.timeout()) {
                        adjust();
                        DCF77_Clock_Controller<DCF77_Frequency_Control>::on_tuned_clock();

                        #if defined(_AVR_EEPROM_H_)
                        // enqueue write to eeprom
                        data_pending = true;
                        #endif
                        // restart calibration next second
                        calibration_state.running = false;
                    }
                } else {
                    // unqualified
                    if (deviation_tracker.timeout()) {
                        // running unqualified for more than tau minutes
                        //   --> the current calibration attempt is doomed
                        calibration_state.running = false;
                    }
                    // else running but unqualified --> wait for better state
                }
            } else {
                // (calibration_state.running == false) --> waiting
                if (calibration_state.qualified) {
                    deviation_tracker.start(decoded_time.minute.digit.lo);
                    calibration_state.running = true;
                }
                // else waiting but unqualified --> nothing to do
            }
        }
    }

    void DCF77_Frequency_Control::process_1_kHz_tick() {
        deviation_tracker.process_tick();
    }

    #if defined(_AVR_EEPROM_H_)
    volatile boolean DCF77_Frequency_Control::data_pending = false;

    // ID constants to see if EEPROM has already something stored
    static const char ID_u = 'u';
    static const char ID_k = 'k';
    void DCF77_Frequency_Control::persist_to_eeprom(const int8_t precision, const int16_t adjust) {
        // this is slow, do not call during interrupt handling
        uint16_t eeprom = DCF77_Frequency_Control::eeprom_base;
        eeprom_write_byte((uint8_t *)(eeprom++), ID_u);
        eeprom_write_byte((uint8_t *)(eeprom++), ID_k);
        eeprom_write_byte((uint8_t *)(eeprom++), (uint8_t) precision);
        eeprom_write_byte((uint8_t *)(eeprom++), (uint8_t) precision);
        eeprom_write_word((uint16_t *)eeprom, (uint16_t) adjust);
        eeprom += 2;
        eeprom_write_word((uint16_t *)eeprom, (uint16_t) adjust);
    }

    void DCF77_Frequency_Control::read_from_eeprom(int8_t &precision, int16_t &adjust) {
        uint16_t eeprom = DCF77_Frequency_Control::eeprom_base;
        if (eeprom_read_byte((const uint8_t *)(eeprom++)) == ID_u &&
            eeprom_read_byte((const uint8_t *)(eeprom++)) == ID_k) {
            uint8_t ee_precision = eeprom_read_byte((const uint8_t *)(eeprom++));
            if (ee_precision == eeprom_read_byte((const uint8_t *)(eeprom++))) {
                const uint16_t ee_adjust = eeprom_read_word((const uint16_t *)eeprom);
                eeprom += 2;
                if (ee_adjust == eeprom_read_word((const uint16_t *)eeprom)) {
                    precision = (int8_t) ee_precision;
                    adjust = (int16_t) ee_adjust;
                    return;
                }
            }
        }
        precision = 0;
        adjust = 0;
    }

    // do not call during ISRs
    void DCF77_Frequency_Control::auto_persist() {
        // ensure that reading of data can not be interrupted!!
        // do not write EEPROM while interrupts are blocked
        int16_t adjust;
        int8_t  precision;
        CRITICAL_SECTION {
            if (data_pending && confirmed_precision > 0) {
                precision = confirmed_precision;
                adjust = Generic_1_kHz_Generator::read_adjustment();
            } else {
                data_pending = false;
            }
        }
        if (data_pending) {
            int16_t ee_adjust;
            int8_t  ee_precision;
            read_from_eeprom(ee_precision, ee_adjust);

            if (confirmed_precision < abs(ee_precision) ||        // - precision better than it used to be
                ( abs(ee_precision) < 8 &&                        // - precision better than 8 Hz or 0.5 ppm @ 16 MHz
                  abs(ee_adjust-adjust) > 8 )           ||        //   deviation worse than 8 Hz (thus 16 Hz or 1 ppm)
                ( confirmed_precision == 1 &&                     // - It takes more than 1 day to arrive at 1 Hz precision
                  abs(ee_adjust-adjust) > 0 ) )                   //   thus it acceptable to always write
            {
                int16_t new_ee_adjust;
                int8_t  new_ee_precision;
                CRITICAL_SECTION {
                    new_ee_adjust    = adjust;
                    new_ee_precision = precision;
                }
                persist_to_eeprom(new_ee_precision, new_ee_adjust);
            }
            data_pending = false;
        }
    }

    void DCF77_Frequency_Control::setup() {
        int16_t adjust;
        int8_t ee_precision;

        read_from_eeprom(ee_precision, adjust);
        if (ee_precision) {
            DCF77_Clock_Controller<DCF77_Frequency_Control>::on_tuned_clock();
        }
        Generic_1_kHz_Generator::adjust(adjust);
    }
    #else
    void DCF77_Frequency_Control::setup() {}
    #endif
    void DCF77_Frequency_Control::debug() {
        sprintln(F("confirmed_precision ?? adjustment, deviation, elapsed"));
        sprint(confirmed_precision);
        sprint(F(" pp16m "));
        sprint(calibration_state.running? '@': '.');
        sprint(calibration_state.qualified? '+': '-');
        sprint(' ');

        sprint(F(", "));
        sprint(Generic_1_kHz_Generator::read_adjustment());
        sprint(F(" pp16m, "));

        sprint(deviation);
        sprint(F(" ticks, "));

        sprint(deviation_tracker.elapsed_minutes);
        sprint(F(" min + "));
        sprint(deviation_tracker.elapsed_ticks_mod_60000);
        sprintln(F(" ticks mod 60000"));
    }

    void DCF77_No_Frequency_Control::process_1_Hz_tick(const DCF77_Encoder &decoded_time) {}
    void DCF77_No_Frequency_Control::process_1_kHz_tick() {}
    void DCF77_No_Frequency_Control::qualify_calibration() {}
    void DCF77_No_Frequency_Control::unqualify_calibration() {}
    void DCF77_No_Frequency_Control::setup() {}
    #if defined(__AVR__)
    void DCF77_No_Frequency_Control::auto_persist() {}
    #endif
}

namespace Internal {
    namespace Generic_1_kHz_Generator {
        uint8_t zero_provider() {
            return 0;
        }

        static Clock::input_provider_t the_input_provider = zero_provider;
        static int16_t adjust_pp16m = 0;
        static int32_t cumulated_phase_deviation = 0;

        void adjust(const int16_t pp16m) {
            CRITICAL_SECTION {
                // positive_value --> increase frequency
                adjust_pp16m = pp16m;
            }
        }

        int16_t read_adjustment() {
            // positive_value --> increase frequency
            CRITICAL_SECTION {
                const int16_t pp16m = adjust_pp16m;
                return pp16m;
            }
        }

        #if defined(__AVR_ATmega168__)  || \
            defined(__AVR_ATmega48__)   || \
            defined(__AVR_ATmega88__)   || \
            defined(__AVR_ATmega328P__) || \
            defined(__AVR_ATmega1280__) || \
            defined(__AVR_ATmega2560__) || \
            defined(__AVR_AT90USB646__) || \
            defined(__AVR_AT90USB1286__)
        void init_timer_2() {
            // Timer 2 CTC mode, prescaler 64
            TCCR2B = (0<<WGM22) | (1<<CS22);
            TCCR2A = (1<<WGM21) | (0<<WGM20);

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

        void setup(const Clock::input_provider_t input_provider) {
            init_timer_2();
            stop_timer_0();
            the_input_provider = input_provider;
        }

        void isr_handler() {
            cumulated_phase_deviation += adjust_pp16m;
            // 1 / 250 / 64000 = 1 / 16 000 000
            if (cumulated_phase_deviation >= 64000) {
                cumulated_phase_deviation -= 64000;
                // cumulated drift exceeds 1 timer step (4 microseconds)
                // drop one timer step to realign
                OCR2A = 248;
            } else
            if (cumulated_phase_deviation <= -64000) {
                // cumulated drift exceeds 1 timer step (4 microseconds)
                // insert one timer step to realign
                cumulated_phase_deviation += 64000;
                OCR2A = 250;
            } else {
                // 249 + 1 == 250 == 250 000 / 1000 =  (16 000 000 / 64) / 1000
                OCR2A = 249;
            }

            Clock_Controller::process_1_kHz_tick_data(the_input_provider());
        }
        #endif

        #if defined(__SAM3X8E__)
        void setup(const Clock::input_provider_t input_provider) {
            // no need to init systicks timer as it runs @1kHz anyway
            the_input_provider = input_provider;
        }

        const uint32_t ticks_per_ms = SystemCoreClock/1000;
        const uint32_t ticks_per_us = ticks_per_ms/1000;

        void isr_handler() {
            cumulated_phase_deviation += adjust_pp16m;
            if (cumulated_phase_deviation >= 16000) {
                cumulated_phase_deviation -= 16000;
                // cumulated drift exceeds microsecond)
                // drop microsecond step to realign
                SysTick->LOAD = ticks_per_ms - ticks_per_us;
            } else if (cumulated_phase_deviation <= -16000) {
                cumulated_phase_deviation += 16000;
                // cumulated drift exceeds 1 microsecond
                // insert one microsecond to realign
                SysTick->LOAD = ticks_per_ms + ticks_per_us;
            } else {
                SysTick->LOAD = ticks_per_ms;
            }

            Clock_Controller::process_1_kHz_tick_data(the_input_provider());
        }
        #endif

        #ifdef STANDALONE
            void setup(const Clock::input_provider_t input_provider) {
                the_input_provider=input_provider;
            }
            void isr_handler() {
                Clock_Controller::process_1_kHz_tick_data(the_input_provider());
            }
        #endif
    }
}

#if defined(__AVR_ATmega168__)  || \
    defined(__AVR_ATmega48__)   || \
    defined(__AVR_ATmega88__)   || \
    defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega1280__) || \
    defined(__AVR_ATmega2560__) || \
    defined(__AVR_AT90USB646__) || \
    defined(__AVR_AT90USB1286__)
ISR(TIMER2_COMPA_vect) {
    Internal::Generic_1_kHz_Generator::isr_handler();
}
#endif

#if defined(__SAM3X8E__)
extern "C" {
    // sysTicks will be triggered once per 1 ms
    int sysTickHook(void) {
        Internal::Generic_1_kHz_Generator::isr_handler();
        return 0;
    }
}
#endif
