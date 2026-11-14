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

.PHONY: all linux test clean

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

clean:
	rm -rf build
