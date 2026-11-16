# Makefile — сборка «Змейки» под все платформы.
#
#   make            — Linux/Unix (терминал)
#   make test       — все автотесты на текущей машине
#   make clean      — удалить build/

CFLAGS   ?= -O2
WARN      = -std=c99 -Wall -Wextra
INCLUDES  = -Isrc -Ithird_party/lua -Ibuild

# Исходники Lua (только ядро и нужные библиотеки)
LUA_SRC = $(addprefix third_party/lua/, \
  lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldebug.c ldo.c \
  ldump.c lfunc.c lgc.c llex.c lmathlib.c lmem.c lobject.c lopcodes.c \
  lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c \
  lundump.c lutf8lib.c lvm.c lzio.c)

CORE_SRC = src/engine.c $(LUA_SRC)
GAME_SRC = src/main.c $(CORE_SRC)
DEPS     = $(wildcard src/*.h) build/game_lua.h

.PHONY: all linux windows dos web test test-mips test-windows clean

all: linux

# --- Встраивание Lua-скрипта в программу ---
# bin2c собирается обычным компилятором машины сборки (HOSTCC).
HOSTCC ?= cc
build/bin2c: tools/bin2c.c
	@mkdir -p build
	$(HOSTCC) -O2 -o $@ $<

build/game_lua.h: game/snake.lua build/bin2c
	build/bin2c game_lua $< $@

# --- Linux / Unix: терминал ---
linux: build/linux/snake

build/linux/snake: $(GAME_SRC) src/plat_tty.c $(DEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARN) $(INCLUDES) -DLUA_USE_POSIX -o $@ \
	  $(GAME_SRC) src/plat_tty.c -lm

# --- Windows (x86-64): консоль WinAPI, кросс-компилятор MinGW-w64 ---
WIN_CC = x86_64-w64-mingw32-gcc
windows: build/windows/snake.exe

build/windows/snake.exe: $(GAME_SRC) src/plat_win.c $(DEPS)
	@mkdir -p $(dir $@)
	$(WIN_CC) $(CFLAGS) $(WARN) $(INCLUDES) -static -o $@ \
	  $(GAME_SRC) src/plat_win.c

# --- MS-DOS (i386, 32 бита): DJGPP, текстовый режим ---
# Кросс-компилятор: https://github.com/andrewwutw/build-djgpp
DJGPP_CC ?= /opt/djgpp/bin/i586-pc-msdosdjgpp-gcc
dos: build/dos/SNAKE.EXE

DJGPP_EXE2COFF ?= /opt/djgpp/i586-pc-msdosdjgpp/bin/exe2coff

# 1) компилируем; 2) exe2coff отрезает стандартный загрузчик;
# 3) приклеиваем загрузчик со встроенным DPMI-сервером CWSDPMI.
build/dos/SNAKE.EXE: $(GAME_SRC) src/plat_dos.c $(DEPS)
	@mkdir -p build/dos/tmp
	$(DJGPP_CC) $(CFLAGS) $(WARN) -std=gnu99 $(INCLUDES) -o build/dos/tmp/game.exe \
	  $(GAME_SRC) src/plat_dos.c -lm
	$(DJGPP_EXE2COFF) build/dos/tmp/game.exe
	cat third_party/cwsdpmi/CWSDSTUB.EXE build/dos/tmp/game > $@

# --- Web (wasm32): Emscripten, canvas + клавиатура + touch ---
EMCC ?= emcc
web: build/web/index.html

build/web/index.html: $(GAME_SRC) src/plat_web.c web/shell.html $(DEPS)
	@mkdir -p $(dir $@)
	$(EMCC) -O2 $(WARN) $(INCLUDES) --shell-file web/shell.html \
	  -s ALLOW_MEMORY_GROWTH=1 -s MINIFY_HTML=0 -o $@ $(GAME_SRC) src/plat_web.c

# --- Автотесты (без платформенного слоя: только ядро + логика) ---
TEST_SRC  = $(wildcard tests/*.c) $(CORE_SRC)
TEST_DEPS = $(DEPS) tests/test.h build/invariants_lua.h build/bot_lua.h

build/invariants_lua.h: tests/invariants.lua build/bin2c
	build/bin2c invariants_lua $< $@

build/bot_lua.h: tests/bot.lua build/bin2c
	build/bin2c bot_lua $< $@

build/test/run_tests: $(TEST_SRC) $(TEST_DEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARN) $(INCLUDES) -Itests -o $@ $(TEST_SRC) -lm

test: build/test/run_tests
	build/test/run_tests

# Те же тесты на 32-битном big-endian процессоре MIPS через эмулятор qemu.
MIPS_CC = mips-linux-gnu-gcc
build/test-mips/run_tests: $(TEST_SRC) $(TEST_DEPS)
	@mkdir -p $(dir $@)
	$(MIPS_CC) $(CFLAGS) $(WARN) $(INCLUDES) -Itests -static -o $@ $(TEST_SRC) -lm

test-mips: build/test-mips/run_tests
	qemu-mips $< 50

# Те же тесты, собранные под Windows, запускаются через Wine.
build/test-windows/run_tests.exe: $(TEST_SRC) $(TEST_DEPS)
	@mkdir -p $(dir $@)
	$(WIN_CC) $(CFLAGS) $(WARN) $(INCLUDES) -Itests -static -o $@ $(TEST_SRC)

test-windows: build/test-windows/run_tests.exe
	WINEDEBUG=-all wine $< 50

clean:
	rm -rf build
