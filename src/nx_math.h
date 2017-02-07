#ifndef CaveStory_nx_math_h
#define CaveStory_nx_math_h

struct NXColor;

struct RectI
{
    int x, y;
    int w, h;
    
    RectI() : x(0), y(0), w(0), h(0) {}
    RectI(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    
};

struct PointF
{
    float x, y;
    PointF(float x, float y) : x(x), y(y) {}
    PointF() : x(0), y(0) {}
    
    PointF operator+(PointF const& r) const;
    
    PointF operator-(PointF const& r) const;
    
    PointF operator*(float k) const;
};

struct RectF
{
    float x, y;
    float w, h;
    
    static RectF centred(PointF const& p, float w, float h);
    
    static RectF fromRectI(RectI const& rect);
    
    bool point_in(PointF const& p) const;
    
    bool point_in(float px, float py) const;
    
    void to_screen_coord(int& x1, int& y1, int& x2, int& y2) const;
    
    void draw_fill_rect(NXColor const& c) const;
    
    void draw_thick_rect(NXColor const& c) const;
    
    void draw_thin_rect(NXColor const& c) const;
    
    void move(PointF const& translation);
    
    RectF scale(float factor);
};

struct TriF
{
    PointF a;
    PointF b, c;
    
    TriF() {}
    TriF(PointF const& a, float size, float rb, float rc);
    
    static float sign(PointF const& p1, PointF const& p2, PointF const& p3);
    
    bool in(PointF const& pt) const;
};

#endif
