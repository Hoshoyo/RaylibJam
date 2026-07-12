#include "game.h"
#include "renderer/renderer.h"
#include "renderer/assets.h"

#include "ui.h"
#include "item.h"
#include "font.h"

static Game game;
static bool ui_hovered_or_active;
static Vector2 camera_position;

void game_init()
{
    game.camera.zoom = 0.5f;
    game.camera.target = (Vector2){-360.0f, -360.0f};
    game.stored_energy   = 0.0f;
    game.needed_energy   = 0.0f;
    game.research_points = 0.0f;
    game.city_size       = 1;
    game.day             = 1;

    font_init();
    ui_init();
    load_assets();
    item_render_init(items_sprite.atlas.texture,
                     rock_icons_recs, ARRAY_LENGTH(rock_icons_recs),
                     merged_icons_recs, ARRAY_LENGTH(merged_icons_recs));
}

void game_update()
{
    // Camera panning control
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !ui_hovered_or_active)
    {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / game.camera.zoom);
        camera_position = Vector2Add(delta, camera_position);
    }

    // Zoom control
    float wheel = GetMouseWheelMove();
    if(wheel > 0 && game.camera.zoom < 2.0f)      game.camera.zoom *= 2.0f;
    else if(wheel < 0 && game.camera.zoom >= 0.5f) game.camera.zoom /= 2.0f;

    // Recenter camera
    Vector2 center = {
        -GetScreenWidth() / 2.0f,
        -GetScreenHeight() / 2.0f
    };
    center = Vector2Scale(center, 1.0f / game.camera.zoom);
    game.camera.target = Vector2Add(center, camera_position);
    game.camera.target.x = (float)round(game.camera.target.x);
    game.camera.target.y = (float)round(game.camera.target.y);
}

//void debug_slider(int id, float* value, float min, float max)
//{
//    if(id < 0 || id > 256) return;
//
//    static bool debug_sliders[256];
//    HoUiInteraction interaction = ho_slider_circle((Vector2){10.0f, 10.0f + id * 50.0f}, debug_sliders[id], (Vector2){180, 2}, value, min, max);
//    if(interaction & HOUI_INTERACT_CLICKED)
//        debug_sliders[id] = true;
//    if(interaction & HOUI_INTERACT_RELEASED)
//        debug_sliders[id] = false;
//    *value = roundf(*value);
//    DrawText(TextFormat("%.2f", *value), 195, id * 50, 20, WHITE);
//
//    ui_hovered_or_active = (interaction & HOUI_INTERACT_HOVERED) != 0 || debug_sliders[id];
//}

Vector2 iso_pos(Vector2 ortho)
{
    return (Vector2){(ortho.x + ortho.y), (ortho.x - ortho.y)};
}

void render_map()
{
    #define GRID_SIZE 180
    #if 1
    SetRandomSeed(456);  // deterministic map geometry every frame
    int j = 0;
    for(int y = 8; y > -8; --y)
    {
        for(int x = -8; x < 8; ++x)
        {
            Vector2 position = (Vector2){x * GRID_SIZE, y * GRID_SIZE};
            position = iso_pos(position);
            int random_building = GetRandomValue(0, pole_sprite.rect_count - 1);
            int random_pole = GetRandomValue(0, pole_sprite.rect_count - 1);

            //if(GetRandomValue(0, 100) < 20)
            {
                render_sprite_static_atlas_offset(&buildings_shadow.atlas, building_recs[random_building], building_offsets[random_building], position, 0, WHITE);
                render_sprite_static_atlas_offset(&buildings_albedo.atlas, building_recs[random_building], building_offsets[random_building], position, 0, WHITE);

                Vector2 pole_position = Vector2Add(position, (Vector2){30, 50});
                render_sprite_static_atlas_offset(&pole_sprite.atlas, pole_sprite.recs[random_pole], pole_offsets[random_pole], pole_position, 0, WHITE);
            }
            
            ///DrawRectangleV(position, (Vector2){200, 200}, RED);
            //DrawRectanglePro((Rectangle){position.x - 200, position.y - 200, 50, 1000}, (Vector2){0}, 45, BLACK);
        }
    }
    #endif
}



void game_render()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    {
        BeginMode2D(game.camera);
    
        render_queue_flush();
        render_map();
        SetRandomSeed((unsigned int)(GetTime() * 100000.0));  // restore varying seed after map

        EndMode2D();

        static int xx = 0;
        if(IsKeyPressed('X'))
        {
            xx = (xx + 1) % ARRAY_LENGTH(building_offsets);
        }
        DrawText(TextFormat("%d", xx), 0, GetScreenHeight() - 40.0f, 20, WHITE);
        //debug_slider(0, &building_offsets[xx].x, -200.0f, 200.0f);
        //debug_slider(1, &building_offsets[xx].y, -200.0f, 200.0f);

        ui_hovered_or_active = ui_render(&game);
    }

    EndDrawing();

    game.tick++;
}