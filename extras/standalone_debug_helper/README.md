#The Standalone Debug Helper

## What It Is
The **Standalone Debug Helper** is a test tool for [my noise resilient DCF77 library project.](https://blog.blinkenlight.net/experiments/dcf77/dcf77-library/) my noise resilient DCF77 library project. The *standalone* part refers to the fact that this tool is not intended to compile for Arduino. It is intended to compile for Linux. That is it is a wrapper around the library which allows to test it directly on Linux without deploying it to Arduino first.

The advantages are enourmous. Because the library is now running on a *real* operating system and because my Laptop has more than 100 times the CPU power of an Arduino testing becomes much easier.

## Features

* It can synthesize a signal and feed it into the library.
* It can read a recorded signal from files and feed it into the library.
* It can add noise to a signal.
* It can simulate clock drift.
* It does not run in real time. Instead it runs at maximum possible speed. Thus the behaviour over days or even weeks can be analyzed in an acceptable time frame.
* Different output modes for different types of analysis are available.
* Features can be controlled from the command line.

And because it runs on Linux it is possible to easily debug using a real debugger. No need for print statements or hardware debuggers. This killer feature comes for free :)

## How To Build It

Instead of make I prefer scons. If you have not installed scons you need to install ony your system before building. You can compile the program just by issuing the command scons in the directory which contains the SConstruct file. If the compilation succeeds it will immediately run the compiled program without any parameters. I did this because it saves me a little bit of typing every once in a while. If you do not like this behaviour remove the "run" dependency from the SConstruct.

If the build succeeded there will be a file "main" in the directory. This is the executable.

## Running It

The debug helper can run without any arguments. Calling it witout any arguments will result in the following output.

    > ./main
    ./main
    main.cpp
      compiled: Mon May  8 19:34:24 2017
      architecture:          UNIX
      compiler version:      4.8.4
      dcf77 library version: 3.2.0

    Configuration
      high_phase_lock_resolution:    0
      phase_lock_resolution:         100
      quality_factor_sync_threshold: 1

    synthesizer parameters
      YY-MM-DD: 17-1-1
      weekday:  7
      hh:mm:ss: 0:0:0

      uses_summertime:                0
      abnormal_transmitter_operation: 0
      timezone_change_scheduled:      0
      leap_second_scheduled:          0

      synthesized_signal_length: 1000

    signal shaper parameters
      startup_ms:             0
      startup_signal:         0
      distort_startup_signal: 0

      drift ppm:   0
      drift_pp16m: 0

      fade_min_ms    : 0
      fade_max_ms    : 0
      fade_min_gap_ms: 0
      fade_max_gap_ms: 0
      faded_signal   : 0

      random_hf_noise_per_1000: 0

    filter parameters
      millisecond_samples: 0


    Final Clock State

    Decoded time: 17-01-01 7 00:15:40 CET ..
      17.01.01(7,0)00:16:39 MEZ 0,0 3 p(6000-0:255) s(165-0:23) m(62-36:5) h(52-26:5) wd(26-13:3) D(52-39:2) M(39-26:2) Y(52-39:2) 26,13,13,50
    output triggered
    Clock state: synced
    Tick: 1
    confirmed_precision ?? adjustment, deviation, elapsed
    0.0000 ppm, @+ , 0.0000 ppm, 0 ticks, 0 min + 3501 ticks mod 60000

    Clock State Statistics
      0 useless : 419
      1 dirty   : 38
      2 free    : 0
      3 unlocked: 0
      4 locked  : 0
      5 synced  : 543

    Clock State Transition Statistics
      useless  => useless : 419
      useless  => dirty   : 1
      dirty    => dirty   : 37
      dirty    => synced  : 1
      synced   => synced  : 542

    Quality Factor Statistics
      0: 418
      1: 38
      2: 382
      3: 162
      4: 0
    average: 1.288  standard deviation: 1.16836

    Prediction Match Statistics
      49: 0
      50: 456
      51: 0
    average: 50  standard deviation: 0

## Interpreting the Output

Lets analyze the pieces of the standard output. The first block is pure boilerplate.

    ./main
    main.cpp
      compiled: Mon May  8 19:34:24 2017
      architecture:          UNIX
      compiler version:      4.8.4
      dcf77 library version: 3.2.0

It tells us about the name of the source file and the executable. The compile time, the target architecture, the compiler and the library version. Since this tools is intended also for "support" purposes I want to have this information always included. Thus I always include it at the top.

The next block tells us about the configuration of the library.

    Configuration
      high_phase_lock_resolution:    0
      phase_lock_resolution:         100
      quality_factor_sync_threshold: 1

This configuration block refers to configuration structure in dcf77.h. I need this for analysis purposes. For ATmega targets the configuration would usually have a phase lock resolution of 100 ticks per second. For ARM / SAM targets the configuration would usually have a higher resolution of 1000 ticks per second. The sync_threshold indicates indicates how agressive or conservative the library tries to acquire a lock. 1 is on the agressive side, 3 is on the conservative side. The point is that in agressive mode there is a higher chance of errors but sync can be acquired faster. In conservative mode errors are (so far) unheard of but syncing may take significantly longer. In doubt 2 (="normal") is a good compromise. All of these parameters have a significant impact on the noise tolerance and lock behaviour. However even for pretty poor signals you might notice no difference. The difference will usually show up only for abmysal signal quality.

