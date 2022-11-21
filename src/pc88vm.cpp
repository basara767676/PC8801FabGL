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

#include "pc88error.h"

#ifdef DEBUG_PC88
// #define DEBUG_PC88VM
#endif

PC88VM::PC88VM() {}
PC88VM::~PC88VM() {}

int PC88VM::init(void) {
    initFileSystem();

    mPC88Settings = new PC88SETTINGS;
    mPC88Settings->init(mRootDir);
    mSettings = mPC88Settings->get();

    mPD3301 = new PD3301;
    mPD3301->init(&mPort40In);
    PC88ERROR::setDisplayController(mPD3301->getDisplayController());
    initGvramCache();

    mPD3301->setCache(mGvramCache200, mGvramCache400);
    mPD3301->run();

    initMemory();
    initFont();
    initDisk();

    mPD3301->setMemory(mRAM0000, mFontROM);

    mPD780C = new fabgl::Z80;

    mXQueue = xQueueCreate(10, sizeof(cpu_cmd_t));
    mXQueueDebug = xQueueCreate(10, sizeof(debug_cmd_t));

    mPD8257 = new PD8257;
    mPD8257->init(this);

    mI8255 = new I8255;
    mI8255->init(I8255_PC8801);

    mPD1990 = new PD1990;

    mPC80S31 = new PC80S31;
    mPC80S31->init(this, mDiskROM, mI8255);

    mPCG8800 = new PCG8800;
    mPCG8800->init(mFontROM, mSettings->volume);

    mDR320 = new DR320;
    mDR320->init(&mXQueue);

    mKeyboard = new PC88KeyBoard;
    mKeyboard->init(&mKeyMap[0], PC88VM::keyboardCallBack, this);

    mPC88MENU = new PC88MENU;

    coldBoot();

#ifdef DEBUG_PC88VM
    Serial.println("PC88VM init completed");
#endif
    return 0;
}

void PC88VM::coldBoot(void) {
#ifdef DEBUG_PC88VM
    Serial.println("cold boot");
#endif

    if (mSettings) free(mSettings);
    mSettings = mPC88Settings->get();

    mHighResolution = !mSettings->line200;

    setCpuSpeed(mSettings->speed);

    if (mHighResolution) {
        mPD780C->setCallbacks(this, readByte, writeByte400, readWord, writeWord400, readIO, writeIO);
    } else {
        mPD780C->setCallbacks(this, readByte, writeByte200, readWord, writeWord200, readIO, writeIO);
    }

    mPCG8800->initFont();

    if (mSettings->pcg) {
        mPCG8800->pcgON();
    }

    mRAM0000 = mTextRAM0000;
    for (int i = 0; i < 0x10000; i += 4) *(uint32_t *)(mRAM0000 + i) = 0xff00ff00;

    if (mSettings->extRam && (mExtRAM == nullptr)) {
        mExtRAM = (uint8_t *)ps_malloc(1024 * 128);
#ifdef DEBUG_PC88VM
        Serial.println("Allocate Ext RAM");
#endif
    }
    if (mExtRAM) {
        for (int i = 0; i < 1024 * 128; i += 4) *(uint32_t *)(mExtRAM + i) = 0xff00ff00;
    }

    reset();
}

