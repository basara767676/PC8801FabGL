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

#include "d88.h"
#include "pc88vm.h"
#include "pd765c.h"

class PC88VM;

#define DRIVES 4

class PC80S31 {
   public:
    PC80S31();
    ~PC80S31();

    int init(PC88VM *vm, uint8_t *mem, I8255 *i8255);
    void reset(void);
    int run(void);

    static int readByte(void *context, int address);
    static void writeByte(void *context, int address, int value);

    static int readWord(void *context, int addr);
    static void writeWord(void *context, int addr, int value);

    static int readIO(void *context, int address);
    static void writeIO(void *context, int address, int value);

    int openDrive(int drive, char *fileName);
    int closeDrive(int drive);

    void eject(void);

   private:
    fabgl::Z80 *mPD780C;
    I8255 *mI8255;
    PD765C *mPD765C;

    bool mReset;
    bool mIRQ;

    uint8_t *mMem;
};
