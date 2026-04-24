CC = gcc

SRC_DIR = src
HEADER_DIR = includes/antide
TEST_DIR = tests
LIB_DIR = lib

CFLAGS = -g -Wall -Wextra -Werror -I $(SRC_DIR) -I $(HEADER_DIR) -I $(TEST_DIR) -I $(LIB_DIR)
LDFLAGS =

SRC =  $(shell find $(SRC_DIR) -name '*.c' ! -name 'main.c') $(shell find $(LIB_DIR) -name '*.c')
HEADERS =  $(shell find $(HEADER_DIR) -name '*.h') $(shell find $(SRC_DIR) -name '*.h') $(shell find $(LIB_DIR) -name '*.h')
EXECUTABLE_SRC = $(SRC_DIR)/main.c $(SRC)
TEST_SRC = $(shell find $(TEST_DIR) -name '*.c')

BIN = bin
EXECUTABLE = $(BIN)/antide
TEST_EXECUTABLE = $(BIN)/tests

.PHONY: all clean test test-continuous

all: $(EXECUTABLE)

$(BIN):
	mkdir -p $(BIN)

$(EXECUTABLE): $(BIN) $(EXECUTABLE_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(EXECUTABLE_SRC) $(LDFLAGS)

$(TEST_EXECUTABLE): $(BIN) $(TEST_SRC) $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(SRC) $(LDFLAGS)

test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

continuous: 
	find . -name "*.[c,h]" | entr -r make

test-continuous: 
	find . -name "*.[c,h]" | entr -r make test

clean:
	rm -rf $(BIN)