void PC88VM::reset(void) {
#ifdef DEBUG_PC88VM
    Serial.println("reset");
#endif

    mPC80S31->reset();

    mKeyboard->reset();

    for (int i = 0; i < 4; i++) {
        if (strlen(mSettings->disk[i]) > 0) {
            mPC80S31->openDrive(i, mSettings->disk[i]);
        }
    }

    mDR320->open(mSettings->tape);

    mPCG8800->reset();
    mPCG8800->setVolume(mSettings->volume);

    mGBank = GBANK_MAIN;
    m0000Bank = mN88ROM;
    mRAM0000 = mTextRAM0000;

    mPortE2 = 0;
    mPortE3 = 0;

    mPort70 = 0x80;

    mDipSW1 = 0x00;                            // N80:0x02 N88:0x03
    if (mSettings->n88) mDipSW1 |= 0x01;       // 0x01 N88 / N80
    mDipSW1 |= 0x02;                           // 0x02 BASIC / Terminal
    if (mSettings->column40) mDipSW1 |= 0x04;  // 0x04 Column 40 / 80
    if (mSettings->row20) mDipSW1 |= 0x08;     // 0x08 Row 20 / 25

    mDipSW2 = 0x00;

    mPD3301->reset();

    memset(mGVRAM0, 0, 0xc000);
    memset(mGvramCache200, 0, 0x10000);
    memset(mGvramCache400, 0, 1024 * 32);

    mPD3301->displayMode(0x01, mHighResolution);

    // Keyboard
    for (int i = 0; i < sizeof(mKeyMap); i++) {
        mKeyMap[i] = 0xff;
    }

    // MMODE: RAM mode control, 0: ROM, RAM mode, 1: 64K RAM Mode
    // RMODE: ROM mode control, 0: N88-BAISC, 1: N-BASIC
    mMemMode = 0;  // N88

    mExtROM = 0xff;

    mPort40In = 0;
    if (mSettings->drive) {
        mPort40In &= 0xf7;  // PC-80S31
    } else {
        mPort40In |= 0x08;  // no disk drives
    }
    if (mSettings->line200) {
#ifdef DEBUG_PC88VM
        Serial.println("Normal resolution");
#endif
        mPort40In |= 0x02;  // 1: Normal resolution
    } else {
#ifdef DEBUG_PC88VM
        Serial.println("High resolution");
#endif
        mPort40In &= 0xfd;  // 0: High resolution
    }
    mPort40In |= 0x04;  // CMT

    mVRTC = false;

    mIntClock = false;
    mIntVRTC = false;
}

void PC88VM::run(void) {
#ifdef DEBUG_PC88VM
    Serial.println("pc-8801 start.");
#endif

    disableCore0WDT();
    delay(100);
    // disableCore1WDT();

    init();

    xTaskCreateUniversal(&pc88Task, "pc88Task", 4096, this, 1, &mTaskHandle, PRO_CPU_NUM);

    // mPD3301->run();
    mPD8257->run();
    mKeyboard->run();

#ifdef DEBUG_PC88VM
    Serial.println("setup() end.");
#endif
}

void PC88VM::keyboardCallBack(void *arg, int value) {
    auto vm = (PC88VM *)arg;
    vm->mKbCmd = value;
    vm->mSuspending = true;
}

void IRAM_ATTR PC88VM::pc88Task(void *pvParameters) {
#ifdef DEBUG_PC88VM
    Serial.println("pc88Task");
#endif

    auto vm = (PC88VM *)pvParameters;

    vm->mSuspending = false;

    vm->mPD780C->reset();
    vm->mPD780C->setPC(0);

    int intCount = 0;
    int cycles = 0;
    int diff = 0;
    uint32_t previousTime = micros();

    while (true) {
        if (vm->mSuspending) {
            vm->vmControl(vm);
            previousTime = micros();
            cycles = 0;
            diff = 0;
        }

        cycles += vm->mPD780C->step();
        vm->mPD3301->updateVRAMcahce();

        if (cycles > 100) {
            uint32_t currentTime = micros();
            int d = currentTime - previousTime;
            if (d < 0) diff = (0xFFFFFFFF - previousTime) + currentTime;
            previousTime = currentTime;
            diff += d;

            int wait = (cycles - d / 10) / vm->mWait;
            if (wait > 0 & !vm->mNoWait) delayMicroseconds(wait);
            cycles = 0;
        }

        if (diff > 1666) {  // 1/600 sec
            if (vm->mIntClock) {
                cpu_cmd_t msg;
                msg.cmd = INT_CLOCK;
                xQueueSend(vm->mXQueue, &msg, 0);
            }
            intCount++;
            if (intCount % 2 == 0) {
                vm->mDR320->interrupt();
            }
            if (intCount > 9) {
                if (vm->mIntVRTC) {
                    cpu_cmd_t msg;
                    msg.cmd = INT_VTRC;
                    xQueueSend(vm->mXQueue, &msg, 0);
                }
                intCount = 0;
            }
            diff = 0;
        }

        if (vm->mPD780C->getIFF1()) {
            cpu_cmd_t msg;
            if (xQueueReceive(vm->mXQueue, &msg, 0)) {
                int intLevel = vm->mPortE4 & 0x03;
                switch (msg.cmd) {
                    case INT_RXRDY:
                        if (intLevel > 0) {
                            vm->mPD780C->IRQ(INT_RXRDY);
                        } else {
                            xQueueSend(vm->mXQueue, &msg, 0);
                        }
                        break;
                    case INT_VTRC:
                        if (intLevel > 1) {
                            vm->mPD780C->IRQ(INT_VTRC);
                        } else {
                            xQueueSend(vm->mXQueue, &msg, 0);
                        }
                        break;
                    case INT_CLOCK:
                        if (intLevel > 2) {
                            vm->mPD780C->IRQ(INT_CLOCK);
                        } else {
                            xQueueSend(vm->mXQueue, &msg, 0);
                        }
                        break;
                }
            }
        }
    }
}

