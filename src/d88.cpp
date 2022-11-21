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

#include "d88.h"

#include <Arduino.h>
#include <sys/stat.h>

#ifdef DEBUG_PC88
// #define DEBUG_D88
#endif

#define DISK_TYPE_UNKNOWN (0x00)
#define DISK_TYPE_D88 (0x01)
#define DISK_TYPE_2W (0x02)

#define DISK_2D (0x00)
#define DISK_2DD (0x10)
#define DISK_2HD (0x20)

PC88D88::PC88D88() {
    mType = DISK_TYPE_UNKNOWN;
    mFP = nullptr;
    mHeader = nullptr;
    mTrack = nullptr;
    mWriteProtect = false;

    mDiskSize = 0;
    mMaxTrack = 0;
}

PC88D88::~PC88D88() {}

int PC88D88::open(const char* fileName) {
    if (mFP != nullptr) {
        close();
    }

    const char* ext = strrchr(fileName, '.');

    if (ext == nullptr) return -1;

    if (!strcasecmp(ext, ".D88")) {
        struct stat fileStat;
        if (stat(fileName, &fileStat) == -1) {
            return -1;
        }

        mHeader = (d88_header_t*)ps_malloc(sizeof(d88_header_t));

        mFP = fopen(fileName, "rb+");
        if (!mFP) {
#ifdef DEBUG_D88
            Serial.printf("Open error: %s\n", fileName);
#endif
            return -1;
        }

        mDiskSize = fileStat.st_size;

        fseek(mFP, 0, SEEK_SET);
        size_t result = fread(mHeader, 1, sizeof(d88_header_t), mFP);

        if (result != sizeof(d88_header_t)) {
            close();
            return -1;
        }

#ifdef DEBUG_D88
        Serial.println(mHeader->name);
#endif
        if (mDiskSize != mHeader->diskSize) {
#ifdef DEBUG_D88
            Serial.printf("disk size error %d %d", mDiskSize, mHeader->diskSize);
#endif
            close();
            return -1;
        }

        if (mHeader->diskType != DISK_2D) return -1;
        mMaxTrack = 84;

        mWriteProtect = mHeader->writeProtect != 0x00;

        if (mWriteProtect) {
            fclose(mFP);
            mFP = fopen(fileName, "rb");
            if (!mFP) {
#ifdef DEBUG_D88
                Serial.printf("Open error: %s\n", fileName);
#endif
                return -1;
            }
#ifdef DEBUG_D88
            Serial.printf("Reopened %s as read-only because it is write-protected.\n", fileName);
#endif
        }

        mTrack = (d88_track_t*)ps_malloc(sizeof(d88_track_t) * mMaxTrack);
        if (mTrack == nullptr) {
            close();
            return -1;
        }

#ifdef DEBUG_D88
        Serial.printf("maxTrack: %d diskSize: %04x\n", mMaxTrack, mHeader->diskSize);
#endif

        for (int i = 0; i < mMaxTrack; i++) {
            mTrack[i].buff = nullptr;
            if (mHeader->track[i] > 0) {
                mTrack[i].offset = mHeader->track[i];
                auto nextOffset = mHeader->diskSize;
                for (int j = i + 1; j < mMaxTrack; j++) {
                    if (mHeader->track[j] > 0) {
                        nextOffset = mHeader->track[j];
                        break;
                    }
                }
                mTrack[i].size = nextOffset - mTrack[i].offset;
#ifdef DEBUG_D88
                Serial.printf("track: %d offset: %04x size: %04x nextOffset: %04x\n", i, mTrack[i].offset, mTrack[i].size, nextOffset);
#endif
            } else {
                mTrack[i].offset = 0;
            }
        }

        mNextSector = 1;
        mType = DISK_TYPE_D88;
        return 0;
    } else {
        return -1;
    }
}

int PC88D88::close(void) {
    if (mFP != nullptr) {
        fclose(mFP);
        mFP = nullptr;
    }
    if (mHeader != nullptr) {
        free(mHeader);
        mHeader = nullptr;
    }
    if (mTrack != nullptr) {
        for (int i = 0; i < mMaxTrack; i++) {
            if (mTrack[i].buff != nullptr) {
                free(mTrack[i].buff);
                mTrack[i].buff = nullptr;
            }
        }
        free(mTrack);
        mTrack = nullptr;
    }
    return 0;
}

int PC88D88::readData(uint8_t* dest, d88_io_parameter_t* ioParam) {
    auto trackNo = ioParam->cylinder * 2 + ioParam->HD;
    if (getTrackBuffer(trackNo) == nullptr) {
        return -1;
    }
    auto track = &mTrack[trackNo];

    auto buf = track->buff;
    auto header = (d88_sector_header_t*)buf;
    for (int i = 1; i <= header->numberOfSector; i++) {
        header = (d88_sector_header_t*)buf;
        if (memcmp(&header->geometry, &ioParam->C, 4) == 0) {
            memcpy(dest, buf + sizeof(d88_sector_header_t), header->sizeOfData);
            return header->sizeOfData;
        }
        buf += sizeof(d88_sector_header_t) + header->sizeOfData;
    }
#ifdef DEBUG_D88
    Serial.printf("D88: sector not found");
#endif
    return -1;
}

int PC88D88::readDiagnostic(uint8_t* dest, d88_io_parameter_t* ioParam) {
    auto trackNo = ioParam->cylinder * 2 + ioParam->HD;
    auto buf = getTrackBuffer(trackNo);
    if (buf != nullptr) {
        return -1;
    }

    auto offset = 0;

    // Preamble
    auto pre = (d88_disk_preamble_t*)dest;
    memset(&pre->gap0[0], 0x4e, 80);
    memset(&pre->sync[0], 0, 12);
    pre->indexMark = 0xfcc2c2c2;
    memset(&pre->gap0[0], 0x4e, 50);

    dest += sizeof(d88_disk_preamble_t);
    auto header = (d88_sector_header_t*)buf;
    auto numberOfSector = header->numberOfSector;
    for (int i = 1; i <= numberOfSector; i++) {
        // ID field
        auto idField = (d88_disk_id_field_t*)dest;
        memset(&idField->sync[0], 0, 12);
        idField->am1 = 0xfea1a1a1;
        memcpy(&idField->geometry, &header->geometry, sizeof(d88_geometry_t));
        idField->crc = 0xffff;
        memset(&idField->gap2[0], 0, 22);
        dest += sizeof(d88_disk_id_field_t);

        // Data field
        auto dataField = (d88_disk_data_field_t*)dest;
        memset(&dataField->sync[0], 0, 12);
        dataField->am2 = 0xfba1a1a1;
        memcpy(&dataField->data[0], buf + sizeof(d88_sector_header_t), header->sizeOfData);
        dataField->crc = 0xffff;
        memset(&dataField->gap3[0], 0x4e, 22);
        dest += sizeof(d88_disk_data_field_t);

        buf += sizeof(d88_sector_header_t) + header->sizeOfData;
        header = (d88_sector_header_t*)buf;
    }
    // Postamble
    auto postamble = (d88_disk_postamble_t*)dest;
    memset(&postamble->gap4[0], 0x4e, 22);

    auto size =
        sizeof(d88_disk_preamble_t) + (sizeof(d88_disk_id_field_t) + sizeof(d88_disk_data_field_t)) * 16 + sizeof(d88_disk_postamble_t);

#ifdef DEBUG_D88
    Serial.printf("D88: %04x\n", size);
#endif
    return size;
}

int PC88D88::readID(uint8_t* dest, d88_io_parameter_t* ioParam) {
    auto trackNo = ioParam->cylinder * 2 + ioParam->HD;
    auto buf = getTrackBuffer(trackNo);
    if (buf == nullptr) {
        return -1;
    }
    auto header = (d88_sector_header_t*)buf;
    auto numberOfSector = header->numberOfSector;

    if (mNextSector > numberOfSector) {
        mNextSector = 1;
    }
#ifdef DEBUG_D88
    Serial.printf("Read id: %d\n", mNextSector);
#endif

    for (int i = 1; i <= numberOfSector; i++) {
        if (mNextSector == i) {
            memcpy(dest, &header->geometry, sizeof(d88_geometry_t));
            mNextSector++;
            return 4;
        }

        buf += sizeof(d88_sector_header_t) + header->sizeOfData;
        header = (d88_sector_header_t*)buf;
    }
    return -1;
}

