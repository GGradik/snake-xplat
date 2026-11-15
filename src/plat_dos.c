/*
 * plat_dos.c — платформа «MS-DOS» (DJGPP, 32-битный защищённый режим, i386).
 *
 * Вывод: текстовый режим 80x25. Видеопамять устроена ровно как наш
 * экранный буфер: на каждую клетку 2 байта — символ и атрибут цвета.
 * ScreenUpdate() копирует готовый кадр в видеопамять (адрес B800:0000).
 * Ввод: kbhit()/getkey() через BIOS; стрелки приходят как расширенные коды.
 * Время: uclock() — таймер с точностью около микросекунды.
 */
#include <conio.h>
#include <dos.h>
#include <keys.h>
#include <pc.h>
#include <time.h>

#include "platform.h"

int plat_init(void) {
    textmode(C80);              /* цветной текстовый режим 80x25 */
    intensevideo();             /* яркие цвета фона вместо мигания */
    _setcursortype(_NOCURSOR);
    return 0;
}

void plat_quit(void) {
    blinkvideo();
    textattr(LIGHTGRAY);
    clrscr();
    _setcursortype(_NORMALCURSOR);
}

int plat_key(void) {
    if (!kbhit()) return KEY_NONE;
    switch (getkey()) {
    case K_Up: case 'w': case 'W': return KEY_UP;
    case K_Down: case 's': case 'S': return KEY_DOWN;
    case K_Left: case 'a': case 'A': return KEY_LEFT;
    case K_Right: case 'd': case 'D': return KEY_RIGHT;
    case K_Return: case ' ': return KEY_ENTER;
    case K_Escape: case 'q': case 'Q': return KEY_QUIT;
    }
    return KEY_NONE;
}

unsigned long plat_ticks(void) {
    return (unsigned long)(uclock() / (UCLOCKS_PER_SEC / 1000));
}

void plat_sleep(int ms) { delay((unsigned)ms); }

void plat_present(const Screen *s) {
    static unsigned short cells[SCREEN_H * SCREEN_W];
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++)
            cells[y * SCREEN_W + x] = (unsigned short)(s->ch[y][x] | (s->attr[y][x] << 8));
    ScreenUpdate(cells);
}
