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
#include "scancode.h"

#ifdef DEBUG_PC88
// #define DEBUG_PC88KEYBOARD
#endif

PC88KeyBoard::PC88KeyBoard() {}
PC88KeyBoard::~PC88KeyBoard() {}

uint8_t *PC88KeyBoard::mKeyMap;

int PC88KeyBoard::init(uint8_t *keyMap, void (*callBack)(void *, int), void *arg) {
    // PS2Controller
    PS2Controller.begin(PS2Preset::KeyboardPort0);
    PS2Controller.keyboard()->begin(false, false, 0);
    PS2Controller.keyboard()->setLayout(&fabgl::JapaneseLayout);

    mLshift = true;
    mRshift = true;

    mLctrl = true;
    mRctrl = true;

    mLalt = true;
    mRalt = true;

    mLwin = true;
    mRwin = true;

    mSuspending = false;

    mKeyMap = keyMap;

    mCallBack = callBack;
    mArg = arg;

    mKana = true;
    mKanaUp = true;

    mCaps = true;
    mCapsUp = true;

    mPadEnter = false;

    return 0;
}

void PC88KeyBoard::reset() {
    mCaps = true;
    mKana = true;
    PS2Controller.keyboard()->setLEDs(false, false, false);
}

void PC88KeyBoard::suspend(bool value) {
    if (value) {
        PS2Controller.keyboard()->setLEDs(false, false, false);
    } else {
        // PS2Controller.keyboard()->reset();
        PS2Controller.keyboard()->enableVirtualKeys(false, false);
        PS2Controller.keyboard()->setLEDs(false, !mCaps, !mKana);
    }
    mSuspending = value;
}

void PC88KeyBoard::run(void) { xTaskCreateUniversal(&keyBoardTask, "keyBoardTaskTask", 2048, this, 1, &mTaskHandle, APP_CPU_NUM); }

#define LEFT_SHIFT (0x12)
#define RIGHT_SHIFT (0x59)

#define LEFT_CTRL (0x14)
#define RIGHT_CTRL (0x114)

#define LEFT_ALT (0x11)
#define RIGHT_ALT (0x111)

#define LEFT_WIN (0x11f)
#define RIGHT_WIN (0x127)

#define KANA (0x13)
#define CAPS (0x58)
#define F09 (0x01)
#define F10 (0x09)
#define F12 (0x07)
#define INSERT (0x170)
#define DELETE (0x171)
#define LEFT (0x16b)
#define RIGHT (0x174)
#define UP (0x175)
#define DOWN (0x172)
#define PAD_ENTER (0x15A)

