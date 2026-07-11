#include "game.h"
#include "renderer/renderer.h"
#include "renderer/assets.h"

#include "ui.h"

static Game game;

void game_init()
{
    game.camera.zoom = 0.5f;
    game.camera.target = (Vector2){360.0f, 360.0f};

    ui_init();
    load_assets();
}

void game_update()
{
    // Camera panning control
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / game.camera.zoom);
        game.camera.target = Vector2Add(game.camera.target, delta);
    }

    // Zoom control
    float wheel = GetMouseWheelMove();
    if(wheel > 0 && game.camera.zoom < 2.0f)      game.camera.zoom *= 2.0f;
    else if(wheel < 0 && game.camera.zoom >= 0.5f) game.camera.zoom /= 2.0f;
}

void game_render()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    {
        BeginMode2D(game.camera);
    
        render_queue_flush();

        //render_sprite_static_atlas(&grass.atlas, grass_recs[0], (Vector2){80, 210}, 0, WHITE);

        //for(int j = 0; j < 5; ++j)
        //{
        //    for(int i = 0; i < 6; ++i)
        //    {
        //        render_sprite_static_atlas(&buildings_shadow.atlas, building_recs[i], (Vector2){720 - i * 120 + j * 200, 0 + i * 110 + j * 200}, 0, WHITE);
        //        render_sprite_static_atlas(&buildings_albedo.atlas, building_recs[i], (Vector2){720 - i * 120 + j * 200, 0 + i * 110 + j * 200}, 0, WHITE);
        //    }
        //}

        //render_sprite_static_atlas(&pole_sprite.atlas, pole_sprite.recs[0], (Vector2){20, 90}, 0, WHITE);

        //render_sprite_animated(&character[0].shadow, (Vector2){100, 200}, 0, game.tick % sprite_frame_count(&character[0].shadow), WHITE);
        //render_sprite_animated(&character[0].albedo, (Vector2){100, 200}, 1, game.tick % sprite_frame_count(&character[0].albedo), WHITE);

        //DrawCircle(360, 360, 3.0f, RED);

        EndMode2D();

        ui_render();
    }

    EndDrawing();

    game.tick++;
}