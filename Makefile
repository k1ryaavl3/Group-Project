CC = gcc
CFLAGS = -Wall -I./raylib/src
LDFLAGS = -L./raylib/build/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: maze_gui

maze_gui: main.c maze.c
	$(CC) main.c maze.c -o maze_gui $(CFLAGS) $(LDFLAGS)

run: maze_gui
	LD_LIBRARY_PATH=./raylib/build/raylib ./maze_gui

clean:
	rm -f maze_gui