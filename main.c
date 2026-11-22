#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "maze.h"
#include "mouse.h"
#include "algorithm.h"

#define VIEW_SIZE 640
#define SCREEN_WIDTH (VIEW_SIZE * 2)
#define SCREEN_HEIGHT VIEW_SIZE
#define CELL_SIZE (VIEW_SIZE / MAZE_SIZE)

#define TILE_3D 2.0f
#define VIEW_RADIUS 6

typedef enum {
    SCREEN_MENU,
    SCREEN_SETTINGS,
    SCREEN_SIMULATION
} AppScreen;

static Color apply_lidar_fog(Color base, float dist, float max_dist) {
    float factor = 1.0f - (dist / max_dist);
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    factor = factor * factor;

    Color bg = { 4, 7, 10, 255 };
    Color res;
    res.r = (unsigned char)(bg.r + (base.r - bg.r) * factor);
    res.g = (unsigned char)(bg.g + (base.g - bg.g) * factor);
    res.b = (unsigned char)(bg.b + (base.b - bg.b) * factor);
    res.a = 255;
    return res;
}

static bool has_h_wall(const Maze *maze, int c, int vr) {
    if (c < 0 || c >= MAZE_SIZE || vr < 0 || vr > MAZE_SIZE) return false;
    if (vr == 0) return (maze_get_walls(maze, c, 0) & WALL_NORTH) != 0;
    return (maze_get_walls(maze, c, vr - 1) & WALL_SOUTH) != 0;
}

static bool has_v_wall(const Maze *maze, int vc, int r) {
    if (vc < 0 || vc > MAZE_SIZE || r < 0 || r >= MAZE_SIZE) return false;
    if (vc == 0) return (maze_get_walls(maze, 0, r) & WALL_WEST) != 0;
    return (maze_get_walls(maze, vc - 1, r) & WALL_EAST) != 0;
}

static void draw_wall_segment_lines(float x0, float z0, float x1, float z1, float h, Color col) {
    DrawLine3D((Vector3){ x0, h, z0 }, (Vector3){ x1, h, z1 }, col);
    DrawLine3D((Vector3){ x0, 0.002f, z0 }, (Vector3){ x1, 0.002f, z1 }, col);
}

