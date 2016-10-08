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
#include <avr/eeprom.h>

const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = A5;  // A5 == d19
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 1;
// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_pin = A4;  // A4 == d18

// will take pins 2-17 for time switch output
const uint8_t time_switch_channel_0_pin = 2;



void space() {
    Serial.print(' ');
}

void digit_print(uint8_t d) {
    if (d==0xf) {
        Serial.print('*');
    } else {
        Serial.print(d, 16);
    }
}

void padded_print(BCD::bcd_t n) {
    digit_print(n.digit.hi);
    digit_print(n.digit.lo);
}

void hex_print(const uint8_t b) {
    Serial.print(b >> 4, 16);
    Serial.print(b & 0xf, 16);
}

void hex_print(const uint16_t w) {
    hex_print((uint8_t)(w>>8));
    hex_print((uint8_t) w);
}

void binary_print(const uint8_t b) {
    uint8_t out = b;
    for (uint8_t d=0; d<8; ++d) {
        if (d==4) {
            space();
        }
        Serial.print((out & 0x80) != 0);
        out <<= 1;
    }
}

void binary_print(const uint16_t w) {
    binary_print((uint8_t)(w>>8));
    space();
    binary_print((uint8_t)w);
}


namespace Alarm_Timer {
    typedef uint16_t channels_t;

    const uint8_t bitmask_monday     = 1                 << 1;
    const uint8_t bitmask_tuesday    = bitmask_monday    << 1;
    const uint8_t bitmask_wednesday  = bitmask_tuesday   << 1;
    const uint8_t bitmask_thursday   = bitmask_wednesday << 1;
    const uint8_t bitmask_friday     = bitmask_thursday  << 1;
    const uint8_t bitmask_saturday   = bitmask_friday    << 1;
    const uint8_t bitmask_sunday     = bitmask_saturday  << 1;

    typedef struct {
        channels_t channels;

        struct {
            // active == 1 --> will trigger alarm
            // active == 0 --> will be ignored as a trigger, group end marker will NOT be ignored
            uint8_t active:1;
            // marker == 0 --> alarm belongs to a group of alarms, there are more to follow
            // marker == 1 --> alarm is the last alarm of a group of alarms
            uint8_t group_end_marker:1;
        } control;

        // binary flags --> 1: alarm will be relevant for this day
        // starting from bitmask monday to sunday
        uint8_t weekdays;

        // FF == every hour
        // 0..23
        BCD::bcd_t hour;
        // 0..59
        // FF == every minute,
        // Fn == minutes n, 1n, 2n ..., 5n
        BCD::bcd_t minute;
        // 0..60
        // FF == every second
        // Fn == seconds n, 1n, 2n ..., 5n
        BCD::bcd_t second;
    } alarm_t;

    void set_alarm(
        alarm_t & alarm,
        const uint16_t channels_as_binary,
        const uint8_t active,
        const uint8_t group_end_marker,
        const uint8_t weekdays,
        const uint8_t bcd_hour,
        const uint8_t bcd_minute,
        const uint8_t bcd_second
    ) {
        alarm.channels            = channels_as_binary;
        alarm.control.active           = active;
        alarm.control.group_end_marker = group_end_marker;
        alarm.weekdays                 = weekdays;
        alarm.hour.val                 = bcd_hour;
        alarm.minute.val               = bcd_minute;
        alarm.second.val               = bcd_second;
    }

    void print_alarm(const alarm_t & alarm) {
        Serial.print(F("A"));
        Serial.print(alarm.control.active);

        Serial.print(F(", C x"));
        hex_print(alarm.channels);

        Serial.print(F(", C b"));
        binary_print(alarm.channels);

        Serial.print(F(", D "));

        for (uint8_t day = bitmask_monday; day > 0; day <<= 1) {
            Serial.print((alarm.weekdays & day) != 0);
        }

        Serial.print(F(", T "));

        padded_print(alarm.hour);
        Serial.print(':');
        padded_print(alarm.minute);
        Serial.print(':');
        padded_print(alarm.second);
        space();
        Serial.println(alarm.control.group_end_marker? '#': '+');
    }
}


namespace eeprom {
    // version string to detect storage format
    const uint32_t version = 20150201;

    // we allow 128 bytes for the DCF77 library although we know that it uses less memory right now
    // this is to ensure compatibility with future versions of the lirary
    uint8_t * const header_base = (uint8_t * const) 0x80;

    // since Arduino has 1024 bytes EEPROM we have more than enough memory left for
    // 100 records of length 7 bytes plus some header information
    typedef struct {
        char u;
        char k;
        uint32_t version;
        uint16_t crc16;
    } header;

    uint8_t * const data_base = header_base+sizeof(header);
    const uint8_t data_records = 100;
    const uint8_t record_length = sizeof(Alarm_Timer::alarm_t);


    static const uint16_t crc_table[16] = {
        0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
        0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
    };

    uint16_t compute_crc16() {
        uint16_t crc = 0;
        size_t data_len = data_records * record_length;
        const uint8_t * ind = data_base;
        uint8_t idx;
        uint8_t data;
        while (data_len--) {
            data = eeprom_read_byte(ind++);
            idx = crc ^ data;
            crc = crc_table[idx & 0x0f] ^ (crc >> 4);
            idx = crc ^ (data >> 4);
            crc = crc_table[idx & 0x0f] ^ (crc >> 4);
        }
        return crc;
    }

    void read_alarm(Alarm_Timer::alarm_t & alarm, const uint8_t index) {
        const uint8_t * ind = data_base + record_length * index;

        for (uint8_t i = 0; i < record_length; ++i) {
            ((uint8_t *) &alarm)[i] = eeprom_read_byte(ind++);
        }
    }

    void write_alarm(const Alarm_Timer::alarm_t & alarm, const uint8_t index) {
        uint8_t * ind = data_base + record_length * index;

        for (uint8_t i = 0; i < record_length; ++i) {
            eeprom_write_byte(ind++, ((uint8_t *) &alarm)[i]);
        }
    }

    void verify_crc16() {
        const uint16_t crc             = compute_crc16();
        const uint16_t crc_from_header = eeprom_read_word((uint16_t *)(header_base+6));

        if (crc == crc_from_header) {
            Serial.println(F("CRC ok"));
        } else {
            Serial.print(F("CRC error - computed value: "));
            hex_print(crc);
            Serial.print(F(" persistent value: "));
            hex_print(crc_from_header);
            Serial.println();
        }
        Serial.println();
    }

    void write_crc16() {
        eeprom_write_word((uint16_t *)(header_base+6), compute_crc16());
    }

    void init() {
        eeprom_write_byte(header_base+0, 'u');
        eeprom_write_byte(header_base+1, 'k');
        eeprom_write_dword((uint32_t *)(header_base+2), version);

        // exploit that statics will be initialized with 0
        // (C99 standard 6.7.8/10 "Initialization)
        static Alarm_Timer::alarm_t alarm;
        for (uint8_t row = 0; row < data_records; ++row) {
            write_alarm(alarm, row);
        }

        write_crc16();
    }

    void dump_header() {
        Serial.print(F("ID: "));
        Serial.print((char)eeprom_read_byte(header_base+0));
        Serial.print((char)eeprom_read_byte(header_base+1));
        Serial.print(F(" Version: "));
        Serial.print(eeprom_read_dword((const uint32_t*)(header_base+2)));
        Serial.print(F(" CRC: "));
        hex_print(eeprom_read_word((const uint16_t*)(header_base+6)));
        Serial.println();
        Serial.println();
    }

    void dump_data(const uint8_t from_index, const uint8_t to_index, const boolean skip_inactive) {
        Alarm_Timer::alarm_t alarm;
        boolean line_feed_pending = false;

        for (uint8_t row = from_index; row <= to_index; ++row) {
            read_alarm(alarm, row);

            if (skip_inactive    &&
                row > from_index &&
                row < to_index   &&
                !alarm.control.active) {

                line_feed_pending = true;
                Serial.print(alarm.control.group_end_marker? '#': '+');

            } else {
                if (line_feed_pending) {
                    Serial.println();
                    line_feed_pending = false;
                }
                Serial.print(F("s "));
                if (row < 10) {
                    Serial.print('0');
                }
                Serial.print(row);
                space();
                Alarm_Timer::print_alarm(alarm);
            }
        }
        Serial.println();
    }

    void dump_all() {
        dump_header();
        verify_crc16();
        dump_data(0, data_records-1, true);
    }

    void hex_dump() {
        const uint8_t * data = (const uint8_t *) 0;

        for (uint8_t row = 0; row < 32; ++row) {
            hex_print((uint16_t)data);
            Serial.print(F(": "));
            for (uint8_t col = 0; col < 32; ++col) {
                hex_print(eeprom_read_byte(data++));
            }
            Serial.println();
        }
        Serial.println();
    }

    void setup() {
        // check if header is OK, if not so initialize eeprom accordingly
        if (!(
            eeprom_read_byte(header_base+0) == 'u' &&
            eeprom_read_byte(header_base+1) == 'k' &&
            eeprom_read_dword((const uint32_t*)(header_base+2)) == version)) {

            Serial.println(F("Initializing EEPROM"));
            init();
        }
    }
}


namespace Alarm_Timer {
    const uint8_t max_alarms = eeprom::data_records;

    typedef struct {
        // 1..7
        BCD::bcd_t weekday;

        // FF == every hour
        // 0..23
        BCD::bcd_t hour;
        // 0..59
        // FF == every minute,
        // Fn == minutes n, 1n, 2n ..., 5n
        BCD::bcd_t minute;
        // 0..60
        // FF == every second
        // Fn == seconds n, 1n, 2n ..., 5n
        BCD::bcd_t second;
    } day_and_time_t;

    boolean operator == (const day_and_time_t & a, const day_and_time_t & b) {
        return
            (a.weekday == b.weekday) &&
            (a.hour    == b.hour)    &&
            (a.minute  == b.minute)  &&
            (a.second  == b.second);
    }

    boolean equal_day_and_time(const day_and_time_t t, const Clock::time_t & now) {
        return
            (t.weekday == now.weekday) &&
            (t.hour    == now.hour)        &&
            (t.minute  == now.minute)      &&
            (t.second  == now.second);
    }

    int8_t compare(const day_and_time_t & ta, const day_and_time_t & tb) {
        // sgn(a-b)
        //    1 --> ta >  tb
        //    0 --> ta == tb
        //   -1 --> ta <  tb

        return
            ta.weekday > tb.weekday ?  1:
            ta.weekday < tb.weekday ? -1:
            ta.hour    > tb.hour    ?  1:
            ta.hour    < tb.hour    ? -1:
            ta.minute  > tb.minute  ?  1:
            ta.minute  < tb.minute  ? -1:
            ta.second  > tb.second  ?  1:
            ta.second  < tb.second  ? -1:
                                       0;
    }

