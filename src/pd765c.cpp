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

#include "pd765c.h"

#include <Arduino.h>

#ifdef DEBUG_PC88
// #define DEBUG_PD765C
#endif

PD765C::PD765C() {
    mMainStatus = SR_RQM;
    mPhase = WAITING_PHASE;

    mCmdCount = 0;
    mResultCount = 0;

    mBuffer = (uint8_t *)ps_malloc(256 * 32);

    for (int i; i < MAX_DRIVE; i++) {
        mDrive[i].motor = false;
        mDrive[i].hasResult = false;
        mDrive[i].result = 0;
        mDrive[i].cylinder = 0;
        mDrive[i].disk = new PC88D88;
    }

    mIRQFlag = nullptr;
}
PD765C::~PD765C() {}

void PD765C::writeF4(uint8_t value) {
#ifdef DEBUG_PD765C
    Serial.printf("PD765C Port F4 : %02x\n", value);
#endif
}

void PD765C::writeF7(uint8_t value) { mVFO = value; }

void PD765C::writeF8(uint8_t value) {
    if (value & 0xf0) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C motor on/off: %02x\n", value & 0x0f);
#endif
        for (int i = 0; i < MAX_DRIVE; i++) {
            mDrive[i].motor = value & 0x01;
            value = value >> 1;
        }
    } else {
        mWritePrecompensation = value;
#ifdef DEBUG_PD765C
        Serial.printf("PD765C write precompensation: %02x\n", value);
#endif
    }
}

uint8_t PD765C::readStatusRegister(void) {  // Port FA
    static uint8_t prevSR = 0;
    if (mMainStatus != prevSR) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C Read status %02x\n", mMainStatus);
#endif
        prevSR = mMainStatus;
    }
    return mMainStatus;
}

void PD765C::writeDataRegister(uint8_t value) {  // Port FB
    if (mPhase == WAITING_PHASE) {
        mPhase = COMMAND_PHASE;
    }

    switch (mPhase) {
        case COMMAND_PHASE:
            commandPhase(value);
            break;
        case EXECUTION_PHASE:
            executionPhaseWrite(value);
            break;
        default:
#ifdef DEBUG_PD765C
            Serial.printf("PD765C Write data register %02x\n", value);
#endif
            break;
    }
}

uint8_t PD765C::readDataRegister(void) {  // Port FB
    auto result = mMainStatus;

    switch (mPhase) {
        case RESULT_PHASE:
            result = mResult[mResultOffset];
            mResultOffset++;
            if (mResultOffset < mResultCount) {
                mMainStatus = SR_RQM | SR_DIO | SR_CB;
            } else {
                cmdComplete();
            }
            break;
        case EXECUTION_PHASE:
            result = executionPhaseRead();
            break;

        default:
#ifdef DEBUG_PD765C
            Serial.printf("PD765C Read data register: unknown phase %d\n", mPhase);
#endif
            break;
    }
    return result;
}

void PD765C::commandPhase(uint8_t value) {
    mCmd[mCmdCount] = value;
    mCmdCount++;
    mExecCmd = mCmd[0] & 0x1f;
    switch (mExecCmd) {
        case READ_DIAGNOSTIC:
            readDiagnostic();
            break;
        case READ_DATA:
            readData();
            break;
        case READ_ID:
            readId();
            break;
        case WRITE_DATA:
            writeData();
            break;
        case WRITE_ID:
            writeId();
            break;
        case SPECIFY:
            specify();
            break;
        case SEEK:
            seek();
            break;
        case RECALIBRATE:
            recalibrate();
            break;
        case SENSE_INTERRUPT_STATUS:
            senseInterruptStatus();
            break;
        case SENSE_DEVICE_STATUS:
            senseDeviceStatus();
            break;
        default:
#ifdef DEBUG_PD765C
            Serial.printf("PD765C Write data register %02x\n", value);
#endif
            break;
    }
}

uint8_t PD765C::executionPhaseRead(void) {
    switch (mExecCmd) {
        case READ_DATA:
            return readDataExecution();
            break;
        case READ_DIAGNOSTIC:
            return readDiagnosticExecution();
            break;
    }
    return 0;
}

void PD765C::executionPhaseWrite(uint8_t value) {
    switch (mExecCmd) {
        case WRITE_DATA:
            return writeDataExecution(value);
            break;
        case WRITE_ID:
            return writeIdExecution(value);
            break;
    }
}

