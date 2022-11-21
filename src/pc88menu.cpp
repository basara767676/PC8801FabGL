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

#include "pc88menu.h"

#include <Update.h>

#include "d88.h"
#include "file-stream.h"

#ifdef DEBUG_PC88
// #define DEBUG_PC88MENU
#endif

PC88MENU::PC88MENU() {}
PC88MENU::~PC88MENU() {}

#define MENU_EXIT (-1)
#define MENU_TO_VM (-3)
#define MENU_CONTINUE (-2)

#define BASIC_MODE (0)
#define DISK_MODE (1)
#define COLUMN_MODE (2)
#define ROW_MODE (3)
#define LINE_MODE (4)
#define EXTRAM_MODE (5)
#define PCG_MODE (6)

#define MENU_MISC_SETTINGS (0)
#define MENU_BASIC_MODE (1)
#define MENU_TAPE (2)
#define MENU_DRIVE_MODE (3)
#define MENU_DRIVE_0 (4)
#define MENU_DRIVE_1 (5)
#define MENU_DRIVE_2 (6)
#define MENU_DRIVE_3 (7)
#define MENU_LOAD_N80_FILE (8)
#define MENU_PC88_RESET (9)
#define MENU_PC88_COLD_BOOT (10)
#define MENU_ESP32_RESTART (11)

#define MENU_FILE_MANAGER (0)
#define MENU_CPU_SPEED (1)
#define MENU_VOLUME (2)
#define MENU_COLUMN (3)
#define MENU_ROW (4)
#define MENU_LINE (5)
#define MENU_EXTRAM (6)
#define MENU_PCG (7)
#define MENU_PAD_ENTER (8)
#define MENU_UPDATE_FW (9)

#define MENU_CREATE_TAPE (0)
#define MENU_RENAME_TAPE (1)
#define MENU_DELETE_TAPE (2)
#define MENU_CREATE_DISK (3)
#define MENU_RENAME_DISK (4)
#define MENU_PROTECT_DISK (5)
#define MENU_DELETE_DISK (6)

int PC88MENU::menu(PC88VM *vm) {
    int rc;
    fabgl::InputBox ib;

    mVM = vm;

    ib.begin(VGA_640x480_60Hz, 640, 480, 4);
    ib.setBackgroundColor(RGB888(0, 0, 0));

    auto current = mVM->getCurrentSettings();
    auto pc88Settings = mVM->getPC88Settings();

    strcpy(mMenuTitle, "PC8801FabGL 1.0.0 preferences");

    do {
        sprintf(mMenuItem,
                "Miscellaneous settings;BASIC: %s;TAPE: %s;DISK: %s;Drive1: %s;Drive2: %s;Drive3: %s;Drive4: %s;Load n80 "
                "file;PC-8801 reset;PC-8801 cold boot;ESP32 reset",
                getMode(BASIC_MODE, current->n88, pc88Settings->getN88()), current->tape,
                getMode(DISK_MODE, current->drive, pc88Settings->getDrive()), current->disk[0], current->disk[1], current->disk[2],
                current->disk[3]);
        rc = ib.menu(mMenuTitle, "Select an item           ", mMenuItem);
        switch (rc) {
            case MENU_MISC_SETTINGS:
                rc = miscSettings(&ib);
                if (rc == MENU_EXIT) rc = MENU_CONTINUE;
                break;
            case MENU_BASIC_MODE:
                pc88Settings->setN88(!pc88Settings->getN88());
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_TAPE:
                rc = tapeSelector(&ib, current, pc88Settings);
                break;
            case MENU_DRIVE_MODE:
                pc88Settings->setDrive(!pc88Settings->getDrive());
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_DRIVE_0:  // drive 0
                rc = diskSelector(&ib, pc88Settings, 0, current->disk[0]);
                break;
            case MENU_DRIVE_1:  // drive 1
                rc = diskSelector(&ib, pc88Settings, 1, current->disk[1]);
                break;
            case MENU_DRIVE_2:  // drive2
                rc = diskSelector(&ib, pc88Settings, 2, current->disk[2]);
                break;
            case MENU_DRIVE_3:  // drive 3
                rc = diskSelector(&ib, pc88Settings, 3, current->disk[3]);
                break;
            case MENU_LOAD_N80_FILE:
                rc = loadN80File(&ib);
                break;
            case MENU_PC88_RESET:
                rc = CMD_RESET;
                break;
            case MENU_PC88_COLD_BOOT:
                rc = CMD_COLD_BOOT;
                break;
            case MENU_ESP32_RESTART:
                rc = CMD_ESP32_RESTART;
                break;
        }
    } while (rc == MENU_CONTINUE);

    ib.end();

    return rc;
}

