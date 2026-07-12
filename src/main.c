#include "raylib.h"
#include <stdlib.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#else
    #define SAVE_PATH "save.dat"
#endif

#include "game.h"

void LoadSave(void)
{
#if defined(PLATFORM_WEB)
    char* text = (char*)EM_ASM_PTR(
        var s = localStorage.getItem('save');
        if (!s) return 0;
        var len = lengthBytesUTF8(s) + 1;
        var buf = _malloc(len);
        stringToUTF8(s, buf, len);
        return buf;
    );
    if (text) {
        TraceLog(LOG_INFO, "Loaded save data! %s", text);
        free(text);
    } else {
        TraceLog(LOG_INFO, "No save data found!");
    }
#else
    char* text = LoadFileText(SAVE_PATH);
    if (text) {
        TraceLog(LOG_INFO, "Loaded save data! %s", text);
        UnloadFileText(text);
    } else {
        TraceLog(LOG_INFO, "No save data found!");
    }
#endif
}

void SaveGame(const char* data)
{
#if defined(PLATFORM_WEB)
    EM_ASM({ localStorage.setItem('save', UTF8ToString($0)); }, data);
    TraceLog(LOG_INFO, "Saved save data!");
#else
    if (SaveFileText(SAVE_PATH, data))
        TraceLog(LOG_INFO, "Saved save data!");
#endif
}

void UpdateDrawFrame(void);

int main(void)
{
    InitWindow(720, 720, "Raylib 6.x Jam");

    game_init();
    
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();
    return 0;
}

void UpdateDrawFrame(void)
{
    game_update();
    game_render();
}