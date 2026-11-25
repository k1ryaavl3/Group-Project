# Group-Project
Kovalenko's and Petrov's group project 

- maze.h(7): #define MAZE_SIZE 32 - визначає розмір лабіринту
- maze.c(235): add_loops(maze, 80); - кизначає кількість дірок
- algorithm.c(12-17):
- #define TIME_DIAG_CONT   71   // Продовження діагоналі 45° (чергування Left <-> Right)
- #define TIME_DIAG_ENTER  95   // Вхід у діагональ 45° з прямої ділянки
- #define TIME_STRAIGHT    100  // Рух прямо без зміни напрямку
- #define TIME_DIAG_EXIT   120  // Вихід із діагоналі 45° на пряму
- #define TIME_SHARP_90    165  // Два повороти в один бік поспіль (гострий кут 90°)
- #define TIME_UTURN       300  // Розворот
- вартості поворотів