void PC88VM::suspend(bool suspend, bool pd3301) {
    mSuspending = suspend;
    mKeyboard->suspend(suspend);
    if (pd3301) mPD3301->suspend(suspend);
    mPCG8800->suspend(suspend);
}

// PC-8801-02N
void PC88VM::setExtRam(void) {
    if (mExtRAM == nullptr) {
        mPortE2 = 0;
        mPortE3 = 0;
        return;
    }

    mPortE2 &= 0x11;
    mPortE3 &= 0x03;

    if (mPortE2 == 0x11) {  // Enable
        m0000Bank = mExtRAM + 0x8000 * mPortE3;
        mRAM0000 = m0000Bank;
    } else {  // Disable
        mPortE2 = 0;
        switch (mMemMode) {
            case 0:  // N80
                m0000Bank = mN80ROM;
                break;
            case 1:  // N88
                m0000Bank = mN88ROM;
                break;
            default:  // RAM
                m0000Bank = mTextRAM0000;
                break;
        }
        mRAM0000 = mTextRAM0000;
    }
}

int IRAM_ATTR PC88VM::readWord(void *context, int addr) { return readByte(context, addr) | (readByte(context, addr + 1) << 8); }

void IRAM_ATTR PC88VM::writeWord200(void *context, int addr, int value) {
    writeByte200(context, addr, value & 0xFF);
    writeByte200(context, addr + 1, value >> 8);
}

void IRAM_ATTR PC88VM::writeWord400(void *context, int addr, int value) {
    writeByte400(context, addr, value & 0xFF);
    writeByte400(context, addr + 1, value >> 8);
}

int IRAM_ATTR PC88VM::readByte(void *context, int address) {
    auto vm = (PC88VM *)context;

    int value;

    if (address < 0x8000) {
        if (vm->mExtROM == 0xff) {
            value = vm->m0000Bank[address];
        } else if (address < 0x6000) {
            value = vm->mN88ROM[address];
        } else {
            if ((vm->mExtROM & 0x01) == 0) {  // 0xFE
                value = vm->m4thROM[address - 0x6000];
            } else if ((vm->mExtROM & 0x02) == 0) {  // 0xFD
                value = vm->mUserROM[address - 0x6000];
            } else {
                value = 0;
            }
        }
    } else {
        // 0x8000 - 0xFFFF
        if (address < 0x8300) {  // Window 0x8000 - 0x82ff
            address = (vm->mPort70 << 8) + address - 0x8000;
        }

        if (address < 0x8000) {
            value = vm->mRAM0000[address];
        } else if (address < 0xc000) {
            value = vm->mRAM8000[address - 0x8000];
        } else {
            value = vm->mGBankMem[vm->mGBank][address - 0xc000];
        }
    }

    return value;
}

void IRAM_ATTR PC88VM::writeByte200(void *context, int address, int value) {
    static uint32_t bankMask[3] = {0x36363636, 0x2d2d2d2d, 0x1b1b1b1b};

    auto vm = (PC88VM *)context;

    value &= 0xff;

    if (address < 0x8000) {
        vm->mRAM0000[address] = value;
    } else {
        if (address < 0x8300) {
            address = (vm->mPort70 << 8) + address - 0x8000;
        }

        if (address < 0x8000) {
            vm->mRAM0000[address] = value;
        } else if (address < 0xc000) {
            vm->mRAM8000[address - 0x8000] = value & 0xff;
        } else {
            address -= 0x0c000;
            auto gBank = vm->mGBank;
            vm->mGBankMem[gBank][address] = value;
            if (gBank == GBANK_MAIN) return;

            *((uint32_t *)&vm->mGvramCache200[address * 4]) &= bankMask[gBank];
            *((uint32_t *)&vm->mGvramCache200[address * 4]) |= *(vm->mBankBit + gBank * 256 + value);
        }
    }
}

