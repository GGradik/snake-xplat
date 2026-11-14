/*
 * test_logic.c — модульные тесты игровой логики snake.lua.
 * Ситуации готовятся прямо из теста Lua-кодом, затем вызывается
 * step() (один шаг змейки) или update() через такты ядра.
 */
#include "test.h"

/* Новая игра в состоянии play, змейка (15,10)(14,10)(13,10) смотрит вправо. */
static Engine *playing(void) {
    Engine *e = make_game(1);
    ticks(e, KEY_ENTER, 1);
    lua(e, "G.food = {x = 0, y = 0}"); /* еду убираем в угол, чтобы не мешала */
    return e;
}

static void test_start_screen(void) {
    Engine *e = make_game(1);
    CHECK(val(e, "G.state == 'title'"));
    CHECK(val(e, "#G.snake") == 3);
    CHECK(val(e, "G.score") == 0);
    ticks(e, KEY_NONE, 50); /* без Enter игра не начинается */
    CHECK(val(e, "G.state == 'title'"));
    ticks(e, KEY_ENTER, 1);
    CHECK(val(e, "G.state == 'play'"));
    engine_free(e);
}

static void test_moves_one_cell_per_step(void) {
    Engine *e = playing();
    lua(e, "step()");
    CHECK(val(e, "G.snake[1].x") == 16 && val(e, "G.snake[1].y") == 10);
    CHECK(val(e, "#G.snake") == 3); /* длина не изменилась */
    CHECK(val(e, "G.snake[3].x") == 14); /* хвост подтянулся */
    engine_free(e);
}

static void test_speed_by_delay(void) {
    Engine *e = playing();
    ticks(e, KEY_NONE, 3); /* задержка 4 такта: после 3 ещё стоим */
    CHECK(val(e, "G.snake[1].x") == 15);
    ticks(e, KEY_NONE, 1);
    CHECK(val(e, "G.snake[1].x") == 16);
    engine_free(e);
}

static void test_turn(void) {
    Engine *e = playing();
    ticks(e, KEY_UP, 4);
    CHECK(val(e, "G.dir == 'up'"));
    CHECK(val(e, "G.snake[1].x") == 15 && val(e, "G.snake[1].y") == 9);
    engine_free(e);
}

static void test_no_reverse(void) {
    Engine *e = playing();
    ticks(e, KEY_LEFT, 4); /* разворот на 180° запрещён */
    CHECK(val(e, "G.dir == 'right'"));
    CHECK(val(e, "G.state == 'play'"));
    engine_free(e);
}

static void test_eat_grows(void) {
    Engine *e = playing();
    lua(e, "G.food = {x = 16, y = 10}"); /* еда прямо перед головой */
    lua(e, "step()");
    CHECK(val(e, "G.score") == 1);
    CHECK(val(e, "#G.snake") == 4);
    CHECK(val(e, "G.food.x ~= 16 or G.food.y ~= 10")); /* появилась новая еда */
    engine_free(e);
}

static void test_speed_up_every_5(void) {
    Engine *e = playing();
    int i;
    for (i = 0; i < 5; i++)
        lua(e, "local h = G.snake[1]; G.food = {x = h.x + 1, y = h.y}; step()");
    CHECK(val(e, "G.score") == 5);
    CHECK(val(e, "G.delay") == 3);
    engine_free(e);
}

static void test_wall_kills(void) {
    Engine *e = playing();
    lua(e, "for i = 1, 14 do step() end"); /* голова у правой стены x=29 */
    CHECK(val(e, "G.state == 'play'"));
    lua(e, "step()");
    CHECK(val(e, "G.state == 'over'"));
    engine_free(e);
}

static void test_self_kills(void) {
    Engine *e = playing();
    /* змейка закручена крючком: поворот влево ведёт голову в своё тело */
    lua(e, "G.snake = {{x=5,y=5},{x=5,y=6},{x=4,y=6},{x=4,y=5},{x=4,y=4}}"
           "G.score = 2; G.dir = 'up'; G.next_dir = 'left'");
    lua(e, "step()"); /* (4,5) — четвёртый сегмент тела */
    CHECK(val(e, "G.state == 'over'"));
    engine_free(e);
}

static void test_can_follow_tail(void) {
    Engine *e = playing();
    /* квадрат 2x2: голова идёт в клетку, которую в этот же шаг покидает хвост */
    lua(e, "G.snake = {{x=5,y=5},{x=5,y=6},{x=6,y=6},{x=6,y=5}}; G.score = 1;"
           "G.dir = 'up'; G.next_dir = 'right'; step()");
    CHECK(val(e, "G.state == 'play'"));
    engine_free(e);
}

static void test_best_and_restart(void) {
    Engine *e = playing();
    lua(e, "G.score = 7; G.snake[4] = {x=12,y=10}; game_over()");
    CHECK(val(e, "G.best") == 7);
    ticks(e, KEY_ENTER, 1);
    CHECK(val(e, "G.state == 'play'"));
    CHECK(val(e, "G.score") == 0 && val(e, "#G.snake") == 3);
    CHECK(val(e, "G.best") == 7); /* рекорд сохраняется между играми */
    engine_free(e);
}

static void test_win_when_field_full(void) {
    Engine *e = playing();
    /* заполняем всё поле змейкой (зигзагом), кроме одной клетки — еды */
    lua(e, "G.snake = {}\n"
           "for y = H - 1, 0, -1 do\n"
           "  for i = 0, W - 1 do\n"
           "    local x = (y % 2 == 0) and (W - 1 - i) or i\n"
           "    if not (x == 0 and y == 0) then table.insert(G.snake, 1, {x=x,y=y}) end\n"
           "  end\n"
           "end\n"
           "G.score = #G.snake - 3\n"
           "G.food = {x = 0, y = 0}; G.dir = 'left'; G.next_dir = 'left'\n"
           "step()");
    CHECK(val(e, "G.state == 'win'"));
    CHECK(val(e, "G.food == nil"));
    engine_free(e);
}

static void test_food_never_on_snake(void) {
    Engine *e = playing();
    int i;
    for (i = 0; i < 300; i++) {
        lua(e, "place_food()");
        CHECK(val(e, "(function() for _, s in ipairs(G.snake) do "
                     "if s.x == G.food.x and s.y == G.food.y then return false end end "
                     "return true end)()"));
    }
    engine_free(e);
}

static void test_draw_shows_score(void) {
    Engine *e = playing();
    const Screen *s;
    lua(e, "G.score = 42");
    ticks(e, KEY_NONE, 1);
    s = engine_screen(e);
    CHECK(s->ch[4][66] == 'S' && s->ch[4][73] == '4' && s->ch[4][74] == '2');
    engine_free(e);
}

void suite_logic(void) {
    RUN(test_start_screen);
    RUN(test_moves_one_cell_per_step);
    RUN(test_speed_by_delay);
    RUN(test_turn);
    RUN(test_no_reverse);
    RUN(test_eat_grows);
    RUN(test_speed_up_every_5);
    RUN(test_wall_kills);
    RUN(test_self_kills);
    RUN(test_can_follow_tail);
    RUN(test_best_and_restart);
    RUN(test_win_when_field_full);
    RUN(test_food_never_on_snake);
    RUN(test_draw_shows_score);
}
