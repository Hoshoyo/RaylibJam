#include "game.h"
#include "renderer/renderer.h"
#include "renderer/assets.h"

#include "ui/hui.h"
static ColorPalette ui_palette;
#include "ui/ho_button.c"
#include "ui/ho_label.c"
#include "ui/ho_slider.c"

static Game game;
static bool ui_hovered_or_active;
static Vector2 camera_position;

void game_init()
{
    game.camera.zoom = 0.5f;
    game.camera.target = (Vector2){-360.0f, -360.0f};

    ui_palette = palette_mountain_ridge;

    load_assets();
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

void debug_slider(int id, float* value, float min, float max)
{
    if(id < 0 || id > 256) return;

    static bool debug_sliders[256];
    HoUiInteraction interaction = ho_slider_circle((Vector2){10.0f, 10.0f + id * 50.0f}, debug_sliders[id], (Vector2){180, 2}, value, min, max);
    if(interaction & HOUI_INTERACT_CLICKED)
        debug_sliders[id] = true;
    if(interaction & HOUI_INTERACT_RELEASED)
        debug_sliders[id] = false;
    *value = roundf(*value);
    DrawText(TextFormat("%.2f", *value), 195, id * 50, 20, WHITE);

    ui_hovered_or_active = (interaction & HOUI_INTERACT_HOVERED) != 0 || debug_sliders[id];
}

void render_ui()
{
    DrawRectangle(360, 360, 200, 200, BROWN);
    // Button
    if(ho_button((Vector2){360.0f,360.0f}, (Vector2){100, 40}, GetFontDefault(), "Button") & HOUI_INTERACT_CLICKED)
    {
        TraceLog(LOG_INFO, "Clicked!");
    }

    // Label
    ho_label((Vector2){360.0f,360.0f + 50.0f}, (Vector2){100, 40}, GetFontDefault(), "Label");

    // Slider
    static bool slider_active = false;
    static float value = 0.0f;
    HoUiInteraction interaction = ho_slider_circle((Vector2){360.0f,360.0f + 2*50.0f}, slider_active, (Vector2){200, 2}, &value, 0, 100.0f);
    if(interaction & HOUI_INTERACT_CLICKED)
        slider_active = true;
    if(interaction & HOUI_INTERACT_RELEASED)
        slider_active = false;

    ui_hovered_or_active = (interaction & HOUI_INTERACT_HOVERED) != 0 || slider_active;
}

Vector2 iso_pos(Vector2 ortho)
{
    return (Vector2){(ortho.x + ortho.y), (ortho.x - ortho.y)};
}

void render_map()
{
    #define GRID_SIZE 180
    #if 1
    SetRandomSeed(456);
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

void
render_items()
{
    // Render special items
    Vector2 position = (Vector2){0, 100};
    for(int i = 0; i < ARRAY_LENGTH(merged_icons_recs); ++i)
    {
        // Change scale if needed
        items_sprite.atlas.scale = (Vector2){0.5f, 0.5f};

        // Render atlas, each item is on merged_icons_recs array of rectangles
        render_sprite_static_atlas_immediate(&items_sprite.atlas, merged_icons_recs[i], position, 0, WHITE);

        // Advance by the size of the item
        position.x += merged_icons_recs[i].width * items_sprite.atlas.scale.x;
    }

    position.x = 0;
    position.y = 160;
    // Render basic rocks
    for(int i = 0; i < ARRAY_LENGTH(rock_icons_recs); ++i)
    {
        // Change scale if needed
        items_sprite.atlas.scale = (Vector2){0.5f, 0.5f};

        // Render atlas, each item is on merged_icons_recs array of rectangles
        render_sprite_static_atlas_immediate(&items_sprite.atlas, rock_icons_recs[i], position, 0, WHITE);

        // Advance by the size of the item
        position.x += rock_icons_recs[i].width * items_sprite.atlas.scale.x;
    }
}

void game_render()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    {
        BeginMode2D(game.camera);
    
        render_queue_flush();

        //render_map();

        EndMode2D();

        render_items();

        static int xx = 0;
        if(IsKeyPressed('X'))
        {
            xx = (xx + 1) % ARRAY_LENGTH(building_offsets);
        }
        DrawText(TextFormat("%d", xx), 0, GetScreenHeight() - 40.0f, 20, WHITE);
        debug_slider(0, &building_offsets[xx].x, -200.0f, 200.0f);
        debug_slider(1, &building_offsets[xx].y, -200.0f, 200.0f);
    }

    EndDrawing();

    game.tick++;
}