#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "mouse.h"
#include <stdbool.h>

typedef enum {
    MODE_EXPLORE,
    MODE_RETURN,
    MODE_SPEEDRUN
} PathMode;

typedef struct {
    float x;
    float y;
    float angle;       // 0, 45, 90, 135, 180, 225, 270, 315
    bool is_diagonal;
} Waypoint;


typedef struct {
    Waypoint points[MAZE_SIZE * MAZE_SIZE];
    int count;
    int current_idx;
} SpeedrunPlan;

void algorithm_compute_distances(const Mouse *mouse, Point target, int dist[MAZE_SIZE][MAZE_SIZE]);
int algorithm_get_next_move(const Mouse *mouse, Point target);

bool algorithm_build_speedrun_plan(const Mouse *mouse, Point target, SpeedrunPlan *plan);
bool algorithm_step_speedrun(Mouse *mouse, SpeedrunPlan *plan, float step_distance);

#endif