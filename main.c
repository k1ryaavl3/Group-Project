#include "raylib.h"
#include <stdio.h>
#include "maze.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define CELL_SIZE (SCREEN_WIDTH / MAZE_SIZE)

extern Cell maze[MAZE_SIZE][MAZE_SIZE];
extern int finish_r, finish_c;

int main(void) {
    init_maze();

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MicroMouse Maze Simulation");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int i = 0; i < MAZE_SIZE; i++) {
            for (int j = 0; j < MAZE_SIZE; j++) {
                int x = j * CELL_SIZE;
                int y = i * CELL_SIZE;

                if (i >= finish_r && i < finish_r + 2 && j >= finish_c && j < finish_c + 2) {
                    DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, GREEN);
                } 
                if (maze[i][j].walls & WALL_NORTH) {
                    DrawLine(x, y, x + CELL_SIZE, y, BLACK);
                }
                if (maze[i][j].walls & WALL_EAST) {
                    DrawLine(x + CELL_SIZE, y, x + CELL_SIZE, y + CELL_SIZE, BLACK);
                }
                if (maze[i][j].walls & WALL_SOUTH) {
                    DrawLine(x, y + CELL_SIZE, x + CELL_SIZE, y + CELL_SIZE, BLACK);
                }
                if (maze[i][j].walls & WALL_WEST) {
                    DrawLine(x, y, x, y + CELL_SIZE, BLACK);
                }
            }
        }
        DrawRectangle(0, 0, CELL_SIZE, CELL_SIZE, BLUE); // Start
        EndDrawing();
    }

    CloseWindow();
    return 0;
}