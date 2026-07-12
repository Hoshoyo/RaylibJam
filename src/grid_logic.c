#include "grid_logic.h"

// ── Effect function type ──────────────────────────────────────────────────────
// Each function adds bonus portions (the part above 1.0) into bonus[r][c].
// The resolution rule: bonuses are additive — x1.20 + x1.30 = x1.50.
typedef void (*EffectFn)(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]);

// ── Shared helper ─────────────────────────────────────────────────────────────
static void add_bonus(const Grid* g, int r, int c, float amount, float (*bonus)[GRID_COLS]) {
    if (r >= 0 && r < GRID_ROWS && c >= 0 && c < GRID_COLS && g->has_item[r][c])
        bonus[r][c] += amount;
}

// ── Effect implementations ────────────────────────────────────────────────────
static void effect_none(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    (void)g; (void)r; (void)c; (void)bonus;
}

// Frostsilver — multiply item directly to the left by x1.45
static void effect_cold_conduction(const Grid* g, int r, int c, float (*bonus)[GRID_COLS])
    { add_bonus(g, r, c-1, 0.45f, bonus); }

// Dunstone — multiply vertical neighbor by x1.20 (below; above if on bottom row)
static void effect_foundation(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    int target_r = (r == GRID_ROWS - 1) ? r - 1 : r + 1;
    add_bonus(g, target_r, c, 0.20f, bonus);
}

// Nightsteel — multiply items to the left and right by x1.20 each
static void effect_dark_edge(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, r, c-1, 0.20f, bonus);
    add_bonus(g, r, c+1, 0.20f, bonus);
}

// Umberite — multiply item directly to the left by x1.30
static void effect_heavy_support(const Grid* g, int r, int c, float (*bonus)[GRID_COLS])
    { add_bonus(g, r, c-1, 0.30f, bonus); }

// Crimsonite — multiply all orthogonally adjacent items by x1.18
static void effect_crimson_pulse(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, r-1, c,   0.18f, bonus);
    add_bonus(g, r+1, c,   0.18f, bonus);
    add_bonus(g, r,   c-1, 0.18f, bonus);
    add_bonus(g, r,   c+1, 0.18f, bonus);
}

// Glacierite — multiply the other item in the same column by x1.45
static void effect_frozen_column(const Grid* g, int r, int c, float (*bonus)[GRID_COLS])
    { add_bonus(g, 1 - r, c, 0.45f, bonus); }

// Vesperium — multiply every diagonally adjacent item by x1.45
static void effect_twilight_echo(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, r-1, c-1, 0.45f, bonus);
    add_bonus(g, r-1, c+1, 0.45f, bonus);
    add_bonus(g, r+1, c-1, 0.45f, bonus);
    add_bonus(g, r+1, c+1, 0.45f, bonus);
}

// Mossiron — multiply the other items in the same row by x1.10
static void effect_root_network(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    for (int cc = 0; cc < GRID_COLS; ++cc)
        if (cc != c) add_bonus(g, r, cc, 0.10f, bonus);
}

// Verdantite — multiply every item with lower nativeEnergy by x1.12
static void effect_cultivation(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    float self_e = g->items[r][c].nativeEnergy;
    for (int rr = 0; rr < GRID_ROWS; ++rr)
        for (int cc = 0; cc < GRID_COLS; ++cc)
            if ((rr != r || cc != c) && g->has_item[rr][cc]
                && g->items[rr][cc].nativeEnergy < self_e)
                bonus[rr][cc] += 0.12f;
}

// Viridium — multiply every orthogonally adjacent item by x1.35
static void effect_green_resonance(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, r-1, c,   0.35f, bonus);
    add_bonus(g, r+1, c,   0.35f, bonus);
    add_bonus(g, r,   c-1, 0.35f, bonus);
    add_bonus(g, r,   c+1, 0.35f, bonus);
}

// Stormshard — multiply item directly to the right by x1.75
static void effect_discharge(const Grid* g, int r, int c, float (*bonus)[GRID_COLS])
    { add_bonus(g, r, c+1, 0.75f, bonus); }

// Aquarite — multiply the other two items in the same row by x1.35
static void effect_energy_flow(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    for (int cc = 0; cc < GRID_COLS; ++cc)
        if (cc != c) add_bonus(g, r, cc, 0.35f, bonus);
}

// Gloamgold — multiply the highest-energy other item by x1.35
static void effect_gilding(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    float best_e = -1.0f;
    int br = -1, bc = -1;
    for (int rr = 0; rr < GRID_ROWS; ++rr)
        for (int cc = 0; cc < GRID_COLS; ++cc)
            if ((rr != r || cc != c) && g->has_item[rr][cc]
                && g->items[rr][cc].energy > best_e) {
                best_e = g->items[rr][cc].energy;
                br = rr; bc = cc;
            }
    if (br >= 0) bonus[br][bc] += 0.35f;
}

