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

class PD1990 {
   public:
    PD1990();
    ~PD1990();

    uint8_t read(void);
    void write(int address, uint8_t value);

   private:
    uint8_t mCmd;
    bool mDataIn;

    bool mCSTB;
    bool mCCK;

    bool mShift;

    uint64_t mInData;
    uint64_t mOutData;

    void clockSTB(void);

    void getDateTime(void);
    uint8_t toBCD(uint8_t value);

    void setDateTime(void);
    uint8_t toBin(uint8_t value);

    static const char *msg[8];
};