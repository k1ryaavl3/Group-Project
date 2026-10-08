#include <stdio.h>
#include "maze.h"
#include <time.h>
#include <stdlib.h>

// Глобальний масив лабіринту
Cell maze[MAZE_SIZE][MAZE_SIZE];
int finish_r = 15, finish_c = 15;

// Функція для перевірки, чи координати знаходяться в межах лабіринту
int is_valid(int x, int y) {
    return x >= 0 && x < MAZE_SIZE && y >= 0 && y < MAZE_SIZE;
}

void carve_2x2(int r, int c) {
    maze[r][c].walls &= ~WALL_EAST;
    maze[r][c+1].walls &= ~WALL_WEST;
    
    maze[r+1][c].walls &= ~WALL_EAST;
    maze[r+1][c+1].walls &= ~WALL_WEST;
    
    maze[r][c].walls &= ~WALL_SOUTH;
    maze[r+1][c].walls &= ~WALL_NORTH;
    
    maze[r][c+1].walls &= ~WALL_SOUTH;
    maze[r+1][c+1].walls &= ~WALL_NORTH;
}

void setup_finish() {
    finish_r = 15;
    finish_c = 15;
    carve_2x2(finish_r, finish_c);
}

void carve_passages(int x, int y, int last_dir) {
    maze[x][y].visited = 1;

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

unsigned char walls[] = {WALL_NORTH, WALL_SOUTH, WALL_WEST, WALL_EAST};
unsigned char opp_walls[] = {WALL_SOUTH, WALL_NORTH, WALL_EAST, WALL_WEST};

int dirs[] = {0, 1, 2, 3}; // 0: North, 1: South, 2: West, 3: East

if (last_dir != -1 && rand() % 100 < 70) { //70 відсотків шансів продовжити в тому ж напрямку
    for (int i = 0; i < 4; i++) {
        if (dirs[i] == last_dir) {
            int temp = dirs[0];
            dirs[0] = dirs[i];
            dirs[i] = temp;
            break;
        }
    }
    for (int i = 1; i < 4; i++) { // Перемішуємо інші напрямки
        int r = rand() % 3;
        int temp = dirs[i];
        dirs[i] = dirs[r];
        dirs[r] = temp;
    } 
} else {
    for (int i = 0; i < 4; i++) {
        int r = rand() % 4;
        int temp = dirs[i];
        dirs[i] = dirs[r];
        dirs[r] = temp;
    }
}

//Перевіряємо чи сусідня клітина в межах сітки і ще не відвідана
for (int i = 0; i < 4; i++) {
    int nx = x + dx[dirs[i]];
    int ny = y + dy[dirs[i]];

    if (is_valid(nx, ny) && !maze[nx][ny].visited) {
        maze[x][y].walls &= ~walls[dirs[i]];
        maze[nx][ny].walls &= ~opp_walls[dirs[i]];
        carve_passages(nx, ny, dirs[i]);
    }
    }
}
void add_loops(int extra_walls_to_remove) {
    for (int k = 0; k < extra_walls_to_remove; k++) {
        int r = rand() % MAZE_SIZE;
        int c = rand() % MAZE_SIZE;
        int dir = rand() % 4;
        
        if (dir == 0 && is_valid(r - 1, c) && (maze[r][c].walls & WALL_NORTH)) {
            maze[r][c].walls &= ~WALL_NORTH;
            maze[r - 1][c].walls &= ~WALL_SOUTH;
        } else if (dir == 1 && is_valid(r + 1, c) && (maze[r][c].walls & WALL_SOUTH)) {
            maze[r][c].walls &= ~WALL_SOUTH;
            maze[r + 1][c].walls &= ~WALL_NORTH;
        } else if (dir == 2 && is_valid(r, c - 1) && (maze[r][c].walls & WALL_WEST)) {
            maze[r][c].walls &= ~WALL_WEST;
            maze[r][c - 1].walls &= ~WALL_EAST;
        } else if (dir == 3 && is_valid(r, c + 1) && (maze[r][c].walls & WALL_EAST)) {
            maze[r][c].walls &= ~WALL_EAST;
            maze[r][c + 1].walls &= ~WALL_WEST;
        }
    }
}

void init_maze()  {
    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            maze[i][j].walls = WALL_NORTH | WALL_SOUTH | WALL_EAST | WALL_WEST;
            maze[i][j].visited = 0;
        }
    }

    srand(time(NULL));
    carve_passages (0, 0, -1); // Починаємо з верхнього лівого кута

    add_loops(80); // Додаємо додаткові проходи для ускладнення лабіринту

    if (is_valid(0, 1)) {
        maze[0][0].walls &= ~WALL_EAST;
        maze[0][1].walls &= ~WALL_WEST;
    }
    if (is_valid(1, 0)) {
        maze[0][0].walls &= ~WALL_SOUTH;
        maze[1][0].walls &= ~WALL_NORTH;
    }

    setup_finish(); 
}

void print_maze() {
    printf("Maze:\n");

// Малюємо верхню межу для всього рядка лабіринту
    for (int j = 0; j < MAZE_SIZE; j++) {
        printf("+---");
    }
    printf("+\n");

    for (int i = 0; i < MAZE_SIZE; i++) {
        printf("|"); // Ліва межа лабіринту
        
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (i >= finish_r && i < finish_r + 2 && j >= finish_c && j < finish_c + 2) {
                printf("\033[42m F \033[0m"); 
            } else {
                printf("   "); 
            }
            
            // Перевіряємо, чи є права стіна у цієї клітинки
            if (maze[i][j].walls & WALL_EAST) {
                printf("|"); // Якщо стіна є — малюємо вертикальну риску
            } else {
                printf(" "); // Якщо стіни немає — залишаємо прохід
            }
        }
        printf("\n");

        // Малюємо нижні стіни для кожної клітинки поточного рядка
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (maze[i][j].walls & WALL_SOUTH) {
                printf("+---"); // Якщо є нижня стіна
            } else {
                printf("+   "); // Якщо є прохід вниз
            }
        }
        printf("+\n");
    }
}