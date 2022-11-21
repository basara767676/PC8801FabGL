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

#include "fabgl.h"

class PC88ERROR {
   public:
    PC88ERROR();
    ~PC88ERROR();

    static fabgl::VGADirectController *mDisplayController;
    static void setDisplayController(fabgl::VGADirectController *displayController) { mDisplayController = displayController; }
    static void dialog(const char *fmt, ...) {
        char str[100];

        if (mDisplayController) {
            mDisplayController->end();
        }

        va_list arg_ptr;

        va_start(arg_ptr, fmt);
        vsprintf(str, fmt, arg_ptr);
        va_end(arg_ptr);

        fabgl::InputBox ib;
        ib.begin(VGA_640x480_60Hz, 500, 400, 4);
        ib.setBackgroundColor(RGB888(0, 0, 0));
        ib.message("Error", str, nullptr, nullptr);
    }
};

fabgl::VGADirectController *PC88ERROR::mDisplayController = nullptr;