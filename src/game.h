#pragma once
#include <raylib.h>
#include <stdint.h>

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

typedef struct {
    uint64_t tick;
    Camera2D camera;
} Game;

void LoadSave(void);
void SaveGame(const char* data);

void game_init();
void game_update();
void game_render();