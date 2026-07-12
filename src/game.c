#include "game.h"
#include "renderer/renderer.h"
#include "renderer/assets.h"

#include <stdlib.h>
#include <stdio.h>

#include "ui.h"
#include "item.h"
#include "font.h"

static Game game;
static bool ui_hovered_or_active;
static Vector2 camera_position;

static Vector2 iso_pos(Vector2 ortho);

int s_fill_order[CITY_GRID * CITY_GRID];

static int fill_order_cmp(const void* a, const void* b)
{
    int ia = *(const int*)a, ib = *(const int*)b;
    float ra = (float)(ia / CITY_GRID), ca = (float)(ia % CITY_GRID) - 7.5f;
    float rb = (float)(ib / CITY_GRID), cb = (float)(ib % CITY_GRID) - 7.5f;
    float da = ra*ra + ca*ca, db = rb*rb + cb*cb;
    if (da != db) return (da > db) - (da < db);
    return ia - ib;
}

void city_init(int building_count)
{
    static const char* street_names[CITY_GRID] = {
        "Sunflower Avenue", "Maple Street",    "Willow Street",   "Lavender Street",
        "Magnolia Avenue",  "Cedar Street",    "Poppy Street",    "Birch Street",
        "Jacaranda Avenue", "Rose Street",     "Elm Street",      "Daisy Street",
        "Acacia Avenue",    "Pine Street",     "Orchid Street",   "Clover Street",
    };
    for (int i = 0; i < CITY_GRID * CITY_GRID; i++) {
        int r = i / CITY_GRID, c = i % CITY_GRID;
        game.city[r][c] = (City_Building){ false, 0.0f, GetRandomValue(0, building_count - 1) };
        snprintf(game.city[r][c].address, sizeof(game.city[r][c].address),
                 "%d %s", 101 + c * 12, street_names[r]);
        game.city[r][c].people_living = GetRandomValue(200, 800);
        s_fill_order[i] = i;
    }
    qsort(s_fill_order, CITY_GRID * CITY_GRID, sizeof(int), fill_order_cmp);
}

void resize_city(int new_size)
{
    // Count currently filled buildings.
    int current_filled = 0;
    for (int i = 0; i < CITY_GRID * CITY_GRID; i++) {
        int idx = s_fill_order[i];
        if (game.city[idx / CITY_GRID][idx % CITY_GRID].filled)
            current_filled++;
    }

    if (new_size > current_filled) {
        // Fill gaps in fill order (closest to center first) until we reach the target.
        int to_add = new_size - current_filled;
        for (int i = 0; i < CITY_GRID * CITY_GRID && to_add > 0; i++) {
            int idx = s_fill_order[i];
            City_Building* b = &game.city[idx / CITY_GRID][idx % CITY_GRID];
            if (!b->filled) {
                b->filled              = true;
                b->needed_energy       = 0.0f;
                b->days_without_energy = 0;
                to_add--;
            }
        }
    } else if (new_size < current_filled) {
        // Remove buildings from the end of fill order (outermost first).
        int to_remove = current_filled - new_size;
        for (int i = CITY_GRID * CITY_GRID - 1; i >= 0 && to_remove > 0; i--) {
            int idx = s_fill_order[i];
            City_Building* b = &game.city[idx / CITY_GRID][idx % CITY_GRID];
            if (b->filled) {
                b->filled = false;
                to_remove--;
            }
        }
    }
}

// Base energy demand increase per building per day (tweak this).
#define ENERGY_PER_HOUSE_BASE 1.5f

bool is_everyone_happy_now(void)
{
    for (int i = 0; i < CITY_GRID * CITY_GRID; i++) {
        int r = i / CITY_GRID, c = i % CITY_GRID;
        if (game.city[r][c].filled && game.city[r][c].days_without_energy != 0)
            return false;
    }
    return true;
}

void game_next_day(bool allow_growth)
{
    if (allow_growth) {
        // Grow city by 1-3 new buildings.
        int new_houses = GetRandomValue(1, 3);
        game.city_size += new_houses;
        if (game.city_size > CITY_GRID * CITY_GRID)
            game.city_size = CITY_GRID * CITY_GRID;
        resize_city(game.city_size);
    }

    // Increase each existing building's energy demand.
    // Formula: base * day_scale * random_factor
    //   day_scale  = 1 + (day-1)*0.15  → +15% per day
    //   random_factor ∈ [0.8, 1.2]
    float day_scale = 1.0f + (game.day - 1) * 0.15f;
    for (int i = 0; i < CITY_GRID * CITY_GRID; i++) {
        int r = i / CITY_GRID, c = i % CITY_GRID;
        if (!game.city[r][c].filled) continue;
        float rand_factor = 0.8f + GetRandomValue(0, 40) / 100.0f;
        game.city[r][c].needed_energy += ENERGY_PER_HOUSE_BASE * day_scale * rand_factor;
    }

    // Aggregate total energy demand across all buildings.
    float total = 0.0f;
    for (int i = 0; i < CITY_GRID * CITY_GRID; i++) {
        int r = i / CITY_GRID, c = i % CITY_GRID;
        if (game.city[r][c].filled)
            total += game.city[r][c].needed_energy;
    }
    game.needed_energy = total;

    game.day++;
}

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
    game.stored_energy   = 0.0f;
    game.needed_energy   = 0.0f;
    game.research_points = 0.0f;
    game.city_size       = 4;
    game.day             = 1;

    font_init();
    ui_init();
    load_assets();
    item_render_init(items_sprite.atlas.texture,
                     rock_icons_recs,   ARRAY_LENGTH(rock_icons_recs),
                     merged_icons_recs, ARRAY_LENGTH(merged_icons_recs),
                     merge_cubes_recs,  ARRAY_LENGTH(merge_cubes_recs));
    load_sounds();
    update_city_sound();
    city_init(ARRAY_LENGTH(building_recs));
    resize_city(game.city_size);

    // Point camera at building #1 (first in fill order)
    int b0 = s_fill_order[0];
    camera_position = iso_pos((Vector2){
        (float)((b0 % CITY_GRID) - 8) * 180.0f,
        (float)(8 - b0 / CITY_GRID)   * 180.0f,
    });
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

