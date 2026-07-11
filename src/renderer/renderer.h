#pragma once
#include <raylib.h>
#include <stdint.h>
#include <raymath.h>

#define ARRAY_LENGTH(A) (sizeof(A) / sizeof(*(A)))
#define MIN(a,b) (((a)<(b))? (a):(b))
#define MAX(a,b) (((a)>(b))? (a):(b))
#define INDEX(X, Y, WIDTH) ((Y) * (WIDTH) + (X))
#define TODO(STR) assert(false && STR)
#define UNREACHABLE() assert(false)

typedef enum {
    SPRITE_TYPE_STATIC_ATLAS,
    SPRITE_TYPE_ANIMATED,
    SPRITE_TYPE_RECT,
    SPRITE_TYPE_LINE,
    SPRITE_TYPE_TEXT,
} SpriteType;

typedef enum {
    SPRITE_FLAG_NONE     = 0,
    SPRITE_FLAG_CENTERED = (1 << 0),
} SpriteFlag;

typedef struct {
    Texture2D texture;
    int32_t   piece_count;
    int32_t   row_count;
    int32_t   column_count;
    int32_t   piece_width;
    int32_t   piece_height;
    float     scale;
} SpriteGridAtlas;

typedef struct {
    const char*     filepath;
    SpriteGridAtlas atlas;
    Vector2         move_factor; // multiplied by the frame_index
    Vector2         offset;
    uint32_t        frame_index;
    float           opacity;
    SpriteFlag      flags;
} SpriteAnimated;

typedef struct {
    const char* path;
    bool        loaded;
    Texture2D   texture;
    Vector2     origin;
    Vector2     scale;
    float       opacity;
    SpriteFlag  flags;
    Rectangle   src_rec;
} SpriteStaticAtlas;

typedef struct {
    SpriteType type;
    union {
        SpriteStaticAtlas static_atlas;
        SpriteAnimated    animated;
    };
} Sprite;

typedef struct {
    Sprite   sprite;
    Vector2  position;
    Vector2  offset;
    Color    tint;
    int32_t  layer;
    int32_t  original_index;
} RenderCommand;

void render_queue_flush();
void render_sprite_animated(SpriteAnimated* sprite, Vector2 position, int32_t layer, uint32_t frame_index, Color tint);
void render_sprite_static_atlas(SpriteStaticAtlas* atlas, Rectangle src, Vector2 position, int32_t layer, Color tint);
void render_sprite_static_atlas_immediate(SpriteStaticAtlas* atlas, Rectangle src, Vector2 position, int32_t layer, Color tint);
void render_sprite_static_atlas_offset(SpriteStaticAtlas* atlas, Rectangle src, Vector2 offset, Vector2 position, int32_t layer, Color tint);

int sprite_frame_count(SpriteAnimated* sprite);