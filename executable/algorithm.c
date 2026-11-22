#include "../libraries/algorithm.h"
#include <math.h>
#include <stdint.h>
#include <limits.h>

// 0 = Північ, 1 = Схід, 2 = Південь, 3 = Захід
static const int DX[4] = { 0, 1, 0, -1 };
static const int DY[4] = { -1, 0, 1, 0 };

static const uint8_t DIR_WALLS[4] = { WALL_N, WALL_E, WALL_S, WALL_W };

#define TIME_DIAG_CONT   71   // Продовження діагоналі 45° (чергування Left <-> Right)
#define TIME_DIAG_ENTER  95   // Вхід у діагональ 45° з прямої ділянки
#define TIME_STRAIGHT    100  // Рух прямо без зміни напрямку
#define TIME_DIAG_EXIT   120  // Вихід із діагоналі 45° на пряму
#define TIME_SHARP_90    165  // Два повороти в один бік поспіль (гострий кут 90°)
#define TIME_UTURN       300  // Розворот

// Типи маневрів
#define TURN_NONE   0  // Прямо
#define TURN_LEFT   1  // Ліворуч
#define TURN_RIGHT  2  // Праворуч
#define TURN_UTURN  3  // Розворот

typedef struct {
    Point data[MAZE_SIZE * MAZE_SIZE];
    int head;
    int tail;
} Queue;

static void queue_init(Queue *q) {
    q->head = 0;
    q->tail = 0;
}

static void queue_push(Queue *q, Point p) {
    q->data[q->tail++] = p;
}

static Point queue_pop(Queue *q) {
    return q->data[q->head++];
}

static bool queue_is_empty(const Queue *q) {
    return q->head == q->tail;
}

typedef struct {
    int time;     // Накопичена вартість шляху до цього стану
    uint8_t x;
    uint8_t y;
    uint8_t dir;  // Напрямок, у якому миша в'їхала в клітинку (0..3)
    uint8_t turn; // Який поворот вона зробила перед в'їздом (TURN_NONE/LEFT/RIGHT/UTURN)
} HeapNode;

// Максимальна кількість станів: 32 * 32 клітинки * 4 напрямки * 4 типи повороту
#define MAX_STATES (MAZE_SIZE * MAZE_SIZE * 4 * 4)

typedef struct {
    HeapNode nodes[MAX_STATES]; // Масив-піраміда, де nodes[0] завжди має мінімальний time
    int size;                   // Поточна кількість активних вузлів у хіпі
} MinHeap;

static void heap_init(MinHeap *h) {
    h->size = 0;
}

static void heap_push(MinHeap *h, HeapNode node) {
    if (h->size >= MAX_STATES) return;

    int i = h->size++;

    while (i > 0) {
        int parent = (i - 1) / 2;

        if (h->nodes[parent].time <= node.time) break;

        h->nodes[i] = h->nodes[parent];
        i = parent;
    }
    h->nodes[i] = node;
}

static HeapNode heap_pop(MinHeap *h) {
    HeapNode top = h->nodes[0];
    HeapNode last = h->nodes[--h->size];

    int i = 0;
    while (1) {
        int left = 2 * i + 1;
        if (left >= h->size) break;

        int smallest = left;
        int right = left + 1;
        if (right < h->size && h->nodes[right].time < h->nodes[left].time) {
            smallest = right;
        }

        if (last.time <= h->nodes[smallest].time) break;

        h->nodes[i] = h->nodes[smallest];
        i = smallest;
    }

    h->nodes[i] = last;
    return top;
}

static inline bool in_bounds(int x, int y) {
    return x >= 0 && x < MAZE_SIZE && y >= 0 && y < MAZE_SIZE;
}

static int get_turn_type(int prev_dir, int new_dir) {
    int diff = (new_dir - prev_dir + 4) % 4;
    if (diff == 0) return TURN_NONE;
    if (diff == 1) return TURN_RIGHT;
    if (diff == 3) return TURN_LEFT;
    return TURN_UTURN;
}

