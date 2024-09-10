PREFIX?=/usr

CC=gcc
CFLAGS=-Wall -g -std=c11 -O3
LDFLAGS=-lvcd -lvjudge

SRC_DIR=src
BUILD_DIR=build

SOURCES=$(wildcard $(SRC_DIR)/*.c)
OBJECTS=$(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))
EXECUTABLE=$(BUILD_DIR)/vjudge

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $@

install: all
	install -m 755 $(EXECUTABLE) $(PREFIX)/bin/

clean:
	rm -rf $(BUILD_DIR)

uninstall:
	rm $(PREFIX)/bin/$(notdir $(EXECUTABLE))

.PHONY: all install clean uninstall
