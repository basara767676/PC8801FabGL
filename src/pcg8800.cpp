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

#include "pcg8800.h"

#include <Arduino.h>

#include <cstring>

#ifdef DEBUG_PC88
// #define DEBUG_PCG8800
#endif

#define I8253_MODE_COUNTER_LATCH 0
#define I8253_MODE_LO_BYTE 1
#define I8253_MODE_HI_BYTE 2
#define I8253_MODE_LO_HI_BYTES 3
#define I8253_MODE_LO_HI_BYTES2 4

PCG8800::PCG8800() {
    mPCGAddr = 0;
    mPCGData = 0;

    for (int i = 0; i < 3; i++) {
        mCounter[i] = false;
        mStatus[i] = false;
        mI8253Mode[i] = I8253_MODE_COUNTER_LATCH;
        mI8253Counter[i] = 0;
    }

    mBit4 = false;
    mBit5 = false;

    mBeep = false;
}

PCG8800::~PCG8800() {}

const uint8_t PCG8800::mVolume[16] = {0, 8, 17, 25, 34, 42, 51, 59, 68, 76, 85, 93, 102, 110, 119, 127};

void PCG8800::init(uint8_t *fontROM, int volume) {
    mPort03 = 0;

    mHighCode = false;
    mLowCode = false;

    mHighCodePCG = 0;
    mLowCodePCG = 0;

    mFontROM80 = fontROM;
    mFontROM40 = fontROM + 0x1400;

    mFont80 = (uint8_t *)ps_malloc((256 * 10 + 256 * 2 * 10) * 3);
    mFont40 = mFont80 + 0xa00;

    mFont80PCG0 = mFont80 + 0x1e00;
    mFont40PCG0 = mFont80PCG0 + 0xa00;

    mFont80PCG1 = mFont80 + 0x1e00;
    mFont40PCG1 = mFont80PCG1 + 0xa00;

    mFontROM80PCG = mFont80PCG0;
    mFontROM40PCG = mFont40PCG0;

    mFont80PCGHigh = mFont80PCG0;
    mFont40PCGHigh = mFont40PCG0;

    mFont80PCGLow = mFont80PCG0;
    mFont40PCGLow = mFont40PCG0;

    initFont();

    setVolume(volume);
    mSoundGenerator.play(true);

    for (int i = 0; i < 3; i++) {
        mSquareWaveformGenerator[i] = new fabgl::SquareWaveformGenerator();
        mSoundGenerator.attach(mSquareWaveformGenerator[i]);
        enable(i, false);
        mCounter[i] = false;
        mStatus[i] = false;
        mI8253Mode[i] = I8253_MODE_COUNTER_LATCH;
        mI8253Counter[i] = 0;
    }

    // Beep
    mSquareWaveformGenerator[3] = new SquareWaveformGenerator();
    mSoundGenerator.attach(mSquareWaveformGenerator[3]);
    mSquareWaveformGenerator[3]->setFrequency(2400);
    mSquareWaveformGenerator[3]->enable(false);
}

void PCG8800::initFont(void) {
    memcpy(mFont80, mFontROM80, 256 * 10);
    memcpy(mFont40, mFontROM40, 256 * 2 * 10);
    memcpy(mFont80PCG0, mFont80, 256 * 10 + 256 * 2 * 10);
    memcpy(mFont80PCG1, mFont80, 256 * 10 + 256 * 2 * 10);
}

void PCG8800::reset(void) {
    for (int i = 0; i < 4; i++) {
        mSquareWaveformGenerator[i]->enable(false);
    }
    mBeepMute = false;
}

void PCG8800::setVolume(int value) {
#ifdef DEBUG_PCG8800
    Serial.printf("PCG8800 Volume: %d\n", value);
#endif
    mBeepMute = false;

    if (0 <= value && value <= 15) {
        mSoundGenerator.setVolume(mVolume[value]);
        mVolumeValue = value;
    }
}

void PCG8800::volumeUp() { setVolume(mVolumeValue + 1); }

void PCG8800::volumeDown() { setVolume(mVolumeValue - 1); }

