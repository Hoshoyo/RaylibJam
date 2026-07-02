#include "game.h"
#include "renderer/renderer.h"
#include "renderer/assets.h"

static Game game;

void game_init()
{
    game.camera.zoom = 1.0f;

    load_assets();
}

void game_update()
{
}

void game_render()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    {
        BeginMode2D(game.camera);
    
        render_sprite_static_atlas(&grass.atlas, grass_recs[0], (Vector2){80, 210}, 0, WHITE);

        render_sprite_animated(&character[0].shadow, (Vector2){100, 200}, 0, game.tick % sprite_frame_count(&character[0].shadow), WHITE);
        render_sprite_animated(&character[0].albedo, (Vector2){100, 200}, 1, game.tick % sprite_frame_count(&character[0].albedo), WHITE);

        render_queue_flush();

        EndMode2D();
    }

    EndDrawing();

    game.tick++;
}