int PC88MENU::miscSettings(fabgl::InputBox *ib) {
    auto current = mVM->getCurrentSettings();
    auto pc88Settings = mVM->getPC88Settings();

    int rc;
    do {
        sprintf(mMenuItem,
                "File Manager;CPU speed: %s;Volume %d;Columns: %s;Rows: %s;Resolution (Hsync): %s;PC-8801-02N (ExtRAM): %s;PCG: "
                "%s;Behavior of PAD enter key: %s;Update firmware",
                cpuSpeedStr(current->speed), current->volume, getMode(COLUMN_MODE, current->column40, pc88Settings->getColumn()),
                getMode(ROW_MODE, current->row20, pc88Settings->getRow()), getMode(LINE_MODE, current->line200, pc88Settings->getLine200()),
                getMode(EXTRAM_MODE, current->extRam, pc88Settings->getExtRAM()), getMode(PCG_MODE, current->pcg, pc88Settings->getPCG()),
                current->padEnter ? "Behave as equal key (=)" : "Behave as RETURN key");
        rc = ib->menu(mMenuTitle, "Select an item", mMenuItem);
        switch (rc) {
            case MENU_FILE_MANAGER:
                rc = fileManager(ib, current, pc88Settings);
                break;
            case MENU_CPU_SPEED:
                rc = cpuSpeed(ib, current, pc88Settings);
                break;
            case MENU_VOLUME:
                rc = changeVolume(ib, current, pc88Settings);
                break;
            case MENU_COLUMN:
                pc88Settings->setColumn(!pc88Settings->getColumn());
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_ROW:
                pc88Settings->setRow(!pc88Settings->getRow());
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_LINE:
                pc88Settings->setLine200(!pc88Settings->getLine200());
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_EXTRAM:
                pc88Settings->setExtRAM(!pc88Settings->getExtRAM());
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_PCG:
                pc88Settings->setPCG(!pc88Settings->getPCG());
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_PAD_ENTER:
                current->padEnter = !current->padEnter;
                pc88Settings->setPadEnter(current->padEnter);
                mVM->getPC88KeyBoard()->setPadEnter(current->padEnter);
                pc88Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_UPDATE_FW:
                rc = updateFirmware(ib);
                break;
        }
    } while (rc == MENU_CONTINUE);

    return rc;
}

