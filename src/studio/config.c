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

#include "config.h"
#include "fs.h"
#include "cart.h"

#if defined (TIC_BUILD_WITH_LUA)
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static void readBool(lua_State* lua, const char* name, bool* val)
{
    lua_getfield(lua, -1, name);

    if (lua_isboolean(lua, -1))
        *val = lua_toboolean(lua, -1);

    lua_pop(lua, 1);
}

static void readInteger(lua_State* lua, const char* name, s32* val)
{
    lua_getfield(lua, -1, name);

    if (lua_isinteger(lua, -1))
        *val = lua_tointeger(lua, -1);

    lua_pop(lua, 1);
}

static void readByte(lua_State* lua, const char* name, u8* val)
{
    s32 res = *val;
    readInteger(lua, name, &res);
    *val = res;
}

static void readGlobalInteger(lua_State* lua, const char* name, s32* val)
{
    lua_getglobal(lua, name);

    if (lua_isinteger(lua, -1))
        *val = lua_tointeger(lua, -1);

    lua_pop(lua, 1);
}

static void readGlobalBool(lua_State* lua, const char* name, bool* val)
{
    lua_getglobal(lua, name);

    if (lua_isboolean(lua, -1))
        *val = lua_toboolean(lua, -1);

    lua_pop(lua, 1);
}

#if defined(CRT_SHADER_SUPPORT)

static void readString(lua_State* lua, const char* name, const char** val)
{
    lua_getfield(lua, -1, name);

    if (lua_isstring(lua, -1))
        *val = strdup(lua_tostring(lua, -1));

    lua_pop(lua, 1);
}

static void readConfigCrtShader(Config* config, lua_State* lua)
{
    lua_getglobal(lua, "CRT_SHADER");

    if(lua_type(lua, -1) == LUA_TTABLE)
    {
        readString(lua, "VERTEX", &config->data.shader.vertex);
        readString(lua, "PIXEL", &config->data.shader.pixel);
    }

#if defined (EMSCRIPTEN)
    // WebGL supports only version 100 shaders.
    // Luckily, the format is nearly identical.
    // This code detects the incompatible line(s) at
    // the beginning of each shader and patches them
    // in-place in memory.
    char *s = (char *)config->data.shader.vertex;
    if (strncmp("\t\t#version 110", s, 14) == 0) {
        // replace the two tabs, with a "//" comment, disabling the #version tag.
        s[0] = '/';
        s[1] = '/';
    }
    s = (char *)config->data.shader.pixel;
    if (strncmp("\t\t#version 110\n\t\t//precision highp float;", s, 41) == 0) {
        // replace the two tabs, with a "//" comment, disabling the #version tag.
        s[0] = '/';
        s[1] = '/';
        // replace the "//" comment with spaces, enabling the precision statement.
        s[17] = ' ';
        s[18] = ' ';
    }
#endif

    lua_pop(lua, 1);        
}

#endif

static void readCodeTheme(Config* config, lua_State* lua)
{
    lua_getfield(lua, -1, "CODE");

    if(lua_type(lua, -1) == LUA_TTABLE)
    {

#define CODE_COLOR_DEF(VAR) readByte(lua, #VAR, &config->data.theme.code.VAR);
        CODE_COLORS_LIST(CODE_COLOR_DEF)
#undef  CODE_COLOR_DEF

        readByte(lua, "SELECT", &config->data.theme.code.select);
        readByte(lua, "CURSOR", &config->data.theme.code.cursor);

        readBool(lua, "SHADOW", &config->data.theme.code.shadow);
        readBool(lua, "ALT_FONT", &config->data.theme.code.altFont);
        readBool(lua, "MATCH_DELIMITERS", &config->data.theme.code.matchDelimiters);
    }

    lua_pop(lua, 1);
}

static void readGamepadTheme(Config* config, lua_State* lua)
{
    lua_getfield(lua, -1, "GAMEPAD");

    if(lua_type(lua, -1) == LUA_TTABLE)
    {
        lua_getfield(lua, -1, "TOUCH");

        if(lua_type(lua, -1) == LUA_TTABLE)
        {
            readByte(lua, "ALPHA", &config->data.theme.gamepad.touch.alpha);
        }

        lua_pop(lua, 1);
    }

    lua_pop(lua, 1);
}

