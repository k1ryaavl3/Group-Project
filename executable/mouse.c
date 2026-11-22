#include "mouse.h"

static const int DX[4] = { 0, 1, 0, -1 };
static const int DY[4] = { -1, 0, 1, 0 };
static const int DIR_WALLS[4] = { WALL_N, WALL_E, WALL_S, WALL_W };
static const int OPPOSITE[16] = {[WALL_N]=WALL_S, [WALL_S]=WALL_N, [WALL_E]=WALL_W, [WALL_W]=WALL_E};
static const float DIR_ANGLES[4] = { -90.0f, 0.0f, 90.0f, 180.0f };

void mouse_init(Mouse *mouse) {
    mouse->x = 0;
    mouse->y = 0;
    mouse->orientation = 0;
    mouse->pos_x = 0.5f;
    mouse->pos_y = 0.5f;
    mouse->angle = DIR_ANGLES[0];

    for (int y = 0; y < MAZE_SIZE; y++) {
        for (int x = 0; x < MAZE_SIZE; x++) {
            mouse->known_walls[y][x] = 0;
            mouse->visited[y][x] = false;
        }
    }

    mouse->visited[0][0] = true;
}

void mouse_update_sensors(Mouse *mouse, const Maze *real_maze) {
    mouse->visited[mouse->y][mouse->x] = true;
    int real_walls = maze_get_walls(real_maze, mouse->x, mouse->y);

    for (int direction = 0; direction < 4; direction++) {
        int wall = DIR_WALLS[direction];

        if ((real_walls & wall) != 0) {
            mouse->known_walls[mouse->y][mouse->x] |= wall;

            int adj_x = mouse->x + DX[direction];
            int adj_y = mouse->y + DY[direction];
            if (adj_x >= 0 && adj_x < MAZE_SIZE && adj_y >= 0 && adj_y < MAZE_SIZE) {
                mouse->known_walls[adj_y][adj_x] |= OPPOSITE[wall];
            }
        }
    }
}

void mouse_move(Mouse *mouse, int direction) {
    mouse->orientation = direction;
    mouse->x += DX[direction];
    mouse->y += DY[direction];
    mouse->pos_x = mouse->x + 0.5f;
    mouse->pos_y = mouse->y + 0.5f;
    mouse->angle = DIR_ANGLES[direction];
}