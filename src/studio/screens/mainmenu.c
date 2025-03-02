// MIT License

// Copyright (c) 2017 Vadim Grigoruk @nesbox // grigoruk@gmail.com

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "studio/studio.h"
#include "studio/config.h"
#include "studio/screens/menu.h"
#include "mainmenu.h"

typedef struct
{
    tic_mapping mapping;
    s32 index;
    s32 key;
} Gamepads;

struct StudioMainMenu
{
    Studio* studio;
    tic_mem* tic;
    Menu* menu;

    MenuItem* items;
    s32 count;

    Gamepads gamepads;
    struct StudioOptions* options;
};

#define OPTION_VALUES(...)                          \
    .values = (const char*[])__VA_ARGS__,           \
    .count = COUNT_OF(((const char*[])__VA_ARGS__))

static void showMainMenu(void* data, s32 pos);

StudioMainMenu* studio_mainmenu_init(Menu *menu, Config *config) 
{
    StudioMainMenu* main = NEW(StudioMainMenu);

    *main = (StudioMainMenu)
    {
        .menu = menu,
        .options = &config->data.options,
        .studio = config->studio,
        .tic = config->tic,
        .gamepads.key = -1,
    };

    showMainMenu(main, 0);

    return main;
}

static void initGamepadMenu(StudioMainMenu* menu);

bool studio_mainmenu_keyboard(StudioMainMenu* main)
{
    if(main && main->gamepads.key >= 0)
    {
        tic_key key = *main->tic->ram->input.keyboard.keys;
        if(key > tic_key_unknown)
        {
            main->gamepads.mapping.data[main->gamepads.index * TIC_BUTTONS + main->gamepads.key] = key;
            initGamepadMenu(main);
        }

        return true;            
    }

    return false;
}

static s32 optionFullscreenGet(void* data)
{
    return tic_sys_fullscreen_get() ? 1 : 0;
}

static void optionFullscreenSet(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    tic_sys_fullscreen_set(main->options->fullscreen = (pos == 1));
}

static const char OffValue[] =  "OFF";
static const char OnValue[] =   "ON";

static MenuOption FullscreenOption = 
{
    OPTION_VALUES({OffValue, OnValue}),
    optionFullscreenGet,
    optionFullscreenSet,
};

#if defined(CRT_SHADER_SUPPORT)
static s32 optionCrtMonitorGet(void* data)
{
    StudioMainMenu* main = data;
    return main->options->crt ? 1 : 0;
}

static void optionCrtMonitorSet(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    main->options->crt = pos == 1;
}

static MenuOption CrtMonitorOption = 
{
    OPTION_VALUES({OffValue, OnValue}),
    optionCrtMonitorGet,
    optionCrtMonitorSet,
};

#endif

static s32 optionVSyncGet(void* data)
{
    StudioMainMenu* main = data;
    return main->options->vsync ? 1 : 0;
}

static void optionVSyncSet(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    main->options->vsync = pos == 1;
}

static MenuOption VSyncOption = 
{
    OPTION_VALUES({OffValue, OnValue}),
    optionVSyncGet,
    optionVSyncSet,
};

static s32 optionVolumeGet(void* data)
{
    StudioMainMenu* main = data;
    return main->options->volume;
}

static void optionVolumeSet(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    main->options->volume = pos;
}

static MenuOption VolumeOption = 
{
    OPTION_VALUES(
    {
        "00", "01", "02", "03", 
        "04", "05", "06", "07", 
        "08", "09", "10", "11", 
        "12", "13", "14", "15", 
    }),
    optionVolumeGet,
    optionVolumeSet,
};

#if defined(BUILD_EDITORS)

static s32 optionDevModeGet(void* data)
{
    StudioMainMenu* main = data;
    return main->options->devmode ? 1 : 0;
}

static void optionDevModeSet(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    main->options->devmode = pos == 1;
}

static MenuOption DevModeOption = 
{
    OPTION_VALUES({OffValue, OnValue}),
    optionDevModeGet,
    optionDevModeSet,
};

#endif

static void showGamepadMenu(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    main->gamepads.index = 0;
    main->gamepads.mapping = main->options->mapping;

    initGamepadMenu(main);
}

