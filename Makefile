# Windows note: mingw32-make runs recipes through MSYS2's sh, so `rm` is available
# there too. Keep C:\msys64\usr\bin on PATH (sh.exe lives there).

SOURCE_DIR = src
SOURCES = $(wildcard $(SOURCE_DIR)/*.c)
HEADERS = $(wildcard $(SOURCE_DIR)/*.h)
USE_WERROR = 0

ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
    # MSYS2 UCRT64 gcc ships no libasan (link fails with: cannot find -lasan),
    # so there are no sanitizers on Windows. Run `make debug` under WSL for those.
    SAN_FLAGS =
else
    EXE_EXT =
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        SAN_FLAGS = -fsanitize=address -fsanitize=undefined
    else
        SAN_FLAGS = -fsanitize=address -fsanitize=undefined -fsanitize=leak
    endif
endif

RM = rm -f

CC = gcc
CFLAGS = -Wall -Wextra -std=c11
CFLAGS_STRICT = -Wshadow -Wformat=2 -Wundef
ifeq ($(USE_WERROR),1)
	CFLAGS_STRICT += -Werror
endif
CFLAGS_RELEASE = -O2
CFLAGS_DEBUG = $(CFLAGS_STRICT) -O0 -ggdb $(SAN_FLAGS)

EXECUTABLE_NAME = chasm$(EXE_EXT)

.PHONY: check debug release compile_headers compile-lexer_h clean clean-gch clean-exe

# Default goal: syntax-check everything. Switch to `debug` once main.c exists.
check: compile_headers
	@echo ""
	@echo "--------------------------------"
	@echo "Checking syntax for: $(SOURCES)"
	@echo "--------------------------------"
	$(CC) $(CFLAGS) $(CFLAGS_STRICT) -fsyntax-only $(SOURCES)
	@echo "Syntax check completed"
	@echo ""

debug: $(SOURCES) $(HEADERS)
	@echo ""
	@echo "--------------------------------"
	@echo "Compiling executable (debug)"
	@echo "--------------------------------"
	@echo ""
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $(SOURCES) -o $(EXECUTABLE_NAME)

release: $(SOURCES) $(HEADERS)
	@echo ""
	@echo "--------------------------------"
	@echo "Compiling executable (release)"
	@echo "--------------------------------"
	@echo ""
	$(CC) $(CFLAGS) $(CFLAGS_RELEASE) $(SOURCES) -o $(EXECUTABLE_NAME)

compile_headers: compile-lexer_h

compile-lexer_h: $(SOURCE_DIR)/lexer.h
	@echo ""
	@echo "--------------------------------"
	@echo "Checking syntax for: lexer.h"
	@echo "--------------------------------"
	$(CC) $(CFLAGS) $(CFLAGS_STRICT) -fsyntax-only $<
	@echo ""

clean: clean-gch clean-exe

clean-gch:
	@echo ""
	@echo "Cleaning .gch files..."
	$(RM) $(SOURCE_DIR)/*.gch
	@echo "All .gch files deleted"
	@echo ""

clean-exe:
	@echo ""
	@echo "Cleaning executables..."
	$(RM) chasm chasm.exe
	@echo "All executables deleted"
	@echo ""
