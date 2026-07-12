#include "ui.h"

#include <stdio.h>
#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)
#include "game.h"
#include "item.h"
#include "font.h"
#include "grid_logic.h"
#include "ui/hui.h"
static ColorPalette ui_palette;
#include "ui/ho_button.c"
#include "ui/ho_label.c"
#include "ui/ho_slider.c"

#define FACTORY_MENU_X0 30.0f
#define FACTORY_MENU_Y0 30.0f
#define FACTORY_MENU_X1 (WINDOW_WIDTH - 30.0f)
#define FACTORY_MENU_Y1 (WINDOW_WIDTH - 30.0f)

// Max buildings evicted per Next Day press (separate from DAYS_UNTIL_EVICTION).
#define MAXIMUM_BUILDINGS_TO_BE_REMOVED 2

#define ARRAY_LENGTH(A) (sizeof(A) / sizeof(*(A)))

extern SoundFxs sounds;
extern float animation_timer;

// Deferred item tooltip — set during the frame, flushed at the very end of ui_render.
typedef struct {
    bool        active;
    const Item* item;
    float       x;
    float       y;
} UiTooltip;
static UiTooltip ui_tooltip;

// Deferred text tooltip — for plain hover text (e.g. buttons).
typedef struct {
    bool        active;
    const char* text;
    float       x;
    float       y;
} UiTextTooltip;
static UiTextTooltip ui_text_tooltip;

// Deferred building tooltip — set from game.c hover, flushed at end of ui_render.
typedef struct {
    bool                 active;
    const City_Building* building;
    float                x, y;
} UiBuildingTooltip;
static UiBuildingTooltip ui_building_tooltip;

void ui_set_building_tooltip(const City_Building* cb, float x, float y)
{
    ui_building_tooltip = (UiBuildingTooltip){ true, cb, x, y };
}

// ── Slot / drag / drop types ─────────────────────────────────────────────────

#define FACTORY_SLOT_COUNT 7
typedef enum {
    SLOT_POPPER     = 0,
    SLOT_CRAFTER_0,
    SLOT_CRAFTER_1,
    SLOT_CRAFTER_2,
    SLOT_CRAFTER_3,
    SLOT_CRAFTER_4,
    SLOT_CRAFTER_5,
} FactorySlotId;

typedef struct {
    bool has_item[FACTORY_SLOT_COUNT];
    Item items[FACTORY_SLOT_COUNT];
} FactoryMenuState;
static FactoryMenuState factory_state;

// Frame-persistent: survives across frames while the mouse button is held.
typedef struct {
    bool          active;
    Item          item;
    FactorySlotId source_slot;
} UiDrag;
static UiDrag ui_drag;

#define ITEM_GENERATES_PER_DAY 8
static int s_items_generated_today = 0;
static int s_last_seen_day         = -1;

// Frame-transient: reset at the top of ui_render every frame.
typedef struct {
    bool          any_hovered;
    FactorySlotId hovered_slot;
    bool          trash_hovered;
} UiDropTarget;
static UiDropTarget ui_drop_target;

static Texture2D tex_close;
static Texture2D tex_place_item;
static Texture2D tex_new;
static Texture2D tex_refresh;
static Texture2D tex_trash;
static Texture2D tex_factory;
static Texture2D tex_research;
static Texture2D tex_energy;
static Texture2D tex_flash;
static Texture2D tex_battery_no;
static Texture2D tex_house;
static Texture2D tex_sun;

static Texture2D tex_moon;

void ui_init() {
    ui_palette = palette_mountain_ridge;
    tex_close      = LoadTexture("res/ui/close.png");
    tex_place_item = LoadTexture("res/ui/place_item.png");
    tex_new        = LoadTexture("res/ui/new.png");
    tex_refresh    = LoadTexture("res/ui/refresh.png");
    tex_trash      = LoadTexture("res/ui/trash.png");
    tex_factory    = LoadTexture("res/ui/factory.png");
    tex_research   = LoadTexture("res/ui/research.png");
    tex_energy     = LoadTexture("res/ui/battery.png");
    tex_flash      = LoadTexture("res/ui/flash.png");
    tex_battery_no = LoadTexture("res/ui/battery_no.png");
    tex_house      = LoadTexture("res/ui/house.png");
    tex_sun        = LoadTexture("res/ui/sun.png");
    tex_moon       = LoadTexture("res/ui/moon.png");
}

// crafted energy — computed each frame when grid is full, -1 otherwise.
static float s_crafted_energy = -1.0f;
// current game pointer — set each frame in ui_render for use by tooltip rendering.
static const Game* s_current_game = NULL;

// Research merge multiplier: sqrt-based, fast early gains with diminishing returns.
static float research_multiplier(void)
{
    if (!s_current_game) return 1.0f;
    return 1.0f + sqrtf(s_current_game->research_points) / 3.0f;
}

