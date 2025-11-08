# ────────────────────────────────
# Makefile for CLI project
# ────────────────────────────────

# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -g -Wno-unused-function \
            -I build -I src -I src/commands -I src/scaffold -I src/template -I src/utils
LDFLAGS := -lfl -lreadline

# Build paths
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/cli

UTIL_SRCS := $(wildcard src/utils/*.c)

# Source files
SRCS := src/main.c \
        src/commands/commands.c \
        src/scaffold/scaffold.c \
        src/template/template.c \
        $(UTIL_SRCS) \
        $(BUILD_DIR)/lex.yy.c \
        $(BUILD_DIR)/cli.tab.c

# Parser / Lexer
LEX_SRC := src/parser/cli.l
YACC_SRC := src/parser/cli.y

# Generated files
LEX_OUT := $(BUILD_DIR)/lex.yy.c
YACC_OUT := $(BUILD_DIR)/cli.tab.c
YACC_HDR := $(BUILD_DIR)/cli.tab.h

# ────────────────────────────────
# Rules
# ────────────────────────────────

all: $(TARGET)

# Ensure build directory exists
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Generate parser
$(YACC_OUT) $(YACC_HDR): $(YACC_SRC) | $(BUILD_DIR)
	@bison -d -o $(YACC_OUT) $(YACC_SRC)

# Generate lexer
$(LEX_OUT): $(LEX_SRC) $(YACC_HDR) | $(BUILD_DIR)
	@flex -o $(LEX_OUT) $(LEX_SRC)

# Compile everything
$(TARGET): $(SRCS) src/utils/defs.h | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

# Run CLI
run: $(TARGET)
	@./$(TARGET)

# Clean build
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)