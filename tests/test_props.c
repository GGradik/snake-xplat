/*
 * test_props.c — тестирование свойств (property-based testing).
 *
 * Идея: вместо того чтобы придумывать отдельные ситуации вручную,
 * генерируем тысячи СЛУЧАЙНЫХ последовательностей нажатий и после
 * каждого такта проверяем свойства-инварианты (tests/invariants.lua),
 * которые должны выполняться ВСЕГДА.
 *
 * Если свойство нарушено, найденная последовательность «уменьшается»
 * (shrinking): из неё по одному убираются нажатия, пока ошибка
 * сохраняется. В итоге печатается короткий пример, который легко
 * воспроизвести и отладить.
 */
#include <stdio.h>
#include <string.h>

#include "test.h"

#define STEPS 1200 /* тактов в одном прогоне (1 минута игры) */

/* Свой генератор случайных чисел (LCG): одинаковые числа на любой
 * платформе, в отличие от rand() из стандартной библиотеки. */
static unsigned long rng;
static int next_rand(int n) {
    rng = (rng * 1103515245UL + 12345UL) & 0xFFFFFFFFUL;
    return (int)((rng >> 16) % (unsigned long)n);
}

/* Последний найденный (уже уменьшенный) контрпример. */
static int ex_keys[STEPS], ex_n;
static unsigned long ex_seed;

/* Генератор нажатий. Чисто случайные нажатия плохи: змейка почти ничего
 * не съедает и остаётся короткой, а короткая змейка не может врезаться
 * в себя. Поэтому смешиваем: чаще всего ничего не нажато, иногда
 * случайная клавиша, а иногда ход «бота», который ведёт змейку к еде
 * (tests/bot.lua) — так змейка растёт и попадает в сложные ситуации. */
static int generate_key(Engine *e) {
    static const int keys[] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER};
    int r = next_rand(100);
    if (r < 60) return KEY_NONE;
    if (r < 70) return keys[next_rand(5)];
    return (int)val(e, "bot()");
}

/* Прогнать игру с seed на нажатиях keys[0..n). Проверить свойство prop
 * (Lua-выражение) после каждого такта. Вернуть номер такта, на котором
 * свойство нарушилось, или -1, если не нарушилось ни разу. */
static int replay(unsigned long seed, const int *keys, int n, const char *prop) {
    Engine *e = make_game(seed);
    int t, failed_at = -1;
    for (t = 0; t < n; t++) {
        engine_push_key(e, keys[t]);
        engine_tick(e);
        if (val(e, prop) != 1) {
            failed_at = t;
            break;
        }
    }
    engine_free(e);
    return failed_at;
}

/* Уменьшение контрпримера: убираем нажатия после ошибки, затем пытаемся
 * заменить каждое оставшееся нажатие на «ничего не нажато». Если ошибка
 * всё равно происходит (пусть и позже) — нажатие было лишним.
 * Возвращает длину примера в тактах. */
static int shrink(unsigned long seed, int *keys, int total, const char *prop) {
    int i, saved, changed, n = replay(seed, keys, total, prop) + 1;
    for (i = n; i < total; i++) keys[i] = KEY_NONE;
    /* Повторяем проходы, пока удаётся что-то убрать: нажатие может стать
     * лишним только после удаления другого. */
    do {
        changed = 0;
        for (i = n - 1; i >= 0; i--) {
            if (keys[i] == KEY_NONE) continue;
            saved = keys[i];
            keys[i] = KEY_NONE;
            if (replay(seed, keys, total, prop) < 0)
                keys[i] = saved; /* без этого нажатия ошибки нет — вернуть */
            else
                changed = 1;
        }
    } while (changed);
    return replay(seed, keys, total, prop) + 1;
}

static void print_example(unsigned long seed, const int *keys, int n) {
    static const char *names[] = {"-", "up", "down", "left", "right", "enter", "quit"};
    int i;
    printf("    контрпример: seed=%lu, %d тактов, нажатия:", seed, n);
    for (i = 0; i < n; i++)
        if (keys[i] != KEY_NONE) printf(" %d:%s", i, names[keys[i]]);
    printf("\n");
}

/* Проверить свойство prop на runs случайных прогонах.
 * Возвращает число нажатий в уменьшенном контрпримере или -1, если
 * контрпример не найден. */
static int check_property(const char *prop, int runs, int verbose) {
    static int keys[STEPS];
    int r, t, n, presses, failed;
    for (r = 0; r < runs; r++) {
        unsigned long seed = (unsigned long)r + 1;
        Engine *e = make_game(seed);
        rng = seed * 2654435761UL;
        /* Играем, записывая нажатия, пока свойство не нарушится */
        for (failed = 0, t = 0; t < STEPS && !failed; t++) {
            keys[t] = generate_key(e);
            engine_push_key(e, keys[t]);
            engine_tick(e);
            failed = val(e, prop) != 1;
        }
        engine_free(e);
        if (!failed) continue;

        for (; t < STEPS; t++) keys[t] = KEY_NONE;
        n = shrink(seed, keys, STEPS, prop);
        if (verbose) print_example(seed, keys, n);
        memcpy(ex_keys, keys, sizeof keys[0] * (size_t)n); /* запомнить пример */
        ex_n = n;
        ex_seed = seed;
        for (presses = 0, t = 0; t < n; t++) presses += keys[t] != KEY_NONE;
        return presses;
    }
    return -1;
}

/* Главное свойство: все инварианты из invariants.lua выполняются всегда. */
static void test_invariants_hold(int runs) {
    int found = check_property("check_invariants()", runs, 1);
    if (found >= 0) {
        /* Повторить контрпример и напечатать, какой инвариант нарушен */
        Engine *e = make_game(ex_seed);
        int t;
        for (t = 0; t < ex_n; t++) ticks(e, ex_keys[t], 1), val(e, "check_invariants()");
        lua(e, "print('    нарушено: ' .. tostring(INV_ERR))");
        engine_free(e);
    }
    printf("    %d прогонов по %d тактов = %ld проверок инвариантов\n",
           runs, STEPS, (long)runs * STEPS);
    CHECK(found < 0);
}

/* Проверка самого механизма: заведомо ложное свойство «голова никогда
 * не заходит в правую часть поля (x >= 20)» должно быть опровергнуто,
 * а контрпример уменьшен до одного нажатия — Enter (дальше змейка
 * сама доедет до x = 20, двигаясь вправо). */
static void test_shrinking_finds_minimal_example(void) {
    int presses = check_property("G.snake[1].x < 20", 50, 1);
    CHECK(presses == 1);
}

static int prop_runs;
static void test_invariants(void) { test_invariants_hold(prop_runs); }

void suite_properties(int runs) {
    prop_runs = runs;
    RUN(test_invariants);
    RUN(test_shrinking_finds_minimal_example);
}
