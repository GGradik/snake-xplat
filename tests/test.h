/*
 * test.h — минимальный фреймворк для модульных тестов на C.
 *
 *   CHECK(условие)  — проверка; при ошибке печатает файл, строку и условие.
 *   RUN(функция)    — запускает тест и печатает его имя.
 */
#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include "engine.h"

extern int tests_failed, checks_total;

#define CHECK(cond)                                                       \
    do {                                                                  \
        checks_total++;                                                   \
        if (!(cond)) {                                                    \
            tests_failed++;                                               \
            printf("    ОШИБКА %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                 \
    } while (0)

#define RUN(fn)                     \
    do {                            \
        printf("  %s\n", #fn);      \
        fn();                       \
    } while (0)

/* Создать ядро с игрой snake.lua, инвариантами и ботом (для всех тестов). */
Engine *make_game(unsigned long seed);

/* Выполнить Lua-код в состоянии игры (для подготовки ситуаций в тестах). */
void lua(Engine *e, const char *code);

/* Значение Lua-выражения как целое число. */
long val(Engine *e, const char *expr);

/* n тактов игры, перед первым — нажатие key (KEY_NONE — без нажатия). */
void ticks(Engine *e, int key, int n);

void suite_core(void);
void suite_logic(void);
void suite_integration(void);
void suite_properties(int runs);

#endif