void PCG8800::enable(int value, bool status) {
    mStatus[value] = status;
    mSquareWaveformGenerator[value]->enable(status);
}

void PCG8800::soundMute() {
    mBeepMute = !mBeepMute;
    if (mBeepMute) {
        mSoundGenerator.setVolume(0);
    } else {
        mSoundGenerator.setVolume(mVolume[mVolumeValue]);
    }
}

void PCG8800::port00(uint8_t value) { mPCGData = value; }
void PCG8800::port01(uint8_t value) { mPCGAddr = (mPCGAddr & 0xff00) | value; }
void PCG8800::port02(uint8_t value) {
    static uint8_t fontConv[16] = {0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f, 0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff};

    mPCGAddr = (mPCGAddr & 0x00ff | (value & 0x07) << 8) ^ 0x400;

    bool curBit4 = value & 0x10;
    bool curBit5 = value & 0x20;

    if (mBit4 && !curBit4) {
        if (mBit5 && !curBit5) {
            auto address = mPCGAddr;
            auto offset = (address / 8) * 10 + address % 8;
            *(mFontROM80PCG + offset) = *(mFontROM80 + offset);
            offset = (address / 8) * 20 + address % 8;
            *(mFontROM40PCG + offset) = *(mFontROM40 + offset);
            *(mFontROM40PCG + offset + 10) = *(mFontROM40 + offset + 10);
        } else {
            auto address = mPCGAddr;
            auto offset = (address / 8) * 10 + address % 8;
            *(mFontROM80PCG + offset) = mPCGData;
            offset = (address / 8) * 20 + address % 8;
            *(mFontROM40PCG + offset) = fontConv[(mPCGData & 0xf0) >> 4];
            *(mFontROM40PCG + offset + 10) = fontConv[(mPCGData & 0x0f)];
            if ((address < 128 * 8 && mLowCode) || (address >= 128 * 8 && mHighCode)) {
                auto offset = (address / 8) * 10 + address % 8;
                *(mFontROM80 + offset) = mPCGData;
                offset = (address / 8) * 20 + address % 8;
                *(mFontROM40 + offset) = fontConv[(mPCGData & 0xf0) >> 4];
                *(mFontROM40 + offset + 10) = fontConv[(mPCGData & 0x0f)];
            }
        }
    }
    mBit4 = curBit4;
    mBit5 = curBit5;

    // PCG Sound enable/disable
    mCounter[0] = value & 0x08;
    mCounter[1] = value & 0x40;
    mCounter[2] = value & 0x80;

    for (int i = 0; i < 3; i++) {
        if (mCounter[i] != mStatus[i]) {
            enable(i, mCounter[i]);
        }
    }
}

