#!/usr/bin/python

"""Extract and analyze debug log blocks of the format

  ??.??.??(?,?)??:??:255 CET 0,0 0 p(0-0:0) s(6-4:0) m(0-0:0) h(0-0:0) wd(0-0:0) D(0-0:0) M(0-0:0) Y(0-0:0) 0,0,0,255
Clock state: useless
Tick: 3999
confirmed_precision ?? adjustment, deviation, elapsed
0.0000 ppm, .- , 0.0000 ppm, 0 ticks, 0 min + 399 ticks mod 60000

or

Decoded time: 17-01-01 7 00:15:39 CET ..
  17.01.01(7,0)00:16:39 CET 0,0 3 p(6000-0:255) s(165-0:23) m(62-36:5) h(52-26:5) wd(26-13:3) D(52-39:2) M(39-26:2) Y(52-39:2) 26,13,13,50
output triggered
Clock state: synced
Tick: 1000
confirmed_precision ?? adjustment, deviation, elapsed
0.0000 ppm, @+ , 0.0000 ppm, 0 ticks, 0 min + 3500 ticks mod 60000

"""

import argparse
import datetime
import re
import sys


def reader(filename):
    if filename:
        try:
            logfile = open(filename)
        except Exception as e:
            print "Failed to open:", filename
            sys.exit(e)
    else:
        logfile = sys.stdin

    for line in logfile:
        yield line

    if filename:
        logfile.close()


# useful resources for python regex: https://pythex.org/

#                                 Decoded time:   1 8 -  0 5 -  1 4   1   1 9 :  3 6 :  0 5  CE    T  .  .
regex_decoded_time = re.compile('^Decoded time: (\d\d)-(\d\d)-(\d\d) \d (\d\d):(\d\d):(\d\d) CE(S?)T (.)(.)')
def parse_time(line):
    """ parser for time output, as created by
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
    """
    if line == None:
        return "year", "month", "day", "hour", "minute", "second", "timezone", "tz_change_notification", "leap_second_notification"

    match = regex_decoded_time.match(line)
    if match:
        year, month, day, hour, minute, second = (int(x) for x in match.group(1, 2, 3, 4, 5, 6))
        timezone, tz_change_notification, leap_second_notification = (str(x) for x in match.group(7, 8, 9))
        if timezone == "S":
            timezone = "CEST"
        else:
            timezone = "CET"

        return year, month, day, hour, minute, second, timezone, tz_change_notification, leap_second_notification
    else:
        return None



