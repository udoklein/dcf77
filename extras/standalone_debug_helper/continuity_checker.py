#!/usr/bin/python

"""Extract and analyze lines of the format

Decoded time: 18-05-14 1 19:36:05 CEST ..

from stdin or from file
"""

import fileinput
import datetime
import re


#                  Decoded time:   1 8 -  0 5 -  1 4   1   1 9 :  3 6 :  0 5
regex = re.compile('^Decoded time: (\d\d)-(\d\d)-(\d\d) \d (\d\d):(\d\d):(\d\d)')

one_second   = datetime.timedelta(seconds=1)
zero_seconds = datetime.timedelta(seconds=0)
previous_time = None
for line in fileinput.input():
    match = regex.match(line)
    if match:
        year, month, day, hour, minute, second = (int(x) for x in match.group(1, 2, 3, 4, 5, 6))
        year += 2000

        parsed_time = datetime.datetime(year, month, day, hour, minute, second)
        if previous_time:
            delta = parsed_time - previous_time
            if delta > one_second or delta < zero_seconds:
                print "Discontinuity detected, " + str(previous_time) + " -> " + str(parsed_time)
                print line

        previous_time = parsed_time
