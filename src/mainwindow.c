/*  mainwindow.c
 *
 *  Main game routines
 *
 *  (c) 2009 Anton Olkhovik <ant007h@gmail.com>
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

#include <stdio.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_ttf.h>
//#define dDOUBLE
#include <ode/ode.h>

#include "types.h"
#include "accelerometers.h"
#include "paramsloader.h"
#include "vibro.h"

#define GRAV_CONST 9.81*1.0
int prev_px, prev_py; // for renderer

Config game_config;
Level* game_levels;
int game_levels_count;
int cur_level;

int fullscreen;

#define GAME_STATE_NORMAL   1
#define GAME_STATE_FAILED   2
#define GAME_STATE_WIN      3
#define GAME_STATE_SAVED    4
int game_state;
int new_game_state;

int LoadImgErrors=0;

//==============================================================================

#define ANIMATION_NONE      0
#define ANIMATION_PLAYING   1
#define ANIMATION_FINISHED  2
typedef struct {

	float		time;
        int             stage;

} Animation;

Animation final_anim;
Animation *keys_anim = NULL;
int keys_passed = 0;
int save_key = -1;

//==============================================================================
float calcdist(float x1,float y1, float x2,float y2)
{
    return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

float calclen(float x, float y)
{
    return sqrt(x*x + y*y);
}

DPoint normalize(DPoint p)
{
    DPoint r;
    float len = calclen(p.x, p.y);
    r.x = p.x/len;
    r.y = p.y/len;
    return r;
}

int inbox(float x, float y, Box box)
{
    return ( (x>=box.x1) && (x<=box.x2) &&
             (y>=box.y1) && (y<=box.y2) );
}

int incircle(float x, float y,  float cx, float cy, float cr)
{
    return (calcdist(x,y, cx,cy) <= cr);
}

int min(int a, int b)
{
    if (a<=b) return a; else return b;
}
int max(int a, int b)
{
    if (a>=b) return a; else return b;
}

float sign(float x)
{
    if (x<0) return -1;
    if (x>0) return +1;
    return 0;
}

void** alloc2d(int mi, int mj, int s, int su)
{
    void **res;
    res = malloc( mi*su );
    for(int i=0; i < mi; i++)
    {
        res[i] = malloc( mj*s );
    }
    return res;
}
//==============================================================================

//----------------------
#define PHYS_SCALE 100.0
//----------------------

float vibro_force_k=1;
#define MAX_BUMP_SPEED 240.0
#define MIN_BUMP_SPEED 70.0
void BumpVibrate(float speed)
{
    if (speed>0)
    {
        //printf("%f\n",speed);
        if (speed>=MIN_BUMP_SPEED)
        {
            float k = (speed-MIN_BUMP_SPEED)/(MAX_BUMP_SPEED-MIN_BUMP_SPEED);
            if (k>1) k=1;
            float lev = 0.27+0.73*k;
            lev *= vibro_force_k;
            BYTE vlevel = (BYTE)(lev*255);
            set_vibro(vlevel);
        }
    }    
}


int fall=0;
int fall_fixed=0;
Point fall_hole;
void GoFall(Point hole);
void GoFallFixed(Point hole);

int testbump(float x, float y)
{

    if (fall)
    {
        if (!fall_fixed)
        {
            if ( (x >= fall_hole.x-game_config.hole_r+game_config.ball_r) &&
                 (x <= fall_hole.x+game_config.hole_r-game_config.ball_r) &&
                 (y >= fall_hole.y-game_config.hole_r+game_config.ball_r) &&
                 (y <= fall_hole.y+game_config.hole_r-game_config.ball_r) )
                GoFallFixed(fall_hole);
        }
        return 0;
    }

    Point final_hole = game_levels[cur_level].fins[0];
    float dist = calcdist(x,y, final_hole.x,final_hole.y);
    if (dist <= game_config.hole_r)
    {
        GoFall(final_hole);
        if (keys_passed == game_levels[cur_level].keys_count) //
            new_game_state = GAME_STATE_WIN;
        else
            new_game_state = GAME_STATE_SAVED;
        return 1;
    }

    for (int i=0; i<game_levels[cur_level].holes_count; i++)
    {
        Point hole = game_levels[cur_level].holes[i];
        Box boundbox;
        boundbox.x1 = hole.x - game_config.hole_r -1;
        boundbox.y1 = hole.y - game_config.hole_r -1;
        boundbox.x2 = hole.x + game_config.hole_r +1;
        boundbox.y2 = hole.y + game_config.hole_r +1;
        if (inbox(x,y, boundbox))
        {
            float dist = calcdist(x,y, hole.x,hole.y);
            if (dist <= game_config.hole_r)
            {
                GoFall(hole);
                new_game_state = GAME_STATE_FAILED;
                return 1;
            }
        }
    }

    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        if (keys_anim[i].stage == ANIMATION_NONE)
        {
            Point key = game_levels[cur_level].keys[i];
            Box boundbox;
            boundbox.x1 = key.x - game_config.key_r -1;
            boundbox.y1 = key.y - game_config.key_r -1;
            boundbox.x2 = key.x + game_config.key_r +1;
            boundbox.y2 = key.y + game_config.key_r +1;
            if (inbox(x,y, boundbox))
            {
                float dist = calcdist(x,y, key.x,key.y);
                if (dist <= game_config.key_r)
                {
                    keys_anim[i].stage = ANIMATION_PLAYING;
                    keys_passed++;
                    save_key = i;
                    if (keys_passed == game_levels[cur_level].keys_count)
                    {
                        final_anim.stage = ANIMATION_PLAYING;
                    }
                    return 0;
                }
            }
        }
    }

    return 0;
}

//== ODE =======================================================================
// dynamics and collision objects
static dGeomID plane;
static dSpaceID space;
static dWorldID world;
static dBodyID body;
static dGeomID geom;
static dMass m;
static dJointGroupID contactgroup;
//static dGeomID *walls = NULL;

// this is called by dSpaceCollide when two objects in space are
// potentially colliding.
#define MAX_CONTACTS 8
static void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
    
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    if (b1 == b2) return;

    dContact contact[MAX_CONTACTS]; // up to MAX_CONTACTS contacts
    for (int i = 0; i < MAX_CONTACTS; i++) {
        contact[i].surface.mode = dContactApprox1 | dContactSoftCFM | dContactSoftERP;
        contact[i].surface.mu = 0.7f;
        contact[i].surface.soft_erp = 0.8f;
        contact[i].surface.soft_cfm = 0.00001f;
    }

    int numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sizeof (dContact));
    if (numc)
    {
        for (int i = 0; i < numc; i++) {
            dJointID c = dJointCreateContact(world, contactgroup, contact + i);
            dJointAttach(c, b1, b2);
        }

        if ((o1!=plane) && (o2!=plane))
        {
            const dReal *Normal = contact[0].geom.normal;
            const dReal *LinearVel = dBodyGetLinearVel(body);

            float vlen = sqrt(  LinearVel[0]*LinearVel[0] +
                                LinearVel[1]*LinearVel[1] +
                                LinearVel[2]*LinearVel[2] );
            float cosa = Normal[0]*LinearVel[0] +
                         Normal[1]*LinearVel[1] +
                         Normal[2]*LinearVel[2];
            float pvel = vlen*cosa;

            BumpVibrate(-pvel*70);
        }
    }

}
//-- ODE -----------------------------------------------------------------------

SDL_Surface *screen;
SDL_Surface *fin_pic, *desk_pic, *wall_pic, *render_pic;
SDL_Rect desk_rect;

BYTE *shadowbuf;
float **hole_zeds;
BYTE **hole_aa;
BYTE **key_aa;

void FillShadow(Box b)
{
    for (int y=b.y1; y<b.y2; y++)
    {
        int adr = game_config.wnd_w * y + b.x1;
        for (int x=b.x1; x<b.x2; x++)
        {
            if ((y>=0)&&(y<screen->h)&&
                (x>=0)&&(x<screen->w)) shadowbuf[adr]=1;
            adr++;
        }
    }
}

/*
void From16BitToColor(Uint16 col, BYTE *c0, BYTE *c1, BYTE *c2)
{
    *c2 = (BYTE)(col & 31);
    *c1 = (BYTE)((col >> 5) & 63);
    *c0 = (BYTE)((col >> 11) & 31);
}
*/

