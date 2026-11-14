/*
 * bin2c — превращает файл в массив C, чтобы встроить Lua-скрипт в программу.
 * Использование: bin2c ИМЯ_МАССИВА входной_файл выходной.h
 * Запускается на машине сборки (не на целевой платформе).
 */
#include <stdio.h>

int main(int argc, char **argv) {
    FILE *in, *out;
    int c, n = 0;
    if (argc != 4) {
        fprintf(stderr, "usage: bin2c NAME input output\n");
        return 1;
    }
    in = fopen(argv[2], "rb");
    out = fopen(argv[3], "w");
    if (!in || !out) {
        perror("bin2c");
        return 1;
    }
    fprintf(out, "/* Сгенерировано bin2c из %s — не редактировать. */\n", argv[2]);
    fprintf(out, "static const char %s[] = {\n", argv[1]);
    while ((c = fgetc(in)) != EOF) {
        fprintf(out, "%d,", c);
        if (++n % 20 == 0) fputc('\n', out);
    }
    fprintf(out, "\n};\n");
    fclose(in);
    fclose(out);
    return 0;
}
