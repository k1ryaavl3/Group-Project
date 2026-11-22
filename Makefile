CC = gcc
BUILD_DIR = build

SRCS = main.c \
       executable/algorithm.c \
       executable/maze.c \
       executable/mouse.c

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

ifeq ($(OS),Windows_NT)
	TARGET = $(BUILD_DIR)/micromouse.exe
	CFLAGS = -Wall -Wextra -std=c11 -O2 -I. -Ilibraries -Ilibraries/raylib-master/src
	LDFLAGS = -Llibraries -Llibraries/raylib-master/src -lraylib -lopengl32 -lgdi32 -lwinmm
	RUN_CMD = $(TARGET)
	ifeq ($(findstring sh.exe,$(SHELL)),sh.exe)
		MKDIR_P = mkdir -p $(1)
		CLEAN_CMD = rm -rf $(BUILD_DIR)
		COPY_DLL = cp -f libraries/raylib.dll $(BUILD_DIR)/ 2>/dev/null || true
	else
		MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
		CLEAN_CMD = if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
		COPY_DLL = if exist libraries\raylib.dll copy /Y libraries\raylib.dll $(BUILD_DIR)\ >nul
	endif
else
	UNAME_S := $(shell uname -s)
	TARGET = $(BUILD_DIR)/micromouse
	MKDIR_P = mkdir -p $(1)
	CLEAN_CMD = rm -rf $(BUILD_DIR)
	COPY_DLL = true
	RUN_CMD = ./$(TARGET)
	ifeq ($(UNAME_S),Darwin)
		CFLAGS = -Wall -Wextra -std=c11 -O2 -I. -Ilibraries
		LDFLAGS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	else
		HAS_LOCAL_RAYLIB := $(shell [ -d "../Group Project/raylib/build/raylib" ] && echo 1)
		ifeq ($(HAS_LOCAL_RAYLIB),1)
			CFLAGS = -Wall -Wextra -std=c11 -O2 -I. -Ilibraries -I"../Group Project/raylib/src"
			LDFLAGS = -L"../Group Project/raylib/build/raylib" -Wl,-rpath,"../Group Project/raylib/build/raylib" -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
		else
			CFLAGS = -Wall -Wextra -std=c11 -O2 -I. -Ilibraries -Ilibraries/raylib-master/src
			LDFLAGS = -Llibraries/raylib-master/src -Wl,-rpath,libraries/raylib-master/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
		endif
	endif
endif

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(call MKDIR_P,$(dir $@))
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	$(COPY_DLL)

$(BUILD_DIR)/%.o: %.c
	$(call MKDIR_P,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

run: build
	$(RUN_CMD)

clean:
	$(CLEAN_CMD)

.PHONY: all build run clean