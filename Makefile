CC = gcc
CFLAGS = -Wall -Wextra -std=c11
CFLAGS_RELEASE = -O2
CFLAGS_DEBUG = -Wshadow -Wformat=2 -Wundef -Werror

SOURCE_DIR = src

.PHONY: all clean compile_headers

all: compile_headers

compile_headers: compile-lexer_h

compile-lexer_h: $(SOURCE_DIR)/lexer.h
	@echo ""
	@echo "--------------------------------"
	@echo "Checking syntax for: lexer.h"
	@echo "--------------------------------"
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $<
	@echo "Compilation completed"
	@echo ""

clean: clean-gch

clean-gch:
	@echo "Cleaning .gch files..."
	rm -f $(SOURCE_DIR)/*.gch