Then we have the synthesizer parameters.

    synthesizer parameters
      YY-MM-DD: 17-1-1
      weekday:  7
      hh:mm:ss: 0:0:0

      uses_summertime:                0
      abnormal_transmitter_operation: 0
      timezone_change_scheduled:      0
      leap_second_scheduled:          0

      synthesized_signal_length: 1000

This indicates that this test run was processing synthesized data. The synthesizer was initialized to the 1st of January 2017, 00:00:00 CET. Weekday was set to 7 (= sunday). Also the control flags indicate that the synthesizer indicated no abnormal transmitter operation and no leap second was scheduled.

Finally we see that it generated 1000 seconds of signal data. Of course the synthesizer parameters can be changed by commandline parameters. It is also possible to use input from logfiles. No matter what the choice of the signal source is the signal goes through a chain of "filters" which I prefer to call "signal shapers" before it is passed to the library.

![Signal Source --> process_1_kHz_tick_data --> inject_drift --> inject_fade --> inject_random_hf_noise --> DCF77_decoder_library](./Signal_Shaper_Sequence_600dpi.png  "Signal Shaper Sequence")


For the test run none of the shapers adds any distortion to the signal.

    signal shaper parameters
      startup_ms:             0
      startup_signal:         0
      distort_startup_signal: 0

      drift ppm:   0
      drift_pp16m: 0

      fade_min_ms    : 0
      fade_max_ms    : 0
      fade_min_gap_ms: 0
      fade_max_gap_ms: 0
      faded_signal   : 0

      random_hf_noise_per_1000: 0

Finally there is the millisecond_samples parameter.

      millisecond_samples: 0

This parameter indicates if the library shall sample with millisecond resolution or with 10 millesecond resolution (and 10 times oversampling). This parameter is somewhat magic as this one gets "compiled" (!) into the library. The trick is that the library is defined by templates. The test wrapper will create two instances of the library and then the command line switch is used to chose which of the library instances will be processing the data. Since it is set to false 10 ms worth of 1 ms samples will be averaged and then decoded by the library.

There could be additional debug output. This is activated by the command line switches documented somewhat below. Since there is no additional debug output active the debug helper ends with its standard statistics. I use this for quick checks and I always want to see it.

The first thing we see is the decoded time at the end of the run.

    Decoded time: 17-01-01 7 00:15:40 CET ..

This appears strange. 1000 seconds after 00:00:00 ought to be 00:16:40. However this is correct. The synthesizer always generates a DCF77 signal for the next minute. That is if it is initialized with 00:00:00 it will generate a signal sequence of 60 seconds duration for 00:00:00. Hence the decoded time after 60 seconds would be 00:00:00. Consequently after 1000 seconds it should be 60 seconds (= 1 minute) behind 00:16:40. As we can see this is actually the case. The two trailing dots indicate the summer time and the leap second flags. Both are properly decoded as false.

The next thing is a peek into the library interal decoder stages and their signal to noise detectors.

      17.01.01(7,0)00:16:39 MEZ 0,0 3 p(6000-0:255) s(165-0:23) m(62-36:5) h(52-26:5) wd(26-13:3) D(52-39:2) M(39-26:2) Y(52-39:2) 26,13,13,50
    output triggered


Looks a little bit overwhelming but lets disect it step by step. The line starts with

      17.01.01(7,0)00:16:39 MEZ 0,0

This is a concatenation of the internal phase state of all decoder stages. It has the format

      YY.MM.DD(W,w)hh:mm:ss ZZZ L,C

Where YY is the year, MM the month, DD the day, W the decoded weekday, w the computed weekday derived from YY.MM.DD, hh the hour, mm the minute, ss the second, ZZZ the timezone, L the leap second scheduled flag, C the timzone change scheduled flag. Notice that the decoded weekday in this example is 7 which indicates a Sunday. The computed weekday is 0 which also indicates Sunday. Under good reception conditions both values will match modulo 7. If they do not match this hints at poor signal quality.

The decoder phase state string is followed by a single number which is the "global quality indicator". It is derived from the numbers which follow plus consideration of the match (or mismatch) of decoded weekday vs. computed weekday. Bigger implies better quality. This number is used by the library to decide on state transitions (e.g. synced vs. locked).

The next part is an indicator for the phase lock quality. It has the format

    p(s,n:q)

The numbers s for the signal and n for the noise are computed by convolution. If s is big and n is low this is an indicator for a good signal. The number q is an indicator for the quality. Since in the example s is 6000 and n equals zero the quality indicator is maxed out at 255.

Then there are more quality indicators.

    s(165-0:23) m(62-36:5) h(52-26:5) wd(26-13:3) D(52-39:2) M(39-26:2) Y(52-39:2)

