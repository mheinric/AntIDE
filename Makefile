CC = gcc

SRC_DIR = src
HEADER_DIR = includes/antide
TEST_DIR = tests

CFLAGS = -g -Wall -Wextra -Werror -I $(SRC_DIR) -I $(HEADER_DIR) -I $(TEST_DIR)
LDFLAGS =

UNITY_DIR = tests/unity

SRC = $(SRC_DIR)/parser.c $(SRC_DIR)/ast.c $(SRC_DIR)/simulation.c
HEADERS = $(HEADER_DIR)/utils.h $(HEADER_DIR)/parser.h $(HEADER_DIR)/ast.h $(SRC_DIR)/internals/vector.h $(SRC_DIR)/parser_private.h $(SRC_DIR)/simulation_private.h
EXECUTABLE_SRC = $(SRC_DIR)/main.c $(SRC)
UNITY_SRC = $(UNITY_DIR)/unity.c
TEST_SRC = $(TEST_DIR)/test_main.c $(TEST_DIR)/test_parser.c $(TEST_DIR)/test_simulation.c

BIN = bin
EXECUTABLE = $(BIN)/antide
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