// circle button with icon shifted up + label text at the bottom
// bg_color overrides the default palette background color.
static HoUiInteraction ho_button_circle_icon_label(Vector2 center, float radius, Texture2D icon, const char* label, bool interactive, Color bg_color, float animation) {
    HoUiInteraction result = {0};

    if (!interactive) {
        const float border_width = 2.0f;
        Color dis_bg     = CLITERAL(Color){ 90,  90,  90, 255 };
        Color dis_border = CLITERAL(Color){ 60,  60,  60, 255 };
        Color dis_icon   = CLITERAL(Color){ 255, 255, 255, 80  };
        Color dis_text   = CLITERAL(Color){ 130, 130, 130, 255 };

        DrawCircleV(center, radius + border_width, dis_border);
        DrawCircleV(center, radius, dis_bg);

        const float icon_size  = radius * 0.9f;
        const float icon_shift = radius * 0.28f;
        DrawTexturePro(icon,
            (Rectangle){0, 0, icon.width, icon.height},
            (Rectangle){center.x - icon_size*0.5f, center.y - icon_size*0.5f - icon_shift, icon_size, icon_size},
            (Vector2){0,0}, 0.0f, dis_icon);

        Font font = *font_get(FONT_SIZE_PANEL);
        Vector2 sz = MeasureTextEx(font, label, font.baseSize, 0);
        DrawTextEx(font, label,
            (Vector2){center.x - floorf(sz.x*0.5f), center.y + radius*0.65f - sz.y},
            font.baseSize, 0, dis_text);

        return result;
    }

    Color color_background = bg_color;
    Color color_text       = ui_palette.colors[PALETTE_LIGHT];
    Color color_border     = ColorBrightness(color_background, 0.1f);
    const float border_width = 2.0f;

    if (CheckCollisionPointCircle(GetMousePosition(), center, radius)) {
        result |= HOUI_INTERACT_HOVERED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  result |= HOUI_INTERACT_CLICKED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    animation = ((animation + 1.0f) / 2.0f) * 0.5f;

    bool hovered = (result & HOUI_INTERACT_HOVERED) != 0;
    DrawCircleV(center, radius + border_width, color_border);
    DrawCircleV(center, radius, hovered ? ColorBrightness(color_background, animation) : color_background);

    const float icon_size = radius * 0.9f;
    const float icon_shift_up = radius * 0.28f;
    Rectangle icon_dest = (Rectangle){
        center.x - icon_size * 0.5f,
        center.y - icon_size * 0.5f - icon_shift_up,
        icon_size, icon_size
    };
    DrawTexturePro(icon,
        (Rectangle){0, 0, icon.width, icon.height},
        icon_dest, (Vector2){0, 0}, 0.0f, WHITE);

    Font font = *font_get(FONT_SIZE_PANEL);
    Vector2 sz = MeasureTextEx(font, label, font.baseSize, 0);
    Vector2 text_pos = (Vector2){
        center.x - floorf(sz.x * 0.5f),
        center.y + radius * 0.65f - sz.y
    };
    DrawTextEx(font, label, text_pos, font.baseSize, 0, hovered ? color_background : color_text);

    return result;
}

static void render_factory_menu_base_init(bool* open, float window_height) {
    // window background
    Rectangle rec = (Rectangle){FACTORY_MENU_X0, FACTORY_MENU_Y0, FACTORY_MENU_X1 - FACTORY_MENU_X0, window_height};
    DrawRectangleRec(rec, LIGHTGRAY);
}

static void render_factory_menu_base_end(bool* open, float window_height) {
    Rectangle rec = (Rectangle){FACTORY_MENU_X0, FACTORY_MENU_Y0, FACTORY_MENU_X1 - FACTORY_MENU_X0, window_height};

    const float btn_size = 24.0f;
    const float btn_margin = 6.0f;
    const float topbar_height = btn_size + btn_margin * 2.0f;

    // top bar background
    Rectangle topbar_rec = (Rectangle){FACTORY_MENU_X0, FACTORY_MENU_Y0, FACTORY_MENU_X1 - FACTORY_MENU_X0, topbar_height};
    DrawRectangleRec(topbar_rec, CLITERAL(Color){ 150, 150, 150, 255 });

    // title
    Font font = *font_get(FONT_SIZE_TITLE);
    const float title_font_size = font.baseSize;
    const char* title = "FACTORY MENU";
    Vector2 title_size = MeasureTextEx(font, title, title_font_size, 0);
    Vector2 title_pos = (Vector2){
        FACTORY_MENU_X0 + btn_margin,
        FACTORY_MENU_Y0 + floorf(topbar_height / 2.0f - title_size.y / 2.0f)
    };
    DrawTextEx(font, title, title_pos, title_font_size, 0, CLITERAL(Color){ 30, 30, 30, 255 });

    // close button
    Rectangle close_rec = (Rectangle){FACTORY_MENU_X1 - btn_size - btn_margin, FACTORY_MENU_Y0 + btn_margin, btn_size, btn_size};
    bool close_hovered = CheckCollisionPointRec(GetMousePosition(), close_rec);
    DrawTexturePro(tex_close,
        (Rectangle){0, 0, tex_close.width, tex_close.height},
        close_rec,
        (Vector2){0, 0}, 0.0f,
        close_hovered ? WHITE : DARKGRAY);
    if (close_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        *open = false;
    }

    // top bar bottom separator
    DrawLineEx(
        (Vector2){FACTORY_MENU_X0, FACTORY_MENU_Y0 + topbar_height},
        (Vector2){FACTORY_MENU_X1, FACTORY_MENU_Y0 + topbar_height},
        2.0f, CLITERAL(Color){ 130, 130, 130, 255 });

    // window outline
    DrawRectangleLinesEx(rec, 2.0f, CLITERAL(Color){ 55, 55, 55, 255 });
}

static void draw_sunken_slot_ex(Rectangle rect, Color fill) {
    const float bevel = 2.0f;
    DrawRectangleRec(rect, fill);
    DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width, (int)bevel, CLITERAL(Color){ 85, 85, 85, 255 });
    DrawRectangle((int)rect.x, (int)rect.y, (int)bevel, (int)rect.height, CLITERAL(Color){ 85, 85, 85, 255 });
    DrawRectangle((int)rect.x, (int)(rect.y + rect.height - bevel), (int)rect.width, (int)bevel, CLITERAL(Color){ 205, 205, 205, 255 });
    DrawRectangle((int)(rect.x + rect.width - bevel), (int)rect.y, (int)bevel, (int)rect.height, CLITERAL(Color){ 205, 205, 205, 255 });
}

static void draw_sunken_slot(Rectangle rect) {
    draw_sunken_slot_ex(rect, CLITERAL(Color){ 148, 148, 148, 255 });
}

// draws a raised-bevel panel; returns the inner-content y offset from panel_y
static void draw_raised_panel(float panel_x, float panel_y, float panel_width, float panel_height, Color bg) {
    const float pbevel = 2.0f;
    DrawRectangle((int)panel_x, (int)panel_y, (int)panel_width, (int)panel_height, bg);
    DrawRectangle((int)panel_x, (int)panel_y, (int)panel_width, (int)pbevel, CLITERAL(Color){ 210, 210, 210, 255 });
    DrawRectangle((int)panel_x, (int)panel_y, (int)pbevel, (int)panel_height, CLITERAL(Color){ 210, 210, 210, 255 });
    DrawRectangle((int)panel_x, (int)(panel_y + panel_height - pbevel), (int)panel_width, (int)pbevel, CLITERAL(Color){ 85, 85, 85, 255 });
    DrawRectangle((int)(panel_x + panel_width - pbevel), (int)panel_y, (int)pbevel, (int)panel_height, CLITERAL(Color){ 85, 85, 85, 255 });
}

