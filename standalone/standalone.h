//
// Created by Gavin on 02/07/2015.
//

#ifndef DCF77_STANDALONE_H
#define DCF77_STANDALONE_H

#include <stdint.h>
#include <c++/iostream>

#include <fstream>
#include <stdlib.h>
#include <stdint-gcc.h>


extern int16_t TCCR2B;
extern int16_t TCCR2A;
extern int16_t OCR2A;
extern int16_t TIMSK0;
extern int TIMSK2;
extern uint8_t SREG;





#define boolean int8_t

uint8_t min(uint8_t a, uint8_t b);
//#define abs(A) ((A<0)?-A:A)

int cli();

//#define min(A,B) ((A<B)?A:B)

#define HEX 1
#define DEC 0
#define BIN 1



#define F(A) A

#define WGM22 0
#define WGM21 0
#define WGM20 0
#define CS22 0
#define OCIE2A 0

class DummySerial {
public:
    size_t print(const char[]) { };
    size_t print() { };
    size_t print(char) { };
    size_t print(unsigned char, int = DEC) { };
    size_t print(int, int = DEC) { };
    size_t print(unsigned int, int = DEC) { };
    size_t print(long, int = DEC) { };
    size_t print(unsigned long, int = DEC) { };
    size_t print(double, int = 2) { };
    size_t println() { };
    size_t println(char) { };
    size_t println(unsigned char, int = DEC) { };
    size_t println(int, int = DEC) { };
    size_t println(const char &c, int = DEC) { };
    size_t println(const char c[], int = DEC) { };
    size_t println(unsigned int, int = DEC) { };
    size_t println(long, int = DEC) { };
    size_t println(unsigned long, int = DEC) { };
    size_t println(double, int = 2) { };
};

extern DummySerial Serial;

#define CRITICAL_SECTION

#endif //DCF77_STANDALONE_H