int PC88MENU::diskSelector(fabgl::InputBox *ib, PC88SETTINGS *pc88Settings, int driveNo, char *driveStr) {
    if (strlen(driveStr) > 0) {
        sprintf(mMenuMsg, "Eject the disk file for drive %d", driveNo + 1);
        sprintf(mMenuItem, "Eject: %s", driveStr);
        auto value = ib->menu(mMenuTitle, mMenuMsg, mMenuItem);
        if (value == 0) {
            strcpy(driveStr, "");
            pc88Settings->setDisk(driveNo, driveStr);
            pc88Settings->save();
            mVM->getPC80S31()->closeDrive(driveNo);
        }
    } else {
        strcpy(mPath, SD_MOUNT_POINT);
        strcat(mPath, PC88DIR);
        strcat(mPath, PC88DIR_DISK);
        strcpy(mFileName, "");
        sprintf(mMenuMsg, "Select the disk file for drive %d", driveNo);

        auto rc = ib->fileSelector(mMenuMsg, "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
        if (rc == InputResult::Enter && strlen(mFileName) > 0) {
            const char *ext = strrchr(mFileName, '.');
            if (ext == nullptr || strcasecmp(ext, ".d88")) return MENU_CONTINUE;

            strcpy(driveStr, mPath);
            strcat(driveStr, "/");
            strcat(driveStr, mFileName);
            pc88Settings->setDisk(driveNo, driveStr);
            pc88Settings->save();
            mVM->getPC80S31()->openDrive(driveNo, driveStr);
        }
    }
    return MENU_CONTINUE;
}

int PC88MENU::tapeSelector(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    if (!strcmp("", current->tape)) {
        strcpy(mPath, SD_MOUNT_POINT);
        strcat(mPath, PC88DIR);
        strcat(mPath, PC88DIR_TAPE);
        strcpy(mFileName, "");
        auto rc = ib->fileSelector("Select tape file to load", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
        if (rc == InputResult::Enter && strcmp("", mFileName)) {
            const char *ext = strrchr(mFileName, '.');
            if (ext == nullptr || strcasecmp(ext, ".cmt")) return MENU_CONTINUE;

            strcpy(current->tape, mPath);
            strcat(current->tape, "/");
            strcat(current->tape, mFileName);
            pc88Settings->setTape(current->tape);
            pc88Settings->save();
            mVM->getDR320()->open(current->tape);
            return MENU_EXIT;
        }
    } else {
        sprintf(mMenuItem, "Eject: %s", current->tape);
        auto value = ib->menu(mMenuTitle, "Eject the tape file", mMenuItem);
        if (value == 0) {
            strcpy(current->tape, "");
            pc88Settings->setTape(current->tape);
            pc88Settings->save();
            mVM->getDR320()->close();
        }
    }
    return MENU_CONTINUE;
}

const int PC88MENU::loadN80File(fabgl::InputBox *ib) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC88DIR);
    strcat(mPath, PC88DIR_N80);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Select the n80 file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);

    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".n80")) return MENU_CONTINUE;

        strcat(mPath, "/");
        strcat(mPath, mFileName);
#ifdef DEBUG_PC88MENU
        Serial.println(mFileName);
#endif

        struct stat fileStat;
        if (stat(mPath, &fileStat) == -1) {
            return MENU_CONTINUE;
        }

        auto fp = fopen(mPath, "r");
        if (!fp) {
#ifdef DEBUG_PC88MENU
            Serial.printf("Open error: %s\n", mPath);
#endif
            return MENU_CONTINUE;
        }

#ifdef DEBUG_PC88MENU
        Serial.printf("%s %d\n", mPath, fileStat.st_size);
#endif

        fseek(fp, 0, SEEK_SET);
        size_t result = fread(mVM->getRAM8000(), 1, fileStat.st_size, fp);

        fclose(fp);

        if (result != fileStat.st_size) {
            return MENU_CONTINUE;
        }

#ifdef DEBUG_PC88MENU
        Serial.printf("%s loaded\n", mPath);
#endif

        return CMD_N80_FILE;
    }

    return MENU_CONTINUE;
}
int PC88MENU::updateFirmware(fabgl::InputBox *ib) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, "/bin");
    strcpy(mFileName, "");

    auto updated = MENU_CONTINUE;
    auto rc = ib->fileSelector("Select the firmware file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);

    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".bin")) return updated;

        strcat(mPath, "/");
        strcat(mPath, mFileName);
#ifdef DEBUG_PC88MENU
        Serial.println(mPath);
#endif

        auto r = ib->progressBox("Updating firmware", nullptr, true, 200, [&](fabgl::ProgressForm *form) {
            FileStream file;
            auto updateSize = file.open(mPath);
            if (updateSize <= 0) return updated;

            if (Update.begin(updateSize)) {
                Update.onProgress([&](unsigned int progress, unsigned int total) {
                    progress = progress / (total / 100);
                    form->update(progress, "%s", mFileName);
                });
                size_t written = Update.writeStream(file);
                if (Update.end()) {
                    if (Update.isFinished() && (written == updateSize)) {
                        ib->message("Done", "Firmware update is complete.", nullptr);
                        updated = CMD_ESP32_RESTART;
                    } else {
                        ib->message("Error", "Firmware update not finished.", nullptr);
                    }
                } else {
                    strcpy(mPath, "Error Occurred. Error #: ");
                    strcat(mPath, String(Update.getError()).c_str());
                    ib->message("Error", mPath, nullptr);
                }
            } else {
                ib->message("Error", "Not enough space to begin OTA", nullptr);
            }
            return updated;
        });
    }

    return updated;
}