They refer to the seconds, minutes, hours, weekday, day, month and year decoder stages. For each state we have a triple of signal and noise and quality indicators. Because the inner workings of these stages are different from the phase lock they usually have a poorer signal to noise ratio. Since the minute and hour decoder can leverage the parity bits for their statistics they can usually reach a better signal to nose ratio. In the example this is clearly visible. Attention: these values are decoder internals and do not refer to dB or any other typical measure.

Then there are the counts for the 3 flags summertime,  timezone_change_scheduled and leap_second_scheduled.

    26,13,13,50

Since the timezone is encoded in two bits the count for the summertime flag is (for a clean signal) twice as high as for the other flags.

And last but not least there is the predication_match. The library synthesizes a clock data stream depending on the decoded time and matches the data stream with the received data stream. If all bits match it will result in 50. If the signal is full with noise you will see numbers around 25. If this number would be 0 it would indicate that for whatever strange reason the synthesized signal is exactly the inverse of the received signal.

Then we have

    output triggered
    Clock state: synced

It tells that the library has triggered an output request (which is supposed to happen once per second). It also tells us that the clock library assumes it is in sync with DCF77. More on the other clock states below. Before we come to the clock state statistics we have the internals of the auto tune mechanis. This is supposed to figure out the accuracy of the crystal clock. If the signal is good enough it will figure out the deviation of the crystal frequency from its nominal frequency and adjust accordingly.  Of course this will only work if the crystal frequency is sufficiently stable (with other words there must be no huge swings in ambient temperaure).

    Tick: 1
    confirmed_precision ?? adjustment, deviation, elapsed
    0.0000 ppm, @+ , 0.0000 ppm, 0 ticks, 0 min + 3501 ticks mod 60000

From the numbers we can see several things. First of all the "confirmed precision" is "0 ppm." This indicates that auto tuning did not yet succeed. Then we have "@+". "@" indicates calibration is running while "." would indicate that it is not. "+" tells us that calibration is currently possible while "-" would tell us that it is not. We also learn that so far no frequency adjustment is applied. The we have how many 1 ms ticks the (crystal) clock deviates from DCF77. And finally there is the runtime in minutes / ticks mod 60000 of the current calibration cycle. The total number of elapsed ticks is then internally computed by means of the chinese remainder theorem.

After the clock state there is a bunch of statistical information with regard to the test run. The first is the clock state statistics.

    Clock State Statistics
      0 useless : 419
      1 dirty   : 38
      2 free    : 0
      3 unlocked: 0
      4 locked  : 0
      5 synced  : 543

It indicates that the clock was in state "useless" for 419 seconds. This is the startup state. Notice that it took 419 seconds for startup althoug the signal was clean. The "dirty" state is the transitional state where it is almost synced. The decoded time may or may not be good. This state was in place for 38 seconds.  Then it was in state "synced" for 543 seconds. This is the most desirable state because it is decoding properly and phase locked to DCF77.

The second best state is "locked". Although the data decoders fail to figure out the decoded time the clock can still keep a phase lock with DCF77. Unless a leap second happens it will still be in fully in sync with DCF77. The third best state is "unlocked". The clock lost even the phase lock. However the event is not that long ago. In particular the clock assumes it did not drift away from DCF77 more than 1/3 second. It is possible for the clock to transition from "unlocked" to "locked" directly.

Finally there is "free". This indicates that the clock synced at least once but currently it is neither synced not phase locked and the last lock was quite a while ago. The only way to get into a better state is a proper sync.

Since the synthesized signal was 100% perfect the states "free", "unlocked" and "locked" were not observed during this run.

The next part are the transition statistics.

    Clock State Transition Statistics
      useless  => useless : 419
      useless  => dirty   : 1
      dirty    => dirty   : 37
      dirty    => synced  : 1
      synced   => synced  : 542

  They show the number of state transitions from each state into other states. In this case the clock remained 419 seconds in the state "useless". There was one transition from "useless" to "dirty". It remained 37 seconds in the state "dirty". There was one transition from "dirty" to "synced". Finally the clock stayed in state "synced" for 542 seconds. This is the expected behaviour for a clean signal. The clock will start in state "useless" and eventually reach "synced" and then stys there. For a noisy signal transitions from "synced" to poorer states (and back) are common.

  The quality factor statistics show how many seconds the clock was running with a specific quality factor.

    Quality Factor Statistics
      0: 418
      1: 38
      2: 382
      3: 162
      4: 0
    average: 1.288  standard deviation: 1.16836

The quality factor is the "global quality indicator" that I already explained below. With the default setting the clock will transition to "synced"    once this reaches 2. If we add up the numbers we get 382+162 = 544. According to the state transition statistics the clock was in mode synced for 543 seconds. You might wonder why one second is missing. Well, the last second in the measurement period is not counted as a state transition. Hence the sum of state transitions is expected to be 1 lower.

