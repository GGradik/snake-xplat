/*
 * engine.c — ядро: связывает интерпретатор Lua с экранным буфером
 * и очередью клавиш. Не содержит ни одной строчки, зависящей от ОС.
 *
 * Lua-скрипт игры должен определить три функции:
 *   init()        — вызывается один раз при старте;
 *   update(key)   — каждый такт (key: "up", "down", "left", "right",
 *                   "enter" или nil, если ничего не нажато);
 *   draw()        — каждый такт после update(), рисует кадр.
 *
 * Скрипту доступны функции рисования:
 *   cls([bg])                        — очистить экран;
 *   text(x, y, str [, fg [, bg]])    — вывести строку;
 *   fill(x, y, w, h [, fg [, bg [, ch]]]) — закрасить прямоугольник.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

struct Engine {
    lua_State *L;
    Screen screen;
    int queue[KEY_QUEUE_SIZE]; /* кольцевая очередь клавиш */
    int qhead, qlen;
    char error[512];
};

/* ---------- Экранный буфер ---------- */

void screen_clear(Screen *s, int attr) {
    memset(s->ch, ' ', sizeof s->ch);
    memset(s->attr, attr & 0xFF, sizeof s->attr);
}

void screen_text(Screen *s, int x, int y, const char *str, int attr) {
    if (y < 0 || y >= SCREEN_H) return;
    for (; *str; str++, x++) {
        if (x < 0) continue;
        if (x >= SCREEN_W) break;
        s->ch[y][x] = (unsigned char)*str;
        s->attr[y][x] = (unsigned char)attr;
    }
}

void screen_fill(Screen *s, int x, int y, int w, int h, int ch, int attr) {
    int i, j;
    for (j = y; j < y + h; j++) {
        if (j < 0 || j >= SCREEN_H) continue;
        for (i = x; i < x + w; i++) {
            if (i < 0 || i >= SCREEN_W) continue;
            s->ch[j][i] = (unsigned char)ch;
            s->attr[j][i] = (unsigned char)attr;
        }
    }
}

/* FNV-1a: одинаковый результат на любой платформе, т.к. считаем в 32 битах. */
unsigned long screen_hash(const Screen *s) {
    unsigned long h = 2166136261UL;
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++) {
            h = ((h ^ s->ch[y][x]) * 16777619UL) & 0xFFFFFFFFUL;
            h = ((h ^ s->attr[y][x]) * 16777619UL) & 0xFFFFFFFFUL;
        }
    return h;
}

/* ---------- Функции, доступные из Lua ---------- */

static Engine *self(lua_State *L) {
    return (Engine *)lua_touserdata(L, lua_upvalueindex(1));
}

static int make_attr(lua_State *L, int fg_idx, int bg_idx) {
    int fg = (int)luaL_optinteger(L, fg_idx, C_LGRAY) & 15;
    int bg = (int)luaL_optinteger(L, bg_idx, C_BLACK) & 15;
    return bg * 16 + fg;
}

static int l_cls(lua_State *L) {
    int bg = (int)luaL_optinteger(L, 1, C_BLACK) & 15;
    screen_clear(&self(L)->screen, bg * 16 + C_LGRAY);
    return 0;
}

static int l_text(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    const char *str = luaL_checkstring(L, 3);
    screen_text(&self(L)->screen, x, y, str, make_attr(L, 4, 5));
    return 0;
}

static int l_fill(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    int attr = make_attr(L, 5, 6);
    const char *ch = luaL_optstring(L, 7, " ");
    screen_fill(&self(L)->screen, x, y, w, h, ch[0] ? ch[0] : ' ', attr);
    return 0;
}

/* ---------- Вспомогательное ---------- */

