#ifndef _BMFONT_H
#define _BMFONT_H

#include <SDL.h>
#include <string>
#include <vector>
#include <map>

class BMFont
{
public:
    struct Glyph {
        uint32_t glyph_id;
        uint32_t atlasid;
        uint32_t x;
        uint32_t y;
        uint32_t w;
        uint32_t h;
        uint32_t xadvance;
        uint32_t xoffset;
        uint32_t yoffset;
    };

    BMFont();
    ~BMFont();
    bool load(const std::string& font);
    void cleanup();
    const uint32_t height();
    const uint32_t base();
    const BMFont::Glyph& glyph(uint32_t codepoint);
    SDL_Texture* atlas(uint32_t idx);
private:
    std::vector<SDL_Texture *> _atlases;
    std::map<uint32_t,Glyph> _glyphs;
    uint32_t _height;
    uint32_t _base;
};

#endif
