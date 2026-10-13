#ifndef MOUSE_H
#define MOUSE_H
#define SENSOR_RANGE 3

#include <stdbool.h>
#include "maze.h"

typedef struct {
    int x;
    int y;
    int orientation;
    int known_walls[MAZE_SIZE][MAZE_SIZE];
    bool visited[MAZE_SIZE][MAZE_SIZE];
} Mouse;

void mouse_init(Mouse *mouse);
void mouse_update_sensors(Mouse *mouse, const Maze *real_maze);
void mouse_move(Mouse *mouse, int direction);

#endif