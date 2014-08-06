/*  render.h
 *
 *  Graphics engine.
 *
 *  (c) 2009-2012 Anton Olkhovik <ant007h@gmail.com>
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
 *  along with Mokomaze.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef RENDER_H
#define RENDER_H

#include <SDL/SDL.h>
#include "mazecore/mazecore.h"

SDL_Surface *CreateSurface(Uint32 flags, int width, int height, const SDL_Surface *display);
void RenderLevel();
void RedrawDesk();
void DrawBall(int tk_px, int tk_py, float poss_z, const dReal *R, SDL_Color bcolor);
void UpdateBufAnimation();
void UpdateScreenAnimation();
void InitRender();

#endif //RENDER_H