    boolean is_trigger_1_before_trigger_2(
        const Clock::time_t & now,
        const day_and_time_t      & t1,
        const day_and_time_t      & t2
    ) {
        // if t1 == t2
        // we will bias in favour of t1

        // if t0 == t1
        // we will bias in favour of t1

        // t1, now, t2            --> true
        // now, t2, t1            --> true
        // t2, t1, now            --> true
        // now, t1, t2            --> false
        // t2, now, t1            --> false
        // t1, t2, now            --> false

        day_and_time_t t0;
        t0.weekday = now.weekday;
        t0.hour    = now.hour;
        t0.minute  = now.minute;
        t0.second  = now.second;

        const int8_t t0_vs_t1 = compare(t0, t1);
        if (t0_vs_t1 == 0) {
            return true;
        }

        const int8_t t0_vs_t2 = compare(t0, t2);
        if (t0_vs_t2 == 0) {
            return false;
        }

        return
            (t0_vs_t1 == t0_vs_t2)
            ?   // t1 and t2 are both before or both after t0
                (compare(t1, t2) <= 0)
            :   // t1 and t2 are on different sides of t0
                (t0_vs_t1 < 0)
            ;
    }

    const uint8_t partial_joker = 0xF;
    const uint8_t full_joker    = 0xFF;

    boolean is_match(const BCD::bcd_t alarm, const BCD::bcd_t now) {
        return (
            (alarm          == now)                                             ||
            (alarm.val      == full_joker)                                      ||
            (alarm.digit.hi == partial_joker && alarm.digit.lo == now.digit.lo) ||
            (alarm.digit.hi == now.digit.hi  && alarm.digit.lo == partial_joker)
        );
    }

    BCD::bcd_t get_earlier_match(const BCD::bcd_t alarm, const BCD::bcd_t now) {
        // determine something that matches alarm but is before now
        // otherwise return 0xff
        BCD::bcd_t best_match;

        if (alarm < now && alarm.digit.lo != partial_joker) {
            // notice that alarm will never match 0xF[0-F] because now is always <= 0x61
            best_match = alarm;
        } else
        if (alarm.val == full_joker && now.val > 0) {
            best_match.val = now.val-1;
            if (best_match.digit.lo > 0x09) {
                best_match.digit.lo -= (0xF - 9);
            }
        } else
        if (alarm.digit.hi == partial_joker) {
            // notice that alarm.digit.lo can not be a partial joker
            // because otherwise we would have found a full joker
            best_match = alarm;

            if (alarm.digit.lo < now.digit.lo) {
                best_match.digit.hi = now.digit.hi;
            } else
            if (now.digit.hi > 0) {
                best_match.digit.hi = now.digit.hi - 1;
            } else {
                best_match.val = 0xFF;
            }
        } else
        if (alarm.digit.lo == partial_joker) {
            if (alarm.digit.hi < now.digit.hi) {
                best_match.val = alarm.val;
                best_match.digit.lo = 9;
            } else
            if (alarm.digit.hi == now.digit.hi && now.digit.lo > 0) {
                // there will be no underflow as the lo digit is >0
                best_match.val = now.val-1;
            } else {
                best_match.val = 0xFF;
            }
        } else {
            best_match.val = 0xFF;
        }

        return best_match;
    }

    BCD::bcd_t get_highest_match(const BCD::bcd_t alarm) {
        if (alarm.val == full_joker) {
            BCD::bcd_t best_match;
            best_match.val  = 0x59;
            return best_match;
        } else
        if (alarm.digit.hi == partial_joker) {
            BCD::bcd_t best_match = alarm;
            best_match.digit.hi = 5;
            return best_match;
        } else
        if (alarm.digit.lo == partial_joker) {
            BCD::bcd_t best_match = alarm;
            best_match.digit.lo = 9;
            return best_match;
        }
        return alarm;
    }

    BCD::bcd_t get_earlier_weekday(const uint8_t weekday_mask, const BCD::bcd_t toweekday) {
        int8_t weekday_minus_1;
        for (weekday_minus_1 = toweekday.val-2; weekday_minus_1 >= 0; --weekday_minus_1) {
            if (weekday_mask & (bitmask_monday << weekday_minus_1)) {
                goto found;
            }
        }
        weekday_minus_1 = 0xFF - 1;

        found: {
            BCD::bcd_t result;
            result.val = weekday_minus_1 + 1;
            return result;
        }
    }

    BCD::bcd_t get_highest_weekday(const uint8_t weekday_mask) {
        int8_t weekday_minus_1;
        for (weekday_minus_1 = 6; weekday_minus_1 >= 0; --weekday_minus_1) {
            if (weekday_mask & (bitmask_monday << weekday_minus_1)) {
                break;
            }
        }

        BCD::bcd_t result;
        result.val = weekday_minus_1 + 1;
        return result;
    }

    day_and_time_t get_best_approximation(const Clock::time_t & now,
                                          const alarm_t             & alarm) {
        day_and_time_t best;

        if (!(alarm.weekdays & (bitmask_monday << (now.weekday.val - 1)))) goto match_earlier_weekday;
        best.weekday = now.weekday;

        if (!is_match(alarm.hour, now.hour)) goto match_earlier_hour;
        best.hour = now.hour;

        if (!is_match(alarm.minute, now.minute)) goto match_earlier_minute;
        best.minute = now.minute;

        if (!is_match(alarm.second, now.second)) goto match_earlier_second;
        best.second = now.second;

        goto done;


        match_earlier_second:
        best.second = get_earlier_match(alarm.second, now.second);
        if (best.second.val <= 0x61) goto done;

        match_earlier_minute:
        best.minute = get_earlier_match(alarm.minute, now.minute);
        if (best.minute.val <= 0x60) goto maximize_second;

        match_earlier_hour:
        best.hour = get_earlier_match(alarm.hour, now.hour);
        if (best.hour.val <= 0x24) goto maximize_minute;

        match_earlier_weekday:
        best.weekday = get_earlier_weekday(alarm.weekdays, now.weekday);
        if (best.weekday.val <= 7) goto maximize_hour;


        // maximize_day:
        best.weekday = get_highest_weekday(alarm.weekdays);

        maximize_hour:
        best.hour = get_highest_match(alarm.hour);
        if (best.hour.val > 0x23) { best.hour.val = 0x23; }

        maximize_minute:
        best.minute = get_highest_match(alarm.minute);

        maximize_second:
        best.second = get_highest_match(alarm.second);

        done:
        return best;
    }