void IRAM_ATTR PC88VM::writeByte400(void *context, int address, int value) {
    static uint32_t bankMask[3] = {0x36363636, 0x2d2d2d2d, 0x1b1b1b1b};

    auto vm = (PC88VM *)context;

    value &= 0xff;

    if (address < 0x8000) {
        vm->mRAM0000[address] = value;
    } else {
        if (address < 0x8300) {
            address = (vm->mPort70 << 8) + address - 0x8000;
        }

        if (address < 0x8000) {
            vm->mRAM0000[address] = value;
        } else if (address < 0xc000) {
            vm->mRAM8000[address - 0x8000] = value & 0xff;
        } else {
            address -= 0x0c000;
            auto gBank = vm->mGBank;
            vm->mGBankMem[gBank][address] = value;
            if (gBank == GBANK_MAIN) return;

            *((uint32_t *)&vm->mGvramCache200[address * 4]) &= bankMask[gBank];
            *((uint32_t *)&vm->mGvramCache200[address * 4]) |= *(vm->mBankBit + gBank * 256 + value);

            address &= 0xfffc;
            if (address < 80 * 200) {
                switch (gBank) {
                    case GBANK0_BLUE:
                        *((uint32_t *)&vm->mGvramCache400[address]) = *((uint32_t *)&vm->mGBankMem[gBank][address]);
                        break;
                    case GBANK1_RED:
                        *((uint32_t *)&vm->mGvramCache400[address + 80 * 200]) = *((uint32_t *)&vm->mGBankMem[gBank][address]);
                        break;
                }
            }
        }
    }
}

int PC88VM::readIO(void *context, int address) {
    auto vm = (PC88VM *)context;
    auto pc = vm->mPD780C->getPC();

    switch (address & 0xff) {
        case 0x00:
            return vm->mKeyMap[0];
        case 0x01:
            return vm->mKeyMap[1];
        case 0x02:
            return vm->mKeyMap[2];
        case 0x03:
            return vm->mKeyMap[3];
        case 0x04:
            return vm->mKeyMap[4];
        case 0x05:
            return vm->mKeyMap[5];
        case 0x06:
            return vm->mKeyMap[6];
        case 0x07:
            return vm->mKeyMap[7];
        case 0x08:
            return vm->mKeyMap[8];
        case 0x09:
            return vm->mKeyMap[9];
        case 0x0a:
            return vm->mKeyMap[10];
        case 0x0b:
            return vm->mKeyMap[11];
        case 0x20:
            return vm->mDR320->readData();
        case 0x21:
            return vm->mDR320->readStatus();
        case 0x30:
            return vm->mDipSW1;
        case 0x31:
            return vm->mDipSW2;
        case 0x40:
            // VRTC is updated in PD3301::drawScanline.
            vm->mPort40In = (vm->mPort40In & 0xef) | vm->mPD1990->read();  // PD1990 calender clock
            return vm->mPort40In;
        case 0x50:
            return vm->mPD3301->inPort50();
        case 0x51:
            return vm->mPD3301->inPort51();
        case 0x5c: {
            static uint8_t port5c[4] = {0xf9, 0xfa, 0xfc, 0xf8};
            return port5c[vm->mGBank];
        }
            return 0;
        case 0x68:
            return vm->mPD8257->inPort68();
        case 0x70:
            return vm->mPort70;
            break;
        case 0x71:
            return vm->mExtROM;
        case 0x78:
            return vm->mPort70;
        case 0xe8:  // Kanji rom
            return *(vm->mKanjiROM + (vm->mKanjiROMAddr << 1) + 1);
        case 0xe9:
            return *(vm->mKanjiROM + (vm->mKanjiROMAddr << 1));
        case 0xe2:  // PC-8801-02N
            return ~vm->mPortE2;
            break;
        case 0xe3:  // PC-8801-02N
            return vm->mPortE3;
            break;
        case 0xea:
            return 0xff;
            break;
        case 0xeb:
            return 0xff;
            break;
        case 0xf4:  // DMA 5inch disk unit
            return 0x01;
        case 0xf8:  // DMA 5inch disk unit
            return 0x01;
        case 0xfc:  // Mini disk unit Port A
            return vm->mI8255->in(I8255_PORT_A);
        case 0xfd:  // Mini disk unit Port B
            return vm->mI8255->in(I8255_PORT_B);
        case 0xfe:  // Mini disk unit Port C
            return vm->mI8255->in(I8255_PORT_C);
        case 0xff:  // Mini disk unit control port
            return vm->mI8255->in(I8255_PORT_CONTROL);
        default:
#ifdef DEBUG_PC88VM
            Serial.printf("Read non-implemeted port %02x\n", address);
#endif
            return 0;
    }
    return 0;
}