static int get_step_time(int prev_turn, int curr_turn) {
    if (curr_turn == TURN_UTURN) {
        return TIME_UTURN;
    }

    if (curr_turn == TURN_NONE) {
        if (prev_turn == TURN_LEFT || prev_turn == TURN_RIGHT) {
            return TIME_DIAG_EXIT;
        }
        return TIME_STRAIGHT;
    }

    if (prev_turn == TURN_NONE || prev_turn == TURN_UTURN) {
        return TIME_DIAG_ENTER;
    }
    if (prev_turn == curr_turn) {
        return TIME_SHARP_90;
    }
    return TIME_DIAG_CONT;
}

static float vector_to_angle_deg(float dx, float dy) {
    return atan2f(dy, dx) * (180.0f / 3.14159265f);
}

// ============================================================================
// БЛОК 3: ПУБЛІЧНІ ФУНКЦІЇ АЛГОРИТМУ
// ============================================================================

void algorithm_compute_distances(const Mouse *mouse, Point target, int dist[MAZE_SIZE][MAZE_SIZE]) {
    for (int y = 0; y < MAZE_SIZE; y++) {
        for (int x = 0; x < MAZE_SIZE; x++) {
            dist[y][x] = -1;
        }
    }

    Queue q;
    queue_init(&q);
    dist[target.y][target.x] = 0;
    queue_push(&q, target);

    while (!queue_is_empty(&q)) {
        Point curr = queue_pop(&q);
        int next_dist = dist[curr.y][curr.x] + 1;

        for (int d = 0; d < 4; d++) {
            if (mouse->known_walls[curr.y][curr.x] & DIR_WALLS[d]) continue;
            int nx = curr.x + DX[d];
            int ny = curr.y + DY[d];
            if (!in_bounds(nx, ny) || dist[ny][nx] != -1) continue;

            dist[ny][nx] = next_dist;
            queue_push(&q, (Point){ nx, ny });
        }
    }
}

int algorithm_get_next_move(const Mouse *mouse, Point target) {
    int mx = mouse->x;
    int my = mouse->y;
    if (mx == target.x && my == target.y) return -1;

    int dist[MAZE_SIZE][MAZE_SIZE];
    algorithm_compute_distances(mouse, target, dist);

    int chosen_dir = -1;
    int best_dist = INT_MAX;
    bool best_unvisited = false;
    int best_diag = INT_MAX;
    bool best_straight = false;

    for (int d = 0; d < 4; d++) {
        if (mouse->known_walls[my][mx] & DIR_WALLS[d]) continue;
        int nx = mx + DX[d];
        int ny = my + DY[d];
        if (!in_bounds(nx, ny) || dist[ny][nx] == -1) continue;

        int candidate_dist = dist[ny][nx];
        bool candidate_unvisited = !mouse->visited[ny][nx];
        int dx_t = (nx > target.x) ? (nx - target.x) : (target.x - nx);
        int dy_t = (ny > target.y) ? (ny - target.y) : (target.y - ny);
        int candidate_diag = (dx_t > dy_t) ? dx_t : dy_t;
        bool candidate_straight = (d == mouse->orientation);

        if (candidate_dist < best_dist) {
            best_dist = candidate_dist;
            best_unvisited = candidate_unvisited;
            best_diag = candidate_diag;
            best_straight = candidate_straight;
            chosen_dir = d;
        } else if (candidate_dist == best_dist) {
            if (candidate_unvisited && !best_unvisited) {
                best_unvisited = true;
                best_diag = candidate_diag;
                best_straight = candidate_straight;
                chosen_dir = d;
            } else if (candidate_unvisited == best_unvisited && candidate_diag < best_diag) {
                best_diag = candidate_diag;
                best_straight = candidate_straight;
                chosen_dir = d;
            } else if (candidate_unvisited == best_unvisited && candidate_diag == best_diag && candidate_straight && !best_straight) {
                best_straight = true;
                chosen_dir = d;
            }
        }
    }
    return chosen_dir;
}

