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

    // ── Merged artifacts (6) ─────────────────────────────────────────────────
    ITEM_MERGED_1,
    ITEM_MERGED_2,
    ITEM_MERGED_3,
    ITEM_MERGED_4,
    ITEM_MERGED_5,
    ITEM_MERGED_6,

    ITEM_COUNT,

    // Range markers (not real enum values, just aliases for bounds checking)
    ITEM_ROCK_BEGIN   = ITEM_BLACKROCK_1,
    ITEM_ROCK_END     = ITEM_FROSTSILVER,   // exclusive upper bound
    ITEM_ORE_BEGIN    = ITEM_FROSTSILVER,
    ITEM_ORE_END      = ITEM_MERGED_1,      // exclusive upper bound
    ITEM_MERGED_BEGIN = ITEM_MERGED_1,
    ITEM_MERGED_END   = ITEM_COUNT,         // exclusive upper bound
} ItemId;

typedef enum {
    ITEM_EFFECT_NONE = 0,          // rocks
    ITEM_EFFECT_COLD_CONDUCTION,   // Frostsilver
    ITEM_EFFECT_FOUNDATION,        // Dunstone
    ITEM_EFFECT_DARK_EDGE,         // Nightsteel
    ITEM_EFFECT_HEAVY_SUPPORT,     // Umberite
    ITEM_EFFECT_CRIMSON_PULSE,     // Crimsonite
    ITEM_EFFECT_FROZEN_COLUMN,     // Glacierite
    ITEM_EFFECT_TWILIGHT_ECHO,     // Vesperium
    ITEM_EFFECT_ROOT_NETWORK,      // Mossiron
    ITEM_EFFECT_CULTIVATION,       // Verdantite
    ITEM_EFFECT_GREEN_RESONANCE,   // Viridium
    ITEM_EFFECT_DISCHARGE,         // Stormshard
    ITEM_EFFECT_ENERGY_FLOW,       // Aquarite
    ITEM_EFFECT_GILDING,           // Gloamgold
    ITEM_EFFECT_GOLDEN_CHAIN,      // Auricite
    ITEM_EFFECT_RISING_LIGHT,      // Dawnstone
    ITEM_EFFECT_RADIANCE,          // Solarium
    ITEM_EFFECT_HEAT_TRANSFER,     // Sunbrass
    ITEM_EFFECT_TEMPERING,         // Ambersteel
    ITEM_EFFECT_SKYFALL,           // Skylume
    ITEM_EFFECT_BRANCHING_GROWTH,  // Wildspire
    ITEM_EFFECT_SINGULARITY,       // Voidstone
    ITEM_EFFECT_EMPOWERMENT,       // Bloodiron
    ITEM_EFFECT_ABYSSAL_FIELD,     // Abyssite
    ITEM_EFFECT_GRIM_FORMATION,    // Grimsteel
    ITEM_EFFECT_COUNT,
} ItemPlacementEffect;

// Static metadata for an item type — never changes at runtime.
typedef struct {
    const char*        name;
    float              nativeEnergy;
    float              rarity;
    ItemPlacementEffect effect;
    bool               movable;   // true = can be dragged out of the crafter grid
} ItemInfo;

// A concrete item instance held by the game.
typedef struct {
    ItemId      id;
    const char* name;          // pointer into item_info_table — no ownership
    float       nativeEnergy;
    float       rarity;
    float       energy;        // computed/modified at runtime; starts at 0
} Item;

// Returns the static metadata for an ItemId (no bounds-check in release).
const ItemInfo* item_info(ItemId id);

// Generate a pseudo-random item: 65% rock, 35% ore.
// Relies on Raylib's GetRandomValue — call after InitWindow.
Item item_generate(float ore_chance);

// Create a merged artifact with the given energy, picking a random
// sprite from the merge_cubes_recs atlas entries.
Item merged_item_generate(float energy);

// Register the atlas and source-rect arrays used by item_render.
// Must be called once after the atlas texture is loaded (e.g. in game_init).
void item_render_init(Texture2D atlas,
                      Rectangle* rock_recs,   int rock_count,
                      Rectangle* ore_recs,    int ore_count,
                      Rectangle* merged_recs, int merged_count);

// Render an item icon at screen position (x, y) scaled to `size` pixels.
// If show_name is true, draws the item name at the bottom-left of the quad.
void item_render(const Item* item, float x, float y, float size, bool show_name);