void PC88VM::writeIO(void *context, int address, int value) {
    auto vm = (PC88VM *)context;

    switch (address & 0xff) {
        case 0x00:
            vm->mPCG8800->port00(value);
            break;
        case 0x01:
            vm->mPCG8800->port01(value);
            break;
        case 0x02:
            vm->mPCG8800->port02(value);
            break;
        case 0x03:
            vm->mPCG8800->port03(value);
            break;
        case 0x0c:
            vm->mPCG8800->port0c(value);
            break;
        case 0x0d:
            vm->mPCG8800->port0d(value);
            break;
        case 0x0e:
            vm->mPCG8800->port0e(value);
            break;
        case 0x0f:
            vm->mPCG8800->port0f(value);
            break;
        case 0x10:
            vm->mPD1990->write(0x10, value);
            break;
        case 0x20:
            vm->mDR320->writeData(value);
            break;
        case 0x21:
            vm->mDR320->modeCommand(value);
            break;
        case 0x30:
            vm->mPort30 = value & 0xff;
            vm->mColumn80 = value & 0x01;
            vm->mPD3301->setCloumn80(vm->mColumn80);
            vm->mDR320->systemControl(value);
            break;
        case 0x31:
            vm->mPort31 = value;
            vm->mMemMode = value & 0x02 ? 2 : value & 0x04 ? 0 : 1;
            switch (vm->mMemMode) {
                case 0:  // N80
                    vm->m0000Bank = vm->mN80ROM;
                    break;
                case 1:  // N88
                    vm->m0000Bank = vm->mN88ROM;
                    break;
                default:  // RAM
                    vm->m0000Bank = vm->mRAM0000;
                    break;
            }
            vm->mPD3301->displayMode(value, !(vm->mPort40In & 0x02));
            break;
        case 0x40:
            vm->mPCG8800->beep(value & 0x20);
            vm->mPD1990->write(0x40, value);
            vm->mPort40Out = value;
            break;
        case 0x50:
            vm->mPD3301->crtcData(value);
            break;
        case 0x51:
            vm->mPD3301->crtcCmd(value);
            break;
        case 0x52:
            vm->mPD3301->outPort52(value);
            break;
        case 0x53:
            vm->mPD3301->outPort53(value);
            break;
        case 0x54:
            vm->mPD3301->setColorPalette(BLACK, value & 0x07);
            break;
        case 0x55:
            vm->mPD3301->setColorPalette(BLUE, value & 0x07);
            break;
        case 0x56:
            vm->mPD3301->setColorPalette(RED, value & 0x07);
            break;
        case 0x57:
            vm->mPD3301->setColorPalette(MAGENTA, value & 0x07);
            break;
        case 0x58:
            vm->mPD3301->setColorPalette(GREEN, value & 0x07);
            break;
        case 0x59:
            vm->mPD3301->setColorPalette(CYAN, value & 0x07);
            break;
        case 0x5a:
            vm->mPD3301->setColorPalette(YELLOW, value & 0x07);
            break;
        case 0x5b:
            vm->mPD3301->setColorPalette(WHITE, value & 0x07);
            break;
        case 0x5c:
            vm->mGBank = GBANK0_BLUE;
            break;
        case 0x5d:
            vm->mGBank = GBANK1_RED;
            break;
        case 0x5e:
            vm->mGBank = GBANK2_GREEN;
            break;
        case 0x5f:
            vm->mGBank = GBANK_MAIN;
            break;
        case 0x60:
            vm->mPD8257->dmaAddress(0, value);
            break;
        case 0x61:
            vm->mPD8257->dmaTerminalCount(0, value);
            break;
        case 0x62:
            vm->mPD8257->dmaAddress(1, value);
            break;
        case 0x63:
            vm->mPD8257->dmaTerminalCount(1, value);
            break;
        case 0x64:
            vm->mPD8257->dmaAddress(2, value);
            break;
        case 0x65:
            vm->mPD8257->dmaTerminalCount(2, value);
            break;
        case 0x66:
            vm->mPD8257->dmaAddress(3, value);
            break;
        case 0x67:
            vm->mPD8257->dmaTerminalCount(3, value);
            break;
        case 0x68:
            vm->mPD8257->dmaCmd(value);
            break;
        case 0x70:
            vm->mPort70 = value & 0xff;
            break;
        case 0x71:
            vm->mExtROM = value & 0xff;
            break;
        case 0x78:
            vm->mPort70++;
            break;
        case 0xc0:  // 8251 RS-232C channel 1 Data
            break;
        case 0xc1:  // 8251 RS-232C channel 1 Command
            break;
        case 0xc2:  // 8251 RS-232C channel 2 Data
            break;
        case 0xc3:  // 8251 RS-232C channel 2 Command
            break;
        case 0xc8:  // Unknown
            break;
        case 0xca:  // Unknown
            break;
        case 0xe2:  // PC-8801-02N
            vm->mPortE2 = value & 0xff;
            vm->setExtRam();
            break;
        case 0xe3:  // PC8801-02N
            vm->mPortE3 = value & 0xff;
            vm->setExtRam();
            break;
        case 0xe4:
            vm->mPortE4 = value & 0xff;
            break;
        case 0xe6:  // Interrupt mask flag
            vm->mIntFlag = value & 0xff;
            vm->mIntClock = value & 0x01;
            vm->mIntVRTC = value & 0x02;
            vm->mDR320->interruptMask(value & 0x04);
            break;
        case 0xe8:  // Kanji rom
            vm->mKanjiROMAddr |= value;
            break;
        case 0xe9:
            vm->mKanjiROMAddr |= value << 8;
            break;
        case 0xea:
            break;
        case 0xeb:
            vm->mKanjiROMAddr = 0;
            break;
        case 0xfc:  //
            vm->mI8255->out(I8255_PORT_A, value & 0xff);
            break;
        case 0xfd:  //
            vm->mI8255->out(I8255_PORT_B, value & 0xff);
            break;
        case 0xfe:  //
            vm->mI8255->out(I8255_PORT_C, value & 0xff);
            break;
        case 0xff:  //
            vm->mI8255->out(I8255_PORT_CONTROL, value & 0xff);
            break;
        default:
#ifdef DEBUG_PC88VM
            Serial.printf("Write non-implemeted port %02x %02x\n", address, value);
#endif
            break;
    }
}