void IRAM_ATTR PC88KeyBoard::keyBoardTask(void *pvParameters) {
    auto kb = (PC88KeyBoard *)pvParameters;

    auto keyboard = kb->PS2Controller.keyboard();

    bool keyUp = false;
    bool e0 = false;

    while (true) {
        while (keyboard->scancodeAvailable() && !kb->mSuspending) {
            auto scanCode = keyboard->getNextScancode();
            if (scanCode != -1) {
                if (scanCode == 0xe0) {
                    e0 = true;
                } else if (scanCode == 0xf0) {
                    keyUp = true;
                } else {
                    if (e0) scanCode += 0x100;
                    switch (scanCodeTable[scanCode].type) {
                        case NORMAL__KEY:
                            if (keyUp) {
                                mKeyMap[scanCodeTable[scanCode].address] |= scanCodeTable[scanCode].data;
                            } else {
                                mKeyMap[scanCodeTable[scanCode].address] &= ~scanCodeTable[scanCode].data;
                            }
                            break;
                        case CTL_ALT_KEY:
                            if (!keyUp && (!kb->mLctrl || !kb->mRctrl) && (!kb->mLalt || !kb->mRalt)) {
                                kb->mSuspending = true;
                                switch (scanCode) {
                                    case INSERT:
                                        (*kb->mCallBack)(kb->mArg, CMD_COLD_BOOT);
                                        break;
                                    case DELETE:
                                        (*kb->mCallBack)(kb->mArg, CMD_RESET);
                                        break;
                                    default:
                                        kb->mSuspending = false;
                                        break;
                                }
                            } else {
                                updateKeyMap(keyUp, scanCode);
                            }
                            break;
                        case WINDOWS_KEY:
                            if (!keyUp && (!kb->mLwin || !kb->mRwin)) {
                                kb->mSuspending = true;
                                switch (scanCode) {
                                    case LEFT:
                                        (*kb->mCallBack)(kb->mArg, CMD_TAPE_REWIND);
                                        break;
                                    case RIGHT:
                                        (*kb->mCallBack)(kb->mArg, CMD_TAPE_EOT);
                                        break;
                                    case UP:
                                        (*kb->mCallBack)(kb->mArg, CMD_VOLUME_UP);
                                        break;
                                    case DOWN:
                                        (*kb->mCallBack)(kb->mArg, CMD_VOLUME_DOWN);
                                        break;
                                    default:
                                        kb->mSuspending = false;
                                        break;
                                }
                                delay(50);
                            } else {
                                updateKeyMap(keyUp, scanCode);
                                if (scanCode == LEFT || scanCode == DOWN) {
                                    updateKeyMap(keyUp, 0x12);
                                }
                            }
                            break;
                        case CPU___SPEED:
                            if (!keyUp && (!kb->mLwin || !kb->mRwin)) {
                                const static uint8_t table[10] = {0x45, 0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46};
                                const static uint8_t cpuTable[10] = {CPU_SPEED_NO_WAIT,       CPU_SPEED_VERY_VERY_FAST, CPU_SPEED_VERY_FAST,
                                                                     CPU_SPEED_FAST,          CPU_SPEED_A_LITTLE_FAST,  CPU_SPEED_NORMAL,
                                                                     CPU_SPEED_A_LITTLE_SLOW, CPU_SPEED_A_LITTLE_SLOW,  CPU_SPEED_VERY_SLOW,
                                                                     CPU_SPEED_VERY_VERY_SLOW};
                                int cpuSpeed = CPU_SPEED_NORMAL;
                                for (int i = 0; i < 10; i++) {
                                    if (scanCode == table[i]) {
                                        cpuSpeed = cpuTable[i];
                                        break;
                                    }
                                }

                                kb->mSuspending = true;
                                (*kb->mCallBack)(kb->mArg, CMD_CPU_SPEED + cpuSpeed);
                                delay(50);
                            } else {
                                updateKeyMap(keyUp, scanCode);
                            }
                            break;

                        case SPECIAL_KEY:
                            switch (scanCode) {
                                case LEFT_SHIFT:
                                    kb->mLshift = keyUp;
                                    updateKeyMap(kb->mLshift && kb->mRshift, scanCode);
                                    break;
                                case RIGHT_SHIFT:
                                    kb->mRshift = keyUp;
                                    updateKeyMap(kb->mLshift && kb->mRshift, scanCode);
                                    break;
                                case LEFT_CTRL:
                                    kb->mLctrl = keyUp;
                                    updateKeyMap(kb->mLctrl && kb->mRctrl, scanCode);
                                    break;
                                case RIGHT_CTRL:
                                    kb->mRctrl = keyUp;
                                    updateKeyMap(kb->mLctrl && kb->mRctrl, scanCode);
                                    break;
                                case LEFT_ALT:
                                    kb->mLalt = keyUp;
                                    updateKeyMap(kb->mLalt && kb->mRalt, scanCode);
                                    break;
                                case RIGHT_ALT:
                                    kb->mRalt = keyUp;
                                    updateKeyMap(kb->mLalt && kb->mRalt, scanCode);
                                    break;
                                case LEFT_WIN:
                                    kb->mLwin = keyUp;
                                    break;
                                case RIGHT_WIN:
                                    kb->mRwin = keyUp;
                                    break;
                                case KANA:
                                    if (keyUp != kb->mKanaUp) {
                                        if (kb->mKanaUp) {
                                            kb->mKana = !kb->mKana;
                                            updateKeyMap(kb->mKana, scanCode);
                                        }
                                        kb->mKanaUp = keyUp;
                                        kb->PS2Controller.keyboard()->setLEDs(false, !kb->mCaps, !kb->mKana);
                                    }
                                    break;
                                case CAPS:
                                    if (keyUp != kb->mCapsUp) {
                                        if (kb->mCapsUp) {
                                            kb->mCaps = !kb->mCaps;
                                            updateKeyMap(kb->mCaps, scanCode);
                                        }
                                        kb->mCapsUp = keyUp;
                                        kb->PS2Controller.keyboard()->setLEDs(false, !kb->mCaps, !kb->mKana);
                                    }
                                    break;
                                case F12:
                                    if (!keyUp) {
                                        kb->mSuspending = true;
                                        (*kb->mCallBack)(kb->mArg, CMD_PC88MENU);
                                    }
                                    break;
                                case F10:
                                    if (!keyUp) {
                                        kb->mSuspending = true;
                                        (*kb->mCallBack)(kb->mArg, CMD_PCG_ON_OFF);
                                    }
                                    break;
                                case F09:
                                    if (!keyUp) {
                                        kb->mSuspending = true;
                                        (*kb->mCallBack)(kb->mArg, CMD_BEEP_MUTE);
                                    }
                                    break;
                                case PAD_ENTER:
                                    if (kb->mPadEnter) {
                                        updateKeyMap(keyUp, (kb->mLshift && kb->mRshift) ? 0x17f : scanCode);
                                    } else {
                                        updateKeyMap(keyUp, (kb->mLshift && kb->mRshift) ? scanCode : 0x17f);
                                    }
                                    break;
                            }
                            break;
                        default:
#ifdef DEBUG_PC88KEYBOARD
                            Serial.printf("%03x %s %s\n", scanCode, scanCodeTable[scanCode].name, keyUp ? "up" : "down");
#endif
                            break;
                    }
                    keyUp = false;
                    e0 = false;
                }
            }
        }
        // delay(1);
    }
}

void IRAM_ATTR PC88KeyBoard::updateKeyMap(bool keyUp, int scanCode) {
    if (keyUp) {
        mKeyMap[scanCodeTable[scanCode].address] |= scanCodeTable[scanCode].data;
    } else {
        mKeyMap[scanCodeTable[scanCode].address] &= ~scanCodeTable[scanCode].data;
    }
}

void PC88KeyBoard::setPadEnter(bool value) { mPadEnter = value; }