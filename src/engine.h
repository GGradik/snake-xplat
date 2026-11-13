/*
 * engine.h — платформо-независимое ядро игры.
 *
 * Ядро запускает игровую логику, написанную на Lua, и даёт ей:
 *   - экранный буфер 80x25 символов (как текстовый режим DOS);
 *   - очередь нажатых клавиш.
 * Про конкретную ОС ядро ничего не знает.
 */
#ifndef ENGINE_H
#define ENGINE_H

#include <stddef.h>

#define SCREEN_W 80
#define SCREEN_H 25

/* Цвета в стиле CGA/EGA: 16 цветов, атрибут = фон * 16 + цвет символа. */
enum {
    C_BLACK, C_BLUE, C_GREEN, C_CYAN, C_RED, C_MAGENTA, C_BROWN, C_LGRAY,
    C_DGRAY, C_LBLUE, C_LGREEN, C_LCYAN, C_LRED, C_LMAGENTA, C_YELLOW, C_WHITE
};

/* Логические клавиши — одинаковые для всех платформ. */
enum { KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_QUIT };

/* Экран: символ и цветовой атрибут для каждой клетки. */
typedef struct {
    unsigned char ch[SCREEN_H][SCREEN_W];
    unsigned char attr[SCREEN_H][SCREEN_W];
} Screen;

#define KEY_QUEUE_SIZE 8
#define TICK_MS 50 /* игра обновляется 20 раз в секунду */

typedef struct Engine Engine;

/* Создать ядро и выполнить скрипт игры (код, длина, имя для сообщений).
 * seed — начальное значение генератора случайных чисел.
 * При ошибке возвращает NULL, а текст ошибки кладёт в err (если не NULL). */
Engine *engine_new(const char *code, size_t len, const char *name,
                   unsigned long seed, char *err, size_t errsize);
void engine_free(Engine *e);

/* Добавить клавишу в очередь (лишние клавиши отбрасываются). */
void engine_push_key(Engine *e, int key);

/* Один такт игры: взять клавишу из очереди, вызвать update() и draw().
 * Возвращает 1 при успехе, 0 при ошибке в скрипте. */
int engine_tick(Engine *e);

/* Экранный буфер, который нарисовала логика. */
const Screen *engine_screen(const Engine *e);

/* Текст последней ошибки Lua (пустая строка, если ошибок нет). */
const char *engine_error(const Engine *e);

/* Для тестов: выполнить дополнительный Lua-код в том же состоянии. */
int engine_run(Engine *e, const char *code, size_t len, const char *name);

/* Для тестов: вычислить Lua-выражение и вернуть его как целое/логическое
 * (true = 1, false/nil = 0). При ошибке возвращает EVAL_ERROR. */
#define EVAL_ERROR (-2147483647L)
long engine_eval_int(Engine *e, const char *expr);

/* Контрольная сумма экрана (FNV-1a) — для сравнения между платформами. */
unsigned long screen_hash(const Screen *s);

/* Низкоуровневые операции с буфером (используются и из Lua, и в тестах). */
void screen_clear(Screen *s, int attr);
void screen_text(Screen *s, int x, int y, const char *str, int attr);
void screen_fill(Screen *s, int x, int y, int w, int h, int ch, int attr);

#endif
