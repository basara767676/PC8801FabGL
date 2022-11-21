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

//  Board information
//    Arduino board:    esp32 by Espressif Systems version 2.0.5
//    Library:          FabGL by Fabrizio Di Vittorio version v1.0.9
//    Board:            ESP32 Dev Module
//    Partition Scheme: Hug APP (3MB No OTA/1MB SPIFFS)
//    PSRAM:            Enabled

#include "src/pc88vm.h"

PC88VM *vm;

void setup() {
#ifdef DEBUG_PC88
    Serial.begin(115200);
    delay(500);
    Serial.write("\n\n\n");
#endif

    vm = new PC88VM;
    vm->run();
}

void loop() { vm->subTask(); }
