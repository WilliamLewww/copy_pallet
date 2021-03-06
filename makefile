CURRENT_PATH=$(shell pwd)

BIN_PATH=$(CURRENT_PATH)/bin
BUILD_PATH=$(CURRENT_PATH)/build
SRC_PATH=$(CURRENT_PATH)/src

CC=gcc
GDB=gdb

COMPILER_FLAGS=-Wall -Wextra
LINKED_LIBRARIES=-lX11

EXEC=copy-pallet.out

all: clean $(EXEC)

SRCS := $(wildcard src/*.c)
OBJS := $(notdir $(SRCS:%.c=%.o))

$(EXEC): $(OBJS)
	$(CC) $(COMPILER_FLAGS) -o $(BIN_PATH)/$(EXEC) $(BUILD_PATH)/*.o $(LINKED_LIBRARIES)

%.o: $(SRC_PATH)/%.c
	$(CC) $(COMPILER_FLAGS) -c $^ -o $(BUILD_PATH)/$@

%.o: $(SRC_PATH)/*/%.c
	$(CC) $(COMPILER_FLAGS) -c $^ -o $(BUILD_PATH)/$@

run:
	$(BIN_PATH)/$(EXEC)

clean: SHELL:=/bin/bash
clean:
	find $(BIN_PATH) -type f -not -name '.gitkeep' -delete
	find $(BUILD_PATH) -type f -not -name '.gitkeep' -delete