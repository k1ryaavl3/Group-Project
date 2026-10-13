#include "mouse.h"

static const int DX[4] = { 0, 1, 0, -1 };
static const int DY[4] = { -1, 0, 1, 0 };
static const int DIR_WALLS[4] = { WALL_N, WALL_E, WALL_S, WALL_W };
static const int OPPOSITE[16] = {[WALL_N]=WALL_S, [WALL_S]=WALL_N, [WALL_E]=WALL_W, [WALL_W]=WALL_E};

void mouse_init(Mouse *mouse) {
    mouse->x = 0;
    mouse->y = 0;
    mouse->orientation = 0;

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

    for (int direction = 0; direction < 4; direction++) {
        for (int visibility = 0; visibility < SENSOR_RANGE; visibility++) {
            int nx = mouse->x + DX[direction] * visibility;
            int ny = mouse->y + DY[direction] * visibility;

            if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) break;

            int real_walls = maze_get_walls(real_maze, nx, ny);
            if ((real_walls & DIR_WALLS[direction]) != 0) {
                mouse->known_walls[ny][nx] |= DIR_WALLS[direction];
                break;
            }
        }
    }
}

void mouse_move(Mouse *mouse, int direction) {
    mouse->orientation = direction;
    mouse->x += DX[direction];
    mouse->y += DY[direction];
}