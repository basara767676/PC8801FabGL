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

class PD8257 {
   public:
    PD8257();
    ~PD8257();

    int init(PC88VM *vm);
    int run(void);

    void dmaAddress(int channel, uint8_t value);
    void dmaTerminalCount(int channel, uint8_t value);
    void dmaCmd(uint8_t value);
    uint8_t inPort68(void);

   private:
    PC88VM *mVM;

    int mChannelAddress[4];
    int mChannelCount[4];

    uint8_t mPort68;
};