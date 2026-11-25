#ifndef MAZE_H
#define MAZE_H

#include <stdbool.h>
#include <stdint.h>

#define MAZE_SIZE 32

#define WALL_NORTH 1
#define WALL_EAST  2
#define WALL_SOUTH 4
#define WALL_WEST  8

#define WALL_N WALL_NORTH
#define WALL_E WALL_EAST
#define WALL_S WALL_SOUTH
#define WALL_W WALL_WEST

#define FLAG_PASSAGE 0x01
#define FLAG_ZIGZAG  0x02

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    uint8_t walls;
    uint8_t flags; // Бітові прапорці
} Cell;

typedef struct {
    Cell cells[MAZE_SIZE][MAZE_SIZE];
    Point finish;
} Maze;

void init_maze(Maze *maze);
int maze_get_walls(const Maze *maze, int x, int y);
void print_maze(const Maze *maze);

#endif