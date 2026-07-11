#pragma once
#include <stdint.h>

uint32_t ho_codepoint_from_utf8(const char* in, uint32_t* codepoint);
uint32_t ho_codepoint_to_utf8(uint32_t codepoint, char* utf8stream);

#ifdef HO_UNICODE_IMPLEMENTATION
uint32_t 
ho_codepoint_from_utf8(const char* in, uint32_t* codepoint) 
{
    const uint8_t* text = (const uint8_t*)in; 
    uint32_t advance = 1;
    if (text[0] & 128) { // 1xxx xxxx
        if (text[0] >= 0xF0) {
            advance = 4;
            *codepoint = ((text[0] & 0x07) << 18) | ((text[1] & 0x3F) << 12) | ((text[2] & 0x3F) << 6) | (text[3] & 0x3F);
        } else if (text[0] >= 0xE0) {
            advance = 3;
            *codepoint = ((text[0] & 0x0F) << 12) | ((text[1] & 0x3F) << 6) | (text[2] & 0x3F);
        } else if (text[0] >= 0xC0) {
            advance = 2;
            *codepoint = ((text[0] & 0x1F) << 6) | (text[1] & 0x3F);
        } else {
            *codepoint = text[0]; // continuation byte
        }
    } else {
        *codepoint = (uint32_t)text[0];
    }
    return advance;
}

uint32_t
ho_codepoint_to_utf8(uint32_t codepoint, char* utf8stream)
{
	char* result = utf8stream;
    if(codepoint <= 0x7f) {
        *result++ = (uint8_t)codepoint;
    } else if(codepoint >= 0x80 && codepoint <= 0x7ff) {
        uint8_t b1 = 0xc0 | (codepoint >> 6);
        uint8_t b2 = 0x80 | ((codepoint & 0x3f) | 0x30000000);
        *result++ = b1;
        *result++ = b2;
    } else if(codepoint >= 0x800 && codepoint <= 0xffff) {
        uint8_t b1 = 0xe0 | (codepoint >> 12);
        uint8_t b2 = 0x80 | (((codepoint >> 6) & 0x3f) | 0x30000000);
        uint8_t b3 = 0x80 | ((codepoint & 0x3f) | 0x30000000);
        *result++ = b1;
        *result++ = b2;
        *result++ = b3;
    } else if(codepoint >= 0x00010000 && codepoint <= 0x001fffff) {
        uint8_t b1 = 0xf0 | (codepoint >> 18);
        uint8_t b2 = 0x80 | (((codepoint >> 12) & 0x3f) | 0x30000000);
        uint8_t b3 = 0x80 | (((codepoint >> 6) & 0x3f) | 0x30000000);
        uint8_t b4 = 0x80 | ((codepoint & 0x3f) | 0x30000000);
        *result++ = b1;
        *result++ = b2;
        *result++ = b3;
        *result++ = b4;
    }
	return (uint32_t)(result - utf8stream);
}

uint32_t
ho_utf8_find_previous(const char* stream)
{
    const char* at = stream;
    while(*at && (*at & 0b11000000) == 0b10000000)
    {
        at--;
    }
    return (uint32_t)(stream - at);
}
#endif