void PD765C::resultPhase(void) {
    switch (mExecCmd) {
        case READ_DATA:
            readDataResult();
            break;
        case READ_DIAGNOSTIC:
            readDiagnosticResult();
            break;
        case READ_ID:
            readIdResult();
            break;
        case WRITE_DATA:
            writeDataResult();
            break;
        case WRITE_ID:
            writeIdResult();
            break;
    }
    mExecCmd = -1;
    mPhase = RESULT_PHASE;
    mMainStatus = SR_RQM | SR_DIO | SR_CB;
}

void PD765C::cmdComplete(void) {
    mPhase = WAITING_PHASE;
    mCmdCount = 0;
    mMainStatus = SR_RQM;
}

uint8_t PD765C::terminalCount(void) {
#ifdef DEBUG_PD765C
    Serial.printf("Terminal Count mBuffOffset:%d\n", mBuffOffset);
#endif
    resultPhase();
    rasieIRQ();
    return 0;
}

void PD765C::rasieIRQ(void) {
    if (mIRQFlag) {
        *mIRQFlag = true;
#ifdef DEBUG_PD765C
        Serial.println("mIRQFlag = true");
#endif
    }
}

void PD765C::setIRQFlag(bool *irqFlag) { mIRQFlag = irqFlag; }

void PD765C::readData(void) {
    if (mCmdCount > 8) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C READ DATA - MT:%d MF:%d SK:%d HD:%d US:%d C:%02x H:%02x R:%02x N:%02x EOT:%02x GPL:%02x DTL:%02x\n",
                      (mCmd[0] & 0x80) >> 7, (mCmd[0] & 0x40) >> 6, (mCmd[0] & 0x20) >> 5, (mCmd[1] & 0x04) >> 2, mCmd[1] & 0x03, mCmd[2],
                      mCmd[3], mCmd[4], mCmd[5], mCmd[6], mCmd[7], mCmd[8]);
#endif

        mIO.MT = mCmd[0] & 0x80;
        mIO.MF = mCmd[0] & 0x40;
        mIO.SK = mCmd[0] & 0x20;
        mIO.US = mCmd[1] & 0x03;
        mIO.HD = (mCmd[1] & 0x04) >> 2;
        memcpy(&mIO.C, &mCmd[2], 5);  // C, H, R, N, EOT, GPL, DTL
        mIO.sectorLength = 128 << mIO.N;

        mBuffCount = 0;
        mBuffOffset = 0;

        memset(&mResult[0], 0, 7);

        if (mDrive[mIO.US].disk->isReady()) {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_NDM | SR_DIO | SR_CB;
        } else {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_DIO | SR_CB;
            mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT | ST0_NR;  // ST0
            mResult[1] = ST1_ND | ST1_MA;
            mResult[2] = ST2_MD;
        }

        rasieIRQ();
    }
}

uint8_t PD765C::readDataExecution(void) {
    if (mBuffCount == 0 || mBuffOffset >= mBuffCount) {
        mIO.cylinder = mDrive[mIO.US].cylinder;
        auto rc = mDrive[mIO.US].disk->readData(mBuffer, &mIO);
        if (rc > 0) {
            mBuffCount = rc;
            mBuffOffset = 0;
            mMainStatus = SR_NDM | SR_DIO | SR_CB;
            mIO.R++;
            if (mIO.R > mIO.EOT) {
                mIO.C++;
                mIO.R = 0;
            }
            mResult[0] = (mIO.HD << 2) | mIO.US;  // ST0
        } else {
            mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT;  // ST0
            mResult[1] = ST1_ND;
            resultPhase();
            rasieIRQ();
            return 0;
        }
    }
    auto result = mBuffer[mBuffOffset];
    mBuffOffset++;
    rasieIRQ();

    return result;
}

void PD765C::readDataResult(void) {
    memcpy(&mResult[3], &mIO.C, 4);  // C, H, R, H
    mResultCount = 7;
    mResultOffset = 0;
}

void PD765C::readDiagnostic(void) {
    if (mCmdCount > 8) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C Read diagnostic %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", mCmd[0], mCmd[1], mCmd[2], mCmd[3], mCmd[4],
                      mCmd[5], mCmd[6], mCmd[7], mCmd[8]);