Finally we have the prediction match statistics.

    Prediction Match Statistics
      49: 0
      50: 456
      51: 0
    average: 50  standard deviation: 0

This is the statistics for the prediction match I explained already above. The prediction match value is computed once per minute but counted on a per second basis. Also it will only kick in as soon as the clock is synced for the first time. Then it starts to locally synthesize the expected signal and compare it with the received signal. In this case we see only 50 bit matches which is an indicator for a perfect signal quality.

## Command Line Options

### Help Option

The Standalone Debug Helper has some command line options in order to change its behaviour. The first one is the "-h" or "--help" option. It will display a comprehensive overview over the options and a short explanation for each.

    ./main -h
    Usage: ./main [options]
    Standalone dcf77 library tester

      -h, --help                                 display this help text

      -v, --view=[adsy]                          verbose output depending on option set
                                                   use option d twice to get more output
                                                   a: show command line argument parser arguments
                                                   d: show debug output
                                                   s: show debug scope output
                                                   y: show synthesizer state

      -i, --input=[0|1|2]                        define signal source
                                                   use signal synthesizer [0],
                                                   read from stdin in swiss army debug helper "scope (Ds)" format [1],
                                                   read from stdin in http://www.dcf77logs.de/ format [2]
                                                   default: 0
      -I, --Infile=<filename>                    name of input file
                                                   default:  (= read from stdin)

                                                 Synthesizer options (applicable for --input=0 only):
      -t, --time=YY.MM.DD@hh:mm:ss                 set start date and time, default: <<<TBD>>>
      -S, --summertime=[0|1]                       set summertime [1] or wintertime [0], default: 0
      -a, --abnormal_transmitter_operation=[0|1]   set abnormal_transmitter_operation flag, default: 0
      -c, --timezone_change_scheduled=[0|1]        set timezone_change_scheduled flag, default: 0
      -l, --leap_second_scheduled=[0|1]            set leap_second_scheduled flag, default: 0
      -s, --signal_length_s=n                      generate test signal data for n seconds, default: 1000

                                                 Signal distortion options (applicable for --input=[0|1|2])
      -d, --startup_ms=n                           prepend test signal with constant value for n milliseconds, default: 0
      -u, --startup_signal=[0|1]                   value of the startup signal, default: 0
      -D, --distort_startup_signal=[0|1]           distortion shall already be applied in the startup phase, default: 0

      -p, --drift_pp16m=[+|-]n                     (signed) drift in parts per 16 million, default = 0
      -P, --drift_ppm=[+|-]n                       (signed) drift in parts per million, default = 0
      -f, --fade=a,b,c,d,[0,1]                     inject random signal fade
                                                     with random duration in [a,b] milliseconds with
                                                     random gap length in [c,d] milliseconds with
                                                     signal value e,
                                                     default value: 0,0,0,0,0
      -r, --random_hf_noise_per_1000=n             replace samples with probability n/1000 by a random value, default: 0

                                                 Filter parameter options
      -m, -millisecond_samples=[0|1]               aggregate 10 samples before filtering [0],
                                                   process millisecond samples directly [1]

    Exit status:
      0:  OK
      4:  Invalid option or parameter

### View Options

The "view" option takes additional letters to display more output for in depth debugging. The least useful option is "-va". It will show some information about the argument parsing process. It is only usefull while debugging the debug helper's argument parser. Its behaviour is best understood by looking into the source code. Unless you are troubleshooting the argument parser (and thus are really looking into the source code) this is useless.

The "-vd" and "-vdd" options are much more useful. They display output like so:

    ./main -vd
    (...)
    Decoded time: 17-01-01 7 00:06:37 CET ..
        17.01.01(7,0)00:07:37 MEZ 0,0 2 p(4650-0:255) s(57-27:5) m(20-12:2) h(16-8:2) wd(8-4:1) D(16-12:1) M(12-8:1) Y(16-12:1) 8,4,4,255
    output triggered
    Clock state: synced
    Tick: 1000
    confirmed_precision ?? adjustment, deviation, elapsed
    0.0000 ppm, .+ , 0.0000 ppm, 6499 ticks, 7 min + 45799 ticks mod 60000

The semantics of this output was already explained above. In mode "-vd" it will be triggered whenever a state change occurs. In mode "-vdd" it will be triggered for each second of processed data.

Mode "-vs" delivers something that was not yet explained. It shows some ASCII art which tries to be sort of an oscilloscope.

    ./main -vs
    (...)
        1, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
        2, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        3, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        4, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        5, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        6, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        7, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        8, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        9, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       10, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       11, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       12, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       13, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       14, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       15, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       16, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       17, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       18, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       19, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       20, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       21, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       22, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       23, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       24, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       25, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       26, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       27, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       28, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       29, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       30, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       31, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       32, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       33, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       34, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       35, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       36, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       37, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       38, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       39, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       40, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       41, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       42, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       43, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       44, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       45, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       46, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       47, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       48, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       49, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       50, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       51, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       52, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       53, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       54, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       55, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       56, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       57, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       58, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
       59, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
       60, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------