// Auricite — multiply other items in the same row by x1.30
static void effect_golden_chain(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    for (int cc = 0; cc < GRID_COLS; ++cc)
        if (cc != c) add_bonus(g, r, cc, 0.30f, bonus);
}

// Dawnstone — multiply every item to the right (same row) by x1.15
static void effect_rising_light(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    for (int cc = c + 1; cc < GRID_COLS; ++cc)
        add_bonus(g, r, cc, 0.15f, bonus);
}

// Solarium — multiply all other items in its row and column by x1.25
static void effect_radiance(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    for (int cc = 0; cc < GRID_COLS; ++cc)
        if (cc != c) add_bonus(g, r, cc, 0.25f, bonus);
    for (int rr = 0; rr < GRID_ROWS; ++rr)
        if (rr != r) add_bonus(g, rr, c, 0.25f, bonus);
}

// Sunbrass — multiply the other item in the same column by x1.55
static void effect_heat_transfer(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, 1 - r, c, 0.55f, bonus);
}

// Ambersteel — multiply items to the left and right by x1.28 each
static void effect_tempering(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, r, c-1, 0.28f, bonus);
    add_bonus(g, r, c+1, 0.28f, bonus);
}

// Skylume — multiply all items in the opposite row by x1.30
static void effect_skyfall(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    int opp = 1 - r;
    for (int cc = 0; cc < GRID_COLS; ++cc)
        add_bonus(g, opp, cc, 0.30f, bonus);
}

// Wildspire — multiply same-column item by x1.85, diagonal neighbors by x1.20
static void effect_branching_growth(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, 1 - r, c,   0.85f, bonus);
    add_bonus(g, r-1,   c-1, 0.20f, bonus);
    add_bonus(g, r-1,   c+1, 0.20f, bonus);
    add_bonus(g, r+1,   c-1, 0.20f, bonus);
    add_bonus(g, r+1,   c+1, 0.20f, bonus);
}

// Voidstone — find the lowest-energy other item and multiply by x2.25
static void effect_singularity(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    float worst_e = 1e9f;
    int wr = -1, wc = -1;
    for (int rr = 0; rr < GRID_ROWS; ++rr)
        for (int cc = 0; cc < GRID_COLS; ++cc)
            if ((rr != r || cc != c) && g->has_item[rr][cc]
                && g->items[rr][cc].energy < worst_e) {
                worst_e = g->items[rr][cc].energy;
                wr = rr; wc = cc;
            }
    if (wr >= 0) bonus[wr][wc] += 1.25f;
}

// Bloodiron — find highest-energy orthogonal neighbor and multiply by x1.60
static void effect_empowerment(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    static const int dr[] = {-1, 1,  0, 0};
    static const int dc[] = { 0, 0, -1, 1};
    float best_e = -1.0f;
    int br = -1, bc = -1;
    for (int i = 0; i < 4; ++i) {
        int nr = r + dr[i], nc = c + dc[i];
        if (nr >= 0 && nr < GRID_ROWS && nc >= 0 && nc < GRID_COLS
            && g->has_item[nr][nc] && g->items[nr][nc].energy > best_e) {
            best_e = g->items[nr][nc].energy;
            br = nr; bc = nc;
        }
    }
    if (br >= 0) bonus[br][bc] += 0.60f;
}

// Abyssite — multiply every other item in the grid by x1.18
static void effect_abyssal_field(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    for (int rr = 0; rr < GRID_ROWS; ++rr)
        for (int cc = 0; cc < GRID_COLS; ++cc)
            if ((rr != r || cc != c) && g->has_item[rr][cc])
                bonus[rr][cc] += 0.18f;
}

// Grimsteel — multiply all orthogonally adjacent items by x1.16
static void effect_grim_formation(const Grid* g, int r, int c, float (*bonus)[GRID_COLS]) {
    add_bonus(g, r-1, c,   0.16f, bonus);
    add_bonus(g, r+1, c,   0.16f, bonus);
    add_bonus(g, r,   c-1, 0.16f, bonus);
    add_bonus(g, r,   c+1, 0.16f, bonus);
}

// ── Effect table ──────────────────────────────────────────────────────────────
typedef struct {
    const char* description;
    EffectFn    apply;
} PlacementEffectInfo;

