#define HO_UNICODE_IMPLEMENTATION
#include "hounicode.h"
#include "hui.h"

int iho_textbox_cursor_add(HoTextBox* tbox, int val, bool do_selection, char* buffer)
{
    tbox->cursor += val;
    uint32_t advance = ho_utf8_find_previous(buffer + tbox->cursor);
    if(val < 0)
        tbox->cursor -= advance;
    else
        tbox->cursor += advance;

    if(tbox->cursor < 0) tbox->cursor = 0;
    if(tbox->cursor > tbox->length) tbox->cursor = tbox->length;
    if(!(do_selection && IsKeyDown(KEY_LEFT_SHIFT)))
        tbox->selection = tbox->cursor;
    return tbox->cursor;
}

int iho_textbox_insert(HoTextBox* tbox, uint32_t c, char* buffer, uint32_t buffer_length)
{
    char stream[8] = {0};
    uint32_t advance = ho_codepoint_to_utf8(c, stream);

    if(tbox->length + advance -1 >= buffer_length && tbox->selection == tbox->cursor)
        return 0;

    if(tbox->selection != tbox->cursor)
    {
        if(tbox->cursor > tbox->selection) SWAP(tbox->cursor, tbox->selection);
        memcpy(buffer + tbox->cursor, buffer + tbox->selection, tbox->length - tbox->selection);
        tbox->length -= tbox->selection - tbox->cursor;
        tbox->selection = tbox->cursor;
    }

    if(tbox->cursor == tbox->length)
    {
        // Insertion at the end
        for(uint32_t i = 0; i < advance; ++i)
        {
            buffer[tbox->length] = stream[i];
            tbox->length++;
        }
    }
    else
    {
        // Insert in the middle
        memmove(buffer + tbox->cursor + advance, buffer + tbox->cursor, tbox->length - tbox->cursor);
        for(uint32_t i = 0; i < advance; ++i)
        {
            buffer[tbox->cursor + i] = stream[i];
            tbox->length++;
        }
    }
    iho_textbox_cursor_add(tbox, advance, false, buffer);
    return c;
}

void iho_textbox_delete_back(HoTextBox* tbox, char* buffer, int buffer_length)
{
    (void)buffer_length;

    if(tbox->selection != tbox->cursor)
    {
        // Delete selection
        if(tbox->cursor > tbox->selection) SWAP(tbox->cursor, tbox->selection);
        memcpy(buffer + tbox->cursor, buffer + tbox->selection, tbox->length - tbox->selection);
        tbox->length -= tbox->selection - tbox->cursor;
        tbox->selection = tbox->cursor;
    }
    else
    {
        // Delete previous character
        if(tbox->cursor == tbox->length && tbox->cursor > 0)
        {
            // We are at the end of the buffer text
            tbox->cursor--;
            tbox->length--;

            uint32_t advance = ho_utf8_find_previous(buffer + tbox->cursor);
            while(advance > 0 && tbox->cursor > 0 && tbox->length > 0)
            {
                tbox->cursor--;
                tbox->length--;
                advance--;
            }

            tbox->selection = tbox->cursor;
        }
        else if(tbox->cursor > 0)
        {
            // TODO(psv): check boundaries here
            uint32_t advance = ho_utf8_find_previous(buffer + tbox->cursor - 1);

            // At the middle of the text
            memcpy(buffer + tbox->cursor - (1 + advance), buffer + tbox->cursor, tbox->length - tbox->cursor);
            tbox->cursor -= (1 + advance);
            tbox->length -= (1 + advance);
            tbox->selection = tbox->cursor;
        }
    }
}

void iho_textbox_delete_forward(HoTextBox* tbox, char* buffer, int buffer_length)
{
    if(tbox->cursor < tbox->length)
    {
        if(tbox->cursor == tbox->selection)
            iho_textbox_cursor_add(tbox, 1, false, buffer);
        iho_textbox_delete_back(tbox, buffer, buffer_length);
    }
}

static HoUiInteraction
iho_textbox_process_key(HoTextBox* tbox, int key, char* buffer, int buffer_length)
{
    HoUiInteraction result = {0};
    switch(key)
    {
        case KEY_BACKSPACE: {
            iho_textbox_delete_back(tbox, buffer, buffer_length);
            result |= HOUI_INTERACT_EDITED;
        } break;
        case KEY_DELETE: {
            iho_textbox_delete_forward(tbox, buffer, buffer_length);
            result |= HOUI_INTERACT_EDITED;
        } break;
        case KEY_LEFT: {
            if(IsKeyDown(KEY_LEFT_CONTROL))
            {
                while(tbox->cursor && isalpha(buffer[iho_textbox_cursor_add(tbox, -1, true, buffer)]));
            }
            else
            {
                iho_textbox_cursor_add(tbox, -1, true, buffer);
            }
        } break;
        case KEY_RIGHT: {
            if(IsKeyDown(KEY_LEFT_CONTROL))
            {
                bool same = true;
                while(same && tbox->cursor < tbox->length)
                {
                    same = isalnum(buffer[tbox->cursor]);
                    iho_textbox_cursor_add(tbox, 1, true, buffer);
                }
            }
            else
            {
                iho_textbox_cursor_add(tbox, 1, true, buffer);
            }
        } break;
        case KEY_HOME: {
            tbox->cursor = 0;
            if(!IsKeyDown(KEY_LEFT_SHIFT))
                tbox->selection = tbox->cursor;
        } break;
        case KEY_END: {
            tbox->cursor = tbox->length;
            if(!IsKeyDown(KEY_LEFT_SHIFT))
                tbox->selection = tbox->cursor;
        } break;
        case KEY_ESCAPE: {
            tbox->selection = tbox->cursor;
        } break;
        default: break;
    }

    return result;
}

