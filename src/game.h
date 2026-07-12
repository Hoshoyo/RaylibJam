#pragma once
#include <raylib.h>
#include <stdint.h>

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

typedef struct {
    uint64_t tick;
    Camera2D camera;
    float stored_energy;
    float needed_energy;
    float research_points;
    int   city_size;
    int   day;
} Game;

void LoadSave(void);
void SaveGame(const char* data);

void game_init();
void game_update();
void game_render();