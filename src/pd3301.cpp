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

#include "pc88vm.h"

#ifdef DEBUG_PC88
// #define DEBUG_PD3301
#endif

#define ATTR_CHAR_GRAPH (0x8000)
#define ATTR_UNDERLINE (0x2000)
#define ATTR_UPPERLINE (0x1000)
#define ATTR_REVERSE (0x0400)
#define ATTR_BLINK (0x0200)
#define ATTR_SECRET (0x0100)

#define SCANLINES_PER_CALLBACK (16)  // 8 or 16, 32

PD3301::PD3301() : mDisplayController(false) {}
PD3301::~PD3301() {}

int PD3301::init(uint8_t *vrtc) {
    mVRTC = vrtc;

    // DisplayController.
    fabgl::BitmappedDisplayController::queueSize = 128;
    mDisplayController.begin();
    mDisplayController.setScanlinesPerCallBack(SCANLINES_PER_CALLBACK);
    mDisplayController.setDrawScanlineCallback(drawScanline, this);
    mDisplayController.setResolution(VGA_640x480_60Hz);

#ifdef DEBUG_PD3301
    Serial.println("DisplayController init completed");
#endif

    mVramCol = (uint8_t *)heap_caps_malloc(20, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    mVramAttr = (uint8_t *)heap_caps_malloc(20, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
#ifdef VRAM_CACHE_CAP_32BIT
    mVramCache = (uint32_t *)heap_caps_malloc(80 * 25 * sizeof(uint32_t), MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
#else
    mVramCache = (uint32_t *)heap_caps_malloc(80 * 25 * sizeof(uint32_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
#endif
    for (int i = 0; i < 80 * 25; i++) {
        *(mVramCache + i) = WHITE;
    }

    mColor[BLACK] = RGB_COLOR222(0, 0, 0);
    mColor[BLUE] = RGB_COLOR222(0, 0, 3);
    mColor[RED] = RGB_COLOR222(3, 0, 0);
    mColor[MAGENTA] = RGB_COLOR222(3, 0, 3);
    mColor[GREEN] = RGB_COLOR222(0, 3, 0);
    mColor[CYAN] = RGB_COLOR222(0, 3, 3);
    mColor[YELLOW] = RGB_COLOR222(3, 3, 0);
    mColor[WHITE] = RGB_COLOR222(3, 3, 3);

    mColor64 = (uint64_t *)heap_caps_malloc(8 * sizeof(uint64_t), MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);

    for (int i = 0; i < 8; i++) {
        uint64_t color64;
        memset(&color64, mColor[i], sizeof(uint64_t));
        *(mColor64 + i) = color64;
    }

    mColorPalette16 = (uint16_t *)heap_caps_malloc(8 * 8 * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

    mFont64 = (uint64_t *)heap_caps_malloc(256 * 8, MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
    auto p = mFont64;

    for (int i = 0; i < 256; i++) {
        uint64_t value = 0;

        value |= i & 0x04 ? 0xff : 0;
        value <<= 8;
        value |= i & 0x08 ? 0xff : 0;
        value <<= 8;
        value |= i & 0x01 ? 0xff : 0;
        value <<= 8;
        value |= i & 0x02 ? 0xff : 0;
        value <<= 8;
        value |= i & 0x40 ? 0xff : 0;
        value <<= 8;
        value |= i & 0x80 ? 0xff : 0;
        value <<= 8;
        value |= i & 0x10 ? 0xff : 0;
        value <<= 8;
        value |= i & 0x20 ? 0xff : 0;

        *p = value;
        p++;
    }

    mGvramMask = 0x3f3f3f3f;

    reset();

#ifdef DEBUG_PD3301
    Serial.printf("FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE %d\n", FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE);

    Serial.println("PD3301 init completed");
#endif

    return 0;
}

void PD3301::setMemory(uint8_t *ramPtr, uint8_t *fontPtr) {
    mRAM = ramPtr;

    mFontPtr = fontPtr;
}

void PD3301::setCache(uint8_t *gVramCache200, uint8_t *gVramCache400) {
    mGvramCache200 = gVramCache200;
    mGvramCache400 = gVramCache400;
}

void PD3301::reset() {
    mHColor = true;

    mDisplay = false;

    mDMAStart = false;
    mCursorDisplay = false;

    mTEXTEnable = false;

    mFrameCounter = 0;
    mTextOn = false;

    mColorMode = false;
    mColumn80 = true;
    mCursorMask = 0xff;

    mLine25 = true;
    mCharRows = 16;

    mPCG = false;
    mUpdateVRAM = false;

    m200Line = true;
    mHighResolution = false;
    mReverse = false;

    initColorPalette();
}

void PD3301::end() { mDisplayController.end(); }

void PD3301::setVRAM(int vram) {
#ifdef DEBUG_PD3301
    Serial.printf("VRAM %04x\n", vram);
#endif
    mVRAM = vram;
}

int PD3301::getVRAM(void) { return mVRAM; }

void PD3301::setDMA(bool status) {
    mDMAStart = status;
#ifdef DEBUG_PD3301
    Serial.printf("DMA start %s\n", status ? "true" : "false");
#endif
    mDisplay = mDMAStart && mTextOn;
}

// dispdrivers/vgadirectcontroller.cpp L287

bool PD3301::VSync(void) { return mDisplayController.VSync(); }

void PD3301::run(void) {
    mDisplayController.setScanlinesPerCallBack(SCANLINES_PER_CALLBACK);
    mDisplayController.setDrawScanlineCallback(drawScanline, this);
    mDisplayController.setResolution(VGA_640x480_60Hz);
    mDisplayController.run();
}

void PD3301::initColorPalette() {
    for (int i = 0; i < 8; i++) {
        mColorPalette[i] = mColor[i];
        mColorPaletteSave[i] = mColor[i];
    }

    initColorPalette16();
}

void PD3301::initColorPalette16() {
    for (int color = 0; color < 8; color++) {
        setColorPalette16(color);
    }
}

void PD3301::setColorPalette(int color, int palette) {
    if (mHColor) {
        mColorPalette[color] = mColor[palette];
        setColorPalette16(color);
    }
    mColorPaletteSave[color] = mColor[palette];
}

void PD3301::setColorPalette16(int color) {
    for (int i = 0; i < 8; i++) {
        *(mColorPalette16 + i * 8 + color) &= 0xff00;
        *(mColorPalette16 + i * 8 + color) |= mColorPalette[color];
        *(mColorPalette16 + color * 8 + i) &= 0x00ff;
        *(mColorPalette16 + color * 8 + i) |= mColorPalette[color] << 8;
    }
}

void PD3301::displayMode(uint8_t value, bool highResolution) {
    bool hColor = value & 0x10;

    if (mHColor != hColor) {
        mHColor = hColor;
        if (hColor) {
            for (int i = 0; i < 8; i++) {
                mColorPalette[i] = mColorPaletteSave[i];
            }
            initColorPalette16();
        } else {
            mColorPalette[BLACK] = RGB_COLOR222(0, 0, 0);
            for (int i = 1; i < 8; i++) {
                mColorPalette[i] = RGB_COLOR222(3, 3, 3);
            }
        }
    }

    if (value & 0x08) {
        setGvramMask();
    } else {
        mGvramMask = 0;
    }

    bool line200 = value & 0x01;
    mHighResolution = highResolution;
    if (!mHighResolution) {
        line200 = true;
    }

    if (m200Line != line200) {
        m200Line = line200;
    }
}

void PD3301::crtcCmd(uint8_t value) {
    mCRTCDataCount = 0;

    if ((value & 0xfe) == CRTC_OCW5) {  // OCw5: Load cursor position
        mCRTCCmd = CRTC_OCW5;
        mCursorDisplay = value & 0x01;
    } else if (value == CRTC_OCW1_ICW) {
        mCRTCCmd = CRTC_OCW1_ICW;  // ICW (OCW1): Stop display
        mTextOn = false;
        mDisplay = mDMAStart && mTextOn;
    } else if ((value & 0xfe) == CRTC_OCW2) {
        mCRTCCmd = CRTC_OCW2;  // OCW2: Start display
        mReverse = value & 0x01;
        mTextOn = true;
        mDisplay = mDMAStart && mTextOn;
#ifdef DEBUG_PD3301
        Serial.println("OCW2: Start display");
#endif
    } else if ((value & 0xfc) == CRTC_OCW3) {
        mCRTCCmd = CRTC_OCW3;
    } else if (value == CRTC_OCW4) {
        mCRTCCmd = CRTC_OCW4;
    } else if (value == CRTC_OCW6) {
        mCRTCCmd = CRTC_OCW6;
    } else if (value == CRTC_OCW7) {
        mCRTCCmd = CRTC_OCW7;
    } else {
        mCRTCCmd = 0xff;
    }
}

void PD3301::crtcData(uint8_t value) {
    mCRTCData[mCRTCDataCount++] = value;
    if ((mCRTCCmd == CRTC_OCW1_ICW) && (mCRTCDataCount == 5)) {
        mLine25 = (mCRTCData[1] & 0x3f) == 0x18;
        mCharRows = mLine25 ? 16 : 20;
        mColorMode = (mCRTCData[4] & 0x40);
        mCRTCCmd = 0;
        mCRTCDataCount = 0;
    } else if ((mCRTCCmd & 0xfe) == CRTC_OCW5 && (mCRTCDataCount == 2)) {
        bool disp = false;
        if (mCursorX != mCRTCData[0]) disp = true;
        if (mCursorY != mCRTCData[1]) disp = true;
        mCursorX = mCRTCData[0];
        mCursorY = mCRTCData[1];
        mCRTCCmd = 0;
        mCRTCDataCount = 0;
    }
}

uint8_t PD3301::inPort50(void) { return 0; }

uint8_t PD3301::inPort51(void) { return mCRTCCmd; }

void PD3301::outPort52(uint8_t value) { mPort52 = value; }

void PD3301::outPort53(uint8_t value) {
    mPort53 = value;

    mTEXTEnable = !(value & 0x01);

    setGvramMask();
}

void PD3301::setGvramMask(void) {
    mGvramMask = 0x3f3f3f3f;

    if (mPort53 & 0x02) {  // GVRAM0 Blue
        mGvramMask &= 0x36363636;
    }
    if (mPort53 & 0x04) {  // GVRAM0 Red
        mGvramMask &= 0x2d2d2d2d;
    }
    if (mPort53 & 0x08) {  // GVRAM0 Green
        mGvramMask &= 0x1b1b1b1b;
    }
}

void PD3301::setCloumn80(bool value) {
    mColumn80 = value;
    mCursorMask = value ? 0xff : 0xfe;
}

void PD3301::setPCG(bool value) {
#ifdef DEBUG_PD3301
    Serial.printf("PD3301:PCG %s\n", value ? "on" : "off");
#endif
    mPCG = value;
}

void PD3301::suspend(bool value) {
    if (value) {
#ifdef DEBUG_PD3301
        Serial.println("mDisplayController.end()");
#endif
        mDisplayController.end();
    } else {
#ifdef DEBUG_PD3301
        Serial.println("mDisplayController.run()");
#endif
        run();
    }
}

uint8_t PD3301::RGB_COLOR222(uint8_t r, uint8_t g, uint8_t b) { return ((b & 0x3) << 4) | ((g & 0x03) << 2) | (r & 0x03); }

#define SCREEN_BORDER 40
#define SCREEN_WIDTH 640

void IRAM_ATTR PD3301::drawScanline(void *arg, uint8_t *dest, int scanLine) {
    auto pd3301 = (PD3301 *)arg;

    auto boarderColor = pd3301->mDisplayController.createRawPixel(RGB222(0, 0, 0));
    auto hvsync = pd3301->mDisplayController.createBlankRawPixel();
    uint64_t hvsyncs64;
    memset(&hvsyncs64, hvsync, 8);
    auto color = pd3301->mColor;
    auto fontPtr = pd3301->mFontPtr;
    auto vramCache = pd3301->mVramCache;
    auto charRows = pd3301->mCharRows;
    auto gVramMask = pd3301->mGvramMask;
    auto color64 = pd3301->mColor64;
    auto font64 = pd3301->mFont64;
    auto colorPalette16 = pd3301->mColorPalette16;

    if (scanLine == 0) {
        pd3301->mFrameCounter++;
        *pd3301->mVRTC &= 0xdf;
    }

    if (pd3301->m200Line) {  // 200 Lines
        if (pd3301->mDisplay) {
            auto cursorMask = pd3301->mCursorMask;
            for (int line = scanLine; line < scanLine + SCANLINES_PER_CALLBACK; line += 2) {
                if (line < SCREEN_BORDER || line >= 400 + SCREEN_BORDER) {
                    memset(dest, boarderColor, SCREEN_WIDTH);
                    memset(dest + SCREEN_WIDTH, boarderColor, SCREEN_WIDTH);
                } else {
                    uint64_t pixels64;
                    uint8_t font;
                    union_8_32_t gColor;
                    uint32_t attr;
                    int y = line - SCREEN_BORDER;

                    uint32_t *gvram = (uint32_t *)(pd3301->mGvramCache200 + (y >> 1) * 320);

                    auto row = (y % charRows) >> 1;
                    auto upper = row == 0;
                    y /= charRows;
                    auto under = row == ((charRows >> 1) - 1);

                    bool blink = (pd3301->mFrameCounter & 0x3f) < 0x0f;
                    bool cursorOn = (pd3301->mCursorDisplay && (pd3301->mCursorY == y)) && blink;

                    auto cursorX = pd3301->mCursorX;

                    y *= 80;

                    for (int x = 0; x < SCREEN_WIDTH / 8; x++) {
                        attr = vramCache[x + y];
                        font = fontPtr[(attr >> 16) * 10 + row];
                        if (attr & ATTR_SECRET) {
                            font = 0;
                        }
                        if (attr & ATTR_REVERSE) {
                            font = ~font;
                        }
                        if ((attr & ATTR_BLINK) && blink) {
                            attr &= 0xf0;
                        }
                        if ((attr & ATTR_UPPERLINE) && upper) {
                            font = 0xff;
                        }
                        if ((attr & ATTR_UNDERLINE) && under) {
                            font = 0xff;
                        }
                        font ^= (cursorOn && (x & cursorMask) == cursorX) ? 0xff : 0;
                        if (pd3301->mReverse) {
                            font = ~font;
                        }

                        gColor.uint32 = *gvram & gVramMask;

                        pixels64 = (uint64_t)colorPalette16[gColor.u.byte2];
                        pixels64 <<= 16;
                        pixels64 |= (uint64_t)colorPalette16[gColor.u.byte3];
                        pixels64 <<= 16;
                        pixels64 |= (uint64_t)colorPalette16[gColor.u.byte0];
                        pixels64 <<= 16;
                        pixels64 |= (uint64_t)colorPalette16[gColor.u.byte1];

                        pixels64 = (color64[attr & 0x07] & font64[font]) | (pixels64 & ~font64[font]) | hvsyncs64;

                        *((uint64_t *)(dest) + x) = pixels64;
                        *((uint64_t *)(dest) + x + SCREEN_WIDTH / 8) = pixels64;

                        gvram++;
                    }
                }
                dest += SCREEN_WIDTH * 2;
            }
        } else {  // 200Lines DMA off
            auto blackColor = pd3301->mDisplayController.createRawPixel(RGB222(0, 0, 0));
            for (int line = scanLine; line < scanLine + SCANLINES_PER_CALLBACK; line += 2) {
                if (line < SCREEN_BORDER || line >= 400 + SCREEN_BORDER) {
                    memset(dest, boarderColor, SCREEN_WIDTH);
                    memset(dest + SCREEN_WIDTH, boarderColor, SCREEN_WIDTH);
                } else {
                    int y = line - SCREEN_BORDER;
                    uint64_t pixels64;
                    union_8_32_t gColor;
                    uint32_t *gvram = (uint32_t *)(pd3301->mGvramCache200 + (y / 2) * 320);

                    for (int x = 0; x < SCREEN_WIDTH / 8; x++) {
                        gColor.uint32 = *gvram & gVramMask;

                        pixels64 = (uint64_t)colorPalette16[gColor.u.byte2];
                        pixels64 <<= 16;
                        pixels64 |= (uint64_t)colorPalette16[gColor.u.byte3];
                        pixels64 <<= 16;
                        pixels64 |= (uint64_t)colorPalette16[gColor.u.byte0];
                        pixels64 <<= 16;
                        pixels64 |= (uint64_t)colorPalette16[gColor.u.byte1];

                        pixels64 |= hvsyncs64;

                        *((uint64_t *)(dest) + x) = pixels64;
                        *((uint64_t *)(dest) + x + SCREEN_WIDTH / 8) = pixels64;

                        gvram++;
                    }
                }
                dest += SCREEN_WIDTH * 2;
            }
        }
    } else {  // 400 Lines
        if (pd3301->mDisplay) {
            auto cursorMask = pd3301->mCursorMask;
            for (int line = scanLine; line < scanLine + SCANLINES_PER_CALLBACK; line += 2) {
                if (line < SCREEN_BORDER || line >= 400 + SCREEN_BORDER) {
                    memset(dest, boarderColor, SCREEN_WIDTH);
                    memset(dest + SCREEN_WIDTH, boarderColor, SCREEN_WIDTH);
                } else {
                    uint8_t font;
                    uint32_t attr;
                    uint64_t textColor64;
                    int y = line - SCREEN_BORDER;

                    auto gVramCache = pd3301->mGvramCache400 + y * 80;

                    auto row = (y % charRows) >> 1;
                    auto upper = row == 0;
                    y /= charRows;
                    auto under = row == ((charRows >> 1) - 1);

                    bool blink = (pd3301->mFrameCounter & 0x3f) < 0x0f;
                    bool cursorOn = (pd3301->mCursorDisplay && (pd3301->mCursorY == y)) && blink;

                    auto cursorX = pd3301->mCursorX;

                    y *= 80;

                    for (int x = 0; x < SCREEN_WIDTH / 8; x++) {
                        attr = vramCache[x + y];
                        font = fontPtr[(attr >> 16) * 10 + row];
                        if (attr & ATTR_SECRET) {
                            font = 0;
                        }
                        if (attr & ATTR_REVERSE) {
                            font = ~font;
                        }
                        if ((attr & ATTR_BLINK) && blink) {
                            attr &= 0xf0;
                        }
                        if ((attr & ATTR_UPPERLINE) && upper) {
                            font = 0xff;
                        }
                        if ((attr & ATTR_UNDERLINE) && under) {
                            font = 0xff;
                        }

                        font ^= (cursorOn && (x & cursorMask) == cursorX) ? 0xff : 0;

                        if (pd3301->mReverse) {
                            font = ~font;
                        }

                        textColor64 = color64[attr & 0x07];

                        *((uint64_t *)(dest) + x) = (textColor64 & font64[font | *(gVramCache + x)]) | hvsyncs64;
                        *((uint64_t *)(dest) + x + SCREEN_WIDTH / 8) = (textColor64 & font64[font | *(gVramCache + x + 80)]) | hvsyncs64;
                    }
                }
                dest += SCREEN_WIDTH * 2;
            }
        } else {  // 400 Lines DMA off
            uint64_t pixels;
            pixels = color[WHITE] << 8 | color[WHITE];
            pixels = pixels << 16 | pixels;
            pixels = pixels << 32 | pixels;
            for (int line = scanLine; line < scanLine + SCANLINES_PER_CALLBACK; line += 2) {
                if (line < SCREEN_BORDER || line >= 400 + SCREEN_BORDER) {
                    memset(dest, boarderColor, SCREEN_WIDTH);
                    memset(dest + SCREEN_WIDTH, boarderColor, SCREEN_WIDTH);
                } else {
                    int y = line - SCREEN_BORDER;
                    auto gVramCache = pd3301->mGvramCache400 + y * 80;

                    for (int x = 0; x < SCREEN_WIDTH / 8; x++) {
                        *((uint64_t *)(dest) + x) = (pixels & font64[*(gVramCache + x)]) | hvsyncs64;
                        *((uint64_t *)(dest) + x + SCREEN_WIDTH / 8) = (pixels & font64[*(gVramCache + x + 80)]) | hvsyncs64;
                    }
                }
                dest += SCREEN_WIDTH * 2;
            }
        }
    }

    if (scanLine >= 480 - SCANLINES_PER_CALLBACK) {
        *pd3301->mVRTC |= 0x20;
        pd3301->mUpdateVRAM = true;
    }
}

bool IRAM_ATTR PD3301::updateVRAMcahce(void) {
    if (!mDisplay || !mUpdateVRAM) return mDisplay;

    mUpdateVRAM = false;

    uint16_t prevAttr = WHITE;

    if (mColumn80) {
        for (int row = 0; row < 25; row++) {
            auto attrMode = false;
            auto attrPtr = mRAM + mVRAM + 120 * row + 80;
            memset(mVramCol, 0x80, 20);
            memset(mVramAttr, 0, 20);
            int j = 0;
            mVramCol[j] = *attrPtr;
            if (*attrPtr >= 0x80) attrMode = true;
            attrPtr++;
            mVramAttr[j] = *attrPtr;
            attrPtr++;
            j++;
            for (int i = 1; i < 20; i++) {
                auto col = *attrPtr;
                attrPtr++;
                auto value = *attrPtr;
                attrPtr++;
                if (col >= 0x80) attrMode = true;
                if (col > mVramCol[j - 1]) {
                    mVramCol[j] = col;
                    mVramAttr[j] = value;
                    j++;
                } else if (col < mVramCol[j - 1]) {
                    for (int k = 0; k < j - 1; k++) {
                        if (mVramCol[k] == col) {
                            break;
                        } else if (mVramCol[k] > col) {
                            for (int l = 19; l >= k; l--) {
                                mVramCol[l + 1] = mVramCol[l];
                                mVramAttr[l + 1] = mVramAttr[l];
                            }
                            mVramCol[k] = col;
                            mVramAttr[k] = value;
                            j++;
                            break;
                        }
                    }
                }
            }

            uint16_t attr;
            uint8_t vramAttr;
            int curCol, col, graphic;
            auto cache = mVramCache + row * 80;
            auto vram = mVRAM + row * 120;

            col = 0;
            curCol = 0;

            if (mColorMode) {  // Color
                bool mode = mVramCol[0] != 0;
                for (int i = 0; i < 20; i++) {
                    col = mVramCol[i];
                    vramAttr = mVramAttr[i];
                    attr = prevAttr & 0xffff;
                    if (vramAttr & 0x08) {
                        attr &= 0x7f00;
                        if (vramAttr & 0x10) {  // Character/Graphic for color mode
                            attr |= ATTR_CHAR_GRAPH;
                        }
                        attr |= vramAttr >> 5;  // color
                    } else {
                        attr &= 0x80ff;
                        attr |= (vramAttr & 0x37) << 8;
                    }

                    if (attrMode) {  // N88
                        prevAttr = attr;
                    }
                    if (col > 0x50) col = 0x50;
                    for (int i = curCol; i < col; i++) {
                        int offset = mRAM[vram + i];
                        if (prevAttr & ATTR_CHAR_GRAPH) {
                            offset += 0x100;
                        }
                        *(cache + i) = prevAttr | (offset << 16);
                    }

                    curCol = col;
                    if (col >= 0x50) {
                        break;
                    }
                    prevAttr = attr;
                }
            } else {  // Mono
                for (int i = 0; i < 20; i++) {
                    col = mVramCol[i];
                    vramAttr = mVramAttr[i];
                    if (vramAttr & 0x08) {
                        attr = prevAttr;
                    } else {
                        attr = WHITE | (vramAttr << 8);
                    }

                    if (attrMode) {  // N88
                        prevAttr = attr;
                    }
                    if (col > 0x50) col = 0x50;
                    for (int i = curCol; i < col; i++) {
                        int offset = mRAM[vram + i];
                        if (prevAttr & ATTR_CHAR_GRAPH) {
                            offset += 0x100;
                        }
                        *(cache + i) = prevAttr | (offset << 16);
                    }

                    curCol = col;
                    if (col >= 0x50) {
                        break;
                    }
                    prevAttr = attr;
                }
            }
        }
    } else {  // 40 columns
        for (int row = 0; row < 25; row++) {
            auto attrMode = false;
            auto attrPtr = mRAM + mVRAM + 120 * row + 80;
            memset(mVramCol, 0x80, 20);
            memset(mVramAttr, 0, 20);
            int j = 0;
            mVramCol[j] = *attrPtr < 0x50 ? *attrPtr : 0x50;
            mVramCol[j] = *attrPtr;
            if (*attrPtr >= 0x80) attrMode = true;
            attrPtr++;
            mVramAttr[j] = *attrPtr;
            attrPtr++;
            j++;
            for (int i = 1; i < 20; i++) {
                auto col = *attrPtr;
                attrPtr++;
                auto value = *attrPtr;
                attrPtr++;
                if (col >= 0x80) attrMode = true;
                if (col > mVramCol[j - 1]) {
                    mVramCol[j] = col;
                    mVramAttr[j] = value;
                    j++;
                } else if (col < mVramCol[j - 1]) {
                    for (int k = 0; k < j - 1; k++) {
                        if (mVramCol[k] == col) {
                            break;
                        } else if (mVramCol[k] > col) {
                            for (int l = 19; l >= k; l--) {
                                mVramCol[l + 1] = mVramCol[l];
                                mVramAttr[l + 1] = mVramAttr[l];
                            }
                            mVramCol[k] = col;
                            mVramAttr[k] = value;
                            j++;
                            break;
                        }
                    }
                }
            }

            uint16_t attr;
            uint8_t vramAttr;
            int curCol, col, graphic;
            auto cache = mVramCache + row * 80;
            auto vram = mVRAM + row * 120;

            col = 0;
            curCol = 0;

            if (mColorMode) {  // Color
                bool mode = mVramCol[0] != 0;
                for (int i = 0; i < 20; i++) {
                    col = mVramCol[i];
                    vramAttr = mVramAttr[i];
                    attr = prevAttr & 0xffff;
                    if (vramAttr & 0x08) {
                        attr &= 0x7f00;
                        if (vramAttr & 0x10) {  // Character/Graphic for color mode
                            attr |= ATTR_CHAR_GRAPH;
                        }
                        attr |= vramAttr >> 5;  // color
                    } else {
                        attr &= 0x80ff;
                        attr |= (vramAttr & 0x37) << 8;
                    }

                    if (col & 0x01) col++;

                    if (attrMode) {  // N88
                        prevAttr = attr;
                    }
                    if (col > 0x50) col = 0x50;
                    for (int i = curCol; i < col; i += 2) {
                        int offset = (mRAM[vram + i] << 1) + 0x200;
                        if (prevAttr & ATTR_CHAR_GRAPH) {
                            offset += 0x200;
                        }
                        *(cache + i) = prevAttr | (offset << 16);
                        *(cache + i + 1) = prevAttr | ((offset + 1) << 16);
                    }

                    curCol = col;
                    if (col >= 0x50) {
                        break;
                    }
                    prevAttr = attr;
                }
            } else {  // Mono
                for (int i = 0; i < 20; i++) {
                    col = mVramCol[i];
                    vramAttr = mVramAttr[i];
                    if (vramAttr & 0x08) {
                        attr = prevAttr;
                    } else {
                        attr = WHITE | (vramAttr << 8);
                    }

                    if (col & 0x01) col++;

                    if (attrMode) {  // N88
                        prevAttr = attr;
                    }
                    if (col > 0x50) col = 0x50;
                    for (int i = curCol; i < col; i += 2) {
                        int offset = (mRAM[vram + i] << 1) + 0x200;
                        if (prevAttr & ATTR_CHAR_GRAPH) {
                            offset += 0x200;
                        }
                        *(cache + i) = prevAttr | (offset << 16);
                        *(cache + i + 1) = prevAttr | ((offset + 1) << 16);
                    }

                    curCol = col;
                    if (col >= 0x50) {
                        break;
                    }
                    prevAttr = attr;
                }
            }
        }
    }

    return mDisplay;
}
