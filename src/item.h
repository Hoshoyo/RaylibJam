#pragma once
#include <raylib.h>
#include <stdbool.h>

typedef enum {
    // ── Rocks (9) ────────────────────────────────────────────────────────────
    ITEM_BLACKROCK_1 = 0,
    ITEM_BLACKROCK_2,
    ITEM_BLACKROCK_3,
    ITEM_BLACKROCK_4,
    ITEM_BLACKROCK_5,
    ITEM_GRAYSTONE_1,
    ITEM_GRAYSTONE_2,
    ITEM_GRAYSTONE_3,
    ITEM_GRAYSTONE_4,

    // ── Ores (24) ────────────────────────────────────────────────────────────
    ITEM_FROSTSILVER,
    ITEM_DUNSTONE,
    ITEM_NIGHTSTEEL,
    ITEM_UMBERITE,
    ITEM_CRIMSONITE,
    ITEM_GLACIERITE,
    ITEM_VESPERIUM,
    ITEM_MOSSIRON,
    ITEM_VERDANTITE,
    ITEM_VIRIDIUM,
    ITEM_STORMSHARD,
    ITEM_AQUARITE,
    ITEM_GLOAMGOLD,
    ITEM_AURICITE,
    ITEM_DAWNSTONE,
    ITEM_SOLARIUM,
    ITEM_SUNBRASS,
    ITEM_AMBERSTEEL,
    ITEM_SKYLUME,
    ITEM_WILDSPIRE,
    ITEM_VOIDSTONE,
    ITEM_BLOODIRON,
    ITEM_ABYSSITE,
    ITEM_GRIMSTEEL,

    ITEM_COUNT,

    // Range markers (not real enum values, just aliases for bounds checking)
    ITEM_ROCK_BEGIN = ITEM_BLACKROCK_1,
    ITEM_ROCK_END   = ITEM_FROSTSILVER,   // exclusive upper bound
    ITEM_ORE_BEGIN  = ITEM_FROSTSILVER,
    ITEM_ORE_END    = ITEM_COUNT,         // exclusive upper bound
} ItemId;

// Static metadata for an item type — never changes at runtime.
typedef struct {
    const char* name;
    float       nativeQuality;
    float       rarity;
} ItemInfo;

// A concrete item instance held by the game.
typedef struct {
    ItemId      id;
    const char* name;          // pointer into item_info_table — no ownership
    float       nativeQuality;
    float       rarity;
    float       quality;       // computed/modified at runtime; starts at 0
} Item;

// Returns the static metadata for an ItemId (no bounds-check in release).
const ItemInfo* item_info(ItemId id);

// Generate a pseudo-random item: 65% rock, 35% ore.
// Relies on Raylib's GetRandomValue — call after InitWindow.
Item item_generate(void);

// Register the atlas and source-rect arrays used by item_render.
// Must be called once after the atlas texture is loaded (e.g. in game_init).
void item_render_init(Texture2D atlas, Rectangle* rock_recs, int rock_count,
                      Rectangle* ore_recs,  int ore_count);

// Render an item icon at screen position (x, y) scaled to `size` pixels.
// If show_name is true, draws the item name at the bottom-left of the quad.
void item_render(const Item* item, float x, float y, float size, bool show_name);