    channels_t trigger(const Clock::time_t & now) {
        boolean start_next_group = true;

        day_and_time_t best_trigger;
        channels_t best_trigger_channels;
        best_trigger_channels = 0;

        channels_t output_channels;
        output_channels = 0;

        for (uint8_t i=0; i<max_alarms; ++i) {
            alarm_t current_alarm;
            eeprom::read_alarm(current_alarm, i);
            const boolean preceeds_a_new_group = current_alarm.control.group_end_marker || (i==max_alarms);

            if (current_alarm.control.active && current_alarm.weekdays) {
                day_and_time_t current_trigger = get_best_approximation(now, current_alarm);
                if (start_next_group || equal_day_and_time(current_trigger, now) ||
                    (is_trigger_1_before_trigger_2(now, best_trigger, current_trigger) && !equal_day_and_time(best_trigger, now))
                ) {
                    best_trigger          = current_trigger;
                    best_trigger_channels = current_alarm.channels;
                }
                start_next_group = preceeds_a_new_group;
            } else {
                start_next_group |= preceeds_a_new_group;
            }

            if (start_next_group) {
                output_channels |= best_trigger_channels;
                // no need to initialize best_trigger_channels as
                // this is already contained in the output
            }
        }
        output_channels |= best_trigger_channels;
        return output_channels;
    }
}


namespace ISR {
    volatile uint16_t tick;

    uint8_t sample_input_pin() {
        ++tick;
        const uint8_t sampled_data =
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));

        digitalWrite(dcf77_monitor_pin, sampled_data);
        return sampled_data;
    }

    volatile boolean output_enabled = true;
    volatile boolean has_staged_data;
    volatile boolean start_of_second;
    volatile Alarm_Timer::channels_t staged_output = 0;

    void pause_output() {
        output_enabled = false;
    }

    void enable_output() {
        output_enabled = true;
    }

    void stage_output(Alarm_Timer::channels_t output) {

        const uint8_t prev_SREG = SREG;
        cli();

        staged_output = output;
        has_staged_data = true;

        SREG = prev_SREG;
    }

    void flush_output() {
        for (uint8_t channel_pin = time_switch_channel_0_pin;
             channel_pin <time_switch_channel_0_pin+16;
             ++channel_pin) {

            digitalWrite(channel_pin, staged_output & 1);
            staged_output >>= 1;
        }
    }

    void clear_output() {
        staged_output = 0;
        flush_output();
    }

    void output_handler(const Clock::time_t &decoded_time) {
        tick = 0;
        start_of_second = true;

        if (has_staged_data) {
            if (output_enabled) {
                flush_output();
            }
            has_staged_data = false;
        }
    }
}


void help() {
    Serial.println(F("H, ?:  this help function"));
    Serial.println();
    Serial.println();
    Serial.println(F("Output control:"));
    Serial.println();
    Serial.println(F("P  pause all output channels"));
    Serial.println(F("B  break - set ouput to zero"));
    Serial.println(F("R  run - also recompute CRC16"));
    Serial.println();
    Serial.println();
    Serial.println(F("Alarm control:"));
    Serial.println();
    Serial.println(F("Erase    erase all alarm settings from EEPROM"));
    Serial.println();
    Serial.println(F("D        display alarm settings"));
    Serial.println(F("D nn     display alarm settings for alarm nn"));
    Serial.println(F("D nn-    display alarm settings for alarm nn-99"));
    Serial.println(F("D -mm    display alarm settings for alarm 00-mm"));
    Serial.println(F("D nn-mm  display alarm settings for alarm nn-mm"));
    Serial.println();
    Serial.println(F("V w dd:mm:ss View how the output would be at weekday w for the given time"));
    Serial.println();
    Serial.println(F("S nn     set trigger nn"));
    Serial.println(F("           Options may be separated by commas"));
    Serial.println(F("  A [0|1]    Activation: 0 --> deactivate, 1 --> activate"));
    Serial.println(F("  C xnnnn    Channels encoded hexadecimal, up to 4 digits"));
    Serial.println(F("  C bnnnn    Channels encoded binary, up to 16 digits"));
    Serial.println(F("             If the maximum number of digits is not used,"));
    Serial.println(F("             the channel setting must be terminated by a comma."));
    Serial.println(F("  D ddddddd  Days, starting from Monday, one binary digit per Day"));
    Serial.println(F("  T          Time in BCD * as Jokers are allowed"));
    Serial.println(F("  +          Continue current group, only admissible as final option"));
    Serial.println(F("  #          Close current group, only admissible as final option"));
    Serial.println();
    Serial.println(F("If the same options is used several times for the same set statement"));
    Serial.println(F("the last one wins."));
    Serial.println();
    Serial.println(F("Examples:"));
    Serial.println(F("s 00 a1                           activate channel 00 without altering its other settings"));
    Serial.println(F("s 01 a0                           deactivate channel 01 without altering ..."));
    Serial.println(F("s 02 a1 xab23                     activate channel 02 and set output to 1010101100100011 without ..."));
    Serial.println(F("s 03 a1 0 b1010101100100011       activate channel 03 set output to xAB23 w/o ..."));
    Serial.println(F("s 04 a1 b1,                       activate channel 04 and set output to x0001 w/o ..."));
    Serial.println(F("s 05 a1 ab, d1110000 t14:15:16 +  activate channel 04 and set output to x000B,"));
    Serial.println(F("                                  trigger on Monday, Tuesday, Wednesday at 14:15:16"));
    Serial.println(F("                                  and do not close current group"));
    Serial.println(F("s06 a1,xb,d1110000 t**:15:16 +    as above but trigger every hour"));
    Serial.println(F("s07 a1,xb,d1110000 t**:*5:16 +    as above but trigger every ten minutes"));
    Serial.println(F("S08 A1,XB,D1110000 T14:5*:*1 +    as above but trigger whenever time matches"));
    Serial.println(F("S09 A1,XB,D1110000 T14:**:** #    as above but close current group"));
    Serial.println(F("s10 t1*:15:16                     not supported - for hours only the double joker ** is supported"));
    Serial.println(F("s11 t*4:15:16                     see above"));
    Serial.println();
}

