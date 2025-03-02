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

#include "core/core.h"

// Fennel requires Lua
#if defined(TIC_BUILD_WITH_LUA)

#include "lua_api.h"

#if defined(TIC_BUILD_WITH_FENNEL)

#include "fennel.h"

#define FENNEL_CODE(...) #__VA_ARGS__

static const char* execute_fennel_src = FENNEL_CODE(
  local fennel = require("fennel")
  debug.traceback = fennel.traceback
  local opts = {filename="game", allowedGlobals = false}
  local src = ...
  if(src:find("\n;; strict: true")) then opts.allowedGlobals = nil end
  local ok, msg = pcall(fennel.eval, src, opts)
  if(not ok) then return msg end
);

static bool initFennel(tic_mem* tic, const char* code)
{
    tic_core* core = (tic_core*)tic;
    closeLua(tic);

    lua_State* lua = core->currentVM = luaL_newstate();
    lua_open_builtins(lua);

    initLuaAPI(core);

    {
        lua_State* fennel = core->currentVM;

        lua_settop(fennel, 0);

        if (luaL_loadbuffer(fennel, (const char *)loadfennel_lua,
                            loadfennel_lua_len, "fennel.lua") != LUA_OK)
        {
            core->data->error(core->data->data, "failed to load fennel compiler");
            return false;
        }

        lua_call(fennel, 0, 0);

        if (luaL_loadbuffer(fennel, execute_fennel_src, strlen(execute_fennel_src), "execute_fennel") != LUA_OK)
        {
            core->data->error(core->data->data, "failed to load fennel compiler");
            return false;
        }

        lua_pushstring(fennel, code);
        lua_call(fennel, 1, 1);
        const char* err = lua_tostring(fennel, -1);

        if (err)
        {
            core->data->error(core->data->data, err);
            return false;
        }
    }

    return true;
}

static const char* const FennelKeywords [] =
{
    "lua", "hashfn","macro", "macros", "macroexpand", "macrodebug",
    "do", "values", "if", "when", "each", "for", "fn", "lambda", "partial",
    "while", "set", "global", "var", "local", "let", "tset", "doto", "match",
    "or", "and", "true", "false", "nil", "not", "not=", "length", "set-forcibly!",
    "rshift", "lshift", "bor", "band", "bnot", "bxor", "pick-values", "pick-args",
    ".", "..", "#", "...", ":", "->", "->>", "-?>", "-?>>", "$", "with-open"
};

static const tic_outline_item* getFennelOutline(const char* code, s32* size)
{
    enum{Size = sizeof(tic_outline_item)};

    *size = 0;

    static tic_outline_item* items = NULL;

    if(items)
    {
        free(items);
        items = NULL;
    }

    const char* ptr = code;

    while(true)
    {
        static const char FuncString[] = "(fn ";

        ptr = strstr(ptr, FuncString);

        if(ptr)
        {
            ptr += sizeof FuncString - 1;

            const char* start = ptr;
            const char* end = start;

            while(*ptr)
            {
                char c = *ptr;

                if(c == ' ' || c == '\t' || c == '\n' || c == '[')
                {
                    end = ptr;
                    break;
                }

                ptr++;
            }

            if(end > start)
            {
                items = realloc(items, (*size + 1) * Size);

                items[*size].pos = start;
                items[*size].size = (s32)(end - start);

                (*size)++;
            }
        }
        else break;
    }

    return items;
}

static void evalFennel(tic_mem* tic, const char* code) {
    tic_core* core = (tic_core*)tic;
    lua_State* fennel = core->currentVM;

    lua_settop(fennel, 0);

    if (luaL_loadbuffer(fennel, execute_fennel_src, strlen(execute_fennel_src), "execute_fennel") != LUA_OK)
    {
        core->data->error(core->data->data, "failed to load fennel compiler");
    }

    lua_pushstring(fennel, code);
    lua_call(fennel, 1, 1);
    const char* err = lua_tostring(fennel, -1);

    if (err)
    {
        core->data->error(core->data->data, err);
    }
}

tic_script_config FennelSyntaxConfig =
{
    .name               = "fennel",
    .fileExtension      = ".fnl",
    .projectComment     = ";;",
    .init               = initFennel,
    .close              = closeLua,
    .tick               = callLuaTick,
    .callback           =
    {
        .scanline       = callLuaScanline,
        .border         = callLuaBorder,
    },

    .getOutline         = getFennelOutline,
    .eval               = evalFennel,

    .blockCommentStart  = NULL,
    .blockCommentEnd    = NULL,
    .blockCommentStart2 = NULL,
    .blockCommentEnd2   = NULL,
    .blockStringStart   = NULL,
    .blockStringEnd     = NULL,
    .singleComment      = ";",
    .blockEnd           = NULL,

    .keywords           = FennelKeywords,
    .keywordsCount      = COUNT_OF(FennelKeywords),
};

#endif /* defined(TIC_BUILD_WITH_FENNEL) */

#endif
