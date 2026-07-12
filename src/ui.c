#include "ui.h"

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
static Texture2D tex_trash;
static Texture2D tex_factory;
static Texture2D tex_research;
static Texture2D tex_energy;
static Texture2D tex_merge;

void ui_init() {
    ui_palette = palette_mountain_ridge;
    tex_close     = LoadTexture("res/ui/close.png");
    tex_place_item = LoadTexture("res/ui/place_item.png");
    tex_new       = LoadTexture("res/ui/new.png");
    tex_trash     = LoadTexture("res/ui/trash.png");
    tex_factory   = LoadTexture("res/ui/factory.png");
    tex_research  = LoadTexture("res/ui/research.png");
    tex_energy    = LoadTexture("res/ui/battery.png");
    tex_merge     = LoadTexture("res/ui/merge.png");
}

// crafted energy — computed each frame when grid is full, -1 otherwise.
static float s_crafted_energy = -1.0f;

// circle button with icon shifted up + label text at the bottom
static HoUiInteraction ho_button_circle_icon_label(Vector2 center, float radius, Texture2D icon, const char* label, bool interactive) {
    HoUiInteraction result = {0};

    Color color_background = ui_palette.colors[PALETTE_DARK];
    Color color_text       = ui_palette.colors[PALETTE_LIGHT];
    Color color_pop        = ui_palette.colors[PALETTE_POP];
    Color color_border     = ColorBrightness(color_background, 0.1f);
    const float border_width = 2.0f;

    if (interactive && CheckCollisionPointCircle(GetMousePosition(), center, radius)) {
        result |= HOUI_INTERACT_HOVERED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  result |= HOUI_INTERACT_CLICKED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    bool hovered = (result & HOUI_INTERACT_HOVERED) != 0;
    DrawCircleV(center, radius + border_width, color_border);
    DrawCircleV(center, radius, hovered ? ColorBrightness(color_background, 0.3f) : color_background);

    // icon: occupy upper ~55% of the circle, shifted up
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

    // label: centered horizontally, slightly toward center of button
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
// locked=true: occupied slots cannot be dragged away; empty slots show a lighter bg.
static void draw_item_slot(Rectangle rect, FactorySlotId id, FactoryMenuState* state, bool locked)
{
    bool is_drag_source = ui_drag.active && ui_drag.source_slot == id;
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);

    if (locked && !state->has_item[id])
        draw_sunken_slot_ex(rect, CLITERAL(Color){ 178, 178, 178, 255 });
    else if (!state->has_item[id])
        draw_sunken_slot_ex(rect, CLITERAL(Color){ 178, 178, 178, 255 });
    else
        draw_sunken_slot(rect);

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

    // Begin drag on click — blocked for locked occupied slots
    if (!locked && !ui_drag.active && hovered && state->has_item[id]
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
    const float padding    = 8.0f;
    const float panel_w    = 220.0f;
    const float text_w     = panel_w - padding * 2.0f;
    Font* font             = font_get(FONT_SIZE_BODY);
    const float line_h     = font->baseSize + 5.0f;

    const Color bg        = CLITERAL(Color){  30,  30,  30, 230 };
    const Color fg        = CLITERAL(Color){ 220, 220, 220, 255 };
    const Color brd       = CLITERAL(Color){  80,  80,  80, 255 };
    const Color eff_color = CLITERAL(Color){ 220,  60,  60, 255 };

    // Pre-compute wrapped effect lines so we know the panel height upfront.
    const char* effect_desc = grid_effect_description(item_info(item->id)->effect);
    char eff_lines[6][256];
    int  eff_line_count = text_wrap_lines(*font, effect_desc, text_w, eff_lines, 6);

    const float panel_h = padding + line_h * 2.0f + line_h * eff_line_count + padding;

    DrawRectangle((int)x, (int)y, (int)panel_w, (int)panel_h, bg);
    DrawRectangleLinesEx((Rectangle){x, y, panel_w, panel_h}, 1.0f, brd);

    float tx = x + padding;
    float ty = y + padding;

    // Name — colored by quality tier
    Color name_color;
    if      (item->quality < 1.5f) name_color = CLITERAL(Color){ 160, 160, 160, 255 };
    else if (item->quality < 3.0f) name_color = CLITERAL(Color){  80, 200,  80, 255 };
    else if (item->quality < 4.0f) name_color = CLITERAL(Color){  30, 180,  30, 255 };
    else                           name_color = CLITERAL(Color){ 220,  60,  60, 255 };
    DrawTextEx(*font, TextToUpper(item->name), (Vector2){tx, ty}, font->baseSize, 0, name_color);
    ty += line_h;

    DrawTextEx(*font, TextFormat("QUALITY: %.2f", item->quality), (Vector2){tx, ty}, font->baseSize, 0, fg);
    ty += line_h;

    // Effect description — word-wrapped, red
    for (int i = 0; i < eff_line_count; i++) {
        DrawTextEx(*font, eff_lines[i], (Vector2){tx, ty}, font->baseSize, 0, eff_color);
        ty += line_h;
    }
}

// Renders a plain text tooltip at (x, y). Word-wraps to fit panel_w.
static void render_text_tooltip_panel(const char* text, float x, float y)
{
    const float padding = 8.0f;
    const float panel_w = 200.0f;
    const float text_w  = panel_w - padding * 2.0f;
    Font* font          = font_get(FONT_SIZE_BODY);
    const float line_h  = font->baseSize + 5.0f;

    const Color bg  = CLITERAL(Color){  30,  30,  30, 230 };
    const Color fg  = CLITERAL(Color){ 220, 220, 220, 255 };
    const Color brd = CLITERAL(Color){  80,  80,  80, 255 };

    char lines[6][256];
    int  line_count = text_wrap_lines(*font, text, text_w, lines, 6);

    const float panel_h = padding + line_h * line_count + padding;

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

static float render_item_popper_panel(float start_y, FactoryMenuState* state) {
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

    // new item button — generates a random item into the popper slot
    if (ho_button_circle_texture((Vector2){content_x + btn_radius, center_y}, btn_radius, tex_new) & HOUI_INTERACT_CLICKED)
    {
        state->items[SLOT_POPPER]    = item_generate();
        state->has_item[SLOT_POPPER] = true;
    }

    // large slot
    float slot_x = content_x + btn_radius * 2.0f + section_gap;
    float slot_y = content_y + floorf((content_height - large_slot_size) * 0.5f);
    draw_item_slot((Rectangle){slot_x, slot_y, large_slot_size, large_slot_size}, SLOT_POPPER, state, false);

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

static float render_item_crafter_panel(float start_y, FactoryMenuState* state) {
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

    // determine lock state and crafted energy
    bool grid_locked = false;
    for (int i = SLOT_CRAFTER_0; i <= SLOT_CRAFTER_5; ++i)
        if (state->has_item[i]) { grid_locked = true; break; }

    Grid grid = {0};
    for (int r = 0; r < GRID_ROWS; ++r)
        for (int c = 0; c < GRID_COLS; ++c) {
            FactorySlotId sid = (FactorySlotId)(SLOT_CRAFTER_0 + r * GRID_COLS + c);
            grid.has_item[r][c] = state->has_item[sid];
            grid.items[r][c]    = state->items[sid];
        }
    bool grid_full = grid_is_full(&grid);
    s_crafted_energy = grid_full ? grid_compute_quality(&grid) : -1.0f;

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
            draw_item_slot(slot, slot_id, state, grid_locked);
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
    const float btns_total_width = action_btn_width * 3.0f + action_btn_gap * 2.0f;
    const float btns_x = panel_x + floorf((panel_width - btns_total_width) * 0.5f);
    const float btns_y = start_y + panel_height - panel_padding - action_btn_height;
    const float icon_size = 26.0f;

    // Generate Energy (dark yellow)
    {
        HoUiInteraction inter = ho_button_icon_label(
            (Rectangle){btns_x, btns_y, action_btn_width, action_btn_height},
            tex_energy, icon_size, body_font, "GENERATE ENERGY", (Color){160,120,20,255}, grid_full);
        if (inter & HOUI_INTERACT_CLICKED)
            TraceLog(LOG_INFO, "Grid quality: %.4f", s_crafted_energy);
        if (inter & HOUI_INTERACT_HOVERED) {
            Vector2 cur = GetMousePosition();
            ui_text_tooltip = (UiTextTooltip){ true, "Energize the city", cur.x + 12.0f, cur.y + 4.0f };
        }
    }
    // Send to Research (light blue)
    {
        HoUiInteraction inter = ho_button_icon_label(
            (Rectangle){btns_x + action_btn_width + action_btn_gap, btns_y, action_btn_width, action_btn_height},
            tex_research, icon_size, body_font, "SEND TO RESEARCH", (Color){60,130,170,255}, grid_full);
        if (inter & HOUI_INTERACT_HOVERED) {
            Vector2 cur = GetMousePosition();
            ui_text_tooltip = (UiTextTooltip){ true, "Energize the research facility to get better quality items", cur.x + 12.0f, cur.y + 4.0f };
        }
    }
    // Merge (dark red)
    {
        HoUiInteraction inter = ho_button_icon_label(
            (Rectangle){btns_x + (action_btn_width + action_btn_gap) * 2.0f, btns_y, action_btn_width, action_btn_height},
            tex_merge, icon_size, body_font, "MERGE", (Color){140,30,30,255}, grid_full);
        if (inter & HOUI_INTERACT_HOVERED) {
            Vector2 cur = GetMousePosition();
            ui_text_tooltip = (UiTextTooltip){ true, "Merge into item of equivalent quality", cur.x + 12.0f, cur.y + 4.0f };
        }
    }

    return start_y + panel_height;
}

static void render_factory_menu(bool* open) {
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
        render_item_crafter_panel(y, &factory_state);

        // draw topbar and outline on top
        render_factory_menu_base_end(open, window_height);
    }
}

bool ui_render()
{
    static bool factory_menu_open = false;
    bool capturing = factory_menu_open || ui_drag.active;

    // reset frame-transient drop target
    ui_drop_target = (UiDropTarget){0};

    // top-right buttons (factory + research)
    const float btn_radius = 36.0f;
    const float btn_margin = 14.0f;
    const float btn_y = WINDOW_HEIGHT - btn_radius - btn_margin;
    const float research_x = WINDOW_WIDTH - btn_radius - btn_margin;
    const float factory_x  = research_x - btn_radius * 2.0f - btn_margin;

    // factory button — opens factory menu
    if (ho_button_circle_icon_label((Vector2){factory_x, btn_y}, btn_radius, tex_factory, "FACTORY", !factory_menu_open) & HOUI_INTERACT_CLICKED)
    {
        factory_menu_open = true;
    }

    // research button (placeholder)
    ho_button_circle_icon_label((Vector2){research_x, btn_y}, btn_radius, tex_research, "RESEARCH", true);

    if (factory_menu_open) {
        render_factory_menu(&factory_menu_open);
    }

    // drop resolution — after all slots have registered their hover state
    if (ui_drag.active && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        if (ui_drop_target.trash_hovered) {
            // discard item
        } else if (ui_drop_target.any_hovered) {
            FactorySlotId target = ui_drop_target.hovered_slot;
            if (factory_state.has_item[target]) {
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
            }
        } else {
            // return to source slot
            factory_state.items[ui_drag.source_slot]    = ui_drag.item;
            factory_state.has_item[ui_drag.source_slot] = true;
        }
        ui_drag.active = false;
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
        render_text_tooltip_panel(ui_text_tooltip.text, ui_text_tooltip.x, ui_text_tooltip.y);
        ui_text_tooltip.active = false;
    }

    return capturing;
}