#                                     ? ?   .      ? ?  .       ? ?  (     ? ,  ?   )         ? ? :       ?  ?: 255  CE    T   0,   0    0  p  (   0 - 0  : 0  )   s(6     - 4   :0     ) m(0     -0    :  0)    h(  0   - 0   :0)      wd( 0    - 0   :0     ) D(0     - 0   :  0   ) M(   0  -0    : 0    ) Y (  0  -0    :0     ) 0    ,0    ,0    ,255
#                                1 7        . 0 1       .  0 1       (  7    ,  0   )    0 0      :  1 6      :  39  CE    T   0,   0    3  p  ( 6000- 0  : 255)   s(165   - 0   :23    ) m (62   -36   :  5)    h( 52   - 26  :5)      wd( 26   - 13  :3     ) D(52    - 39  :  2   ) M ( 39  -26   :  2   ) Y ( 52  - 39  : 2    ) 26   ,13   ,13   ,50
regex_quality = re.compile("^ *(\d\d|\?\?)\.(\d\d|\?\?)\.(\d\d|\?\?)\((\d|\?),(\d|\?)\)(\d\d|\?\?):(\d\d|\?\?):(\d*) CE(S?)T (\d),(\d) (\d) p\((\d*)-(\d*):(\d*)\) s\((\d*)-(\d*):(\d*)\) m\((\d*)-(\d*):(\d*)\) h\((\d*)-(\d*):(\d*)\) wd\((\d*)-(\d*):(\d*)\) D\((\d*)-(\d*):(\d*)\) M\((\d*)-(\d*):(\d*)\) Y\((\d*)-(\d*):(\d*)\) (\d*),(\d*),(\d*),(\d*)")
def parse_quality(line):
    """ parser for quality analysis output as created by
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
    """

    if line == None:
        return (
              "year", "month", "day", "weekday_value", "computed_weekday", "hour", "minute", "second", "timezone", "leap_second_scheduled", "timezone_change_scheduled", "overall_quality_factor",
              "phase_lock_max", "phase_lock_noise", "phase_lock_quality",
              "second_max",     "second_noise",     "second_quality",
              "minute_max",    "minute_noise",      "minute_quality",
              "hour_max",       "hour_noise",       "hour_quality",
              "weekday_max",    "weekday_noise",    "weekday_quality",
              "day_max",        "day_noise",        "day_quality",
              "month_max",      "month_noise",      "month_quality",
              "year_max",       "year_noise",       "year_quality",
              "uses_summertime_quality", "timezone_change_scheduled_quality", "leap_second_scheduled_quality", "predictiction_match"
          )

    match = regex_quality.match(line)
    if match:
        ##                                1 7        . 0 1       .  0 1       (  7    ,  0   )    0 0      :  1 6      :  39  CE    T     0,   0    3  p  ( 6000- 0  : 255)   s(165   - 0   :23    ) m (62   -36   :  5)    h( 52   - 26  :5)      wd( 26   - 13  :3     ) D(52    - 39  :  2   ) M ( 39  -26   :  2   ) Y ( 52  - 39  : 2    ) 26   ,13   ,13   ,50
        #regex_quality = re.compile("^ *(\d\d|\?\?)\.(\d\d|\?\?)\.(\d\d|\?\?)\((\d|\?),(\d|\?)\)(\d\d|\?\?):(\d\d|\?\?):(\d*) CE(S?)T (\d),(\d) (\d) p\((\d*)-(\d*):(\d*)\) s\((\d*)-(\d*):(\d*)\) m\((\d*)-(\d*):(\d*)\) h\((\d*)-(\d*):(\d*)\) wd\((\d*)-(\d*):(\d*)\) D\((\d*)-(\d*):(\d*)\) M\((\d*)-(\d*):(\d*)\) Y\((\d*)-(\d*):(\d*)\) (\d*),(\d*),(\d*),(\d*)")

        year, month, day, weekday_value, computed_weekday, hour, minute, second = (safe_int(x, -1) for x in match.group(1, 2, 3, 4, 5, 6, 7, 8))

        timezone = str(match.group(9))
        if timezone == "S":
            timezone = "CEST"
        else:
            timezone = "CET"

        leap_second_scheduled, timezone_change_scheduled, overall_quality_factor = (int(x) for x in match.group(10, 11, 12))

        phase_lock_max, phase_lock_noise, phase_lock_quality = (int(x) for x in match.group(13, 14, 15))
        second_max,     second_noise,     second_quality     = (int(x) for x in match.group(16, 17, 18))
        minute_max,     minute_noise,     minute_quality     = (int(x) for x in match.group(19, 20, 21))
        hour_max,       hour_noise,       hour_quality       = (int(x) for x in match.group(22, 23, 24))
        weekday_max,    weekday_noise,    weekday_quality    = (int(x) for x in match.group(25, 26, 27))
        day_max,        day_noise,        day_quality        = (int(x) for x in match.group(28, 29, 30))
        month_max,      month_noise,      month_quality      = (int(x) for x in match.group(31, 32, 33))
        year_max,       year_noise,       year_quality       = (int(x) for x in match.group(34, 35, 36))

        uses_summertime_quality, timezone_change_scheduled_quality, leap_second_scheduled_quality, predictiction_match = (int(x) for x in match.group(37, 38, 39, 40))

        return year, month, day, weekday_value, computed_weekday, hour, minute, second, timezone, leap_second_scheduled, timezone_change_scheduled, overall_quality_factor, \
              phase_lock_max, phase_lock_noise, phase_lock_quality, \
              second_max,     second_noise,     second_quality, \
              minute_max,     minute_noise,     minute_quality, \
              hour_max,       hour_noise,       hour_quality, \
              weekday_max,    weekday_noise,    weekday_quality, \
              day_max,        day_noise,        day_quality, \
              month_max,      month_noise,      month_quality, \
              year_max,       year_noise,       year_quality, \
              uses_summertime_quality, timezone_change_scheduled_quality, leap_second_scheduled_quality, predictiction_match
    else:
        return None


