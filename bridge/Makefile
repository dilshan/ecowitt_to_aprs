# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./src -I./src/mongoose
LDFLAGS = -lm

# Directories and files
SRC_DIR = src
MONGOOSE_DIR = $(SRC_DIR)/mongoose
BUILD_DIR = build
BIN_DIR = bin
OUTPUT = $(BIN_DIR)/ec-aprs-wx
CONFIG_SRC = sample.cfg
CONFIG_DST = $(BIN_DIR)/config.cfg

# Source and object files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(MONGOOSE_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Default target
all: $(OUTPUT) $(CONFIG_DST)

# Link the binary
$(OUTPUT): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Copy config file
$(CONFIG_DST): $(CONFIG_SRC)
	@mkdir -p $(BIN_DIR)
	cp $< $@

# Compile .c to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
