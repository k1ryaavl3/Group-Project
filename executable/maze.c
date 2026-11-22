#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "maze.h"

static bool is_valid(int x, int y);
static void carve_2x2(Maze *maze, int r, int c);
static void setup_finish(Maze *maze);
static void carve_passages(Maze *maze, uint8_t visited[MAZE_SIZE][MAZE_SIZE], int x, int y, int last_dir);
static void add_loops(Maze *maze, int extra_walls_to_remove);
static void add_zigzag(Maze *maze, uint8_t visited[MAZE_SIZE][MAZE_SIZE], int num_zigzag);

int maze_get_walls(const Maze *maze, int x, int y) {
    if (!is_valid(x, y)) {
        return WALL_NORTH | WALL_SOUTH | WALL_EAST | WALL_WEST;
    }
    return maze->cells[y][x].walls;
}

static bool is_valid(int x, int y) {
    return x >= 0 && x < MAZE_SIZE && y >= 0 && y < MAZE_SIZE;
}

static void carve_2x2(Maze *maze, int r, int c) {
    maze->cells[r][c].walls &= ~WALL_EAST;
    maze->cells[r][c+1].walls &= ~WALL_WEST;
    
    maze->cells[r+1][c].walls &= ~WALL_EAST;
    maze->cells[r+1][c+1].walls &= ~WALL_WEST;
    
    maze->cells[r][c].walls &= ~WALL_SOUTH;
    maze->cells[r+1][c].walls &= ~WALL_NORTH;
    
    maze->cells[r][c+1].walls &= ~WALL_SOUTH;
    maze->cells[r+1][c+1].walls &= ~WALL_NORTH;
}

static void setup_finish(Maze *maze) {
    int center = MAZE_SIZE / 2 - 1;
    maze->finish.x = center;
    maze->finish.y = center;
    carve_2x2(maze, center, center);
}

