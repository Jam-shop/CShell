CC = gcc
CFLAGS = -std=c99 \
		 -D_POSIX_C_SOURCE=200809L \
		 -D_XOPEN_SOURCE=700 \
		 -Wall -Wextra -Werror \
		 -Wno-unused-parameter \
		 -fno-asm \
		 -Iinclude

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

SRCS = $(SRC_DIR)/activities.c \
	   $(SRC_DIR)/bgExecution.c \
	   $(SRC_DIR)/cfgParser.c \
	   $(SRC_DIR)/hop.c \
	   $(SRC_DIR)/keyboardShortcuts.c \
	   $(SRC_DIR)/log.c \
	   $(SRC_DIR)/redirection.c \
	   $(SRC_DIR)/reveal.c \
	   $(SRC_DIR)/sequential.c \
	   $(SRC_DIR)/shellInput.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = shell.out


all: $(TARGET)

$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
		$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@


clean:
		rm -f $(BUILD_DIR) $(TARGET)

install: $(TARGET)
		cp $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
		rm -f /usr/local/bin/$(TARGET)

run: $(TARGET)
		./$(TARGET)image.png

debug: CFLAGS += -g
debug: $(TARGET)


.PHONY: all clean install uninstall run debug