#                               Clock state:  synced
regex_clock_state = re.compile('^Clock state: (\w*)')
def parse_state(line):
    if line == None:
        return "state",  # comma makes result a tuple

    match = regex_clock_state.match(line)
    if match:
        state = str(match.group(1))
        return state,  # comma makes result a tuple
    else:
        return None


#                         Tick: 3999
regex_tick = re.compile('^Tick: (\d*)')
def parse_tick(line):
    if line == None:
        return "tick",  # comma makes result a tuple

    match = regex_tick.match(line)
    if match:
        tick = int(match.group(1))
        return tick,  # comma makes result a tuple
    else:
        return None


#       53, 6---------+---------+---------+---------+---------+---------+---------+---------+---------2XXXXXXXXX
regex_scope = re.compile('^ *(\d*), ([0-9X+-]{100})')
def parse_scope(line):
    if line == None:
        return "second", "scope"

    match = regex_scope.match(line)
    if match:
        second, scope = (str(x) for x in match.group(1, 2))
        return second, scope
    else:
        return None


#                           0.0000    ppm,    .  - ,     0.0000   ppm,     0   ticks,   0   min +  399 ticks mod 60000
regex_drift = re.compile('^(-?\d*\.\d*) ppm, (.)(.) , (-?\d*.\d*) ppm, (-?\d*) ticks, (\d*) min \+ (\d*) ticks mod 60000')
def parse_drift(line):
    """ parser for drift output as created by
        void DCF77_Frequency_Control::debug() {
            using namespace Debug;
            sprintln(F("confirmed_precision ?? adjustment, deviation, elapsed"));
            sprintpp16m(confirmed_precision);
            sprint(F(", "));
            sprint(calibration_state.running? '@': '.');
            sprint(calibration_state.qualified? '+': '-');
            sprint(' ');

            sprint(F(", "));
            sprintpp16m(Generic_1_kHz_Generator::read_adjustment());
            sprint(F(", "));

            sprint(deviation);
            sprint(F(" ticks, "));

            sprint(deviation_tracker.elapsed_minutes);
            sprint(F(" min + "));
            sprint(deviation_tracker.elapsed_ticks_mod_60000);
            sprintln(F(" ticks mod 60000"));
        }
    """
    if line == None:
        return "confirmed_precision", "runnning", "qualified", "adjustment", "deviation", "elapsed_minutes", "elapsed_ticks_mod_60000"

    match = regex_drift.match(line)
    if match:
        runnning, qualified = (str(x) for x in match.group(2, 3))
        confirmed_precision, adjustment = (float(x) for x in match.group(1, 4))
        deviation, elapsed_minutes, elapsed_ticks_mod_60000 = (int(x) for x in match.group(5, 6, 7))
        return confirmed_precision, runnning, qualified, adjustment, deviation, elapsed_minutes, elapsed_ticks_mod_60000
    else:
        return None


def safe_int(val, default=None):
    try:
        return int(val)
    except (ValueError, TypeError):
        return default


def parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                    description="log to CSV converter for debug helper logs")

    parser.add_argument("-f", "--file", type=str, help="log filename")
    parser.add_argument("-n", "--named", action="store_true", help="output column names")

    group = parser.add_mutually_exclusive_group()

    group.add_argument("-t", "--time",    action="store_true", help="parse time log")
    group.add_argument("-q", "--quality", action="store_true", help="parse quality log")
    group.add_argument("-c", "--clock",   action="store_true", help="parse clock state log")
    group.add_argument("-d", "--drift",   action="store_true", help="parse drift log")
    group.add_argument("-s", "--scope",   action="store_true", help="parse scope")
    group.add_argument("-k", "--tick",    action="store_true", help="parse tick")


    return parser.parse_args()

def cook_output(tuple):
    return ",".join((str(item) for item in tuple))

args = parse_args()
filename          = args.file
named_columns     = args.named
try:
    line_parser = (
                    [parse_time, parse_quality, parse_state, parse_drift, parse_tick, parse_scope][
                    [ args.time,  args.quality,  args.clock,  args.drift,  args.tick,  args.scope].index(True)]
                )
except (ValueError):
    print "No parser selected -> nothing to do -> exiting"
    sys.exit(0)

if named_columns:
    print cook_output(line_parser(None))

one_second   = datetime.timedelta(seconds=1)
zero_seconds = datetime.timedelta(seconds=0)
previous_time = None
for line in reader(filename):
    out = line_parser(line)
    if out:
        print cook_output(out)