Notice that it displays 100 samples per second although it processes 1000 samples per second. Each 1/100 second ticks is either "-" or "+" or a digit in the range 1-9 or an "X". These symbols indicate how many of the 10 1/1000 second samples in this 1/100 second were "1". "-" and "+" imply none at all, "X" implies all were "1". Digits between 1 and 9 give the count of ones found in the sampling period.

As the output above is generated from a clean synthesized signal you see optimal output without any noise and any drift. Pay special attention to the second mark in second 60. Also notice the missing signal in seconds 2-15. The real DCF77 signal contains encrypted weather information that I can not synthesized.

The last of the view options exhibits the synthesizer state.

    ./main -vy
    (…)
    next second will be:   17.01.01(7,0)00:00:00 MEZ 0,0
    next second will be:   17.01.01(7,0)00:00:01 MEZ 0,0
    next second will be:   17.01.01(7,0)00:00:02 MEZ 0,0

It is admissible to combine the view modes. Of course the output will become harder to read but sometimes it is required for in depth analysis.

### Input options

The input options control where the input for the clock library shall originate. The (default) option is "-i0" which indicates that use of the build in signal synthesizer.

With "-i1" it is possible to parse files that were generated by the "Swiss Army Debug Helper" sketch. The sketch must run in mode "Ds".  The parser is smart enough to discard useless lines from the log files. Thus any other mode e.g. "Da" that outputs additional debug information can still be used to create parsable log files.

 Finally with option "-i2" it can read files in the format found at www.dcf77logs.de  Unfortunately these log files do not possess a well defined syntax. Sometimes they contain natural language comments. In particular parsing in mode "-i2" may result in very unexpected results. In this cases please double check if the log file is "clean".

For  "-i1" or "-i2" the tool will read from stdin by default. With the additional option "-I" it is possible to read directly from an input file. This is in particualr useful in combination with automated test tools.

### A real world example

