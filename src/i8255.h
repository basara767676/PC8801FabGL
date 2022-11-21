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

#pragma once

#pragma GCC optimize("O2")

#include <cstdint>

#define I8255_PORT_A 0
#define I8255_PORT_B 1
#define I8255_PORT_C 2
#define I8255_PORT_CONTROL 3

#define I8255_PC8801 1
#define I8255_PC80S31 2

class I8255;

typedef struct {
    uint8_t (*portA)(I8255 *i8255);
    uint8_t (*portB)(I8255 *i8255);
    uint8_t (*portC)(I8255 *i8255);
} i8255_callback_t;

class I8255 {
   public:
    I8255();
    ~I8255();

    uint8_t in(int port);
    void out(int port, uint8_t value);

    static uint8_t portA(I8255 *i8255);
    static uint8_t portB(I8255 *i8255);
    static uint8_t portC(I8255 *i8255);

    void setCallBack(i8255_callback_t *callBack, I8255 *i8255);

    uint8_t mPortA;
    uint8_t mPortB;
    uint8_t mPortC;

    void init(int value) { mID = value; }

   private:
    I8255 *mI8255;
    i8255_callback_t mCallBack;

    uint8_t mCmd;

    uint8_t mPortAMode;
    uint8_t mPortBMode;
    uint8_t mPortCLowerMode;
    uint8_t mPortCUpperMode;

    uint8_t mGroupAMode;
    uint8_t mGroupBMode;

    int mID;

    bool mATN;

    void control(uint8_t value);
    const char *getID(void);
};