static void draw_maze_lidar_3d(const Maze *maze, int mouse_c, int mouse_r, float cam_x, float cam_z) {
    float max_dist = (VIEW_RADIUS + 0.6f) * TILE_3D;
    float wall_h = 0.62f;
    float wall_t = 0.12f;
    float half_t = wall_t * 0.5f;

    int min_r = mouse_r - VIEW_RADIUS - 1;
    int max_r = mouse_r + VIEW_RADIUS + 1;
    int min_c = mouse_c - VIEW_RADIUS - 1;
    int max_c = mouse_c + VIEW_RADIUS + 1;

    if (min_r < 0) min_r = 0;
    if (max_r >= MAZE_SIZE) max_r = MAZE_SIZE - 1;
    if (min_c < 0) min_c = 0;
    if (max_c >= MAZE_SIZE) max_c = MAZE_SIZE - 1;

    Color void_core = { 4, 7, 10, 255 };
    Color floor_base = { 10, 85, 105, 255 };
    Color wall_base  = { 0, 235, 255, 255 };

    for (int vr = min_r; vr <= max_r + 1; vr++) {
        float z = (vr - 0.5f) * TILE_3D;
        for (int c = min_c; c <= max_c; c++) {
            float x0 = (c - 0.5f) * TILE_3D;
            float x1 = (c + 0.5f) * TILE_3D;
            float mx = (x0 + x1) * 0.5f;
            float dist = sqrtf((mx - cam_x) * (mx - cam_x) + (z - cam_z) * (z - cam_z));
            if (dist <= max_dist) {
                DrawLine3D((Vector3){ x0, 0.0f, z }, (Vector3){ x1, 0.0f, z }, apply_lidar_fog(floor_base, dist, max_dist));
            }
        }
    }

    for (int vc = min_c; vc <= max_c + 1; vc++) {
        float x = (vc - 0.5f) * TILE_3D;
        for (int r = min_r; r <= max_r; r++) {
            float z0 = (r - 0.5f) * TILE_3D;
            float z1 = (r + 0.5f) * TILE_3D;
            float mz = (z0 + z1) * 0.5f;
            float dist = sqrtf((x - cam_x) * (x - cam_x) + (mz - cam_z) * (mz - cam_z));
            if (dist <= max_dist) {
                DrawLine3D((Vector3){ x, 0.0f, z0 }, (Vector3){ x, 0.0f, z1 }, apply_lidar_fog(floor_base, dist, max_dist));
            }
        }
    }

    for (int vr = min_r; vr <= max_r + 1; vr++) {
        float cz = (vr - 0.5f) * TILE_3D;
        for (int c = min_c; c <= max_c; c++) {
            if (!has_h_wall(maze, c, vr)) continue;
            float cx = c * TILE_3D;
            float dist = sqrtf((cx - cam_x) * (cx - cam_x) + (cz - cam_z) * (cz - cam_z));
            if (dist <= max_dist) {
                DrawCube((Vector3){ cx, wall_h * 0.5f, cz }, TILE_3D + wall_t - 0.006f, wall_h - 0.004f, wall_t - 0.006f, void_core);
            }
        }
    }

    for (int vc = min_c; vc <= max_c + 1; vc++) {
        float cx = (vc - 0.5f) * TILE_3D;
        for (int r = min_r; r <= max_r; r++) {
            if (!has_v_wall(maze, vc, r)) continue;
            float cz = r * TILE_3D;
            float dist = sqrtf((cx - cam_x) * (cx - cam_x) + (cz - cam_z) * (cz - cam_z));
            if (dist <= max_dist) {
                DrawCube((Vector3){ cx, wall_h * 0.5f, cz }, wall_t - 0.006f, wall_h - 0.004f, TILE_3D + wall_t - 0.006f, void_core);
            }
        }
    }

    for (int vr = min_r; vr <= max_r + 1; vr++) {
        float vz = (vr - 0.5f) * TILE_3D;
        for (int c = min_c; c <= max_c; c++) {
            if (!has_h_wall(maze, c, vr)) continue;

            float cx = c * TILE_3D;
            float dist = sqrtf((cx - cam_x) * (cx - cam_x) + (vz - cam_z) * (vz - cam_z));
            if (dist > max_dist) continue;

            Color col = apply_lidar_fog(wall_base, dist, max_dist);

            bool n_left  = has_v_wall(maze, c, vr - 1);
            bool s_left  = has_v_wall(maze, c, vr);
            bool w_left  = has_h_wall(maze, c - 1, vr);

            bool n_right = has_v_wall(maze, c + 1, vr - 1);
            bool s_right = has_v_wall(maze, c + 1, vr);
            bool e_right = has_h_wall(maze, c + 1, vr);

            float vx0 = (c - 0.5f) * TILE_3D;
            float vx1 = (c + 0.5f) * TILE_3D;

            float nx0 = vx0 + (n_left  ? half_t : (w_left  ? 0.0f : -half_t));
            float nx1 = vx1 - (n_right ? half_t : (e_right ? 0.0f : -half_t));
            draw_wall_segment_lines(nx0, vz - half_t, nx1, vz - half_t, wall_h, col);

            float sx0 = vx0 + (s_left  ? half_t : (w_left  ? 0.0f : -half_t));
            float sx1 = vx1 - (s_right ? half_t : (e_right ? 0.0f : -half_t));
            draw_wall_segment_lines(sx0, vz + half_t, sx1, vz + half_t, wall_h, col);

            if (!w_left && !n_left && !s_left) {
                draw_wall_segment_lines(vx0 - half_t, vz - half_t, vx0 - half_t, vz + half_t, wall_h, col);
            }
            if (!e_right && !n_right && !s_right) {
                draw_wall_segment_lines(vx1 + half_t, vz - half_t, vx1 + half_t, vz + half_t, wall_h, col);
            }
        }
    }

    for (int vc = min_c; vc <= max_c + 1; vc++) {
        float vx = (vc - 0.5f) * TILE_3D;
        for (int r = min_r; r <= max_r; r++) {
            if (!has_v_wall(maze, vc, r)) continue;

            float cz = r * TILE_3D;
            float dist = sqrtf((vx - cam_x) * (vx - cam_x) + (cz - cam_z) * (cz - cam_z));
            if (dist > max_dist) continue;

            Color col = apply_lidar_fog(wall_base, dist, max_dist);

            bool w_top = has_h_wall(maze, vc - 1, r);
            bool e_top = has_h_wall(maze, vc, r);
            bool n_top = has_v_wall(maze, vc, r - 1);

            bool w_bot = has_h_wall(maze, vc - 1, r + 1);
            bool e_bot = has_h_wall(maze, vc, r + 1);
            bool s_bot = has_v_wall(maze, vc, r + 1);

            float vz0 = (r - 0.5f) * TILE_3D;
            float vz1 = (r + 0.5f) * TILE_3D;

            float wz0 = vz0 + (w_top ? half_t : (n_top ? 0.0f : -half_t));
            float wz1 = vz1 - (w_bot ? half_t : (s_bot ? 0.0f : -half_t));
            draw_wall_segment_lines(vx - half_t, wz0, vx - half_t, wz1, wall_h, col);

            float ez0 = vz0 + (e_top ? half_t : (n_top ? 0.0f : -half_t));
            float ez1 = vz1 - (e_bot ? half_t : (s_bot ? 0.0f : -half_t));
            draw_wall_segment_lines(vx + half_t, ez0, vx + half_t, ez1, wall_h, col);

            if (!n_top && !w_top && !e_top) {
                draw_wall_segment_lines(vx - half_t, vz0 - half_t, vx + half_t, vz0 - half_t, wall_h, col);
            }
            if (!s_bot && !w_bot && !e_bot) {
                draw_wall_segment_lines(vx - half_t, vz1 + half_t, vx + half_t, vz1 + half_t, wall_h, col);
            }
        }
    }

    for (int vr = min_r; vr <= max_r + 1; vr++) {
        float vz = (vr - 0.5f) * TILE_3D;
        for (int vc = min_c; vc <= max_c + 1; vc++) {
            float vx = (vc - 0.5f) * TILE_3D;
            float dist = sqrtf((vx - cam_x) * (vx - cam_x) + (vz - cam_z) * (vz - cam_z));
            if (dist > max_dist) continue;

            bool n = has_v_wall(maze, vc, vr - 1);
            bool s = has_v_wall(maze, vc, vr);
            bool w = has_h_wall(maze, vc - 1, vr);
            bool e = has_h_wall(maze, vc, vr);

            if (!n && !s && !w && !e) continue;

            Color col = apply_lidar_fog(wall_base, dist, max_dist);

            if (n == w) DrawLine3D((Vector3){ vx - half_t, 0.002f, vz - half_t }, (Vector3){ vx - half_t, wall_h, vz - half_t }, col);
            if (n == e) DrawLine3D((Vector3){ vx + half_t, 0.002f, vz - half_t }, (Vector3){ vx + half_t, wall_h, vz - half_t }, col);
            if (s == e) DrawLine3D((Vector3){ vx + half_t, 0.002f, vz + half_t }, (Vector3){ vx + half_t, wall_h, vz + half_t }, col);
            if (s == w) DrawLine3D((Vector3){ vx - half_t, 0.002f, vz + half_t }, (Vector3){ vx - half_t, wall_h, vz + half_t }, col);
        }
    }
}