// Draws one item slot: background, hover highlight (during drag), item icon,
// drag-start on click, drop-target registration, and tooltip deferral.
// check_movable=true: non-movable occupied slots are locked (dark bg, no drag). Set false for the popper slot.
static void draw_item_slot(Rectangle rect, FactorySlotId id, FactoryMenuState* state, bool force_locked, bool check_movable)
{
    bool is_drag_source = ui_drag.active && ui_drag.source_slot == id;
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    bool item_locked = force_locked
        || (check_movable && state->has_item[id] && !item_info(state->items[id].id)->movable);

    // Background: non-movable = darkest; movable = medium; empty/popper = light
    if (check_movable && state->has_item[id] && !item_info(state->items[id].id)->movable)
        draw_sunken_slot_ex(rect, CLITERAL(Color){ 130, 130, 130, 255 });
    else
        draw_sunken_slot_ex(rect, CLITERAL(Color){ 178, 178, 178, 255 });

    // Highlight as valid drop target when dragging over it
    if (ui_drag.active && hovered) {
        ui_drop_target.any_hovered  = true;
        ui_drop_target.hovered_slot = id;
        DrawRectangleLinesEx(rect, 2.0f, CLITERAL(Color){ 255, 220, 60, 255 });
    }

    // Draw item (hidden while being dragged — rendered as ghost on cursor instead)
    if (state->has_item[id] && !is_drag_source) {
        float item_size = rect.width - 8.0f;
        if (item_size < 4.0f) item_size = 4.0f;
        float ix = rect.x + floorf((rect.width  - item_size) * 0.5f);
        float iy = rect.y + floorf((rect.height - item_size) * 0.5f);
        item_render(&state->items[id], ix, iy, item_size, false);
    }

    // Begin drag on click — blocked if item is not movable
    if (!item_locked && !ui_drag.active && hovered && state->has_item[id]
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        ui_drag.active      = true;
        ui_drag.item        = state->items[id];
        ui_drag.source_slot = id;
        state->has_item[id] = false;
    }

    // Tooltip — suppressed while any drag is active
    if (!ui_drag.active && hovered && state->has_item[id])
    {
        Vector2 cursor = GetMousePosition();
        ui_tooltip.active = true;
        ui_tooltip.item   = &state->items[id];
        ui_tooltip.x      = cursor.x + 12.0f;
        ui_tooltip.y      = cursor.y + 4.0f;
    }
}

// Splits `text` into word-wrapped lines that fit within `max_width` pixels.
// Returns the number of lines produced (at most max_lines).
static int text_wrap_lines(Font font, const char* text, float max_width,
                            char lines[][256], int max_lines)
{
    int line_count = 0;
    char current[256];
    int  cur_len = 0;
    current[0] = '\0';

    const char* p = text;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        const char* word_start = p;
        while (*p && *p != ' ') p++;
        int word_len = (int)(p - word_start);

        // build candidate: current + optional space + word
        char test[256];
        int  test_len = 0;
        for (int i = 0; i < cur_len; i++)               test[test_len++] = current[i];
        if (cur_len > 0)                                  test[test_len++] = ' ';
        for (int i = 0; i < word_len && test_len < 255; i++) test[test_len++] = word_start[i];
        test[test_len] = '\0';

        float w = MeasureTextEx(font, test, font.baseSize, 0).x;
        if (w > max_width && cur_len > 0) {
            // flush current line
            if (line_count < max_lines) {
                for (int i = 0; i <= cur_len; i++) lines[line_count][i] = current[i];
                line_count++;
            }
            // start new line with this word
            cur_len = 0;
            for (int i = 0; i < word_len && i < 255; i++) current[cur_len++] = word_start[i];
            current[cur_len] = '\0';
        } else {
            for (int i = 0; i <= test_len; i++) current[i] = test[i];
            cur_len = test_len;
        }
    }

    if (cur_len > 0 && line_count < max_lines) {
        for (int i = 0; i <= cur_len; i++) lines[line_count][i] = current[i];
        line_count++;
    }
    return line_count;
}

// Renders an item description tooltip at (x, y) — top-left corner.
// Uses a plain raised panel with dark background.
static void render_item_description_panel(const Item* item, float x, float y)
{
    const float padding     = 8.0f;
    const float max_text_w  = 330.0f;   // hard cap so very long effect strings don't run off-screen
    Font* font              = font_get(FONT_SIZE_BODY);
    const float line_h      = font->baseSize + 5.0f;

    const Color bg        = CLITERAL(Color){  30,  30,  30, 230 };
    const Color fg        = CLITERAL(Color){ 220, 220, 220, 255 };
    const Color brd       = CLITERAL(Color){  80,  80,  80, 255 };
    const Color eff_color = CLITERAL(Color){ 220,  60,  60, 255 };
    const Color eff_blue  = CLITERAL(Color){ 120, 190, 220, 255 };

    // Measure natural (unwrapped) width of every text piece.
    char energy_base[32];
    snprintf(energy_base, sizeof(energy_base), "ENERGY: %.2f", item->energy);

    const char* effect_desc   = grid_effect_description(item_info(item->id)->effect);
    bool        is_immovable  = !item_info(item->id)->movable;
    const char* immovable_str = "This item cannot be moved once placed in the crafting grid.";

    float w = MeasureTextEx(*font, TextToUpper(item->name), font->baseSize, 0).x;
    float t = MeasureTextEx(*font, energy_base,              font->baseSize, 0).x;
    if (t > w) w = t;
    t = MeasureTextEx(*font, effect_desc, font->baseSize, 0).x;
    if (t > w) w = t;
    if (is_immovable) {
        t = MeasureTextEx(*font, immovable_str, font->baseSize, 0).x;
        if (t > w) w = t;
    }

    // Clamp to cap; text_wrap_lines will only wrap if a line exceeds text_w.
    float text_w  = w < max_text_w ? w : max_text_w;
    float panel_w = text_w + padding * 2.0f;

    // Pre-compute wrapped lines so we know panel height upfront.
    char eff_lines[6][256];
    int  eff_line_count = text_wrap_lines(*font, effect_desc, text_w, eff_lines, 6);

    char immov_lines[4][256];
    int  immov_line_count = is_immovable ? text_wrap_lines(*font, immovable_str, text_w, immov_lines, 4) : 0;

    int total_lines = 2 + eff_line_count + immov_line_count;
    const float panel_h = padding + line_h * total_lines - (line_h - font->baseSize) + padding;

    DrawRectangle((int)x, (int)y, (int)panel_w, (int)panel_h, bg);
    DrawRectangleLinesEx((Rectangle){x, y, panel_w, panel_h}, 1.0f, brd);

    float tx = x + padding;
    float ty = y + padding;

    // Name — colored by item category
    Color name_color;
    if      (item->id >= ITEM_MERGED_BEGIN && item->id < ITEM_MERGED_END)
        name_color = CLITERAL(Color){ 220,  60,  60, 255 }; // merged  → red
    else if (item->id >= ITEM_ORE_BEGIN    && item->id < ITEM_ORE_END)
        name_color = CLITERAL(Color){ 120, 210, 120, 255 }; // ore     → light green
    else
        name_color = CLITERAL(Color){ 160, 160, 160, 255 }; // rock    → gray
    DrawTextEx(*font, TextToUpper(item->name), (Vector2){tx, ty}, font->baseSize, 0, name_color);
    ty += line_h;

    // Energy line
    DrawTextEx(*font, energy_base, (Vector2){tx, ty}, font->baseSize, 0, fg);
    ty += line_h;

    // Effect description — light blue
    for (int i = 0; i < eff_line_count; i++) {
        DrawTextEx(*font, eff_lines[i], (Vector2){tx, ty}, font->baseSize, 0, eff_blue);
        ty += line_h;
    }

    // Immovable warning — red
    for (int i = 0; i < immov_line_count; i++) {
        DrawTextEx(*font, immov_lines[i], (Vector2){tx, ty}, font->baseSize, 0, eff_color);
        ty += line_h;
    }
}

