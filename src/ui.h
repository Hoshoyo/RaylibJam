#pragma once
#include <stdbool.h>
#include "game.h"

void ui_init();
// Returns true when the UI is capturing mouse input (menu open or item being dragged).
// Store the result and pass it to game_update to suppress camera movement.
bool ui_render(const Game* game);