bool algorithm_build_speedrun_plan(const Mouse *mouse, Point target, SpeedrunPlan *plan) {
    if (!mouse || !plan) return false;

    plan->count = 0;
    plan->current_idx = 0;

    if (!in_bounds(target.x, target.y)) return false;
    if (!mouse->visited[target.y][target.x]) return false;

    static int time[MAZE_SIZE][MAZE_SIZE][4][4];
    static uint16_t parent[MAZE_SIZE][MAZE_SIZE][4][4];
    static MinHeap heap;

    for (int y = 0; y < MAZE_SIZE; y++) {
        for (int x = 0; x < MAZE_SIZE; x++) {
            for (int d = 0; d < 4; d++) {
                for (int t = 0; t < 4; t++) {
                    time[y][x][d][t] = INT_MAX;
                    parent[y][x][d][t] = 0xFFFF;
                }
            }
        }
    }

    heap_init(&heap);

    int sx = mouse->x;
    int sy = mouse->y;
    int sdir = mouse->orientation;
    int sturn = TURN_NONE;

    time[sy][sx][sdir][sturn] = 0;
    HeapNode start_node = { .x = (uint8_t)sx, .y = (uint8_t)sy, .dir = (uint8_t)sdir, .turn = (uint8_t)sturn, .time = 0 };
    heap_push(&heap, start_node);

    const int dx[4] = { 0, 1, 0, -1 };
    const int dy[4] = { -1, 0, 1, 0 };

    while (heap.size > 0) {
        HeapNode curr = heap_pop(&heap);

        if (curr.time > time[curr.y][curr.x][curr.dir][curr.turn]) continue;

        for (int ndir = 0; ndir < 4; ndir++) {
            if (mouse->known_walls[curr.y][curr.x] & (1 << ndir)) continue;

            int nx = curr.x + dx[ndir];
            int ny = curr.y + dy[ndir];

            if (!in_bounds(nx, ny)) continue;
            if (!mouse->visited[ny][nx]) continue;

            int turn = get_turn_type(curr.dir, ndir);
            int new_time = curr.time + get_step_time(curr.turn, turn);

            if (new_time < time[ny][nx][ndir][turn]) {
                time[ny][nx][ndir][turn] = new_time;
                parent[ny][nx][ndir][turn] = (uint16_t)((curr.y << 10) | (curr.x << 5) | (curr.dir << 2) | curr.turn);
                HeapNode next_node = { .x = (uint8_t)nx, .y = (uint8_t)ny, .dir = (uint8_t)ndir, .turn = (uint8_t)turn, .time = new_time };
                heap_push(&heap, next_node);
            }
        }
    }

    int best_time = INT_MAX;
    int best_x = -1;
    int best_y = -1;
    int best_d = -1;
    int best_t = -1;

    for (int fy = target.y; fy <= target.y + 1; fy++) {
        for (int fx = target.x; fx <= target.x + 1; fx++) {
            if (!in_bounds(fx, fy) || !mouse->visited[fy][fx]) continue;
            for (int d = 0; d < 4; d++) {
                for (int t = 0; t < 4; t++) {
                    if (time[fy][fx][d][t] < best_time) {
                        best_time = time[fy][fx][d][t];
                        best_x = fx;
                        best_y = fy;
                        best_d = d;
                        best_t = t;
                    }
                }
            }
        }
    }

    if (best_time == INT_MAX) return false;

    static Point cells[1024];
    int cell_count = 0;
    int cy = best_y;
    int cx = best_x;
    int cd = best_d;
    int ct = best_t;

    while (cell_count < 1024) {
        cells[cell_count++] = (Point){ cx, cy };
        uint16_t p = parent[cy][cx][cd][ct];
        if (p == 0xFFFF) break;
        cy = (p >> 10) & 0x1F;
        cx = (p >> 5) & 0x1F;
        cd = (p >> 2) & 0x03;
        ct = p & 0x03;
    }

    for (int i = 0; i < cell_count / 2; i++) {
        Point tmp = cells[i];
        cells[i] = cells[cell_count - 1 - i];
        cells[cell_count - 1 - i] = tmp;
    }

    static float raw_x[1025];
    static float raw_y[1025];
    int raw_count = 0;

    raw_x[raw_count] = (float)cells[0].x;
    raw_y[raw_count] = (float)cells[0].y;
    raw_count++;

    for (int i = 0; i < cell_count - 1; i++) {
        raw_x[raw_count] = (cells[i].x + cells[i + 1].x) * 0.5f;
        raw_y[raw_count] = (cells[i].y + cells[i + 1].y) * 0.5f;
        raw_count++;
    }

    if (cell_count > 1) {
        raw_x[raw_count] = (float)cells[cell_count - 1].x;
        raw_y[raw_count] = (float)cells[cell_count - 1].y;
        raw_count++;
    }

    int max_points = (int)(sizeof(plan->points) / sizeof(plan->points[0]));

    if (raw_count == 1) {
        if (max_points > 0) {
            plan->points[0] = (Waypoint){ raw_x[0], raw_y[0], mouse->angle, false };
            plan->count = 1;
        }
        return plan->count > 0;
    }

    if (max_points > 0) {
        float first_angle = vector_to_angle_deg(raw_x[1] - raw_x[0], raw_y[1] - raw_y[0]);
        plan->points[0] = (Waypoint){ raw_x[0], raw_y[0], first_angle, false };
        plan->count = 1;
        plan->current_idx = 1;
    }

    for (int i = 1; i < raw_count; i++) {
        float seg_dx = raw_x[i] - raw_x[i - 1];
        float seg_dy = raw_y[i] - raw_y[i - 1];
        float angle = vector_to_angle_deg(seg_dx, seg_dy);
        bool is_diag = (fabsf(seg_dx) > 0.01f && fabsf(seg_dy) > 0.01f);

        float angle_diff = angle - plan->points[plan->count - 1].angle;
        while (angle_diff > 180.0f) angle_diff -= 360.0f;
        while (angle_diff < -180.0f) angle_diff += 360.0f;

        if (plan->count > 1 && fabsf(angle_diff) < 0.1f) {
            plan->points[plan->count - 1].x = raw_x[i];
            plan->points[plan->count - 1].y = raw_y[i];
            // Суцільний зігзаг під 45° об'єднується в одну пряму, тому позначаємо як пряму ділянку
            plan->points[plan->count - 1].is_diagonal = false;
        } else if (plan->count < max_points) {
            plan->points[plan->count++] = (Waypoint){ raw_x[i], raw_y[i], angle, is_diag };
        }
    }
    return plan->count > 0;
}