Vector2 iso_pos(Vector2 ortho)
{
    return (Vector2){(ortho.x + ortho.y), (ortho.x - ortho.y)};
}

void render_map()
{
    Vector2 mouse_world = GetScreenToWorld2D(GetMousePosition(), game.camera);

    #define GRID_SIZE 180

    SetRandomSeed(456);
    for(int y = 8; y > -8; --y)
    {
        for(int x = -8; x < 8; ++x)
        {
            int row = 8 - y;
            int col = x + 8;
            const City_Building* cb = &game.city[row][col];

            Vector2 position = (Vector2){x * GRID_SIZE, y * GRID_SIZE};
            position = iso_pos(position);
            int random_pole = GetRandomValue(0, pole_sprite.rect_count - 1);
            int random_building = GetRandomValue(0, pole_sprite.rect_count - 1);

            Rectangle bbox = (Rectangle){position.x-100.0f, position.y-200.0f, 200, 300};
            Color hover_tint = WHITE;
            if(CheckCollisionPointRec(mouse_world, bbox))
            {
                if (cb->filled) {
                    hover_tint = ColorBrightness(GREEN, 0.5f);
                    ui_set_building_tooltip(cb, GetMousePosition().x, GetMousePosition().y);
                }
            }

            render_sprite_static_atlas(&road.atlas, (Rectangle){0, 0, road.atlas.texture.width, road.atlas.texture.height - 100}, position, 0, WHITE);
            if (cb->filled) {
                render_sprite_static_atlas_offset(&buildings_shadow.atlas, building_recs[random_building], building_offsets[random_building], position, 1, WHITE);
                render_sprite_static_atlas_offset(&buildings_albedo.atlas, building_recs[random_building], building_offsets[random_building], position, 1, hover_tint);
                Vector2 pole_position = Vector2Add(position, (Vector2){30, 50});
                render_sprite_static_atlas_offset(&pole_sprite.atlas, pole_sprite.recs[random_pole], pole_offsets[random_pole], pole_position, 1, WHITE);
            }
            if(GetRandomValue(0, 100) < 70)
            {
                int random_tree = GetRandomValue(0, trees_sprite.rect_count - 1);
                int random_spread = GetRandomValue(-80, 80);
                Vector2 tree_position = Vector2Add(position, (Vector2){random_spread, 20});
                render_sprite_static_atlas(&trees_sprite.atlas, trees_sprite.recs[random_tree], tree_position, 1, WHITE);
            }

            // Power-out icon: only on filled buildings that did not get energy last day
            if (cb->filled && cb->days_without_energy > 0)
            {
                const float icon_size = 64.0f;
                Vector2 icon_pos = Vector2Add(position, (Vector2){-icon_size * 0.5f, -130.0f});
                Rectangle src = power_icons_recs[0];
                Rectangle dst = (Rectangle){icon_pos.x, icon_pos.y, icon_size, icon_size};
                DrawTexturePro(power_icons.atlas.texture, src, dst, (Vector2){0,0}, 0, WHITE);

                Font font = *font_get(FONT_SIZE_LARGE);
                char energy_str[16];
                snprintf(energy_str, sizeof(energy_str), "%.1f", cb->needed_energy);
                Vector2 tsz = MeasureTextEx(font, energy_str, font.baseSize, 0);
                Vector2 text_pos = (Vector2){
                    icon_pos.x + (icon_size - tsz.x) * 0.5f,
                    icon_pos.y + icon_size + 2.0f
                };
                DrawTextEx(font, energy_str, text_pos, font.baseSize, 0, WHITE);
                //DrawTextEx(font, TextFormat("%.2f %.2f", position.x, position.y), text_pos, font.baseSize, 0, WHITE);
            }
            
        }
    }

    render_sprite_static_atlas(&powerplant.atlas, (Rectangle){0, 0, powerplant.atlas.texture.width, powerplant.atlas.texture.height }, (Vector2){1500, -2200}, 0, WHITE);

    // Ambient sound
    SetRandomSeed((int)(GetTime() * 1000.0f));
    if(!IsSoundPlaying(sounds.city[0]) && !IsSoundPlaying(sounds.city[1]))
    {
        int ambient = GetRandomValue(0, ARRAY_LENGTH(sounds.city) - 1);
        PlaySound(sounds.city[ambient]);
        TraceLog(LOG_INFO, "Changed ambient sound to %d", ambient);
    }
    if(!IsSoundPlaying(sounds.birds))
    {
        PlaySound(sounds.birds);
    }
}

void game_render()
{
    BeginDrawing();
    ClearBackground((Color){161,151,132,0xff});

    {
        BeginMode2D(game.camera);
    
        render_queue_flush();

        render_map();
        SetRandomSeed((unsigned int)(GetTime() * 100000.0));  // restore varying seed after map

        EndMode2D();

        ui_hovered_or_active = ui_render(&game);
    }

    EndDrawing();

    game.animation_timer = sinf(GetTime() * 10.0f);

    game.tick++;
}