static const PlacementEffectInfo effect_table[ITEM_EFFECT_COUNT] = {
    [ITEM_EFFECT_NONE]             = { "No effect.",                                                                                            effect_none            },
    [ITEM_EFFECT_COLD_CONDUCTION]  = { "Cold Conduction: multiplies the energy of the item directly to its left by x1.45.",                     effect_cold_conduction },
    [ITEM_EFFECT_FOUNDATION]       = { "Foundation: multiplies the energy of the vertical neighbor by x1.20.",                                  effect_foundation      },
    [ITEM_EFFECT_DARK_EDGE]        = { "Dark Edge: multiplies the energy of the items to its left and right by x1.20 each.",                    effect_dark_edge       },
    [ITEM_EFFECT_HEAVY_SUPPORT]    = { "Heavy Support: multiplies the energy of the item directly to its left by x1.30.",                       effect_heavy_support   },
    [ITEM_EFFECT_CRIMSON_PULSE]    = { "Crimson Pulse: multiplies the energy of every orthogonally adjacent item by x1.18.",                    effect_crimson_pulse   },
    [ITEM_EFFECT_FROZEN_COLUMN]    = { "Frozen Column: multiplies the energy of the other item in the same column by x1.45.",                   effect_frozen_column   },
    [ITEM_EFFECT_TWILIGHT_ECHO]    = { "Twilight Echo: multiplies the energy of every diagonally adjacent item by x1.45.",                      effect_twilight_echo   },
    [ITEM_EFFECT_ROOT_NETWORK]     = { "Root Network: multiplies the energy of the other items in the same row by x1.10.",                      effect_root_network    },
    [ITEM_EFFECT_CULTIVATION]      = { "Cultivation: multiplies the energy of every item with lower base energy by x1.12.",                     effect_cultivation     },
    [ITEM_EFFECT_GREEN_RESONANCE]  = { "Green Resonance: multiplies the energy of every orthogonally adjacent item by x1.35.",                  effect_green_resonance },
    [ITEM_EFFECT_DISCHARGE]        = { "Discharge: multiplies the energy of the item directly to its right by x1.75.",                          effect_discharge       },
    [ITEM_EFFECT_ENERGY_FLOW]      = { "Energy Flow: multiplies the energy of the other item in the same row by x1.35.",                   effect_energy_flow     },
    [ITEM_EFFECT_GILDING]          = { "Gilding: multiplies the energy of the highest-energy other item in the crafting grid by x1.35.", effect_gilding },
    [ITEM_EFFECT_GOLDEN_CHAIN]     = { "Golden Chain: multiplies the energy of the other items in the same row by x1.30.",                      effect_golden_chain    },
    [ITEM_EFFECT_RISING_LIGHT]     = { "Rising Light: multiplies the energy of every item positioned to its right by x1.15.",                   effect_rising_light    },
    [ITEM_EFFECT_RADIANCE]         = { "Radiance: multiplies the energy of all other items in its row and column by x1.25.",                    effect_radiance        },
    [ITEM_EFFECT_HEAT_TRANSFER]    = { "Heat Transfer: multiplies the energy of the other item in the same column by x1.55.",                   effect_heat_transfer   },
    [ITEM_EFFECT_TEMPERING]        = { "Tempering: multiplies the energy of the items to its left and right by x1.28 each.",                    effect_tempering       },
    [ITEM_EFFECT_SKYFALL]          = { "Skyfall: multiplies the energy of all items in the opposite row by x1.30.",                             effect_skyfall         },
    [ITEM_EFFECT_BRANCHING_GROWTH] = { "Branching Growth: multiplies the energy of the same-column item by x1.85, and diagonal neighbors by x1.20.", effect_branching_growth},
    [ITEM_EFFECT_SINGULARITY]      = { "Singularity: multiplies the energy of the lowest-energy other item by x2.25.",                             effect_singularity  },
    [ITEM_EFFECT_EMPOWERMENT]      = { "Empowerment: multiplies the energy of the highest-energy orthogonal neighbor by x1.60.",                   effect_empowerment  },
    [ITEM_EFFECT_ABYSSAL_FIELD]    = { "Abyssal Field: multiplies the energy of every other item in the grid by x1.18.",                        effect_abyssal_field   },
    [ITEM_EFFECT_GRIM_FORMATION]   = { "Grim Formation: multiplies the energy of all orthogonally adjacent items by x1.16.",                    effect_grim_formation  },
};

// ── Public API ────────────────────────────────────────────────────────────────
bool grid_is_full(const Grid* grid)
{
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c)
            if (!grid->has_item[r][c]) return false;
    return true;
}

float grid_compute_energy(const Grid* grid)
{
    // Accumulate all bonus portions simultaneously — no chain reactions.
    float bonus[GRID_ROWS][GRID_COLS] = {{0}};

    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c)
            if (grid->has_item[r][c]) {
                ItemPlacementEffect eff = item_info(grid->items[r][c].id)->effect;
                effect_table[eff].apply(grid, r, c, bonus);
            }

    // Apply placement bonuses and sum raw item energies
    float total = 0.0f;
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c)
            if (grid->has_item[r][c])
                total += grid->items[r][c].energy * (1.0f + bonus[r][c]);

    return total;
}

const char* grid_effect_description(ItemPlacementEffect effect)
{
    if (effect < 0 || effect >= ITEM_EFFECT_COUNT) return "Unknown effect.";
    return effect_table[effect].description;
}
