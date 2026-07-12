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

void play_random_pitch(Sound sound, float range)
{
    int v = GetRandomValue(-100, 100);
    float f = ((float)v / 100.0f) * range;
    SetSoundPitch(sound, 1.0f + f);
    PlaySound(sound);
}

void update_city_sound()
{
    float value = (game.camera.zoom > 0.4f) ? 0.2f : 0.1f;

    SetSoundVolume(sounds.city[0], sounds.master_volume * value);
    SetSoundVolume(sounds.city[1], sounds.master_volume * value);
    SetSoundVolume(sounds.birds, sounds.master_volume * value * 2.5f);
}

void game_init()
{
    game.camera.zoom = 0.5f;
    game.camera.target = (Vector2){-360.0f, -360.0f};

    ui_palette = palette_mountain_ridge;

    load_assets();
    load_sounds();
    update_city_sound();
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
    if(wheel > 0 && game.camera.zoom < 2.0f)
    {
        game.camera.zoom *= 2.0f;
        update_city_sound();
    }
    else if(wheel < 0 && game.camera.zoom >= 0.5f)
    {
        game.camera.zoom /= 2.0f;
        update_city_sound();
    }

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
    for(int y = 8; y > -8; --y)
    {
        for(int x = -8; x < 8; ++x)
        {
            Vector2 position = (Vector2){x * GRID_SIZE, y * GRID_SIZE};
            position = iso_pos(position);
            int random_building = GetRandomValue(0, pole_sprite.rect_count - 1);
            int random_pole = GetRandomValue(0, pole_sprite.rect_count - 1);

            //if(building_here)
            {
                render_sprite_static_atlas_offset(&buildings_shadow.atlas, building_recs[random_building], building_offsets[random_building], position, 0, WHITE);
                render_sprite_static_atlas_offset(&buildings_albedo.atlas, building_recs[random_building], building_offsets[random_building], position, 0, WHITE);

                Vector2 pole_position = Vector2Add(position, (Vector2){30, 50});
                render_sprite_static_atlas_offset(&pole_sprite.atlas, pole_sprite.recs[random_pole], pole_offsets[random_pole], pole_position, 0, WHITE);
            }
            if(GetRandomValue(0, 100) < 70)
            {
                int random_tree = GetRandomValue(0, trees_sprite.rect_count - 1);
                int random_spread = GetRandomValue(-80, 80);
                Vector2 tree_position = Vector2Add(position, (Vector2){random_spread, 20});
                render_sprite_static_atlas(&trees_sprite.atlas, trees_sprite.recs[random_tree], tree_position, 0, WHITE);
            }

            // Power out icon
            if(GetRandomValue(0, 100) < 10)
            {
                render_sprite_static_atlas(&power_icons.atlas, power_icons_recs[0], Vector2Add(position, (Vector2){0, -100}), 1, WHITE);
            }
        }
    }

    SetRandomSeed((int)(GetTime() * 1000.0f));
    // Ambient sound
    if(!IsSoundPlaying(sounds.city[0]) && !IsSoundPlaying(sounds.city[1]))
    {
        PlaySound(sounds.city[GetRandomValue(0, ARRAY_LENGTH(sounds.city) - 1)]);
    }
    if(!IsSoundPlaying(sounds.birds))
    {
        PlaySound(sounds.birds);
    }

    if(IsKeyPressed('Q'))
    {
        play_random_pitch(sounds.click, 0.1f);
    }
    if(IsKeyPressed('E'))
    {
        int discharge = GetRandomValue(0, ARRAY_LENGTH(sounds.discharge) - 1);
        //PlaySound(sounds.discharge[discharge]);
        play_random_pitch(sounds.discharge[discharge], 0.1f);
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

        render_map();

        EndMode2D();

        render_items();
    }

    EndDrawing();

    game.tick++;
}