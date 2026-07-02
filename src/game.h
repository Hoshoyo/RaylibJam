#include <raylib.h>

typedef struct {
    Camera camera;
    Model cube;
} Game;

void LoadSave(void);
void SaveGame(const char* data);

void game_init();
void game_update();
void game_render();