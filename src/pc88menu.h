#pragma once

#pragma GCC optimize("O2")

#include "pc88settings.h"
#include "pc88vm.h"

#define PC88MENU_DONE (100)
#define PC88MENU_N80 (101)
#define PC88MENU_PC88_RESET (102)
#define PC88MENU_ESP32_RESTART (103)

class PC88VM;

class PC88MENU {
   public:
    PC88MENU();
    ~PC88MENU();

    int menu(PC88VM *vm);

   private:
    char mMenuTitle[64];
    char mMenuMsg[64];
    char mPath[512];
    char mFileName[64];
    char mPath2[512];
    char mFileName2[64];
    char mMenuItem[1024];

    PC88VM *mVM;

    int diskSelector(fabgl::InputBox *ib, PC88SETTINGS *pc88Settings, int drive, char *driveStr);
    int tapeSelector(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);

    const char *getMode(int mode, bool cur, bool next);
    const int loadN80File(fabgl::InputBox *ib);

    int miscSettings(fabgl::InputBox *ib);
    int updateFirmware(fabgl::InputBox *ib);

    int changeVolume(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);

    int fileManager(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);

    int cpuSpeed(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);
    const char *cpuSpeedStr(int i);

    int createDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);
    int renameDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);
    int protectDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);
    int deleteDisk(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);

    int createTape(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);
    int renameTape(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);
    int deleteTape(fabgl::InputBox *ib, pc88_settings_t *current, PC88SETTINGS *pc88Settings);

    bool isMounted(const char *fileName, pc88_settings_t *current);
};