#pragma once
#include <raylib.h>
#include <stdint.h>

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

#define CITY_GRID 16
#define DAYS_UNTIL_EVICTION 2  // days without energy before a building is removed

typedef struct {
    bool  filled;
    float needed_energy;
    int   render_ref;
    int   days_without_energy;
    int   people_living;        // set once at city_init, randomised
    char  address[32];          // e.g. "125 Cedar Street" — set once at city_init
} City_Building;

typedef struct {
    uint64_t tick;
    Camera2D camera;
    float stored_energy;
    float needed_energy;
    float research_points;
    int   city_size;
    int   day;
    City_Building city[CITY_GRID][CITY_GRID];
} Game;

void LoadSave(void);
void SaveGame(const char* data);

void city_init(int building_count);
void resize_city(int new_size);
void game_next_day(bool allow_growth);
bool is_everyone_happy_now(void);

// Shared fill order: s_fill_order[0] = innermost building slot, [N-1] = outermost.
extern int s_fill_order[CITY_GRID * CITY_GRID];
// Center-out (same order resize_city uses when adding buildings).
static inline int city_fill_order(int i)  { return s_fill_order[i]; }
// Outer-in (reverse: prefer evicting buildings added latest).
static inline int city_evict_order(int i) { return s_fill_order[CITY_GRID * CITY_GRID - 1 - i]; }

void game_init();
void game_update();
void game_render();

typedef struct {
    float master_volume;
    Sound city[2];
    Sound birds;
    Sound discharge[2];
    Sound click;
} SoundFxs;

void play_random_pitch(Sound sound, float range);