#endif
        mIO.MT = mCmd[0] & 0x80;
        mIO.MF = mCmd[0] & 0x40;
        mIO.SK = mCmd[0] & 0x20;
        mIO.US = mCmd[1] & 0x03;
        mIO.HD = (mCmd[1] & 0x04) >> 2;
        memcpy(&mIO.C, &mCmd[2], 5);  // C, H, R, N, EOT, GPL, DTL
        mIO.sectorLength = 128 << mIO.N;

        mBuffCount = 0;
        mBuffOffset = 0;

        memset(&mResult[0], 0, 7);

        if (mDrive[mIO.US].disk->isReady()) {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_NDM | SR_DIO | SR_CB;
        } else {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_DIO | SR_CB;
            mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT | ST0_NR;  // ST0
            mResult[1] = ST1_ND | ST1_MA;
            mResult[2] = ST2_MD;
        }

        rasieIRQ();
    }
}

uint8_t PD765C::readDiagnosticExecution(void) {
    if (mBuffCount == 0 || mBuffOffset >= mBuffCount) {
        mIO.cylinder = mDrive[mIO.US].cylinder;
        auto rc = mDrive[mIO.US].disk->readDiagnostic(mBuffer, &mIO);
        if (rc > 0) {
            mBuffCount = rc;
            mBuffOffset = 0;
            mIO.R++;
            if (mIO.R > mIO.EOT) {
                mIO.C++;
                mIO.R = 0;
            }
            mResult[0] = (mIO.HD << 2) | mIO.US;  // ST0
        } else {
            mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT;  // ST0
            resultPhase();
            rasieIRQ();
            return 0;
        }
    }
    auto result = mBuffer[mBuffOffset];
    mBuffOffset++;
    rasieIRQ();

    return result;
}

void PD765C::readDiagnosticResult(void) {
    memcpy(&mResult[3], &mIO.C, 4);  // C, H, R, H
    mResultCount = 7;
    mResultOffset = 0;
}

void PD765C::readId(void) {
    if (mCmdCount > 1) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C Read ID MT:%d MF:%d SK:%d HD:%d US:%d\n", (mCmd[0] & 0x80) >> 7, (mCmd[0] & 0x40) >> 6, (mCmd[0] & 0x20) >> 5,
                      (mCmd[1] & 0x04) >> 2, mCmd[1] & 0x03);
#endif

        auto us = mCmd[1] & 0x03;
        auto hd = (mCmd[1] & 0x04) >> 2;

        mIO.MT = mCmd[0] & 0x80;
        mIO.MF = mCmd[0] & 0x40;
        mIO.SK = mCmd[0] & 0x20;
        mIO.US = mCmd[1] & 0x03;
        mIO.HD = (mCmd[1] & 0x04) >> 2;
        memcpy(&mIO.C, &mCmd[2], 5);  // C, H, R, N, EOT, GPL, DTL
        mIO.sectorLength = 128 << mIO.N;

        mBuffCount = 0;
        mBuffOffset = 0;

        memset(&mResult[0], 0, 7);

        if (mDrive[mIO.US].disk->isReady()) {
            auto rc = mDrive[us].disk->readID(&mIO.C, &mIO);
            if (rc > 0) {
                mPhase = EXECUTION_PHASE;
                mMainStatus = SR_NDM | SR_DIO | SR_CB;
                mResult[0] = (mIO.HD << 2) | mIO.US;  // ST0
            } else {
                mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT;  // ST0
                mResult[1] = ST1_ND;
            }
        } else {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_DIO | SR_CB;
            mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT | ST0_NR;  // ST0
            mResult[1] = ST1_ND | ST1_MA;
            mResult[2] = ST2_MD;
        }

        resultPhase();
        rasieIRQ();
    }
}

void PD765C::readIdResult(void) {
    memcpy(&mResult[3], &mIO.C, 4);  // C, H, R, H
    mResultCount = 7;
    mResultOffset = 0;
}

void PD765C::writeData(void) {
    if (mCmdCount > 8) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C WRITE DATA - MT:%d MF:%d SK:%d HD:%d US:%d C:%02x H:%02x R:%02x N:%02x EOT:%02x GPL:%02x DTL:%02x\n",
                      (mCmd[0] & 0x80) >> 7, (mCmd[0] & 0x40) >> 6, (mCmd[0] & 0x20) >> 5, (mCmd[1] & 0x04) >> 2, mCmd[1] & 0x03, mCmd[2],
                      mCmd[3], mCmd[4], mCmd[5], mCmd[6], mCmd[7], mCmd[8]);
