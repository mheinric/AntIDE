CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -I src -I tests
LDFLAGS =

SRC_DIR = src
TEST_DIR = tests
UNITY_DIR = tests/unity

SRC = $(SRC_DIR)/parser.c $(SRC_DIR)/parser2.c  $(SRC_DIR)/ast.c
HEADERS = $(SRC_DIR)/parser.h $(SRC_DIR)/utils.h $(SRC_DIR)/parser2.h $(SRC_DIR)/ast.h 
EXECUTABLE_SRC = $(SRC_DIR)/main.c $(SRC)
UNITY_SRC = $(UNITY_DIR)/unity.c
TEST_SRC = $(TEST_DIR)/test_main.c $(TEST_DIR)/test_parser.c $(TEST_DIR)/test_parser2.c

BIN = bin
EXECUTABLE = $(BIN)/ant_check
TEST_EXECUTABLE = $(BIN)/tests

.PHONY: all clean test test-continuous

all: $(EXECUTABLE)

$(BIN):
	mkdir -p $(BIN)

$(EXECUTABLE): $(BIN) $(EXECUTABLE_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(EXECUTABLE_SRC) $(LDFLAGS)

$(TEST_EXECUTABLE): $(BIN) $(TEST_SRC) $(SRC) $(UNITY_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(SRC) $(UNITY_SRC) $(LDFLAGS)

test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

test-continuous: 
	find . -name "*.[c,h]" | entr -r make test

clean:
	rm -rf $(BIN)