int PC88D88::writeData(uint8_t* src, d88_io_parameter_t* ioParam) {
    if (!isReady()) {
        return D88_NO_READY;
    }
    if (isWriteProtect()) {
        return D88_WRITE_PROTECT;
    }
    auto trackNo = ioParam->cylinder * 2 + ioParam->HD;
    if (getTrackBuffer(trackNo) == nullptr) {
        return D88_IO_ERROR;
    }
    auto track = &mTrack[trackNo];

#ifdef DEBUG_D88
    Serial.printf("PC88D88::writeData: offset %08x\n", track->offset);
#endif

    auto buff = track->buff;
    auto header = (d88_sector_header_t*)buff;

    for (int i = 1; i <= header->numberOfSector; i++) {
        header = (d88_sector_header_t*)buff;
        if (memcmp(&header->geometry, &ioParam->C, 4) == 0) {
#ifdef DEBUG_D88
            Serial.printf("write Data: %02x %02x %02x %02x %03x\n", ioParam->C, ioParam->H, ioParam->R, ioParam->N, header->sizeOfData);
#endif
            memcpy(buff + sizeof(d88_sector_header_t), src, header->sizeOfData);
            fseek(mFP, track->offset + buff + sizeof(d88_sector_header_t) - track->buff, SEEK_SET);
            size_t result = fwrite(src, 1, header->sizeOfData, mFP);
            if (result != header->sizeOfData) {
                return D88_IO_ERROR;
            }
#ifdef DEBUG_D88
            Serial.println("write data ok");
#endif
            return header->sizeOfData;
        }
        buff += sizeof(d88_sector_header_t) + header->sizeOfData;
    }
#ifdef DEBUG_D88
    Serial.printf("D88: sector not found\n");
#endif
    return D88_SECTOR_NOT_FOUND;
}

int PC88D88::writeID(d88_write_id_t* ioParam) {
    if (!isReady()) {
        return D88_NO_READY;
    }
    if (isWriteProtect()) {
        return D88_WRITE_PROTECT;
    }
    auto trackNo = ioParam->cylinder * 2 + ioParam->HD;
    if (getTrackBuffer(trackNo) == nullptr) {
        return D88_IO_ERROR;
    }
    auto track = &mTrack[trackNo];

    auto buf = track->buff;

    auto sectorSize = 128 << ioParam->N;
    d88_sector_header_t sectorHeader;
    memset(&sectorHeader, 0, sizeof(d88_sector_header_t));
    sectorHeader.numberOfSector = ioParam->SC;
    sectorHeader.sizeOfData = sectorSize;

    int offset = 0;

    for (int i = 0; i < ioParam->SC; i++) {
#ifdef DEBUG_D88
        Serial.printf("%02x %02x %02x %02x\n", ioParam->id[i].C, ioParam->id[i].H, ioParam->id[i].R, ioParam->id[i].N);
#endif
        memcpy(&sectorHeader.geometry, &ioParam->id[i].C, sizeof(d88_geometry_t));
        memcpy(buf + offset, &sectorHeader, sizeof(d88_sector_header_t));
        offset += sizeof(d88_sector_header_t);
        memset(buf + offset, ioParam->DataPattern, sectorSize);
        offset += sectorSize;
    }
    fseek(mFP, track->offset, SEEK_SET);
    size_t result = fwrite(buf, 1, offset, mFP);
    if (result != offset) {
        return D88_IO_ERROR;
    }
#ifdef DEBUG_D88
    Serial.printf("%08x %04x\n", track->offset, offset);
#endif
    return ioParam->SC;
}

uint8_t* PC88D88::getTrackBuffer(int trackNo) {
    auto track = &mTrack[trackNo];
    if (track->buff == nullptr) {
        track->buff = (uint8_t*)ps_malloc(track->size);
        if (track->buff == nullptr) {
#ifdef DEBUG_D88
            Serial.printf("readData - ps_malloc error %d\n", track->size);
#endif
            return nullptr;
        }
        fseek(mFP, track->offset, SEEK_SET);
        size_t result = fread(track->buff, 1, track->size, mFP);
        if (result != track->size) {
            return nullptr;
        }
#ifdef DEBUG_D88
        Serial.println("readData - read buff");
#endif
    }
    return track->buff;
}

bool PC88D88::isReady(void) { return mFP != nullptr; }

bool PC88D88::isWriteProtect(void) { return mWriteProtect; }

bool PC88D88::exists(const char* fileName) {
    auto f = fopen(fileName, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}