bool algorithm_step_speedrun(Mouse *mouse, SpeedrunPlan *plan, float step_distance) {
    if (plan->current_idx >= plan->count) return true;

    Waypoint target_wp = plan->points[plan->current_idx];
    float dx = target_wp.x - mouse->pos_x;
    float dy = target_wp.y - mouse->pos_y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (plan->current_idx > 0) {
        Waypoint prev_wp = plan->points[plan->current_idx - 1];
        float seg_len = hypotf(target_wp.x - prev_wp.x, target_wp.y - prev_wp.y);

        // Розганяємось як на ортогональних прямих, так і на прямих зігзагах під 45° (seg_len > 1.0f)
        if (seg_len > 1.0f) {
            float dist_done = fmaxf(0.0f, seg_len - dist);
            float max_boost = 1.0f + fminf((seg_len - 1.0f) * 0.35f, 1.8f);
            float ramp_len = fminf(seg_len * 0.35f, 2.0f);
            float ramp = fminf(fminf(dist_done, dist) / ramp_len, 1.0f);
            step_distance *= 1.0f + (max_boost - 1.0f) * ramp;
        }
    }

    float diff = target_wp.angle - mouse->angle;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    float max_turn = 15.0f + step_distance * 200.0f;
    mouse->angle += (fabsf(diff) <= max_turn) ? diff : (diff > 0.0f ? max_turn : -max_turn);

    if (dist <= step_distance) {
        mouse->pos_x = target_wp.x;
        mouse->pos_y = target_wp.y;
        plan->current_idx++;
    } else {
        mouse->pos_x += (dx / dist) * step_distance;
        mouse->pos_y += (dy / dist) * step_distance;
    }

    mouse->x = (int)(mouse->pos_x + 0.5f);
    mouse->y = (int)(mouse->pos_y + 0.5f);
    mouse->orientation = (((int)roundf((mouse->angle + 90.0f) / 90.0f)) % 4 + 4) % 4;

    return plan->current_idx >= plan->count;
}