static void carve_passages(Maze *maze, uint8_t visited[MAZE_SIZE][MAZE_SIZE], int x, int y, int last_dir) {
    visited[x][y] = 1;

    static const int dx[] = {-1, 1, 0, 0};
    static const int dy[] = {0, 0, -1, 1};

    static const unsigned char walls[] = {WALL_NORTH, WALL_SOUTH, WALL_WEST, WALL_EAST};
    static const unsigned char opp_walls[] = {WALL_SOUTH, WALL_NORTH, WALL_EAST, WALL_WEST};

    int dirs[] = {0, 1, 2, 3}; 

    if (last_dir != -1 && rand() % 100 < 80) { 
        for (int i = 0; i < 4; i++) {
            if (dirs[i] == last_dir) {
                int temp = dirs[0];
                dirs[0] = dirs[i];
                dirs[i] = temp;
                break;
            }
        }
        for (int i = 1; i < 4; i++) {
            int r = 1 + rand() % 3;
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

    for (int i = 0; i < 4; i++) {
        int nx = x + dx[dirs[i]];
        int ny = y + dy[dirs[i]];

        if (is_valid(nx, ny) && !visited[nx][ny]) {
            maze->cells[x][y].walls &= ~walls[dirs[i]];
            maze->cells[nx][ny].walls &= ~opp_walls[dirs[i]];

            if (dirs[i] == last_dir) {
                maze->cells[x][y].flags |= FLAG_PASSAGE;   
                maze->cells[nx][ny].flags |= FLAG_PASSAGE; 
            }
            carve_passages(maze, visited, nx, ny, dirs[i]);
        }
    }
}

static void add_loops(Maze *maze, int extra_walls_to_remove) {
    static const int dr[] = {-1, 1, 0, 0};
    static const int dc[] = {0, 0, -1, 1};
    static const unsigned char walls[] = {WALL_NORTH, WALL_SOUTH, WALL_WEST, WALL_EAST};
    static const unsigned char opp_walls[] = {WALL_SOUTH, WALL_NORTH, WALL_EAST, WALL_WEST};

    for (int k = 0; k < extra_walls_to_remove; k++) {
        int r = rand() % MAZE_SIZE;
        int c = rand() % MAZE_SIZE;
        int dir = rand() % 4;

        int nr = r + dr[dir];
        int nc = c + dc[dir];

        if (is_valid(nr, nc) && (maze->cells[r][c].walls & walls[dir])) {
            maze->cells[r][c].walls &= ~walls[dir];
            maze->cells[nr][nc].walls &= ~opp_walls[dir];
        }
    }
}

// НОВА ЛОГІКА ЗИГЗАГІВ: Працює на етапі ПОПЕРЕДНЬОЇ генерації
static void add_zigzag(Maze *maze, uint8_t visited[MAZE_SIZE][MAZE_SIZE], int num_zigzag) {
    for (int k = 0; k < num_zigzag; k++) {
        int lenght = 8 + rand() % 8; 
        int dir = rand() % 4; 

        int curr_r = 4 + rand() % (MAZE_SIZE - 8);
        int curr_c = 4 + rand() % (MAZE_SIZE - 8);

        // Не починаємо там, де вже є проходи, або біля фінішу
        if (visited[curr_r][curr_c] || (abs(curr_r - maze->finish.y) < 5 && abs(curr_c - maze->finish.x) < 5)) {
            continue;
        }

        int start_r = curr_r;
        int start_c = curr_c;
        int actual_steps = 0;

        for (int step = 0; step < lenght; step++) {
            visited[curr_r][curr_c] = 1; // Заливаємо «бетоном», щоб основний алгоритм сюди не заліз
            maze->cells[curr_r][curr_c].flags |= FLAG_ZIGZAG;
            actual_steps++;

            if (dir == 0) { // Вниз-вправо
                if (step % 2 == 0) { // вправо
                    if (is_valid(curr_r, curr_c + 1) && !visited[curr_r][curr_c + 1]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_EAST;
                        maze->cells[curr_r][curr_c + 1].walls &= ~WALL_WEST;
                        curr_c++;
                    } else break; // Зупиняємося, якщо врізалися в іншу структуру
                } else { // вниз
                    if (is_valid(curr_r + 1, curr_c) && !visited[curr_r + 1][curr_c]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_SOUTH;
                        maze->cells[curr_r + 1][curr_c].walls &= ~WALL_NORTH;
                        curr_r++;
                    } else break;
                }
            } else if (dir == 1) { // Вниз-вліво
                if (step % 2 == 0) { // вліво
                    if (is_valid(curr_r, curr_c - 1) && !visited[curr_r][curr_c - 1]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_WEST;
                        maze->cells[curr_r][curr_c - 1].walls &= ~WALL_EAST;
                        curr_c--;
                    } else break;
                } else { // вниз
                    if (is_valid(curr_r + 1, curr_c) && !visited[curr_r + 1][curr_c]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_SOUTH;
                        maze->cells[curr_r + 1][curr_c].walls &= ~WALL_NORTH;
                        curr_r++;
                    } else break;
                }
            } else if (dir == 2) { // Вверх-вправо
                if (step % 2 == 0) { // вправо
                    if (is_valid(curr_r, curr_c + 1) && !visited[curr_r][curr_c + 1]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_EAST;
                        maze->cells[curr_r][curr_c + 1].walls &= ~WALL_WEST;
                        curr_c++;
                    } else break;
                } else { // вверх
                    if (is_valid(curr_r - 1, curr_c) && !visited[curr_r - 1][curr_c]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_NORTH;
                        maze->cells[curr_r - 1][curr_c].walls &= ~WALL_SOUTH;
                        curr_r--;
                    } else break;
                }
            } else if (dir == 3) { // Вверх-вліво
                if (step % 2 == 0) { // вліво
                    if (is_valid(curr_r, curr_c - 1) && !visited[curr_r][curr_c - 1]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_WEST;
                        maze->cells[curr_r][curr_c - 1].walls &= ~WALL_EAST;
                        curr_c--;
                    } else break;
                } else { // вверх
                    if (is_valid(curr_r - 1, curr_c) && !visited[curr_r - 1][curr_c]) {
                        maze->cells[curr_r][curr_c].walls &= ~WALL_NORTH;
                        maze->cells[curr_r - 1][curr_c].walls &= ~WALL_SOUTH;
                        curr_r--;
                    } else break;
                }
            }
        }

        // Позначаємо останню клітинку
        visited[curr_r][curr_c] = 1;
        maze->cells[curr_r][curr_c].flags |= FLAG_ZIGZAG;

        if (actual_steps > 1) {
            visited[start_r][start_c] = 0;
            visited[curr_r][curr_c] = 0;
        }
    }
}

void init_maze(Maze *maze) {
    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            maze->cells[i][j].walls = WALL_NORTH | WALL_SOUTH | WALL_EAST | WALL_WEST;
            maze->cells[i][j].flags = 0;
        }
    }

    uint8_t visited[MAZE_SIZE][MAZE_SIZE] = {0};
    srand(time(NULL));

    //резервуємо центр, щоб там не нагенерувалося сміття
    int center = MAZE_SIZE / 2 - 1;
    maze->finish.x = center;
    maze->finish.y = center;
    visited[center][center] = 1;
    visited[center][center+1] = 1;
    visited[center+1][center] = 1;
    visited[center+1][center+1] = 1;
    visited[center][center] = 0; // Робимо одну клітинку невідвіданою, щоб алгоритм знайшов до неї вхід!

    add_zigzag(maze, visited, 15); 

    carve_passages(maze, visited, 0, 0, -1); 

    add_loops(maze, 80); 

    // 5. Відкриваємо вхід
    if (is_valid(0, 1)) {
        maze->cells[0][0].walls &= ~WALL_EAST;
        maze->cells[0][1].walls &= ~WALL_WEST;
    }
    if (is_valid(1, 0)) {
        maze->cells[0][0].walls &= ~WALL_SOUTH;
        maze->cells[1][0].walls &= ~WALL_NORTH;
    }

    setup_finish(maze); 
}

void print_maze(const Maze *maze) {
    printf("Maze:\n");
    for (int j = 0; j < MAZE_SIZE; j++) printf("+---");
    printf("+\n");

    for (int i = 0; i < MAZE_SIZE; i++) {
        printf("|"); 
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (i >= maze->finish.y && i < maze->finish.y + 2 && j >= maze->finish.x && j < maze->finish.x + 2) {
                printf("\033[42m F \033[0m"); 
            } else if (maze->cells[i][j].flags & FLAG_ZIGZAG) {
                printf("\033[43m Z \033[0m"); 
            } else if (maze->cells[i][j].flags & FLAG_PASSAGE) {
                printf("\033[41m P \033[0m"); 
            } else {
                printf("   "); 
            }
            if (maze->cells[i][j].walls & WALL_EAST) printf("|"); 
            else printf(" "); 
        }
        printf("\n");
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (maze->cells[i][j].walls & WALL_SOUTH) printf("+---"); 
            else printf("+   "); 
        }
        printf("+\n");
    }
}