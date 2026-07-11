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