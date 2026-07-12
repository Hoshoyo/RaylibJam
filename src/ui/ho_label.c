#include "hui.h"

HoUiInteraction
ho_label(Vector2 position, Vector2 size, Font font, const char* text)
{
    HoUiInteraction result = {0};

    Rectangle clipping = (Rectangle){position.x, position.y, size.x, size.y};

    // Color scheme
    Color color_text = ui_palette.colors[PALETTE_LIGHT];
    Color color_pop = ui_palette.colors[PALETTE_POP];

    if(CheckCollisionPointRec(GetMousePosition(), clipping))
    {
        result |= HOUI_INTERACT_HOVERED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            result |= HOUI_INTERACT_CLICKED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    bool hovered = (result & HOUI_INTERACT_HOVERED) != 0;

    Vector2 text_size = MeasureTextEx(font, text, font.baseSize, 0);
    position.y += floorf((size.y / 2.0f) - (text_size.y / 2.0f));

    BeginScissorMode(clipping.x, clipping.y, clipping.width, clipping.height);
    DrawTextEx(font, text, position, font.baseSize, 0, (hovered) ? color_pop : color_text);
    EndScissorMode();

    return result;
}
