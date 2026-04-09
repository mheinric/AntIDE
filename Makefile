CC = gcc
CFLAGS = -Wall -Wextra -I src -I tests
LDFLAGS =

SRC_DIR = src
TEST_DIR = tests
UNITY_DIR = tests/unity

SRC = $(SRC_DIR)/lib.c
EXECUTABLE_SRC = $(SRC_DIR)/main.c $(SRC)
UNITY_SRC = $(UNITY_DIR)/unity.c
TEST_SRC = $(TEST_DIR)/test_add.c

BIN = bin
EXECUTABLE = $(BIN)/tests
TEST_EXECUTABLE = $(BIN)/tests

.PHONY: all clean test test-continuous

all: $(EXECUTABLE)

$(BIN):
	mkdir -p $(BIN)

$(EXECUTABLE): $(BIN) $(EXECUTABLE_SRC)
	$(CC) $(CFLAGS) -o $@ $(EXECUTABLE_SRC) $(LDFLAGS)

$(TEST_EXECUTABLE): $(BIN) $(SRC) $(UNITY_SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(UNITY_SRC) $(TEST_SRC) $(LDFLAGS)

test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

test-continuous: 
	find . -name "*.[c,h]" | entr -r make test

clean:
	rm -rf $(BIN)
