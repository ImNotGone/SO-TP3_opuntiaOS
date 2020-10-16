/*
 * Copyright (C) 2020 Nikita Melekhin
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */

#include "Rect.h"

namespace LG {

void Rect::intersect(const Rect& other)
{
    // (a, b) - top left corner
    // (c, d) - bottom right corner
    int a = max(min_x(), other.min_x());
    int b = max(min_y(), other.min_y());
    int c = min(max_x(), other.max_x());
    int d = min(max_y(), other.max_y());

    if (a > c || b > d) {
        set_width(0);
        set_height(0);
        return;
    }

    set_x(a);
    set_y(b);
    set_width(c - a + 1);
    set_height(d - b + 1);
}

bool Rect::intersects(const Rect& other)
{
    // (a, b) - top left corner
    // (c, d) - bottom right corner
    int a = max(min_x(), other.min_x());
    int b = max(min_y(), other.min_y());
    int c = min(max_x(), other.max_x());
    int d = min(max_y(), other.max_y());
    return c >= a && d >= b;
}

void Rect::encode(EncodedMessage& buf) const
{
    Encoder::append(buf, m_origin);
    Encoder::append(buf, m_width);
    Encoder::append(buf, m_height);
}

void Rect::decode(const char* buf, size_t& offset)
{
    Encoder::decode(buf, offset, m_origin);
    Encoder::decode(buf, offset, m_width);
    Encoder::decode(buf, offset, m_height);
}

}