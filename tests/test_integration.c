/*
 * test_integration.c — интеграционные тесты: ядро + логика вместе,
 * полный сценарий игры через очередь клавиш, как у настоящего игрока.
 */
#include <stdio.h>

#include "test.h"

/* Контрольная сумма экрана после сценария play_full_game(2026).
 * Одинакова на всех платформах: x86-64, i386 (DOS), MIPS big-endian, WASM.
 * Если вы намеренно изменили логику или отрисовку — обновите значение
 * (тест напечатает новое). */
#define GOLDEN_HASH 0x0221A045UL

/* Заставка -> игра ботом до проигрыша -> экран GAME OVER.
 * Бот (tests/bot.lua) «нажимает» клавишу только в такт перед шагом змейки.
 * Возвращает число сыгранных тактов. */
static int play_full_game(Engine *e) {
    int t = 0;
    ticks(e, KEY_ENTER, 1);
    while (val(e, "G.state == 'play'") && t < 20000) {
        int key = val(e, "G.timer == G.delay - 1") ? (int)val(e, "bot()") : KEY_NONE;
        ticks(e, key, 1);
        t++;
    }
    return t;
}

static void test_full_game(void) {
    Engine *e = make_game(2026);
    int t = play_full_game(e);
    long score = val(e, "G.score");
    unsigned long h = screen_hash(engine_screen(e));
    printf("    бот сыграл %d тактов, счёт %ld, хэш экрана 0x%08lX\n", t, score, h);
    CHECK(val(e, "G.state == 'over'"));
    CHECK(score >= 5);                  /* бот должен успеть что-то съесть */
    CHECK(val(e, "G.best") == score);   /* рекорд обновился */
    CHECK(engine_screen(e)->ch[10][28] == 'G'); /* на экране «GAME OVER» */
    CHECK(h == GOLDEN_HASH);            /* кадр совпадает с эталоном */

    /* Новая игра после проигрыша: счёт сброшен, рекорд сохранён */
    ticks(e, KEY_ENTER, 1);
    CHECK(val(e, "G.state == 'play'") && val(e, "G.score") == 0);
    CHECK(val(e, "G.best") == score);
    engine_free(e);
}

static void test_deterministic(void) {
    /* Одинаковый seed и одинаковые нажатия -> одинаковая игра */
    Engine *a = make_game(7), *b = make_game(7), *c = make_game(8);
    play_full_game(a);
    play_full_game(b);
    play_full_game(c);
    CHECK(screen_hash(engine_screen(a)) == screen_hash(engine_screen(b)));
    CHECK(screen_hash(engine_screen(a)) != screen_hash(engine_screen(c)));
    engine_free(a);
    engine_free(b);
    engine_free(c);
}

void suite_integration(void) {
    RUN(test_full_game);
    RUN(test_deterministic);
}