namespace parser {
    // this refers to output print to the serial part
    enum output_state_t {
        no_output_control       =    0,
        implicit_output_control = 0b01,
        explicit_output_control = 0b10,
        imex_output_control     = implicit_output_control | explicit_output_control
    } output_state = no_output_control;


    boolean parser_controled_output() {
        return output_state;
    }


    const char command_separator = ';';
    const char option_separator = ',';

    boolean is_joker(const char c) {
        return (c == '*');
    }

    uint8_t parse_joker(const char c) {
        return 0xf;
    }

    boolean is_decimal_digit(const char c) {
        return (('0' <= c) && (c <= '9'));
    }

    uint8_t parse_decimal_digit(const char c) {
        return c - '0';
    }

    boolean is_decimal_digit_or_joker(const char c) {
        return (('0' <= c) && (c <= '9')) || (c == '*');
    }

    uint8_t parse_decimal_digit_or_joker(const char c) {
        return
            is_joker(c) ? parse_joker(c)
                        : parse_decimal_digit(c);
    }

    boolean is_hexadecimal_digit(const char c) {
        return ((('0' <= c) && (c <= '9')) ||
                (('a' <= c) && (c <= 'f')) ||
                (('A' <= c) && (c <= 'F')));
    }

    uint8_t parse_hexadecimal_digit(const char c) {
        return
            is_decimal_digit(c)      ? parse_decimal_digit(c) :
            ('a' <= c) && (c <= 'f') ? c - 'a' + 10
                                     : c - 'A' + 10;
    }

    boolean is_binary_digit(const char c) {
        return (('0' <= c) && (c <= '1'));
    }

    uint8_t parse_binary_digit(const char c) {
        return c - '0';
    }