static void draw_maze_2d_explored(const Maze *maze, const Mouse *mouse, const SpeedrunPlan *plan, PathMode mode, int offset_x) {
    DrawRectangle(offset_x, 0, VIEW_SIZE, SCREEN_HEIGHT, (Color){ 15, 19, 25, 255 });

    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (mouse->visited[i][j]) continue;

            int x = offset_x + j * CELL_SIZE;
            int y = i * CELL_SIZE;
            int walls = maze_get_walls(maze, j, i);

            if (j >= maze->finish.x && j < maze->finish.x + 2 &&
                i >= maze->finish.y && i < maze->finish.y + 2) {
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, (Color){ 20, 75, 42, 255 });
            }

            Color wall_col = (Color){ 62, 72, 88, 255 };
            if (maze->cells[i][j].flags & FLAG_ZIGZAG) wall_col = (Color){ 115, 95, 30, 255 };
            else if (maze->cells[i][j].flags & FLAG_PASSAGE) wall_col = (Color){ 115, 42, 48, 255 };

            if (walls & WALL_NORTH) DrawLine(x, y, x + CELL_SIZE, y, wall_col);
            if (walls & WALL_EAST)  DrawLine(x + CELL_SIZE, y, x + CELL_SIZE, y + CELL_SIZE, wall_col);
            if (walls & WALL_SOUTH) DrawLine(x, y + CELL_SIZE, x + CELL_SIZE, y + CELL_SIZE, wall_col);
            if (walls & WALL_WEST)  DrawLine(x, y, x, y + CELL_SIZE, wall_col);
        }
    }

    int explored_count = 0;

    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (!mouse->visited[i][j]) continue;
            explored_count++;

            int x = offset_x + j * CELL_SIZE;
            int y = i * CELL_SIZE;
            int walls = maze_get_walls(maze, j, i);

            Color floor_col = (Color){ 28, 48, 72, 255 };
            if (i == 0 && j == 0) {
                floor_col = (Color){ 35, 100, 220, 255 };
            } else if (j >= maze->finish.x && j < maze->finish.x + 2 &&
                       i >= maze->finish.y && i < maze->finish.y + 2) {
                floor_col = (Color){ 35, 200, 90, 255 };
            }
            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, floor_col);

            Color wall_col = (Color){ 225, 232, 240, 255 };
            if (maze->cells[i][j].flags & FLAG_ZIGZAG) wall_col = GOLD;
            else if (maze->cells[i][j].flags & FLAG_PASSAGE) wall_col = RED;

            if (walls & WALL_NORTH) DrawLineEx((Vector2){ (float)x, (float)y }, (Vector2){ (float)(x + CELL_SIZE), (float)y }, 2.0f, wall_col);
            if (walls & WALL_EAST)  DrawLineEx((Vector2){ (float)(x + CELL_SIZE), (float)y }, (Vector2){ (float)(x + CELL_SIZE), (float)(y + CELL_SIZE) }, 2.0f, wall_col);
            if (walls & WALL_SOUTH) DrawLineEx((Vector2){ (float)x, (float)(y + CELL_SIZE) }, (Vector2){ (float)(x + CELL_SIZE), (float)(y + CELL_SIZE) }, 2.0f, wall_col);
            if (walls & WALL_WEST)  DrawLineEx((Vector2){ (float)x, (float)y }, (Vector2){ (float)x, (float)(y + CELL_SIZE) }, 2.0f, wall_col);
        }
    }

    if (mode == MODE_SPEEDRUN && plan->count > 1) {
        for (int i = 1; i < plan->count; i++) {
            Vector2 p0 = { offset_x + (plan->points[i - 1].x + 0.5f) * CELL_SIZE, (plan->points[i - 1].y + 0.5f) * CELL_SIZE };
            Vector2 p1 = { offset_x + (plan->points[i].x + 0.5f) * CELL_SIZE, (plan->points[i].y + 0.5f) * CELL_SIZE };
            DrawLineEx(p0, p1, 3.0f, plan->points[i].is_diagonal ? GOLD : SKYBLUE);
        }
    }

    float gx = (mode == MODE_SPEEDRUN) ? mouse->pos_x : (float)mouse->x;
    float gy = (mode == MODE_SPEEDRUN) ? mouse->pos_y : (float)mouse->y;
    int mx = offset_x + (int)(gx * CELL_SIZE);
    int my = (int)(gy * CELL_SIZE);
    DrawCircle(mx + CELL_SIZE / 2, my + CELL_SIZE / 2, CELL_SIZE / 3.0f, PURPLE);

    float rad = (mode == MODE_SPEEDRUN)
        ? (mouse->angle + 90.0f) * (PI / 180.0f)
        : mouse->orientation * (PI / 2.0f);
    float cx = mx + CELL_SIZE / 2.0f;
    float cy = my + CELL_SIZE / 2.0f;
    DrawLineEx((Vector2){ cx, cy }, (Vector2){ cx + sinf(rad) * (CELL_SIZE / 3.0f), cy - cosf(rad) * (CELL_SIZE / 3.0f) }, 3.0f, WHITE);

    int pct = (explored_count * 100) / (MAZE_SIZE * MAZE_SIZE);
    const char *stat = TextFormat("LEARNED: %i%%", pct);
    int stat_w = MeasureText(stat, 15);
    DrawRectangle(offset_x + VIEW_SIZE - stat_w - 24, 12, stat_w + 16, 26, (Color){ 10, 14, 20, 210 });
    DrawRectangleLines(offset_x + VIEW_SIZE - stat_w - 24, 12, stat_w + 16, 26, (Color){ 0, 210, 240, 200 });
    DrawText(stat, offset_x + VIEW_SIZE - stat_w - 16, 17, 15, WHITE);
}

