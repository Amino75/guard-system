.PHONY: all clean rebuild run

BUILD_DIR := build
TARGET := $(BUILD_DIR)/guard-system

# Default target: build
all: $(TARGET)

# Configure and build
$(TARGET):
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && $(MAKE)

# Clean: remove build directory
clean:
	rm -rf $(BUILD_DIR)

# Rebuild: clean then build
rebuild: clean all


# Run the executable
run: $(TARGET)
	./$(TARGET)