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

#include <stdio.h>

#include <cstring>
#include <cstdint>

#define D88_IO_ERROR (-1)
#define D88_NO_READY (-2)
#define D88_WRITE_PROTECT (-3)
#define D88_SECTOR_NOT_FOUND (-4)

typedef struct {
    bool MT;
    bool MF;
    bool SK;
    int HD;
    int US;
    int sectorLength;
    int cylinder;
    uint8_t C;
    uint8_t H;
    uint8_t R;
    uint8_t N;
    uint8_t EOT;
    uint8_t GPL;
    uint8_t DTL;
} d88_io_parameter_t;

typedef struct {
    bool MF;
    int HD;
    int US;
    int N;
    int SC;
    int GPL;
    uint8_t DataPattern;
    int cylinder;
    struct {
        uint8_t C;
        uint8_t H;
        uint8_t R;
        uint8_t N;
    } id[20];
} d88_write_id_t;

typedef struct {
    char name[17];
    uint8_t reserve[9];
    uint8_t writeProtect;
    uint8_t diskType;
    uint32_t diskSize;
    uint32_t track[164];
} d88_header_t;

typedef struct {
    uint8_t c;
    uint8_t h;
    uint8_t r;
    uint8_t n;
} d88_geometry_t;

typedef struct {
    d88_geometry_t geometry;
    uint16_t numberOfSector;
    uint8_t density;
    uint8_t deletedMark;
    uint8_t status;
    uint8_t reserve[5];
    uint16_t sizeOfData;
} d88_sector_header_t;

typedef struct {
    int offset;
    uint32_t size;
    uint8_t* buff;
} d88_track_t;

typedef struct {
    uint8_t gap0[80];
    uint8_t sync[12];
    uint32_t indexMark;
    uint8_t gap1[50];
} d88_disk_preamble_t;

typedef struct {
    uint8_t sync[12];
    uint32_t am1;
    d88_geometry_t geometry;
    uint16_t crc;
    uint8_t gap2[22];
} d88_disk_id_field_t;

typedef struct {
    uint8_t sync[12];
    uint32_t am2;
    uint8_t data[256];
    uint16_t crc;
    uint8_t gap3[22];
} d88_disk_data_field_t;

typedef struct {
    uint8_t gap4[22];
} d88_disk_postamble_t;

class PC88D88 {
   public:
    PC88D88();
    ~PC88D88();

    int open(const char* fileName);
    int close(void);

    int readData(uint8_t* dest, d88_io_parameter_t* ioParam);
    int readDiagnostic(uint8_t* dest, d88_io_parameter_t* ioParam);
    int readID(uint8_t* dest, d88_io_parameter_t* ioParam);

    int writeData(uint8_t* src, d88_io_parameter_t* ioParam);
    int writeID(d88_write_id_t* ioParam);

    bool isReady(void);
    bool isWriteProtect(void);

    static bool exists(const char* fileName);

    uint8_t* getTrackBuffer(int trackNo);

   private:
    int mType;
    FILE* mFP;
    d88_header_t* mHeader;
    d88_track_t* mTrack;
    long mDiskSize;
    int mMaxTrack;
    int mNextSector;
    bool mWriteProtect;
};
