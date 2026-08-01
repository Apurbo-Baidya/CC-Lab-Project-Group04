# ============================================================
# Makefile for the mini-language compiler front end.
#
# Build order (must match the Bison-first rule from every lab
# manual: Bison generates the token header that the Flex-
# generated scanner needs to #include):
#   1. bison  -> parser.tab.c / parser.tab.h
#   2. flex   -> lex.yy.c
#   3. gcc    -> compile everything into ./compiler
# ============================================================

CC      = gcc
CFLAGS  = -Wall -g -Isrc
BISON   = bison
FLEX    = flex

SRC_DIR = src
BUILD   = build

OBJS = $(BUILD)/parser.tab.o \
       $(BUILD)/lex.yy.o \
       $(BUILD)/ast.o \
       $(BUILD)/symbol_table.o \
       $(BUILD)/semantic.o \
       $(BUILD)/codegen.o \
       $(BUILD)/main.o

.PHONY: all clean test

all: compiler

$(BUILD):
	mkdir -p $(BUILD)

# Bison must run before Flex: it produces parser.tab.h, which
# lexer.l includes for the token definitions.
$(SRC_DIR)/parser/parser.tab.c $(SRC_DIR)/parser/parser.tab.h: $(SRC_DIR)/parser/parser.y | $(BUILD)
	$(BISON) -d -o $(SRC_DIR)/parser/parser.tab.c $(SRC_DIR)/parser/parser.y

$(SRC_DIR)/lexer/lex.yy.c: $(SRC_DIR)/lexer/lexer.l $(SRC_DIR)/parser/parser.tab.h
	$(FLEX) -o $(SRC_DIR)/lexer/lex.yy.c $(SRC_DIR)/lexer/lexer.l

$(BUILD)/parser.tab.o: $(SRC_DIR)/parser/parser.tab.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/lex.yy.o: $(SRC_DIR)/lexer/lex.yy.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/ast.o: $(SRC_DIR)/ast/ast.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/symbol_table.o: $(SRC_DIR)/symbol_table/symbol_table.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/semantic.o: $(SRC_DIR)/semantic/semantic.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/codegen.o: $(SRC_DIR)/codegen/codegen.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main.o: $(SRC_DIR)/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

compiler: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lfl -o compiler

test: compiler
	@for f in examples/valid_*.txt; do \
		echo "----- $$f -----"; ./compiler $$f; echo; \
	done

clean:
	rm -rf $(BUILD) compiler \
		$(SRC_DIR)/parser/parser.tab.c $(SRC_DIR)/parser/parser.tab.h \
		$(SRC_DIR)/lexer/lex.yy.c
