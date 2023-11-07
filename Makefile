CC=gcc
CFLAGS=-Wall -g -std=c11 -O3 -Isrc/ -Iinclude/
LDFLAGS=

SRC_DIR=src
LIB_DIR=lib
BUILD_DIR=build

SOURCES=$(wildcard $(SRC_DIR)/*.c $(LIB_DIR)/*.c)
OBJECTS=$(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.a, $(SOURCES))
EXECUTABLE=$(BUILD_DIR)/vjudge

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.a: $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

install: all
	install -m 755 $(EXECUTABLE) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/vjudges

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all install uninstall clean
