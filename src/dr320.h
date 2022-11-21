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
#include <cstdio>

#include "fabgl.h"

class DR320 {
   public:
    DR320();
    ~DR320();

    void init(QueueHandle_t* xQueue);
    int open(const char* fileName);
    int close(void);
    uint8_t readData(void);
    void writeData(uint8_t value);
    uint8_t readStatus(void);
    void modeCommand(uint8_t value);
    void systemControl(uint8_t value);
    void interruptMask(bool value);
    void interrupt(void);
    void rewind(void);
    void eot(void);

   private:
    QueueHandle_t* mXQueue;
    FILE* mTape;
    uint8_t mStatus;
    bool mMode;
    bool mMTON;
    bool mCmtEnable;
    bool mHighBps;
    bool mCDS;
    bool mInterruptEnable;
    bool mInit;
    bool mWrite;
};