void PC88VM::vmControl(PC88VM *vm) {
    auto cmd = vm->mKbCmd;

    if (cmd == CMD_PC88MENU) {
        vm->suspend(true);
        cmd = vm->mPC88MENU->menu(vm);
        vm->suspend(false);
        if (cmd == -1) return;
    }

    switch (cmd) {
        case CMD_N80_FILE:  // for n80 file
            vm->mPD780C->reset();
            vm->mPD780C->setPC(0xff3d);
            vm->mPD780C->writeRegWord(Z80_SP, *(uint16_t *)&vm->mRAM0000[0xff3e]);
            break;
        case CMD_RESET:
            vm->reset();
            vm->mPD780C->reset();
            vm->mPD780C->setPC(0);
            break;
        case CMD_COLD_BOOT:
            vm->coldBoot();
            vm->mPD780C->reset();
            vm->mPD780C->setPC(0);
            break;
        case CMD_ESP32_RESTART:
            esp32Restart(vm);
            break;
        case CMD_PCG_ON_OFF:
            vm->mPCG8800->pcg();
            break;
        case CMD_TAPE_REWIND:
            vm->mDR320->rewind();
            break;
        case CMD_TAPE_EOT:
            vm->mDR320->eot();
            break;
        case CMD_BEEP_MUTE:
            vm->mPCG8800->soundMute();
            break;
        case CMD_VOLUME_UP:
            vm->mPCG8800->volumeUp();
            break;
        case CMD_VOLUME_DOWN:
            vm->mPCG8800->volumeDown();
        default:
            if (CMD_CPU_SPEED + CPU_SPEED_NO_WAIT <= cmd && cmd <= CMD_CPU_SPEED + CPU_SPEED_VERY_VERY_SLOW) {
                vm->setCpuSpeed(cmd - CMD_CPU_SPEED);
            }
            break;
    }
    vm->suspend(false, false);
}

void PC88VM::subTask(void) {
    if (mDiskROM) {
        mPC80S31->run();
    }
}

int PC88VM::initFileSystem(void) {
    strcpy(mRootDir, SD_MOUNT_POINT);
    strcat(mRootDir, PC88DIR);

    auto ret = FileBrowser::mountSDCard(false, SD_MOUNT_POINT);
    if (!ret) {
#ifdef DEBUG_PC88VM
        Serial.println("SD Mount failed");
#endif
        return -1;
    }

    const char *dirName[3] = {PC88DIR_DISK, PC88DIR_TAPE, PC88DIR_N80};

    FileBrowser dir(mRootDir);

    for (int i = 0; i < 3; i++) {
        char name[32];
        strcpy(&name[0], dirName[i]);
        if (!dir.exists(&name[1])) {
            dir.makeDirectory(&name[1]);
        }
    }

    strcat(mRootDir, "/");

    return ret;
}

