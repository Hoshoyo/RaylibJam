#pragma once
#include <raylib.h>
#include <stdint.h>

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

#define CITY_GRID 16

typedef struct {
    bool  filled;
    float needed_energy;
    int   render_ref;
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
void game_next_day(void);

void game_init();
void game_update();
void game_render();