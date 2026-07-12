#pragma once
#include <raylib.h>

// All available font sizes. Add entries here if new sizes are needed.
typedef enum {
    FONT_SIZE_TITLE = 0,  // 20px — window titles, headers
    FONT_SIZE_PANEL,      // 16px — panel labels, buttons
    FONT_SIZE_BODY,       // 12px — small labels, item names
    FONT_SIZE_LARGE,      // 48px — world-space labels (avoids upscale pixelation)
    FONT_SIZE_COUNT,
} FontSize;

// Load all fonts. Must be called once after InitWindow.
void font_init(void);

// Return a reference to a loaded Font by size enum.
// Valid after font_init(); do not call before.
Font* font_get(FontSize size);