int PC88MENU::changeVolume(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    mMenuItem[0] = 0;
    char temp[16];
    for (int i = 0; i < 16; i++) {
        sprintf(temp, "Volume %d;", i);
        strcat(mMenuItem, temp);
    }
    mMenuItem[strlen(mMenuItem) - 1] = 0;
    int value = ib->select("Sound volume", "Select volume", mMenuItem);
    if (0 <= value && value <= 15) {
        mVM->setVolume(value);
        current->volume = value;
        pc88Settings->setVolume(value);
        pc88Settings->save();
    }
    return MENU_CONTINUE;
}

int PC88MENU::fileManager(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    int rc = MENU_CONTINUE;

    do {
        rc = ib->select("File Manager", "Select item",
                        "Create new tape;Rename tape;Delete tape;Create new disk;Rename disk;Protect disk;Delete disk");
        switch (rc) {
            case MENU_CREATE_TAPE:
                rc = createTape(ib, current, pc88Settings);
                break;
            case MENU_RENAME_TAPE:
                rc = renameTape(ib, current, pc88Settings);
                break;
            case MENU_DELETE_TAPE:
                rc = deleteTape(ib, current, pc88Settings);
                break;
            case MENU_CREATE_DISK:
                rc = createDisk(ib, current, pc88Settings);
                break;
            case MENU_RENAME_DISK:
                rc = renameDisk(ib, current, pc88Settings);
                break;
            case MENU_PROTECT_DISK:
                rc = protectDisk(ib, current, pc88Settings);
                break;
            case MENU_DELETE_DISK:
                rc = deleteDisk(ib, current, pc88Settings);
                break;
        }
    } while (rc == MENU_CONTINUE);

    return rc;
}

int PC88MENU::createDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC88DIR);
    strcat(mPath, PC88DIR_DISK);
    strcpy(mFileName, "");

    auto rc = MENU_CONTINUE;

    if (ib->textInput("Create a new disk", "file name", mFileName, 31, nullptr, "OK") == InputResult::Enter) {
        strcat(mPath, "/");
        strcat(mPath, mFileName);
        strcat(mPath, ".D88");
#ifdef DEBUG_PC88MENU
        Serial.println(mPath);
#endif

        FILE *fp = fopen(mPath, "rb");
        if (fp) {
            fclose(fp);
            sprintf(mPath, "File already exists: %s.D88", mFileName);
            ib->message("Error", mPath, nullptr);
            return MENU_CONTINUE;
        }

        auto buf = (uint8_t *)ps_malloc(16 * (sizeof(d88_sector_header_t) + 0x100));

        auto r = ib->progressBox("Creating a new disk", nullptr, true, 200, [&](fabgl::ProgressForm *form) {
            fp = fopen(mPath, "w");
            auto p = (d88_header_t *)buf;
            memset(p, 0, sizeof(d88_header_t));
            p->diskSize = sizeof(d88_header_t) + 40 * 2 * 16 * (sizeof(d88_sector_header_t) + 0x100);
            auto offset = sizeof(d88_header_t);
            for (int i = 0; i < 40 * 2; i++) {
                p->track[i] = offset;
                offset += (sizeof(d88_sector_header_t) + 0x100) * 16;
            }
            fwrite(p, 1, sizeof(d88_header_t), fp);
            for (int i = 0; i < 80; i++) {
                form->update((i * 100) / 80, "%s", mFileName);
                auto sector = buf;
                memset(buf, 0xff, 16 * (sizeof(d88_sector_header_t) + 0x100));
                for (int j = 0; j < 16; j++) {
                    auto header = (d88_sector_header_t *)sector;
                    memset(header, 0, sizeof(d88_sector_header_t));
                    header->geometry.c = i / 2;
                    header->geometry.h = i % 2;
                    header->geometry.r = j + 1;
                    header->geometry.n = 1;
                    header->sizeOfData = 0x100;
                    sector += sizeof(d88_sector_header_t) + 0x100;
                }
                fwrite(buf, 1, 16 * (sizeof(d88_sector_header_t) + 0x100), fp);
            }
            fclose(fp);
        });

        free(buf);

        rc = MENU_CONTINUE;

        // mount new disk
        for (int i = 0; i < 4; i++) {
            if (strlen(current->disk[i]) == 0) {
                strcpy(current->disk[i], mPath);
                pc88Settings->setDisk(i, mPath);
                pc88Settings->save();
                mVM->getPC80S31()->openDrive(i, mPath);
                rc = MENU_EXIT;
                break;
            }
        }
    }
    return rc;
}

