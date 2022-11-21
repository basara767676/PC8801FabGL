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

class PC88VM;

#define BLACK 0
#define BLUE 1
#define RED 2
#define MAGENTA 3
#define GREEN 4
#define CYAN 5
#define YELLOW 6
#define WHITE 7

#define CRTC_OCW1_ICW 0
#define CRTC_OCW2 0x20
#define CRTC_OCW3 0x40
#define CRTC_OCW4 0x60
#define CRTC_OCW5 0x80
#define CRTC_OCW6 0xa0
#define CRTC_OCW7 0xa0

#define GBANK0_BLUE 0
#define GBANK1_RED 1
#define GBANK2_GREEN 2
#define GBANK_MAIN 3
#define GBANK_UNUSED 4

union union_8_32_t {
    uint32_t uint32;
    struct {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
    } u;
};

class PD3301 {
   public:
    PD3301();
    ~PD3301();

    int init(uint8_t *vrtc);
    void setMemory(uint8_t *ramPtr, uint8_t *fontPtr);
    void setCache(uint8_t *gVramCache200, uint8_t *gVramCache400);

    void reset(void);
    void setVRAM(int vram);
    void setDMA(bool status);
    void end(void);
    void run(void);
    bool VSync(void);
    void setColorPalette(int color, int palette);
    void displayMode(uint8_t value, bool highResolution);

    void crtcCmd(uint8_t value);
    void crtcData(uint8_t value);
    uint8_t inPort50(void);
    uint8_t inPort51(void);
    void outPort52(uint8_t value);
    void outPort53(uint8_t value);
    int getVRAM(void);

    void setCloumn80(bool value);
    void setPCG(bool value);
    void suspend(bool value);

    bool updateVRAMcahce(void);

    void initColorPalette();

    fabgl::VGADirectController *getDisplayController(void) { return &mDisplayController; }

   private:
    uint8_t mCRTCCmd;
    uint8_t mCRTCData[5];
    int mCRTCDataCount;

    bool mDisplay;
    bool mTextOn;
    bool mDMAStart;
    bool mCursorDisplay;

    bool mHColor;

    uint32_t mFrameCounter;

    uint8_t *mRamPtr;
    uint8_t *mFontPtr;

    uint8_t *mGvramCache200;
    uint8_t *mGvramCache400;

    bool mTEXTEnable;

    uint8_t mPort52;
    uint8_t mPort53;

    int mVRAM;
    uint8_t *mRAM;

    int mCursorX;
    int mCursorY;

    uint8_t mColor[8];
    uint8_t mColorPalette[8];
    uint8_t mColorPaletteSave[8];

    uint16_t *mColorPalette16;

    bool mColorMode;
    bool mColumn80;
    int mCursorMask;
    bool mLine25;
    int mCharRows;

    uint8_t *mVramCol;     // 20
    uint8_t *mVramAttr;    // 20;
    uint32_t *mVramCache;  // 80*25;

    uint8_t *mVRTC;

    bool mPCG;
    volatile bool mUpdateVRAM;

    bool m200Line;
    bool mHighResolution;
    bool mReverse;

    uint32_t mGvramMask;

    uint64_t *mFont64;
    uint64_t *mColor64;

    fabgl::VGADirectController mDisplayController;

    static void drawScanline(void *arg, uint8_t *dest, int scanLine);
    uint8_t RGB_COLOR222(uint8_t r, uint8_t g, uint8_t b);
    void setGvramMask(void);
    void initColorPalette16();
    void setColorPalette16(int color);
};