#include <cmath>

#include "nx_math.h"
#include "graphics/graphics.h"

PointF PointF::operator+(PointF const& r) const
{
    return PointF(x + r.x, y + r.y);
}

PointF PointF::operator-(PointF const& r) const
{
    return PointF(x - r.x, y - r.y);
}

PointF PointF::operator*(float k) const
{
    return PointF(k * x, k * y);
}





RectF RectF::centred(PointF const& p, float w, float h)
{
    RectF r = {p.x - w/2, p.y - h/2, w, h};
    return r;
}

RectF RectF::fromRectI(RectI const& rect)
{
    RectF r = {(float)rect.x / Graphics::SCREEN_WIDTH,
        (float)rect.y / Graphics::SCREEN_HEIGHT,
        (float)rect.w / Graphics::SCREEN_WIDTH,
        (float)rect.h / Graphics::SCREEN_HEIGHT,
    };
    return r;
}

bool RectF::point_in(PointF const& p) const
{
    return point_in(p.x, p.y);
}

bool RectF::point_in(float px, float py) const
{
    return !(px < x || x + w < px || py < y || y + h < py);
}

void RectF::to_screen_coord(int& x1, int& y1, int& x2, int& y2) const
{
    x1 = Graphics::SCREEN_WIDTH  *  x;
    y1 = Graphics::SCREEN_HEIGHT *  y;
    x2 = Graphics::SCREEN_WIDTH  * (x + w);
    y2 = Graphics::SCREEN_HEIGHT * (y + h);
}

void RectF::draw_fill_rect(NXColor const& c) const
{
    int x1, y1, x2, y2;
    to_screen_coord(x1, y1, x2, y2);
    Graphics::FillRect(x1, y1, x2, y2, c);
}

void RectF::draw_thick_rect(NXColor const& c) const
{
    int x1, y1, x2, y2;
    to_screen_coord(x1, y1, x2, y2);
    Graphics::DrawRect(x1, y1, x2, y2, c);
}

void RectF::draw_thin_rect(NXColor const& c) const
{
    int x1, y1, x2, y2;
    to_screen_coord(x1, y1, x2, y2);
    Graphics::DrawLine(x1, y1, x2, y1, c);
    Graphics::DrawLine(x1, y2, x2, y2, c);
    Graphics::DrawLine(x1, y1, x1, y2, c);
    Graphics::DrawLine(x2, y1, x2, y2, c);
}

void RectF::move(PointF const& translation)
{
    x += translation.x;
    y += translation.y;
}

RectF RectF::scale(float factor)
{
//    float cx = x + w/2;
//    float cy = y + h/2;
//    
//    float nw = w * factor;
//    float nh = h * factor;
//    
//    RectF r = {cx - 2*nw, cy - 2*nh, nw, nh};
    
    RectF r = {x, y, w * factor, h * factor};
    return r;
}


TriF::TriF(PointF const& a, float size, float rb, float rc) :
a(a)
{
#define P(a) (double(a) * M_PI / 8.0)
    b = PointF(cos(P(rb)), sin(P(rb))) * size + a;
    c = PointF(cos(P(rc)), sin(P(rc))) * size + a;
#undef P
}

float TriF::sign(PointF const& p1, PointF const& p2, PointF const& p3)
{
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool TriF::in(PointF const& pt) const
{
    bool b1, b2, b3;
    
    b1 = sign(pt, a, b) < 0.0f;
    b2 = sign(pt, b, c) < 0.0f;
    b3 = sign(pt, c, a) < 0.0f;
    
    return ((b1 == b2) && (b2 == b3));
}
