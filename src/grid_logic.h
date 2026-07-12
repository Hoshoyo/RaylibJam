#pragma once
#include "item.h"
#include <stdbool.h>

#define GRID_ROWS 2
#define GRID_COLS 3

// Represents the 2x3 crafting grid.
// grid[row][col]: row 0 = top row, row 1 = bottom row.
typedef struct {
    bool has_item[GRID_ROWS][GRID_COLS];
    Item items[GRID_ROWS][GRID_COLS];
} Grid;

// Returns true if all GRID_ROWS * GRID_COLS slots are occupied.
bool grid_is_full(const Grid* grid);

// Computes the final aggregated quality value for the grid,
// applying all placement effects simultaneously (additive bonus model).
// Bonuses from multiple sources are added (e.g. x1.20 + x1.30 = x1.50).
// research_bonus is added flat to each occupied item's effective quality.
// Returns the sum of all modified item qualities.
float grid_compute_quality(const Grid* grid);

// Returns the human-readable description of a placement effect.
const char* grid_effect_description(ItemPlacementEffect effect);
