/*
 * plat_web.c — платформа «браузер» (Emscripten -> WebAssembly).
 *
 * C-код компилируется в WebAssembly (wasm32). Небольшие вставки на
 * JavaScript (EM_JS) рисуют кадр на <canvas> и собирают нажатия:
 * клавиатура, экранные кнопки и свайпы на сенсорном экране.
 * Главный цикл крутит сам браузер (см. main.c, emscripten_set_main_loop).
 */
#include <emscripten.h>

#include "platform.h"

/* Подготовка canvas и обработчиков ввода. Клавиши складываются в очередь. */
EM_JS(void, js_init, (int w, int h), {
    var canvas = document.getElementById('screen');
    var ctx = canvas.getContext('2d');
    var CW = 12, CH = 24;
    canvas.width = w * CW;
    canvas.height = h * CH;
    ctx.font = '20px "Courier New", monospace';
    ctx.textBaseline = 'middle';
    ctx.textAlign = 'center';
    Module.snake = { ctx: ctx, cw: CW, ch: CH, keys: [] };

    // Коды KEY_* из engine.h: 1 вверх, 2 вниз, 3 влево, 4 вправо, 5 Enter
    var push = function(k) { Module.snake.keys.push(k); };
    window.snakeKey = push; // для экранных кнопок в index.html
    var map = { ArrowUp: 1, KeyW: 1, ArrowDown: 2, KeyS: 2, ArrowLeft: 3, KeyA: 3,
                ArrowRight: 4, KeyD: 4, Enter: 5, Space: 5 };
    window.addEventListener('keydown', function(e) {
        if (map[e.code]) { push(map[e.code]); e.preventDefault(); }
    });

    // Свайп по экрану — поворот, касание без движения — Enter
    var sx = 0, sy = 0;
    canvas.addEventListener('touchstart', function(e) {
        sx = e.touches[0].clientX; sy = e.touches[0].clientY; e.preventDefault();
    }, { passive: false });
    canvas.addEventListener('touchend', function(e) {
        var dx = e.changedTouches[0].clientX - sx, dy = e.changedTouches[0].clientY - sy;
        if (Math.abs(dx) < 20 && Math.abs(dy) < 20) push(5);
        else if (Math.abs(dx) > Math.abs(dy)) push(dx > 0 ? 4 : 3);
        else push(dy > 0 ? 2 : 1);
        e.preventDefault();
    }, { passive: false });
});

EM_JS(int, js_key, (void), {
    var k = Module.snake.keys;
    return k.length ? k.shift() : 0;
});

/* Нарисовать кадр: chars и attrs — массивы w*h байт в памяти WebAssembly. */
EM_JS(void, js_present, (const unsigned char *chars, const unsigned char *attrs, int w, int h), {
    var pal = ['#000000', '#0000aa', '#00aa00', '#00aaaa', '#aa0000', '#aa00aa', '#aa5500', '#aaaaaa',
               '#555555', '#5555ff', '#55ff55', '#55ffff', '#ff5555', '#ff55ff', '#ffff55', '#ffffff'];
    var s = Module.snake, ctx = s.ctx;
    for (var y = 0; y < h; y++) {
        for (var x = 0; x < w; x++) {
            var i = y * w + x, a = HEAPU8[attrs + i], c = HEAPU8[chars + i];
            ctx.fillStyle = pal[a >> 4];
            ctx.fillRect(x * s.cw, y * s.ch, s.cw, s.ch);
            if (c !== 32) {
                ctx.fillStyle = pal[a & 15];
                ctx.fillText(String.fromCharCode(c), x * s.cw + s.cw / 2, y * s.ch + s.ch / 2 + 1);
            }
        }
    }
});

int plat_init(void) {
    js_init(SCREEN_W, SCREEN_H);
    return 0;
}

void plat_quit(void) {}

int plat_key(void) { return js_key(); }

unsigned long plat_ticks(void) { return (unsigned long)emscripten_get_now(); }

void plat_sleep(int ms) { (void)ms; /* браузер сам вызывает кадры */ }

void plat_present(const Screen *s) {
    js_present(&s->ch[0][0], &s->attr[0][0], SCREEN_W, SCREEN_H);
}
