/*  gui_font.h
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

#ifndef GUI_FONT_H
#define GUI_FONT_H

#include <SDL/SDL_ttf.h>
#include <guichan.hpp>
#include <guichan/image.hpp>
#include <guichan/sdl.hpp>

class Font : public gcn::Font, public gcn::SDLImageLoader
{
private:
    int height;
    SDL_Color fg;
    TTF_Font *font;
public:
    Font(const std::string &fname, int size, int *color,
        bool bold = false, bool italic = false, bool underline = false);
    void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y);
    virtual ~Font();
    virtual int getHeight() const;
    virtual int getWidth(const std::string &text) const;
};

#endif /* GUI_FONT_H */
