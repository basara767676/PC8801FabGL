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

#include <Arduino.h>
#include <sys/stat.h>

#include "pc88vm.h"

#ifdef DEBUG_PC88
// #define DEBUG_DR320
#endif

#define PD8251_STATUS_DSR (0x80)
#define PD8251_STATUS_FE (0x20)
#define PD8251_STATUS_OE (0x10)
#define PD8251_STATUS_PE (0x08)
#define PD8251_STATUS_TXE (0x04)
#define PD8251_STATUS_RXRDY (0x02)
#define PD8251_STATUS_TXRDY (0x01)

DR320::DR320() {
    mTape = nullptr;
    mMode = true;
    mCmtEnable = false;
    mMTON = false;
    mHighBps = false;
    mCDS = false;
    mInterruptEnable = false;
}
DR320::~DR320() {}

void DR320::init(QueueHandle_t* xQueue) { mXQueue = xQueue; }

void DR320::systemControl(uint8_t value) {
#ifdef DEBUG_DR320
    Serial.printf("DR320 - BS: %s, MTON: %s CDS: %s\n", value & 0x20 ? "-" : (value & 0x10 ? "1200bps" : "600bps"),
                  value & 0x08 ? "ON" : "OFF", value & 0x04 ? "Mark" : "Space");
#endif
    mCmtEnable = !(value & 0x20);
    mHighBps = value & 0x10;
    mMTON = value & 0x08;
    mCDS = value & 0x04;

    if (!mMTON) {
        if (mTape && mWrite) {
            fflush(mTape);
        }
    }
}

void DR320::interruptMask(bool value) {
#ifdef DEBUG_DR320
    Serial.printf("DR320 - Interrupt %s\n", mInterruptEnable ? "enable" : "disable");
#endif
    mInterruptEnable = value;
}

int DR320::open(const char* fileName) {
#ifdef DEBUG_DR320
    Serial.println(fileName);
#endif

    close();

    if (fileName == nullptr || strlen(fileName) == 0) {
        return -1;
    }

    struct stat fileStat;
    if (stat(fileName, &fileStat) == -1) {
        return -1;
    }

    mTape = fopen(fileName, "rb+");
    if (!mTape) {
#ifdef DEBUG_DR320
        Serial.printf("Open error: %s\n", fileName);
#endif
        return -1;
    }
#ifdef DEBUG_DR320
    Serial.printf("Open: %s\n", fileName);
#endif

    fseek(mTape, 0, SEEK_SET);

    mStatus = 0;

    mInit = false;

    return 0;
}

int DR320::close(void) {
    if (mTape) {
        fclose(mTape);
        mTape = nullptr;
    }
    return 0;
}

uint8_t DR320::readData(void) {  // Port 20;
    uint8_t buf = 0xff;

    if (mTape) {
        if (mInit) {
            mInit = false;
            return 0x3a;
        }
        auto s = fread(&buf, 1, 1, mTape);
        if (s != 1) {
            mStatus &= ~PD8251_STATUS_RXRDY;
        }
    }

#ifdef DEBUG_DR320
    Serial.printf("%02x ", buf);
#endif
    return buf;
}

void DR320::writeData(uint8_t value)  // Port 20
{
    mWrite = true;
    fwrite(&value, 1, 1, mTape);
#ifdef DEBUG_DR320
    Serial.printf("%02x ", value);
#endif
}

uint8_t DR320::readStatus(void) {  // Port 21;
#ifdef DEBUG_DR320
    Serial.printf("readStatus ");
#endif
    return mStatus;
}

void DR320::modeCommand(uint8_t value) {  // Port 21
    if (mMode) {
#ifdef DEBUG_DR320
        Serial.printf("PD8251 mode: %02x\n", value);
#endif
        mMode = !mMode;
    } else {
#ifdef DEBUG_DR320
        Serial.printf("PD8251 cmd:  %02x\n", value);
#endif
        if (value & 0x40) {  // IR
            mMode = true;
        }
        if (value & 0x04) {  // RxE
            mStatus |= PD8251_STATUS_RXRDY;
            mInit = true;
        } else {
            mStatus &= ~PD8251_STATUS_RXRDY;
        }
        if (value & 0x01) {  // TxEN
            mStatus |= PD8251_STATUS_TXRDY;
        } else {
            mStatus &= ~PD8251_STATUS_TXRDY;
        }
    }
}

void DR320::interrupt(void) {
    if (mInterruptEnable & mMTON) {
        cpu_cmd_t msg;
        msg.cmd = INT_RXRDY;
        xQueueSend(*mXQueue, &msg, 0);
    }
}

void DR320::rewind(void) {
    if (mTape) {
        fseek(mTape, 0, SEEK_SET);
#ifdef DEBUG_DR320
        Serial.println("DR320 -- rewind");
#endif
    }
}

void DR320::eot(void) {
    if (mTape) {
        fseek(mTape, 0, SEEK_END);
#ifdef DEBUG_DR320
        Serial.println("DR320 -- EOT");
#endif
    }
}