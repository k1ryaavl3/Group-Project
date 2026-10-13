#ifndef MAZE_H
#define MAZE_H

#define MAZE_SIZE 32

#define WALL_NORTH 0x1
#define WALL_EAST  0x2
#define WALL_SOUTH 0x4
#define WALL_WEST  0x8

typedef struct {
    unsigned char walls; 
    int visited;
    int is_passage;
    int is_zigzag;
} Cell;

extern Cell maze[MAZE_SIZE][MAZE_SIZE];

void init_maze();
void print_maze();

#endif