// Renders a plain text tooltip at (x, y). Word-wraps to fit panel_w.
static void render_text_tooltip_panel(const char* text, float x, float y)
{
    const float padding    = 8.0f;
    const float max_text_w = 300.0f;
    Font* font             = font_get(FONT_SIZE_BODY);
    const float line_h     = font->baseSize + 5.0f;

    const Color bg  = CLITERAL(Color){  30,  30,  30, 230 };
    const Color fg  = CLITERAL(Color){ 220, 220, 220, 255 };
    const Color brd = CLITERAL(Color){  80,  80,  80, 255 };

    // Size panel to fit text naturally, capped at max_text_w.
    float natural_w = MeasureTextEx(*font, text, font->baseSize, 0).x;
    float text_w    = natural_w < max_text_w ? natural_w : max_text_w;
    float panel_w   = text_w + padding * 2.0f;

    char lines[6][256];
    int  line_count = text_wrap_lines(*font, text, text_w, lines, 6);

    const float panel_h = padding + line_h * line_count - (line_h - font->baseSize) + padding;

    // clamp so tooltip doesn't go off screen
    float rx = x, ry = y;
    if (rx + panel_w > WINDOW_WIDTH)  rx = WINDOW_WIDTH - panel_w - 4.0f;
    if (ry + panel_h > WINDOW_HEIGHT) ry = y - panel_h - 8.0f;

    DrawRectangle((int)rx, (int)ry, (int)panel_w, (int)panel_h, bg);
    DrawRectangleLinesEx((Rectangle){rx, ry, panel_w, panel_h}, 1.0f, brd);

    float tx = rx + padding;
    float ty = ry + padding;
    for (int i = 0; i < line_count; i++) {
        DrawTextEx(*font, lines[i], (Vector2){tx, ty}, font->baseSize, 0, fg);
        ty += line_h;
    }
}

// Renders the building hover tooltip: address, residents, happiness status.
static void render_building_tooltip_panel(const City_Building* cb, float bx, float by)
{
    Font* font = font_get(FONT_SIZE_BODY);
    const float pad    = 8.0f;
    const float line_h = font->baseSize + 5.0f;

    const Color bg     = CLITERAL(Color){  30,  30,  30, 230 };
    const Color brd    = CLITERAL(Color){  80,  80,  80, 255 };
    const Color title  = CLITERAL(Color){ 240, 220, 130, 255 };
    const Color fg     = CLITERAL(Color){ 220, 220, 220, 255 };
    const Color c_good = CLITERAL(Color){  80, 200, 120, 255 };
    const Color c_bad  = CLITERAL(Color){ 220,  80,  80, 255 };

    char line_people[32];
    char line_status[80];
    Color status_color;

    snprintf(line_people, sizeof(line_people), "%d people live here", cb->people_living);
    if (cb->days_without_energy == 0) {
        snprintf(line_status, sizeof(line_status), "People are happy with the service.");
        status_color = c_good;
    } else {
        int days_left = DAYS_UNTIL_EVICTION - cb->days_without_energy;
        snprintf(line_status, sizeof(line_status), "People are complaining and may leave soon.");
        status_color = c_bad;
    }

    float w0 = MeasureTextEx(*font, cb->address,   font->baseSize, 0).x;
    float w1 = MeasureTextEx(*font, line_people,   font->baseSize, 0).x;
    float w2 = MeasureTextEx(*font, line_status,   font->baseSize, 0).x;
    float max_w = w0 > w1 ? w0 : w1;
    if (w2 > max_w) max_w = w2;

    float panel_w = max_w + pad * 2.0f;
    float panel_h = pad + line_h * 3.0f - (line_h - font->baseSize) + pad;

    float rx = bx + 12.0f, ry = by + 4.0f;
    if (rx + panel_w > WINDOW_WIDTH)  rx = WINDOW_WIDTH - panel_w - 4.0f;
    if (ry + panel_h > WINDOW_HEIGHT) ry = by - panel_h - 8.0f;

    DrawRectangle((int)rx, (int)ry, (int)panel_w, (int)panel_h, bg);
    DrawRectangleLinesEx((Rectangle){rx, ry, panel_w, panel_h}, 1.0f, brd);

    float tx = rx + pad, ty = ry + pad;
    DrawTextEx(*font, cb->address,  (Vector2){tx, ty}, font->baseSize, 0, title);       ty += line_h;
    DrawTextEx(*font, line_people,  (Vector2){tx, ty}, font->baseSize, 0, fg);           ty += line_h;
    DrawTextEx(*font, line_status,  (Vector2){tx, ty}, font->baseSize, 0, status_color);
}

