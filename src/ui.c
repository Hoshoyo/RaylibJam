#include "ui.h"

#include "game.h"
#include "item.h"
#include "font.h"
#include "ui/hui.h"
static ColorPalette ui_palette;
#include "ui/ho_button.c"
#include "ui/ho_label.c"
#include "ui/ho_slider.c"

#define FACTORY_MENU_X0 30.0f
#define FACTORY_MENU_Y0 30.0f
#define FACTORY_MENU_X1 (WINDOW_WIDTH - 30.0f)
#define FACTORY_MENU_Y1 (WINDOW_WIDTH - 30.0f)

// Deferred tooltip — set during the frame, flushed at the very end of ui_render.
typedef struct {
    bool        active;
    const Item* item;
    float       x;
    float       y;
} UiTooltip;
static UiTooltip ui_tooltip;

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

static void draw_sunken_slot(Rectangle rect) {
    const float bevel = 2.0f;
    // slot fill
    DrawRectangleRec(rect, CLITERAL(Color){ 148, 148, 148, 255 });
    // top & left shadow edges
    DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width, (int)bevel, CLITERAL(Color){ 85, 85, 85, 255 });
    DrawRectangle((int)rect.x, (int)rect.y, (int)bevel, (int)rect.height, CLITERAL(Color){ 85, 85, 85, 255 });
    // bottom & right highlight edges
    DrawRectangle((int)rect.x, (int)(rect.y + rect.height - bevel), (int)rect.width, (int)bevel, CLITERAL(Color){ 205, 205, 205, 255 });
    DrawRectangle((int)(rect.x + rect.width - bevel), (int)rect.y, (int)bevel, (int)rect.height, CLITERAL(Color){ 205, 205, 205, 255 });
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

// Renders an item description tooltip at (x, y) — top-left corner.
// Uses a plain raised panel with dark background.
static void render_item_description_panel(const Item* item, float x, float y)
{
    const float padding    = 8.0f;
    const float panel_w    = 140.0f;
    Font* font             = font_get(FONT_SIZE_BODY);
    const float line_h     = font->baseSize + 5.0f;
    const float panel_h    = padding + line_h * 3.0f + padding;

    const Color bg  = CLITERAL(Color){  30,  30,  30, 230 };
    const Color fg  = CLITERAL(Color){ 220, 220, 220, 255 };
    const Color brd = CLITERAL(Color){  80,  80,  80, 255 };

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

    DrawTextEx(*font, "PLACEMENT:", (Vector2){tx, ty}, font->baseSize, 0, fg);
}

static float render_item_popper_panel(float start_y, bool* has_item, Item* slot_item) {
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

    // new item button — generates a random item into the slot
    if (ho_button_circle_texture((Vector2){content_x + btn_radius, center_y}, btn_radius, tex_new) & HOUI_INTERACT_CLICKED)
    {
        *slot_item = item_generate();
        *has_item = true;
    }

    // large slot
    float slot_x = content_x + btn_radius * 2.0f + section_gap;
    float slot_y = content_y + floorf((content_height - large_slot_size) * 0.5f);
    Rectangle large_slot = (Rectangle){slot_x, slot_y, large_slot_size, large_slot_size};
    draw_sunken_slot(large_slot);
    if (*has_item)
    {
        const float item_size = large_slot_size - 16.0f;
        float item_x = slot_x + floorf((large_slot_size - item_size) * 0.5f);
        float item_y = slot_y + floorf((large_slot_size - item_size) * 0.5f);
        item_render(slot_item, item_x, item_y, item_size, false);
    }

    // tooltip: defer to end of frame so it renders on top of everything
    if (*has_item && CheckCollisionPointRec(GetMousePosition(), large_slot))
    {
        Vector2 cursor = GetMousePosition();
        ui_tooltip.active = true;
        ui_tooltip.item   = slot_item;
        ui_tooltip.x      = cursor.x + 12.0f;
        ui_tooltip.y      = cursor.y + 4.0f;
    }

    // trash button — clears the slot
    float trash_x = slot_x + large_slot_size + section_gap + btn_radius;
    if (ho_button_circle_texture((Vector2){trash_x, center_y}, btn_radius, tex_trash) & HOUI_INTERACT_CLICKED)
    {
        *has_item = false;
    }

    return start_y + panel_height;
}

static float render_item_crafter_panel(float start_y) {
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
    const float title_font_size = font.baseSize;
    const float title_area_height = title_font_size + 12.0f;

    const float panel_x = window_x0;
    const float panel_width = window_x1 - window_x0;
    const float panel_height = panel_padding + title_area_height + 8.0f + grid_height + action_area_height + panel_padding;

    // light green panel
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
            draw_sunken_slot(slot);
            DrawTexturePro(tex_close,
                (Rectangle){0, 0, tex_close.width, tex_close.height},
                slot, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }

    // bottom action buttons centered
    const float action_btn_width = 130.0f;
    const float action_btn_gap = 10.0f;
    const float btns_total_width = action_btn_width * 3.0f + action_btn_gap * 2.0f;
    const float btns_x = panel_x + floorf((panel_width - btns_total_width) * 0.5f);
    const float btns_y = start_y + panel_height - panel_padding - action_btn_height;
    const float icon_size = 26.0f;
    const float icon_text_gap = 4.0f;

    // Generate Energy (dark yellow)
    ho_button_icon_label(
        (Rectangle){btns_x, btns_y, action_btn_width, action_btn_height},
        tex_energy, icon_size, *font_get(FONT_SIZE_BODY), "GENERATE ENERGY", (Color){160,120,20,255});
    // Send to Research (light blue)
    ho_button_icon_label(
        (Rectangle){btns_x + action_btn_width + action_btn_gap, btns_y, action_btn_width, action_btn_height},
        tex_research, icon_size, *font_get(FONT_SIZE_BODY), "SEND TO RESEARCH", (Color){60,130,170,255});
    // Merge (dark red)
    ho_button_icon_label(
        (Rectangle){btns_x + (action_btn_width + action_btn_gap) * 2.0f, btns_y, action_btn_width, action_btn_height},
        tex_merge, icon_size, *font_get(FONT_SIZE_BODY), "MERGE", (Color){140,30,30,255});

    return start_y + panel_height;
}

static void render_factory_menu(bool* open) {
    static bool popper_has_item = false;
    static Item popper_slot_item;

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
        const float crafter_height = panel_padding + (font.baseSize + 12.0f) + 8.0f + grid_height + action_area_height + panel_padding;

        float window_height = topbar_height + popper_height + crafter_height;

        // draw background and topbar first
        render_factory_menu_base_init(open, window_height);

        // draw panels (no gaps)
        float y = FACTORY_MENU_Y0 + topbar_height;
        y = render_item_popper_panel(y, &popper_has_item, &popper_slot_item);
        render_item_crafter_panel(y);

        // draw topbar and outline on top
        render_factory_menu_base_end(open, window_height);
    }
}

void ui_render()
{
    static bool factory_menu_open = false;

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

    // flush deferred tooltip on top of everything
    if (ui_tooltip.active)
    {
        render_item_description_panel(ui_tooltip.item, ui_tooltip.x, ui_tooltip.y);
        ui_tooltip.active = false;
    }
}