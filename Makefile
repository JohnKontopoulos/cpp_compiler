CC = gcc
CFLAGS = -Wall -g
SRC_DIR = src
BUILD_DIR = build

all: $(BUILD_DIR)/compiler

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/parser.tab.c $(BUILD_DIR)/parser.tab.h: $(SRC_DIR)/parser.y | $(BUILD_DIR)
	bison -d -v -o $(BUILD_DIR)/parser.tab.c $(SRC_DIR)/parser.y

$(BUILD_DIR)/lex.yy.c: $(SRC_DIR)/lexer.l $(BUILD_DIR)/parser.tab.h | $(BUILD_DIR)
	flex -o $(BUILD_DIR)/lex.yy.c $(SRC_DIR)/lexer.l
	cp $(BUILD_DIR)/lex.yy.c $(BUILD_DIR)/lexer_generated.c

$(BUILD_DIR)/parser.tab.o: $(BUILD_DIR)/parser.tab.c
	$(CC) $(CFLAGS) -I$(BUILD_DIR) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/lex.yy.o: $(BUILD_DIR)/lex.yy.c
	$(CC) $(CFLAGS) -I$(BUILD_DIR) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/symtable.o: $(SRC_DIR)/symtable.c $(SRC_DIR)/symtable.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast.c $(SRC_DIR)/ast.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/semantic.o: $(SRC_DIR)/semantic.c $(SRC_DIR)/semantic.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/dataspace.o: $(SRC_DIR)/dataspace.c $(SRC_DIR)/dataspace.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/codegen.o: $(SRC_DIR)/codegen.c $(SRC_DIR)/codegen.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/main_test.o: $(SRC_DIR)/main_test.c
	$(CC) $(CFLAGS) -I$(BUILD_DIR) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/optimizer.o: $(SRC_DIR)/optimizer.c $(SRC_DIR)/optimizer.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/compiler: $(BUILD_DIR)/lex.yy.o $(BUILD_DIR)/parser.tab.o \
                       $(BUILD_DIR)/symtable.o $(BUILD_DIR)/ast.o \
                       $(BUILD_DIR)/semantic.o $(BUILD_DIR)/dataspace.o \
                       $(BUILD_DIR)/optimizer.o $(BUILD_DIR)/codegen.o \
                       $(BUILD_DIR)/main_test.o
	$(CC) $(CFLAGS) -o $@ $^ -lfl

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean