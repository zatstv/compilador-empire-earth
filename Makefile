CC        := gcc
FLEX      := flex
BISON     := bison

BUILD_DIR := build
CFLAGS    := -Wall -Wextra -std=gnu11 -O2 -I. -I$(BUILD_DIR)
LDFLAGS   :=

TARGET    := compiler
HEADERS   := ast.h symbol_table.h semantic.h runtime.h string_utils.h
LEX_SRC   := lexer.l
YACC_SRC  := parser.y

# Generated source files
BISON_C   := $(BUILD_DIR)/parser.tab.c
BISON_H   := $(BUILD_DIR)/parser.tab.h
LEX_C     := $(BUILD_DIR)/lex.yy.c

# Object files
OBJS      := $(BUILD_DIR)/main.o \
             $(BUILD_DIR)/ast.o \
             $(BUILD_DIR)/symbol_table.o \
             $(BUILD_DIR)/semantic.o \
             $(BUILD_DIR)/runtime.o \
             $(BUILD_DIR)/string_utils.o \
             $(BUILD_DIR)/parser.tab.o \
             $(BUILD_DIR)/lex.yy.o

.PHONY: all clean run

all: $(TARGET)

# Final Linker Step
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Compile main.c
$(BUILD_DIR)/main.o: main.c $(HEADERS) $(BISON_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $@

# Compile ast.c
$(BUILD_DIR)/ast.o: ast.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ast.c -o $@

$(BUILD_DIR)/symbol_table.o: symbol_table.c symbol_table.h ast.h string_utils.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c symbol_table.c -o $@

$(BUILD_DIR)/semantic.o: semantic.c semantic.h symbol_table.h ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c semantic.c -o $@

$(BUILD_DIR)/runtime.o: runtime.c runtime.h symbol_table.h ast.h string_utils.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c runtime.c -o $@

$(BUILD_DIR)/string_utils.o: string_utils.c string_utils.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c string_utils.c -o $@

# Bison rule
$(BISON_C) $(BISON_H): $(YACC_SRC) | $(BUILD_DIR)
	$(BISON) -d -o $(BISON_C) $(YACC_SRC)

# Compile parser.tab.c
$(BUILD_DIR)/parser.tab.o: $(BISON_C) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(BISON_C) -o $@

# Flex rule (depends on BISON_H so tokens exist first)
$(LEX_C): $(LEX_SRC) $(BISON_H) | $(BUILD_DIR)
	$(FLEX) -o $(LEX_C) $(LEX_SRC)

# Compile lex.yy.c
$(BUILD_DIR)/lex.yy.o: $(LEX_C) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-sign-compare -c $(LEX_C) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET) source.ee

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