void PCG8800::port03(uint8_t value) {
#ifdef DEBUG_PCG8800
    Serial.printf("PCG8800 %02x %02x\n", value, mPort03);
#endif

    if ((mPort03 & 0x10) != (value & 0x10)) {
        if (value & 0x10) {  // RAM-1
            mFontROM80PCG = mFont80PCG1;
            mFontROM40PCG = mFont40PCG1;
        } else {  // RAM-0
            mFontROM80PCG = mFont80PCG0;
            mFontROM40PCG = mFont40PCG0;
        }
    }

    if ((mPort03 & 0x08) != (value & 0x08)) {  // High code
        if (value & 0x08) {                    // RAM
            memcpy(mFontROM80 + 0x500, mFont80PCGHigh + 0x500, 0x500);
            memcpy(mFontROM40 + 0xa00, mFont40PCGHigh + 0xa00, 0xa00);
            mHighCode = true;
#ifdef DEBUG_PCG8800
            Serial.println("PCG8800 High code: true");
#endif
        } else {  // CG ROM
            memcpy(mFontROM80 + 0x500, mFont80 + 0x500, 0x500);
            memcpy(mFontROM40 + 0xa00, mFont40 + 0xa00, 0xa00);
#ifdef DEBUG_PCG8800
            Serial.println("PCG8800 High code: false");
#endif
            mHighCode = false;
        }
    }

    if ((mPort03 & 0x04) != (value & 0x04)) {  // High code RAM
        if (value & 0x04) {
            mFont80PCGHigh = mFont80PCG1;
            mFont40PCGHigh = mFont40PCG1;
        } else {
            mFont80PCGHigh = mFont80PCG0;
            mFont40PCGHigh = mFont40PCG0;
        }
        if (mHighCode) {
            memcpy(mFontROM80 + 0x500, mFont80PCGHigh + 0x500, 0x500);
            memcpy(mFontROM40 + 0xa00, mFont40PCGHigh + 0xa00, 0xa00);
        }
    }

    if ((mPort03 & 0x02) != (value & 0x02)) {  // Low code
        if (value & 0x02) {                    // RAM
            memcpy(mFontROM80, mFont80PCGLow, 0x500);
            memcpy(mFontROM40, mFont40PCGLow, 0xa00);
            mLowCode = true;
        } else {  // CG ROM
            memcpy(mFontROM80, mFont80, 0x500);
            memcpy(mFontROM40, mFont40, 0xa00);
            mLowCode = false;
        }
    }

    if ((mPort03 & 0x01) != (value & 0x01)) {  // Low code RAM
        if (value & 0x01) {
            mFont80PCGLow = mFont80PCG1;
            mFont40PCGLow = mFont40PCG1;
        } else {
            mFont80PCGLow = mFont80PCG0;
            mFont40PCGLow = mFont40PCG0;
        }
        if (mLowCode) {
            memcpy(mFontROM80 + 0x500, mFont80PCGLow + 0x500, 0x500);
            memcpy(mFontROM40 + 0xa00, mFont40PCGLow + 0xa00, 0xa00);
        }
    }

    mPort03 = value;
}

void PCG8800::pcg(void) {
    uint8_t value = mPort03;

    if (value & 0x08) {
        value &= 0xf5;
    } else {
        value &= 0xf5;
        value |= 0x08;
    }
    port03(value);
}

void PCG8800::pcgON(void) { port03(0x08); }

void PCG8800::port0c(uint8_t value) { setCounter(0, value); }

void PCG8800::port0d(uint8_t value) { setCounter(1, value); }

void PCG8800::port0e(uint8_t value) { setCounter(2, value); }

void PCG8800::port0f(uint8_t value) {
    int counter = (value & 0xc0) >> 6;
    int data = (value & 0x30) >> 4;

    if (counter < 3) {
        switch (data) {
            case 0:
                mI8253Mode[counter] = I8253_MODE_COUNTER_LATCH;
                break;
            case 1:
                mI8253Mode[counter] = I8253_MODE_LO_BYTE;
                break;
            case 2:
                mI8253Mode[counter] = I8253_MODE_HI_BYTE;
                break;
            case 3:
                mI8253Mode[counter] = I8253_MODE_LO_HI_BYTES;
                break;
        }
    }
}

void PCG8800::setCounter(int counter, int value) {
    value &= 0xff;
    switch (mI8253Mode[counter]) {
        case I8253_MODE_LO_BYTE:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0xff00) | value;
            setFrequency(counter);
            break;
        case I8253_MODE_HI_BYTE:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0x00ff) | (value << 8);
            setFrequency(counter);
            break;
        case I8253_MODE_LO_HI_BYTES:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0xff00) | value;
            mI8253Mode[counter] = I8253_MODE_LO_HI_BYTES2;
            break;
        case I8253_MODE_LO_HI_BYTES2:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0x00ff) | (value << 8);
            mI8253Mode[counter] = I8253_MODE_LO_HI_BYTES;
            setFrequency(counter);
            break;
    }
}

void PCG8800::setFrequency(int counter) {
    int freq = 0;
    if (mI8253Counter[counter] > 0) {
        freq = 4000000 / (int)mI8253Counter[counter];
    }
    if (freq > 15000) {
        freq = 15000;
    }
    mSquareWaveformGenerator[counter]->setFrequency(freq);
}

void PCG8800::suspend(bool value) {
    if (value) {
        mSoundGenerator.play(false);
    } else {
        mSoundGenerator.play(true);
    }
}

void PCG8800::beep(bool value) {
    if (mBeep != value) {
        mSquareWaveformGenerator[3]->enable(value);
        mBeep = value;
    };
}