Uint16 ColorTo16Bit(BYTE c0, BYTE c1, BYTE c2)
{
    return (c0<<11) | (c1<<5) | (c2);
}

Uint16 Shade16BitColor(Uint16 col, float k)
{
    k = 1-k;
    BYTE c0,c1,c2;
    c2 = (BYTE)((col & 31) * k);
    c1 = (BYTE)(((col >> 5) & 63) * k);
    c0 = (BYTE)(((col >> 11) & 31) * k);
    return ColorTo16Bit(c0,c1,c2);
}

Uint16 Mix16BitColor(Uint16 col, SDL_Color mix, float k)
{
    BYTE fc0,fc1,fc2;
    fc2 = (col & 31)*(1-k) + mix.b*k;
    fc1 = ((col >> 5) & 63)*(1-k) + mix.g*k;
    fc0 = ((col >> 11) & 31)*(1-k) + mix.r*k;
    if (fc2>31) fc2=31;
    if (fc1>63) fc1=63;
    if (fc0>31) fc0=31;

    BYTE c0,c1,c2;
    c2 = (BYTE)fc2;
    c1 = (BYTE)fc1;
    c0 = (BYTE)fc0;
    return ColorTo16Bit(c0,c1,c2);
}

void DrawHole(int x0, int y0, int r, float grayk, float shiftk)
{
    for (int y=-r; y<=r; y++)
    {
        int adr = game_config.wnd_w*(y0+y) + (x0-r);
        for (int x=-r; x<=r; x++)
        {
            if ((y0+y>=0)&&(y0+y<screen->h)&&
                (x0+x>=0)&&(x0+x<screen->w))
            {
                float k = hole_zeds[x+r][y+r];
                if (k>=0)
                {
                    Uint16 col;
                    BYTE c0,c1,c2;
                    float kk = k*shiftk;
                    if (kk>1) kk=1.0;
                    c0=(BYTE)((31*grayk)*kk);
                    c1=(BYTE)((63*grayk)*kk);
                    c2=(BYTE)((31*grayk)*kk);
                    col = (c0<<11) | (c1<<5) | (c2);
                    ((Uint16*)render_pic->pixels)[adr] = col;
                }
                else
                {
                    BYTE aa_k = hole_aa[x+r][y+r];
                    if (aa_k>0)
                    {
                        Uint16 col = ((Uint16*)render_pic->pixels)[adr];
                        col = Shade16BitColor(col, aa_k/16.0);
                        ((Uint16*)render_pic->pixels)[adr] = col;
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
    mix.r = (BYTE)( 6*(1.0-anim) + 28*anim);
    mix.g = (BYTE)(24*(1.0-anim) + 28*anim);
    mix.b = (BYTE)(18*(1.0-anim) +  0*anim);
       
    Uint16 def_color = ColorTo16Bit(mix.r, mix.g, mix.b);
    for (int y=-r; y<=r; y++)
    {
        int adr = game_config.wnd_w*(y0+y) + (x0-r);
        for (int x=-r; x<=r; x++)
        {
            if ((y0+y>=0)&&(y0+y<screen->h)&&
                (x0+x>=0)&&(x0+x<screen->w))
            {
                BYTE aa_k = key_aa[x+r][y+r];
                if (aa_k>0)
                {
                    Uint16 col;
                    if (aa_k==16)
                    {
                        col = def_color;
                    }
                    else
                    {
                        col = ((Uint16*)screen->pixels)[adr];
                        col = Mix16BitColor(col, mix, aa_k/16.0);
                    }
                    ((Uint16*)screen->pixels)[adr] = col;
                }
            }
            adr++;
        }
    }
}

void TryToShadow(int x, int y, float k)
{
    int adr = y*game_config.wnd_w + x;
    if (shadowbuf[adr]) return;
    Uint16 col = Shade16BitColor( ((Uint16*)render_pic->pixels)[adr], k );
    ((Uint16*)render_pic->pixels)[adr] = col;
}

void RedrawDesk()
{
    SDL_BlitSurface(render_pic, &desk_rect, screen, &desk_rect);
}

void ZeroAnim()
{
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        keys_anim[i].stage = ANIMATION_NONE;
        keys_anim[i].time = 0;
    }
    final_anim.stage = ANIMATION_NONE;
    final_anim.time = 0;

    keys_passed = 0;
    save_key = -1;
}

void RenderLevel()
{
    if (keys_anim) free(keys_anim);
    keys_anim = (Animation*)malloc(game_levels[cur_level].keys_count * sizeof(Animation));

    ZeroAnim();
    
//-- Prepare background --------------------------------------------------------
    SDL_BlitSurface(desk_pic, &desk_rect, render_pic, &desk_rect);
    for (int i=0; i<game_config.wnd_w*game_config.wnd_h; i++)
        shadowbuf[i]=0;

//-- Draw the walls ------------------------------------------------------------
    int b_co = game_levels[cur_level].boxes_count;
    Box *bxs = game_levels[cur_level].boxes;
    for (int i=0; i<b_co; i++)
    {
        SDL_Rect wall_rect;
        wall_rect.x = bxs[i].x1; wall_rect.y = bxs[i].y1;
        wall_rect.w = bxs[i].x2 - bxs[i].x1;
        wall_rect.h = bxs[i].y2 - bxs[i].y1;
        SDL_BlitSurface(wall_pic, &wall_rect, render_pic, &wall_rect);
        FillShadow(bxs[i]);
    }

//-- Generate shadows-----------------------------------------------------------
    for (int i=0; i<b_co; i++)
    {
        int x,y;
        int x1,y1;
        int x2,y2;

        //light vertical lines
        y1 = max(bxs[i].y1-1, 0);
        y2 = min(bxs[i].y2+0, game_config.wnd_h-1);
        x  = bxs[i].x1-1;
        if (x>=0)
            for (y=y1; y<=y2; y++)
                TryToShadow(x,y,0.3);
        x  = bxs[i].x2+0;
        if (x<game_config.wnd_w)
            for (y=y1; y<=y2; y++)
                TryToShadow(x,y,0.3);

        //light horisontal lines
        x1 = max(bxs[i].x1, 0);
        x2 = min(bxs[i].x2-1, game_config.wnd_w-1);
        y  = bxs[i].y1-1;
        if (y>=0)
            for (x=x1; x<=x2; x++)
                TryToShadow(x,y,0.3);
        y  = bxs[i].y2+0;
        if (y<game_config.wnd_h)
            for (x=x1; x<=x2; x++)
                TryToShadow(x,y,0.3);

        //dark vertical lines
        y1 = max(bxs[i].y1-1, 0);
        y2 = min(bxs[i].y2+0, game_config.wnd_h-1);
        x  = bxs[i].x1-2;
        if (x>=0)
            for (y=y1; y<=y2; y++)
                TryToShadow(x,y,0.6);
        x  = bxs[i].x2+1;
        if (x<game_config.wnd_w)
            for (y=y1; y<=y2; y++)
                TryToShadow(x,y,0.6);

        //dark horisontal lines
        x1 = max(bxs[i].x1-1, 0);
        x2 = min(bxs[i].x2+0, game_config.wnd_w-1);
        y  = bxs[i].y1-2;
        if (y>=0)
            for (x=x1; x<=x2; x++)
                TryToShadow(x,y,0.6);
        y  = bxs[i].y2+1;
        if (y<game_config.wnd_h)
            for (x=x1; x<=x2; x++)
                TryToShadow(x,y,0.6);
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
    DrawHole( game_levels[cur_level].fins[0].x,
              game_levels[cur_level].fins[0].y,
              game_config.hole_r,
              0.85, 0.50 );

    if (game_levels[cur_level].keys_count == 0)
    {
        SDL_Rect om_rect;
        int om_x0 = game_levels[cur_level].fins[0].x - fin_pic->w/2;
        int om_y0 = game_levels[cur_level].fins[0].y - fin_pic->h/2;
        om_rect.x = om_x0; om_rect.y = om_y0;
        om_rect.w = fin_pic->w; om_rect.h = fin_pic->h;
        SDL_BlitSurface(fin_pic, NULL, render_pic, &om_rect);
    }
//------------------------------------------------------------------------------
}

#define PHYS_BALL_SHIFT  0.02
#define PHYS_WALL_HEIGHT ((game_config.ball_r+1)*(1+PHYS_BALL_SHIFT)*2 / PHYS_SCALE)
#define PHYS_WALL_WIDTH  40.0
int first_init = 1;
void InitState()
{
    dGeomID wall;
    
    float px,py;
    px=game_levels[cur_level].init.x; py=game_levels[cur_level].init.y;
    prev_px=px; prev_py=py;

    if (!first_init) dWorldDestroy(world);
    first_init = 0;

    fall = 0;
    fall_fixed = 0;

    // create world
    world = dWorldCreate ();
    space = dHashSpaceCreate (0);
    dWorldSetGravity (world,0,0, -GRAV_CONST*0.5);
    dWorldSetCFM (world,1e-5);
    plane = dCreatePlane (space,0,0,1,0);

    //TODO: adjust
    dWorldSetContactSurfaceLayer(world, 0.00001f);
    dWorldSetContactMaxCorrectingVel(world,1);

    int b_co = game_levels[cur_level].boxes_count;
    Box *bxs = game_levels[cur_level].boxes;

    for (int i=0; i<b_co; i++)
    {
        float boxr_x, boxr_y, boxr_w, boxr_h;
        boxr_x=(bxs[i].x2 + bxs[i].x1)/2.0;
        boxr_y=(bxs[i].y2 + bxs[i].y1)/2.0;
        boxr_w=(bxs[i].x2 - bxs[i].x1);
        boxr_h=(bxs[i].y2 - bxs[i].y1);
        wall = dCreateBox(space, boxr_w/PHYS_SCALE, boxr_h/PHYS_SCALE, PHYS_WALL_HEIGHT);
        dGeomSetPosition(wall, boxr_x/PHYS_SCALE, boxr_y/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);
    }

//------------------------------------------------------------------------------

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, PHYS_WALL_WIDTH/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, game_config.wnd_w/2.0/PHYS_SCALE, (-PHYS_WALL_WIDTH/2.0)/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, PHYS_WALL_WIDTH/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, game_config.wnd_w/2.0/PHYS_SCALE, (game_config.wnd_h+PHYS_WALL_WIDTH/2.0)/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, PHYS_WALL_WIDTH/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, (-PHYS_WALL_WIDTH)/2.0/PHYS_SCALE, game_config.wnd_h/2.0/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, PHYS_WALL_WIDTH/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, (game_config.wnd_w+PHYS_WALL_WIDTH/2.0)/PHYS_SCALE, game_config.wnd_h/2.0/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);
    //-- top --------
    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_WALL_WIDTH/PHYS_SCALE);
    dGeomSetPosition(wall, game_config.wnd_w/2.0/PHYS_SCALE, game_config.wnd_h/2.0/PHYS_SCALE, (PHYS_WALL_HEIGHT + PHYS_WALL_WIDTH/2.0/PHYS_SCALE));
    //--------------

    contactgroup = dJointGroupCreate(0);
    // create object
    body = dBodyCreate (world);
    geom = dCreateSphere (space, game_config.ball_r/PHYS_SCALE);
    dMassSetSphere (&m,1,game_config.ball_r/PHYS_SCALE);
    dBodySetMass (body,&m);
    dGeomSetBody (geom,body);
    // set initial position
    int ix,iy;
    if (save_key<0)
    {
        ix = px;
        iy = py;
    }
    else
    {
        ix = game_levels[cur_level].keys[save_key].x;
        iy = game_levels[cur_level].keys[save_key].y;
    }
    dBodySetPosition ( body, ix/PHYS_SCALE, iy/PHYS_SCALE,
                       (game_config.ball_r/PHYS_SCALE)*(1+PHYS_BALL_SHIFT) );
//------------------------------------------------------------------------------

    RedrawDesk();
}

#define PHYS_HOLE_DEPTH (game_config.ball_r*2 / PHYS_SCALE)
void GoFall(Point hole)
{
    dGeomID wall;

    dWorldSetGravity (world,0,0, -(GRAV_CONST*0.5)*1.6);

    dGeomPlaneSetParams (plane, 0,0,1, -PHYS_HOLE_DEPTH);

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, game_config.ball_r/PHYS_SCALE, PHYS_HOLE_DEPTH);
    dGeomSetPosition(wall, hole.x/PHYS_SCALE, (hole.y-game_config.hole_r-game_config.ball_r/2.0)/PHYS_SCALE, -PHYS_HOLE_DEPTH/2.0);

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, game_config.ball_r/PHYS_SCALE, PHYS_HOLE_DEPTH);
    dGeomSetPosition(wall, hole.x/PHYS_SCALE, (hole.y+game_config.hole_r+game_config.ball_r/2.0)/PHYS_SCALE, -PHYS_HOLE_DEPTH/2.0);

    wall = dCreateBox(space, game_config.ball_r/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_HOLE_DEPTH);
    dGeomSetPosition(wall, (hole.x+game_config.hole_r+game_config.ball_r/2.0)/PHYS_SCALE, hole.y/PHYS_SCALE, -PHYS_HOLE_DEPTH/2.0);

    wall = dCreateBox(space, game_config.ball_r/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_HOLE_DEPTH);
    dGeomSetPosition(wall, (hole.x-game_config.hole_r-game_config.ball_r/2.0)/PHYS_SCALE, hole.y/PHYS_SCALE, -PHYS_HOLE_DEPTH/2.0);

    //----

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, game_config.ball_r/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, hole.x/PHYS_SCALE, (hole.y-game_config.hole_r-game_config.ball_r*3.0/2.0)/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, game_config.ball_r/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, hole.x/PHYS_SCALE, (hole.y+game_config.hole_r+game_config.ball_r*3.0/2.0)/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, game_config.ball_r/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, (hole.x+game_config.hole_r+game_config.ball_r*3.0/2.0)/PHYS_SCALE, hole.y/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, game_config.ball_r/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, (hole.x-game_config.hole_r-game_config.ball_r*3.0/2.0)/PHYS_SCALE, hole.y/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    fall = 1;
    fall_hole = hole;
}

void GoFallFixed(Point hole)
{
    dGeomID wall;

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, game_config.ball_r/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, hole.x/PHYS_SCALE, (hole.y-game_config.hole_r-game_config.ball_r/2.0)/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, game_config.wnd_w/PHYS_SCALE, game_config.ball_r/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, hole.x/PHYS_SCALE, (hole.y+game_config.hole_r+game_config.ball_r/2.0)/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, game_config.ball_r/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, (hole.x+game_config.hole_r+game_config.ball_r/2.0)/PHYS_SCALE, hole.y/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    wall = dCreateBox(space, game_config.ball_r/PHYS_SCALE, game_config.wnd_h/PHYS_SCALE, PHYS_WALL_HEIGHT);
    dGeomSetPosition(wall, (hole.x-game_config.hole_r-game_config.ball_r/2.0)/PHYS_SCALE, hole.y/PHYS_SCALE, PHYS_WALL_HEIGHT/2.0);

    fall_fixed = 1;
}


SDL_Surface* CreateSurface(Uint32 flags,int width,int height,const SDL_Surface* display)
{
  const SDL_PixelFormat fmt = *(display->format);
  return SDL_CreateRGBSurface(flags,width,height,
                  fmt.BitsPerPixel,
                  fmt.Rmask,fmt.Gmask,fmt.Bmask,fmt.Amask);
}

void DrawBlended(SDL_Surface* from, SDL_Surface* to, float k)
{
    for (int i=0; i<from->w*from->h; i++)
    {
        Uint32 c = ((Uint32*)from->pixels)[i];
        BYTE a = ((c & from->format->Amask) >> from->format->Ashift) * k;
        c &= ~from->format->Amask;
        c |= a << from->format->Ashift;
        ((Uint32*)to->pixels)[i] = c;
    }
}

SDL_Surface* LoadImg(char *file)
{
    SDL_Surface* res = IMG_Load(file);
    if (res==NULL)
    {
        fprintf(stderr, "Main: can't load image '%s'.\n", file);
        LoadImgErrors++;
    }
    return res;
}


//------------------------------------------------------------------------------
//-- Matrix operations ---------------------------------------------------------
//------------------------------------------------------------------------------
void MatrixInversion(float **A, int order, float **Y);
int GetMinor(float **src, float **dest, int row, int col, int order);
float CalcDeterminant( float **mat, int order);

// matrix inversion
// the result is put in Y
void MatrixInversion(float **A, int order, float **Y)
{
     // get the determinant of a
     float det = 1.0/CalcDeterminant(A,order);

     // memory allocation
     float *temp = malloc( (order-1)*(order-1)*sizeof(float) );
     float **minor = malloc( (order-1)*sizeof(float*) );
     for(int i=0;i<order-1;i++)
         minor[i] = temp+(i*(order-1));

     for(int j=0;j<order;j++)
     {
         for(int i=0;i<order;i++)
         {
             // get the co-factor (matrix) of A(j,i)
             GetMinor(A,minor,j,i,order);
             Y[i][j] = det*CalcDeterminant(minor,order-1);
             if( (i+j)%2 == 1)
                 Y[i][j] = -Y[i][j];
         }
     }

     // release memory
     //minor[0];
     //minor;
     free(temp);
     free(minor);
 }

// calculate the cofactor of element (row,col)
int GetMinor(float **src, float **dest, int row, int col, int order)
{
    // indicate which col and row is being copied to dest
    int colCount=0,rowCount=0;

    for(int i = 0; i < order; i++ )
    {
         if( i != row )
         {
             colCount = 0;
            for(int j = 0; j < order; j++ )
            {
                 // when j is not the element
                 if( j != col )
                 {
                     dest[rowCount][colCount] = src[i][j];
                     colCount++;
                 }
            }
            rowCount++;
         }
     }

     return 1;
 }

// Calculate the determinant recursively.
float CalcDeterminant(float **mat, int order)
{
    // order must be >= 0
    // stop the recursion when matrix is a single element
    if( order == 1 )
        return mat[0][0];

    // the determinant value
    float det = 0;

     // allocate the cofactor matrix
    float **minor;
    minor = malloc( sizeof(float*)*(order-1) );
    for(int i=0;i<order-1;i++)
        minor[i] = malloc( sizeof(float)*(order-1) );

    for(int i = 0; i < order; i++ )
    {
        // get minor of element (0,i)
        GetMinor( mat, minor, 0, i , order);
        // the recursion is here!
        det += pow( -1.0, i ) * mat[0][i] * CalcDeterminant( minor,order-1 );
    }

    // release memory
    for(int i=0;i<order-1;i++)
        free(minor[i]);
    free(minor);

    return det;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//-- Draw the ball -------------------------------------------------------------
//------------------------------------------------------------------------------
float matrix[16];
float **a, **a_1;
float **zeds;
BYTE **ball_aa;
int rad;

void texSmooth(BYTE* c0, BYTE* c1, BYTE* c2, SDL_Color secc, float fixx)
{
    *c0 = (*c0)*(0.5+fixx) + secc.r*(0.5-fixx);
    *c1 = (*c1)*(0.5+fixx) + secc.g*(0.5-fixx);
    *c2 = (*c2)*(0.5+fixx) + secc.b*(0.5-fixx);
    if (*c0>31) *c0=31; if (*c1>63) *c1=63; if (*c2>31) *c1=31;
}

void DrawBall(int tk_px, int tk_py, float poss_z, const dReal *R, SDL_Color bcolor)
{
    //prepare to draw the ball
    matrix[0]=R[0];
    matrix[1]=R[4];
    matrix[2]=R[8];
    matrix[3]=0;
    matrix[4]=R[1];
    matrix[5]=R[5];
    matrix[6]=R[9];
    matrix[7]=0;
    matrix[8]=R[2];
    matrix[9]=R[6];
    matrix[10]=R[10];
    matrix[11]=0;
    matrix[12]=0;
    matrix[13]=0;
    matrix[14]=0;
    matrix[15]=1;

    a[0][0]=matrix[0];  a[0][1]=matrix[1];  a[0][2]=matrix[2];
    a[1][0]=matrix[4];  a[1][1]=matrix[5];  a[1][2]=matrix[6];
    a[2][0]=matrix[8];  a[2][1]=matrix[9];  a[2][2]=matrix[10];

    MatrixInversion(a, 3, a_1);

    //redraw the ball at new location
    if(SDL_MUSTLOCK(screen))
    {
        if(SDL_LockSurface(screen) < 0) return;
    }

    BYTE c0,c1,c2;
    for (int x=-rad; x<=rad; x++)
    for (int y=-rad; y<=rad; y++)
    {
            float z;

            if ((tk_py+y>=0)&&(tk_py+y<screen->h)&&
                (tk_px+x>=0)&&(tk_px+x<screen->w)&&
                (z = zeds[x+rad][y+rad]) >= 0)
            {
                    float ksi,fi;
                    float x0,y0,z0;

                    x0 = a_1[0][0]*x + a_1[1][0]*y +a_1[2][0]*z;
                    y0 = a_1[0][1]*x + a_1[1][1]*y +a_1[2][1]*z;
                    z0 = a_1[0][2]*x + a_1[1][2]*y +a_1[2][2]*z;

                    ksi = (float)z0/rad;
                    
                    if (x0!=0) fi  = (float)y0/x0;
                          else fi  = 0;

                    float fip;
                    if (y0!=0) fip  = (float)x0/y0;
                          else fip  = 0;

                    #define COS_PI_4   0.7071
                    #define COS_PI_2   0
                    #define COS_3PI_4 -COS_PI_4

                    float ksim = ksi - COS_PI_2;
                    SDL_Color secc;

                    if (ksi>=COS_PI_2)
                    {
                        if (fi<=0)
                        {
                            c0=bcolor.r; c1=bcolor.g; c2=bcolor.b;
                            secc.r=31; secc.g=63; secc.b=31;
                        }
                        else
                        {
                            c0=31; c1=63; c2=31;
                            secc.r=bcolor.r; secc.g=bcolor.g; secc.b=bcolor.b;
                        }
                    }
                    else
                    {
                        if (fi<=0)
                        {
                            c0=31; c1=63; c2=31;
                            secc.r=bcolor.r; secc.g=bcolor.g; secc.b=bcolor.b;
                        }
                        else
                        {
                            c0=bcolor.r; c1=bcolor.g; c2=bcolor.b;
                            secc.r=31; secc.g=63; secc.b=31;
                        }
                    }

                    if ((fi<0.04) && (fi>=0.0))
                        texSmooth(&c0,&c1,&c2, secc, fi/0.08);
                    else
                    if ((fi>-0.04) && (fi<0.0))
                        texSmooth(&c0,&c1,&c2, secc, -fi/0.08);
                    else

                    if ((ksim<0.02) && (ksim>=0.0))
                        texSmooth(&c0,&c1,&c2, secc, ksim/0.04);
                    else
                    if ((ksim>-0.02) && (ksim<0.0))
                        texSmooth(&c0,&c1,&c2, secc, -ksim/0.04);
                    else

                    if ((fip<0.04) && (fip>=0.0))
                        texSmooth(&c0,&c1,&c2, secc, fip/0.08);
                    else
                    if ((fip>-0.04) && (fip<0.0))
                        texSmooth(&c0,&c1,&c2, secc, -fip/0.08);

                    //float cosa = z/rad;
                    //if (cosa<0) cosa=0;

                    //shader
                    float mz = (rad-poss_z*PHYS_SCALE);
                    if (mz>0.6*rad) mz=0.6*rad;
                    if (mz<0) mz=0;
                    float cosa = (z-mz)/rad;
                    if (cosa<0) cosa=0;
                    //if (cosa>1) cosa=1;

                    c0 = (BYTE)((float)c0*cosa);
                    c1 = (BYTE)((float)c1*cosa);
                    c2 = (BYTE)((float)c2*cosa);

                    int adr = ((tk_py+y)*screen->w + (tk_px+x));
                    Uint16 col = (c0<<11) | (c1<<5) | (c2);
                    ((Uint16*)screen->pixels)[adr] = col;
            }
            else
            {
                BYTE aa_k = ball_aa[x+rad][y+rad];
                if (aa_k>0)
                {
                    int adr = ((tk_py+y)*screen->w + (tk_px+x));
                    Uint16 col = ((Uint16*)screen->pixels)[adr];
                    col = Shade16BitColor(col, aa_k/16.0);
                    ((Uint16*)screen->pixels)[adr] = col;
                }
            }
    }

    if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
}
//------------------------------------------------------------------------------

SDL_Surface *fin_pic_blended;
#define MAX_ANIM_TIME 0.3
void UpdateBufAnimation(float do_phys_step)
{
    int kx,ky,kr;
    int hx,hy,hr;
    SDL_Rect fin_rect, key_rect;

    hx = game_levels[cur_level].fins[0].x;
    hy = game_levels[cur_level].fins[0].y;
    hr = game_config.hole_r;

    fin_rect.x=hx-hr; fin_rect.y=hy-hr;
    fin_rect.w=hr*2+1; fin_rect.h=hr*2+1;

    //restore background around final hole
    if (game_levels[cur_level].keys_count > 0)
    if (keys_passed == game_levels[cur_level].keys_count)
    {
        SDL_BlitSurface(render_pic, &fin_rect, screen, &fin_rect);
    }

    //restore background around keys
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        kx = game_levels[cur_level].keys[i].x;
        ky = game_levels[cur_level].keys[i].y;
        kr = game_config.key_r;

        key_rect.x=kx-kr; key_rect.y=ky-kr;
        key_rect.w=kr*2+1; key_rect.h=kr*2+1;

        SDL_BlitSurface(render_pic, &key_rect, screen, &key_rect);
    }

    //draw keys with actual animation stages
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        kx = game_levels[cur_level].keys[i].x;
        ky = game_levels[cur_level].keys[i].y;
        kr = game_config.key_r;

        key_rect.x=kx-kr; key_rect.y=ky-kr;
        key_rect.w=kr*2+1; key_rect.h=kr*2+1;

        float kk = 0;
        if (keys_anim[i].stage == ANIMATION_NONE) kk=0;
        else if (keys_anim[i].stage == ANIMATION_FINISHED) kk=1;
        else if (keys_anim[i].stage == ANIMATION_PLAYING)
        {
            keys_anim[i].time += do_phys_step;
            kk = keys_anim[i].time / MAX_ANIM_TIME;
            if (kk>1) kk=1;
        }
        DrawKey( kx, ky, kr, kk );
    }

    //draw final hole with actual animation stage
    if (game_levels[cur_level].keys_count > 0)
    if (keys_passed == game_levels[cur_level].keys_count)
    {
        float kk=0;
        if (final_anim.stage == ANIMATION_PLAYING)
        {
            final_anim.time += do_phys_step;
            kk = final_anim.time / MAX_ANIM_TIME;
            if (kk>1) kk=1;
        }
        else if (final_anim.stage == ANIMATION_FINISHED)
        {
            kk=1;
        }

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
            if (keys_anim[i].time >= MAX_ANIM_TIME) keys_anim[i].stage = ANIMATION_FINISHED;
        }
    }

    if (final_anim.stage == ANIMATION_PLAYING)
    {
        SDL_Rect om_rect;
        om_rect.x = game_levels[cur_level].fins[0].x - fin_pic->w/2;
        om_rect.y = game_levels[cur_level].fins[0].y - fin_pic->h/2;
        om_rect.w = fin_pic->w; om_rect.h = fin_pic->h;
        SDL_UpdateRect(screen, om_rect.x, om_rect.y, om_rect.w, om_rect.h);
        if (final_anim.time >= MAX_ANIM_TIME) final_anim.stage = ANIMATION_FINISHED;
    }
}

