#include <raylib.h>
#include <stdint.h>

typedef struct {
    uint64_t tick;
    Camera2D camera;
} Game;

void LoadSave(void);
void SaveGame(const char* data);

void game_init();
void game_update();
void game_render();