int PC88MENU::renameDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC88DIR);
    strcat(mPath, PC88DIR_DISK);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Rename disk file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".d88")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not disk file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcpy(mFileName2, "");
        if (ib->textInput("Enter new disk name", "file name", mFileName2, 31, nullptr, "OK") == InputResult::Enter) {
            ext = strrchr(mFileName2, '.');
            if (ext == nullptr || strcasecmp(ext, ".d88")) {
                strcat(mFileName2, ".d88");
            }
            strcat(mPath, "/");
            strcpy(mPath2, mPath);
            strcat(mPath, mFileName);
            strcat(mPath2, mFileName2);
            if (isMounted(mPath, current)) {
                ib->message("Error: already mounted", mPath, nullptr);
                return MENU_CONTINUE;
            }
            FILE *fp = fopen(mPath2, "r");
            if (fp) {
                fclose(fp);
                ib->message("Error: already exists", mPath2, nullptr);
                return MENU_CONTINUE;
            }
            rename(mPath, mPath2);
        }
    }
    return MENU_CONTINUE;
}

int PC88MENU::protectDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC88DIR);
    strcat(mPath, PC88DIR_DISK);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Delete disk file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".d88")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not disk file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcat(mPath, "/");
        strcat(mPath, mFileName);

        if (isMounted(mPath, current)) {
            ib->message("Error: already mounted", mPath, nullptr);
            return MENU_CONTINUE;
        }

        FILE *fp = fopen(mPath, "rb+");
        if (fp) {
            uint8_t buf;
            fseek(fp, 0x1a, SEEK_SET);
            fread(&buf, 1, 1, fp);
            buf &= 0x10;
            buf ^= 0x10;
            fseek(fp, 0x1a, SEEK_SET);
            fwrite(&buf, 1, 1, fp);
            fclose(fp);

#ifdef DEBUG_PC88MENU
            Serial.printf("protect %02x", buf);
#endif
            ib->message(buf == 0 ? "Writable" : "Protected", mPath, nullptr);
        }
    }
    return MENU_CONTINUE;
}

int PC88MENU::deleteDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC88DIR);
    strcat(mPath, PC88DIR_DISK);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Delete disk file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".d88")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not disk file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcat(mPath, "/");
        strcat(mPath, mFileName);

        if (isMounted(mPath, current)) {
            ib->message("Error: already mounted", mPath, nullptr);
            return MENU_CONTINUE;
        }
        if (ib->message("Do you want to delete this ?", mPath) == InputResult::Enter) {
            remove(mPath);
        }
    }
    return MENU_CONTINUE;
}

int PC88MENU::createTape(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    int rc = MENU_CONTINUE;

    strcpy(mFileName, "");

    if (ib->textInput("Create a new tape", "file name", mFileName, 31, nullptr, "OK") == InputResult::Enter) {
#ifdef DEBUG_PC88MENU
        Serial.printf("OK: %s\n", mFileName);
#endif

        strcpy(mPath, SD_MOUNT_POINT);
        strcat(mPath, PC88DIR);
        strcat(mPath, PC88DIR_TAPE);
        strcat(mPath, "/");
        strcat(mPath, mFileName);
        strcat(mPath, ".CMT");

        FILE *fp = fopen(mPath, "rb");
        if (fp) {
            fclose(fp);
            sprintf(mPath, "File already exists: %s.CMT", mFileName);
            ib->message("Error", mPath, nullptr);
        } else {
            fp = fopen(mPath, "w");
            fclose(fp);
            strcpy(current->tape, mPath);
            pc88Settings->setTape(current->tape);
            pc88Settings->save();
            mVM->getDR320()->open(current->tape);
            rc = MENU_EXIT;
        }
    } else {
#ifdef DEBUG_PC88MENU
        Serial.printf("Cancel: %s\n", mFileName);
#endif
    }

    return rc;
}