#endif

        mIO.MT = mCmd[0] & 0x80;
        mIO.MF = mCmd[0] & 0x40;
        mIO.SK = mCmd[0] & 0x20;
        mIO.US = mCmd[1] & 0x03;
        mIO.HD = (mCmd[1] & 0x04) >> 2;
        memcpy(&mIO.C, &mCmd[2], 5);  // C, H, R, N, EOT, GPL, DTL
        mIO.sectorLength = 128 << mIO.N;

        mBuffCount = mIO.sectorLength;
        mBuffOffset = 0;

        memset(&mResult[0], 0, 7);

        if (mDrive[mIO.US].disk->isReady()) {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_NDM | SR_CB;
            if (mDrive[mIO.US].disk->isWriteProtect()) {
                mPhase = EXECUTION_PHASE;
                mMainStatus = SR_DIO | SR_CB;
                mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT;  // ST0
                mResult[1] = ST1_NW;
            }
        } else {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_DIO | SR_CB;
            mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT | ST0_NR;  // ST0
            mResult[1] = ST1_ND | ST1_MA;
            mResult[2] = ST2_MD;
        }

        rasieIRQ();
    }
}

void PD765C::writeDataExecution(uint8_t value) {
    mBuffer[mBuffOffset] = value;
    mBuffOffset++;
    if (mBuffOffset < mBuffCount) {
        mMainStatus = SR_NDM | SR_CB;
    } else {
#ifdef DEBUG_PD765C
        Serial.printf("writeDataExecution: write: %d\n", mIO.sectorLength);
#endif
        mIO.cylinder = mDrive[mIO.US].cylinder;
        auto rc = mDrive[mIO.US].disk->writeData(mBuffer, &mIO);
        mIO.R++;
        if (mIO.R > mIO.EOT) {
            mIO.C++;
            mIO.R = 0;
        }
        mBuffOffset = 0;
        mResult[0] = (mIO.HD << 2) | mIO.US;  // ST0

        if (rc > 0) {
            mResult[1] = 0;
            mResult[2] = 0;
        } else {
            switch (rc) {
                case D88_IO_ERROR:
                    mResult[0] |= ST0_AT;
                    mResult[1] = ST1_MA;
                    break;
                case D88_NO_READY:
                    mResult[0] |= ST0_AT | ST0_NR;
                    break;
                case D88_WRITE_PROTECT:
                    mResult[0] |= ST0_AT;
                    mResult[1] = ST1_NW;
                    break;
                case D88_SECTOR_NOT_FOUND:
                    mResult[0] |= ST0_AT;
                    mResult[1] = ST1_ND | ST1_MA;
                    mResult[2] = ST2_MD;
                    break;
                default:
                    mResult[0] |= ST0_AT;
                    mResult[1] = ST1_MA;
                    break;
            }
        }
        resultPhase();
    }
    rasieIRQ();
}

void PD765C::writeDataResult(void) {
    mResult[3] = mCmd[2];  // C
    mResult[4] = mCmd[3];  // H
    mResult[5] = mCmd[4];  // R
    mResult[6] = mCmd[5];  // N
    mResultCount = 7;
    mResultOffset = 0;
}

void PD765C::writeId(void) {
    if (mCmdCount > 5) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C Write ID %02x %02x %02x %02x %02x %02x\n", mCmd[0], mCmd[1], mCmd[2], mCmd[3], mCmd[4], mCmd[5]);
#endif
        mWriteID.MF = mCmd[0] & 0x40;
        mWriteID.US = mCmd[1] & 0x03;
        mWriteID.HD = (mCmd[1] & 0x04) >> 2;
        mWriteID.N = mCmd[2];
        mWriteID.SC = mCmd[3];
        mWriteID.GPL = mCmd[4];
        mWriteID.DataPattern = mCmd[5];

        mBuffCount = 4 * mWriteID.SC;
        mBuffOffset = 0;

        memset(&mResult[0], 0, 7);

        if (mDrive[mWriteID.US].disk->isReady()) {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_NDM | SR_CB;
            if (mDrive[mIO.US].disk->isWriteProtect()) {
                mPhase = EXECUTION_PHASE;
                mMainStatus = SR_DIO | SR_CB;
                mResult[0] = ((mIO.HD << 2) | mIO.US) | ST0_AT;  // ST0
                mResult[1] = ST1_NW;
            }
        } else {
            mPhase = EXECUTION_PHASE;
            mMainStatus = SR_DIO | SR_CB;
            mResult[0] = ((mWriteID.HD << 2) | mWriteID.US) | ST0_AT | ST0_NR;  // ST0
            mResult[1] = ST1_ND | ST1_MA;
            mResult[2] = ST2_MD;
        }
        rasieIRQ();
    }
}
void PD765C::writeIdExecution(uint8_t value) {
    auto p = &mWriteID.id[0].C + mBuffOffset;
    *p = value;
    mBuffOffset++;
    if (mBuffOffset < mBuffCount) {
        mMainStatus = SR_NDM | SR_CB;
    } else {
        mWriteID.cylinder = mDrive[mWriteID.US].cylinder;
        auto rc = mDrive[mWriteID.US].disk->writeID(&mWriteID);
        mResult[0] = ((mWriteID.HD << 2) | mWriteID.US);  // ST0
        if (rc > 0) {
            mResult[1] = 0;
            mResult[2] = 0;
        } else {
            switch (rc) {
                case D88_IO_ERROR:
                    mResult[0] |= ST0_AT;
                    mResult[1] = ST1_MA;
                    break;
                case D88_NO_READY:
                    mResult[0] |= ST0_AT | ST0_NR;
                    break;
                case D88_WRITE_PROTECT:
                    mResult[0] |= ST0_AT;
                    mResult[1] = ST1_NW;
                    break;
                default:
                    mResult[0] |= ST0_AT;
                    mResult[1] = ST1_MA;
                    break;
            }
        }
        resultPhase();
    }
    rasieIRQ();
}

