/*  gui_font.cpp
 *
 *  (c) 2011-2012 Anton Olkhovik <ant007h@gmail.com>
 *
 *  Based on a sample Guichan font class by knives.dev@gmail.com
 *  http://docs.google.com/Doc?id=dgp8xrct_9db3zc2hc
 *
 *  This file is part of Mokomaze - labyrinth game.
 *
 *  Mokomaze is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mokomaze is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mokomaze.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui_font.h"

Font::Font(const std::string &fname, int size, int *color, bool bold, bool italic, bool underline)
{
    if (!TTF_WasInit())
    {
        printf("SDL_ttf is not initialized!\n");
        exit(EXIT_FAILURE);
    }

    int style = (bold ? TTF_STYLE_BOLD : 0) |
        (italic ? TTF_STYLE_ITALIC : 0) |
        (underline ? TTF_STYLE_UNDERLINE : 0);
    
    font = TTF_OpenFont(fname.c_str(), size);
    TTF_SetFontStyle(font, style);

    height = TTF_FontHeight(font);
    fg.r = color[0];
    fg.g = color[1];
    fg.b = color[2];
    fg.unused = color[3];

    for (Uint16 count = 0; count < GLYPHS_COUNT; count++)
    {
        char sample_text[2] = {0};
        sample_text[0] = (char)count;
        if (SDL_Surface *tscreen = TTF_RenderText_Blended(font, sample_text, fg))
        {
            SDL_Surface *tscreen_converted = convertToStandardFormat(tscreen);
            SDL_FreeSurface(tscreen);
            glyphs[count].img = new gcn::SDLImage(tscreen_converted, true);
            glyphs[count].img->convertToDisplayFormat();
        }
        else
            glyphs[count].img = NULL;
        TTF_GlyphMetrics(font, count, &glyphs[count].minx, &glyphs[count].maxx, &glyphs[count].miny, &glyphs[count].maxy, &glyphs[count].advance);
    }
}

Font::~Font()
{
    for (Uint16 count = 0; count < GLYPHS_COUNT; count++)
    {
        if (glyphs[count].img)
            delete glyphs[count].img;
    }
    TTF_CloseFont(font);
}

void Font::drawString(gcn::Graphics *graphics, const std::string &text, int x, int y)
{
    if (text.empty())
        return;
    int m_xpos = 0;
    for (std::string::const_iterator m_itr = text.begin(); m_itr != text.end(); m_itr++)
    {
        const Uint16 chr = *m_itr;
        if (chr >= GLYPHS_COUNT)
            continue;
        if (glyphs[chr].img)
            graphics->drawImage(glyphs[chr].img, x + m_xpos, y);
        m_xpos += glyphs[chr].advance;
    }
}

int Font::getWidth(const std::string &text) const
{
    int w = 0, h = 0;
    TTF_SizeText(font, text.c_str(), &w, &h);
    return w;
}

int Font::getHeight() const
{
    return height;
}
