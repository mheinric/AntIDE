CC = gcc

SRC_DIR = src
HEADER_DIR = includes/antide
TEST_DIR = tests
LIB_DIR = lib
EXTENSION_DIR = vscode_extension

VERSION = $(shell  jq -r '.version' $(EXTENSION_DIR)/package.json)

#-fwrapv : allow signed integer overflow
CFLAGS = -g -Wall -Wextra -Werror -fwrapv -I $(SRC_DIR) -I $(HEADER_DIR) -I $(TEST_DIR) -I $(LIB_DIR) -D'ANTIDE_VERSION="$(VERSION)"'
LDFLAGS =

SRC =  $(shell find $(SRC_DIR) -name '*.c' ! -name 'main.c') $(shell find $(LIB_DIR) -name '*.c')
HEADERS =  $(shell find $(HEADER_DIR) -name '*.h') $(shell find $(SRC_DIR) -name '*.h') $(shell find $(LIB_DIR) -name '*.h')
EXECUTABLE_SRC = $(SRC_DIR)/main.c $(SRC)
TEST_SRC = $(shell find $(TEST_DIR) -name '*.c')

JS_SRC = $(EXTENSION_DIR)/src/extension.ts $(EXTENSION_DIR)/media/grid.html 

BIN = bin
EXECUTABLE = $(BIN)/antide
TEST_EXECUTABLE = $(BIN)/tests
VSCODE_EXT = $(EXTENSION_DIR)/antide-$(VERSION).vsix

.PHONY: all clean test test-continuous

all: $(EXECUTABLE) $(VSCODE_EXT)

$(BIN):
	mkdir -p $(BIN)

$(EXECUTABLE): $(BIN) $(EXECUTABLE_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(EXECUTABLE_SRC) $(LDFLAGS)

$(VSCODE_EXT): $(EXECUTABLE) $(JS_SRC) $(EXTENSION_DIR)/.vscodeignore
	cd $(EXTENSION_DIR) && npm run compile
	cp $(EXECUTABLE) $(EXTENSION_DIR)/out
	cp License.md $(EXTENSION_DIR)/License.md
	cd $(EXTENSION_DIR) && vsce package
	mv -f $(VSCODE_EXT) $(BIN)

$(TEST_EXECUTABLE): $(BIN) $(TEST_SRC) $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(SRC) $(LDFLAGS)

test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

continuous: 
	find . -name "*.[c,h]" | entr -r make $(EXECUTABLE)

test-continuous: 
	find . -name "*.[c,h]" | entr -r make test

clean:
	rm -rf $(BIN)
	rm -rf $(EXTENSION_DIR)/out
