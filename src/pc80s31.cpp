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

#include "d88.h"
#include "pc88vm.h"

#ifdef DEBUG_PC88
// #define DEBUG_PC80S31
#endif

PC80S31::PC80S31(){};
PC80S31::~PC80S31(){};

int PC80S31::init(PC88VM *vm, uint8_t *mem, I8255 *i8255) {
    mMem = mem;

    mI8255 = new I8255;

    mI8255->init(I8255_PC80S31);

    i8255_callback_t callback;

    callback.portA = &i8255->portB;
    callback.portB = &i8255->portA;
    callback.portC = &i8255->portC;
    mI8255->setCallBack(&callback, i8255);

    callback.portA = &mI8255->portB;
    callback.portB = &mI8255->portA;
    callback.portC = &mI8255->portC;
    i8255->setCallBack(&callback, mI8255);

    mPD765C = new PD765C;
    mPD765C->setIRQFlag(&mIRQ);

    mPD780C = new fabgl::Z80;
    mPD780C->setCallbacks(this, readByte, writeByte, readWord, writeWord, readIO, writeIO);

#ifdef DEBUG_PC80S31
    Serial.println("PC-80S31 init completed");
#endif

    return 0;
}

void PC80S31::reset(void) { mReset = true; }

int IRAM_ATTR PC80S31::run(void) {
    mIRQ = false;
    mReset = false;

    mPD780C->reset();
    mPD780C->setPC(0);

    while (true) {
        if (mPD780C->getStatus() == fabgl::Z80_STATUS_HALT) {
            if (mPD780C->getIFF1() && mIRQ) {
                mPD780C->IRQ(0x00);
                mIRQ = false;
            }
        } else {
            mPD780C->step();
        }
        if (mReset) {
            mReset = false;
            mIRQ = false;
            mPD780C->reset();
            mPD780C->setPC(0);
        }
    }
}

int IRAM_ATTR PC80S31::readByte(void *context, int address) {
    if (address < 0x8000) {
        return ((PC80S31 *)context)->mMem[address];
    } else {
        return 0xff;
    }
}

void IRAM_ATTR PC80S31::writeByte(void *context, int address, int value) {
    if (0x4000 <= address && address < 0x8000) {
        ((PC80S31 *)context)->mMem[address] = value;
    }
}

int IRAM_ATTR PC80S31::readWord(void *context, int addr) { return readByte(context, addr) | (readByte(context, addr + 1) << 8); }

void IRAM_ATTR PC80S31::writeWord(void *context, int addr, int value) {
    writeByte(context, addr, value & 0xFF);
    writeByte(context, addr + 1, value >> 8);
}

int PC80S31::readIO(void *context, int address) {
    auto vm = (PC80S31 *)context;

    switch (address) {
        case 0xf8:
            return vm->mPD765C->terminalCount();
        case 0xfa:
            return vm->mPD765C->readStatusRegister();
        case 0xfb:
            return vm->mPD765C->readDataRegister();
        case 0xfc:
            return vm->mI8255->in(I8255_PORT_A);
        case 0xfd:
            return vm->mI8255->in(I8255_PORT_B);
        case 0xfe:
            return vm->mI8255->in(I8255_PORT_C);
        case 0xff:
            return vm->mI8255->in(I8255_PORT_CONTROL);
        default:
#ifdef DEBUG_PC80S31
            Serial.printf("80S31 Read value from non-implemeted port %02x\n", address);
#endif
            break;
    }

    return 0;
}

void PC80S31::writeIO(void *context, int address, int value) {
    auto vm = (PC80S31 *)context;

    switch (address) {
        case 0xf4:
            vm->mPD765C->writeF4(value);
            break;
        case 0xf7:  // Window value for VFO
            vm->mPD765C->writeF7(value);
            break;
        case 0xf8:  //
            vm->mPD765C->writeF8(value);
            break;
        case 0xfb:
            vm->mPD765C->writeDataRegister(value);
            break;
        case 0xfc:
            vm->mI8255->out(I8255_PORT_A, value & 0xff);
            break;
        case 0xfd:
            vm->mI8255->out(I8255_PORT_B, value & 0xff);
            break;
        case 0xfe:
            vm->mI8255->out(I8255_PORT_C, value & 0xff);
            break;
        case 0xff:
            vm->mI8255->out(I8255_PORT_CONTROL, value & 0xff);
            break;
        default:
#ifdef DEBUG_PC80S31
            Serial.printf("80S31 Write %02x to non-implemeted port %02x\n", value, address);
#endif
            break;
    }
}

int PC80S31::openDrive(int drive, char *fileName) { return mPD765C->openDrive(drive, fileName); }

int PC80S31::closeDrive(int drive) { return mPD765C->closeDrive(drive); }

void PC80S31::eject(void) { mPD765C->eject(); }