Now that we are through the basic options lets have a look at some excerpt of real data.

    > ./main  -vs -i1 -I test

    (...)

      484, 93--------+---------+-------XX1---------+---------+----4XXXXXXXXXXX1--+-------1XXXXX3-----+---------
      485, +---------+---------+---------+--4XXX9XXX7--------+----16--7X574------+---------+---------+---------
      486, +------2XXXXXXXXXXXX3---4XX3--+---------+---------+-----XXXXXXXXXXXX7-+9XXXXXXXX9--4X3----+-5XX8----
      487, +---------+--1------+---------+---------+---------+---------+8XX4-----+---------+---------+---------
      488, +---------+---------+---------+-------7X12--6X5-5X7--------8XXXXX-----+---------+---------+--5X1----
      489, +---------+---------+---------+---------+-9XX1----+---4XXXXXXXXX2-----+---------+---------+---------
      490, +---------+---------+3XX2---6X+---------+-----5X7-+----5XXXXXXXXXXXXXXXXXXXXXXXXXXX2------+---------
      491, +6X77X1---+---------+-88------+---------+---------+---9XXXXXXXXX4-----+-------2X8---------+---------
      492, +---X1----+---------+---------+---------+---------+---1XXXXXXXXXXXXXXXXXXX9-----+---------6XXX7-4X5-
      493, +---------+------7--+---------+---------+---------+-----XXXXXXXX7-----+---------+---------+---------
      494, +---------1XX5------+X7-------+---------+---------+-----9XXXXXXXXXXXXXXXXXX6----+---------+---------
      495, +---------5X6-------+---------+---------+-------3XXX58XXXXXXXXXXXXX3--+--1XXX---+---------+---------
      496, +---------+------1--+---------+--7XXX3--9X6--2----+--6XXXXXXXXXX8-----+----8XXXXX2--------+---------
      497, +---------+---------+---------+-----6XXXXXXXX8----+----3XXXXXXXXXXXXXXXXXX7-----+--2XXXX2-+---------
      498, +---------+---------+61----4XXXXXXX52---+---------+--3XXXX8-+-9XXX2---+---------+---------+---------
      499, +---------+-22------+---------+---8XXX9-+---------+----XXXXXXXXXXXXXXXXXXXX7----+---------+---------
      500, +4XXXXXX7-5XXX------+---------+---------+---------+-7XXXXXXXXXXXXXXXXXXXXXX31XXX1---------+-4-------
      501, +---X1----+---------+----3X6--+---------+------5XX8----5XXXXXXXXXXXXXXXXXXXXXX8-+-9X6-----+8X3------
      502, +---------+---------+---------+--------2X88XXXXXXXX1---8XXXXXXXX2-----+---------+---------+---------
      503, +---------+---------+---------+---------+---------+----5XXXXXXXX3--2XXXXXXX31---+---------+---------

    (...)

      809, +---------+---------+---------+---------+--------8XXXXXXX8--+---------+---------+---------+---------
      810, +---------+---------+---------+---------+------5XXXXXXXXXX5-+---------+---------+---------+---------
      811, +---------+---------+---------+---------+------XXXXXXXXXXXXXXXXXXXXXXX2---------+---------+---------
      812, +---------+---------+---------+----2X4--+------9XXXXXXXXXX99XXXX1-----+---------+---------+---------
      813, +---------+---------XX9-------+---------+------4XXXXXXXXXX--+---------+---------+---------+---------
      814, +---------+----51---+---------+---------+------3XXXXXXXXXXXXXXXXXXXXX5+---------+---------+---------
      815, +---------+---------+---------+---------+--------9XXXXXXXXX3+---------+---------+---------+---------
      816, +---------+---------+---------+---------+-------8XXXXXXXXX6-+---------+---------+---------+---------
      817, +---------+---------+---------+---------+------5XXXXXXXXXXXXXXXXXXXX9-+---------+---------+---------
      818, +---------+---------+---------+---------+------9XXXXXXXXXX7-+1--------+---------+---------+---------
      819, +---------+-38------+---------+---------+-----4XXXXXXXXXXX4-+---------+---------+---------+---------
      820, +---------+---------+---------+---------+-----5XXXXXXXXXX9--+---------+---------+---------+---------
      821, +---------+---------+---------+---------+------1XXXXXXXXXX9-+---------+---------+---------+---------
      822, +---------+---------+---------+---------+-----1XXXXXXXXXXXXXXXXXXXXXXXX3--------+---------+---------
      823, +---------+---------+---------+---------+------1XXXXXXXXXXXXXXXXXXXXX1+---------+---------+---------
      824, +---------+---------+---------+---------+------1XXXXXXXXXXXXXXXXXXXXXXX2--------+---------+---------
      825, +---------+-----38--+---------+---------+------XXXXXXXXXXXXX3---------+---------+---------+---------
      826, +---------+---------+---------+---------+-----4XXXXXXXXXXXXXXXXXXXXX9-+----45---+---------+---------

    (...)

    Final Clock State

    Decoded time: 17-01-09 1 17:06:34 CET ..
    17.01.09(1,1)17:07:34 MEZ 0,0 10 p(7701-114:255) s(157-0:22) m(244-86:22) h(247-105:19) wd(237-156:11) D(244-177:9) M(235-158:10) Y(250-179:9) 127,7,7,50
    output triggered
    Clock state: synced
    Tick: 441
    confirmed_precision ?? adjustment, deviation, elapsed
    0.0000 ppm, @+ , 0.0000 ppm, -94 ticks, 73 min + 44851 ticks mod 60000

    Clock State Statistics
    0 useless : 718
    1 dirty   : 228
    2 free    : 0
    3 unlocked: 0
    4 locked  : 0
    5 synced  : 4059

    Clock State Transition Statistics
    useless  => useless : 716
    useless  => dirty   : 2
    useless  => synced  : 1
    dirty    => useless : 2
    dirty    => dirty   : 226
    synced   => synced  : 4058

    Quality Factor Statistics
    0: 717
    1: 228
    2: 528
    3: 480
    4: 480
    5: 420
    6: 540
    7: 420
    8: 419
    9: 540
    10: 233
    11: 0
    average: 4.68851  standard deviation: 3.11919

    Prediction Match Statistics
    41: 0
    42: 60
    43: 0
    44: 0
    45: 0
    46: 120
    47: 60
    48: 119
    49: 720
    50: 2911
    51: 0
    average: 49.4742  standard deviation: 1.26579

Due to the scope mode we can easily see that the signal is slightly noisy. We can also see that the signal seems to drift away. Since DCF77 will (by definition) never drift we have to assume that the receiver clock is not running at its nominal frequency. Since the DCF77 signal appears to be to fast it implies that the receiver clock is actually to slow.

Notice how the signal is not as sharp as in the synthetic case. Also notice that the edges of the signal usually are indicated by digits intead of the "-" and "X" symbols. This is because the signal edges are not aligned with the 10 ms spaced sample periods anymore.

Since the quality factor is up to 10 we know that the signal quality was overall excellent and that the clock was in perfect sync.

From the statistics we can infer that the clock library never transitioned out of sync. We can also see that there was only one minute with 8 bit errors. The rest of the time reception was much better. 

### Synthesizer Options

The synthesizer options are very straightforward. 

      -t, --time=YY.MM.DD@hh:mm:ss                 set start date and time, default: <<<TBD>>>
      -S, --summertime=[0|1]                       set summertime [1] or wintertime [0], default: 0
      -a, --abnormal_transmitter_operation=[0|1]   set abnormal_transmitter_operation flag, default: 0
      -c, --timezone_change_scheduled=[0|1]        set timezone_change_scheduled flag, default: 0
      -l, --leap_second_scheduled=[0|1]            set leap_second_scheduled flag, default: 0
      -s, --signal_length_s=n                      generate test signal data for n seconds, default: 1000

You just setup whatever start time you want and how many seconds the signal shall be generated. Notice that summertime/wintertime can usually (except for one hour each year) be inferred from the time. Also leap seconds can only be scheduled for 4 specific seconds each year. The synthesizer may ignore inconsistent flag settings.

