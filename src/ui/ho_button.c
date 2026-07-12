#define HO_UNICODE_IMPLEMENTATION
#include "hounicode.h"
#include "hui.h"

HoUiInteraction
ho_button(Vector2 position, Vector2 size, Font font, const char* text)
{
    HoUiInteraction result = {0};

    Rectangle clipping = (Rectangle){position.x, position.y, size.x, size.y};

    // Color scheme
    
    Color color_background = ui_palette.colors[PALETTE_DARK];
    Color color_text = ui_palette.colors[PALETTE_LIGHT];
    Color color_pop = ui_palette.colors[PALETTE_POP];
    Color color_border = ColorBrightness(color_background, 0.1f);

    Vector2 border_size = (Vector2){2.0f, 2.0f};

    if(CheckCollisionPointRec(GetMousePosition(), clipping))
    {
        result |= HOUI_INTERACT_HOVERED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            result |= HOUI_INTERACT_CLICKED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    bool hovered = (result & HOUI_INTERACT_HOVERED) != 0;
    DrawRectangleV(position, size, color_border);
    DrawRectangleV(Vector2Add(position, border_size), Vector2Subtract(size, Vector2Scale(border_size, 2.0f)), (hovered) ? color_pop : color_background);

    Vector2 text_size = MeasureTextEx(font, text, font.baseSize, 0);
    
    // center
    position.x += floorf(size.x / 2.0f - text_size.x / 2.0f);
    position.y += floorf(size.y / 2.0f - text_size.y / 2.0f);

    DrawTextEx(font, text, position, font.baseSize, 0, (hovered) ? color_background : color_text);

    return result;
}

HoUiInteraction
ho_button_circle(Vector2 center, float radius, Font font, const char* text, bool interactive)
{
    HoUiInteraction result = {0};

    // Color scheme
    Color color_background = ui_palette.colors[PALETTE_DARK];
    Color color_text = ui_palette.colors[PALETTE_LIGHT];
    Color color_pop = ui_palette.colors[PALETTE_POP];
    Color color_border = ColorBrightness(color_background, 0.1f);

    float border_width = 2.0f;

    if(interactive && CheckCollisionPointCircle(GetMousePosition(), center, radius))
    {
        result |= HOUI_INTERACT_HOVERED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            result |= HOUI_INTERACT_CLICKED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    bool hovered = (result & HOUI_INTERACT_HOVERED) != 0;
    DrawCircleV(center, radius + border_width, color_border);
    DrawCircleV(center, radius, (hovered) ? color_pop : color_background);

    Vector2 text_size = MeasureTextEx(font, text, font.baseSize, 0);

    Vector2 text_pos;
    text_pos.x = center.x + floorf(-text_size.x / 2.0f);
    text_pos.y = center.y + floorf(-text_size.y / 2.0f);

    DrawTextEx(font, text, text_pos, font.baseSize, 0, (hovered) ? color_background : color_text);

    return result;
}

HoUiInteraction
ho_button_circle_texture(Vector2 center, float radius, Texture2D texture)
{
    HoUiInteraction result = {0};

    Color color_background = ui_palette.colors[PALETTE_DARK];
    Color color_border = ColorBrightness(color_background, 0.1f);

    float border_width = 2.0f;

    if(CheckCollisionPointCircle(GetMousePosition(), center, radius))
    {
        result |= HOUI_INTERACT_HOVERED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            result |= HOUI_INTERACT_CLICKED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    bool hovered = (result & HOUI_INTERACT_HOVERED) != 0;
    DrawCircleV(center, radius + border_width, color_border);
    DrawCircleV(center, radius, (hovered) ? ColorBrightness(color_background, 0.3f) : color_background);

    // texture centered inside the circle
    float tex_size = radius * 1.2f;
    Rectangle dest = (Rectangle){
        center.x - tex_size / 2.0f,
        center.y - tex_size / 2.0f,
        tex_size, tex_size
    };
    DrawTexturePro(texture,
        (Rectangle){0, 0, texture.width, texture.height},
        dest,
        (Vector2){0, 0}, 0.0f,
        WHITE);

    return result;
}

static HoUiInteraction
ho_button_circle_texture_disabled(Vector2 center, float radius, Texture2D texture)
{
    HoUiInteraction result = {0};

    if (CheckCollisionPointCircle(GetMousePosition(), center, radius))
        result |= HOUI_INTERACT_HOVERED;

    DrawCircleV(center, radius + 2.0f, CLITERAL(Color){ 60,  60,  60, 255 });
    DrawCircleV(center, radius,        CLITERAL(Color){ 90,  90,  90, 255 });

    float tex_size = radius * 1.2f;
    DrawTexturePro(texture,
        (Rectangle){0, 0, texture.width, texture.height},
        (Rectangle){center.x - tex_size*0.5f, center.y - tex_size*0.5f, tex_size, tex_size},
        (Vector2){0,0}, 0.0f,
        CLITERAL(Color){255, 255, 255, 80});

    return result;
}

// Rectangular button with an icon left-aligned and label centered in remaining space.
// font is used for the label. icon_size controls the icon square size.
// When enabled=false the button is drawn grayed-out and returns no interactions.
HoUiInteraction
ho_button_icon_label(Rectangle rect, Texture2D icon, float icon_size, Font font, const char* label, Color base_color, bool enabled)
{
    HoUiInteraction result = {0};

    if (!enabled) {
        const int bw = 2;
        Color dis_bg = CLITERAL(Color){ 90, 90, 90, 255 };
        DrawRectangleRec(rect, dis_bg);
        DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width, bw, CLITERAL(Color){130,130,130,255});
        DrawRectangle((int)rect.x, (int)rect.y, bw, (int)rect.height, CLITERAL(Color){130,130,130,255});
        DrawRectangle((int)rect.x, (int)(rect.y+rect.height-bw), (int)rect.width, bw, CLITERAL(Color){60,60,60,255});
        DrawRectangle((int)(rect.x+rect.width-bw), (int)rect.y, bw, (int)rect.height, CLITERAL(Color){60,60,60,255});
        const float icon_margin = 6.0f;
        DrawTexturePro(icon,
            (Rectangle){0, 0, (float)icon.width, (float)icon.height},
            (Rectangle){rect.x+icon_margin, rect.y+floorf((rect.height-icon_size)*0.5f), icon_size, icon_size},
            (Vector2){0,0}, 0.0f, CLITERAL(Color){255,255,255,80});
        Vector2 sz = MeasureTextEx(font, label, font.baseSize, 0);
        float text_x = rect.x + floorf((rect.width - sz.x) * 0.5f) + (icon_size + icon_margin) * 0.5f;
        float text_y = rect.y + floorf((rect.height - sz.y) * 0.5f);
        DrawTextEx(font, label, (Vector2){text_x, text_y}, font.baseSize, 0, CLITERAL(Color){130,130,130,255});
        return result;
    }

    bool hov     = CheckCollisionPointRec(GetMousePosition(), rect);
    bool pressed = hov && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (hov) {
        result |= HOUI_INTERACT_HOVERED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  result |= HOUI_INTERACT_CLICKED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    // background
    DrawRectangleRec(rect, hov ? ColorBrightness(base_color, 0.25f) : base_color);

    // bevel: raised normally, inverted when pressed
    Color bevel_lt = pressed ? (Color){80,80,80,255}   : (Color){210,210,210,255};
    Color bevel_rb = pressed ? (Color){210,210,210,255} : (Color){80,80,80,255};
    DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width, 2, bevel_lt);
    DrawRectangle((int)rect.x, (int)rect.y, 2, (int)rect.height, bevel_lt);
    DrawRectangle((int)rect.x, (int)(rect.y + rect.height - 2), (int)rect.width, 2, bevel_rb);
    DrawRectangle((int)(rect.x + rect.width - 2), (int)rect.y, 2, (int)rect.height, bevel_rb);

    // icon: left-aligned, vertically centered
    const float icon_margin = 6.0f;
    float icon_x = rect.x + icon_margin;
    float icon_y = rect.y + floorf((rect.height - icon_size) * 0.5f);
    DrawTexturePro(icon,
        (Rectangle){0, 0, icon.width, icon.height},
        (Rectangle){icon_x, icon_y, icon_size, icon_size},
        (Vector2){0, 0}, 0.0f, WHITE);

    // label: centered in remaining space to the right of the icon
    Vector2 sz = MeasureTextEx(font, label, font.baseSize, 0);
    float text_x = rect.x + floorf((rect.width - sz.x) * 0.5f) + (icon_size + icon_margin) * 0.5f;
    float text_y = rect.y + floorf((rect.height - sz.y) * 0.5f);
    DrawTextEx(font, label, (Vector2){text_x, text_y}, font.baseSize, 0, WHITE);    

    return result;
}

HoUiInteraction
ho_combo_box(Vector2 position, bool active, Vector2 size, Font font, const char** options, int option_count, int* selected_index)
{
    Vector2 original_size = size;

    Color color_background = ui_palette.colors[PALETTE_DARK];

    size.x -= 30.0f;
    ho_button(position, size, font, options[*selected_index]);

    Vector2 arrow_pos = position;
    arrow_pos.x += size.x;
    HoUiInteraction arrow_interaction = ho_button(arrow_pos, (Vector2){30.0f, size.y}, font, "v");
    
    if(active)
    {
        float combo_height = original_size.y * option_count;
        DrawRectangleV(Vector2Add(position, (Vector2){0, original_size.y}), (Vector2){original_size.x, combo_height}, color_background);

        for(int opt = 0; opt < option_count; ++opt)
        {
            if(ho_label(Vector2Add(position, (Vector2){0, (opt + 1) * original_size.y}), original_size, font, options[opt]) & HOUI_INTERACT_CLICKED)
            {
                *selected_index = opt;
                arrow_interaction |= HOUI_INTERACT_CLICKED;
            }
        }
    }

    return arrow_interaction;
}