//------------------------------------------------------------------------------

int fastchange_step;
SDL_TimerID fastchange_timer = 0;
int must_fastchange = 0;
int fastchange_dostep;
#define FASTCHANGE_INTERVAL 1000
Uint32 fastchange_callback (Uint32 interval, void *param)
{
    fastchange_dostep = fastchange_step;
    must_fastchange = 1;
    return interval;
}

void StopFastChange()
{
    if (fastchange_timer)
    {
        SDL_RemoveTimer(fastchange_timer);
        fastchange_timer=0;
    }
    must_fastchange = 0;
}

//------------------------------------------------------------------------------

#define FONT_NAME "LiberationMono-Regular.ttf"
#define FIRST_TRY_DELAY 500
#define PHYS_MIN_FALL_VEL 0.09

void render_window(int start_level)
{
	SDL_Event event;

        game_config = GetGameConfig();
        game_levels = GetGameLevels();
        game_levels_count = GetGameLevelsCount();
        vibro_force_k = GetVibroForce()/100.0;

        //== font and labels ===================================================
        TTF_Font* font = TTF_OpenFont(MFONTDIR FONT_NAME, 24);
        if (!font)
        {
            fprintf(stderr, "Main: can't load font '" MFONTDIR FONT_NAME "'. Exiting.\n");
            return;
        }
        SDL_Color fontColor;
        
//--enable-rgb-swap
#ifndef RGBSWAP
        fontColor.r = 231; fontColor.g = 190; fontColor.b = 114;
#else
        fontColor.r = 114; fontColor.g = 190; fontColor.b = 231;
#endif

        SDL_Surface *levelTextSurface, *infoTextSurface;
        levelTextSurface = TTF_RenderText_Blended(font, "Level X/Y", fontColor);
        infoTextSurface  = TTF_RenderText_Blended(font, "Touch the screen to continue", fontColor);
        SDL_Rect levelTextLocation = {  (game_config.wnd_w - levelTextSurface->w) / 2,
                                        levelTextSurface->h * 2,
                                        0, 0 };
        //== buttons ===========================================================
        SDL_Surface *back_pic, *forward_pic, *reset_pic, *exit_pic;
        SDL_Surface *back_p_pic, *forward_p_pic, *reset_p_pic; //, *exit_p_pic;
        SDL_Surface *back_i_pic, *forward_i_pic, *reset_i_pic;

	// Load Pictures
        desk_pic = LoadImg(MDIR "desk.png");
        wall_pic = LoadImg(MDIR "wall.png");
        fin_pic  = LoadImg(MDIR "fin.png");

        back_pic    = LoadImg(MDIR "prev.png");
        forward_pic = LoadImg(MDIR "next.png");
        reset_pic   = LoadImg(MDIR "reset.png");
        exit_pic    = LoadImg(MDIR "close.png");
        //----------------------------------------------------------------------
        back_i_pic    = LoadImg(MDIR "prev-i.png");
        forward_i_pic = LoadImg(MDIR "next-i.png");
        reset_i_pic   = LoadImg(MDIR "reset-i.png");
        //----------------------------------------------------------------------
        back_p_pic    = LoadImg(MDIR "prev-p.png");
        forward_p_pic = LoadImg(MDIR "next-p.png");
        reset_p_pic   = LoadImg(MDIR "reset-p.png");
        //exit_p_pic    = IMG_Load(MDIR "exit-p.png");

        if (LoadImgErrors>0)
        {
            fprintf(stderr, "Main: some images was not loaded. Exiting.\n");
            return;
        }

        //== pos
        int btnw = back_pic->w;
        int btnh = back_pic->h;
        SDL_Rect gui_rect_1, gui_rect_2, gui_rect_3, gui_rect_4;
        gui_rect_1.y = gui_rect_2.y = gui_rect_3.y = gui_rect_4.y = levelTextSurface->h * (2*2 + 1);
        int btnspace = (game_config.wnd_w - btnw*4) / 5;
        gui_rect_1.x = btnspace*1 + btnw*0;
        gui_rect_2.x = btnspace*2 + btnw*1;
        gui_rect_3.x = btnspace*3 + btnw*2;
        gui_rect_4.x = btnspace*4 + btnw*3;
        //== for click detection
        Box gui_box_1, gui_box_2, gui_box_3, gui_box_4;
        gui_box_1.x1 = gui_rect_1.x;        gui_box_1.y1 = gui_rect_1.y;
        gui_box_1.x2 = gui_rect_1.x + btnw; gui_box_1.y2 = gui_rect_1.y + btnh;
        gui_box_2.x1 = gui_rect_2.x;        gui_box_2.y1 = gui_rect_2.y;
        gui_box_2.x2 = gui_rect_2.x + btnw; gui_box_2.y2 = gui_rect_2.y + btnh;
        gui_box_3.x1 = gui_rect_3.x;        gui_box_3.y1 = gui_rect_3.y;
        gui_box_3.x2 = gui_rect_3.x + btnw; gui_box_3.y2 = gui_rect_3.y + btnh;
        gui_box_4.x1 = gui_rect_4.x;        gui_box_4.y1 = gui_rect_4.y;
        gui_box_4.x2 = gui_rect_4.x + btnw; gui_box_4.y2 = gui_rect_4.y + btnh;
        //==
        SDL_Rect infoTextLocation  = {  (game_config.wnd_w - infoTextSurface->w) / 2,
                                        game_config.wnd_h/2 + levelTextSurface->h*(2*2 + 1)/2 + btnh/2 - infoTextSurface->h/2,
                                        0, 0 };

        //== initializing window ===============================================
        fullscreen = 1;
        Uint32 sdl_flags = SDL_SWSURFACE;
        if (game_config.fullscreen) sdl_flags |= SDL_FULLSCREEN;
	screen = SDL_SetVideoMode(game_config.wnd_w, game_config.wnd_h, 16, sdl_flags);
        if (screen==NULL)
        {
            fprintf(stderr, "Main: can't set video mode. Exiting.\n");
            return;
        }
	SDL_WM_SetCaption("Mokomaze", "Mokomaze");

        //create surface for rendering the level
        render_pic = CreateSurface(SDL_SWSURFACE, game_config.wnd_w, game_config.wnd_h, screen);

        if (game_config.fullscreen)
        {
            SDL_ShowCursor(!fullscreen);
        }

        desk_rect.x=0; desk_rect.y=0;
        desk_rect.w=game_config.wnd_w; desk_rect.h=game_config.wnd_h;

	SDL_Rect ball_rect;
	SDL_Rect hole_rect;
        hole_rect.w=game_config.hole_r * 2; hole_rect.h=game_config.hole_r * 2; //

        //alloc buffer for speed up drawing of the shadows
        shadowbuf = malloc(game_config.wnd_w * game_config.wnd_h);

        SDL_Color ballColor;
        ballColor.r = 31; ballColor.g = 31; ballColor.b = 0;

        fin_pic_blended = CreateSurface(SDL_SWSURFACE, fin_pic->w, fin_pic->h, fin_pic); // blended final image

        //init matrices for drawing the ball
        a = (float**)alloc2d(3,3,sizeof(float),sizeof(float*));
        a_1 = (float**)alloc2d(3,3,sizeof(float),sizeof(float*));
        
        //alloc ball z-values array
        rad = game_config.ball_r-1; //
        int diam = rad*2 + 1;
        zeds = (float**)alloc2d(diam,diam,sizeof(float),sizeof(float*));
        ball_aa = (BYTE**)alloc2d(diam,diam,sizeof(BYTE),sizeof(BYTE*));
        //calculate z-values
        for (int x=-rad; x<=rad; x++)
        for (int y=-rad; y<=rad; y++)
        {
            float xdist = x*x+y*y;
            int sample_hit=0;

            if (xdist>(rad+1)*(rad+1)) sample_hit=0;
            else if (xdist<(rad-1)*(rad-1)) sample_hit=16;
            else
            {
                for (int ii=0; ii<4; ii++)
                    for (int jj=0; jj<4; jj++)
                        if  ((x-0.3+0.2*ii)*(x-0.3+0.2*ii)+(y-0.3+0.2*jj)*(y-0.3+0.2*jj) <= rad*rad)
                            sample_hit++;
            }
            
            ball_aa[x+rad][y+rad] = sample_hit;
            if (sample_hit == 16) zeds[x+rad][y+rad] = sqrt(rad*rad - x*x - y*y);
            else zeds[x+rad][y+rad] = -1;
        }

        //alloc hole z-values array
        int h_rad = game_config.hole_r;
        int h_diam = h_rad*2 + 1;
        hole_zeds = (float**)alloc2d(h_diam,h_diam,sizeof(float),sizeof(float*));
        hole_aa = (BYTE**)alloc2d(h_diam,h_diam,sizeof(BYTE),sizeof(BYTE*));
        for (int x=-h_rad; x<=h_rad; x++)
        for (int y=-h_rad; y<=h_rad; y++)
        {
            float xdist = x*x+y*y;
            int sample_hit=0;

            if (xdist>(h_rad+1)*(h_rad+1)) sample_hit=0;
            else if (xdist<(h_rad-1)*(h_rad-1)) sample_hit=16;
            else
            {
                for (int ii=0; ii<4; ii++)
                    for (int jj=0; jj<4; jj++)
                        if  ((x-0.3+0.2*ii)*(x-0.3+0.2*ii)+(y-0.3+0.2*jj)*(y-0.3+0.2*jj) <= h_rad*h_rad)
                            sample_hit++;
            }

            hole_aa[x+h_rad][y+h_rad] = sample_hit;
            if (sample_hit == 16) hole_zeds[x+h_rad][y+h_rad] = sqrt(h_rad*h_rad - x*x - y*y)  / h_rad;
            else hole_zeds[x+h_rad][y+h_rad] = -1;
        }

        //alloc array for image of key
        int k_rad = game_config.key_r;
        int k_rad_v = k_rad*75/100;
        int k_diam = k_rad*2 + 1;
        key_aa = (BYTE**)alloc2d(k_diam,k_diam,sizeof(BYTE),sizeof(BYTE*));
        for (int x=-k_rad; x<=k_rad; x++)
        for (int y=-k_rad; y<=k_rad; y++)
        {
            float xdist = x*x+y*y;
            int sample_hit=0;

            if ( (xdist>(k_rad+1)*(k_rad+1)) ||
                 (xdist<(k_rad_v-1)*(k_rad_v-1)) ) sample_hit=0;
            else if ( (xdist<(k_rad-1)*(k_rad-1)) &&
                      (xdist>(k_rad_v+1)*(k_rad_v+1)) ) sample_hit=16;
            else
            {
                for (int ii=0; ii<4; ii++)
                    for (int jj=0; jj<4; jj++)
                    {
                        float rast = (x-0.3+0.2*ii)*(x-0.3+0.2*ii)+(y-0.3+0.2*jj)*(y-0.3+0.2*jj);
                        if ( (rast <= k_rad*k_rad) && (rast >= k_rad_v*k_rad_v) )
                            sample_hit++;
                    }
            }

            key_aa[x+k_rad][y+k_rad] = sample_hit;
        }


        /* Start the accelerometer thread */
	accelerometer_start();

        dInitODE ();


        cur_level=start_level;
        RenderLevel(); InitState();
        game_state = GAME_STATE_NORMAL;
        int first_draw = 1;

        int prev_ticks = SDL_GetTicks();
        float prev_phys_step = 0;

        Point mouse = {0, 0};

        int done = 0;
        
	while(!done) {

                int wasclick=0;

		while(SDL_PollEvent(&event))
                {
                    if (event.type == SDL_QUIT)
                    {
                        done=1;
                    }
                    if (event.type == SDL_MOUSEMOTION)
                    {
                        mouse.x = event.motion.x;
                        mouse.y = event.motion.y;
                    }
                    if (event.type == SDL_MOUSEBUTTONUP)
                    {
                        StopFastChange();
                    }
                    if (event.type == SDL_MOUSEBUTTONDOWN)
                    {
                        if (!fullscreen)
                        {
                            if (inbox(mouse.x,mouse.y, gui_box_1))
                            {
                                if (cur_level > 0)
                                {
                                    SDL_BlitSurface(back_p_pic, NULL, screen, &gui_rect_1);
                                    SDL_UpdateRect(screen, gui_rect_1.x,gui_rect_1.y,gui_rect_1.w,gui_rect_1.h);
                                    
                                    RedrawDesk();
                                    cur_level--;
                                    RenderLevel(); InitState();
                                    game_state = GAME_STATE_NORMAL;
                                    first_draw=1;
                                    wasclick=1; //

                                    fastchange_step = -10;
                                    StopFastChange();
                                    fastchange_timer = SDL_AddTimer(FASTCHANGE_INTERVAL, fastchange_callback, NULL);
                                }
                                continue;
                            }

                            if (inbox(mouse.x,mouse.y, gui_box_2))
                            {
                                if (cur_level < game_levels_count-1)
                                {
                                    SDL_BlitSurface(forward_p_pic, NULL, screen, &gui_rect_2);
                                    SDL_UpdateRect(screen, gui_rect_2.x,gui_rect_2.y,gui_rect_2.w,gui_rect_2.h);

                                    RedrawDesk();
                                    cur_level++;
                                    RenderLevel(); InitState();
                                    game_state = GAME_STATE_NORMAL;
                                    first_draw=1;
                                    wasclick=1; //

                                    fastchange_step = +10;
                                    StopFastChange();
                                    fastchange_timer = SDL_AddTimer(FASTCHANGE_INTERVAL, fastchange_callback, NULL);
                                }
                                continue;
                            }

                            if (inbox(mouse.x,mouse.y, gui_box_3))
                            {
                                if (cur_level>0)
                                {
                                    SDL_BlitSurface(reset_p_pic, NULL, screen, &gui_rect_3);
                                    SDL_UpdateRect(screen, gui_rect_3.x,gui_rect_3.y,gui_rect_3.w,gui_rect_3.h);

                                    RedrawDesk();
                                    cur_level=0;
                                    RenderLevel(); InitState();
                                    game_state = GAME_STATE_NORMAL;
                                    first_draw=1;
                                    wasclick=1; //
                                }
                                continue;
                            }

                            if (inbox(mouse.x,mouse.y, gui_box_4))
                            {
                                done=1;
                                continue;
                            }
                        }

                        fullscreen = !fullscreen;
                        if (!fullscreen)
                        {
                            wasclick=1;
                        }
                        else
                        {
                            RedrawDesk();
                            first_draw=1;
                        }

                        if (game_config.fullscreen)
                        {
                            SDL_WM_ToggleFullScreen(screen);
                            SDL_ShowCursor(!fullscreen);
                        }

                        prev_ticks = SDL_GetTicks();
                        prev_phys_step = 0;

                    } //if (event.type == SDL_MOUSEBUTTONDOWN)
		}

                if ((!fullscreen) && (!wasclick) && (must_fastchange))
                {
                    int new_cur_level = cur_level + fastchange_dostep;
                    if (new_cur_level > game_levels_count-1) new_cur_level = game_levels_count-1;
                    if (new_cur_level < 0) new_cur_level = 0;

                    if (new_cur_level != cur_level)
                    {

                        if (fastchange_dostep < 0)
                        {
                            SDL_BlitSurface(back_p_pic, NULL, screen, &gui_rect_1);
                            SDL_UpdateRect(screen, gui_rect_1.x,gui_rect_1.y,gui_rect_1.w,gui_rect_1.h);
                        }
                        else
                        {
                            SDL_BlitSurface(forward_p_pic, NULL, screen, &gui_rect_2);
                            SDL_UpdateRect(screen, gui_rect_2.x,gui_rect_2.y,gui_rect_2.w,gui_rect_2.h);
                        }

                        RedrawDesk();
                        cur_level = new_cur_level;
                        RenderLevel(); InitState();
                        game_state = GAME_STATE_NORMAL;
                        first_draw=1;
                        wasclick=1; //

                    }
                    must_fastchange = 0;
                }

                if ((!fullscreen) && (!wasclick))
                {
                    SDL_Delay(game_config.f_delay);
                    continue;
                }

//==============================================================================
//== Phys. step ================================================================
//==============================================================================

                int ticks = SDL_GetTicks();
                int delta_ticks = ticks - prev_ticks;
                prev_ticks = ticks;
                if (delta_ticks<=0) delta_ticks=1;
                if (delta_ticks>1000/15) delta_ticks=1000/15;

                float ax =  getacx();
                float ay = -getacy();
                float forcex = ax*0.45;
                float forcey = ay*0.45;

                const dReal *poss;
                const dReal *R;

                //printf("%d\n", delta_ticks);
                float phys_step = (0.013/30.0) * delta_ticks;
                if (prev_phys_step>0) phys_step = prev_phys_step + (phys_step - prev_phys_step) * 0.6;
                prev_phys_step = phys_step;

                float do_phys_step = phys_step;
                if (do_phys_step > 0.013/30.0*250) do_phys_step = 0.013/30.0*250;
                if (do_phys_step <= 0) do_phys_step = 0.013/30.0; //

                int wnanc=0;

                if (fullscreen)
                    for (int i=0; i<3; i++)
                    {

                        const dReal * Position = dBodyGetPosition   (body);
                        const dReal * Rotation = dBodyGetRotation   (body);
                        const dReal * Quaternion = dBodyGetQuaternion (body);
                        const dReal * LinearVel = dBodyGetLinearVel  (body);
                        const dReal * AngularVel = dBodyGetAngularVel (body);

                        dReal xPosition[3];
                        dReal xRotation[12];
                        dReal xQuaternion[4];
                        dReal xLinearVel[3];
                        dReal xAngularVel[3];

                        for (int j=0; j<3; j++)
                        {
                            xPosition[j] = Position[j];
                            xLinearVel[j] = LinearVel[j];
                            xAngularVel[j] = AngularVel[j];
                        }
                        for (int j=0; j<12; j++)
                            xRotation[j] = Rotation[j];
                        for (int j=0; j<4; j++)
                            xQuaternion[j] = Quaternion[j];

                        //if (i==0)
                        if (!wnanc && !fall)
                        {
                            dBodyAddForce(body, forcex, 0, 0);
                            dBodyAddForce(body, 0, forcey, 0);

                            //printf("%.4f  %.4f  %.4f\n", LinearVel[0], LinearVel[1], LinearVel[2]);
                            //printf("%.4f  %.4f  %.4f\n", AngularVel[0], AngularVel[1], AngularVel[2]);
                            float qu;

                            qu=0;
                            if (LinearVel[0] >  0.002) qu=-1;
                            if (LinearVel[0] < -0.002) qu= 1;
                            if (qu!=0) dBodyAddForce(body, qu*0.0017*( 0.5*GRAV_CONST*cos(asin(ax)) ), 0, 0);

                            qu=0;
                            if (LinearVel[1] >  0.002) qu=-1;
                            if (LinearVel[1] < -0.002) qu= 1;
                            if (qu!=0) dBodyAddForce(body, 0, qu*0.0017*( 0.5*GRAV_CONST*cos(asin(ay)) ), 0);

                            qu=0;
                            if (AngularVel[2] >  0.003) qu=-1;
                            if (AngularVel[2] < -0.003) qu= 1;
                            if (qu!=0) dBodyAddTorque(body, 0,0, qu*0.0005);
                        }
                        else //fall
                        {
                            float cpx = Position[0]*PHYS_SCALE;
                            float cpy = Position[1]*PHYS_SCALE;
                            float tkdi = calcdist(fall_hole.x,fall_hole.y, cpx,cpy);
                            if (tkdi > game_config.hole_r)
                            {
                                float tfx,tfy;
                                float fo = 0.25 * (tkdi-game_config.hole_r) / (game_config.hole_r*(sqrt(2)-1));
                                tfx = ((fall_hole.x - cpx) / tkdi) * fo;
                                tfy = ((fall_hole.y - cpy) / tkdi) * fo;
                                dBodyAddForce(body, tfx, tfy, 0);
                                //printf("%.4f %.4f\n",tfx,tfy);
                            }
                        }

                        //======================================================
                        dSpaceCollide (space,0,&nearCallback);
                        //dWorldSetQuickStepNumIterations(world,20);
                        //dWorldQuickStep (world,0.01);
                        //dWorldStep (world,0.02);
                        dWorldStep (world, do_phys_step);
                        dJointGroupEmpty (contactgroup);
                        //======================================================

                        poss = dGeomGetPosition (geom);

                        //printf("%.4f %.4f %.4f\n",poss[0],poss[1],poss[2]);

                        int nanc=0;
                        for (int j=0; j<3; j++)
                        {
                            if ( poss[j] <= 0.0 )
                            {}
                            else if ( poss[j] > 0.0)
                            {}
                            else //poss[j] is NaN
                            { nanc=1; break; }
                        }

                        if (nanc)
                        {
                            dBodySetPosition   (body, xPosition[0],xPosition[1],xPosition[2]);
                            dBodySetRotation   (body, xRotation);
                            dBodySetQuaternion (body, xQuaternion);
                            dBodySetLinearVel  (body, xLinearVel[0],xLinearVel[1],xLinearVel[2]);
                            dBodySetAngularVel (body, xAngularVel[0],xAngularVel[1],xAngularVel[2]);
                            wnanc=1;
                        }

                    }

                //determing new position of the ball
                poss = dGeomGetPosition (geom);
                R = dGeomGetRotation (geom);
                int tk_px=poss[0]*PHYS_SCALE, tk_py=poss[1]*PHYS_SCALE;

                //test if it fall out
                testbump(tk_px, tk_py);

                if (fall)
                {
                    const dReal *lv = dBodyGetLinearVel(body);
                    //printf ("%f %f %f\n",lv[0],lv[1],lv[2]);
                    if ( sqrt(lv[0]*lv[0]+lv[1]*lv[1]+lv[2]*lv[2]) < PHYS_MIN_FALL_VEL )
                        if (poss[2]*PHYS_SCALE <= -game_config.ball_r*3.0/4.0)
                            game_state = new_game_state;
                }
//------------------------------------------------------------------------------
                
                //mem leaks?

                //restore the background
                ball_rect.w=game_config.ball_r * 2; ball_rect.h=game_config.ball_r * 2; //
                ball_rect.x=prev_px-game_config.ball_r; ball_rect.y=prev_py-game_config.ball_r;
                SDL_BlitSurface(render_pic, &ball_rect, screen, &ball_rect);

                UpdateBufAnimation(do_phys_step);

//------------------------------------------------------------------------------
                DrawBall(tk_px, tk_py, poss[2], R, ballColor);
//------------------------------------------------------------------------------

                //updating the screen
                if (!first_draw)
                {
                    int min_px, max_px;
                    int min_py, max_py;
                    if (prev_px <= tk_px)
                    {
                        min_px = prev_px;
                        max_px = tk_px;
                    }
                    else
                    {
                        min_px = tk_px;
                        max_px = prev_px;
                    }

                    if (prev_py <= tk_py)
                    {
                        min_py = prev_py;
                        max_py = tk_py;
                    }
                    else
                    {
                        min_py = tk_py;
                        max_py = prev_py;
                    }
                    min_px -= game_config.ball_r;
                    max_px += game_config.ball_r;
                    min_py -= game_config.ball_r;
                    max_py += game_config.ball_r;
                    if (min_px<0) min_px=0;
                    if (max_px>=game_config.wnd_w) max_px=game_config.wnd_w-1;
                    if (min_py<0) min_py=0;
                    if (max_py>=game_config.wnd_h) max_py=game_config.wnd_h-1;
                    SDL_UpdateRect(screen, min_px, min_py, max_px-min_px, max_py-min_py);

                    UpdateScreenAnimation();
                }

                prev_px=tk_px; prev_py=tk_py;

                //== GUI =======================================================
                if ((wasclick) && (!fullscreen))
                {
                    char txt[32];
                    sprintf(txt, "Level %d/%d", cur_level+1, game_levels_count);
                    SDL_FreeSurface(levelTextSurface);
                    levelTextSurface = TTF_RenderText_Blended(font, txt, fontColor);
                    levelTextLocation.x = (game_config.wnd_w - levelTextSurface->w) / 2;
                    SDL_BlitSurface(levelTextSurface, NULL, screen, &levelTextLocation);
                    SDL_BlitSurface(infoTextSurface, NULL, screen, &infoTextLocation);
                    
                    if (cur_level >0)
                        SDL_BlitSurface(back_pic, NULL, screen, &gui_rect_1);
                    else
                        SDL_BlitSurface(back_i_pic, NULL, screen, &gui_rect_1);

                    if (cur_level < game_levels_count-1)
                        SDL_BlitSurface(forward_pic, NULL, screen, &gui_rect_2);
                    else
                        SDL_BlitSurface(forward_i_pic, NULL, screen, &gui_rect_2);

                    if (cur_level > 0)
                        SDL_BlitSurface(reset_pic, NULL, screen, &gui_rect_3);
                    else
                        SDL_BlitSurface(reset_i_pic, NULL, screen, &gui_rect_3);

                    SDL_BlitSurface(exit_pic, NULL, screen, &gui_rect_4);
                    first_draw=1;
                }

                //update the whole screen if needed
                if (first_draw)
                {
                    SDL_Flip(screen);
                    first_draw=0;
                }

                if (game_state == GAME_STATE_FAILED)
                {
                    ZeroAnim();
                    InitState();
                    game_state = GAME_STATE_NORMAL;
                    first_draw=1;
                }

                if (game_state == GAME_STATE_SAVED)
                {
                    InitState();
                    game_state = GAME_STATE_NORMAL;
                    first_draw=1;
                }

                if (game_state == GAME_STATE_WIN)
                {
                    if (cur_level == game_levels_count-1)
                    {
                        cur_level=0;
                    }
                    else
                    {
                        cur_level++;
                    }
                    RenderLevel(); InitState();
                    game_state = GAME_STATE_NORMAL;
                    first_draw=1;
                }

                SDL_Delay(game_config.f_delay);
                
	}

        SaveLevel(cur_level+1);

        SDL_FreeSurface(infoTextSurface);
        SDL_FreeSurface(levelTextSurface);
        TTF_CloseFont(font);

        accelerometer_stop();
        
}
