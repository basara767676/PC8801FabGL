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

#include "i8255.h"

#include <Arduino.h>

#ifdef DEBUG_PC88
// #define DEBUG_I8255
// #define DEBUG_PC80S31_CMD
#endif

#define I8255_IN 0
#define I8255_OUT 1

#define I8255_MODE_0 0
#define I8255_MODE_1 1
#define I8255_MODE_2 2

static const char *command[31] = {"Initialize",
                                  "Write Data",
                                  "Read Data",
                                  "Send Data",
                                  "Copy",
                                  "Format",
                                  "Send result status",
                                  "Send dirve status",
                                  "Test memory",
                                  "Send memory",
                                  "Port output",
                                  "Send memory",
                                  "Write memory",
                                  "Exec",
                                  "Load data",
                                  "Save data",
                                  "Load and go",
                                  "Write disk",
                                  "Send data (High speed)",
                                  "Error status",
                                  "Device status",
                                  "Send memory (High speed)",
                                  "Receive memory",
                                  "Change mode",
                                  "Send mode data",
                                  "Read after write mode set",
                                  "Read after write mode reset",
                                  "Restart",
                                  "Set break point",
                                  "Set register",
                                  "Read register"};

I8255::I8255() {
    mPortA = 0;
    mPortB = 0;
    mPortC = 0;

    mCmd = 0;

    mPortAMode;
    mPortBMode;
    mPortCLowerMode;
    mPortCUpperMode;

    mGroupAMode;
    mGroupBMode;

    mI8255 = nullptr;

    mID = 0;

    mATN = false;

    memset(&mCallBack, 0, sizeof(i8255_callback_t));
}

I8255::~I8255() {}

static bool sendStatusCmd = false;

uint8_t I8255::in(int port) {
    switch (port) {
        case I8255_PORT_A:
            if (mPortAMode == I8255_IN && mCallBack.portA) {
                mPortA = (*mCallBack.portA)(mI8255);
                if (mID == I8255_PC8801 && sendStatusCmd) {
                    sendStatusCmd = false;
#ifdef DEBUG_I8255
                    Serial.printf("Command: 06 Send result status: %02x\n", mPortA);
#endif
                }
                return mPortA;
            }
            break;
        case I8255_PORT_B:
            if (mPortBMode == I8255_IN && mCallBack.portB) {
                mPortB = (*mCallBack.portB)(mI8255);
                return mPortB;
            }
            break;
        case I8255_PORT_C:
            if ((mPortCLowerMode == I8255_IN || mPortCUpperMode == I8255_IN) && mCallBack.portC) {
                auto value = (*mCallBack.portC)(mI8255);
                if (mPortCLowerMode == I8255_IN) {
                    mPortC = (mPortC & 0xf0) | ((value & 0xf0) >> 4);
                }
                if (mPortCUpperMode == I8255_IN) {
                    mPortC = (mPortC & 0x0f) | ((value & 0x0f) << 4);
                }
                if (mID == I8255_PC80S31) {
#ifdef DEBUG_I8255
                    Serial.printf("%s 8255 port C: %02x\n", getID(), value);
#endif
                }
                return mPortC;
            }
            break;
    }

    return 0xff;
}

void I8255::out(int port, uint8_t value) {
    static bool readCmd = false;
    static uint8_t param[10];
    static int offset;

    switch (port) {
        case I8255_PORT_A:
            if (mPortAMode == I8255_OUT) {
                mPortA = value;
            }
            break;
        case I8255_PORT_B:
            if (mPortBMode == I8255_OUT) {
                mPortB = value;
            }
#ifdef DEBUG_PC80S31_CMD
            if (mATN) {
                if (value <= 30) {
                    if (value != 0x06) {
                        printf("Command: %02x %s\n", value, command[value]);
                    }
                    if (value == 0x02) {
                        readCmd = true;
                        offset = 0;
                    } else if (value == 0x06) {
                        sendStatusCmd = true;
                    }
                } else {
                    printf("Command: %02x\n", value);
                }
                mATN = false;
            } else if (readCmd) {
                param[offset] = value;
                offset++;
                if (offset > 3) {
                    printf("Read command: %02x %02x %02x %02x\n", param[0], param[1], param[2], param[3]);
                    readCmd = false;
                }
            }
#endif
            break;
        case I8255_PORT_C:
            if (mPortCLowerMode == I8255_OUT) {
                mPortC = (mPortC & 0xf0) | (value & 0x0f);
            }
            if (mPortCUpperMode == I8255_OUT) {
                mPortC = (mPortC & 0x0f) | (value & 0xf0);
            }
            break;
        case I8255_PORT_CONTROL:
            control(value);
            break;
    }
}

void I8255::control(uint8_t value) {
    mCmd = value;
    if (mCmd & 0x80) {
        mGroupAMode = (mCmd & 0x40) ? I8255_MODE_2 : (mCmd & 0x20) ? I8255_MODE_1 : I8255_MODE_0;
        if (mGroupAMode != I8255_MODE_0) {
#ifdef DEBUG_I8255
            Serial.printf("Not support mode %d\n", mGroupAMode);
#endif
            mGroupAMode = I8255_MODE_0;
        }
        mPortAMode = mCmd & 0x10 ? I8255_IN : I8255_OUT;
        mPortCUpperMode = mCmd & 0x08 ? I8255_IN : I8255_OUT;

        mGroupBMode = (mCmd & 0x04) ? I8255_MODE_1 : I8255_MODE_0;
        mPortBMode = mCmd & 0x02 ? I8255_IN : I8255_OUT;
        mPortCLowerMode = mCmd & 0x01 ? I8255_IN : I8255_OUT;
#ifdef DEBUG_I8255
        Serial.printf("%s 8255 control: %02x\n", getID(), value);
#endif
    } else {
        uint8_t bit = 0x01 << ((mCmd & 0x0e) >> 1);
        if (mCmd & 0x01) {  // reset
            mPortC |= bit;
        } else {  // set
            mPortC &= ~bit;
        }

        if (mPortC & 0x80) mATN = true;
#ifdef DEBUG_I8255
        if (mID == I8255_PC80S31) {
            Serial.printf("%s 8255 control: %02x Port C: %02x %s %s %s %s\n", getID(), value, mPortC, (mPortC & 0x80) ? "ATN " : "---",
                          (mPortC & 0x40) ? "DAC " : "---", (mPortC & 0x20) ? "RFD " : "---", (mPortC & 0x10) ? "DAV " : "---");
        }
#endif
    }
}

uint8_t I8255::portA(I8255 *i8255) { return i8255->mPortA; }
uint8_t I8255::portB(I8255 *i8255) { return i8255->mPortB; }
uint8_t I8255::portC(I8255 *i8255) { return i8255->mPortC; }

void I8255::setCallBack(i8255_callback_t *callBack, I8255 *i8255) {
    memcpy(&mCallBack, callBack, sizeof(i8255_callback_t));
    mI8255 = i8255;
}

const char *I8255::getID() {
    switch (mID) {
        case I8255_PC8801:
            return "8801";
            break;
        case I8255_PC80S31:
            return "80S31";
            break;
    }
    return "unknown";
}
