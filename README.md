# dcf77 library

[![GitHub license](https://img.shields.io/badge/license-AGPL-blue.svg)](https://raw.githubusercontent.com/udoklein/dcf77/master/LICENSE)

Noise resilient DCF77 decoder library for Arduino.

(c) Udo Klein -- http://www.blinkenlight.net

This repository contains a DCF77 decoder library for Arduino.
It was designed with the goal to create the most noise resilent
decoder possible.

There are also some ready to run examples.

For the details and background information you may want to consult
the DCF77 section of [my webpage.](http://blog.blinkenlight.net/experiments/dcf77/)

The Library itself and its history are documented
in [my webpage.](http://blog.blinkenlight.net/experiments/dcf77/dcf77-library/)
This page also contains a short FAQ sections. Please read this page
first if you encounter any issues with my library.

**Attention:** the library requires a crystal oscillator. The Arduino
Uno comes out of the box with a resonator. Thus it will not work
with this library. "Older" designs like the [Blinkenlighty](
http://www.amazon.de/gp/product/3645651306/?ie=UTF8&camp=1638&creative=6742&linkCode=ur2&site-redirect=de&tag=wwwblinkenlig-21)
have a (more expensive, more stable and more precise) crystal oscillator.


# Getting Started
## Required components
The first things you will need is DCF77 module and a crystal based Arduino.
**Attention:** the Arduino Uno is NOT crystal based. Although there is a crystal on
the board this crystal is not used to clock the main controller. The main controller
only has a resonator. Ensure that you have crystal based controller or the library
will not work for you.

## Wiring it
The output of the DCF77 module must be connected with the "sample pin" on the Arduino
side. The sketches usually use digital pin 19 or analog pin 5
(this is actually the same pin). For the Arduino Due the pin is usually digital pin 53.

Some modules require 5 V and some require 3.3 V as a supply voltage.
Some other modules can deal with 3.3 to 5 V. Ensure that the supply voltage
matches with your Arduino. Look up the datasheet of your module to figure
out what it requires.

A lot of modules come with some "enable" pin. Often this pin is labelled
"PON" in the datasheet and must be connected to ground. However your mileage
may vary. You definitely need to look this up in the datasheet.

Then there are some modules that require a pull up resistor, some others need no
pull up resistor and yet some other modules must not used with a pull up. By default
my examples will activate the internal pull up resistor which is in the range
of 10-50k and connected to VCC. Thus if your module can drive at least
500 μA you are on the safe side. If your module can not drive this
there are two options:

1) The module has a push/pull output. (E.g. the well known "Pollin" module).
Then disable the internal pullup.

2) The module is open collector but can not drive 500 μA. Then you need
to insert a driver or some transistor after your module. Often the
datasheet has example circuits on how to achieve this. Otherwise search
the net for examples on how to wire the module.

To disable the pull up resistor set 'dcf77_pull_up = 0;' otherwise
enable it with 'dcf77_pull_up = 1;'.

## Testing it
Once you have everything ready its time to have a look at the signal. This is
most conveniently achieved with the **DCF77 Scope sketch**. This sketch does not
use the library. It is included for basic analysis and troubleshooting purposes.

It mimics some an oscilloscope with a timebase of 1 s and a resolution of 10 milliseconds
A "-" or "+" indicates that it sampled a logical low. Any digit d indicates that it sampled a
logical high for d milliseconds during a sample period of 1 s. The digits are in base 11,
1,2,...,8,9,X. Thus X indicates that the signal was high for 10 ms or 1/100 s.


If everything went right you should see some output on the serial monitor.
The connection speed for all but the MB Emulator is 115200.

## Analyzing the test output
What you want to achieve is test output that optimally looks like this:

```
 10, +---------+---------+---------+---------4XXXXXXXXXX3--------+---------+---------+---------+---------
 11, +---------+---------+---------+---------4XXXXXXXXXX2--------+---------+---------+---------+---------
 12, +---------+---------+---------+---------4XXXXXXXXXXXXXXXXXXXX---------+---------+---------+---------
 13, +---------+---------+---------+---------2XXXXXXXXXXXXXXXXXXXX---------+---------+---------+---------
 14, +---------+---------+---------+---------3XXXXXXXXXXXXXXXXXXX8---------+---------+---------+---------
 15, +---------+---------+---------+---------2XXXXXXXXXXXXXXXXXXXX2--------+---------+---------+---------
 16, +---------+---------+---------+---------3XXXXXXXXXX1--------+---------+---------+---------+---------
 17, +---------+---------+---------+---------3XXXXXXXXXXXXXXXXXXX9---------+---------+---------+---------
 18, +---------+---------+---------+---------2XXXXXXXXXX2--------+---------+---------+---------+---------
 19, +---------+---------+---------+---------2XXXXXXXXXXXXXXXXXXX9---------+---------+---------+---------
 20, +---------+---------+---------+---------4XXXXXXXXXXXXXXXXXXX9---------+---------+---------+---------
 21, +---------+---------+---------+---------+XXXXXXXXXXXXXXXXXXXX---------+---------+---------+---------
 22, +---------+---------+---------+---------1XXXXXXXXX9---------+---------+---------+---------+---------
```

You can see the different pulses of the DCF77 signal and there is no noise interferring with the signal.
Also the signal is in phase. There is little to none local clock drift.

What you do not want to see is the signal below.

```
  3, +---------+---------+---------+---------+---------+----2XXXXXXXXXX3---+---------+---------+---------
  4, +---------+---------+---------+---------+---------+----5XXXXXXXXX8----+---------+---------+---------
  5, +---------+---------+---------+---------+---------+----6XXXXXXXXXX4---+---------+---------+---------
  6, +---------+---------+---------+---------+---------+----3XXXXXXXXXX1---+---------+---------+---------
  7, +---------+---------+---------+---------+---------+-----XXXXXXXXXX1---+---------+---------+---------
  8, +---------+---------+---------+---------+---------+----5XXXXXXXXXXXXXXXXXXXX1---+---------+---------
  9, +---------+---------+---------+---------+---------+---2XXXXXXXXXXXXXXXXXXXXX----+---------+---------
 10, +---------+---------+---------+---------+---------+---1XXXXXXXXXXXXXXXXXXXX1----+---------+---------
 11, +---------+---------+---------+---------+---------+---5XXXXXXXXX9-----+---------+---------+---------
 12, +---------+---------+---------+---------+---------+---8XXXXXXXXXXXXXXXXXXX3-----+---------+---------
 13, +---------+---------+---------+---------+---------+---3XXXXXXXXXX2----+---------+---------+---------
 14, +---------+---------+---------+---------+---------+---1XXXXXXXXXXXXXXXXXXX7-----+---------+---------
 15, +---------+---------+---------+---------+---------+---5XXXXXXXXXX2----+---------+---------+---------
 16, +---------+---------+---------+---------+---------+---XXXXXXXXXX4-----+---------+---------+---------
 17, +---------+---------+---------+---------+---------+--5XXXXXXXXXXXX3---+---------+---------+---------
 18, +---------+---------+---------+---------+---------+--5XXXXXXXXXXXXXXXXXXXX------+---------+---------
 19, +---------+---------+---------+---------+---------+---XXXXXXXXX9------+---------+---------+---------
 20, +---------+---------+---------+---------+---------+--3XXXXXXXXXX1-----+---------+---------+---------
 21, +---------+---------+---------+---------+---------+--4XXXXXXXXX9------+---------+---------+---------
 22, +---------+---------+---------+---------+---------+--4XXXXXXXXXXXXXXXXXXX6------+---------+---------
 23, +---------+---------+---------+---------+---------+--8XXXXXXXXX3------+---------+---------+---------
 24, +---------+---------+---------+---------+---------+--9XXXXXXXXXXXXXXXXXXX4------+---------+---------
 25, +---------+---------+---------+---------+---------+-3XXXXXXXXXXXXXXXXXXXX5------+---------+---------
 26, +---------+---------+---------+---------+---------+-3XXXXXXXXX9-------+---------+---------+---------
 27, +---------+---------+---------+---------+---------+--XXXXXXXXXX-------+---------+---------+---------
 28, +---------+---------+---------+---------+---------+-2XXXXXXXXXXXXXXXXXXX5-------+---------+---------
 29, +---------+---------+---------+---------+---------+-3XXXXXXXXX8-------+---------+---------+---------
 30, +---------+---------+---------+---------+---------+-5XXXXXXXXX9-------+---------+---------+---------
 31, +---------+---------+---------+---------+---------+-4XXXXXXXXX6-------+---------+---------+---------
 32, +---------+---------+---------+---------+---------+2XXXXXXXXXX5-------+---------+---------+---------
 33, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
 34, +---------+---------+---------+---------+---------+-9XXXXXXXXX--------+---------+---------+---------
 35, +---------+---------+---------+---------+---------+3XXXXXXXXX9--------+---------+---------+---------
 36, +---------+---------+---------+---------+---------+6XXXXXXXXXXXXXXXXXXXX3-------+---------+---------
 37, +---------+---------+---------+---------+---------+7XXXXXXXXX7--------+---------+---------+---------
 38, +---------+---------+---------+---------+---------+6XXXXXXXXX9--------+---------+---------+---------
 39, +---------+---------+---------+---------+---------+6XXXXXXXXX4--------+---------+---------+---------
 40, +---------+---------+---------+---------+---------3XXXXXXXXXX2--------+---------+---------+---------
```

It indicates that the received signal is clean but your local clock drifts way to much. (In this case
DCF77 seems to be slow. However since DCF77 is accurate by definition the local clock must be to fast.
A drift in the other direction would indicate the local clock as to slow.)
This is typical for the Arduino Uno. It is the tell tale sign of a resonator instead of a crystal.
It is the worst case as this can not be easily fixed in software.

The issue is not the large drift due to the inaccuracy of the resonator. The issue is that a resonator
is also not very stable. That is any drift compensation will face a hard time to readjust in a
timely manner.

The best fix for this is to replace the resonator by a crystal or to get a crystal based Arduino clone.
Or you switch to the Arduino Due which (at the time of this writing) still comes with a crystal.

What you might also see is a signal like the next one.
```
159, XXXXXXXXXXXXXXXXXXX69XXXXXXXXXXXXXXXXXXXX4--------+-4XXXXXXXXX7XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
164, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX2-------+-5XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
165, XXXXXXXXXXXXXXXXXXXXXX3XXXXXXXXXXXXXXXXXX7--------+---------+1XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
166, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX---------+---------+4XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
167, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX7--------+---------+7XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

This is unwanted as well but easy to fix. It indicates that the receiver and the sketch have a different
opinion on what constitutes a zero and what constitutes a one. This is very easy to fix in software.
If your setup contains the line

```
const uint8_t dcf77_inverted_samples = 1;
```
replace the value by 0. If it is already 0 replace it by one. Then you should get a proper signal.

You might also see something like
```
159, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
160, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
161, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```
or
```
 33, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
 34, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
 35, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
```

This is the tell tale sign of improper wiring. Ensure that your receiver is properly powered and connected
to the proper input pin.

You might also see somthing like this:
```
 1, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
 2, +-432----1XXXX6-----5XXXXXXXXXXXXXXXXXXXXXXX6-----+---------+3--------+5--------+---------+--X5-----
 3, +------2--XXXXXX8--XXXXXXXXXXXX1--------+---8XX---+------5--+---------+--3-XXXXXXXXXXX311X1---------
 4, +---5XXXXXXX6-------463XXXX66XXX835-6XX-+--17-----+---------+------3X52X1-4-683XXXXXXXXX6XX5----3XX4
 5, +---------+---------5XXXXXXXXX7---------+----6XX75X4--------+--8XXXXXXX---------6---------+XXXXXX3--
 6, +-----2---+518------+XXXXXX7-XXXXXX73XX7+--12-----+---------+-----9---+--6X9-864XX6-------+---------
 7, +8XX1---X-9XX2--7XX2+-------482XXXXX8---+------1911------2X1+---------+16-------+---------+------2XX
 8, XXX2------+-----77-2XXXXXXXXX62XXXXX2---+---------+2-----28-+-2XX8----6XX1---7XXX3-----4X-+53-X8--63
 9, 6X8-------+------X5-6XX2--9XXXX1--------+---------+---3X5-79+-4X6---27+---------+---------+----3----
10, +-----6XXXXXXXXXX9--92-XXXXXXXX-9XX2----+7XXXXXX6-+---------+-28XX32--+-------16+-9XXXX396XX743-----
11, +------8XXXXX25-----+57XXXXXXX1X94XXXXXX522-------+--------5+---------+-----6XX24---------+------5--
12, +-----6XX6+---------7XXXXXXXXX5-XX2-----9--------667----68--+---------+---------4-2XXX--33XXXX1--17-
13, +-7X-2XX846--------4XXXXXXXXXXXXXXXXXXXXX6--------+---------+---------6X7----3XXX1--------+----88---
14, +---------+--------3X2-6XXXXXXXXXXXXXXXXXXX2------+---------+---------+---------XXXXX3----+---------
15, +--8XXXX7-+--2XXXXXXXXXXXXXXXXXXXXXXXXXXX---------+--3XXXX3-+--67-----+-49--8X--+---------+---------
16, +359XXX9--+--------3XXXXXXXXXXX5--------+-47X-----+-------64+-7-------+---------+----79---+--3------
17, +----56---+---------+1XXXXXXXXXXXXX--141XX8XX4-89-+---------+--------1XX9-----3XX1-XX-----+--XXXX5--
18, 5XX12X2---+---------4X9XXXXXXXXX6-------+-6-------+---------+-------7XXXX53-64--+---------+25XXX8-4X
19, X9--------+--8XXX97XXXXXXXXX5-+---------+---------+--------X73--7XXXXX7--------1+-----71--+6X1------
20, +-------9XX6----2X8X8XXXXXXXXXXXX-------+99-------+--7X4----+1X5------+7X2------+--9XXX27XXXXXXXXXX6
```

This is a properly wired receiver that picks up a noisy signal. This is actually what the library is designed
for. As long as you have no clock drift (that is a crystal and not a resonator) this is OK.

## Adapting the sketch configuration

Most of the example sketches come with some kind of configuration settings. Most of the example sketches
contain sections similar to this

```
const uint16_t EEPROM_base = 0x20;

const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = 19; // A5

const uint8_t dcf77_inverted_samples = 1;

const uint8_t dcf77_analog_samples = 1;

const uint8_t dcf77_monitor_led = 18;
```

The most straightforward approach is to use them as is and adapt only the "dcf77_inverted_samples" constant
such that the signal can be decoded properly. Admissible values are 0 or 1. The proper value depends on
your choice of module.

The "dcf77_analog_samples" mode controls how the sketch will sample data. If set to 0 digital sampling is used
and the receiver must be connected to the "dcf77_sample_pin".
If set to 1 analog sampling is used and the receiver must be connected to the "dcf77_analog_sample_pin".
In the default setup the physical pins digital 19 and analog 5 are the same pin. This is admissible.

You may of course use different pins to connect the receiver. But read the comments on the monitor LED and the light show
before you change the pin assignments.

Choosing digital vs. analog sampling controls the voltage threshhold for sampling "high". If the
receiver can not drive 5 V or if the receiver is weak and the pin is loaded
(e.g. with a Blinkenlight shield) then analog sampling may be necessary. In all other cases it does
not really matter. The only drawback of analog sampling is slightly higher contention in the
decoder interrupt. Thus if you have a standard Arduino you might want to prefer digital sampling.

The "dcf77_monitor_led" indicates where a monitoring led is connected to visualize the received
signal state. Obviously most Arduino users might want to set this to 13. There is some pitfall
here though. Most of the example sketches include some kind of light show in case a
[Blinkenlight shield](http://www.amazon.de/gp/product/3645651462/?ie=UTF8&camp=1638&creative=6742&linkCode=ur2&site-redirect=de&tag=wwwblinkenlig-21)
is connected. The pin settings for the light show are somehting like

```
const uint8_t lower_output_pin = 2;
const uint8_t upper_output_pin = 17;
```

Obviously this will interfere with setting any pin to a value between 2 and 17.
You have two options here. Either you modify the code to get rid of the light show or you
map the light show to pins that are otherwise unused. E.g.

```
const uint8_t lower_output_pin = 2;
const uint8_t upper_output_pin = 3;
```

would allow to use all pins >3 for something else. Do not set the values to something out of
the range of valid pins as the Arduino libraries do not implement bounds checking. Setting
these values out of admissible ranges is most likely to result in undefined behaviour.


# Examples
The library comes with a set of examples which serve 3 different purposes.

The *DCF77 Scope* and the *Swiss Army Debug Helper* are primarily intended for
troubleshooting and error analysis.

The *Simple Clock* and the  *Simple Clock with Timezone Support* are intended
to illustrate the basic use of the library.

All other examples are some applications of the library that may provide some
utility on their own right.

All the examples but the *MB Emulator* communicate through the serial port
running at 115200 baud.

## DCF77 Scope
The *DCF77 Scope* is the only example that does not use the library. It is a
standalone sketch. Its main purpose is for troubleshooting the receiver
module and the wiring. If the *DCF77 Scope* indicates any trouble then all
other sketches will most likely not work.

It mimics some an oscilloscope with a timebase of 1 s and a resolution of 10 milliseconds
A "-" or "+" indicates that it sampled a logical low. Any digit d indicates that it sampled a
logical high for d milliseconds during a sample period of 1 s. The digits are in base 11,
1,2,...,8,9,X. Thus X indicates that the signal was high for 10 ms or 1/100 s.

Output of the sketch usually looks as follows.

```
  1, +---------+---------+---------+---------4XXXXXXXXXX2--------+---------+---------+---------+---------
  2, +---------+---------+---------+---------4XXXXXXXXXXXXXXXXXXXX---------+---------+---------+---------
  3, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
```

The first column shows the seconds since measurement started. The second column is 1 second of sampled
signal. In the example above second 3 shows 1 second with a low signal. (Maybe a minute marker).
You may notice that both "+" and "-" indicate a low. The point is that the "+" are spaced by 9 "-".
Or with other words a period starting with a "+" and ending with the "-" before the next "+" is 100 ms
(as measured by the local clock).

You may also notice that in second 1 and 2 the transition to high starts with a "4" instead of an "X".
Since the signal is obviously clean this is a sign that the actual transitions does not start at a
10 ms boundary but 4 ms before the end of the 10 ms sampling interval.

The DCF77 Scope can also be used to analyze local clock drift. If the decoded signal drifts to the
right it seems as if DCF77 is to fast. Of course DCF77 is accurate by definition, hence the local clock
must be to slow. If the signal drifts to the left the same holds true with reversed signs. That is
the local clock would be to fast.

A drift of up to 100 ppm is within the design margins of the library. If you run the analysis
for 1000 seconds or roughly 17 minutes you should see a drift of at most 1/10 s. If you see a
significantly larger drift, e.g. more than 1000 ppm or more than 1/10 s within 2 minutes then
you have resonator based Arduino and the library will not be able to lock to the signal.

## Swiss Army Debug Helper

The main purposes of the *Swiss Army Debug Helper* is for troubleshooting the library during development.
It allows to peek into a lot of the libraries internals. This is mostly interesting for developer and
for support purposes. This is the tool that I use almost exclusively during development and testing.

It does not serve as an example of using the API. In fact it calls library internals which are supposed
to be changed incompatible without prior notice.

It is included in case anyone experiences difficulties with my library. I will then need suitable
logs to be generated with this tool.

## Simple Clock
As the name implies this is a simple clock implementation. It's main purpose is to illustrate the
most basic use of the library. The code is very short. It should give you a starting point for
your own developments.

## Simple Clock with Timezone Support
From the feedback I got to my library I learned that a lot of its users are outside the CET/CEST
timezone. Those users require an adjustment for the timezone to their timezone. The example assumes
a fixed timezone offset.

## MB Emulator
The *MB Emulator* emulates a Meinberg DCF77 clock. That is it implements the "Meinberg protocoll".
Thus it communicates at 9600 7E2. In particular this is supported by most NTPD implementations.
You might want to read the NTPD configuration documentation or consult
[my web page](http://blog.blinkenlight.net/experiments/dcf77/meinberg-emulator/ntpd-configuration/)
to learn how to configure this.

Of course nowadays you can setup an NTP server with GPS. But if you require to add an independent
clock or if you have no clear view of the sky then DCF77 is still a valid option.

At the time of this writing the communication mode 9600 7E2 is NOT supported by the Arduino DUE
and there may be other Arduinos with the same issue. The "standard" Arduinos and clones typically
support this though.

## Superfilter

The "super filter" is intended to go between your project and a DCF77 module. It was originally
published [on my webpage](http://blog.blinkenlight.net/experiments/dcf77/super-filter-and-synthesizer/)
and is documented there. Basically it will decode the DCF77 signal with my library and then
synthesize a DCF77 signal locally. This can then be fed into your project. So baiscally any
existing project can get the noise tolerance of my library without any code changes at all.

## The Clock

*["The Clock"](http://blog.blinkenlight.net/experiments/dcf77/the-clock/)* was the starting point
of my dcf77 library project. This is the library version of
["The Clock"](http://blog.blinkenlight.net/experiments/dcf77/the-clock/). This sketch exists
for illustration and historical purpose. The "Swiss Army Debug Helper* offers a superset of the
functionality.

## Time Switch

The *[Time Switch](http://blog.blinkenlight.net/experiments/dcf77/dcf77-library/time-switch/)* is
a fully configurable time switch. It controls 16 independent channels controlled by 100 Triggers.
A trigger can fire on a specific date/time or on reoccuring events. Triggers may be grouped for
even more sophisticated control. Each trigger can control any subset of the outputs.

For a full description read the
*[documentation here.](http://blog.blinkenlight.net/experiments/dcf77/dcf77-library/time-switch/)*

Due to the lack of EEPROM the time switch does not support the Arduino Due. Currently the only
supported Arduino variants are AVR based.


# Configuration

The library will work out of the box. However for ARM based controller boards (Arduino Due)
it supports two different configurations.

1) Lower resolution (10 ms) with higher noise tolerance or
2) Higher resolution (1 ms) lower noise tolerance.

The setup is controller with the constant

```
    const boolean want_high_phase_lock_resolution = true;
    //const boolean want_high_phase_lock_resolution = false;
```

in namespace ```Configuration```. You may want to adapt the dcf77.h file according to your preferences.
