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

#include "d88.h"

#define WAITING_PHASE (0)
#define COMMAND_PHASE (1)
#define EXECUTION_PHASE (2)
#define RESULT_PHASE (3)

#define READ_DATA (0x06)
#define READ_DELETED_DATA (0x0c)
#define READ_ID (0x0a)
#define READ_DIAGNOSTIC (0x02)
#define SCAN_EQUAL (0x11)
#define SCAN_LOW_OR_EQUAL (0x1d)
#define SCAN_HIGH_OR_EQUAL (0x19)
#define WRITE_DATA (0x05)
#define WRITE_DELETED_DATA (0x09)
#define WRITE_ID (0x0d)
#define RECALIBRATE (0x07)
#define SEEK (0x0f)
#define SENSE_INTERRUPT_STATUS (0x08)
#define SENSE_DEVICE_STATUS (0x04)
#define SPECIFY (0x03)

#define SR_D0B (0x01)  // FDD 0 Busy
#define SR_D1B (0x02)  // FDD 1 Busy
#define SR_D2B (0x04)  // FDD 2 Busy
#define SR_D3B (0x08)  // FDD 3 Busy
#define SR_CB (0x10)   // FDC Busy
#define SR_NDM (0x20)  // Execution Mode
#define SR_DIO (0x40)  // Data Input/Output
#define SR_RQM (0x80)  // Request for Master

// Status Register 0
#define ST0_INTERRUPT_CODE (0xC0)  // Interrupt code
#define ST0_NT (0x00)              // Interrupt code (NT)
#define ST0_AT (0x40)              // Interrupt code (AT)
#define ST0_IC (0x80)              // Interrupt code (IC)
#define ST0_AI (0xC0)              // Interrupt code (AI)
#define ST0_SE (0x20)              // Seek end
#define ST0_EC (0x10)              // Equipment check
#define ST0_NR (0x08)              // Not ready
#define ST0_HD (0x04)              // Head address
#define ST0_US1 (0x02)             // Unit select 1
#define ST0_US0 (0x01)             // Unit select 0

// Status Register 1
#define ST1_EN (0x80)  // End of cylinder
#define ST1_DE (0x20)  // Data error
#define ST1_OR (0x10)  // Over run
#define ST1_ND (0x04)  // No data
#define ST1_NW (0x02)  // Not writeable
#define ST1_MA (0x01)  // Missing address mark

// Result status 2
#define ST2_CM (0x40)  // Control mark
#define ST2_DD (0x20)  // Data error in data field
#define ST2_NC (0x10)  // Wrong cylinder
#define ST2_SH (0x08)  // Scan equal hit
#define ST2_SN (0x04)  // Scan not satisfied
#define ST2_BC (0x02)  // Bad cylinder
#define ST2_MD (0x01)  // Missing address mark in data field

// Result status 3
#define ST3_FT (0x80)   // Fault
#define ST3_WP (0x40)   // Write Protected
#define ST3_RY (0x20)   // Ready
#define ST3_T0 (0x10)   // Track 0
#define ST3_TS (0x08)   // Two side
#define ST3_HD (0x04)   // Head address
#define ST3_US1 (0x02)  // Unit select 1
#define ST3_US0 (0x01)  // Uint select 0

#define MAX_DRIVE (4)

typedef struct {
    bool motor;
    bool hasResult;
    uint8_t result;
    int cylinder;
    PC88D88 *disk;
} drive_status_t;

class PD765C {
   public:
    PD765C();
    ~PD765C();

    void writeF4(uint8_t value);
    void writeF7(uint8_t value);
    void writeF8(uint8_t value);
    void writeDataRegister(uint8_t value);

    uint8_t terminalCount(void);
    uint8_t readStatusRegister(void);
    uint8_t readDataRegister(void);

    void setIRQFlag(bool *irqFlag);

    int openDrive(int drive, char *fileName);
    int closeDrive(int drive);

    void eject(void);

   private:
    uint8_t mMainStatus;

    int mPhase;
    int mCmdCount;
    uint8_t mCmd[10];

    int mUS;

    d88_io_parameter_t mIO;
    d88_write_id_t mWriteID;

    uint8_t mResult[10];
    int mResultCount;
    int mResultOffset;

    uint8_t *mBuffer;
    int mBuffOffset;
    int mBuffCount;

    drive_status_t mDrive[MAX_DRIVE];

    int mExecCmd;

    bool *mIRQFlag;

    uint8_t mWritePrecompensation;
    uint8_t mVFO;

    void commandPhase(uint8_t value);
    uint8_t executionPhaseRead(void);
    void executionPhaseWrite(uint8_t value);
    void resultPhase(void);
    void cmdComplete(void);

    void readData(void);
    uint8_t readDataExecution(void);
    void readDataResult(void);

    void readDiagnostic(void);
    uint8_t readDiagnosticExecution(void);
    void readDiagnosticResult(void);

    void readId(void);
    void readIdResult(void);

    void writeData(void);
    void writeDataExecution(uint8_t value);
    void writeDataResult(void);

    void writeId(void);
    void writeIdExecution(uint8_t value);
    void writeIdResult(void);

    void specify(void);
    void seek(void);
    void recalibrate(void);
    void senseInterruptStatus(void);
    void senseDeviceStatus(void);

    void rasieIRQ(void);
};