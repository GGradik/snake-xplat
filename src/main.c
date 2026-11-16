/*
 * main.c — главный цикл игры. Один и тот же для всех платформ.
 *
 * Каждую итерацию: читаем клавиши -> раз в TICK_MS мс делаем такт
 * игровой логики -> выводим экран. В браузере цикл крутит сам браузер
 * (emscripten_set_main_loop), на остальных платформах — обычный while.
 */
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "platform.h"
#include "game_lua.h" /* скрипт snake.lua, встроенный в программу при сборке */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static Engine *game;
static unsigned long next_tick;
static int running = 1;

static void fail(const char *msg) {
    plat_quit();
    fprintf(stderr, "Ошибка: %s\n", msg);
    exit(1);
}

/* Одна итерация цикла. */
static void frame(void) {
    int key;
    while ((key = plat_key()) != KEY_NONE) {
        if (key == KEY_QUIT) running = 0;
        else engine_push_key(game, key);
    }
    /* Если программа «уснула» (свернули окно, вкладка в фоне) —
     * не пытаемся догнать пропущенные такты, а продолжаем с текущего. */
    if (plat_ticks() > next_tick + 1000) next_tick = plat_ticks();
    if (plat_ticks() >= next_tick) {
        next_tick += TICK_MS;
        if (!engine_tick(game)) fail(engine_error(game));
        plat_present(engine_screen(game));
    }
}

int main(void) {
    char err[512];
    if (plat_init() != 0) {
        fprintf(stderr, "Не удалось инициализировать платформу\n");
        return 1;
    }
    game = engine_new(game_lua, sizeof game_lua, "snake.lua", plat_ticks(),
                      err, sizeof err);
    if (!game) fail(err);
    next_tick = plat_ticks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (running) {
        frame();
        plat_sleep(5);
    }
    plat_quit();
    engine_free(game);
#endif
    return 0;
}