static float render_item_popper_panel(float start_y, FactoryMenuState* state) {
    // Auto-populate: if slot is empty, no drag in progress from it, and uses remain, fill it.
    if (!state->has_item[SLOT_POPPER]
        && !(ui_drag.active && ui_drag.source_slot == SLOT_POPPER)
        && s_items_generated_today < ITEM_GENERATES_PER_DAY)
    {
        state->items[SLOT_POPPER]    = item_generate();
        state->has_item[SLOT_POPPER] = true;
        s_items_generated_today++;
    }

    const float window_x0 = 30.0f;
    const float window_x1 = WINDOW_WIDTH - 30.0f;
    const float panel_padding = 14.0f;
    const float btn_radius = 24.0f;
    const float large_slot_size = 80.0f;
    const float section_gap = 16.0f;

    Font font = *font_get(FONT_SIZE_PANEL);
    const float title_font_size = font.baseSize;
    const float title_area_height = title_font_size + 12.0f;

    const float content_height = large_slot_size; // slot is tallest element
    const float panel_x = window_x0;
    const float panel_width = window_x1 - window_x0;
    const float panel_height = panel_padding + title_area_height + 8.0f + content_height + panel_padding;

    // grey panel
    draw_raised_panel(panel_x, start_y, panel_width, panel_height, CLITERAL(Color){ 172, 172, 172, 255 });

    // title
    DrawTextEx(font, "ITEM POPPER",
        (Vector2){panel_x + panel_padding, start_y + panel_padding},
        title_font_size, 0, CLITERAL(Color){ 30, 30, 30, 255 });

    // content: [new btn] [large slot] [trash btn] centered
    const float total_content_width = btn_radius * 2.0f + section_gap + large_slot_size + section_gap + btn_radius * 2.0f;
    const float content_x = panel_x + floorf((panel_width - total_content_width) * 0.5f);
    const float content_y = start_y + panel_padding + title_area_height + 8.0f;
    const float center_y  = content_y + floorf(content_height * 0.5f);

    // reroll button — replaces (or creates) item in the popper slot
    bool can_generate = s_items_generated_today < ITEM_GENERATES_PER_DAY;
    Vector2 new_btn_center = (Vector2){content_x + btn_radius, center_y};
    HoUiInteraction new_btn_result = can_generate
        ? ho_button_circle_texture(new_btn_center, btn_radius, tex_refresh)
        : ho_button_circle_texture_disabled(new_btn_center, btn_radius, tex_refresh);
    if (new_btn_result & HOUI_INTERACT_HOVERED) {
        static char tooltip_buf[48];
        if (s_items_generated_today >= ITEM_GENERATES_PER_DAY)
            snprintf(tooltip_buf, sizeof(tooltip_buf), "No more items can be generated today");
        else
            snprintf(tooltip_buf, sizeof(tooltip_buf), "Shuffle item");
        ui_text_tooltip = (UiTextTooltip){ true, tooltip_buf, GetMousePosition().x, GetMousePosition().y };
    }
    if (can_generate && (new_btn_result & HOUI_INTERACT_CLICKED))
    {
        state->items[SLOT_POPPER]    = item_generate();
        state->has_item[SLOT_POPPER] = true;
        s_items_generated_today++;
    }

    // usage dots above popper slot
    {
        float slot_x_dots = content_x + btn_radius * 2.0f + section_gap;
        float slot_y_dots = content_y + floorf((content_height - large_slot_size) * 0.5f);
        const float dot_r   = 3.0f;
        const float dot_gap = 4.0f;
        float dots_total_w = ITEM_GENERATES_PER_DAY * dot_r * 2.0f + (ITEM_GENERATES_PER_DAY - 1) * dot_gap;
        float dots_x = slot_x_dots + (large_slot_size - dots_total_w) * 0.5f;
        float dots_y = slot_y_dots - dot_r * 2.0f - 4.0f;
        for (int d = 0; d < ITEM_GENERATES_PER_DAY; d++) {
            Vector2 dc = (Vector2){dots_x + d * (dot_r * 2.0f + dot_gap) + dot_r, dots_y + dot_r};
            if (d < s_items_generated_today)
                DrawCircleV(dc, dot_r, CLITERAL(Color){100, 100, 100, 200});
            else
                DrawCircleV(dc, dot_r, CLITERAL(Color){220, 220, 220, 230});
        }
    }

    // large slot
    float slot_x = content_x + btn_radius * 2.0f + section_gap;
    float slot_y = content_y + floorf((content_height - large_slot_size) * 0.5f);
    draw_item_slot((Rectangle){slot_x, slot_y, large_slot_size, large_slot_size}, SLOT_POPPER, state, false, false);

    // trash button
    Vector2 trash_center = (Vector2){slot_x + large_slot_size + section_gap + btn_radius, center_y};

    // register as drop target when dragging over trash
    if (ui_drag.active && CheckCollisionPointCircle(GetMousePosition(), trash_center, btn_radius))
        ui_drop_target.trash_hovered = true;

    // draw and handle normal click (clears slot when not dragging)
    if (ho_button_circle_texture(trash_center, btn_radius, tex_trash) & HOUI_INTERACT_CLICKED)
    {
        if (!ui_drag.active)
            state->has_item[SLOT_POPPER] = false;
    }

    return start_y + panel_height;
}

