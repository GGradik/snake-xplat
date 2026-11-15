/*
 * plat_win.c — платформа «консоль Windows» (WinAPI).
 *
 * Вывод: WriteConsoleOutput записывает весь кадр 80x25 разом.
 * Цветовые атрибуты консоли Windows устроены так же, как в CGA/DOS
 * (биты: синий=1, зелёный=2, красный=4, яркость=8, фон — старшие 4 бита),
 * поэтому атрибуты из экранного буфера передаются без преобразования.
 * Ввод: ReadConsoleInput, клавиши по виртуальным кодам (VK_UP и т.д.).
 */
#include <windows.h>

#include "platform.h"

static HANDLE out, in;
static DWORD saved_mode;
static CONSOLE_CURSOR_INFO saved_cursor;

int plat_init(void) {
    CONSOLE_CURSOR_INFO cursor;
    COORD size = {SCREEN_W, SCREEN_H};
    out = GetStdHandle(STD_OUTPUT_HANDLE);
    in = GetStdHandle(STD_INPUT_HANDLE);
    if (out == INVALID_HANDLE_VALUE || in == INVALID_HANDLE_VALUE) return -1;
    GetConsoleMode(in, &saved_mode);
    SetConsoleMode(in, ENABLE_EXTENDED_FLAGS); /* без эха и строкового ввода */
    GetConsoleCursorInfo(out, &saved_cursor);
    cursor = saved_cursor;
    cursor.bVisible = FALSE;
    SetConsoleCursorInfo(out, &cursor);
    SetConsoleScreenBufferSize(out, size); /* может не получиться — не страшно */
    SetConsoleTitleA("Snake");
    return 0;
}

void plat_quit(void) {
    COORD home = {0, 0};
    DWORD n;
    FillConsoleOutputAttribute(out, 7, SCREEN_W * SCREEN_H, home, &n);
    FillConsoleOutputCharacterA(out, ' ', SCREEN_W * SCREEN_H, home, &n);
    SetConsoleCursorPosition(out, home);
    SetConsoleCursorInfo(out, &saved_cursor);
    SetConsoleMode(in, saved_mode);
}

int plat_key(void) {
    INPUT_RECORD rec;
    DWORD count = 0;
    while (GetNumberOfConsoleInputEvents(in, &count) && count > 0) {
        ReadConsoleInputA(in, &rec, 1, &count);
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) continue;
        switch (rec.Event.KeyEvent.wVirtualKeyCode) {
        case VK_UP: case 'W': return KEY_UP;
        case VK_DOWN: case 'S': return KEY_DOWN;
        case VK_LEFT: case 'A': return KEY_LEFT;
        case VK_RIGHT: case 'D': return KEY_RIGHT;
        case VK_RETURN: case VK_SPACE: return KEY_ENTER;
        case VK_ESCAPE: case 'Q': return KEY_QUIT;
        }
    }
    return KEY_NONE;
}

unsigned long plat_ticks(void) { return (unsigned long)GetTickCount(); }

void plat_sleep(int ms) { Sleep((DWORD)ms); }

void plat_present(const Screen *s) {
    static CHAR_INFO buf[SCREEN_H * SCREEN_W];
    COORD size = {SCREEN_W, SCREEN_H}, origin = {0, 0};
    SMALL_RECT rect = {0, 0, SCREEN_W - 1, SCREEN_H - 1};
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++) {
            buf[y * SCREEN_W + x].Char.AsciiChar = (CHAR)s->ch[y][x];
            buf[y * SCREEN_W + x].Attributes = s->attr[y][x];
        }
    WriteConsoleOutputA(out, buf, size, origin, &rect);
}