void PD765C::writeIdResult(void) {
    mResultCount = 7;
    mResultOffset = 0;
}

void PD765C::specify(void) {
    if (mCmdCount > 2) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C Specify %02x %02x %02x\n", mCmd[0], mCmd[1], mCmd[2]);
#endif
        cmdComplete();
    }
}

void PD765C::seek(void) {
    if (mCmdCount > 2) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C Seek (%02x %02x %02x) US: %d NCN: %02x\n", mCmd[0], mCmd[1], mCmd[2], mCmd[1] & 0x3, mCmd[2]);
#endif
        mUS = mCmd[1] & 0x3;
        mDrive[mUS].cylinder = mCmd[2];
        mResult[0] = ST0_SE | mUS;
        if (!mDrive[mUS].disk->isReady()) {
            mResult[0] |= ST0_AT;
            mResult[0] |= ST0_NR;
        }
        mResult[1] = mCmd[2];
        mResultOffset = 0;
        mResultCount = 2;
        cmdComplete();
        rasieIRQ();
    }
}

void PD765C::recalibrate(void) {
    if (mCmdCount > 1) {
#ifdef DEBUG_PD765C
        Serial.printf("PD765C Recalibrate (%02x %02x) US: %d\n", mCmd[0], mCmd[1], mCmd[1] & 0x3);
#endif
        mUS = mCmd[1] & 0x3;
        mResult[0] = ST0_SE | mUS;
        mResult[1] = 0;
        mResultOffset = 0;
        mResultCount = 2;
        cmdComplete();
        rasieIRQ();
    }
}

void PD765C::senseInterruptStatus(void) {
    if (mCmdCount > 0) {
#ifdef DEBUG_PD765C
        Serial.printf("Sense interrupt status %02x\n", mCmd[0]);
#endif
        resultPhase();
    }
}

void PD765C::senseDeviceStatus(void) {
    if (mCmdCount > 1) {
        mUS = mCmd[1] & 0x03;
        mResult[0] = mCmd[1] & 0x07;
        mResult[0] |= ST3_TS;

        if (mDrive[mUS].disk->isReady()) {
            mResult[0] |= ST3_RY;
            if (mDrive[mUS].disk->isWriteProtect()) {
                mResult[0] |= ST3_WP;
            }
            if (mDrive[mUS].cylinder == 0) {
                mResult[0] |= ST3_T0;
            }
        }

#ifdef DEBUG_PD765C
        Serial.printf("Sense device status (%02x %02x) %d %d Result: %02x\n", mCmd[0], mCmd[1], (mCmd[1] & 0x04) >> 2, mCmd[1] & 0x03,
                      mResult[0]);
#endif
        mResultCount = 1;
        mResultOffset = 0;
        resultPhase();
    }
}

int PD765C::openDrive(int drive, char *fileName) { return mDrive[drive].disk->open(fileName); }

int PD765C::closeDrive(int drive) { return mDrive[drive].disk->close(); }

void PD765C::eject(void) {
    for (int i = 0; i < MAX_DRIVE; i++) {
        mDrive[i].disk->close();
    }
}