### Manipulation and Distortion Options

There is a whole bunch of options to modify the signal. This is intended to simulate the effect of noise. For test purposes I usually start with synthesized signals. However it can also be used with recorded signals to get an idea of how much headroom a real signal has.

#### Startup Delay

The first two options are not really distorting the signal. They are for simulating the behaviour of some receivers which are flat for some period immediately after reset.

     -d, --startup_ms=n                           prepend test signal with constant value for n milliseconds, default: 0
      -u, --startup_signal=[0|1]                   value of the startup signal, default: 0

The distort already during startup is technically possible but I am not aware of any realistic scenario where this would happen. Still the option is in place just in case. It may be used to visualize the effect of the noise on a flat signal.

    -D, --distort_startup_signal=[0|1]           distortion shall already be applied in the startup phase, default: 0
    
  #### Drift  

The drift options are more interesting. They simulate a clock that deviates from its nominal frequency.

Here is an example of a abnormal large deviation of -1000 ppm (or 16 kHz below 16 Mhz).

    ./main -vs -P-1000 
    (...)
          1, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
        2, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        3, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        4, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        5, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        6, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        7, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        8, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
        9, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       10, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       11, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       12, +---------+---------+---------+---------+---------+---------+---------+---------+---------+ ---------
       13, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       14, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       15, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
       16, +5XXXXXXXXX5--------+---------+---------+---------+---------+---------+---------+---------+---------
       17, +4XXXXXXXXX6--------+---------+---------+---------+---------+---------+---------+---------+---------
       18, +3XXXXXXXXX7--------+---------+---------+---------+---------+---------+---------+---------+---------
       19, +2XXXXXXXXXXXXXXXXXXX8--------+---------+---------+---------+---------+---------+---------+---------
       20, +1XXXXXXXXX9--------+---------+---------+---------+---------+---------+---------+---------+---------
       21, +-XXXXXXXXXXXXXXXXXXXX--------+---------+---------+---------+---------+---------+---------+---------
       22, +-9XXXXXXXXX1-------+---------+---------+---------+---------+---------+---------+---------+---------
       23, +-8XXXXXXXXX2-------+---------+---------+---------+---------+---------+---------+---------+---------
       24, +-7XXXXXXXXX3-------+---------+---------+---------+---------+---------+---------+---------+---------
       25, +-6XXXXXXXXX4-------+---------+---------+---------+---------+---------+---------+---------+---------
       26, +-5XXXXXXXXX5-------+---------+---------+---------+---------+---------+---------+---------+---------
       27, +-4XXXXXXXXX6-------+---------+---------+---------+---------+---------+---------+---------+---------
       28, +-3XXXXXXXXX7-------+---------+---------+---------+---------+---------+---------+---------+---------
       29, +-2XXXXXXXXX8-------+---------+---------+---------+---------+---------+---------+---------+---------
       30, +-1XXXXXXXXX9-------+---------+---------+---------+---------+---------+---------+---------+---------
       31, +--XXXXXXXXXX-------+---------+---------+---------+---------+---------+---------+---------+---------
       32, +--9XXXXXXXXX1------+---------+---------+---------+---------+---------+---------+---------+---------

You can clearly see how the signal drifts to the right. That is the synthesized signal is to slow by 1 millisecond each second.

#### Fade

The fade option is intended to simulate signal fading. Some receivers default to all low and others to all high output during fading conditions. Thus there are parameters to control the lenght of signal fading as well as the signal default. The behaviour of
    
    -f, --fade=a,b,c,d,[0,1]

is to generate a fade condition of a random duration in [a, b] milliseconds  (end values included). The fade conditions will have gaps  [c,  d] milliseconds. During the fade duration the signal will be set to 0 or 1.

The behaviour is best understood with some examples. The first example is equivalent to no fading at all and shows the undistorted signal.

    > ./main -vs -f0,0,0,0,0 | sed -n '161,165p' 
      118, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
      119, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
      120, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
      121, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
      122, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------

The next examples exhibit fading with constant spacing.

    > ./main -vs -f20,20,30,30,0 | sed -n '161,165p' 
      118, XXX--XXX--+---------+---------+---------+---------+---------+---------+---------+---------+---------
      119, XXX--XXX--XXX--XXX--+---------+---------+---------+---------+---------+---------+---------+---------
      120, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
      121, XXX--XXX--+---------+---------+---------+---------+---------+---------+---------+---------+---------
      122, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
      
      > ./main -vs -f20,20,30,30,1 | sed -n '161,165p' 
      118, XXXXXXXXXX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX
      119, XXXXXXXXXXXXXXXXXXXX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX
      120, +--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX
      121, XXXXXXXXXX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX
      122, +--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX+--XX---XX

      > ./main -vs -f1,1,99,99,1 | sed -n '161,165p' 
      118, XXXXXXXXXX+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1
      119, XXXXXXXXXXXXXXXXXXXX+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1
      120, +--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1
      121, XXXXXXXXXX+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1
      122, +--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1+--------1

