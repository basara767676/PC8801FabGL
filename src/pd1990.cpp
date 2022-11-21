/*
    This file is part of PC8801FabGL.

    https://github.com/Basara767676/PC8801FabGL

    Copyright (C) 2022 Basara767676

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "pd1990.h"

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#ifdef DEBUG_PC88
// #define DEBUG_PD1990
#endif

#define PD1990_REG_HOLD (0)
#define PD1990_REG_SHIFT (1)
#define PD1990_TIME_SET (2)
#define PD1990_TIME_READ (3)
#define PD1990_CD0 (4)

#define PD1990_CDI (0x10)

PD1990::PD1990() {
    mCSTB = false;
    mCCK = false;
    mShift = false;
}

PD1990::~PD1990() {}

uint8_t PD1990::read(void) { return mOutData & 0x01 ? PD1990_CDI : 0; }

#define PD1990_CSTB (0x02)
#define PD1990_CCK (0x04)

void PD1990::write(int address, uint8_t value) {
    bool cstb = value & PD1990_CSTB;

    if (address == 0x40) {
        if (mCSTB != cstb && !cstb) {
            clockSTB();
        }
        mCSTB = cstb;

        bool cck = value & PD1990_CCK;
        if (mShift) {
            if (mCCK != cck && !cck) {
                if (mDataIn) {
                    mInData |= 0x10000000000;
                }
                mInData >>= 1;
                mOutData >>= 1;
#ifdef DEBUG_PD1990
                Serial.printf("%08llx\n", mInData);
#endif
            }
        }
        mCCK = cck;

    } else if (address == 0x10) {
#ifdef DEBUG_PD1990
        Serial.printf("PD1990 Port 10: %02x\n", value & 0x01);
#endif
        mCmd = value & 0x07;
        mDataIn = value & 0x08;
    }
}

#ifdef DEBUG_PD1990
const char *PD1990::msg[8] = {"Register hold", "Register shift", "Time set & Counter hold", "Time read", "TP 64", "TP 256",
                              "TP 2048",       "Test mode"};
#endif

void PD1990::clockSTB(void) {
#ifdef DEBUG_PD1990
    Serial.printf("PD1990 %s\n", msg[mCmd]);
#endif
    switch (mCmd) {
        case PD1990_REG_HOLD:
            mShift = false;
            break;
        case PD1990_REG_SHIFT:
            mShift = true;
            break;
        case PD1990_TIME_SET:
            mShift = false;
            setDateTime();
            break;
        case PD1990_TIME_READ:
            mShift = false;
            getDateTime();
            break;
        default:
            mShift = false;
    }
}

void PD1990::getDateTime(void) {
    struct timeval timeValue;
    struct tm *now;

    gettimeofday(&timeValue, NULL);
    now = localtime(&timeValue.tv_sec);

    mOutData = ((uint64_t)(now->tm_mon + 1) << 36) | ((uint64_t)now->tm_wday << 32) | (toBCD(now->tm_mday) << 24) |
               (toBCD(now->tm_hour) << 16) | (toBCD(now->tm_min) << 8) | toBCD(now->tm_sec);
}

void PD1990::setDateTime(void) {
    struct timeval timeValue;
    struct tm now;

    memset(&timeValue, 0, sizeof(timeValue));
    memset(&now, 0, sizeof(now));

    now.tm_year = 1982 - 1900;
    now.tm_mon = ((mInData & 0xf000000000) >> 36) - 1;
    now.tm_wday = toBin((mInData & 0xf00000000) >> 32);
    now.tm_mday = toBin((mInData & 0xff000000) >> 24);
    now.tm_hour = toBin((mInData & 0xff0000) >> 16);
    now.tm_min = toBin((mInData & 0xff00) >> 8);
    now.tm_sec = toBin(mInData & 0xff);

    timeValue.tv_sec = mktime(&now);
    settimeofday(&timeValue, NULL);
}

uint8_t PD1990::toBCD(uint8_t value) { return (value / 10) * 16 + (value % 10); }
uint8_t PD1990::toBin(uint8_t value) { return (value / 16) * 10 + (value % 16); }
