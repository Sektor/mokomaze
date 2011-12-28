/*  gui_font.cpp
 *
 *  (c) 2011 Anton Olkhovik <ant007h@gmail.com>
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
    font = TTF_OpenFont(fname.c_str(), size);
    height = TTF_FontHeight(font);
    fg.r = color[0];
    fg.g = color[1];
    fg.b = color[2];
    fg.unused = color[3];
    int style = (bold ? TTF_STYLE_BOLD : 0) |
        (italic ? TTF_STYLE_ITALIC : 0) |
        (underline ? TTF_STYLE_UNDERLINE : 0);
    TTF_SetFontStyle(font, style);
}

Font::~Font()
{
    TTF_CloseFont(font);
}

void Font::drawString(gcn::Graphics *graphics, const std::string &text, int x, int y)
{
    SDL_Surface *s = TTF_RenderText_Blended(font, text.c_str(), fg);
    SDL_Surface *s_converted = convertToStandardFormat(s);
    SDL_FreeSurface(s);
    gcn::SDLImage *img = new gcn::SDLImage(s_converted, true);
    img->convertToDisplayFormat();
    graphics->drawImage(img, x, y);
    delete img;
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
