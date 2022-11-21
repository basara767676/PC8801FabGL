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
// #define DEBUG_PD8257
#endif

PD8257::PD8257() {}
PD8257::~PD8257() {}

int PD8257::init(PC88VM *vm) {
    mVM = vm;

    for (int i = 0; i < 4; i++) {
        mChannelAddress[i] = 0;
        mChannelCount[i] = 0;
    }

#ifdef DEBUG_PD8257
    Serial.println("PD8257 init completed");
#endif

    return 0;
}

int PD8257::run(void) { return 0; }

void PD8257::dmaAddress(int channel, uint8_t value) {
#ifdef DEBUG_PD8257
    Serial.printf("DMA channel %d %02x\n", channel, value);
#endif

    switch (mChannelCount[channel]) {
        case 0:
            mChannelAddress[channel] = value;
            mChannelCount[channel]++;
            break;

        case 1:
            mChannelAddress[channel] += value * 0x100;
            mChannelCount[channel] = 0;
#ifdef DEBUG_PD8257
            Serial.printf("DMA Address %d %04x\n", channel, mChannelAddress[channel]);
#endif
            if (channel == 2) {
                mVM->getPD3301()->setVRAM(mChannelAddress[channel]);
                channel = 0;
            }
            break;
    }
}

void PD8257::dmaTerminalCount(int channel, uint8_t value) {}
void PD8257::dmaCmd(uint8_t value)  // Port68
{
#ifdef DEBUG_PD8257
    Serial.printf("DMA command %04x\n", value);
#endif
    mVM->getPD3301()->setDMA((value & 0x04) == 0x04);
    mPort68 = value;
}

uint8_t PD8257::inPort68(void) { return mPort68; }