    void static_parse(const char c) {
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
        // follow the two links below.
        //     https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html
        //     http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html

        static void * parser_state = &&l_parser_start;

        output_state = (output_state_t) (output_state | implicit_output_control);
        if (parser_state < &&l_generic_error) {
            Serial.print(c);
        }

        if (c == ' ') {
            return;
        }

        if (c == command_separator) {
            Serial.println();
        }

        goto *parser_state;
        #define LABEL(N) label_ ## N
        #define XLABEL(N) LABEL(N)
        #define YIELD_NEXT_CHAR                                               \
            do {                                                              \
                parser_state = &&XLABEL(__LINE__); return; XLABEL(__LINE__):; \
            } while (0)

        l_parser_start: ;

        if (c == ' ') {
            // ignore leading space
            YIELD_NEXT_CHAR;
        }

        switch (c) {
            case '?':
            case 'h': {
                help();
                // ignore all characters till command end
                while (c != command_separator) {
                    YIELD_NEXT_CHAR;
                }
                goto l_done;
            }

            case 'p': { // pause
                YIELD_NEXT_CHAR;
                if (c == command_separator) {
                    Serial.println(F("Pause output"));
                    output_state = (output_state_t) (output_state | explicit_output_control);
                    ISR::pause_output();
                    goto l_done;
                }
                goto l_syntax_error;
            }

            case 'b': { // break
                YIELD_NEXT_CHAR;
                if (c == command_separator) {
                    Serial.println(F("Break"));
                    output_state = (output_state_t) (output_state | explicit_output_control);
                    ISR::pause_output();
                    ISR::clear_output();
                    goto l_done;
                }
                goto l_syntax_error;
            }

            case 'r': { // run
                YIELD_NEXT_CHAR;
                if (c == command_separator) {
                    eeprom::write_crc16();
                    Serial.println(F("Run"));
                    output_state = (output_state_t) (output_state & implicit_output_control);
                    ISR::enable_output();
                    goto l_done;
                }
                goto l_syntax_error;
            }

            case 'e': { // erase all EEPROM contents
                YIELD_NEXT_CHAR;
                if (c != 'r') { goto l_syntax_error; }

                YIELD_NEXT_CHAR;
                if (c != 'a' ) { goto l_syntax_error; }

                YIELD_NEXT_CHAR;
                if (c != 's') { goto l_syntax_error; }

                YIELD_NEXT_CHAR;
                if (c != 'e') { goto l_syntax_error; }

                YIELD_NEXT_CHAR;
                if (c != command_separator) { goto l_syntax_error; }

                Serial.println(F("Erase EEPROM"));
                eeprom::init();
                goto l_done;
            }

            case 'v': { // view
                static Clock::time_t test_time;

                // parse weekday
                YIELD_NEXT_CHAR;
                if (!is_decimal_digit(c)) { goto l_digit_expected; }
                test_time.weekday.val = parse_decimal_digit(c);
                if (test_time.weekday.val < 1 || test_time.weekday.val > 7) { goto l_out_of_range_error; }

                // parse hours
                YIELD_NEXT_CHAR;
                if (!is_decimal_digit(c)) { goto l_digit_expected; }
                test_time.hour.digit.hi = parse_decimal_digit(c);
                if (test_time.hour.digit.hi > 2) { goto l_out_of_range_error; }

                YIELD_NEXT_CHAR;
                if (!is_decimal_digit(c)) { goto l_digit_expected; }
                test_time.hour.digit.lo = parse_decimal_digit(c);
                if (test_time.hour.val > 0x23) { goto l_out_of_range_error; }

                YIELD_NEXT_CHAR;
                if (c != ':') { goto l_separator_error; }

                // parse minutes
                YIELD_NEXT_CHAR;
                if (!is_decimal_digit(c)) { goto l_digit_expected; }
                test_time.minute.digit.hi = parse_decimal_digit(c);
                if (test_time.minute.digit.hi > 5) { goto l_out_of_range_error; }

                YIELD_NEXT_CHAR;
                if (!is_decimal_digit(c)) { goto l_digit_expected; }
                test_time.minute.digit.lo = parse_decimal_digit(c);

                YIELD_NEXT_CHAR;
                if (c != ':') { goto l_separator_error; }

                // parse seconds
                YIELD_NEXT_CHAR;
                if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                test_time.second.digit.hi = parse_decimal_digit(c);
                if ((test_time.second.digit.hi > 5)) { goto l_out_of_range_error; }

                YIELD_NEXT_CHAR;
                if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                test_time.second.digit.lo = parse_decimal_digit(c);

                YIELD_NEXT_CHAR;
                if (c != command_separator) { goto l_syntax_error; }

                Serial.print(F("Alarm Test: "));
                Serial.print(test_time.weekday.val);
                space();
                padded_print(test_time.hour);
                Serial.print(':');
                padded_print(test_time.minute);
                Serial.print(':');
                padded_print(test_time.second);

                Serial.print(F("  Channels: "));
                const Alarm_Timer::channels_t channels = Alarm_Timer::trigger(test_time);
                binary_print(channels);
                Serial.println();
                goto l_done;
            }

            case 'd': { // dump
                static uint8_t dump_from;
                static uint8_t dump_to;
                static boolean skip_inactive;

                skip_inactive = false;

                // digit efault range
                dump_from = 0;
                dump_to = eeprom::data_records - 1;

                YIELD_NEXT_CHAR;
                if (c == command_separator) { goto l_dump_eeprom; }
                if (c == '-')               { skip_inactive = true; goto l_parse_upper_bound; }
                if (!is_decimal_digit(c))   { goto l_digit_expected; }

                skip_inactive = false;
                dump_from = parse_decimal_digit(c);
                dump_to = dump_from;


                YIELD_NEXT_CHAR;
                if (c == command_separator) { goto l_dump_eeprom; }
                if (c == '-')               { goto l_parse_upper_bound; }
                if (!is_decimal_digit(c))   { goto l_digit_expected; }

                dump_from = 10 * dump_from + parse_decimal_digit(c);
                dump_to = dump_from;


                YIELD_NEXT_CHAR;
                if (c == command_separator) { goto l_dump_eeprom; }
                if (c != '-')               { goto l_separator_error; }

                l_parse_upper_bound:
                dump_to = eeprom::data_records - 1;


                YIELD_NEXT_CHAR;
                if (c == command_separator) { goto l_dump_eeprom; }
                if (!is_decimal_digit(c))   { goto l_digit_expected; }

                skip_inactive = false;
                dump_to = parse_decimal_digit(c);


                YIELD_NEXT_CHAR;
                if (c == command_separator) { goto l_dump_eeprom; }
                if (!is_decimal_digit(c))   { goto l_digit_expected; }

                dump_to = 10 * dump_to + parse_decimal_digit(c);


                YIELD_NEXT_CHAR;
                if (c == command_separator) { goto l_dump_eeprom; }
                goto l_separator_error;

                l_dump_eeprom:
                if (dump_from <= dump_to) {
                    eeprom::dump_data(dump_from, dump_to, skip_inactive);
                } else {
                    eeprom::dump_data(dump_to, dump_from, skip_inactive);
                }
                goto l_done;
            }

            case 's': { // set
                static uint8_t alarm_index;
                static Alarm_Timer::alarm_t alarm;

                // determine the channel that will be set
                YIELD_NEXT_CHAR;
                if (!is_decimal_digit(c)) { goto l_digit_expected; }

                alarm_index = parse_decimal_digit(c);

                YIELD_NEXT_CHAR;
                if (is_decimal_digit(c)) {
                    alarm_index = 10*alarm_index + parse_decimal_digit(c);
                    YIELD_NEXT_CHAR;
                }

                if (c == command_separator) { goto l_after_set; }

                // now the desired alarm_index is known
                eeprom::read_alarm(alarm, alarm_index);

                // parse the different options of the set command
                // if the same option is parsed multiple times --> last one wins
                while (true) {
                    static int8_t digit_index;

                    switch (c) {
                        case option_separator:
                            YIELD_NEXT_CHAR;
                            continue;

                        case 'a': // activation
                            YIELD_NEXT_CHAR;
                            if (!is_binary_digit(c)) { goto l_binary_digit_expected; }
                            alarm.control.active = parse_binary_digit(c);
                            break;

                        case 'c': // channels
                            alarm.channels = 0;

                            YIELD_NEXT_CHAR;
                            if (c == option_separator) { continue; }
                            if (c == 'x') {
                                for (digit_index = 0; digit_index < 4; ++digit_index) {
                                    YIELD_NEXT_CHAR;
                                    if (c == option_separator) { break; }
                                    if (c == command_separator) { goto l_set_now; }
                                    if (!is_hexadecimal_digit(c)) { goto l_hexadecimal_digit_expected; }
                                    alarm.channels = (alarm.channels<<4) + parse_hexadecimal_digit(c);
                                }
                                break;
                            }
                            if (c == 'b') {
                                for (digit_index = 0; digit_index < 16; ++digit_index) {
                                    YIELD_NEXT_CHAR;
                                    if (c == option_separator) { break; }
                                    if (c == command_separator) { goto l_set_now; }
                                    if (!is_binary_digit(c)) { goto l_binary_digit_expected; }
                                    alarm.channels = (alarm.channels<<1) + parse_binary_digit(c);
                                }
                                break;
                            }
                            goto l_syntax_error;

                        case 'd': // weekdays
                            alarm.weekdays = 0;

                            for (digit_index = 0; digit_index < 7; ++digit_index) {
                                YIELD_NEXT_CHAR;
                                if (!is_binary_digit(c)) { goto l_binary_digit_expected; }

                                if (parse_binary_digit(c)) {
                                    alarm.weekdays += (Alarm_Timer::bitmask_monday << digit_index);
                                }
                            }

                            break;

                        case 't': // time
                            YIELD_NEXT_CHAR;
                            if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                            alarm.hour.digit.hi = parse_decimal_digit_or_joker(c);
                            if ((alarm.hour.digit.hi > 2) && !is_joker(c)) { goto l_out_of_range_error; }

                            YIELD_NEXT_CHAR;
                            if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                            alarm.hour.digit.lo = parse_decimal_digit_or_joker(c);
                            if ((alarm.hour.digit.lo == 0xf) != (alarm.hour.digit.hi == 0xf)) { goto l_joker_expected; }
                            if (alarm.hour.val > 0x23 && !is_joker(c)) { goto l_out_of_range_error; }

                            YIELD_NEXT_CHAR;
                            if (c != ':') { goto l_separator_error; }

                            // parse minutes
                            YIELD_NEXT_CHAR;
                            if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                            alarm.minute.digit.hi = parse_decimal_digit_or_joker(c);
                            if ((alarm.minute.digit.hi > 5) && !is_joker(c)) { goto l_out_of_range_error; }

                            YIELD_NEXT_CHAR;
                            if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                            alarm.minute.digit.lo = parse_decimal_digit_or_joker(c);

                            YIELD_NEXT_CHAR;
                            if (c != ':') { goto l_separator_error; }

                            // parse seconds
                            YIELD_NEXT_CHAR;
                            if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                            alarm.second.digit.hi = parse_decimal_digit_or_joker(c);
                            if ((alarm.second.digit.hi > 5) && !is_joker(c)) { goto l_out_of_range_error; }

                            YIELD_NEXT_CHAR;
                            if (!is_decimal_digit_or_joker(c)) { goto l_digit_or_joker_expected; }
                            alarm.second.digit.lo = parse_decimal_digit_or_joker(c);

                            break;

                        case '+':
                            alarm.control.group_end_marker = 0;
                            goto l_ready_to_set;

                        case '#':
                            alarm.control.group_end_marker = 1;
                            goto l_ready_to_set;

                        case ';':
                            goto l_set_now;

                        default: goto l_syntax_error;
                    }

                    YIELD_NEXT_CHAR;
                }

                l_ready_to_set:
                YIELD_NEXT_CHAR;
                if (c != command_separator) { goto l_separator_error; }
                l_set_now:
                eeprom::write_alarm(alarm, alarm_index);
                l_after_set:
                eeprom::dump_data(alarm_index, alarm_index, false);

                goto l_done;
            }

            //'h': help
            //'?': help
            default: goto l_unknown_command;
        }

        l_unknown_command:
        Serial.println();
        Serial.print(F(" Unknown command"));
        goto l_generic_error;

        l_joker_expected:
        Serial.println();
        Serial.print(F(" * expected"));
        goto l_generic_error;

        l_digit_or_joker_expected:
        Serial.println();
        Serial.print(F(" Digit or * expected"));
        goto l_generic_error;

        l_digit_expected:
        Serial.println();
        Serial.print(F(" Digit expected"));
        goto l_generic_error;

        l_binary_digit_expected:
        Serial.println();
        Serial.print(F(" Binary digit expected"));
        goto l_generic_error;

        l_hexadecimal_digit_expected:
        Serial.println();
        Serial.print(F(" Hexadecimal digit expected"));
        goto l_generic_error;

        l_separator_error:
        Serial.println();
        Serial.print(F(" Unexpected or missing separator"));
        goto l_generic_error;

        l_out_of_range_error:
        Serial.println();
        Serial.print(F(" Digit out of range error"));
        goto l_generic_error;

        l_syntax_error:
        Serial.println();
        goto l_generic_error;

        l_generic_error:

        Serial.print(F(" error at '"));
        Serial.print(c);
        if (c== command_separator) {
            Serial.println('\'');
        } else {
            Serial.print(F("' before "));
            do {
                YIELD_NEXT_CHAR;
                Serial.print(c);
            } while (c != command_separator);

            Serial.println();
        }
        Serial.println();
        Serial.println(F("Use h for help!"));
        goto l_done;

        l_done:
        output_state = (output_state_t) (output_state & explicit_output_control);
        Serial.println();
        parser_state = &&l_parser_start;
        return;
    }


