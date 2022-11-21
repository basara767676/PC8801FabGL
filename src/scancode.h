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

#define NO___EFFECT 0
#define NORMAL__KEY 1
#define SPECIAL_KEY 2
#define WINDOWS_KEY 3
#define CTL_ALT_KEY 4
#define CPU___SPEED 5

keyTable_t PC88KeyBoard::scanCodeTable[512] = {
    {NO___EFFECT, 0, 0, ""},               // 00
    {SPECIAL_KEY, 0, 0, "F9"},             // 01 "F9"
    {NO___EFFECT, 0, 0, ""},               // 02
    {NORMAL__KEY, 0x09, 0x20, "F5"},       // 03 "F5"
    {NORMAL__KEY, 0x09, 0x08, "F3"},       // 04 "F3"
    {NORMAL__KEY, 0x09, 0x02, "F1"},       // 05 "F1"
    {NORMAL__KEY, 0x09, 0x04, "F2"},       // 06 "F2"
    {SPECIAL_KEY, 0, 0, "F12"},            // 07 "F12"
    {NO___EFFECT, 0, 0, ""},               // 08
    {SPECIAL_KEY, 0, 0, "F10"},            // 09 "F10"
    {NO___EFFECT, 0, 0, "F8"},             // 0A "F8"
    {NO___EFFECT, 0, 0, "F6"},             // 0B "F6"
    {NORMAL__KEY, 0x09, 0x10, "F4"},       // 0C "F4"
    {NORMAL__KEY, 0x0a, 0x01, "TAB"},      // 0D TAB
    {NORMAL__KEY, 0x09, 0x80, "HANKAKU"},  // 0E HANKAKU/ZENKAKU/KANJI
    {NO___EFFECT, 0, 0, ""},               // 0F

    {NORMAL__KEY, 0, 0, ""},                  // 10
    {SPECIAL_KEY, 0x08, 0x10, "LEFT ALT"},    // 11 Left ALT
    {SPECIAL_KEY, 0x08, 0x40, "LEFT SHIFT"},  // 12 Left SHIFT
    {SPECIAL_KEY, 0x08, 0x20, "KANA"},        // 13 KANA
    {SPECIAL_KEY, 0x08, 0x80, "LEFT CTRL"},   // 14 Left CTRL
    {NORMAL__KEY, 0x04, 0x02, "Q"},           // 15 "Q"
    {CPU___SPEED, 0x06, 0x02, "1"},           // 16 "1"
    {NO___EFFECT, 0, 0, ""},                  // 17
    {NO___EFFECT, 0, 0, ""},                  // 18
    {NO___EFFECT, 0, 0, ""},                  // 19
    {NORMAL__KEY, 0x05, 0x04, "Z"},           // 1A "Z"
    {NORMAL__KEY, 0x04, 0x08, "S"},           // 1B "S"
    {NORMAL__KEY, 0x02, 0x02, "A"},           // 1C "A"
    {NORMAL__KEY, 0x04, 0x80, "W"},           // 1D "W"
    {CPU___SPEED, 0x06, 0x04, "2"},           // 1E "2"
    {NO___EFFECT, 0, 0, ""},                  // 1F

    {NO___EFFECT, 0, 0, ""},             // 20
    {NORMAL__KEY, 0x02, 0x08, "C"},      // 21 "C"
    {NORMAL__KEY, 0x05, 0x01, "X"},      // 22 "X"
    {NORMAL__KEY, 0x02, 0x10, "D"},      // 23 "D"
    {NORMAL__KEY, 0x02, 0x20, "E"},      // 24 "E"
    {CPU___SPEED, 0x06, 0x10, "4"},      // 25 "4"
    {CPU___SPEED, 0x06, 0x08, "3"},      // 26 "3"
    {NO___EFFECT, 0, 0, ""},             // 27
    {NO___EFFECT, 0, 0, ""},             // 28
    {NORMAL__KEY, 0x09, 0x40, "SPACE"},  // 29 Space
    {NORMAL__KEY, 0x04, 0x40, "V"},      // 2A "V"
    {NORMAL__KEY, 0x02, 0x40, "F"},      // 2B "F"
    {NORMAL__KEY, 0x04, 0x10, "T"},      // 2C "T"
    {NORMAL__KEY, 0x04, 0x04, "R"},      // 2D "R"
    {CPU___SPEED, 0x06, 0x20, "5"},      // 2E "5"
    {NO___EFFECT, 0, 0, ""},             // 2F

    {NO___EFFECT, 0, 0, ""},         // 30
    {NORMAL__KEY, 0x03, 0x40, "N"},  // 31 "N"
    {NORMAL__KEY, 0x02, 0x04, "B"},  // 32 "B"
    {NORMAL__KEY, 0x03, 0x01, "H"},  // 33 "H"
    {NORMAL__KEY, 0x02, 0x80, "G"},  // 34 "G"
    {NORMAL__KEY, 0x05, 0x02, "Y"},  // 35 "Y"
    {CPU___SPEED, 0x06, 0x40, "6"},  // 36 "6"
    {NO___EFFECT, 0, 0, ""},         // 37
    {NO___EFFECT, 0, 0, ""},         // 38
    {NO___EFFECT, 0, 0, ""},         // 39
    {NORMAL__KEY, 0x03, 0x20, "M"},  // 3A "M"
    {NORMAL__KEY, 0x03, 0x04, "J"},  // 3B "J"
    {NORMAL__KEY, 0x04, 0x20, "U"},  // 3C "U"
    {CPU___SPEED, 0x06, 0x80, "7"},  // 3D "7"
    {CPU___SPEED, 0x07, 0x01, "8"},  // 3E "8"
    {NO___EFFECT, 0, 0, ""},         // 3F

    {NO___EFFECT, 0, 0, ""},         // 40
    {NORMAL__KEY, 0x07, 0x10, ","},  // 41 ","
    {NORMAL__KEY, 0x03, 0x08, "K"},  // 42 "K"
    {NORMAL__KEY, 0x03, 0x02, "I"},  // 43 "I"
    {NORMAL__KEY, 0x03, 0x80, "O"},  // 44 "O"
    {CPU___SPEED, 0x06, 0x01, "0"},  // 45 "0"
    {CPU___SPEED, 0x07, 0x02, "9"},  // 46 "9"
    {NO___EFFECT, 0, 0, ""},         // 47
    {NO___EFFECT, 0, 0, ""},         // 48
    {NORMAL__KEY, 0x07, 0x20, "."},  // 49 "."
    {NORMAL__KEY, 0x07, 0x40, "/"},  // 4A "/"
    {NORMAL__KEY, 0x03, 0x10, "L"},  // 4B "L"
    {NORMAL__KEY, 0x07, 0x08, ";"},  // 4C ";"
    {NORMAL__KEY, 0x04, 0x01, "P"},  // 4D "P"
    {NORMAL__KEY, 0x05, 0x80, "-"},  // 4E "-"
    {NO___EFFECT, 0, 0, ""},         // 4F

    {NO___EFFECT, 0, 0, ""},                   // 50
    {NORMAL__KEY, 0x07, 0x80, "\\"},           // 51 \ (0x5c)
    {NORMAL__KEY, 0x07, 0x04, ":"},            // 52 ":"
    {NORMAL__KEY, 0, 0, ""},                   // 53
    {NORMAL__KEY, 0x02, 0x01, "@"},            // 54 "@"
    {NORMAL__KEY, 0x05, 0x40, "^"},            // 55 "^"
    {NO___EFFECT, 0, 0, ""},                   // 56
    {NO___EFFECT, 0, 0, ""},                   // 57
    {SPECIAL_KEY, 0x0a, 0x80, "CAPS"},         // 58 CAPS
    {SPECIAL_KEY, 0x08, 0x40, "RIGHT SHIFT"},  // 59 Right SHIFT
    {NORMAL__KEY, 0x01, 0x80, "ENTER"},        // 5A Enter
    {NORMAL__KEY, 0x05, 0x08, "["},            // 5B "["
    {NO___EFFECT, 0, 0, ""},                   // 5C
    {NORMAL__KEY, 0x05, 0x20, "]"},            // 5D "]"
    {NO___EFFECT, 0, 0, ""},                   // 5E
    {NORMAL__KEY, 0, 0, ""},                   // 5F

    {NO___EFFECT, 0, 0, ""},                 // 60
    {NO___EFFECT, 0, 0, ""},                 // 61
    {NO___EFFECT, 0, 0, ""},                 // 62
    {NO___EFFECT, 0, 0, ""},                 // 63
    {NO___EFFECT, 0, 0, "HENKAN"},           // 64 HENKAN
    {NORMAL__KEY, 0, 0, ""},                 // 65
    {NORMAL__KEY, 0x08, 0x08, "Backspace"},  // 66 Backspace
    {NO___EFFECT, 0, 0, ""},                 // 67 MUHENKAN
    {NO___EFFECT, 0, 0, ""},                 // 68
    {NORMAL__KEY, 0, 0x02, ""},              // 69 PAD "1"
    {NORMAL__KEY, 0x05, 0x10, "\\"},         // 6A "\" (0x5c)
    {NORMAL__KEY, 0, 0x10, "PAD 4"},         // 6B PAD "4"
    {NORMAL__KEY, 0, 0x80, ""},              // 6C PAD "7"
    {NO___EFFECT, 0, 0, ""},                 // 6D
    {NO___EFFECT, 0, 0, ""},                 // 6E
    {NO___EFFECT, 0, 0, ""},                 // 6F

    {NORMAL__KEY, 0, 0x01, "PAD 0"},       // 70 PAD "0"
    {NORMAL__KEY, 0, 0, "PAD ."},          // 71 PAD ""."
    {NORMAL__KEY, 0, 0x04, "PAD 2"},       // 72 PAD "2"
    {NORMAL__KEY, 0, 0x20, "PAD 5"},       // 73 PAD "5"
    {NORMAL__KEY, 0, 0x40, "PAD 6"},       // 74 PAD "6"
    {NORMAL__KEY, 0x01, 0x01, "PAD 8"},    // 75 PAD "8"
    {NORMAL__KEY, 0x09, 0x01, "ESC"},      // 76 ESC  (STOP)
    {NORMAL__KEY, 0x08, 0x01, "Numlock"},  // 77 Numlock
    {NORMAL__KEY, 0x09, 0x02, "F1"},       // 78 F1
    {NORMAL__KEY, 0x01, 0x08, "PAD +"},    // 79 PAD "+"
    {NORMAL__KEY, 0, 0x08, "PAD 3"},       // 7A PAD "3"
    {NORMAL__KEY, 0x0a, 0x20, "PAD -"},    // 7B PAD "-"
    {NORMAL__KEY, 0x01, 0x04, "PAD *"},    // 7C PAD "*"
    {NORMAL__KEY, 0x01, 0x02, "PAD 9"},    // 7D PAD "9"
    {NORMAL__KEY, 0x0a, 0x10, "Scrlk"},    // 7E ScrLk
    {NO___EFFECT, 0, 0, ""},               // 7F

    {NO___EFFECT, 0, 0, ""},    // 80
    {NO___EFFECT, 0, 0, ""},    // 81
    {NO___EFFECT, 0, 0, ""},    // 82
    {NO___EFFECT, 0, 0, "F7"},  // 83 "F7"
    {NO___EFFECT, 0, 0, ""},    // 84
    {NO___EFFECT, 0, 0, ""},    // 85
    {NO___EFFECT, 0, 0, ""},    // 86
    {NO___EFFECT, 0, 0, ""},    // 87
    {NO___EFFECT, 0, 0, ""},    // 88
    {NO___EFFECT, 0, 0, ""},    // 89
    {NO___EFFECT, 0, 0, ""},    // 8A
    {NO___EFFECT, 0, 0, ""},    // 8B
    {NO___EFFECT, 0, 0, ""},    // 8C
    {NO___EFFECT, 0, 0, ""},    // 8D
    {NO___EFFECT, 0, 0, ""},    // 8E
    {NO___EFFECT, 0, 0, ""},    // 8F

    {NO___EFFECT, 0, 0, ""},  // 90
    {NO___EFFECT, 0, 0, ""},  // 91
    {NO___EFFECT, 0, 0, ""},  // 92
    {NO___EFFECT, 0, 0, ""},  // 93
    {NO___EFFECT, 0, 0, ""},  // 94
    {NO___EFFECT, 0, 0, ""},  // 95
    {NO___EFFECT, 0, 0, ""},  // 96
    {NO___EFFECT, 0, 0, ""},  // 97
    {NO___EFFECT, 0, 0, ""},  // 98
    {NO___EFFECT, 0, 0, ""},  // 99
    {NO___EFFECT, 0, 0, ""},  // 9A
    {NO___EFFECT, 0, 0, ""},  // 9B
    {NO___EFFECT, 0, 0, ""},  // 9C
    {NO___EFFECT, 0, 0, ""},  // 9D
    {NO___EFFECT, 0, 0, ""},  // 9E
    {NO___EFFECT, 0, 0, ""},  // 9F

    {NO___EFFECT, 0, 0, ""},  // A0
    {NO___EFFECT, 0, 0, ""},  // A1
    {NO___EFFECT, 0, 0, ""},  // A2
    {NO___EFFECT, 0, 0, ""},  // A3
    {NO___EFFECT, 0, 0, ""},  // A4
    {NO___EFFECT, 0, 0, ""},  // A5
    {NO___EFFECT, 0, 0, ""},  // A6
    {NO___EFFECT, 0, 0, ""},  // A7
    {NO___EFFECT, 0, 0, ""},  // A8
    {NO___EFFECT, 0, 0, ""},  // A9
    {NO___EFFECT, 0, 0, ""},  // AA
    {NO___EFFECT, 0, 0, ""},  // AB
    {NO___EFFECT, 0, 0, ""},  // AC
    {NO___EFFECT, 0, 0, ""},  // AD
    {NO___EFFECT, 0, 0, ""},  // AE
    {NO___EFFECT, 0, 0, ""},  // AF

    {NO___EFFECT, 0, 0, ""},  // B0
    {NO___EFFECT, 0, 0, ""},  // B1
    {NO___EFFECT, 0, 0, ""},  // B2
    {NO___EFFECT, 0, 0, ""},  // B3
    {NO___EFFECT, 0, 0, ""},  // B4
    {NO___EFFECT, 0, 0, ""},  // B5
    {NO___EFFECT, 0, 0, ""},  // B6
    {NO___EFFECT, 0, 0, ""},  // B7
    {NO___EFFECT, 0, 0, ""},  // B8
    {NO___EFFECT, 0, 0, ""},  // B9
    {NO___EFFECT, 0, 0, ""},  // BA
    {NO___EFFECT, 0, 0, ""},  // BB
    {NO___EFFECT, 0, 0, ""},  // BC
    {NO___EFFECT, 0, 0, ""},  // BD
    {NO___EFFECT, 0, 0, ""},  // BE
    {NO___EFFECT, 0, 0, ""},  // BF

    {NO___EFFECT, 0, 0, ""},  // C0
    {NO___EFFECT, 0, 0, ""},  // C1
    {NO___EFFECT, 0, 0, ""},  // C2
    {NO___EFFECT, 0, 0, ""},  // C3
    {NO___EFFECT, 0, 0, ""},  // C4
    {NO___EFFECT, 0, 0, ""},  // C5
    {NO___EFFECT, 0, 0, ""},  // C6
    {NO___EFFECT, 0, 0, ""},  // C7
    {NO___EFFECT, 0, 0, ""},  // C8
    {NO___EFFECT, 0, 0, ""},  // C9
    {NO___EFFECT, 0, 0, ""},  // CA
    {NO___EFFECT, 0, 0, ""},  // CB
    {NO___EFFECT, 0, 0, ""},  // CC
    {NO___EFFECT, 0, 0, ""},  // CD
    {NO___EFFECT, 0, 0, ""},  // CE
    {NO___EFFECT, 0, 0, ""},  // CF

    {NO___EFFECT, 0, 0, ""},  // D0
    {NO___EFFECT, 0, 0, ""},  // D1
    {NO___EFFECT, 0, 0, ""},  // D2
    {NO___EFFECT, 0, 0, ""},  // D3
    {NO___EFFECT, 0, 0, ""},  // D4
    {NO___EFFECT, 0, 0, ""},  // D5
    {NO___EFFECT, 0, 0, ""},  // D6
    {NO___EFFECT, 0, 0, ""},  // D7
    {NO___EFFECT, 0, 0, ""},  // D8
    {NO___EFFECT, 0, 0, ""},  // D9
    {NO___EFFECT, 0, 0, ""},  // DA
    {NO___EFFECT, 0, 0, ""},  // DB
    {NO___EFFECT, 0, 0, ""},  // DC
    {NO___EFFECT, 0, 0, ""},  // DD
    {NO___EFFECT, 0, 0, ""},  // DE
    {NO___EFFECT, 0, 0, ""},  // DF

    {NO___EFFECT, 0, 0, ""},  // E0
    {NO___EFFECT, 0, 0, ""},  // E1
    {NO___EFFECT, 0, 0, ""},  // E2
    {NO___EFFECT, 0, 0, ""},  // E3
    {NO___EFFECT, 0, 0, ""},  // E4
    {NO___EFFECT, 0, 0, ""},  // E5
    {NO___EFFECT, 0, 0, ""},  // E6
    {NO___EFFECT, 0, 0, ""},  // E7
    {NO___EFFECT, 0, 0, ""},  // E8
    {NO___EFFECT, 0, 0, ""},  // E9
    {NO___EFFECT, 0, 0, ""},  // EA
    {NO___EFFECT, 0, 0, ""},  // EB
    {NO___EFFECT, 0, 0, ""},  // EC
    {NO___EFFECT, 0, 0, ""},  // ED
    {NO___EFFECT, 0, 0, ""},  // EE
    {NO___EFFECT, 0, 0, ""},  // EF

    {NO___EFFECT, 0, 0, ""},  // F0
    {NO___EFFECT, 0, 0, ""},  // F1
    {NO___EFFECT, 0, 0, ""},  // F2
    {NO___EFFECT, 0, 0, ""},  // F3
    {NO___EFFECT, 0, 0, ""},  // F4
    {NO___EFFECT, 0, 0, ""},  // F5
    {NO___EFFECT, 0, 0, ""},  // F6
    {NO___EFFECT, 0, 0, ""},  // F7
    {NO___EFFECT, 0, 0, ""},  // F8
    {NO___EFFECT, 0, 0, ""},  // F9
    {NO___EFFECT, 0, 0, ""},  // FA
    {NO___EFFECT, 0, 0, ""},  // FB
    {NO___EFFECT, 0, 0, ""},  // FC
    {NO___EFFECT, 0, 0, ""},  // FD
    {NO___EFFECT, 0, 0, ""},  // FE
    {NO___EFFECT, 0, 0, ""},  // FF
    {NO___EFFECT, 0, 0, ""},  // 100
    {NO___EFFECT, 0, 0, ""},  // 101
    {NO___EFFECT, 0, 0, ""},  // 102
    {NO___EFFECT, 0, 0, ""},  // 103
    {NO___EFFECT, 0, 0, ""},  // 104
    {NO___EFFECT, 0, 0, ""},  // 105
    {NO___EFFECT, 0, 0, ""},  // 106
    {NO___EFFECT, 0, 0, ""},  // 107
    {NO___EFFECT, 0, 0, ""},  // 108
    {NO___EFFECT, 0, 0, ""},  // 109
    {NO___EFFECT, 0, 0, ""},  // 10A
    {NO___EFFECT, 0, 0, ""},  // 10B
    {NO___EFFECT, 0, 0, ""},  // 10C
    {NO___EFFECT, 0, 0, ""},  // 10D
    {NO___EFFECT, 0, 0, ""},  // 10E
    {NO___EFFECT, 0, 0, ""},  // 10F

    {NO___EFFECT, 0, 0, ""},                  // 110
    {SPECIAL_KEY, 0x08, 0x10, "RIGHT ALT"},   // 111 Right ALT
    {NO___EFFECT, 0, 0, ""},                  // 112
    {NO___EFFECT, 0, 0, ""},                  // 113
    {SPECIAL_KEY, 0x08, 0x80, "RIGHT CTRL"},  // 114 Right CTRL
    {NO___EFFECT, 0, 0, ""},                  // 115
    {NO___EFFECT, 0, 0, ""},                  // 116
    {NO___EFFECT, 0, 0, ""},                  // 117
    {NO___EFFECT, 0, 0, ""},                  // 118
    {NO___EFFECT, 0, 0, ""},                  // 119
    {NO___EFFECT, 0, 0, ""},                  // 11A
    {NO___EFFECT, 0, 0, ""},                  // 11B
    {NO___EFFECT, 0, 0, ""},                  // 11C
    {NO___EFFECT, 0, 0, ""},                  // 11D
    {NO___EFFECT, 0, 0, ""},                  // 11E
    {SPECIAL_KEY, 0, 0, "LEFT WIN"},          // 11F Left WIN

    {NO___EFFECT, 0, 0, ""},             // 120
    {NO___EFFECT, 0, 0, ""},             // 121
    {NO___EFFECT, 0, 0, ""},             // 122
    {NO___EFFECT, 0, 0, ""},             // 123
    {NO___EFFECT, 0, 0, ""},             // 124
    {NO___EFFECT, 0, 0, ""},             // 125
    {NO___EFFECT, 0, 0, ""},             // 126
    {SPECIAL_KEY, 0, 0, "RIGHT WIN"},    // 127 Right WIN
    {NO___EFFECT, 0, 0, ""},             // 128
    {NO___EFFECT, 0, 0, ""},             // 129
    {NO___EFFECT, 0, 0, ""},             // 12A
    {NO___EFFECT, 0, 0, ""},             // 12B
    {NO___EFFECT, 0, 0, ""},             // 12C
    {NO___EFFECT, 0, 0, ""},             // 12D
    {NO___EFFECT, 0, 0, ""},             // 12E
    {NO___EFFECT, 0, 0, "APPLICATION"},  // 12F Application

    {NO___EFFECT, 0, 0, ""},  // 130
    {NO___EFFECT, 0, 0, ""},  // 131
    {NO___EFFECT, 0, 0, ""},  // 132
    {NO___EFFECT, 0, 0, ""},  // 133
    {NO___EFFECT, 0, 0, ""},  // 134
    {NO___EFFECT, 0, 0, ""},  // 135
    {NO___EFFECT, 0, 0, ""},  // 136
    {NO___EFFECT, 0, 0, ""},  // 137
    {NO___EFFECT, 0, 0, ""},  // 138
    {NO___EFFECT, 0, 0, ""},  // 139
    {NO___EFFECT, 0, 0, ""},  // 13A
    {NO___EFFECT, 0, 0, ""},  // 13B
    {NO___EFFECT, 0, 0, ""},  // 13C
    {NO___EFFECT, 0, 0, ""},  // 13D
    {NO___EFFECT, 0, 0, ""},  // 13E
    {NO___EFFECT, 0, 0, ""},  // 13F

    {NO___EFFECT, 0, 0, ""},             // 140
    {NO___EFFECT, 0, 0, ""},             // 141
    {NO___EFFECT, 0, 0, ""},             // 142
    {NO___EFFECT, 0, 0, ""},             // 143
    {NO___EFFECT, 0, 0, ""},             // 144
    {NO___EFFECT, 0, 0, ""},             // 145
    {NO___EFFECT, 0, 0, ""},             // 146
    {NO___EFFECT, 0, 0, ""},             // 147
    {NO___EFFECT, 0, 0, ""},             // 148
    {NO___EFFECT, 0, 0, ""},             // 149
    {NORMAL__KEY, 0x0a, 0x40, "PAD /"},  // 14A PAD "/"
    {NO___EFFECT, 0, 0, ""},             // 14B
    {NO___EFFECT, 0, 0, ""},             // 14C
    {NO___EFFECT, 0, 0, ""},             // 14D
    {NO___EFFECT, 0, 0, ""},             // 14E
    {NO___EFFECT, 0, 0, ""},             // 14F

    {NO___EFFECT, 0, 0, ""},                 // 150
    {NO___EFFECT, 0, 0, ""},                 // 151
    {NO___EFFECT, 0, 0, ""},                 // 152
    {NO___EFFECT, 0, 0, ""},                 // 153
    {NO___EFFECT, 0, 0, ""},                 // 154
    {NO___EFFECT, 0, 0, ""},                 // 155
    {NO___EFFECT, 0, 0, ""},                 // 156
    {NO___EFFECT, 0, 0, ""},                 // 157
    {NO___EFFECT, 0, 0, ""},                 // 158
    {NO___EFFECT, 0, 0, ""},                 // 159
    {SPECIAL_KEY, 0x01, 0x80, "PAD ENTER"},  // 15A PAD ENTER
    {NO___EFFECT, 0, 0, ""},                 // 15B
    {NO___EFFECT, 0, 0, ""},                 // 15C
    {NO___EFFECT, 0, 0, ""},                 // 15D
    {NO___EFFECT, 0, 0, ""},                 // 15E
    {NO___EFFECT, 0, 0, ""},                 // 15F

    {NO___EFFECT, 0, 0, ""},            // 160
    {NO___EFFECT, 0, 0, ""},            // 161
    {NO___EFFECT, 0, 0, ""},            // 162
    {NO___EFFECT, 0, 0, ""},            // 163
    {NO___EFFECT, 0, 0, ""},            // 164
    {NO___EFFECT, 0, 0, ""},            // 165
    {NO___EFFECT, 0, 0, ""},            // 166
    {NO___EFFECT, 0, 0, ""},            // 167
    {NO___EFFECT, 0, 0, ""},            // 168
    {NORMAL__KEY, 0x0a, 0x08, "END"},   // 169 END
    {NO___EFFECT, 0, 0, ""},            // 16A
    {WINDOWS_KEY, 0x0a, 0x04, "LEFT"},  // 16B LEFT
    {NORMAL__KEY, 0x08, 0x01, "HOME"},  // 16C HOME
    {NO___EFFECT, 0, 0, ""},            // 16D
    {NO___EFFECT, 0, 0, ""},            // 16E
    {NO___EFFECT, 0, 0, ""},            // 16F

    {CTL_ALT_KEY, 0, 0, "INSERT"},        // 170 INSERT
    {CTL_ALT_KEY, 0x08, 0x08, "DELETE"},  // 171 DELETE
    {WINDOWS_KEY, 0x0a, 0x02, "DOWN"},    // 172 DOWN
    {NO___EFFECT, 0, 0, ""},              // 173
    {WINDOWS_KEY, 0x08, 0x04, "RIGHT"},   // 174 RIGHT
    {WINDOWS_KEY, 0x08, 0x02, "UP"},      // 175 UP
    {NO___EFFECT, 0, 0, ""},              // 176
    {NO___EFFECT, 0, 0, ""},              // 177
    {NO___EFFECT, 0, 0, ""},              // 178
    {NO___EFFECT, 0, 0, ""},              // 179
    {NORMAL__KEY, 0x0b, 0x01, "PGDOWN"},  // 17A PGDOWN
    {NO___EFFECT, 0, 0, ""},              // 17B
    {NO___EFFECT, 0, 0, ""},              // 17C
    {NORMAL__KEY, 0x0b, 0x02, "PGUP"},    // 17D PGUP
    {NO___EFFECT, 0, 0, ""},              // 17E
    {NO___EFFECT, 0x01, 0x10, "PAD ="},   // 17F PAD_ENTER "ENTER" or "="
};