int PC88VM::initMemory(void) {
    mRAM0000 = lalloc(0x10000, false);

    mTextRAM0000 = mRAM0000;

    mRAM8000 = mRAM0000 + 0x8000;
    mRAMC000 = mRAM0000 + 0xc000;

    for (int i = 0; i < 0x10000; i += 4) *(uint32_t *)(mRAM0000 + i) = 0xff00ff00;

    mGVRAM0 = lalloc(0xc000);
    mGVRAM1 = mGVRAM0 + 0x4000;
    mGVRAM2 = mGVRAM0 + 0x8000;
    memset(mGVRAM0, 0, 0xc000);

    mGBankMem[GBANK0_BLUE] = mGVRAM0;
    mGBankMem[GBANK1_RED] = mGVRAM1;
    mGBankMem[GBANK2_GREEN] = mGVRAM2;
    mGBankMem[GBANK_UNUSED] = (uint8_t *)0;
    mGBankMem[GBANK_MAIN] = mRAMC000;

    mN88ROM = lalloc(0x8000, false, "N88.ROM");
    mN80ROM = lalloc(0x8000, false, "N80.ROM");
    m4thROM = lalloc(0x2000, false, "N88_0.ROM");

    // workaround to avoid bug of IPL for mini disk unit
    // 45d1 37 -> 3f ; SCF -> CCF
    if (*(mN88ROM + 0x45d1) == 0x37) {
        *(mN88ROM + 0x45d1) = 0x3f;
#ifdef DEBUG_PC88VM
        Serial.println("Patched: 45d1");
#endif
    }

    mExtRAM = nullptr;

    mUserROM = lalloc(0x2000, false, "USER.ROM", false);
    if (mUserROM == nullptr) {
        mUserROM = lalloc(0x2000);
    }

    return 0;
}

int PC88VM::initFont(void) {
    static uint8_t fontConv[16] = {0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f, 0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff};

    auto font = lalloc(2048, false, "FONT.ROM");

    mFontROM = lalloc(10 * 256 * 2 * 3, true);
    memset(mFontROM, 0, 10 * 256 * 2 * 3);

    auto src = font;
    auto dest = mFontROM;
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 8; j++) {
            *dest = *src;
            src++;
            dest++;
        }
        *dest = 0;
        dest++;
        *dest = 0;
        dest++;
    }

    auto p = mFontROM + 10 * 256;
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 4; j++) {
            if ((i >> j) & 0x01) {
                *(p + j * 2) |= 0xf0;
                *(p + j * 2 + 1) |= 0xf0;
            }
            if ((i >> (j + 4)) & 0x01) {
                *(p + j * 2) |= 0x0f;
                *(p + j * 2 + 1) |= 0x0f;
            }
        }
        p += 10;
    }

    free(font);

    src = mFontROM;
    dest = mFontROM + (10 * 256) * 2;
    for (int i = 0; i < 256 * 2; i++) {
        for (int j = 0; j < 10; j++) {
            *(dest + j) = fontConv[(*(src + j) & 0xf0) >> 4];
            *(dest + j + 10) = fontConv[(*(src + j) & 0x0f)];
        }
        src += 10;
        dest += 20;
    }

    mKanjiROM = lalloc(128 * 1024, false, "KANJI1.ROM", false);
    if (mKanjiROM == nullptr) {
        mKanjiROM = lalloc(128 * 1024);
    }
    mKanjiROMAddr = 0;

    return 0;
}

int PC88VM::initDisk(void) {
#ifdef DEBUG_PC88VM
    Serial.println("initDisk");
#endif

    auto diskROM = lalloc(2048, false, "DISK.ROM", false);

    if (diskROM) {
        mDiskROM = lalloc(0x8000);
        memcpy(mDiskROM, diskROM, 2048);
        free(diskROM);
    } else {
        mDiskROM = nullptr;
    }

    return 0;
}