static void readTheme(Config* config, lua_State* lua)
{
    lua_getglobal(lua, "THEME");

    if(lua_type(lua, -1) == LUA_TTABLE)
    {
        readCodeTheme(config, lua);
        readGamepadTheme(config, lua);
    }

    lua_pop(lua, 1);
}

static void readConfig(Config* config)
{
    lua_State* lua = luaL_newstate();

    if(lua)
    {
        if(luaL_loadstring(lua, config->cart->code.data) == LUA_OK && lua_pcall(lua, 0, LUA_MULTRET, 0) == LUA_OK)
        {
            readGlobalInteger(lua,  "GIF_LENGTH",           &config->data.gifLength);
            readGlobalInteger(lua,  "GIF_SCALE",            &config->data.gifScale);
            readGlobalBool(lua,     "CHECK_NEW_VERSION",    &config->data.checkNewVersion);
            readGlobalInteger(lua,  "UI_SCALE",             &config->data.uiScale);
            readGlobalBool(lua,     "SOFTWARE_RENDERING",   &config->data.soft);

#if defined(CRT_SHADER_SUPPORT)
            readConfigCrtShader(config, lua);
#endif
            readTheme(config, lua);
        }

        lua_close(lua);
    }
}
#else

static void readConfig(Config* config)
{
    config->data = (StudioConfig)
    {
        .uiScale = 4,
        .cart = config->cart,
    };
}

#endif

static void update(Config* config, const u8* buffer, s32 size)
{
    tic_cart_load(config->cart, buffer, size);

    readConfig(config);
    studioConfigChanged(config->studio);
}

static void setDefault(Config* config)
{
    config->data = (StudioConfig)
    {
        .cart = config->cart,
        .options = 
        {
#if defined(CRT_SHADER_SUPPORT)
            .crt        = false,
#endif
            .volume     = MAX_VOLUME,
            .vsync      = true,
            .fullscreen = false,
#if defined(BUILD_EDITORS)
            .devmode    = false,
#endif
        },
    };

    tic_sys_default_mapping(&config->data.options.mapping);

    {
        static const u8 ConfigZip[] =
        {
            #include "../build/assets/config.tic.dat"
        };

        u8* data = malloc(sizeof(tic_cartridge));

        SCOPE(free(data))
        {
            update(config, data, tic_tool_unzip(data, sizeof(tic_cartridge), ConfigZip, sizeof ConfigZip));
        }
    }
}

static void saveConfig(Config* config, bool overwrite)
{
    u8* buffer = malloc(sizeof(tic_cartridge));

    if(buffer)
    {
        s32 size = tic_cart_save(config->data.cart, buffer);

        tic_fs_saveroot(config->fs, CONFIG_TIC_PATH, buffer, size, overwrite);

        free(buffer);
    }
}

static void reset(Config* config)
{
    setDefault(config);
    saveConfig(config, true);
}

static void save(Config* config)
{
    *config->cart = config->tic->cart;
    readConfig(config);
    saveConfig(config, true);

    studioConfigChanged(config->studio);
}

static const char OptionsDatPath[] = TIC_LOCAL_VERSION "options.dat";

static void loadConfigData(tic_fs* fs, const char* path, void* dst, s32 size)
{
    s32 dataSize = 0;
    u8* data = (u8*)tic_fs_loadroot(fs, path, &dataSize);

    if(data) SCOPE(free(data))
        if(dataSize == size)
            memcpy(dst, data, size);
}

void initConfig(Config* config, Studio* studio, tic_fs* fs)
{
    *config = (Config)
    {
        .studio = studio,
        .tic = getMemory(studio),
        .cart = realloc(config->cart, sizeof(tic_cartridge)),
        .save = save,
        .reset = reset,
        .fs = fs,
    };

    setDefault(config);

    // read config.tic
    {
        s32 size = 0;
        u8* data = (u8*)tic_fs_loadroot(fs, CONFIG_TIC_PATH, &size);

        if(data)
        {
            update(config, data, size);

            free(data);
        }
        else saveConfig(config, false);        
    }

    loadConfigData(fs, OptionsDatPath, &config->data.options, sizeof config->data.options);

    tic_api_reset(config->tic);
}

void freeConfig(Config* config)
{
    tic_fs_saveroot(config->fs, OptionsDatPath, &config->data.options, sizeof config->data.options, true);

    free(config->cart);

#if defined(CRT_SHADER_SUPPORT)

    free((void*)config->data.shader.vertex);
    free((void*)config->data.shader.pixel);
#endif

    free(config);
}
