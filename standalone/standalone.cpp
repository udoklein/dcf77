//
// Simple Test Harness to Allow calling Decoder using canned data on a PC
//
// Reads a .csv which contains a row for each 10mS period from an Arduino captured sample
// from a live signal.  Each .csv file has a corresponding log file which gives a graphical
// representation of the sample (one line per second).  The samples are not aligned so the signal
// will appear somewhere across the second.
//

#include "standalone.h"
#include "../dcf77.h"

void output_handler(const Clock::time_t &decoded_time) {

    // Only log Minutes to reduce log output
    if (decoded_time.second.val != 0) return;

    std::cout << "OutTime: ";

    std::cout << (int) decoded_time.day.digit.hi << (int) decoded_time.day.digit.lo;
    std::cout << "/" << (int) decoded_time.month.digit.hi << (int) decoded_time.month.digit.lo;
    std::cout << "/20" << (int) decoded_time.year.digit.hi << (int) decoded_time.year.digit.lo << " ";

    std::cout << (int) decoded_time.hour.digit.hi << (int) decoded_time.hour.digit.lo;
    std::cout << ":" << (int) decoded_time.minute.digit.hi << (int) decoded_time.minute.digit.lo;
    std::cout << ":" << (int) decoded_time.second.digit.hi << (int) decoded_time.second.digit.lo;

    if (decoded_time.uses_summertime) std::cout << " Summer";

    std::cout << std::endl;
}

static uint8_t next_val;
uint8_t input_handler() {
    return next_val;
}

int main() {

    DCF77_Clock::set_output_handler(&output_handler);
    Internal::Generic_1_kHz_Generator::setup(input_handler);
#ifndef MSF60
    std::ifstream infile("standalone/capture_dcf77_20150816.csv");
#else
    std::ifstream infile("standalone/capture_msf60_20150804.csv");
#endif

    uint8_t a;
    std::string line;
    while (infile >> line) {
        a = atoi(line.c_str());
        for (int j = 0; j < 10; j++) {
            next_val = a;
            Internal::Generic_1_kHz_Generator::isr_handler();
        }
    }
    return 0;
}

// Some stub functions that might be useful when running away from Arduino

uint8_t min(uint8_t a, uint8_t b) { return a; }
int cli() { return 0; };

int16_t TCCR2B;
int16_t TCCR2A;
int16_t OCR2A;
int16_t TIMSK0;
int TIMSK2;
uint8_t SREG;

DummySerial Serial;

uint8_t reverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}