static float render_item_crafter_panel(float start_y, FactoryMenuState* state, Game* game) {
    const float window_x0 = 30.0f;
    const float window_x1 = WINDOW_WIDTH - 30.0f;
    const float panel_padding = 14.0f;
    const float small_slot_size = 42.0f;
    const float slot_gap = 6.0f;
    const int grid_cols = 3;
    const int grid_rows = 2;
    const float grid_width  = grid_cols * small_slot_size + (grid_cols - 1) * slot_gap;
    const float grid_height = grid_rows * small_slot_size + (grid_rows - 1) * slot_gap;
    const float action_btn_height = 34.0f;
    const float action_area_height = action_btn_height + 14.0f;

    Font font = *font_get(FONT_SIZE_PANEL);
    Font body_font = *font_get(FONT_SIZE_BODY);
    const float title_font_size = font.baseSize;
    const float title_area_height = title_font_size + 12.0f;
    const float energy_row_height = font.baseSize + 10.0f;

    const float panel_x = window_x0;
    const float panel_width = window_x1 - window_x0;
    const float panel_height = panel_padding + title_area_height + 8.0f + grid_height + energy_row_height + action_area_height + panel_padding;

    // determine crafted energy (no grid_locked needed — per-item movable handles it)
    bool grid_locked = false; // kept for compatibility with draw_item_slot force_locked param
    (void)grid_locked;

    Grid grid = {0};
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c) {
            FactorySlotId sid = (FactorySlotId)(SLOT_CRAFTER_0 + r * GRID_COLS + c);
            grid.has_item[r][c] = state->has_item[sid];
            grid.items[r][c]    = state->items[sid];
        }
    bool grid_full = grid_is_full(&grid);
    s_crafted_energy = grid_full ? grid_compute_energy(&grid) : -1.0f;

    // panel
    draw_raised_panel(panel_x, start_y, panel_width, panel_height, LIGHTGRAY);

    // title
    DrawTextEx(font, "ITEM CRAFTER",
        (Vector2){panel_x + panel_padding, start_y + panel_padding},
        title_font_size, 0, CLITERAL(Color){ 30, 30, 30, 255 });

    // 3x2 grid centered
    const float grid_x = panel_x + floorf((panel_width - grid_width) * 0.5f);
    const float grid_y = start_y + panel_padding + title_area_height + 8.0f;
    for (int row = 0; row < grid_rows; ++row) {
        for (int col = 0; col < grid_cols; ++col) {
            Rectangle slot = (Rectangle){
                grid_x + col * (small_slot_size + slot_gap),
                grid_y + row * (small_slot_size + slot_gap),
                small_slot_size, small_slot_size
            };
            FactorySlotId slot_id = (FactorySlotId)(SLOT_CRAFTER_0 + row * grid_cols + col);
            draw_item_slot(slot, slot_id, state, false, true);
        }
    }

    // crafted energy text
    {
        const char* energy_str = (s_crafted_energy >= 0.0f)
            ? TextFormat("Crafted Energy: %.2f", s_crafted_energy)
            : "-";
        Vector2 esz = MeasureTextEx(font, energy_str, font.baseSize, 0);
        float energy_text_x = panel_x + floorf((panel_width - esz.x) * 0.5f);
        float energy_text_y = grid_y + grid_height + floorf((energy_row_height - font.baseSize) * 0.5f);
        DrawTextEx(font, energy_str, (Vector2){energy_text_x, energy_text_y},
            font.baseSize, 0, CLITERAL(Color){ 160, 120, 20, 255 });
    }

    // bottom action buttons centered
    const float action_btn_width = 130.0f;
    const float action_btn_gap = 10.0f;
    const float btns_total_width = action_btn_width * 2.0f + action_btn_gap * 1.0f;
    const float btns_x = panel_x + floorf((panel_width - btns_total_width) * 0.5f);
    const float btns_y = start_y + panel_height - panel_padding - action_btn_height;
    const float icon_size = 26.0f;

    // Merge (dark yellow) — energizes the city
    {
        HoUiInteraction inter = ho_button_icon_label(
            (Rectangle){btns_x, btns_y, action_btn_width, action_btn_height},
            tex_energy, icon_size, body_font, "MERGE", (Color){160,120,20,255}, grid_full);
        if (inter & HOUI_INTERACT_CLICKED) {
            game->stored_energy += s_crafted_energy;
            for (int i = SLOT_CRAFTER_0; i <= SLOT_CRAFTER_5; ++i)
                state->has_item[i] = false;
            
            int discharge = GetRandomValue(0, ARRAY_LENGTH(sounds.discharge) - 1);
            play_random_pitch(sounds.discharge[discharge], 0.1f);
        }
        if (inter & HOUI_INTERACT_HOVERED) {
            Vector2 cur = GetMousePosition();
            ui_text_tooltip = (UiTextTooltip){ true, "Energize the city", cur.x, cur.y };
        }
    }
    // Send to Research (light blue)
    {
        HoUiInteraction inter = ho_button_icon_label(
            (Rectangle){btns_x + action_btn_width + action_btn_gap, btns_y, action_btn_width, action_btn_height},
            tex_research, icon_size, body_font, "SEND TO RESEARCH", (Color){60,130,170,255}, grid_full);
        if (inter & HOUI_INTERACT_CLICKED) {
            game->research_points += s_crafted_energy;
            for (int i = SLOT_CRAFTER_0; i <= SLOT_CRAFTER_5; ++i)
                state->has_item[i] = false;
        }
        if (inter & HOUI_INTERACT_HOVERED) {
            Vector2 cur = GetMousePosition();
            ui_text_tooltip = (UiTextTooltip){ true, "Energize the research facility for a stronger merge multiplier", cur.x, cur.y };
        }
    }

    return start_y + panel_height;
}

static void render_factory_menu(bool* open, Game* game) {
    // compute total window height from panel sizes before drawing anything
    const float btn_size = 24.0f;
    const float btn_margin = 6.0f;
    const float topbar_height = btn_size + btn_margin * 2.0f;

    // compute panel heights without rendering (panels return bottom y)
    // popper panel height
    {
        Font font = *font_get(FONT_SIZE_PANEL);
        const float panel_padding = 14.0f;
        const float large_slot_size = 80.0f;
        const float title_area_height = font.baseSize + 12.0f;
        const float popper_height = panel_padding + title_area_height + 8.0f + large_slot_size + panel_padding;

        // crafter panel height
        const float small_slot_size = 42.0f;
        const float slot_gap = 6.0f;
        const int grid_cols = 3;
        const int grid_rows = 2;
        const float grid_height = grid_rows * small_slot_size + (grid_rows - 1) * slot_gap;
        const float action_btn_height = 34.0f;
        const float action_area_height = action_btn_height + 14.0f;
        const float energy_row_height = font.baseSize + 10.0f;
        const float crafter_height = panel_padding + (font.baseSize + 12.0f) + 8.0f + grid_height + energy_row_height + action_area_height + panel_padding;

        float window_height = topbar_height + popper_height + crafter_height;

        // draw background and topbar first
        render_factory_menu_base_init(open, window_height);

        // draw panels (no gaps)
        float y = FACTORY_MENU_Y0 + topbar_height;
        y = render_item_popper_panel(y, &factory_state);
        render_item_crafter_panel(y, &factory_state, game);

        // draw topbar and outline on top
        render_factory_menu_base_end(open, window_height);
    }
}