    boolean parse() {  // deliver true if some character was available
        static char previous_char;
        if (Serial.available()) {
            char c = (char) Serial.read();

            // interpret tab as whitespace
            if (c == 0x09) {
                c = ' ';
            }

            // map newline and carriage return to command separator
            if (c==0x0A || c==0x0D) {
                c = command_separator;
            }

            // ignore successive separators
            if ((c == option_separator || c == command_separator) && (c == previous_char)) {
                return true;
            }

            // map everything to lower case
            if (('A' <= c) && (c <= 'Z')) {
                c+= 'a' - 'A';
            }

            static_parse(c);
            if (c != ' ') {
                previous_char = c;
            }

            return true;
        }
        return false;
    }
}

uint8_t lock_progress() {
    typedef Internal::DCF77_Clock_Controller<Configuration, Internal::DCF77_Frequency_Control> Clock_Controller;
    Clock_Controller::clock_quality_factor_t quality;
    Clock_Controller::get_quality_factor(quality);

    return
        (quality.phase   > 0) +
        (quality.second  > 0) +
        (quality.minute  > 0) +
        (quality.hour    > 0) +
        (quality.weekday > 0) +
        (quality.day     > 0) +
        (quality.month   > 0) +
        (quality.year    > 0);
}

void setup() {
    pinMode(dcf77_monitor_pin, OUTPUT);
    pinMode(dcf77_sample_pin, dcf77_pin_mode);

    for (uint8_t channel_pin = time_switch_channel_0_pin; channel_pin <time_switch_channel_0_pin+16; ++channel_pin) {
        pinMode(channel_pin, OUTPUT);
        digitalWrite(channel_pin, LOW);
    }

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(ISR::sample_input_pin);
    DCF77_Clock::set_output_handler(ISR::output_handler);


    Serial.begin(115200);
    Serial.println();
    Serial.println(F("DCF77 Timeswitch V3.1.1"));
    Serial.println(F("(c) Udo Klein 2016"));
    Serial.println(F("www.blinkenlight.net"));
    Serial.println();
    Serial.print(F("Sample Pin:    ")); Serial.println(dcf77_sample_pin);
    Serial.print(F("Sample Pin Mode: ")); Serial.println(dcf77_pin_mode);
    Serial.print(F("Inverted Mode: ")); Serial.println(dcf77_inverted_samples);
    Serial.print(F("Analog Mode:   ")); Serial.println(dcf77_analog_samples);
    Serial.print(F("Monitor Pin:   ")); Serial.println(dcf77_monitor_pin);
    Serial.print(F("Drift Adjust:  ")); Serial.println(Internal::Generic_1_kHz_Generator::read_adjustment());
    Serial.println();
    Serial.println();
    Serial.println(F("Initializing..."));


    eeprom::setup();
    eeprom::dump_all();

    // Wait till clock is synced, depending on the signal quality this may take
    // rather long. About 5 minutes with a good signal, 30 minutes or longer
    // with a bad signal
    for (uint8_t state = Clock::useless;
         state == Clock::useless || state == Clock::dirty;
         state = DCF77_Clock::get_clock_state()) {

        // wait for next sec
        Clock::time_t now;
        DCF77_Clock::get_current_time(now);

        if (!parser::parser_controled_output()) {
            // render one dot per second while initializing
            static uint8_t count = 0;
            Serial.print(lock_progress());
            ++count;
            if (count == 60) {
                count = 0;
                Serial.println();
            }
        }

        while (parser::parse()) {};
    }

    Serial.println();
    Serial.println();

    ISR::enable_output();
}

