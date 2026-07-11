#include "font.h"

#define FONT_PATH "res/ui/fonts/Barlow_Semi_Condensed/BarlowSemiCondensed-SemiBold.ttf"

static const int font_pixel_sizes[FONT_SIZE_COUNT] = {
    [FONT_SIZE_TITLE] = 20,
    [FONT_SIZE_PANEL] = 16,
    [FONT_SIZE_BODY]  = 12,
};

static Font fonts[FONT_SIZE_COUNT];

void font_init(void)
{
    for (int i = 0; i < FONT_SIZE_COUNT; ++i) {
        fonts[i] = LoadFontEx(FONT_PATH, font_pixel_sizes[i], 0, 0);
        SetTextureFilter(fonts[i].texture, TEXTURE_FILTER_POINT);
    }
}

Font* font_get(FontSize size)
{
    return &fonts[size];
}
