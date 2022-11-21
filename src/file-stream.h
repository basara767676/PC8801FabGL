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

#include "Stream.h"

class FileStream : public Stream {
   public:
    FileStream() { mFile = nullptr; }
    ~FileStream() {
        if (!mFile) fclose(mFile);
    }
    int peek() {
        if (!mFile) return 0;
        int byte;
        auto pos = ftell(mFile);
        size_t result = fread(&byte, 1, 1, mFile);
        fseek(mFile, pos, SEEK_SET);
        return byte;
    }
    size_t readBytes(char *buffer, size_t length) {
        if (!mFile) return 0;
        return fread(buffer, 1, length, mFile);
    }
    size_t open(const char *fileName) {
        mFile = fopen(fileName, "rb");
        if (!mFile) return -1;
        fseek(mFile, 0, SEEK_END);
        auto size = ftell(mFile);
        fseek(mFile, 0, SEEK_SET);
        return size;
    }
    size_t write(uint8_t) { return 0; }
    int available() { return 0; }
    int read() { return 0; }
    void flush() {}

   private:
    FILE *mFile;
};