void loop() {
    if (ISR::start_of_second) {
        ISR::start_of_second = false;

        Clock::time_t now_plus_1s;
        DCF77_Clock::read_future_time(now_plus_1s);
        ISR::stage_output(Alarm_Timer::trigger(now_plus_1s));
        Clock::time_t now;
        DCF77_Clock::read_current_time(now);
        if (now.month.val > 0) {
            const uint8_t state = DCF77_Clock::get_clock_state();
            if (state != Clock::useless) {
                if (!parser::parser_controled_output()) {
                    switch (state) {
                        case Clock::useless: Serial.print(F("useless")); break;
                        case Clock::dirty:   Serial.print(F("dirty: ")); break;
                        case Clock::synced:  Serial.print(F("synced:")); break;
                        case Clock::locked:  Serial.print(F("locked:")); break;
                    }

                    Serial.print(F(" 20"));
                    padded_print(now.year);
                    Serial.print('-');
                    padded_print(now.month);
                    Serial.print('-');
                    padded_print(now.day);
                    space();
                    Serial.print(now.weekday.val);
                    space();
                    padded_print(now.hour);
                    Serial.print(':');
                    padded_print(now.minute);
                    Serial.print(':');
                    padded_print(now.second);

                    Serial.print(F(" UTC+0"));
                    Serial.print(now.uses_summertime? '2': '1');

                    Serial.print("  Channels: ");

                    Alarm_Timer::channels_t observed_output;
                    for (uint8_t channel_pin = time_switch_channel_0_pin+16-1;
                         channel_pin >= time_switch_channel_0_pin;
                         --channel_pin) {
                        observed_output <<= 1;
                        observed_output |= digitalRead(channel_pin);
                    }
                    binary_print(observed_output);
                    Serial.println();
                }
            }
        }
    }

    parser::parse();
}