int PC88MENU::renameTape(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC88DIR);
    strcat(mPath, PC88DIR_TAPE);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Rename tape file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".cmt")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not tape file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcpy(mFileName2, "");
        if (ib->textInput("Enter new tape name", "file name", mFileName2, 31, nullptr, "OK") == InputResult::Enter) {
            ext = strrchr(mFileName2, '.');
            if (ext == nullptr || strcasecmp(ext, ".cmt")) {
                strcat(mFileName2, ".cmt");
            }
            strcat(mPath, "/");
            strcpy(mPath2, mPath);
            strcat(mPath, mFileName);
            strcat(mPath2, mFileName2);
            if (isMounted(mPath, current)) {
                ib->message("Error: already mounted", mPath, nullptr);
                return MENU_CONTINUE;
            }
            FILE *fp = fopen(mPath2, "r");
            if (fp) {
                fclose(fp);
                ib->message("Error: already exists", mPath2, nullptr);
                return MENU_CONTINUE;
            }
            rename(mPath, mPath2);
        }
    }
    return MENU_CONTINUE;
}

int PC88MENU::deleteTape(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC88DIR);
    strcat(mPath, PC88DIR_TAPE);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Delete tape file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".cmt")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not tape file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcat(mPath, "/");
        strcat(mPath, mFileName);

        if (isMounted(mPath, current)) {
            ib->message("Error: already mounted", mPath, nullptr);
            return MENU_CONTINUE;
        }
        if (ib->message("Do you want to delete this ?", mPath) == InputResult::Enter) {
            remove(mPath);
        }
    }
    return MENU_CONTINUE;
}

const char *PC88MENU::getMode(int mode, bool cur, bool next) {
    static const char *msg[][4] = {{"N80", "N88", "N80 (N88 when next cold boot)", "N88 (N80 when next cold boot)"},
                                   {"Disable", "Enable", "Disable (Enable when next cold boot)", "Enable (Disable when next cold boot)"},
                                   {"80", "40", "80 (40 when next cold boot)", "40 (80 when next cold boot)"},
                                   {"25", "20", "25 (20 when next cold boot)", "20 (25 when next cold boot)"},
                                   {"High (24.8kHz)", "Normal (15.7kHz)", "High (24.8kHz) (15.7kHz when next cold boot)",
                                    "Normal (15.7kHz) (24.8kHz when next cold boot)"},
                                   {"Disable", "Enable", "Disable (Enable when next cold boot)", "Enable (Disable when next cold boot)"},
                                   {"Auto", "On", "Auto (On when next cold boot)", "On (Auto when next cold boot)"}};

    int index = 0;

    if (cur) index = 0x01;
    if (cur != next) index |= 0x02;

    return msg[mode][index];
}

const char *PC88MENU::cpuSpeedStr(int i) {
    static const char *str[10] = {"No wait", "Very very fast", "Very fast", "Fast",      "A little fast",
                                  "Normal",  "A little slow",  "Slow",      "Very slow", "Very very slow"};
    if (0 <= i && i < 10) {
        return str[i];
    } else {
        return "Unknown";
    }
}

int PC88MENU::cpuSpeed(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings) {
    mMenuItem[0] = 0;
    for (int i = 0; i < 10; i++) {
        strcat(mMenuItem, cpuSpeedStr(i));
        strcat(mMenuItem, ";");
    }
    mMenuItem[strlen(mMenuItem) - 1] = 0;
    int value = ib->select("CPU speed", "Select speed", mMenuItem);
    if (CPU_SPEED_NO_WAIT <= value && value <= CPU_SPEED_VERY_VERY_SLOW) {
        mVM->setCpuSpeed(value);
        current->speed = value;
        pc88Settings->setCpuSpeed(value);
        pc88Settings->save();
    }
    return MENU_TO_VM;
}

bool PC88MENU::isMounted(const char *fileName, pc88_settings_t *current) {
    for (int i = 0; i < 4; i++) {
        if (strcmp(fileName, current->disk[i]) == 0) return true;
    }
    return false;
}