HoUiInteraction
ho_textbox_input(HoTextBox* tbox, char* buffer, int buffer_length)
{
    HoUiInteraction result = {0};

    if(IsKeyPressed(KEY_V) && IsKeyDown(KEY_LEFT_CONTROL))
    {
        const char* text = GetClipboardText();
        while(*text)
        {
            uint32_t codepoint = 0;
            text += ho_codepoint_from_utf8(text, &codepoint);
            iho_textbox_insert(tbox, codepoint, buffer, buffer_length);
            result |= HOUI_INTERACT_EDITED;
        }
    }

    if(IsKeyPressed(KEY_C) && IsKeyDown(KEY_LEFT_CONTROL))
    {
        if(tbox->selection != tbox->cursor)
        {
            const char* txt = TextFormat("%.*s", ABS(tbox->selection - tbox->cursor), buffer + MIN(tbox->selection, tbox->cursor));
            SetClipboardText(txt);
            result |= HOUI_INTERACT_EDITED;
        }
    }

    int c = 0;
    while((c = GetCharPressed()))
    {
        iho_textbox_insert(tbox, c, buffer, buffer_length);
        result |= HOUI_INTERACT_EDITED;
    }
    
    int repeat_keys[] = {KEY_BACKSPACE, KEY_DELETE, KEY_LEFT, KEY_RIGHT};
    for(uint32_t k = 0; k < ARRAY_LENGTH(repeat_keys); ++k)
        if(IsKeyPressedRepeat(repeat_keys[k]))
            result |= iho_textbox_process_key(tbox, repeat_keys[k], buffer, buffer_length);
    
    int d = 0;
    while((d = GetKeyPressed()))
    {
        result |= iho_textbox_process_key(tbox, d, buffer, buffer_length);
    }

    return result;
}

HoUiInteraction
ho_textbox_render(HoTextBox* tbox, bool active, Vector2 position, Vector2 size, Vector2 margin, Font font, const char* buffer)
{
    HoUiInteraction result = {0};

    size = Vector2Add(size, Vector2Scale(margin, 2.0f));
    Rectangle clipping = (Rectangle){position.x, position.y, size.x, size.y};

    // Color scheme
    Color color_background = ui_palette.colors[PALETTE_DARK];
    Color color_text = ui_palette.colors[PALETTE_LIGHT];
    Color color_cursor = color_text;
    color_cursor.a = 0x88;
    Color color_selection = ui_palette.colors[PALETTE_POP];
    color_selection.a = 0x88;
    Color color_border = ColorBrightness(color_background, 0.1f);

    if(CheckCollisionPointRec(GetMousePosition(), clipping))
    {
        result |= HOUI_INTERACT_HOVERED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            result |= HOUI_INTERACT_CLICKED;
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            result |= HOUI_INTERACT_RIGHT_CLICKED;
    }

    DrawRectangleV(position, size, color_background);

    Vector2 text_size = MeasureTextEx(font, TextFormat("%.*s", tbox->cursor, buffer), font.baseSize, 0);
    float excess = text_size.x - 300.0f + font.baseSize / 2.0f;
    if(excess < 0) excess = 0;
    position.x -= excess;

    position = Vector2Add(margin, position);

    BeginScissorMode(clipping.x, clipping.y, clipping.width, clipping.height);
    
    DrawTextEx(font, TextFormat("%.*s", tbox->length, buffer), position, font.baseSize, 0, color_text);
    
    // Draw cursor
    if(active)
        DrawRectangle(position.x + text_size.x, position.y, font.baseSize / 2.0f, font.baseSize, color_cursor);

    if(tbox->selection != tbox->cursor && active)
    {
        Vector2 select_size = MeasureTextEx(font, TextFormat("%.*s", tbox->selection, buffer), font.baseSize, 0);
        
        // Draw selection
        Rectangle selection_rect = (Rectangle){
            .x = position.x + select_size.x, 
            .y = position.y,
            .width = ABS(text_size.x - select_size.x),
            .height = font.baseSize
        };
        if(text_size.x - select_size.x < 0)
        {
            selection_rect.x -= (selection_rect.width - font.baseSize / 2.0f);
            selection_rect.width -= font.baseSize / 2.0f;
        }
        
        DrawRectangleRec(selection_rect, color_selection);
    }

    EndScissorMode();

    return result;
}

HoUiInteraction
ho_textbox(HoTextBox* tbox, bool active, Vector2 position, Vector2 size, Vector2 margin, Font font, char* buffer, int buffer_length)
{
    HoUiInteraction result = {0};
    if(active)
        result |= ho_textbox_input(tbox, buffer, buffer_length);
    result |= ho_textbox_render(tbox, active, position, size, margin, font, buffer);

    return result;
}