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
SERVICE_FILE = ec-aprs-wx.service

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

# Install binary, config, and service
install: all
	@echo "Installing ec-aprs-wx..."
	sudo cp $(OUTPUT) /usr/local/bin/ec-aprs-wx
	sudo cp $(CONFIG_DST) /etc/ec-aprs-wx.cfg
	sudo cp $(SERVICE_FILE) /etc/systemd/system/ec-aprs-wx.service
	sudo systemctl daemon-reload
	sudo systemctl enable ec-aprs-wx.service
	@echo "Installation complete. You can start the service with:"
	@echo "  sudo systemctl start ec-aprs-wx"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean install
