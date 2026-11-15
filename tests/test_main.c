/*
 * test_main.c — запуск всех автотестов.
 *
 *   run_tests            — все тесты (200 прогонов тестирования свойств)
 *   run_tests N          — то же, но N прогонов тестирования свойств
 *
 * Код возврата 0 — все тесты прошли, 1 — есть ошибки.
 * Тесты не используют платформенный слой: работают только ядро и логика,
 * поэтому один и тот же набор запускается на x86-64, MIPS (big-endian),
 * Windows (через Wine) и т.д.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "game_lua.h"
#include "invariants_lua.h"
#include "bot_lua.h"

#ifdef _WIN32
#include <windows.h> /* чтобы русский текст выводился в консоль Windows */
#endif

int tests_failed = 0, checks_total = 0;

Engine *make_game(unsigned long seed) {
    char err[512];
    Engine *e = engine_new(game_lua, sizeof game_lua, "snake.lua", seed, err, sizeof err);
    if (!e) {
        printf("Не удалось загрузить игру: %s\n", err);
        exit(1);
    }
    if (!engine_run(e, invariants_lua, sizeof invariants_lua, "invariants.lua")) {
        printf("Не удалось загрузить инварианты: %s\n", engine_error(e));
        exit(1);
    }
    if (!engine_run(e, bot_lua, sizeof bot_lua, "bot.lua")) {
        printf("Не удалось загрузить бота: %s\n", engine_error(e));
        exit(1);
    }
    return e;
}

void lua(Engine *e, const char *code) {
    if (!engine_run(e, code, strlen(code), "test")) {
        printf("    Ошибка Lua: %s\n", engine_error(e));
        tests_failed++;
    }
}

long val(Engine *e, const char *expr) { return engine_eval_int(e, expr); }

void ticks(Engine *e, int key, int n) {
    int i;
    engine_push_key(e, key);
    for (i = 0; i < n; i++) engine_tick(e);
}

static const char *byte_order(void) {
    unsigned int one = 1;
    return *(unsigned char *)&one ? "little-endian" : "big-endian";
}

int main(int argc, char **argv) {
    int runs = argc > 1 ? atoi(argv[1]) : 200;
#ifdef _WIN32
    SetConsoleOutputCP(65001); /* UTF-8 */
#endif

    printf("Платформа: %d-битные указатели, %s\n", (int)(sizeof(void *) * 8), byte_order());

    printf("Модульные тесты ядра:\n");
    suite_core();
    printf("Модульные тесты логики:\n");
    suite_logic();
    printf("Интеграционные тесты:\n");
    suite_integration();
    printf("Тестирование свойств:\n");
    suite_properties(runs);

    printf("\nПроверок: %d, ошибок: %d — %s\n", checks_total, tests_failed,
           tests_failed ? "ЕСТЬ ОШИБКИ" : "ВСЕ ТЕСТЫ ПРОШЛИ");
    return tests_failed ? 1 : 0;
}
