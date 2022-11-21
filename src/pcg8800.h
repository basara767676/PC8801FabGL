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

#include "fabgl.h"

class PCG8800 {
   public:
    PCG8800();
    ~PCG8800();

    void init(uint8_t *fontROM, int volume);
    void reset(void);
    void initFont(void);

    void port00(uint8_t value);
    void port01(uint8_t value);
    void port02(uint8_t value);
    void port03(uint8_t value);
    void port0c(uint8_t value);
    void port0d(uint8_t value);
    void port0e(uint8_t value);
    void port0f(uint8_t value);

    void suspend(bool value);
    void beep(bool value);
    void pcg(void);
    void pcgON(void);

    void setVolume(int value);
    void soundMute(void);

    void volumeUp(void);
    void volumeDown(void);

   private:
    uint8_t *mFontROM80;
    uint8_t *mFontROM40;

    uint8_t *mFont80;
    uint8_t *mFont40;

    uint8_t *mFontROM80PCG;
    uint8_t *mFontROM40PCG;

    uint8_t *mFont80PCG0;
    uint8_t *mFont40PCG0;

    uint8_t *mFont80PCG1;
    uint8_t *mFont40PCG1;

    uint8_t *mFont80PCGHigh;
    uint8_t *mFont40PCGHigh;

    uint8_t *mFont80PCGLow;
    uint8_t *mFont40PCGLow;

    bool mHighCode;
    bool mLowCode;

    int mHighCodePCG;
    int mLowCodePCG;

    int mPCGAddr;
    uint8_t mPCGData;

    bool mCounter[3];

    bool mBit4;
    bool mBit5;

    bool mBeep;

    uint8_t mPort03;

    fabgl::SoundGenerator mSoundGenerator;
    fabgl::WaveformGenerator *mSquareWaveformGenerator[4];
    bool mStatus[3];
    uint8_t mI8253Mode[3];
    uint16_t mI8253Counter[3];

    bool mBeepMute = false;

    static const uint8_t mVolume[16];
    int mVolumeValue;
    void enable(int value, bool status);
    void setCounter(int counter, int value);
    void setFrequency(int counter);
};