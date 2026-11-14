/*
 * plat_tty.c — платформа «терминал Unix» (Linux, macOS, *BSD).
 *
 * Ввод: терминал переводится в «сырой» режим (termios), клавиши
 * читаются без ожидания Enter. Стрелки приходят как ESC [ A..D.
 * Вывод: управляющие последовательности ANSI (цвет, позиция курсора).
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "platform.h"

static struct termios saved; /* настройки терминала до запуска игры */

/* Номера цветов CGA -> ANSI (в CGA синий=1, красный=4, в ANSI наоборот). */
static const int ansi[8] = {0, 4, 2, 6, 1, 5, 3, 7};

int plat_init(void) {
    struct termios raw;
    if (tcgetattr(STDIN_FILENO, &saved) != 0) return -1;
    raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO); /* без буферизации строк и эха */
    raw.c_cc[VMIN] = 0;              /* read() не ждёт нажатия */
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    /* альтернативный экран, скрыть курсор, очистить */
    fputs("\033[?1049h\033[?25l\033[2J", stdout);
    fflush(stdout);
    return 0;
}

void plat_quit(void) {
    fputs("\033[0m\033[?25h\033[?1049l", stdout);
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
}

int plat_key(void) {
    unsigned char buf[8];
    ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
    if (n <= 0) return KEY_NONE;
    if (buf[0] == 27) {          /* ESC: либо стрелка, либо выход */
        if (n >= 3 && buf[1] == '[') {
            switch (buf[2]) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            }
            return KEY_NONE;
        }
        return KEY_QUIT;
    }
    switch (buf[0]) {
    case 'w': case 'W': return KEY_UP;
    case 's': case 'S': return KEY_DOWN;
    case 'a': case 'A': return KEY_LEFT;
    case 'd': case 'D': return KEY_RIGHT;
    case '\n': case '\r': case ' ': return KEY_ENTER;
    case 'q': case 'Q': return KEY_QUIT;
    }
    return KEY_NONE;
}

unsigned long plat_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)ts.tv_sec * 1000UL + (unsigned long)(ts.tv_nsec / 1000000L);
}

void plat_sleep(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void plat_present(const Screen *s) {
    /* Собираем весь кадр в одну строку и выводим одним write — без мерцания. */
    static char out[SCREEN_W * SCREEN_H * 24];
    char *p = out;
    int x, y, last = -1;
    p += sprintf(p, "\033[H");
    for (y = 0; y < SCREEN_H; y++) {
        for (x = 0; x < SCREEN_W; x++) {
            int a = s->attr[y][x];
            if (a != last) {
                int fg = a & 15, bg = (a >> 4) & 15;
                p += sprintf(p, "\033[0;%d;%dm",
                             (fg & 8 ? 90 : 30) + ansi[fg & 7],
                             (bg & 8 ? 100 : 40) + ansi[bg & 7]);
                last = a;
            }
            *p++ = (char)s->ch[y][x];
        }
        if (y < SCREEN_H - 1) p += sprintf(p, "\033[0m\r\n"), last = -1;
    }
    fwrite(out, 1, (size_t)(p - out), stdout);
    fflush(stdout);
}
