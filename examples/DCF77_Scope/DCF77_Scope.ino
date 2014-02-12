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


const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = A5;  // == D19 for standard Arduinos
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 1;

const uint8_t dcf77_monitor_pin = A4; // == D18 for standard Arduinos

const uint8_t lower_output_pin = 2;
const uint8_t upper_output_pin = 17;


void stopTimer0() {
    // ensure that the standard timer interrupts will not
    // mess with msTimer2
    TIMSK0 = 0;
}

ISR(TIMER2_COMPA_vect) {
    process_one_sample();
}

void initTimer2() {
    // Timer 2 CTC mode, prescaler 64
    TCCR2B = (1<<WGM22) | (1<<CS22);
    TCCR2A = (1<<WGM21);

    // 249 + 1 == 250 == 250 000 / 1000 =  (16 000 000 / 64) / 1000
    OCR2A = 249;

    // enable Timer 2 interrupts
    TIMSK2 = (1<<OCIE2A);
}

uint8_t sample_input_pin() {
    const uint8_t sampled_data = dcf77_inverted_samples ^
        (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200):
                                digitalRead(dcf77_sample_pin));

    digitalWrite(dcf77_monitor_pin, sampled_data);
    return sampled_data;
}

void led_display_signal(const uint8_t sample) {
    static uint8_t ticks_per_cycle = 12;
    static uint8_t rolling_pin = lower_output_pin;
    static uint8_t counter = 0;

    digitalWrite(rolling_pin, sample);

    if (counter < ticks_per_cycle) {
        ++counter;
    } else {
        rolling_pin = rolling_pin < upper_output_pin? rolling_pin + 1: lower_output_pin;
        counter = 1;
        // toggle between 12 and 13 to get 12.5 on average
        ticks_per_cycle = 25-ticks_per_cycle;
    }
}

const uint16_t samples_per_second = 1000;
const uint8_t bins                = 100;
const uint8_t samples_per_bin     = samples_per_second / bins;

volatile uint8_t gbin[bins];
boolean samples_pending = false;

void process_one_sample() {
    static uint8_t sbin[bins];

    const uint8_t sample = sample_input_pin();
    led_display_signal(sample);

    static uint16_t ticks = 999;  // first pass will init the bins    
    ++ticks;

    if (ticks == 1000) {
        ticks = 0;
        memcpy((void *)gbin, sbin, bins);
        memset(sbin, 0, bins);
        samples_pending = true;
    }

    sbin[ticks/samples_per_bin] += sample;
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    pinMode(dcf77_sample_pin, INPUT);
    digitalWrite(dcf77_sample_pin, HIGH);

    pinMode(dcf77_monitor_pin, OUTPUT);

    for (uint8_t pin = lower_output_pin; pin <= upper_output_pin; ++pin) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    initTimer2();
    stopTimer0();
}


int32_t sign(int32_t value) {
    return (value>0) - (value<0);
}

void loop() {
    static int64_t count = 0;
    uint8_t lbin[bins];

    if (samples_pending) {
        cli();
        memcpy(lbin, (void *)gbin, bins);
        samples_pending = false;
        sei();

        ++count;
        // ensure the count values will be aligned to the right
        for (int32_t val=count; val < 100000000; val *= 10) {
            Serial.print(' ');
        }
        Serial.print((int32_t)count);
        Serial.print(", ");
        for (uint8_t bin=0; bin<bins; ++bin) {
            switch (lbin[bin]) {
                case  0: Serial.print(bin%10? '-': '+'); break;
                case 10: Serial.print('X'); break;
                default: Serial.print(lbin[bin]);
            }
        }
        Serial.println();
     }
}

