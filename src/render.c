/*  render.c
 *
 *  Graphics engine.
 *
 *  (c) 2009-2011 Anton Olkhovik <ant007h@gmail.com>
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

#include "render.h"
#include "matrix.h"
#include "mazecore/mazehelpers.h"
#include "types.h"

extern Level *game_levels;
extern MazeConfig game_config;
extern int cur_level;
extern int prev_px, prev_py;
extern int disp_bpp;

extern Animation final_anim;
extern Animation *keys_anim;

extern SDL_Surface *screen;
extern SDL_Surface *fin_pic, *desk_pic, *wall_pic, *render_pic;
extern SDL_Rect desk_rect;

//------------------------------------------------------------------------------

void **alloc2d(int mi, int mj, int s, int su)
{
    void **res;
    res = malloc( mi*su );
    for(int i=0; i < mi; i++)
        res[i] = malloc( mj*s );
    return res;
}

#define ALLOC2D_DIAM(type, diam) (type**) alloc2d(diam, diam, sizeof(type), sizeof(type*))

#define AA_SAMPLING_STEP 4
#define AA_SAMPLES_COUNT (AA_SAMPLING_STEP * AA_SAMPLING_STEP)
void calc_circle(float **zeds, uint8_t **aa, int rad1, int rad2, bool rel)
{
    float zk = rel ? 1.0/rad2 : 1;
    for (int x = -rad2; x <= rad2; x++)
    {
        for (int y = -rad2; y <= rad2; y++)
        {
            float xdist = x*x + y*y;
            int sample_hit = 0;

            if ( xdist > sqr(rad2 + 1) ||
                 ( rad1 > 0 && xdist < sqr(rad1 - 1) ) ) sample_hit = 0;
            else if ( xdist < sqr(rad2 - 1) &&
                     ( rad1 == 0 || xdist > sqr(rad1 + 1) ) ) sample_hit = AA_SAMPLES_COUNT;
            else
            {
                for (int ii = 0; ii < AA_SAMPLING_STEP; ii++)
                    for (int jj = 0; jj < AA_SAMPLING_STEP; jj++)
                    {
                        float rast = sqr(x - 0.3 + 0.2 * ii) + sqr(y - 0.3 + 0.2 * jj);
                        if ((rast <= rad2 * rad2) && (rast >= rad1 * rad1))
                            sample_hit++;
                    }
            }

            if (aa) aa[x + rad2][y + rad2] = sample_hit;
            if (zeds) zeds[x + rad2][y + rad2] = (sample_hit == AA_SAMPLES_COUNT) ? sqrt(rad2 * rad2 - xdist)*zk : -1;
        }
    }
}

//------------------------------------------------------------------------------

#define rshift screen->format->Rshift
#define gshift screen->format->Gshift
#define bshift screen->format->Bshift
#define ashift screen->format->Ashift

#define rmask screen->format->Rmask
#define gmask screen->format->Gmask
#define bmask screen->format->Bmask
#define amask screen->format->Amask

#define extract_r(col) ((col & rmask) >> rshift)
#define extract_g(col) ((col & gmask) >> gshift)
#define extract_b(col) ((col & bmask) >> bshift)
#define extract_a(col) ((col & amask) >> ashift)

#define max_r (rmask >> rshift)
#define max_g (gmask >> gshift)
#define max_b (bmask >> bshift)
#define max_a (amask >> ashift)

Uint32 GetPixel(SDL_Surface *surf, int adr)
{
    if (disp_bpp == 32)
        return ((Uint32*)surf->pixels)[adr];
    else
        return ((Uint16*)surf->pixels)[adr];
}

void PutPixel(SDL_Surface *surf, int adr, Uint32 col)
{
    if (disp_bpp == 32)
        ((Uint32*)surf->pixels)[adr] = col;
    else
        ((Uint16*)surf->pixels)[adr] = col;
}

Uint32 ColorToBit(uint8_t c0, uint8_t c1, uint8_t c2)
{
    return (c0<<rshift) | (c1<<gshift) | (c2<<bshift);
}

Uint32 ShadeBitColor(Uint32 col, float k)
{
    k = 1-k;
    uint8_t c0, c1, c2;
    c2 = (uint8_t)(extract_b(col) * k);
    c1 = (uint8_t)(extract_g(col) * k);
    c0 = (uint8_t)(extract_r(col) * k);
    return ColorToBit(c0, c1, c2);
}

Uint32 MixBitColor(Uint32 col, SDL_Color mix, float k)
{
    uint8_t fc0, fc1, fc2;
    fc2 = extract_b(col)*(1-k) + mix.b*k;
    fc1 = extract_g(col)*(1-k) + mix.g*k;
    fc0 = extract_r(col)*(1-k) + mix.r*k;
    clamp_max(fc2, max_b);
    clamp_max(fc1, max_g);
    clamp_max(fc0, max_r);

    uint8_t c0, c1, c2;
    c2 = (uint8_t)fc2;
    c1 = (uint8_t)fc1;
    c0 = (uint8_t)fc0;
    return ColorToBit(c0, c1, c2);
}

//------------------------------------------------------------------------------

SDL_Surface *CreateSurface(Uint32 flags, int width, int height, const SDL_Surface *display)
{
    const SDL_PixelFormat fmt = *(display->format);
    return SDL_CreateRGBSurface(flags, width, height,
                                fmt.BitsPerPixel,
                                fmt.Rmask, fmt.Gmask, fmt.Bmask, fmt.Amask);
}

void DrawBlended(SDL_Surface *from, SDL_Surface *to, float k)
{
    for (int i=0; i<from->w*from->h; i++)
    {
        Uint32 c = ((Uint32*)from->pixels)[i];
        uint8_t a = ((c & from->format->Amask) >> from->format->Ashift) * k;
        c &= ~from->format->Amask;
        c |= a << from->format->Ashift;
        ((Uint32*)to->pixels)[i] = c;
    }
}

//------------------------------------------------------------------------------

static float **hole_zeds = NULL;
static uint8_t **hole_aa = NULL;
static uint8_t **key_aa = NULL;

void DrawHole(int x0, int y0, int r, float grayk, float shiftk)
{
    for (int y=-r; y<=r; y++)
    {
        int adr = game_config.wnd_w*(y0+y) + (x0-r);
        for (int x=-r; x<=r; x++)
        {
            if ((y0+y>=0)&&(y0+y<render_pic->h)&&
                (x0+x>=0)&&(x0+x<render_pic->w))
            {
                float k = hole_zeds[x+r][y+r];
                if (k>=0)
                {
                    uint8_t c0,c1,c2;
                    float kk = k*shiftk;
                    clamp_max(kk, 1);
                    c0=(uint8_t)((max_r*grayk)*kk);
                    c1=(uint8_t)((max_g*grayk)*kk);
                    c2=(uint8_t)((max_b*grayk)*kk);
                    Uint32 col = ColorToBit(c0,c1,c2);
                    PutPixel(render_pic, adr, col);
                }
                else
                {
                    uint8_t aa_k = hole_aa[x+r][y+r];
                    if (aa_k>0)
                    {
                        Uint32 col = GetPixel(render_pic, adr);
                        col = ShadeBitColor(col, (float)aa_k/AA_SAMPLES_COUNT);
                        PutPixel(render_pic, adr, col);
                    }
                }
            }
            adr++;
        }
    }
}

void DrawKey(int x0, int y0, int r, float anim)
{
    SDL_Color mix;
    mix.unused = 0;
    mix.r = (uint8_t)( 6.0/31*max_r*(1.0-anim) + 28.0/31*max_r*anim);
    mix.g = (uint8_t)(24.0/65*max_g*(1.0-anim) + 28.0/65*max_g*anim);
    mix.b = (uint8_t)(18.0/31*max_b*(1.0-anim) +  0.0/31*max_b*anim);

    Uint32 def_color = ColorToBit(mix.r, mix.g, mix.b);
    for (int y=-r; y<=r; y++)
    {
        int adr = game_config.wnd_w*(y0+y) + (x0-r);
        for (int x=-r; x<=r; x++)
        {
            if ((y0+y>=0)&&(y0+y<screen->h)&&
                (x0+x>=0)&&(x0+x<screen->w))
            {
                uint8_t aa_k = key_aa[x+r][y+r];
                if (aa_k>0)
                {
                    Uint32 col;
                    if (aa_k==AA_SAMPLES_COUNT)
                    {
                        col = def_color;
                    }
                    else
                    {
                        col = GetPixel(screen, adr);
                        col = MixBitColor(col, mix, (float)aa_k/AA_SAMPLES_COUNT);
                    }
                    PutPixel(screen, adr, col);
                }
            }
            adr++;
        }
    }
}

void TryToShadow(int x, int y, float k)
{
    if ( x<0 || x>=render_pic->w || y<0 || y>=render_pic->h )
        return;
    int adr = y*render_pic->w + x;
    Uint32 col = GetPixel(render_pic, adr);
    col = ShadeBitColor(col, k);
    PutPixel(render_pic, adr, col);
}

float GetShadowKoef(float r)
{
    float k = 0.8;
    if (game_config.shadow-1 > 0)
        k += (0.1-0.8)*r/(game_config.shadow-1);
    return k;
}

void RenderLevel()
{
//-- Prepare background --------------------------------------------------------
    SDL_BlitSurface(desk_pic, &desk_rect, render_pic, &desk_rect);

//-- Generate shadows-----------------------------------------------------------
    int b_co = game_levels[cur_level].boxes_count;
    Box *bxs = game_levels[cur_level].boxes;
    for (int i=0; i<b_co; i++)
    {
        Box *b = &bxs[i];

        for (int i=0; i<game_config.shadow; i++)
        {
            float k = GetShadowKoef(i);
            for (int y=b->y1; y<b->y2; y++)
            {
                TryToShadow(b->x1-1-i, y, k);
                TryToShadow(b->x2+i, y, k);
            }
            for (int x=b->x1; x<b->x2; x++)
            {
                TryToShadow(x, b->y1-1-i, k);
                TryToShadow(x, b->y2+i, k);
            }
        }

        for (int x=0; x<game_config.shadow; x++)
            for (int y=0; y<game_config.shadow; y++)
            {
                float r = calclen(x,y,0);
                if (r < game_config.shadow-0.5)
                {
                    float k = GetShadowKoef(r);
                    TryToShadow(b->x1-1-x, b->y1-1-y, k);
                    TryToShadow(b->x2+x, b->y2+y, k);
                    TryToShadow(b->x1-1-x, b->y2+y, k);
                    TryToShadow(b->x2+x, b->y1-1-y, k);
                }
            }
    }

//-- Draw the walls ------------------------------------------------------------
    for (int i=0; i<b_co; i++)
    {
        SDL_Rect wall_rect;
        wall_rect.x = bxs[i].x1; wall_rect.y = bxs[i].y1;
        wall_rect.w = bxs[i].x2 - bxs[i].x1;
        wall_rect.h = bxs[i].y2 - bxs[i].y1;
        SDL_BlitSurface(wall_pic, &wall_rect, render_pic, &wall_rect);
    }

//-- Draw holes ----------------------------------------------------------------
    for (int i=0; i<game_levels[cur_level].holes_count; i++)
    {
        DrawHole( game_levels[cur_level].holes[i].x,
                  game_levels[cur_level].holes[i].y,
                  game_config.hole_r,
                  0.18, 1 );
    }

    //final hole
    DrawHole(game_levels[cur_level].fins[0].x,
             game_levels[cur_level].fins[0].y,
             game_config.hole_r,
             0.85, 0.50);

    if (game_levels[cur_level].keys_count == 0)
    {
        SDL_Rect om_rect;
        int om_x0 = game_levels[cur_level].fins[0].x - fin_pic->w/2;
        int om_y0 = game_levels[cur_level].fins[0].y - fin_pic->h/2;
        om_rect.x = om_x0; om_rect.y = om_y0;
        om_rect.w = fin_pic->w; om_rect.h = fin_pic->h;
        SDL_BlitSurface(fin_pic, NULL, render_pic, &om_rect);
    }
}

void RedrawDesk()
{
    SDL_BlitSurface(render_pic, &desk_rect, screen, &desk_rect);
}

//------------------------------------------------------------------------------
//-- Ball drawing --------------------------------------------------------------
//------------------------------------------------------------------------------
static float matrix[16];
static float **a = NULL, **a_1 = NULL;
static float **zeds = NULL;
static uint8_t **ball_aa = NULL;
static int rad = 0;

void texSmooth(uint8_t *c0, uint8_t *c1, uint8_t *c2, SDL_Color secc, float fixx)
{
    *c0 = (*c0)*(0.5+fixx) + secc.r*(0.5-fixx);
    *c1 = (*c1)*(0.5+fixx) + secc.g*(0.5-fixx);
    *c2 = (*c2)*(0.5+fixx) + secc.b*(0.5-fixx);
    clamp_max(*c0, max_r);
    clamp_max(*c1, max_g);
    clamp_max(*c2, max_b);
}

#define COS_PI_4   0.7071
#define COS_PI_2   0
#define COS_3PI_4 -COS_PI_4
void DrawBall(int tk_px, int tk_py, float poss_z, const dReal *R, SDL_Color bcolor)
{
    //prepare to draw the ball
    matrix[0] = R[0];
    matrix[1] = R[4];
    matrix[2] = R[8];
    matrix[3] = 0;
    matrix[4] = R[1];
    matrix[5] = R[5];
    matrix[6] = R[9];
    matrix[7] = 0;
    matrix[8] = R[2];
    matrix[9] = R[6];
    matrix[10] = R[10];
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;

    a[0][0] = matrix[0];
    a[0][1] = matrix[1];
    a[0][2] = matrix[2];
    a[1][0] = matrix[4];
    a[1][1] = matrix[5];
    a[1][2] = matrix[6];
    a[2][0] = matrix[8];
    a[2][1] = matrix[9];
    a[2][2] = matrix[10];

    MatrixInversion(a, 3, a_1);

    //redraw the ball at new location
    if (SDL_MUSTLOCK(screen))
        if (SDL_LockSurface(screen) < 0)
            return;

    Uint32 bcolor_r = bcolor.r/255.0*max_r;
    Uint32 bcolor_g = bcolor.g/255.0*max_g;
    Uint32 bcolor_b = bcolor.b/255.0*max_b;
    clamp_max(bcolor_r, max_r);
    clamp_max(bcolor_g, max_g);
    clamp_max(bcolor_b, max_b);
    bcolor.r = bcolor_r;
    bcolor.g = bcolor_g;
    bcolor.b = bcolor_b;

    uint8_t c0, c1, c2;
    for (int x = -rad; x <= rad; x++)
    {
        for (int y = -rad; y <= rad; y++)
        {
            float z;

            if ((tk_py + y >= 0) && (tk_py + y < screen->h) &&
                (tk_px + x >= 0) && (tk_px + x < screen->w))
            {
                if ((z = zeds[x + rad][y + rad]) >= 0)
                {
                    float x0, y0, z0;
                    x0 = a_1[0][0] * x + a_1[1][0] * y + a_1[2][0] * z;
                    y0 = a_1[0][1] * x + a_1[1][1] * y + a_1[2][1] * z;
                    z0 = a_1[0][2] * x + a_1[1][2] * y + a_1[2][2] * z;

                    float ksi = (float) z0 / rad;
                    float fi = (x0 != 0 ? (float) y0 / x0 : 0);

                    float fip = (y0 != 0 ? (float) x0 / y0 : 0);
                    float ksim = ksi - COS_PI_2;
                    SDL_Color secc;

                    if (ksi >= COS_PI_2)
                    {
                        if (fi <= 0)
                        {
                            c0 = bcolor.r;
                            c1 = bcolor.g;
                            c2 = bcolor.b;
                            secc.r = max_r * 1;
                            secc.g = max_g * 1;
                            secc.b = max_b * 1;
                        }
                        else
                        {
                            c0 = max_r * 1;
                            c1 = max_g * 1;
                            c2 = max_b * 1;
                            secc.r = bcolor.r;
                            secc.g = bcolor.g;
                            secc.b = bcolor.b;
                        }
                    }
                    else
                    {
                        if (fi <= 0)
                        {
                            c0 = max_r * 1;
                            c1 = max_g * 1;
                            c2 = max_b * 1;
                            secc.r = bcolor.r;
                            secc.g = bcolor.g;
                            secc.b = bcolor.b;
                        }
                        else
                        {
                            c0 = bcolor.r;
                            c1 = bcolor.g;
                            c2 = bcolor.b;
                            secc.r = max_r * 1;
                            secc.g = max_g * 1;
                            secc.b = max_b * 1;
                        }
                    }

                    if ((fi < 0.04) && (fi >= 0.0))
                        texSmooth(&c0, &c1, &c2, secc, fi / 0.08);
                    else
                    if ((fi>-0.04) && (fi < 0.0))
                        texSmooth(&c0, &c1, &c2, secc, -fi / 0.08);
                    else

                    if ((ksim < 0.02) && (ksim >= 0.0))
                        texSmooth(&c0, &c1, &c2, secc, ksim / 0.04);
                    else
                    if ((ksim>-0.02) && (ksim < 0.0))
                        texSmooth(&c0, &c1, &c2, secc, -ksim / 0.04);
                    else

                    if ((fip < 0.04) && (fip >= 0.0))
                        texSmooth(&c0, &c1, &c2, secc, fip / 0.08);
                    else
                    if ((fip>-0.04) && (fip < 0.0))
                        texSmooth(&c0, &c1, &c2, secc, -fip / 0.08);

                    //shader
                    float mz = (rad - poss_z);
                    clamp_max(mz, 0.6 * rad);
                    clamp_min(mz, 0);
                    float cosa = (z - mz) / rad;
                    clamp_min(cosa, 0);

                    c0 = (uint8_t) ((float) c0 * cosa);
                    c1 = (uint8_t) ((float) c1 * cosa);
                    c2 = (uint8_t) ((float) c2 * cosa);

                    int adr = ((tk_py + y) * screen->w + (tk_px + x));
                    Uint32 col = ColorToBit(c0,c1,c2);
                    PutPixel(screen, adr, col);
                }
                else
                {
                    uint8_t aa_k = ball_aa[x + rad][y + rad];
                    if (aa_k > 0)
                    {
                        int adr = ((tk_py + y) * screen->w + (tk_px + x));
                        Uint32 col = GetPixel(screen, adr);
                        col = ShadeBitColor(col, (float)aa_k/AA_SAMPLES_COUNT);
                        PutPixel(screen, adr, col);
                    }
                }
            }
        } //for y
    } //for x

    if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);
}

//------------------------------------------------------------------------------

static SDL_Surface *fin_pic_blended = NULL;
void UpdateBufAnimation()
{
    int kx, ky, kr;
    int hx, hy, hr;
    SDL_Rect fin_rect, key_rect;

    hx = game_levels[cur_level].fins[0].x;
    hy = game_levels[cur_level].fins[0].y;
    hr = game_config.hole_r;

    fin_rect.x = hx-hr; fin_rect.y = hy-hr;
    fin_rect.w = hr*2+1; fin_rect.h = hr*2+1;

    //restore background around final hole
    if (maze_is_keys_passed())
        SDL_BlitSurface(render_pic, &fin_rect, screen, &fin_rect);

    //restore background around keys
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        kx = game_levels[cur_level].keys[i].x;
        ky = game_levels[cur_level].keys[i].y;
        kr = game_config.key_r;

        key_rect.x = kx-kr; key_rect.y = ky-kr;
        key_rect.w = kr*2+1; key_rect.h = kr*2+1;

        SDL_BlitSurface(render_pic, &key_rect, screen, &key_rect);
    }

    //draw keys with actual animation stages
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        kx = game_levels[cur_level].keys[i].x;
        ky = game_levels[cur_level].keys[i].y;
        kr = game_config.key_r;

        key_rect.x = kx-kr; key_rect.y = ky-kr;
        key_rect.w = kr*2+1; key_rect.h = kr*2+1;

        float kk = keys_anim[i].progress;
        DrawKey( kx, ky, kr, kk );
    }

    //draw final hole with actual animation stage
    if (maze_is_keys_passed())
    {
        float kk = final_anim.progress;

        SDL_Rect om_rect;
        om_rect.x = game_levels[cur_level].fins[0].x - fin_pic->w/2;
        om_rect.y = game_levels[cur_level].fins[0].y - fin_pic->h/2;
        om_rect.w = fin_pic->w; om_rect.h = fin_pic->h;

        if (kk<1)
        {
            DrawBlended(fin_pic, fin_pic_blended, kk);
            SDL_BlitSurface(fin_pic_blended, NULL, screen, &om_rect);
        }
        else
        {
            SDL_BlitSurface(fin_pic, NULL, screen, &om_rect);
        }
    }
}

void UpdateScreenAnimation()
{
    //updating animation on the screen
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        if (keys_anim[i].stage == ANIMATION_PLAYING)
        {
            int kx,ky,kr;
            kx = game_levels[cur_level].keys[i].x;
            ky = game_levels[cur_level].keys[i].y;
            kr = game_config.key_r;

            SDL_Rect key_rect;
            key_rect.x=kx-kr; key_rect.y=ky-kr;
            key_rect.w=kr*2+1; key_rect.h=kr*2+1;

            SDL_UpdateRect(screen, key_rect.x, key_rect.y, key_rect.w, key_rect.h);
        }
    }

    if (final_anim.stage == ANIMATION_PLAYING)
    {
        SDL_Rect om_rect;
        om_rect.x = game_levels[cur_level].fins[0].x - fin_pic->w/2;
        om_rect.y = game_levels[cur_level].fins[0].y - fin_pic->h/2;
        om_rect.w = fin_pic->w; om_rect.h = fin_pic->h;
        SDL_UpdateRect(screen, om_rect.x, om_rect.y, om_rect.w, om_rect.h);
    }
}

//------------------------------------------------------------------------------

void InitRender()
{
    fin_pic_blended = CreateSurface(SDL_SWSURFACE, fin_pic->w, fin_pic->h, fin_pic); // blended final image

    //init matrices for drawing the ball
    a = ALLOC2D_DIAM(float, 3);
    a_1 = ALLOC2D_DIAM(float, 3);

    //alloc ball z-values array
    rad = game_config.ball_r - 1; //
    int diam = rad * 2 + 1;
    zeds = ALLOC2D_DIAM(float, diam);
    ball_aa = ALLOC2D_DIAM(uint8_t, diam);
    calc_circle(zeds, ball_aa, 0, rad, false);

    //alloc hole z-values array
    int h_rad = game_config.hole_r;
    int h_diam = h_rad * 2 + 1;
    hole_zeds = ALLOC2D_DIAM(float, h_diam);
    hole_aa = ALLOC2D_DIAM(uint8_t, h_diam);
    calc_circle(hole_zeds, hole_aa, 0, h_rad, true);

    //alloc array for image of key
    int k_rad = game_config.key_r;
    int k_rad_v = k_rad * 75 / 100;
    int k_diam = k_rad * 2 + 1;
    key_aa = ALLOC2D_DIAM(uint8_t, k_diam);
    calc_circle(NULL, key_aa, k_rad_v, k_rad, false);
}