static const MenuItem OptionMenu[] =
{
#if defined(CRT_SHADER_SUPPORT)
    {"CRT MONITOR",     NULL,   &CrtMonitorOption},
#endif
#if defined(BUILD_EDITORS)
    {"DEV MODE",        NULL,   &DevModeOption, "The game menu is disabled in dev mode."},
#endif
    {"VSYNC",           NULL,   &VSyncOption, "VSYNC needs restart!"},
    {"FULLSCREEN",      NULL,   &FullscreenOption},
    {"VOLUME",          NULL,   &VolumeOption},
    {"SETUP GAMEPAD",   showGamepadMenu},
    {""},
    {"BACK",            showMainMenu, .back = true},
};

static void showOptionsMenu(void* data, s32 pos);
static void gameMenuHandler(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    tic_mem* tic = main->tic;
    tic_core_script_config(tic)->callback.gamemenu(tic, pos, NULL);
    resumeGame(main->studio);
}

static void freeItems(StudioMainMenu* menu)
{
    if(menu && menu->items)
    {
        for(MenuItem *it = menu->items, *end = it + menu->count; it != end; ++it)
            free((void*)it->label);

        free(menu->items);
        menu->count = 0;
        menu->items = NULL;
    }
}

void studio_mainmenu_free(StudioMainMenu* menu)
{
    freeItems(menu);
    FREE(menu);
}

static void initGameMenu(StudioMainMenu* main)
{
    tic_mem* tic = main->tic;

    freeItems(main);

    char* menu = tic_tool_metatag(tic->cart.code.data, "menu", tic_core_script_config(tic)->singleComment);

    if(menu) SCOPE(free(menu))
    {
        MenuItem *items = NULL;
        s32 count = 0;

        char* label = strtok(menu, " ");
        while(label)
        {
            items = realloc(items, sizeof(MenuItem) * ++count);
            items[count - 1] = (MenuItem){strdup(label), gameMenuHandler};

            label = strtok(NULL, " ");
        }

        count += 2;
        items = realloc(items, sizeof(MenuItem) * count);
        items[count - 2] = (MenuItem){strdup("")};
        items[count - 1] = (MenuItem){strdup("BACK"), showMainMenu, .back = true};

        main->items = items;
        main->count = count;
    }
}

static void showGameMenu(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    studio_menu_init(main->menu, main->items, main->count, 0, 0, showMainMenu, main);
}

static inline s32 mainMenuStart(StudioMainMenu* menu)
{
    return menu->count ? 0 : 1;
}

static void onResumeGame(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    resumeGame(main->studio);
}

static void onResetGame(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    tic_api_reset(main->tic);
    setStudioMode(main->studio, TIC_RUN_MODE);
}

static void onExitStudio(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    exitStudio(main->studio);
}

static void onExitGame(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    exitGame(main->studio);
}

static const MenuItem MainMenu[] =
{
    {"GAME MENU",   showGameMenu},
    {"RESUME GAME", onResumeGame},
    {"RESET GAME",  onResetGame},
#if defined(BUILD_EDITORS)
    {"CLOSE GAME",  onExitGame},
#endif
    {"OPTIONS",     showOptionsMenu},
    {""},
    {"QUIT TIC-80", onExitStudio},
};

static void showMainMenu(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    initGameMenu(main);

    s32 start = mainMenuStart(main);

    studio_menu_init(main->menu, MainMenu + start, COUNT_OF(MainMenu) - start, 0, 0, onResumeGame, main);
}

static void showOptionsMenuPos(void* data, s32 pos)
{
    StudioMainMenu* main = data;

    studio_menu_init(main->menu, OptionMenu, 
        COUNT_OF(OptionMenu), pos, COUNT_OF(MainMenu) - 3 - mainMenuStart(main), showMainMenu, main);
}

static void showOptionsMenu(void* data, s32 pos)
{
    showOptionsMenuPos(data, COUNT_OF(OptionMenu) - 4);
}

static void saveGamepadMenu(void* data, s32 pos)
{
    StudioMainMenu* main = data;

    main->options->mapping = main->gamepads.mapping;
    showOptionsMenuPos(data, COUNT_OF(OptionMenu) - 3);
}