// ── Home UI ──────────────────────────────────────────────────────────────────
// Always-visible UI layer: bottom action buttons + top-right status bar.

// Renders one status icon + numeric value at (x, y).
// Returns the x coordinate to use for the next (leftward) item.
static float render_stat_icon(Texture2D icon, Color tint, const char* value_str,
                               float x, float y, float icon_size,
                               const char* tooltip_text)
{
    Font* font   = font_get(FONT_SIZE_PANEL);
    Vector2 tsz  = MeasureTextEx(*font, value_str, font->baseSize, 0);
    const float gap     = 6.0f;
    const float spacing = 14.0f;
    float item_w = icon_size + gap + tsz.x;
    float ix = x - item_w;

    DrawTexturePro(icon,
        (Rectangle){0, 0, (float)icon.width, (float)icon.height},
        (Rectangle){ix, y, icon_size, icon_size},
        (Vector2){0, 0}, 0.0f, tint);
    DrawTextEx(*font, value_str,
        (Vector2){ix + icon_size + gap, y + floorf((icon_size - font->baseSize) * 0.5f)},
        font->baseSize, 0, tint);

    // tooltip hover region
    Rectangle hover_rect = (Rectangle){ix, y, item_w, icon_size};
    if (CheckCollisionPointRec(GetMousePosition(), hover_rect)) {
        Vector2 cur = GetMousePosition();
        ui_text_tooltip = (UiTextTooltip){ true, tooltip_text, cur.x, cur.y };
    }

    return ix - spacing;
}

static void render_home_ui(Game* game, bool* factory_menu_open)
{
    // ── Bottom action buttons ─────────────────────────────────────────────────
    const float btn_radius = 36.0f;
    const float btn_margin = 14.0f;
    const float btn_y = WINDOW_HEIGHT - btn_radius - btn_margin;

    // Two buttons right-to-left: Next Day, Factory
    const float next_day_x = WINDOW_WIDTH - btn_radius - btn_margin;
    const float factory_x  = next_day_x   - btn_radius * 2.0f - btn_margin;

    const Color factory_color  = CLITERAL(Color){ 140, 30,  30,  255 };
    const Color next_day_color = ui_palette.colors[PALETTE_DARK];

    // factory button — opens factory menu
    if (ho_button_circle_icon_label((Vector2){factory_x, btn_y}, btn_radius, tex_factory, "FACTORY", !(*factory_menu_open), factory_color, game->animation_timer) & HOUI_INTERACT_CLICKED)
    {
        *factory_menu_open = true;
        play_random_pitch(sounds.click, 0.1f);
    }

    // next day button — always enabled; distributes stored energy to houses on click
    if (ho_button_circle_icon_label((Vector2){next_day_x, btn_y}, btn_radius, tex_moon, "NEXT DAY", true, next_day_color, game->animation_timer) & HOUI_INTERACT_CLICKED) {
        // Collect indices of filled buildings with positive energy demand (center-out order).
        int indices[CITY_GRID * CITY_GRID];
        int count = 0;
        for (int i = 0; i < CITY_GRID * CITY_GRID; i++) {
            int idx = city_fill_order(i);
            int r = idx / CITY_GRID, c = idx % CITY_GRID;
            if (game->city[r][c].filled && game->city[r][c].needed_energy > 0.0f)
                indices[count++] = idx;
        }
        // Insertion sort by needed_energy ascending — pay cheapest houses first
        for (int a = 1; a < count; a++) {
            int key = indices[a];
            float key_e = game->city[key / CITY_GRID][key % CITY_GRID].needed_energy;
            int b = a - 1;
            while (b >= 0) {
                float e = game->city[indices[b] / CITY_GRID][indices[b] % CITY_GRID].needed_energy;
                if (e <= key_e) break;
                indices[b + 1] = indices[b];
                b--;
            }
            indices[b + 1] = key;
        }
        // Distribute stored energy
        for (int k = 0; k < count; k++) {
            int r = indices[k] / CITY_GRID, c = indices[k] % CITY_GRID;
            City_Building* b = &game->city[r][c];
            if (game->stored_energy >= b->needed_energy) {
                game->stored_energy -= b->needed_energy;
                b->needed_energy = 0.0f;
                b->days_without_energy = 0;
            } else {
                // Partial payment: drain whatever remains into this house's need
                if (game->stored_energy > 0.0f) {
                    b->needed_energy -= game->stored_energy;
                    game->stored_energy = 0.0f;
                }
                b->days_without_energy++;
            }
        }
        // Determine if any building went unmet this turn, and remove those stuck for DAYS_UNTIL_EVICTION+ days.
        // At most MAXIMUM_BUILDINGS_TO_BE_REMOVED evictions per day.
        bool any_unmet = false;
        int removed = 0;
        for (int i = 0; i < CITY_GRID * CITY_GRID; i++) {
            int idx = city_evict_order(i);
            int r = idx / CITY_GRID, c = idx % CITY_GRID;
            City_Building* b = &game->city[r][c];
            if (!b->filled) continue;
            if (b->days_without_energy > 0) {
                any_unmet = true;
                if (b->days_without_energy >= DAYS_UNTIL_EVICTION && removed < MAXIMUM_BUILDINGS_TO_BE_REMOVED) {
                    b->filled = false;
                    b->needed_energy = 0.0f;
                    b->days_without_energy = 0;
                    game->city_size--;
                    removed++;
                }
            }
        }
        play_random_pitch(sounds.click, 0.1f);
        game_next_day(!any_unmet);
    }

    // ── Top-right status bar ──────────────────────────────────────────────────
    const float stat_icon_size = 24.0f;
    const float stat_margin    = 14.0f;
    const float stat_pad       = 10.0f;  // padding inside the background rect
    const float stat_y         = stat_margin;

    // Helpers to measure one stat item width (icon + gap + text)
    Font* stat_font = font_get(FONT_SIZE_PANEL);
    const float gap = 6.0f;
    const float spacing = 14.0f;
    #define STAT_ITEM_W(value_str) \
        (stat_icon_size + gap + MeasureTextEx(*stat_font, (value_str), stat_font->baseSize, 0).x)

    // Pre-compute total bar width to draw the background first
    char str_day[16], str_city[16], str_research[16], str_needed[16], str_stored[16];
    snprintf(str_day,      sizeof(str_day),      "%d",   game->day);
    snprintf(str_city,     sizeof(str_city),     "%d",   game->city_size);
    snprintf(str_research, sizeof(str_research), "%.1f", game->research_points);
    snprintf(str_needed,   sizeof(str_needed),   "%.1f", game->needed_energy);
    snprintf(str_stored,   sizeof(str_stored),   "%.1f", game->stored_energy);

    float total_w = STAT_ITEM_W(str_day) + spacing
                  + STAT_ITEM_W(str_city) + spacing
                  + STAT_ITEM_W(str_research) + spacing
                  + STAT_ITEM_W(str_needed) + spacing
                  + STAT_ITEM_W(str_stored);
    #undef STAT_ITEM_W

    float bg_x = (float)WINDOW_WIDTH - stat_margin - total_w - stat_pad;
    float bg_y = stat_y - stat_pad;
    float bg_w = total_w + stat_pad * 2.0f;
    float bg_h = stat_icon_size + stat_pad * 2.0f;
    DrawRectangleRounded(
        (Rectangle){bg_x, bg_y, bg_w, bg_h},
        0.4f, 8,
        CLITERAL(Color){ 30, 30, 30, 160 });

    // Draw icons left-to-right (render_stat_icon advances right-to-left)
    float sx = (float)WINDOW_WIDTH - stat_margin;
    sx = render_stat_icon(tex_sun,        WHITE,
        str_day,
        sx, stat_y, stat_icon_size, "Days passed");
    sx = render_stat_icon(tex_house,      WHITE,
        str_city,
        sx, stat_y, stat_icon_size, "City size");
    sx = render_stat_icon(tex_research,   CLITERAL(Color){120, 190, 220, 255},
        str_research,
        sx, stat_y, stat_icon_size, TextFormat("x%.2f merge multiplier from research", research_multiplier()));
    sx = render_stat_icon(tex_battery_no, CLITERAL(Color){220, 100, 100, 255},
        str_needed,
        sx, stat_y, stat_icon_size, "City needed energy");
    sx = render_stat_icon(tex_energy,     CLITERAL(Color){240, 220, 100, 255},
        str_stored,
        sx, stat_y, stat_icon_size, "City stored energy, ready to use");
    (void)sx;
}

