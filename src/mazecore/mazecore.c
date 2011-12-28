/*  mazecore.c
 *
 *  Game logic routines.
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

#include <math.h>
#include <ode/ode.h>

#include "mazecore.h"
#include "mazehelpers.h"

#define GRAV_CONST 9.81*1.0
#define PHYS_SCALE (100.0*game_config.ball_r/23)

static MazeConfig game_config;
static Level *game_levels = NULL;
static int game_levels_count = 0;
static int cur_level = 0;

static GameState new_game_state;
static float acx = 0, acy = 0, acz = 0;
static int ball_pos_x = 0, ball_pos_y = 0, ball_pos_z = 0;
static const dReal *ball_rot = NULL;

static Animation final_anim;
static Animation *keys_anim = NULL;
static int keys_passed = 0;
static int save_key = -1;

static void (*vibro_callback)(float) = NULL;

//==============================================================================

// dynamics and collision objects
static dGeomID plane;
static dSpaceID space;
static dWorldID world;
static dBodyID body;
static dGeomID geom;
static dMass mass;
static dJointGroupID contactgroup;

static bool fall = false;
static bool fall_fixed = false;
static Point fall_hole;

#define BALL_R_PHYS game_config.ball_r/PHYS_SCALE
#define BALL_SHIFT  game_config.ball_r/100.0
#define WALL_H_PHYS ((game_config.ball_r+1)*(1+BALL_SHIFT)*2/PHYS_SCALE)
#define WALL_W_PHYS BALL_R_PHYS

#define WND_W_PHYS game_config.wnd_w/PHYS_SCALE
#define WND_H_PHYS game_config.wnd_h/PHYS_SCALE
#define HOLE_DEPTH_PHYS (game_config.ball_r*2/PHYS_SCALE)
#define BOX_SHIFT(x) (game_config.hole_r + x*game_config.ball_r)/PHYS_SCALE
#define BOX_SHIFT_CLOSE BOX_SHIFT(1.0/2.0)
#define BOX_SHIFT_FAR BOX_SHIFT(3.0/2.0)
#define HOLE_X_PHYS hole.x/PHYS_SCALE
#define HOLE_Y_PHYS hole.y/PHYS_SCALE

#define CYLINDER_SIDES 16
void CreateCylinder(Point hole, float r, float h, float z)
{
    for (int i=0; i<CYLINDER_SIDES; i++)
    {
        float a = 2*M_PI*i/CYLINDER_SIDES;
        float x = cos(a);
        float y = sin(a);
        dGeomID wall = dCreateBox(space, WALL_W_PHYS, WND_H_PHYS, h);
        dGeomSetPosition(wall, HOLE_X_PHYS + x*r, HOLE_Y_PHYS + y*r, z);
        dMatrix3 rot;
        dRFromAxisAndAngle(rot,0,0,1,a);
        dGeomSetRotation(wall,rot);
    }
}

void GoFall(Point hole)
{
    dWorldSetGravity(world,0,0, -(GRAV_CONST*0.5)*1.6);
    dGeomPlaneSetParams(plane, 0,0,1, -HOLE_DEPTH_PHYS);

    CreateCylinder(hole, BOX_SHIFT_CLOSE, HOLE_DEPTH_PHYS, -HOLE_DEPTH_PHYS/2.0);
    CreateCylinder(hole, BOX_SHIFT_FAR, WALL_H_PHYS, WALL_H_PHYS/2.0);

    fall = true;
    fall_hole = hole;
}

void GoFallFixed(Point hole)
{
    CreateCylinder(hole, BOX_SHIFT_CLOSE, WALL_H_PHYS, WALL_H_PHYS/2.0);
    fall_fixed = true;
}

bool testbump(float x, float y)
{
    if (fall)
    {
        if (!fall_fixed)
            if (inbox_r(x,y, fall_hole, game_config.hole_r-game_config.ball_r))
                GoFallFixed(fall_hole);
        return false;
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
        return true;
    }

    for (int i=0; i<game_levels[cur_level].holes_count; i++)
    {
        Point hole = game_levels[cur_level].holes[i];
        if (inbox_r(x,y, hole, game_config.hole_r+1))
        {
            float dist = calcdist(x,y, hole.x,hole.y);
            if (dist <= game_config.hole_r)
            {
                GoFall(hole);
                new_game_state = GAME_STATE_FAILED;
                return true;
            }
        }
    }

    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        if (keys_anim[i].stage == ANIMATION_NONE)
        {
            Point key = game_levels[cur_level].keys[i];
            if (inbox_r(x,y, key, game_config.key_r+1))
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
                    return false;
                }
            }
        }
    }

    return false;
}

// this is called by dSpaceCollide when two objects in space are
// potentially colliding.
#define MAX_CONTACTS 8
void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    if (b1 == b2)
        return;

    dContact contact[MAX_CONTACTS]; // up to MAX_CONTACTS contacts
    for (int i = 0; i < MAX_CONTACTS; i++)
    {
        contact[i].surface.mode = dContactApprox1 | dContactSoftCFM | dContactSoftERP;
        contact[i].surface.mu = 0.7f;
        contact[i].surface.soft_erp = 0.8f;
        contact[i].surface.soft_cfm = 0.00001f;
    }

    int numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact));
    if (numc)
    {
        for (int i = 0; i < numc; i++)
        {
            dJointID c = dJointCreateContact(world, contactgroup, contact + i);
            dJointAttach(c, b1, b2);
        }

        if ((o1!=plane) && (o2!=plane))
        {
            const dReal *Normal = contact[0].geom.normal;
            const dReal *LinearVel = dBodyGetLinearVel(body);

            float vlen = calclen(LinearVel[0], LinearVel[1], LinearVel[2]);
            float cosa = Normal[0]*LinearVel[0] +
                         Normal[1]*LinearVel[1] +
                         Normal[2]*LinearVel[2];

            float pvel = vlen*cosa;
            if (vibro_callback)
                vibro_callback(-pvel*70);
        }
    }
}

#define BOX_WND_HOR wall = dCreateBox(space, WND_W_PHYS, WALL_W_PHYS, WALL_H_PHYS);
#define BOX_WND_VER wall = dCreateBox(space, WALL_W_PHYS, WND_H_PHYS, WALL_H_PHYS);
#define BOX_WND_TOP wall = dCreateBox(space, WND_W_PHYS, WND_H_PHYS, WALL_W_PHYS);

#define BOX_POS_HOR(a,b) dGeomSetPosition(wall, WND_W_PHYS/2.0, a*WND_H_PHYS + b*WALL_W_PHYS/2.0, WALL_H_PHYS/2.0);
#define BOX_POS_VER(a,b) dGeomSetPosition(wall, a*WND_W_PHYS + b*WALL_W_PHYS/2.0, WND_H_PHYS/2.0, WALL_H_PHYS/2.0);
#define BOX_POS_TOP dGeomSetPosition(wall, WND_W_PHYS/2.0, WND_H_PHYS/2.0, WALL_H_PHYS+WALL_W_PHYS/2.0);

void InitState()
{
    dGeomID wall;
    
    float px, py;
    px = game_levels[cur_level].init.x;
    py = game_levels[cur_level].init.y;

    static bool first_init = true;
    if (!first_init)
        dWorldDestroy(world);
    first_init = false;

    fall = false;
    fall_fixed = false;

    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    dWorldSetGravity(world,0,0, -GRAV_CONST*0.5);
    dWorldSetCFM(world,1e-5);
    plane = dCreatePlane(space,0,0,1,0);

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
        wall = dCreateBox(space, boxr_w/PHYS_SCALE, boxr_h/PHYS_SCALE, WALL_H_PHYS);
        dGeomSetPosition(wall, boxr_x/PHYS_SCALE, boxr_y/PHYS_SCALE, WALL_H_PHYS/2.0);
    }

    BOX_WND_HOR; BOX_POS_HOR( 0,-1);
    BOX_WND_HOR; BOX_POS_HOR( 1, 1);
    BOX_WND_VER; BOX_POS_VER( 0,-1);
    BOX_WND_VER; BOX_POS_VER( 1, 1);
    //-- top -------------------------------------------------------------------
    BOX_WND_TOP; BOX_POS_TOP;
    //--------------------------------------------------------------------------

    contactgroup = dJointGroupCreate(0);
    // create object
    body = dBodyCreate(world);
    geom = dCreateSphere(space, BALL_R_PHYS);
    dMassSetSphere(&mass,1,BALL_R_PHYS);
    dBodySetMass(body,&mass);
    dGeomSetBody(geom,body);
    // set initial position
    int ix, iy;
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
    dBodySetPosition( body, ix/PHYS_SCALE, iy/PHYS_SCALE,
                      BALL_R_PHYS*(1+BALL_SHIFT) );
}

//------------------------------------------------------------------------------

void ZeroAnim(Animation *anim)
{
    anim->stage = ANIMATION_NONE;
    anim->time = 0;
    anim->progress = 0;
    anim->played = false;
}

void ZeroAnims()
{
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        ZeroAnim(&keys_anim[i]);
    }
    ZeroAnim(&final_anim);

    keys_passed = 0;
    save_key = -1;
}

void NewAnim()
{
    if (keys_anim) free(keys_anim);
    keys_anim = (Animation*)malloc(game_levels[cur_level].keys_count * sizeof(Animation));
    ZeroAnims();
}

#define MAX_ANIM_TIME 0.3
void UpdateAnim(Animation *anim, float do_phys_step)
{
    if (anim->stage == ANIMATION_PLAYING)
    {
        if (anim->played)
        {
            anim->stage = ANIMATION_FINISHED;
        }
        else
        {
            anim->time += do_phys_step;
            if (anim->time >= MAX_ANIM_TIME)
            {
                anim->time = MAX_ANIM_TIME;
                anim->played = true;
            }
            anim->progress = anim->time / MAX_ANIM_TIME;
        }
    }
}

void UpdateAnims(float do_phys_step)
{
    for (int i=0; i<game_levels[cur_level].keys_count; i++)
    {
        UpdateAnim(&keys_anim[i], do_phys_step);
    }

    if ( (game_levels[cur_level].keys_count > 0) &&
         (keys_passed == game_levels[cur_level].keys_count) )
    {
        UpdateAnim(&final_anim, do_phys_step);
    }
}

//------------------------------------------------------------------------------

float get_phys_step(int delta_ticks)
{
    #define STEP_QUANT 0.013/30.0
    float do_phys_step = STEP_QUANT * delta_ticks;
    clamp_max(do_phys_step, STEP_QUANT * 250);
    clamp_min(do_phys_step, STEP_QUANT);
    return do_phys_step;
}

#define PHYS_MIN_FALL_VEL 0.09
GameState maze_step(int delta_ticks)
{
    float do_phys_step = get_phys_step(delta_ticks);

    float forcex = acx*0.45;
    float forcey = acy*0.45;

    const dReal *poss = NULL;
    bool wnanc = false;
    for (int i=0; i<3; i++)
    {
        const dReal *Position = dBodyGetPosition(body);
        const dReal *Rotation = dBodyGetRotation(body);
        const dReal *Quaternion = dBodyGetQuaternion(body);
        const dReal *LinearVel = dBodyGetLinearVel(body);
        const dReal *AngularVel = dBodyGetAngularVel(body);

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
        {
            xRotation[j] = Rotation[j];
        }
        for (int j=0; j<4; j++)
        {
            xQuaternion[j] = Quaternion[j];
        }

        if (!wnanc)
        {
            if (!fall)
            {
                dBodyAddForce(body, forcex, 0, 0);
                dBodyAddForce(body, 0, forcey, 0);
            }
            else //fall
            {
                float cpx = Position[0]*PHYS_SCALE;
                float cpy = Position[1]*PHYS_SCALE;
                float tkdi = calcdist(fall_hole.x,fall_hole.y, cpx,cpy);
                float tkmin = game_config.hole_r - game_config.ball_r/2.0;
                if (tkdi > tkmin)
                {
                    float tfx, tfy;
                    float fo = 0.2 * (tkdi-tkmin)/(game_config.ball_r/2.0);
                    tfx = ((fall_hole.x - cpx) / tkdi) * fo;
                    tfy = ((fall_hole.y - cpy) / tkdi) * fo;
                    dBodyAddForce(body, tfx, tfy, 0);
                }
            }

            int qu;
            float qacx = (fall ? 0 : acx);
            float qacy = (fall ? 0 : acy);
            float qk = (fall ? 4.5 : 1);

            qu = -sign(LinearVel[0], 0.002);
            if (qu!=0) dBodyAddForce(body, qk*qu*0.0017*( 0.5*GRAV_CONST*cos(asin(qacx)) ), 0, 0);

            qu = -sign(LinearVel[1], 0.002);
            if (qu!=0) dBodyAddForce(body, 0, qk*qu*0.0017*( 0.5*GRAV_CONST*cos(asin(qacy)) ), 0);

            qu = -sign(AngularVel[2], 0.003);
            if (qu!=0) dBodyAddTorque(body, 0,0, qu*0.0005);
        }

        dSpaceCollide(space,0,&nearCallback);
        dWorldStep(world, do_phys_step);
        dJointGroupEmpty(contactgroup);

        poss = dGeomGetPosition(geom);

        bool nanc = false;
        for (int j=0; j<3; j++)
        {
            if (!(poss[j] <= 0.0) && !(poss[j] > 0.0))
            {
                //poss[j] is NaN
                nanc = true;
                break;
            }
        }

        if (nanc)
        {
            dBodySetPosition(body, xPosition[0],xPosition[1],xPosition[2]);
            dBodySetRotation(body, xRotation);
            dBodySetQuaternion(body, xQuaternion);
            dBodySetLinearVel(body, xLinearVel[0],xLinearVel[1],xLinearVel[2]);
            dBodySetAngularVel(body, xAngularVel[0],xAngularVel[1],xAngularVel[2]);
            wnanc = true;
        }
    } //for

    //determing new position of the ball
    poss = dGeomGetPosition(geom);
    ball_rot = dGeomGetRotation(geom);
    ball_pos_x = poss[0]*PHYS_SCALE;
    ball_pos_y = poss[1]*PHYS_SCALE;
    ball_pos_z = poss[2]*PHYS_SCALE;

    //test if it falls out
    testbump(ball_pos_x, ball_pos_y);

    GameState game_state = GAME_STATE_NORMAL;
    if (fall)
    {
        const dReal *lv = dBodyGetLinearVel(body);
        if ( calclen(lv[0],lv[1],lv[2]) < PHYS_MIN_FALL_VEL )
            if (ball_pos_z <= -game_config.ball_r*3.0/4.0)
                game_state = new_game_state;
    }

    UpdateAnims(do_phys_step);

    return game_state;
}

void maze_set_level(int n)
{
    cur_level = n;
    NewAnim();
    InitState();
}

void maze_restart_level()
{
    ZeroAnims();
    InitState();
}

void maze_reload_level()
{
    InitState();
}

void maze_set_config(MazeConfig cfg)
{
    game_config = cfg;
}

void maze_set_levels_data(Level *lvls, int levels_count)
{
    game_levels = lvls;
    game_levels_count = levels_count;
}

void maze_set_vibro_callback(void (*f)(float))
{
    vibro_callback = f;
}

void maze_set_forces(float x, float y, float z)
{
    acx = x;
    acy = y;
    acz = z;
}

void maze_get_ball(int *x, int *y, int *z, const dReal **rot)
{
    if (x) *x = ball_pos_x;
    if (y) *y = ball_pos_y;
    if (z) *z = ball_pos_z;
    if (rot) *rot = ball_rot;
}

void maze_get_animations(Animation **keys, Animation *final)
{
    *keys = keys_anim;
    *final = final_anim;
}

bool maze_is_keys_passed()
{
    return ( (game_levels[cur_level].keys_count > 0) &&
             (keys_passed == game_levels[cur_level].keys_count) );
}

void maze_init()
{
    dInitODE();
}

void maze_quit()
{
}
