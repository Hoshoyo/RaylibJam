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
ho_combo_box(Vector2 position, bool active, Vector2 size, Font font, const char** options, int option_count, int* selected_index)
{
    Vector2 original_size = size;

    Color color_background = ui_palette.colors[PALETTE_DARK];

    size.x -= 30.0f;
    HoUiInteraction result = ho_button(position, size, font, options[*selected_index]);

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