//
//  www.blinkenlight.net
//
//  Copyright 2017 Udo Klein
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

#ifndef DCF77_TEST_H
#define DCF77_TEST_H

const bool stop_on_first_error = false;
const bool verbose = false;

#include <iostream>
#include <string>
#include <stdio.h>

// start of definitions to ensure the DCF77 library has all the Arduino stuff it requires compile
#include <stdint.h>


#define __unit_test__ 1

typedef bool boolean;

template <typename T> constexpr const T& min (const T& a, const T& b) { return a<b? a: b; };
template <typename T> constexpr const T& max (const T& a, const T& b) { return a>b? a: b; };


#define F(X) X



const int BIN =  2;
const int OCT =  8;
const int DEC = 10;
const int HEX = 16;

class FakeSerial {
private:
    void print_formatted_not_0(unsigned long d, int f) {
        if (d < f) {
            std::cout << d;
        } else {
            print_formatted_not_0(d / f, f);
            std::cout << (d % f);
        }
    }

    void print_formatted(unsigned long d, int f) {
        if (d==0) {
            std::cout << d;
        } else {
            print_formatted_not_0(d, f);
        }
    }

public:
    void print() { };
    void print(char          d )             { std::cout << d; };
    void print(unsigned long d, int f = DEC) { if (f == DEC) { std::cout <<      d; } else { print_formatted(d, f); } };
    void print(long          d, int f = DEC) { if (f == DEC) { std::cout <<      d; } else { print((unsigned long)d, f); } };
    void print(unsigned char d, int f = DEC) { if (f == DEC) { std::cout << (int)d; } else { print((unsigned long)d, f); } };
    void print(int           d, int f = DEC) { if (f == DEC) { std::cout <<      d; } else { print((unsigned long)d, f); } };
    void print(unsigned int  d, int f = DEC) { if (f == DEC) { std::cout <<      d; } else { print((unsigned long)d, f); } };
    void print(const char   *d)              { while (*d) { std::cout << *d; ++d; } };
    void print(double        d, int f = 2)   { std::cout << d; };

    void println() { std::cout << std::endl; };
    void println(char          d)              { print(d);    println(); };
    void println(unsigned char d, int f = DEC) { print(d, f); println(); };
    void println(int           d, int f = DEC) { print(d, f); println(); };
    void println(const char   &d, int f = DEC) { print(d, f); println(); };
    void println(const char   *d)              { print(d);    println(); };
    void println(unsigned int  d, int f = DEC) { print(d, f); println(); };
    void println(long          d, int f = DEC) { print(d, f); println(); };
    void println(unsigned long d, int f = DEC) { print(d, f); println(); };
    void println(double        d, int f = 2)   { print(d, f); println(); };
};

extern FakeSerial Serial;

#include "../../dcf77.h"
// end of definitions to ensure the DCF77 library has all the Arduino stuff it requires compile

//#include <fstream>
//#include <stdlib.h>
//#include <stdint-gcc.h>


    // templates to have a print function with multiple arguments which will be
    // output in a comma separated fashion
    template <typename T>
    void print(T v) {
        std::cout << v;
    }

    template <typename T>
    void println(T v) {
        std::cout << v << "\n";
    }

    void println() { std::cout << std::endl; }


    template <typename T, typename... Args>
    void print(T first, Args... args) {
        print(first);
        print(F(", "));
        print(args...);
    }


    void handle_error(bool ok) {
        if (!ok) {
            exit(4);
        }
    };

// very simple unit test "framework"
namespace ut {
    uint32_t passed = 0;
    uint32_t failed = 0;

    void assert_internal(std::string file_name, std::string function_name, uint32_t line, std::string what, bool ok) {
        if (ok) {
            ++passed;
            if (verbose) {
                print("OK: ");
            }
        } else {
            ++failed;
            print("failed: ");
        }
        if (verbose || failed) {
            print("file: ");
            print(file_name);
            print("  function: ");
            print(function_name);
            print("  line: ");
            print(line);
            print("  assertion: ");
            println(what);
        }
    }

    void assert(std::string file_name, std::string function_name, uint32_t line, std::string what, bool ok) {
         assert_internal(file_name, function_name, line, what, ok);
         handle_error(ok);
    }

    // templates to implement the actual validation
    template <typename... Args>
    void assert(std::string file_name, std::string function_name, uint32_t line, std::string what, bool ok, Args... args) {
         assert_internal(file_name, function_name, line, what, ok);
         if (!ok) {
            print(" [");
            print(args...);
            println(']');
            println();
         }
         handle_error(ok);
    }

    // we want to have the line number automatically inserted
    // notice that macros are visible outside the namespace since
    // they are processed by the preprocessor
    #define assert(...) ut::assert(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

    void print_statistics() {
        println();
        print("passed: ");
        println(passed);

        print("failed: ");
        println(failed);
        println();
    }
}

void todo(std::string file_name, std::string function_name, uint32_t line) {
    print(file_name);
    print("  ");
    print(function_name);
    print("  ");
    print(line);
    println(": TODO");
}

#define todo(...) todo(__FILE__, __FUNCTION__, __LINE__)

#endif // DCF77_TEST_H