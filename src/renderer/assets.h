#pragma once
#include "renderer.h"
#include <stdbool.h>

typedef struct {
    SpriteStaticAtlas atlas;
    int rect_count;
    Rectangle* recs;
} AssetAtlas;

typedef struct {
    SpriteAnimated albedo; 
    SpriteAnimated shadow;
} AssetAnimatedSprite;

#include "data.h"

static AssetAtlas* assets[] = {
    &grass,
    &buildings_albedo,
    &buildings_shadow,
    &pole_sprite,
    &items_sprite,
    &trees_sprite,
    &power_icons,
};

static void load_assets()
{
    for(int i = 0; i < ARRAY_LENGTH(assets); ++i)
    {
        if(!assets[i]->atlas.loaded)
        {
            assets[i]->atlas.loaded = true;
            assets[i]->atlas.texture = LoadTexture(assets[i]->atlas.path);
        }
    }

    character[0].albedo.atlas.texture = LoadTexture(character->albedo.filepath);
    character[0].shadow.atlas.texture = LoadTexture(character->shadow.filepath);
}

typedef struct {
    float master_volume;
    Sound city[2];
    Sound birds;
    Sound discharge[2];
    Sound click;
} SoundFxs;
static SoundFxs sounds;

static void load_sounds()
{
    InitAudioDevice();
    sounds.master_volume = 0.5f;

    sounds.city[0] = LoadSound("res/sound/p1_839933__kevp888__250823_163306_ph_traffic_in_calapan_city.ogg");
    sounds.city[1] = LoadSound("res/sound/p2_839933__kevp888__250823_163306_ph_traffic_in_calapan_city.ogg");

    SetSoundVolume(sounds.city[0], sounds.master_volume * 0.1f);
    SetSoundVolume(sounds.city[1], sounds.master_volume * 0.1f);

    sounds.birds = LoadSound("res/sound/563547__valentinpetiteau__ambience-european-city-park-morning-birds-light-wind.ogg");
    SetSoundVolume(sounds.birds, sounds.master_volume * 0.5f);

    sounds.discharge[0] = LoadSound("res/sound/d1_721021__chungus43a__high-voltage-equipment.ogg");
    sounds.discharge[1] = LoadSound("res/sound/d2_721021__chungus43a__high-voltage-equipment.ogg");

    SetSoundVolume(sounds.discharge[0], sounds.master_volume * 0.5f);
    SetSoundVolume(sounds.discharge[1], sounds.master_volume * 0.5f);

    sounds.click = LoadSound("res/sound/switch.wav");
    SetSoundVolume(sounds.click, sounds.master_volume * 0.2f);
}