bool ui_render(const Game* game)
{
    static bool factory_menu_open = false;
    bool capturing = factory_menu_open || ui_drag.active;

    s_current_game  = game;
    // reset daily item-gen counter when the day advances
    if (game->day != s_last_seen_day) {
        s_items_generated_today = 0;
        s_last_seen_day = game->day;
    }

    // reset frame-transient drop target and text tooltip
    ui_drop_target       = (UiDropTarget){0};
    ui_text_tooltip      = (UiTextTooltip){0};

    // ── Home UI (always visible) ─────────────────────────────────────────────
    render_home_ui((Game*)game, &factory_menu_open);

    // ── Module: Factory Menu ─────────────────────────────────────────────────
    if (factory_menu_open) {
        render_factory_menu(&factory_menu_open, (Game*)game);
    }

    // drop resolution — after all slots have registered their hover state
    if (ui_drag.active && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        bool drag_from_popper = (ui_drag.source_slot == SLOT_POPPER);
        bool successfully_placed = false;

        if (ui_drop_target.trash_hovered) {
            // discard item — popper slot already cleared when drag started
            successfully_placed = true;
        } else if (ui_drop_target.any_hovered) {
            FactorySlotId target = ui_drop_target.hovered_slot;
            if (target == ui_drag.source_slot) {
                // dropped back onto its own slot — just return it, no generation
                factory_state.items[target]    = ui_drag.item;
                factory_state.has_item[target] = true;
            } else if (factory_state.has_item[target]) {
                // swap
                Item tmp                              = factory_state.items[target];
                factory_state.items[target]           = ui_drag.item;
                factory_state.has_item[target]        = true;
                factory_state.items[ui_drag.source_slot]    = tmp;
                factory_state.has_item[ui_drag.source_slot] = true;
            } else {
                // move
                factory_state.items[target]    = ui_drag.item;
                factory_state.has_item[target] = true;
                successfully_placed = true;
            }
        } else {
            // return to source slot
            factory_state.items[ui_drag.source_slot]    = ui_drag.item;
            factory_state.has_item[ui_drag.source_slot] = true;
        }
        ui_drag.active = false;

        // auto-generate a new item in the popper slot when one was moved out
        if (drag_from_popper && successfully_placed && s_items_generated_today < ITEM_GENERATES_PER_DAY) {
            factory_state.items[SLOT_POPPER]    = item_generate();
            factory_state.has_item[SLOT_POPPER] = true;
            s_items_generated_today++;
        }
    }

    // draw drag ghost centered on cursor (above everything)
    if (ui_drag.active)
    {
        const float ghost_size = 40.0f;
        Vector2 cursor = GetMousePosition();
        item_render(&ui_drag.item,
            cursor.x - ghost_size * 0.5f,
            cursor.y - ghost_size * 0.5f,
            ghost_size, false);
    }

    // flush deferred item tooltip on top of everything
    if (ui_tooltip.active)
    {
        render_item_description_panel(ui_tooltip.item, ui_tooltip.x, ui_tooltip.y);
        ui_tooltip.active = false;
    }

    // flush deferred text tooltip (item tooltip takes priority)
    if (!ui_tooltip.active && ui_text_tooltip.active)
    {
        render_text_tooltip_panel(ui_text_tooltip.text, ui_text_tooltip.x + 12.0f, ui_text_tooltip.y + 4.0f);
        ui_text_tooltip.active = false;
    }

    // flush deferred building tooltip (lowest priority)
    if (!ui_tooltip.active && !ui_text_tooltip.active && ui_building_tooltip.active)
    {
        render_building_tooltip_panel(ui_building_tooltip.building, ui_building_tooltip.x, ui_building_tooltip.y);
        ui_building_tooltip.active = false;
    }

    return capturing;
}