/* Добавляет к ошибке стек вызовов Lua — так проще найти ошибку в скрипте. */
static int traceback(lua_State *L) {
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

/* Вызвать функцию на вершине стека с nargs аргументами, перехватив ошибку. */
static int call(Engine *e, int nargs) {
    lua_State *L = e->L;
    int base = lua_gettop(L) - nargs;
    int rc;
    lua_pushcfunction(L, traceback);
    lua_insert(L, base);
    rc = lua_pcall(L, nargs, 0, base);
    lua_remove(L, base);
    if (rc != LUA_OK) {
        snprintf(e->error, sizeof e->error, "%s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }
    return 1;
}

/* Вызвать глобальную функцию name без аргументов или с одним строковым. */
static int call_global(Engine *e, const char *name, const char *arg) {
    if (lua_getglobal(e->L, name) != LUA_TFUNCTION) {
        lua_pop(e->L, 1);
        snprintf(e->error, sizeof e->error, "в скрипте нет функции %s()", name);
        return 0;
    }
    if (arg) lua_pushstring(e->L, arg); else lua_pushnil(e->L);
    return call(e, 1);
}

static void open_libs(lua_State *L) {
    /* Только безопасные библиотеки: без io, os, package, debug. */
    static const luaL_Reg libs[] = {
        {LUA_GNAME, luaopen_base},       {LUA_COLIBNAME, luaopen_coroutine},
        {LUA_MATHLIBNAME, luaopen_math}, {LUA_STRLIBNAME, luaopen_string},
        {LUA_TABLIBNAME, luaopen_table}, {LUA_UTF8LIBNAME, luaopen_utf8},
        {NULL, NULL}};
    const luaL_Reg *lib;
    for (lib = libs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);
    }
}

static void register_api(Engine *e) {
    static const luaL_Reg api[] = {
        {"cls", l_cls}, {"text", l_text}, {"fill", l_fill}, {NULL, NULL}};
    lua_pushglobaltable(e->L);
    lua_pushlightuserdata(e->L, e);
    luaL_setfuncs(e->L, api, 1); /* каждая функция получает Engine* как upvalue */
    lua_pop(e->L, 1);
}

/* ---------- Публичный интерфейс ---------- */

Engine *engine_new(const char *code, size_t len, const char *name,
                   unsigned long seed, char *err, size_t errsize) {
    Engine *e = (Engine *)calloc(1, sizeof *e);
    if (!e) return NULL;
    e->L = luaL_newstate();
    if (!e->L) { free(e); return NULL; }
    open_libs(e->L);
    register_api(e);
    screen_clear(&e->screen, C_LGRAY);

    /* math.randomseed(seed) — одинаковый seed даёт одинаковую игру везде. */
    lua_getglobal(e->L, "math");
    lua_getfield(e->L, -1, "randomseed");
    lua_pushinteger(e->L, (lua_Integer)seed);
    lua_call(e->L, 1, 0);
    lua_pop(e->L, 1);

    if (!engine_run(e, code, len, name) || !call_global(e, "init", NULL)) {
        if (err) snprintf(err, errsize, "%s", e->error);
        engine_free(e);
        return NULL;
    }
    return e;
}

void engine_free(Engine *e) {
    if (!e) return;
    lua_close(e->L);
    free(e);
}

void engine_push_key(Engine *e, int key) {
    if (key == KEY_NONE || e->qlen == KEY_QUEUE_SIZE) return;
    e->queue[(e->qhead + e->qlen) % KEY_QUEUE_SIZE] = key;
    e->qlen++;
}

static int pop_key(Engine *e) {
    int key;
    if (e->qlen == 0) return KEY_NONE;
    key = e->queue[e->qhead];
    e->qhead = (e->qhead + 1) % KEY_QUEUE_SIZE;
    e->qlen--;
    return key;
}

int engine_tick(Engine *e) {
    static const char *names[] = {NULL, "up", "down", "left", "right", "enter", NULL};
    int key = pop_key(e);
    return call_global(e, "update", names[key]) && call_global(e, "draw", NULL);
}

const Screen *engine_screen(const Engine *e) { return &e->screen; }

const char *engine_error(const Engine *e) { return e->error; }

int engine_run(Engine *e, const char *code, size_t len, const char *name) {
    if (luaL_loadbufferx(e->L, code, len, name, "t") != LUA_OK) {
        snprintf(e->error, sizeof e->error, "%s", lua_tostring(e->L, -1));
        lua_pop(e->L, 1);
        return 0;
    }
    return call(e, 0);
}

long engine_eval_int(Engine *e, const char *expr) {
    lua_State *L = e->L;
    long result = EVAL_ERROR;
    lua_pushstring(L, "return ");
    lua_pushstring(L, expr);
    lua_concat(L, 2);
    if (luaL_loadstring(L, lua_tostring(L, -1)) == LUA_OK &&
        lua_pcall(L, 0, 1, 0) == LUA_OK) {
        if (lua_isboolean(L, -1) || lua_isnil(L, -1))
            result = lua_toboolean(L, -1);
        else if (lua_isinteger(L, -1))
            result = (long)lua_tointeger(L, -1);
    } else {
        snprintf(e->error, sizeof e->error, "%s", lua_tostring(L, -1));
    }
    lua_pop(L, 2);
    return result;
}
