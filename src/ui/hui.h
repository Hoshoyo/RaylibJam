#pragma once
#include <raylib.h>
#include <raymath.h>

typedef enum {
	PALETTE_DARK,
	PALETTE_LIGHT,
	PALETTE_POP,
	PALETTE_MID,
} PaletteKind;

typedef struct {
	Color colors[4];
} ColorPalette;

// Fog Between pines
static const ColorPalette palette_fog_between_pines = {{
	{ 0x24, 0x26, 0x2B, 0xFF },	// black
	{ 0xE1, 0xDF, 0xD0, 0xFF },	// ivory
	{ 0xB4, 0x6C, 0x56, 0xFF }, // Fire red //{ 0x7D, 0xA1, 0xBF, 0xFF },	// blue gray
	{ 0x52, 0x55, 0x50, 0xFF },	// gray
}};

// Peaceful cottage path
static const ColorPalette palette_peaceful_cottage_path = {{
	{ 0x41, 0x47, 0x54, 0xFF },	// charcoal
	{ 0xE3, 0xE8, 0xF0, 0xFF },	// baby blue
	{ 0xE5, 0xD7, 0xBE, 0xFF },	// sand altar
	{ 0x82, 0x80, 0x7F, 0xFF },	// gray
}};

// Nature wonders
static const ColorPalette palette_nature_wonders = {{
	{ 0x59, 0x2F, 0x2D, 0xFF },	// carafe
	{ 0xFA, 0xCF, 0xDF, 0xFF },	// rose quartz
	{ 0xFB, 0x8A, 0x36, 0xFF },	// orange
	{ 0x7A, 0x40, 0x3E, 0xFF },	// marsala
}};

// Mountain ridge
static const ColorPalette palette_mountain_ridge = {{
	{ 0x43, 0x58, 0x61, 0xFF },	// charcoal
	{ 0xE7, 0xE3, 0xDD, 0xFF },	// ivory
	{ 0xEA, 0xC2, 0x88, 0xFF },	// peach
	{ 0x4D, 0x77, 0x4D, 0xFF },	// green
}};

typedef enum {
    HOUI_INTERACT_NONE          = 0,
    HOUI_INTERACT_HOVERED       = (1 << 0),
    HOUI_INTERACT_CLICKED       = (1 << 1),
    HOUI_INTERACT_RIGHT_CLICKED = (1 << 2),
    HOUI_INTERACT_EDITED        = (1 << 3),
    HOUI_INTERACT_RELEASED      = (1 << 4),
} HoUiInteraction;

typedef struct {
    int length;
    int cursor;
    int selection;
} HoTextBox;

void hui_init();
void hui_color_palette_set(ColorPalette palette);
Font hui_default_font_get();

HoUiInteraction ho_textbox(HoTextBox* tbox, bool active, Vector2 position, Vector2 size, Vector2 margin, Font font, char* buffer, int buffer_length);
HoUiInteraction ho_button(Vector2 position, Vector2 size, Font font, const char* text);
HoUiInteraction ho_button_circle(Vector2 center, float radius, Font font, const char* text, bool interactive);
HoUiInteraction ho_button_circle_texture(Vector2 center, float radius, Texture2D texture);
HoUiInteraction ho_slider_circle(Vector2 position, bool active, Vector2 size, float* value, float min, float max);
HoUiInteraction ho_label(Vector2 position, Vector2 size, Font font, const char* text);
HoUiInteraction ho_button_icon_label(Rectangle rect, Texture2D icon, float icon_size, Font font, const char* label, Color base_color, bool enabled, float animation);
HoUiInteraction ho_combo_box(Vector2 position, bool active, Vector2 size, Font font, const char** options, int option_count, int* selected_index);