Finally some examples with random fadeing.

    > ./main -vs -f0,500,0,500,0 | sed -n '161,165p' 
      118, +--4XXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
      119, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
      120, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
      121, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
      122, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
    
    > ./main -vs -f0,500,0,500,1 | sed -n '161,165p' 
      118, XXXXXXXXXX+---------+------4XXXXXXXXXXX1+---------+---------+-4XXXXXXXXXXXXXXXXXXXXXXXXX2-+---------
      119, XXXXXXXXXXXXXXXXXXXX+---------+------3XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX-+---------+---------+---------
      120, +------6XXXXXXX2----+---------+---------+---------+----5XXXXXXXXX1----+-----2XXXXXXXXXXXXXX3--------
      121, XXXXXXXXXX+---------+---4XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX7--+---------+---------+------1XX
      122, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX9+---------+---------+-2XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

### Noise

The random noise option will add random noise to the signal. 

    -r, --random_hf_noise_per_1000=n
        
The parameter n controls the probability of seeting a signal bit to a random value.

Examples with successively more noise.
    
    > ./main -vs -r0 | sed -n '161,165p' 
      118, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
      119, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------
      120, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------
      121, XXXXXXXXXX+---------+---------+---------+---------+---------+---------+---------+---------+---------
      122, +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------

    > ./main -vs -r10 | sed -n '161,165p' 
      118, 9XXXXXXX9X+-------1-+---------+----1----+---------+-------1-+---------+---------+--------1+---------
      119, XXXXXXXXXXXXXXXXXXXX+---------+---------+---------1---------+1--1----1+--------1+-----1---+--------1
      120, +----1----+---------+1--------+---------+-1-------+---1-----+---------+---------+---------+---------
      121, 9XXX9XXXXX+----1-1--+---------1---------+------1--+---------+---1-----+----1----+--1------+---------
      122, +---------+---------+-1-------+---------+---------+---------+----1----+---------+---------+---------

    > ./main -vs -r100 | sed -n '161,165p' 
      118, XXX99XX9XX1-1--1---1111-----1-11-----2-1+-------1-+-1--1--1-2--3-23--11---------+1--1---3-2-11113---
      119, XXXXX9X9XX99X9XXX9X91--11-----+-1---12--+1--1-----1-------2-1-----21-1+1-21----1+-1--1-1--+-1-1-1--1
      120, 1-1--1111-+-1-------+1112122--+1-1--1---+2-11--1-12-1----1-1+1-1--21-1+21-111--1+1-2-1---1+----1-2--
      121, 9XX9XXXX9X+----1111-11231--3-1+---2----21---11-1-1+1-1--12-1+--13-----+-1-1111--+1--------13-1--2111
      122, +---1-21-11--12----1+-21-----1+-1-1--3--+1--1-----+--11---1-1--1--111-+--11-1-2-1--1--141-+11-1-31--

    > ./main -vs -r500 | sed -n '161,165p' 
      118, 667X8799773254412-124236423233+22342121411-41-23323253344114312322351-34--21321231325-16225334531141
      119, 88495779X999788778774145-2351413433225222221215421331113321134122212243251243631132312162-2345114-1-
      120, +251424-34442213611-432413134441121-232232243132-15323542211212432322224411122624133322423142435-3-2
      121, 7979766X8833124311162-1325332173133-133323244212-222225-334143423333-25412-435444-23432243+323311213
      122, 5231222331232222214421422311232115511515322534341542332342244414441211231113333124133416231324132132
      
Noise is always injected after fading is applied. This is exhibited by the next example.

    > ./main -vs -f999999,999999,1,1,0 -r 100 | sed -n '161,165p' 
      118, +-111--2-11-1--1---1111-----1-11-----2-1+-------1-+-1--1--1-2--3-23--11---------+1--1---3-2-11113---
      119, +11-1-2-1-12113-1--21--11-----+-1---12--+1--1-----1-------2-1-----21-1+1-21----1+-1--1-1--+-1-1-1--1
      120, 1-1--1111-+-1-------+1112122--+1-1--1---+2-11--1-12-1----1-1+1-1--21-1+21-111--1+1-2-1---1+----1-2--
      121, +23-----21+----1111-11231--3-1+---2----21---11-1-1+1-1--12-1+--13-----+-1-1111--+1--------13-1--2111
      122, +---1-21-11--12----1+-21-----1+-1-1--3--+1--1-----+--11---1-1--1--111-+--11-1-2-1--1--141-+11-1-31--


### Filter Parameter Options

The final option controls how the library will sample the data. 

    -m, -millisecond_samples=[0|1] 

For AVR Atmega micro controllers the library will collect 10 samples of 1 millisecond each and the evaluate their average. This is due to memory constraints of these controllers. For ARM based controllers it will sample each millisecond (option -m1). As a consequence the ARM based version gets better phase locks. However you can only notice this with synthesized signal data. This is because the recorded files of the debug helper provide data only in 10 millisecond aggegates.