static float compute_speedrun_step(const Mouse *mouse, const SpeedrunPlan *plan, float straight_turn_diff, float dt) {
    float turn_speed = 5.5f - straight_turn_diff * 2.0f;
    float straight_speed = 5.5f + straight_turn_diff * 7.5f;

    if (!plan || plan->current_idx <= 0 || plan->current_idx >= plan->count) {
        return turn_speed * dt;
    }

    Waypoint prev_wp = plan->points[plan->current_idx - 1];
    Waypoint curr_wp = plan->points[plan->current_idx];

    float seg_len = hypotf(curr_wp.x - prev_wp.x, curr_wp.y - prev_wp.y);
    if (curr_wp.is_diagonal || seg_len <= 1.05f) {
        return turn_speed * dt;
    }

    float dist_to_end = hypotf(curr_wp.x - mouse->pos_x, curr_wp.y - mouse->pos_y);
    float dist_from_start = fmaxf(0.0f, seg_len - dist_to_end);
    float ramp_dist = fminf(seg_len * 0.45f, 1.5f);
    float edge_dist = fminf(dist_from_start, dist_to_end);
    float factor = (ramp_dist > 0.001f) ? fminf(edge_dist / ramp_dist, 1.0f) : 1.0f;

    float current_speed = turn_speed + (straight_speed - turn_speed) * factor;
    return current_speed * dt;
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MicroMouse Maze Simulation");
    SetTargetFPS(60);

    Maze maze;
    Mouse mouse;
    SpeedrunPlan plan = { 0 };
    PathMode mode = MODE_EXPLORE;
    bool finished = false;
    float step_timer = 0.0f;

    RenderTexture2D fpvTarget = LoadRenderTexture(VIEW_SIZE, SCREEN_HEIGHT);

    AppScreen currentScreen = SCREEN_MENU;

    int btnWidth = 200;
    int btnHeight = 52;

    Rectangle startBtn = { SCREEN_WIDTH / 2.0f - btnWidth / 2.0f, SCREEN_HEIGHT / 2.0f - 30, btnWidth, btnHeight };
    Rectangle settingsBtn = { SCREEN_WIDTH / 2.0f - btnWidth / 2.0f, SCREEN_HEIGHT / 2.0f + 40, btnWidth, btnHeight };

    Rectangle backBtn = { SCREEN_WIDTH / 2.0f - btnWidth / 2.0f, SCREEN_HEIGHT - 95, btnWidth, btnHeight };
    Rectangle exploreSpeedSlider = { SCREEN_WIDTH / 2.0f - 140, 270, 280, 18 };
    Rectangle straightDiffSlider = { SCREEN_WIDTH / 2.0f - 140, 390, 280, 18 };

    Rectangle hudMenuBtn = { 14, 14, 88, 30 };

    float explore_speed = 0.5f;
    float straight_turn_diff = 0.5f;

    float cam_x = 0.0f;
    float cam_z = 0.0f;
    float cam_yaw = PI;

    Camera3D camera = { 0 };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 68.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mousePoint = GetMousePosition();

        if (currentScreen == SCREEN_MENU) {
            if (CheckCollisionPointRec(mousePoint, startBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                init_maze(&maze);
                mouse_init(&mouse);
                mouse_update_sensors(&mouse, &maze);
                memset(&plan, 0, sizeof(plan));
                mode = MODE_EXPLORE;
                finished = false;
                step_timer = 0.0f;

                cam_x = mouse.x * TILE_3D;
                cam_z = mouse.y * TILE_3D;
                cam_yaw = mouse.orientation * (PI / 2.0f);
                currentScreen = SCREEN_SIMULATION;
            }
            if (CheckCollisionPointRec(mousePoint, settingsBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                currentScreen = SCREEN_SETTINGS;
            }
        }
        else if (currentScreen == SCREEN_SETTINGS) {
            if (CheckCollisionPointRec(mousePoint, backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                currentScreen = SCREEN_MENU;
            }

            Rectangle exploreHitbox = { exploreSpeedSlider.x - 20, exploreSpeedSlider.y - 20, exploreSpeedSlider.width + 40, exploreSpeedSlider.height + 40 };
            if (CheckCollisionPointRec(mousePoint, exploreHitbox) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                explore_speed = (mousePoint.x - exploreSpeedSlider.x) / exploreSpeedSlider.width;
                if (explore_speed < 0.0f) explore_speed = 0.0f;
                if (explore_speed > 1.0f) explore_speed = 1.0f;
            }

            Rectangle diffHitbox = { straightDiffSlider.x - 20, straightDiffSlider.y - 20, straightDiffSlider.width + 40, straightDiffSlider.height + 40 };
            if (CheckCollisionPointRec(mousePoint, diffHitbox) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                straight_turn_diff = (mousePoint.x - straightDiffSlider.x) / straightDiffSlider.width;
                if (straight_turn_diff < 0.0f) straight_turn_diff = 0.0f;
                if (straight_turn_diff > 1.0f) straight_turn_diff = 1.0f;
            }
        }
        else if (currentScreen == SCREEN_SIMULATION) {
            if (CheckCollisionPointRec(mousePoint, hudMenuBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                currentScreen = SCREEN_MENU;
            }

            if (!finished) {
                if (mode == MODE_EXPLORE || mode == MODE_RETURN) {
                    step_timer += dt;
                    float step_interval = 0.28f - explore_speed * 0.23f;

                    if (step_timer >= step_interval) {
                        step_timer = 0.0f;
                        mouse_update_sensors(&mouse, &maze);

                        bool at_finish = (mouse.x >= maze.finish.x && mouse.x < maze.finish.x + 2 &&
                                          mouse.y >= maze.finish.y && mouse.y < maze.finish.y + 2);

                        if (mode == MODE_EXPLORE && at_finish) {
                            mode = MODE_RETURN;
                        } else if (mode == MODE_RETURN && mouse.x == 0 && mouse.y == 0) {
                            mouse.pos_x = 0.0f;
                            mouse.pos_y = 0.0f;
                            if (algorithm_build_speedrun_plan(&mouse, maze.finish, &plan)) {
                                mode = MODE_SPEEDRUN;
                            } else {
                                finished = true;
                            }
                        } else {
                            Point target = (mode == MODE_EXPLORE) ? maze.finish : (Point){ 0, 0 };
                            int next_dir = algorithm_get_next_move(&mouse, target);
                            if (next_dir >= 0 && next_dir < 4) {
                                mouse_move(&mouse, next_dir);
                                mouse_update_sensors(&mouse, &maze);
                            } else {
                                finished = true;
                            }
                        }
                    }
                }
                else if (mode == MODE_SPEEDRUN) {
                    float step_dist = compute_speedrun_step(&mouse, &plan, straight_turn_diff, dt);
                    if (algorithm_step_speedrun(&mouse, &plan, step_dist)) {
                        finished = true;
                    }
                }
            }

            float target_x = ((mode == MODE_SPEEDRUN) ? mouse.pos_x : (float)mouse.x) * TILE_3D;
            float target_z = ((mode == MODE_SPEEDRUN) ? mouse.pos_y : (float)mouse.y) * TILE_3D;
            float target_yaw = (mode == MODE_SPEEDRUN)
                ? (mouse.angle + 90.0f) * (PI / 180.0f)
                : mouse.orientation * (PI / 2.0f);

            cam_x += (target_x - cam_x) * 14.0f * dt;
            cam_z += (target_z - cam_z) * 14.0f * dt;

            float yaw_diff = target_yaw - cam_yaw;
            while (yaw_diff > PI) yaw_diff -= 2.0f * PI;
            while (yaw_diff < -PI) yaw_diff += 2.0f * PI;
            cam_yaw += yaw_diff * (12.0f + straight_turn_diff * 4.0f) * dt;

            camera.position = (Vector3){ cam_x, 0.36f, cam_z };
            camera.target = (Vector3){ cam_x + sinf(cam_yaw), 0.32f, cam_z - cosf(cam_yaw) };

            int cur_c = (mode == MODE_SPEEDRUN) ? (int)(mouse.pos_x + 0.5f) : mouse.x;
            int cur_r = (mode == MODE_SPEEDRUN) ? (int)(mouse.pos_y + 0.5f) : mouse.y;

            BeginTextureMode(fpvTarget);
                ClearBackground((Color){ 4, 7, 10, 255 });
                BeginMode3D(camera);
                    draw_maze_lidar_3d(&maze, cur_c, cur_r, cam_x, cam_z);
                EndMode3D();
            EndTextureMode();
        }

        BeginDrawing();

        if (currentScreen == SCREEN_MENU) {
            ClearBackground(BLACK);

            const char *title = "MICROMOUSE SIMULATION";
            int titleWidth = MeasureText(title, 38);
            DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2, SCREEN_HEIGHT / 3 - 10, 38, WHITE);

            Color startColor = CheckCollisionPointRec(mousePoint, startBtn) ? LIGHTGRAY : GRAY;
            DrawRectangleRec(startBtn, startColor);
            DrawRectangleLinesEx(startBtn, 3, WHITE);
            const char *startText = "START";
            DrawText(startText, startBtn.x + btnWidth / 2 - MeasureText(startText, 26) / 2, startBtn.y + 13, 26, BLACK);

            Color setColor = CheckCollisionPointRec(mousePoint, settingsBtn) ? LIGHTGRAY : GRAY;
            DrawRectangleRec(settingsBtn, setColor);
            DrawRectangleLinesEx(settingsBtn, 3, WHITE);
            const char *setText = "SETTINGS";
            DrawText(setText, settingsBtn.x + btnWidth / 2 - MeasureText(setText, 26) / 2, settingsBtn.y + 13, 26, BLACK);
        }
        else if (currentScreen == SCREEN_SETTINGS) {
            ClearBackground(BLACK);

            const char *title = "SETTINGS";
            DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 38) / 2, 110, 38, WHITE);

            DrawText("EXPLORE SPEED", exploreSpeedSlider.x, exploreSpeedSlider.y - 28, 18, LIGHTGRAY);
            DrawRectangleRec(exploreSpeedSlider, DARKGRAY);
            DrawRectangle(exploreSpeedSlider.x, exploreSpeedSlider.y, exploreSpeedSlider.width * explore_speed, exploreSpeedSlider.height, GREEN);
            DrawCircle(exploreSpeedSlider.x + exploreSpeedSlider.width * explore_speed, exploreSpeedSlider.y + exploreSpeedSlider.height / 2, 11, WHITE);
            DrawText(TextFormat("%i%%", (int)(explore_speed * 100)), exploreSpeedSlider.x + exploreSpeedSlider.width + 18, exploreSpeedSlider.y - 1, 18, WHITE);

            DrawText("STRAIGHT / TURN RATIO", straightDiffSlider.x, straightDiffSlider.y - 28, 18, LIGHTGRAY);
            DrawRectangleRec(straightDiffSlider, DARKGRAY);
            DrawRectangle(straightDiffSlider.x, straightDiffSlider.y, straightDiffSlider.width * straight_turn_diff, straightDiffSlider.height, ORANGE);
            DrawCircle(straightDiffSlider.x + straightDiffSlider.width * straight_turn_diff, straightDiffSlider.y + straightDiffSlider.height / 2, 11, WHITE);
            DrawText(TextFormat("%i%%", (int)(straight_turn_diff * 100)), straightDiffSlider.x + straightDiffSlider.width + 18, straightDiffSlider.y - 1, 18, WHITE);

            Color backColor = CheckCollisionPointRec(mousePoint, backBtn) ? LIGHTGRAY : GRAY;
            DrawRectangleRec(backBtn, backColor);
            DrawRectangleLinesEx(backBtn, 3, WHITE);
            const char *backText = "BACK";
            DrawText(backText, backBtn.x + btnWidth / 2 - MeasureText(backText, 26) / 2, backBtn.y + 13, 26, BLACK);
        }
        else if (currentScreen == SCREEN_SIMULATION) {
            ClearBackground((Color){ 4, 7, 10, 255 });

            DrawTextureRec(fpvTarget.texture, (Rectangle){ 0.0f, 0.0f, (float)fpvTarget.texture.width, -(float)fpvTarget.texture.height }, (Vector2){ 0.0f, 0.0f }, WHITE);

            draw_maze_2d_explored(&maze, &mouse, &plan, mode, VIEW_SIZE);

            DrawLineEx((Vector2){ (float)VIEW_SIZE, 0.0f }, (Vector2){ (float)VIEW_SIZE, (float)SCREEN_HEIGHT }, 2.0f, (Color){ 0, 210, 240, 210 });

            Color menuHudCol = CheckCollisionPointRec(mousePoint, hudMenuBtn) ? (Color){ 20, 48, 60, 235 } : (Color){ 8, 16, 24, 220 };
            DrawRectangleRec(hudMenuBtn, menuHudCol);
            DrawRectangleLinesEx(hudMenuBtn, 2, (Color){ 0, 210, 240, 220 });
            DrawText("MENU", hudMenuBtn.x + hudMenuBtn.width / 2 - MeasureText("MENU", 15) / 2, hudMenuBtn.y + 8, 15, (Color){ 180, 245, 255, 255 });

            const char *mode_label = finished ? "FINISHED"
                : (mode == MODE_EXPLORE ? "EXPLORE"
                : (mode == MODE_RETURN ? "RETURN" : "SPEEDRUN"));
            DrawText(mode_label, 114, 21, 16, (Color){ 0, 235, 255, 255 });
        }
        EndDrawing();
    }

    UnloadRenderTexture(fpvTarget);
    CloseWindow();
    return 0;
}