int PC88VM::initGvramCache(void) {
    mGvramCache200 = lalloc(0x10000, true);
    memset(mGvramCache200, 0, 0x10000);

    mGvramCache400 = (uint8_t *)heap_caps_malloc(1024 * 32, MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
    auto dest = (uint32_t *)mGvramCache400;
    for (int i = 0; i < 1024 * 32; i += 4) {
        *dest = 0;
        dest++;
    }

    mBankBit = (uint32_t *)heap_caps_malloc(3 * 256 * sizeof(uint32_t), MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);

    static uint8_t bankBit[][3] = {
        {0x08, 0x10, 0x20},
        {0x01, 0x02, 0x04},
    };

    for (int j = 0; j < 3; j++) {
        for (int i = 0; i <= 255; i++) {
            uint32_t b = 0;

            if (i & 0x40) b |= bankBit[0][j];
            if (i & 0x80) b |= bankBit[1][j];

            if (i & 0x10) b |= (bankBit[0][j] << 8);
            if (i & 0x20) b |= (bankBit[1][j] << 8);

            if (i & 0x04) b |= (bankBit[0][j] << 16);
            if (i & 0x08) b |= (bankBit[1][j] << 16);

            if (i & 0x01) b |= (bankBit[0][j] << 24);
            if (i & 0x02) b |= (bankBit[1][j] << 24);

            *(mBankBit + j * 256 + i) = b;
        }
    }

    return 0;
}

// load file with memory allocation

uint8_t *PC88VM::lalloc(size_t size, bool internal, const char *fileName, bool require) {
    auto mem = (uint8_t *)(internal ? heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) : ps_malloc(size));
    if (!mem) {
        PC88ERROR::dialog("Memory allocation error");
        return nullptr;
    }
#ifdef DEBUG_PC88VM
    Serial.printf("memory allocated: 0x%x bytes ", size);
#endif
    memset(mem, 0, size);

    if (fileName == nullptr) {
#ifdef DEBUG_PC88VM
        Serial.println("");
#endif
        return mem;
    }

    char fName[32];
    strcpy(fName, mRootDir);
    strcat(fName, fileName);

    struct stat fileStat;
    if (stat(fName, &fileStat) == -1) {
        if (require) {
            PC88ERROR::dialog("%s not found", fileName);
        }
        return nullptr;
    }

    if (size != fileStat.st_size) {
#ifdef DEBUG_PC88VM
        Serial.printf("File size error: %s\n", fName);
#endif
        if (require) {
            PC88ERROR::dialog("File size error: %s %d %d", fileName, size, fileStat.st_size);
        }
        free(mem);
        return nullptr;
    }

    auto fp = fopen(fName, "rb");
    if (!fp) {
        if (require) {
            PC88ERROR::dialog("Open error: %s", fileName);
        }
        free(mem);
        return nullptr;
    }

    fseek(fp, 0, SEEK_SET);
    size_t result = fread(mem, 1, fileStat.st_size, fp);

    fclose(fp);

    if (result != fileStat.st_size) {
        PC88ERROR::dialog("Read error: %s", fileName);
        free(mem);
        return nullptr;
    }

#ifdef DEBUG_PC88VM
    Serial.printf("%s is loaded %d bytes\n", fileName, size);
#endif

    return mem;
}

void PC88VM::setPCG(bool value, void *context) { ((PC88VM *)context)->mPD3301->setPCG(value); }

void PC88VM::setVolume(int value) { mPCG8800->setVolume(value); }

void PC88VM::setCpuSpeed(int speed) {
    mNoWait = false;
    mSettings->speed = speed;

    switch ((speed)) {
        case CPU_SPEED_NO_WAIT:
            mNoWait = true;
            break;
        case CPU_SPEED_VERY_VERY_FAST:
            mWait = 22;
            break;
        case CPU_SPEED_VERY_FAST:
            mWait = 13;
            break;
        case CPU_SPEED_FAST:
            mWait = 9;
            break;
        case CPU_SPEED_A_LITTLE_FAST:
            mWait = 7;
            break;
        case CPU_SPEED_NORMAL:
            mWait = 6;
            break;
        case CPU_SPEED_A_LITTLE_SLOW:
            mWait = 5;
            break;
        case CPU_SPEED_SLOW:
            mWait = 4;
            break;
        case CPU_SPEED_VERY_SLOW:
            mWait = 3;
            break;
        case CPU_SPEED_VERY_VERY_SLOW:
            mWait = 2;
            break;
        default:
            mWait = 6;
            break;
    }
}

void PC88VM::esp32Restart(PC88VM *vm) {
    vm->mKeyboard->reset();
    vm->mPC80S31->eject();
    vm->mDR320->close();
    ESP.restart();
}