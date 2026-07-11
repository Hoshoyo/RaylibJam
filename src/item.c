#include "item.h"
#include "font.h"
// raylib.h and stdbool.h are pulled in via item.h

// Static lookup table, indexed directly by ItemId.
// Order must match the ItemId enum exactly.
static const ItemInfo item_info_table[ITEM_COUNT] = {
    // ── Rocks ─────────────────────────────────────────────────────────────────
    [ITEM_BLACKROCK_1] = { "Blackrock", 0.9f, 4.7f },
    [ITEM_BLACKROCK_2] = { "Blackrock", 1.0f, 4.5f },
    [ITEM_BLACKROCK_3] = { "Blackrock", 0.8f, 4.8f },
    [ITEM_BLACKROCK_4] = { "Blackrock", 0.9f, 4.6f },
    [ITEM_BLACKROCK_5] = { "Blackrock", 1.0f, 4.5f },
    [ITEM_GRAYSTONE_1] = { "Graystone", 1.5f, 4.1f },
    [ITEM_GRAYSTONE_2] = { "Graystone", 1.6f, 4.0f },
    [ITEM_GRAYSTONE_3] = { "Graystone", 1.4f, 4.2f },
    [ITEM_GRAYSTONE_4] = { "Graystone", 1.7f, 3.9f },

    // ── Ores ──────────────────────────────────────────────────────────────────
    [ITEM_FROSTSILVER] = { "Frostsilver", 3.0f, 2.6f },
    [ITEM_DUNSTONE]    = { "Dunstone",    2.2f, 3.5f },
    [ITEM_NIGHTSTEEL]  = { "Nightsteel",  3.1f, 2.4f },
    [ITEM_UMBERITE]    = { "Umberite",    2.8f, 2.9f },
    [ITEM_CRIMSONITE]  = { "Crimsonite",  3.2f, 2.3f },
    [ITEM_GLACIERITE]  = { "Glacierite",  3.0f, 2.5f },
    [ITEM_VESPERIUM]   = { "Vesperium",   3.5f, 1.9f },
    [ITEM_MOSSIRON]    = { "Mossiron",    2.7f, 3.1f },
    [ITEM_VERDANTITE]  = { "Verdantite",  3.4f, 2.1f },
    [ITEM_VIRIDIUM]    = { "Viridium",    4.0f, 1.4f },
    [ITEM_STORMSHARD]  = { "Stormshard",  3.3f, 2.2f },
    [ITEM_AQUARITE]    = { "Aquarite",    4.1f, 1.2f },
    [ITEM_GLOAMGOLD]   = { "Gloamgold",   3.0f, 2.5f },
    [ITEM_AURICITE]    = { "Auricite",    3.5f, 1.9f },
    [ITEM_DAWNSTONE]   = { "Dawnstone",   2.9f, 2.6f },
    [ITEM_SOLARIUM]    = { "Solarium",    3.8f, 1.6f },
    [ITEM_SUNBRASS]    = { "Sunbrass",    3.1f, 2.4f },
    [ITEM_AMBERSTEEL]  = { "Ambersteel",  3.4f, 2.0f },
    [ITEM_SKYLUME]     = { "Skylume",     4.2f, 1.0f },
    [ITEM_WILDSPIRE]   = { "Wildspire",   3.7f, 1.7f },
    [ITEM_VOIDSTONE]   = { "Voidstone",   4.6f, 0.6f },
    [ITEM_BLOODIRON]   = { "Bloodiron",   3.2f, 2.2f },
    [ITEM_ABYSSITE]    = { "Abyssite",    4.4f, 0.8f },
    [ITEM_GRIMSTEEL]   = { "Grimsteel",   3.0f, 2.5f },
};

const ItemInfo* item_info(ItemId id)
{
    return &item_info_table[id];
}

Item item_generate(void)
{
    ItemId id;

    // 65% rock, 35% ore
    bool is_rock = (GetRandomValue(1, 100) <= 65);
    if (is_rock) {
        id = (ItemId)GetRandomValue(ITEM_ROCK_BEGIN, ITEM_ROCK_END - 1);
    } else {
        id = (ItemId)GetRandomValue(ITEM_ORE_BEGIN, ITEM_ORE_END - 1);
    }

    const ItemInfo* info = &item_info_table[id];
    Item item;
    item.id            = id;
    item.name          = info->name;
    item.nativeQuality = info->nativeQuality;
    item.rarity        = info->rarity;
    item.quality       = GetRandomValue(0, 500) / 100.0f;
    return item;
}

// ── Rendering ────────────────────────────────────────────────────────────────

static Texture2D  s_atlas;
static Rectangle* s_rock_recs;
static int        s_rock_count;
static Rectangle* s_ore_recs;
static int        s_ore_count;

void item_render_init(Texture2D atlas, Rectangle* rock_recs, int rock_count,
                      Rectangle* ore_recs,  int ore_count)
{
    s_atlas      = atlas;
    s_rock_recs  = rock_recs;
    s_rock_count = rock_count;
    s_ore_recs   = ore_recs;
    s_ore_count  = ore_count;
}

void item_render(const Item* item, float x, float y, float size, bool show_name)
{
    if (!item) return;

    Rectangle src;
    if (item->id < ITEM_ROCK_END) {
        int idx = item->id - ITEM_ROCK_BEGIN;
        if (idx >= s_rock_count) return;
        src = s_rock_recs[idx];
    } else {
        int idx = item->id - ITEM_ORE_BEGIN;
        if (idx >= s_ore_count) return;
        src = s_ore_recs[idx];
    }

    DrawTexturePro(s_atlas,
        src,
        (Rectangle){x, y, size, size},
        (Vector2){0, 0}, 0.0f, WHITE);

    if (show_name) {
        Font* font = font_get(FONT_SIZE_BODY);

        Color name_color;
        if      (item->quality < 1.5f) name_color = CLITERAL(Color){ 40, 40, 40, 255 }; // gray
        else if (item->quality < 3.0f) name_color = CLITERAL(Color){  80, 200,  80, 255 }; // green
        else if (item->quality < 4.0f) name_color = CLITERAL(Color){  30, 130,  30, 255 }; // darker green
        else                           name_color = CLITERAL(Color){ 200,  40,  40, 255 }; // red

        DrawTextEx(*font, TextToUpper(item->name),
            (Vector2){x, y + size - font->baseSize},
            font->baseSize, 0, name_color);
    }
}
