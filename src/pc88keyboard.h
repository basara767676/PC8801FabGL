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

typedef struct {
    uint8_t type;
    uint8_t address;
    uint8_t data;
    const char *name;
} keyTable_t;

class PC88KeyBoard {
   public:
    PC88KeyBoard();
    ~PC88KeyBoard();

    int init(uint8_t *keyMap, void (*callBack)(void *, int), void *arg);
    void run(void);

    void suspend(bool value);
    void reset(void);
    void setPadEnter(bool value);

   private:
    fabgl::PS2Controller PS2Controller;

    bool mLshift;
    bool mRshift;

    bool mLctrl;
    bool mRctrl;

    bool mLalt;
    bool mRalt;

    bool mLwin;
    bool mRwin;

    bool mKana;
    bool mKanaUp;

    bool mCaps;
    bool mCapsUp;

    bool mPadEnter;

    static uint8_t *mKeyMap;
    static keyTable_t scanCodeTable[512];

    bool mSuspending;
    TaskHandle_t mTaskHandle;

    void (*mCallBack)(void *, int);
    void *mArg;

    static void keyBoardTask(void *pvParameters);
    static void updateKeyMap(bool keyUp, int scanCode);
};
