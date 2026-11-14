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

.PHONY: all linux test test-mips clean

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

clean:
	rm -rf build
