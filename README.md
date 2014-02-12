dcf77_library
=============

Noise resilent DCF77 decoder library for Arduino

This repository contains a DCF77 decoder library for Arduino.
It was designed with the goal to create the most noise resilent
decoder possible.

There are also some ready to run examples.

For the details and background information you may want to consult
the DCF77 section of my blog http://blog.blinkenlight.net/experiments/dcf77/.


Attention: the library requires a crystal oscillator. The Arduino
Uno comes out of the box with a resonator. Thus it will not work
with this library. "Older" designs like the Blinkenlighty
http://www.amazon.de/gp/product/3645651306/?ie=UTF8&camp=1638&creative=6742&linkCode=ur2&site-redirect=de&tag=wwwblinkenlig-21
have a (more expensive) crystal oscillator.
