# PC8801FabGL

## What is PC8801FabGL

The PC8801FabGL is an emulator of PC-8801 running on ESP32 with [FabGL](http://www.fabglib.org/).
It emulates not only the main unit but also peripheral devices such as disk units.

### Emulated hardware

| Hardware    | Description                                                  |
| ----------- | ------------------------------------------------------------ |
| PC-8801     | Main unit                                                    |
| PC-8801-01  | Kanji ROM board for PC-8801                                  |
| PC-8801-02N | 128K bytes RAM board                                         |
| PC-80S31    | Dual mini disk units (for supporting d88 file)               |
| PC-80S32    | Dual mini disk units for expansion (for supporting d88 file) |
| DR320       | Data recoder (for supporting cmt file)                       |
| PCG-8800    | Programmable character generator board for PC-8801           |

## Requirements

### Hardware

-   LILYGO [TTGO VGA32 V1.4](http://www.lilygo.cn/prod_view.aspx?TypeId=50063&Id=1083)
-   PicoSoft [ORANGE-ESPer](http://www.picosoft.co.jp/ESP32/index.html) with [ESP32-WROVER-E](https://akizukidenshi.com/catalog/g/gM-15674/)
-   PS/2 Japanese 106/109 keyboard
-   Display with VGA port (VGA 640×480 60Hz)
-   VGA cable
-   Micro SD card

### Software

-   [Arduino ESP32 Version 2.0.5](https://github.com/espressif/arduino-esp32/releases/tag/2.0.5)
-   [FabGL v1.0.9](https://github.com/fdivitto/FabGL/releases/tag/v1.0.9)
-   [Arduino IDE](https://www.arduino.cc/) (for building PC8801FabGL)

### ROM images

| File name  | Requirement | File size     |
| ---------- | ----------- | ------------- |
| N88.ROM    | Required    | 32,768 bytes  |
| N80.ROM    | Required    | 32,768 bytes  |
| N88_0.ROM  | Required    | 8,192 bytes   |
| FONT.ROM   | Required    | 2,048 bytes   |
| DISK.ROM   | Optional    | 2,048 bytes   |
| KANJI1.ROM | Optional    | 131,072 bytes |
| USER.ROM   | Optional    | 8,192 bytes   |

## How to make micro SD card image

In a micro SD card, create a `pc8801` folder in the root. Under the folder,  
put ROM images and create some folders as shown:

```
/
+-- pc8801/
    +-- N88.ROM
    +-- N80.ROM
    +-- N88_0.ROM
    +-- FONT.ROM
    +-- DISK.ROM (optional)
    +-- KANJI.ROM (optional)
    +-- USER.ROM (optional)
    +-- disk/
        +--- *.d88
    +-- tape/
        +--- *.cmt
    +-- n80/
        +--- *.n80
    +-- bin/
        +--- *.bin
```

The files with `.ROM` extension are ROM images. The `disk` is a folder putting d88 files.
The `tape` is a folder putting cmt files. The `n80` is a folder putting n80 files.
The `bin` is the folder where bin files that are compiled sketches put.

## How to build PC8801FabGL

### Set up Arduino IDE

First of all, set up the Arduino IDE on your machine. You can get it from [the Arduino web site](https://www.arduino.cc/en/software).

### Download PC8801FabGL

Download PC8801FabGL from `https://github.com/Basara767676/PC8801FabGL`. Put it in your Sketchbook location.

### Install Arduino ESP32 Version 2.0.5

Run the Arduino IDE. Select the `Preferences` from the `File` menu. Add the following URL in the Additional Boards Manager URLs.

```
https://dl.espressif.com/dl/package_esp32_index.json
```

Next, select the `Board` from the `Tools` menu and open the Boards Manager. Search for `ESP32` and install the esp32 by Espressif
Systems board with version 2.0.5.

### Install FabGL v1.0.9

Select the `Manage Libraries...` from the `Tools` menu. Search for `FabGL` and install FabGL by Fabrizio Di Vittorio version v1.0.9.

### Specify the board information

Specify the board information from the `Tools` menu.

 -   Board:            ESP32 Dev Module
 -   Partition Scheme: Hug APP (3MB No OTA/1MB SPIFFS)
 -   PSRAM:            Enabled

### Upload the PC8801FabGL

Complete the sketch of PC8801FabGL and upload it into your ESP32.

## Operation

### PC-8801 keyboard mapping to PS/2 keyboard

| PC-8801 Keyboard | PS/2 Japanese 106/109 keyboard                                                 |
| ---------------- | ------------------------------------------------------------------------------ |
| STOP             | Esc                                                                            |
| COPY             | Scroll Lock                                                                    |
| ESC              | 半角/全角\|漢字                                                                |
| TAB              | TAB                                                                            |
| CTRL             | Ctrl                                                                           |
| CAPS             | Caps Lock  (Turn on LED of Caps Lock on Keyboard when enabling.)               |
| カナ             | カタカナひらがな\|漢字 (Turn on LED of Scroll Lock on Keyboard when enabling.) |
| GRPH             | Alt                                                                            |
| INS DEL          | BackSpace or Delete                                                            |
| ROLL UP          | Page Down                                                                      |
| ROLL DOWN        | Page Up                                                                        |
| HOME CLR         | Home                                                                           |
| HELP             | End                                                                            |
| PAD =            | Shift + PAD Enter

## Keys and Key combination

The keys and key combination to operating an emulator are as shown:

| Keys and Key combination | Description                                             |
| ------------------------ | ------------------------------------------------------- |
| F9                       | Whether to mute BEEP and PCG sound.                     |
| F10                      | Whether to force enable PCG. (Output 8 to I/O port 3)   |
| F12                      | Enter preferences mode.                                 |
| Ctrl + Alt + Delete      | Reset PC-8801 with keeping memory contents.                 |
| Ctrl + Alt + Insert      | Power on reset PC-8801 without keeping memory contents.     |
| Win + Left arrow         | Rewind tape. (Move tape to BOT)                             |
| Win + Right arrow        | Move tape to EOT.                                           |
| Win + Up arrow           | Up sound volume.                                            |
| Win + Dwon arrow         | Down sound volume.                                          |
| Win + (from 0 to 9)      | 0: No wait, 1: Very very fast, 5: Normal, 9: Very very slow |

## Preferences

Press `F12` key to enter the preferences mode. To exit this mode, press `Esc` key.

| Item                   | Description                                                         |
| ---------------------- | ------------------------------------------------------------------- |
| Miscellaneous settings | Move to miscellaneous settings.                                     |
| BASIC                  | Swich BASIC mode. N-BASIC or N88-BASIC.                             |
| TAPE                   | Specify a cmt file to be mounted on the tape unit.                  |
| DISK                   | Conect or disconect disk units.                                     |
| Drive1                 | Specify a d88 file to be mounted on the drive unit 1.               |
| Drive2                 | Specify a d88 file to be mounted on the drive unit 2.               |
| Drive3                 | Specify a d88 file to be mounted on the drive unit 3.               |
| Drive4                 | Specify a d88 file to be mounted on the drive unit 4.               |
| Load n80 file          | Specify a n80 file. Switch to N-BASIC mode when using this feature. |
| PC-8801 reset          | Reset PC-8801 with keeping memory contents.                         |
| PC-8801 cold boot      | Power on reset PC-8801 without keeping memory contents.             |
| ESP32 reset            | Reset ESP32                                                         |

### Miscellaneous settings

| Item                      | Description                                                            |
| --------------------------| ---------------------------------------------------------------------- |
| File Manager              | Enter File Manager menu.                                               |
| CPU Speed                 | Set CPU Speed.                                                         |
| Volume                    | Set volume level                                                       |
| Columns                   | Specify columns of text screen. 40 or 80                               |
| Rows                      | Specify rows of text screen. 20 or 25                                  |
| Resolution (Hsync)        | Specify display resolution. 200 lines (15.7kHz) or 400 lines (24.8kHz) |
| PC-8801-02N               | Enable or disable 128K bytes RAM board                                 |
| PCG                       | Whether to enable PCG-8800. (Auto or On)                               |
| Behavior of PAD enter key | Specify behavior of PAD enter key as `=` key or `RETURN` key.          |
| Update firmware           | Update firmware for this emulator.                                     |

### File Manager

| Item                       | Description                                                       |
| -------------------------- | ----------------------------------------------------------------- |
| Create a new tape          | Create a new empty cmt file.                                      |
| Rename a tape              | Rename a cmt file.                                                |
| Delete a tape              | Delete a cmt file.                                                |
| Create a new disk          | Create a new d88 file that is a blank disk formatted for 2D disk. |
| Rename a disk              | Rename a d88 file.                                                |
| Protect a disk             | Whether to set a d88 file to protected or writable.               |
| Delete a disk              | Delete a d88 file.                                                |

## Architecture

### Tasks

| Task           | Core | Priority    |
| -------------- | ---- | ----------- |
| PC-8801 task   | 0    | 1           |
| PC-80S31 task  | 1    | 1           |
| Keyboard task  | 1    | 1           |
| FabGL tasks    | 1    | more than 1 |

This program runs without the Wifi and Bluetooth feature.

## Dependencies

| Software                                                                             | OSS license                                          |
| ------------------------------------------------------------------------------------ | -------------------------------------------------|
| [FabGL v1.0.9](https://github.com/fdivitto/fabgl)                                    | [GNU GENERAL PUBLIC LICENSE Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html) |
| [Arduino ESP32 Version 2.0.5](https://github.com/espressif/arduino-esp32/tree/2.0.5) | [GNU LESSER GENERAL PUBLIC LICENSE Version 2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html) |

## OSS license

GNU GENERAL PUBLIC LICENSE Version 3 (GNU GPLv3)

## Copyright

Copyright (C) 2022 Basara767676
