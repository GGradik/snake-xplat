/*
 * test_core.c — модульные тесты ядра (engine.c), без игровой логики.
 */
#include <string.h>

#include "test.h"

/* Создать ядро с произвольным маленьким скриптом. */
static Engine *make_script(const char *code, char *err, size_t errsize) {
    return engine_new(code, strlen(code), "test.lua", 1, err, errsize);
}

static const char *MINI =
    "function init() end\n"
    "function update(k) last = k end\n"
    "function draw() cls() text(0, 0, 'hi', 14, 1) end\n";

static void test_clear_fills_spaces(void) {
    Screen s;
    screen_clear(&s, 0x17);
    CHECK(s.ch[0][0] == ' ' && s.ch[SCREEN_H - 1][SCREEN_W - 1] == ' ');
    CHECK(s.attr[12][40] == 0x17);
}

static void test_text_writes_and_clips(void) {
    Screen s;
    screen_clear(&s, 7);
    screen_text(&s, 78, 0, "abcd", 0x1E); /* вылезает за правый край */
    CHECK(s.ch[0][78] == 'a' && s.ch[0][79] == 'b');
    CHECK(s.ch[1][0] == ' ');             /* не перенеслось на новую строку */
    CHECK(s.attr[0][78] == 0x1E);
    screen_text(&s, -2, 3, "xyz", 7);     /* левый край */
    CHECK(s.ch[3][0] == 'z');
    screen_text(&s, 0, -1, "no", 7);      /* строки вне экрана игнорируются */
    screen_text(&s, 0, SCREEN_H, "no", 7);
    CHECK(s.ch[0][0] == ' ');
}

static void test_fill_clips(void) {
    Screen s;
    screen_clear(&s, 7);
    screen_fill(&s, -5, -5, 200, 200, '#', 0x22); /* больше экрана — не падает */
    CHECK(s.ch[0][0] == '#' && s.ch[SCREEN_H - 1][SCREEN_W - 1] == '#');
}

static void test_hash_changes(void) {
    Screen a, b;
    screen_clear(&a, 7);
    screen_clear(&b, 7);
    CHECK(screen_hash(&a) == screen_hash(&b));
    b.attr[5][5] = 8;
    CHECK(screen_hash(&a) != screen_hash(&b));
}

static void test_script_draws(void) {
    char err[256];
    Engine *e = make_script(MINI, err, sizeof err);
    CHECK(e != NULL);
    if (!e) return;
    CHECK(engine_tick(e));
    CHECK(engine_screen(e)->ch[0][0] == 'h');
    CHECK(engine_screen(e)->attr[0][1] == 0x1E); /* фон 1, цвет 14 */
    engine_free(e);
}

static void test_keys_reach_script(void) {
    char err[256];
    Engine *e = make_script(MINI, err, sizeof err);
    if (!e) { CHECK(0); return; }
    engine_push_key(e, KEY_LEFT);
    engine_push_key(e, KEY_ENTER);
    engine_tick(e);
    CHECK(val(e, "last == 'left'"));
    engine_tick(e);
    CHECK(val(e, "last == 'enter'"));
    engine_tick(e);
    CHECK(val(e, "last == nil")); /* очередь пуста */
    engine_free(e);
}

static void test_key_queue_overflow(void) {
    char err[256];
    int i;
    Engine *e = make_script(MINI, err, sizeof err);
    if (!e) { CHECK(0); return; }
    for (i = 0; i < 100; i++) engine_push_key(e, KEY_UP); /* не должно переполниться */
    for (i = 0; i < KEY_QUEUE_SIZE; i++) engine_tick(e);
    CHECK(val(e, "last == 'up'"));
    engine_tick(e);
    CHECK(val(e, "last == nil")); /* лишние клавиши отброшены */
    engine_free(e);
}

static void test_syntax_error_reported(void) {
    char err[256] = "";
    Engine *e = make_script("function init( end", err, sizeof err);
    CHECK(e == NULL);
    CHECK(strstr(err, "test.lua") != NULL); /* в сообщении есть имя файла */
}

static void test_missing_function_reported(void) {
    char err[256] = "";
    Engine *e = make_script("x = 1", err, sizeof err);
    CHECK(e == NULL);
    CHECK(strstr(err, "init") != NULL);
}

static void test_runtime_error_reported(void) {
    char err[256];
    Engine *e = make_script("function init() end\n"
                            "function update() error('boom') end\n"
                            "function draw() end\n", err, sizeof err);
    if (!e) { CHECK(0); return; }
    CHECK(engine_tick(e) == 0);
    CHECK(strstr(engine_error(e), "boom") != NULL);
    engine_free(e);
}

static void test_bad_api_args(void) {
    char err[256];
    Engine *e = make_script("function init() end\n"
                            "function update() end\n"
                            "function draw() text('a', 0, 'x') end\n", err, sizeof err);
    if (!e) { CHECK(0); return; }
    CHECK(engine_tick(e) == 0); /* ошибка в аргументах — исключение Lua, а не падение */
    engine_free(e);
}

static void test_sandbox(void) {
    char err[256];
    Engine *e = make_script(MINI, err, sizeof err);
    if (!e) { CHECK(0); return; }
    CHECK(val(e, "io == nil and os == nil")); /* скрипт не имеет доступа к ОС */
    CHECK(val(e, "math ~= nil and string ~= nil"));
    engine_free(e);
}

void suite_core(void) {
    RUN(test_clear_fills_spaces);
    RUN(test_text_writes_and_clips);
    RUN(test_fill_clips);
    RUN(test_hash_changes);
    RUN(test_script_draws);
    RUN(test_keys_reach_script);
    RUN(test_key_queue_overflow);
    RUN(test_syntax_error_reported);
    RUN(test_missing_function_reported);
    RUN(test_runtime_error_reported);
    RUN(test_bad_api_args);
    RUN(test_sandbox);
}