static void resetGamepadMenu(void* data, s32 pos);

static char MappingItems[TIC_BUTTONS][sizeof "RIGHT - RIGHT"];

static const char* const ButtonLabels[] = 
{
    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",
    "A",
    "B",
    "X",
    "Y",
};

enum{KeyMappingStart = 2};

static void assignMapping(void* data, s32 pos)
{
    StudioMainMenu* main = data;

    main->gamepads.key = pos - KeyMappingStart;

    static const char Fmt[] = "to assign to (%s) button...";
    static char str[sizeof Fmt + STRLEN("RIGHT")];

    static const MenuItem AssignKeyMenu[] =
    {
        {"Please, press a key you want"},
        {str},
    };

    sprintf(str, Fmt, ButtonLabels[main->gamepads.key]);

    studio_menu_init(main->menu, AssignKeyMenu, COUNT_OF(AssignKeyMenu), 1, 0, NULL, main);
}

static void initGamepadButtons(StudioMainMenu* menu)
{
    static const char* const KeysList[] =
    {
        "...",
        "A",    "B",    "C",    "D",    "E",    "F",    "G",    "H", 
        "I",    "J",    "K",    "L",    "M",    "N",    "O",    "P", 
        "Q",    "R",    "S",    "T",    "U",    "V",    "W",    "X", 
        "Y",    "Z",    "0",    "1",    "2",    "3",    "4",    "5", 
        "6",    "7",    "8",    "9",    "-",    "=",    "[",    "]", 
        "\\",   ";",    "'",    "`",    ",",    ".",    "/",    "SPCE", 
        "TAB",  "RET",  "BACKS","DEL",  "INS",  "PGUP", "PGDN", "HOME", 
        "END",  "UP",   "DOWN", "LEFT", "RIGHT","CAPS", "CTRL", "SHIFT", 
        "ALT",  "ESC",  "F1",   "F2",   "F3",   "F4",   "F5",   "F6", 
        "F7",   "F8",   "F9",   "F10",  "F11",  "F12",
    };

    for(s32 i = 0, index = menu->gamepads.index * TIC_BUTTONS; i != TIC_BUTTONS; ++i)
        sprintf(MappingItems[i], "%-5s - %-5s", ButtonLabels[i], KeysList[menu->gamepads.mapping.data[index++]]);
}

static s32 optionGamepadGet(void* data)
{
    StudioMainMenu* main = data;
    return main->gamepads.index;
}

static void optionGamepadSet(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    main->gamepads.index = pos;
    initGamepadButtons(main);
}

static MenuOption GamepadOption = 
{
    OPTION_VALUES({"1", "2", "3", "4"}),
    optionGamepadGet,
    optionGamepadSet,
};

static void initGamepadMenu(StudioMainMenu* main)
{
    static const MenuItem GamepadMenu[] =
    {
        {"GAMEPAD", NULL, &GamepadOption},
        {""},

        {MappingItems[0], assignMapping},
        {MappingItems[1], assignMapping},
        {MappingItems[2], assignMapping},
        {MappingItems[3], assignMapping},
        {MappingItems[4], assignMapping},
        {MappingItems[5], assignMapping},
        {MappingItems[6], assignMapping},
        {MappingItems[7], assignMapping},

        {""},
        {"SAVE MAPPING",        saveGamepadMenu},
        {"RESET TO DEFAULTS",   resetGamepadMenu},
        {"BACK",                showOptionsMenu, .back = true},
    };

    initGamepadButtons(main);

    studio_menu_init(main->menu, GamepadMenu, COUNT_OF(GamepadMenu), 
        main->gamepads.key < 0 ? KeyMappingStart : main->gamepads.key + KeyMappingStart, 
        COUNT_OF(OptionMenu) - 3, showOptionsMenu, main);

    main->gamepads.key = -1;
}

static void resetGamepadMenu(void* data, s32 pos)
{
    StudioMainMenu* main = data;
    main->gamepads.index = 0;
    ZEROMEM(main->gamepads.mapping);
    tic_sys_default_mapping(&main->gamepads.mapping);
    initGamepadMenu(main);
}
