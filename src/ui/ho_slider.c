#include "hui.h"

// size: width and height in px
HoUiInteraction
ho_slider_circle(Vector2 position, bool active, Vector2 size, float* value, float min, float max)
{
    HoUiInteraction result = {0};

    Color color_light = ui_palette.colors[PALETTE_LIGHT];
    Color color_pop = ui_palette.colors[PALETTE_POP];

    DrawRectangleV(position, size, color_light);

    float percentage = (*value - min) / (max - min);
    float radius = size.y * 4.0f;

    Vector2 value_pos = (Vector2){position.x + size.x * percentage, position.y + 1.0f};

    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointCircle(mouse, value_pos, radius);

    if(hovered)
    {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            active = true;
            result |= HOUI_INTERACT_CLICKED;
        }
    }

    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        active = false;
        result |= HOUI_INTERACT_RELEASED;
    }

    if(active)
    {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            Vector2 v = GetMouseDelta();
            float delta = v.x / size.x;
            float sum = delta * (max - min);
    
            *value = Clamp(*value + sum, min, max);
            if(sum != 0)
                result |= HOUI_INTERACT_EDITED;
        }
    }

    DrawCircleV(value_pos, radius, (active) ? color_pop : color_light);

    return result;
}