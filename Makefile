INCLUDE := src/include
SRC := src/main.c src/renderer.c src/ttml.c src/app.c

COMPILER ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -I$(INCLUDE)
LDFLAGS ?=

TARGET ?= build/nil

BUILD_DIR ?= build

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(COMPILER) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
