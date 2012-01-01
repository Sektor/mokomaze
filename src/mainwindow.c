/*  mainwindow.c
 *
 *  Main loop.
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

#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include "render.h"
#include "mazecore/mazecore.h"
#include "mazecore/mazehelpers.h"
#include "paramsloader.h"
#include "input/input.h"
#include "vibro/vibro.h"
#include "gui/gui_settings.h"
#include "fonts.h"
#include "types.h"

#define LOG_MODULE "Main"
#include "logging.h"

static InputInterface input = {0};
static VibroInterface vibro = {0};
static bool input_cal_cycle = false;
static int disp_x = 0, disp_y = 0;

static int game_levels_count = 0;
Level *game_levels = NULL;
MazeConfig game_config = {0};
static Prompt arguments = {0};
static User *user_set = NULL;

int cur_level = 0;
int prev_px = 0, prev_py = 0;
int disp_bpp = 0;

Animation final_anim;
Animation *keys_anim = NULL;

SDL_Surface *screen = NULL;
SDL_Surface *fin_pic = NULL, *desk_pic = NULL, *wall_pic = NULL, *render_pic = NULL;
SDL_Rect desk_rect;

static int LoadImgErrors = 0;
static bool ingame = false;

//==============================================================================

#define vibro_force_k  1.0
#define MAX_BUMP_SPEED 240.0
#define MIN_BUMP_SPEED 70.0
void BumpVibrate(float speed)
{
    if (speed>0)
    {
        //log_debug("BumpVibrate(%f)",speed);
        if (speed>=MIN_BUMP_SPEED)
        {
            float k = (speed-MIN_BUMP_SPEED)/(MAX_BUMP_SPEED-MIN_BUMP_SPEED);
            clamp_max(k, 1);
            int lev = (0.27+0.73*k) * vibro_force_k * 255;
            clamp_max(lev, 255);
            uint8_t vlevel = (uint8_t)lev;
            vibro.bump(vlevel);
        }
    }
}

SDL_Surface *LoadImg(char *file)
{
    SDL_Surface *res = IMG_Load(file);
    if (res==NULL)
    {
        log_error("can't load image '%s'", file);
        LoadImgErrors++;
    }
    return res;
}

SDL_Surface *LoadSvg(char *fname, int hor_width, int hor_height, bool rot)
{
    rsvg_init();
    GError *error = NULL;
    RsvgHandle *rsvg_handle = rsvg_handle_new_from_file(fname, &error);
    if (!rsvg_handle)
    {
        log_error("can't load image '%s'", fname);
        LoadImgErrors++;
        return NULL;
    }
    RsvgDimensionData dimensions;
    rsvg_handle_get_dimensions(rsvg_handle, &dimensions);
    int svg_width = dimensions.width;
    int svg_height = dimensions.height;

    float svg_koef = (float)svg_width / svg_height;
    float hor_koef = (float)hor_width / hor_height;
    float scale = (svg_koef > hor_koef ? (float)hor_height / svg_height : (float)hor_width / svg_width);

    int scaled_width = (int)(svg_width * scale);
    int scaled_height = (int)(svg_height * scale);

    int res_width = ( rot ? hor_height : hor_width );
    int res_height = ( rot ? hor_width : hor_height );

    int stride = res_width * 4; /* 4 bytes/pixel (32bpp RGBA) */
    void *image = calloc(stride * res_height, 1);
    cairo_surface_t *cairo_surf = cairo_image_surface_create_for_data(
            image, CAIRO_FORMAT_ARGB32,
            res_width, res_height, stride);
    cairo_t *cr = cairo_create(cairo_surf);

    if (rot)
    {
        cairo_translate(cr, -(scaled_height-res_width)/2, res_height+(scaled_width-res_height)/2);
        cairo_scale(cr, scale, scale);
        cairo_rotate(cr, -M_PI/2);
    }
    else
    {
        cairo_translate(cr, -(scaled_width-res_width)/2, -(scaled_height-res_height)/2);
        cairo_scale(cr, scale, scale);
    }

    rsvg_handle_render_cairo(rsvg_handle, cr);

    cairo_surface_finish(cairo_surf);
    cairo_destroy(cr);
    rsvg_term();

    uint32_t rmask = 0x00ff0000;
    uint32_t gmask = 0x0000ff00;
    uint32_t bmask = 0x000000ff;
    uint32_t amask = 0xff000000;
    //Notice that it matches CAIRO_FORMAT_ARGB32
    SDL_Surface *res = SDL_CreateRGBSurfaceFrom(
            (void*) image,
            res_width, res_height,
            32, //4 bytes/pixel = 32bpp
            stride,
            rmask, gmask, bmask, amask);

    return res;
}

//------------------------------------------------------------------------------

static int fastchange_step = 0;
static SDL_TimerID fastchange_timer = 0;
static bool must_fastchange = false;
static int fastchange_dostep = 0;
#define FASTCHANGE_INTERVAL 1000

Uint32 fastchange_callback (Uint32 interval, void *param)
{
    fastchange_dostep = fastchange_step;
    must_fastchange = true;
    return interval;
}

void StopFastChange()
{
    if (fastchange_timer)
    {
        SDL_RemoveTimer(fastchange_timer);
        fastchange_timer = 0;
    }
    must_fastchange = false;
}

//------------------------------------------------------------------------------

void ResetPrevPos()
{
    maze_get_ball(&prev_px, &prev_py, NULL, NULL);
}

int get_start_level()
{
    int start_level = (arguments.level_set ? arguments.level-1 : user_set->level-1);
    clamp(start_level, 0, game_levels_count-1);
    return start_level;
}

void ApplyArguments()
{
    if (arguments.bpp_set)
    {
        user_set->bpp = arguments.bpp;
    }
    if (arguments.scrolling_set)
    {
        user_set->scrolling = arguments.scrolling;
    }
    if (arguments.geom_set)
    {
        user_set->geom_x = arguments.geom_x;
        user_set->geom_y = arguments.geom_y;
    }
    if (arguments.cal_reset)
    {
        user_set->input_calibration_data.cal_x = 0;
        user_set->input_calibration_data.cal_y = 0;
    }
    if (arguments.cal_auto)
    {
        input_calibration_reset();
        input_cal_cycle = true;
    }
    if (arguments.fullscreen_mode_set)
        user_set->fullscreen_mode = arguments.fullscreen_mode;
    if (arguments.input_set)
        user_set->input_type = arguments.input_type;
    if (arguments.vibro_set)
        user_set->vibro_type = arguments.vibro_type;
}

#define swap_t(t,x,y) \
{ \
    t _tmpx = x; \
    x = y; \
    y = _tmpx; \
}

#define swap(x,y) swap_t(int,x,y)

#define rotate(x,y,w) \
{ \
    int _tmpx = x; \
    x = (w-1)-y; \
    y = _tmpx; \
}

#define scale(x,y,s) \
{ \
    x *= s; y *= s; \
}

#define scale_rotate(x,y,s,r,w) \
{ \
    scale(x, y, s); \
    if (r) \
        rotate(x, y, w); \
}

bool TransformGeom()
{
    float disp_koef = (float)disp_x / disp_y;
    float pack_koef = (float)game_config.wnd_w / game_config.wnd_h;
    bool rot = ( sign(disp_koef-1,0) * sign(pack_koef-1,0) < 0 );

    if (rot)
        swap(game_config.wnd_w, game_config.wnd_h);

    pack_koef = (float)game_config.wnd_w / game_config.wnd_h;
    float k1 = (float)disp_x / game_config.wnd_w;
    float k2 = (float)disp_y / game_config.wnd_h;
    if (user_set->scrolling)
        swap_t(float, k1, k2);
    float scale = (pack_koef > disp_koef ? k1 : k2);
    int scaled_width = game_config.wnd_w * scale;
    //int scaled_height = game_config.wnd_h * scale;

    game_config.ball_r *= scale;
    game_config.hole_r *= scale;
    game_config.key_r *= scale;
    game_config.wnd_w *= scale;
    game_config.wnd_h *= scale;
    game_config.shadow *= scale;

    if (!user_set->scrolling)
    {
        disp_x = game_config.wnd_w;
        disp_y = game_config.wnd_h;
    }

    for (int i=0; i<game_levels_count; i++)
    {
        Level *lvl = &game_levels[i];
        for (int j=0; j<lvl->boxes_count; j++)
        {
            Box *box = &lvl->boxes[j];
            scale(box->x1, box->y1, scale);
            scale(box->x2, box->y2, scale);
            if (rot)
            {
                rotate(box->x1, box->y1, scaled_width);
                rotate(box->x2, box->y2, scaled_width);
                if (box->x1 > box->x2)
                    swap(box->x1, box->x2);
                if (box->y1 > box->y2)
                    swap(box->y1, box->y2);
            }
        }

        for (int j=0; j<lvl->fins_count; j++)
        {
            Point *p = &lvl->fins[j];
            scale_rotate(p->x, p->y, scale, rot, scaled_width);
        }

        for (int j=0; j<lvl->holes_count; j++)
        {
            Point *p = &lvl->holes[j];
            scale_rotate(p->x, p->y, scale, rot, scaled_width);
        }

        for (int j=0; j<lvl->keys_count; j++)
        {
            Point *p = &lvl->keys[j];
            scale_rotate(p->x, p->y, scale, rot, scaled_width);
        }

        Point *p = &lvl->init;
        scale_rotate(p->x, p->y, scale, rot, scaled_width);
    }

    return (pack_koef < 1);
}

void SetInput()
{
    input.shutdown();

    InputType itype = user_set->input_type;
    if (itype == INPUT_KEYBOARD)
        input_get_keyboard(&input, &user_set->input_keyboard_data);
    else if (itype == INPUT_JOYSTICK)
        input_get_joystick(&input, &user_set->input_joystick_data);
    else
        input_get_dummy(&input);

    input.init();
}

void SetVibro()
{
    vibro.shutdown();

    VibroType vtype = user_set->vibro_type;
    if (vtype == VIBRO_FREERUNNER)
        vibro_get_freerunner(&vibro, &user_set->vibro_freeerunner_data);
    else
        vibro_get_dummy(&vibro);

    vibro.init();
}

void ChangeLevel(int new_level, bool *redraw_all, bool *wasclick)
{
    RedrawDesk();
    cur_level = new_level;
    RenderLevel();
    RedrawDesk();
    maze_set_level(cur_level);
    ResetPrevPos();
    *redraw_all = true;
    *wasclick = true; //
}

#define MAX_CALIBRATION_SAMPLES 200
void render_window()
{
    game_config = GetGameConfig();
    game_levels = GetGameLevels();
    game_levels_count = GetGameLevelsCount();

    arguments = GetArguments();
    user_set = GetUserSettings();
    int start_level = get_start_level();

//------------------------------------------------------------------------------
    ApplyArguments();

    const SDL_VideoInfo *video_info = SDL_GetVideoInfo();
    if (user_set->geom_x > 0 && user_set->geom_y > 0)
    {
        disp_x = user_set->geom_x;
        disp_y = user_set->geom_y;
    }
    else
    {
        disp_x = video_info->current_w;
        disp_y = video_info->current_h;
    }

    int probe_bpp = (user_set->bpp == 0 ? video_info->vfmt->BitsPerPixel : user_set->bpp);
    disp_bpp = (probe_bpp == 32 ? probe_bpp : 16);

    bool rot = TransformGeom();

//------------------------------------------------------------------------------
    #define BTN_SPACE_KOEF 4.5
    int btn_side = disp_y / 7;
    int btn_space = btn_side / BTN_SPACE_KOEF;
    int btns_padded_width = btn_side * 4 + btn_space * 5;
    if (btns_padded_width > disp_x)
    {
        //btn_side = ( wnd_w - 5 * ( btn_side / BTN_SPACE_KOEF ) ) / 4
        btn_side = (int)(disp_x / (4 + 5 / BTN_SPACE_KOEF));
        btn_space = btn_side / BTN_SPACE_KOEF;
    }
    int btns_width = btn_side * 4 + btn_space * 3;
    int font_height = btn_side / 3;
    int font_padding = font_height / 2;

//-- font and labels -----------------------------------------------------------
    TTF_Font *font = TTF_OpenFont(DEFAULT_FONT_FILE, font_height);
    if (!font)
    {
        log_error("Can't load font '%s'. Exiting.", DEFAULT_FONT_FILE);
        return;
    }

    SDL_Surface *levelTextSurface = NULL;
    SDL_Rect levelTextLocation;
    levelTextLocation.y = font_padding;

    SDL_Color fontColor;
#ifndef RGBSWAP
    fontColor.r = 231;
    fontColor.g = 190;
    fontColor.b = 114;
#else
    //--enable-rgb-swap
    fontColor.r = 114;
    fontColor.g = 190;
    fontColor.b = 231;
#endif

//-- load pictures -------------------------------------------------------------
    SDL_Surface *back_pic     = LoadSvg(MDIR "prev-main.svg", btn_side, btn_side, false);
    SDL_Surface *forward_pic  = LoadSvg(MDIR "next-main.svg", btn_side, btn_side, false);
    SDL_Surface *settings_pic = LoadSvg(MDIR "settings-main.svg", btn_side, btn_side, false);
    SDL_Surface *exit_pic     = LoadSvg(MDIR "close-main.svg", btn_side, btn_side, false);

    SDL_Surface *back_i_pic    = LoadSvg(MDIR "prev-grey.svg", btn_side, btn_side, false);
    SDL_Surface *forward_i_pic = LoadSvg(MDIR "next-grey.svg", btn_side, btn_side, false);

    SDL_Surface *back_p_pic    = LoadSvg(MDIR "prev-light.svg", btn_side, btn_side, false);
    SDL_Surface *forward_p_pic = LoadSvg(MDIR "next-light.svg", btn_side, btn_side, false);

    int tmpx = (rot ? game_config.wnd_h : game_config.wnd_w);
    int tmpy = (rot ? game_config.wnd_w : game_config.wnd_h);
    desk_pic = LoadSvg(MDIR "desk.svg", tmpx, tmpy, rot);
    wall_pic = LoadSvg(MDIR "wall.svg", tmpx, tmpy, rot);
    int hole_d = game_config.hole_r * 2;
    fin_pic = LoadSvg(MDIR "openmoko.svg", hole_d, hole_d, rot);

    if (LoadImgErrors > 0)
    {
        log_error("Some images were not loaded. Exiting.");
        return;
    }

//-- positions of buttons ------------------------------------------------------
    SDL_Rect gui_rect_1, gui_rect_2, gui_rect_3, gui_rect_4;
    gui_rect_1.y = gui_rect_2.y = gui_rect_3.y = gui_rect_4.y = levelTextLocation.y + font_height + font_padding;
    gui_rect_1.x = (disp_x - btns_width) / 2;
    gui_rect_2.x = gui_rect_1.x + btn_side + btn_space;
    gui_rect_3.x = gui_rect_2.x + btn_side + btn_space;
    gui_rect_4.x = gui_rect_3.x + btn_side + btn_space;
//-- for click detection -------------------------------------------------------
    Box gui_box_1, gui_box_2, gui_box_3, gui_box_4;
    gui_box_1.x1 = gui_rect_1.x;
    gui_box_1.y1 = gui_rect_1.y;
    gui_box_1.x2 = gui_rect_1.x + btn_side;
    gui_box_1.y2 = gui_rect_1.y + btn_side;
    gui_box_2.x1 = gui_rect_2.x;
    gui_box_2.y1 = gui_rect_2.y;
    gui_box_2.x2 = gui_rect_2.x + btn_side;
    gui_box_2.y2 = gui_rect_2.y + btn_side;
    gui_box_3.x1 = gui_rect_3.x;
    gui_box_3.y1 = gui_rect_3.y;
    gui_box_3.x2 = gui_rect_3.x + btn_side;
    gui_box_3.y2 = gui_rect_3.y + btn_side;
    gui_box_4.x1 = gui_rect_4.x;
    gui_box_4.y1 = gui_rect_4.y;
    gui_box_4.x2 = gui_rect_4.x + btn_side;
    gui_box_4.y2 = gui_rect_4.y + btn_side;

    /* Window initialization */
    ingame = true;
    Uint32 sdl_flags = SDL_SWSURFACE;
    if (user_set->fullscreen_mode != FULLSCREEN_NONE)
        sdl_flags |= SDL_FULLSCREEN;
    SDL_Surface *disp = SDL_SetVideoMode(disp_x, disp_y, disp_bpp, sdl_flags);
    if (disp == NULL)
    {
        log_error("Can't set video mode %dx%dx%dbpp. Exiting.", disp_x, disp_y, disp_bpp);
        return;
    }
    screen = disp;
    SDL_Surface *gui_surface = disp;
    SDL_WM_SetCaption("Mokomaze", "Mokomaze");

    //create surface for rendering the level
    render_pic = CreateSurface(SDL_SWSURFACE, game_config.wnd_w, game_config.wnd_h, disp);

    if (user_set->scrolling)
        screen = CreateSurface(SDL_SWSURFACE, game_config.wnd_w, game_config.wnd_h, disp);

    if (user_set->fullscreen_mode != FULLSCREEN_NONE)
        SDL_ShowCursor(!ingame);

    desk_rect.x = 0;
    desk_rect.y = 0;
    desk_rect.w = game_config.wnd_w;
    desk_rect.h = game_config.wnd_h;

    SDL_Rect ball_rect;

    int disp_scroll_border = min(disp_x, disp_y) * 0.27;
    SDL_Rect screen_rect;
    screen_rect.x = 0;
    screen_rect.y = 0;
    screen_rect.w = disp_x;
    screen_rect.h = disp_y;
    SDL_Rect disp_rect;
    disp_rect = screen_rect;
    
    SDL_Color ballColor;
    ballColor.r = 255;
    ballColor.g = 127;
    ballColor.b = 0;

    User user_set_new = *user_set;
    bool video_set_modified = false;

    /* Settings window initialization */
    settings_init(disp, font_height, user_set, &user_set_new);

    /* Render initialization */
    InitRender();

    /* Input system initialization */
    input_get_dummy(&input);
    SetInput();

    /* Vibro system initialization */
    vibro_get_dummy(&vibro);
    SetVibro();
    
    /* MazeCore initialization */
    maze_init();
    maze_set_config(game_config);
    maze_set_vibro_callback(BumpVibrate);
    maze_set_levels_data(game_levels, game_levels_count);

    cur_level = start_level;
    RenderLevel();
    RedrawDesk();
    maze_set_level(cur_level);
    ResetPrevPos();

    SDL_Event event;
    bool done = false;
    bool redraw_all = true;
    bool ingame_changed = false;
    int prev_ticks = SDL_GetTicks();
    Point mouse = {0};
    
//== Game Loop =================================================================
    while (!done)
    {
        bool wasclick = false;
        bool show_settings = false;
        while (SDL_PollEvent(&event))
        {
            bool btndown = false;
            bool btnesc = false;
            if (event.type == SDL_QUIT)
            {
                done = true;
            }
            else if (event.type == SDL_ACTIVEEVENT)
            {
                int g = event.active.gain;
                int s = event.active.state;
                if (ingame && !g && ((s & SDL_APPINPUTFOCUS) || (s & SDL_APPACTIVE)))
                {
                    btndown = true;
                }
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                mouse.x = event.motion.x;
                mouse.y = event.motion.y;
            }
            else if (event.type == SDL_MOUSEBUTTONUP)
            {
                StopFastChange();
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                btndown = true;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    btndown = true;
                    btnesc = true;
                }
            }

            if (btndown)
            {
                if (!ingame)
                {
                    if (inbox(mouse.x, mouse.y, gui_box_1) && !btnesc)
                    {
                        if (cur_level > 0)
                        {
                            SDL_BlitSurface(back_p_pic, NULL, gui_surface, &gui_rect_1);
                            SDL_UpdateRect(gui_surface, gui_rect_1.x, gui_rect_1.y, gui_rect_1.w, gui_rect_1.h);

                            ChangeLevel(cur_level-1, &redraw_all, &wasclick);

                            fastchange_step = -10;
                            StopFastChange();
                            fastchange_timer = SDL_AddTimer(FASTCHANGE_INTERVAL, fastchange_callback, NULL);
                        }
                        continue;
                    }
                    else if (inbox(mouse.x, mouse.y, gui_box_2) && !btnesc)
                    {
                        if (cur_level < game_levels_count - 1)
                        {
                            SDL_BlitSurface(forward_p_pic, NULL, gui_surface, &gui_rect_2);
                            SDL_UpdateRect(gui_surface, gui_rect_2.x, gui_rect_2.y, gui_rect_2.w, gui_rect_2.h);

                            ChangeLevel(cur_level+1, &redraw_all, &wasclick);

                            fastchange_step = +10;
                            StopFastChange();
                            fastchange_timer = SDL_AddTimer(FASTCHANGE_INTERVAL, fastchange_callback, NULL);
                        }
                        continue;
                    }
                    else if (inbox(mouse.x, mouse.y, gui_box_3) && !btnesc)
                    {
                        show_settings = true;
                        RedrawDesk();
                        redraw_all = true;
                        wasclick = true;
                        continue;
                    }
                    else if (inbox(mouse.x, mouse.y, gui_box_4) || btnesc)
                    {
                        done = true;
                        continue;
                    }
                }
                ingame_changed = true;
            } //if (btndown)
        }

        if (ingame_changed)
        {
            ingame = !ingame;
            if (!ingame)
            {
                wasclick = true;
            }
            else
            {
                RedrawDesk();
                redraw_all = true;
            }

            if (user_set->fullscreen_mode == FULLSCREEN_INGAME)
                SDL_WM_ToggleFullScreen(disp);
            if (user_set->fullscreen_mode != FULLSCREEN_NONE)
                SDL_ShowCursor(!ingame);

            prev_ticks = SDL_GetTicks();
            ingame_changed = false;
        }

        if ((!ingame) && (!wasclick) && (must_fastchange))
        {
            int new_cur_level = cur_level + fastchange_dostep;
            clamp_max(new_cur_level, game_levels_count - 1);
            clamp_min(new_cur_level, 0);

            if (new_cur_level != cur_level)
            {
                if (fastchange_dostep < 0)
                {
                    SDL_BlitSurface(back_p_pic, NULL, gui_surface, &gui_rect_1);
                    SDL_UpdateRect(gui_surface, gui_rect_1.x, gui_rect_1.y, gui_rect_1.w, gui_rect_1.h);
                }
                else
                {
                    SDL_BlitSurface(forward_p_pic, NULL, gui_surface, &gui_rect_2);
                    SDL_UpdateRect(gui_surface, gui_rect_2.x, gui_rect_2.y, gui_rect_2.w, gui_rect_2.h);
                }

                ChangeLevel(new_cur_level, &redraw_all, &wasclick);
            }
            must_fastchange = false;
        }

        if (!ingame && !wasclick)
        {
            SDL_Delay(user_set->frame_delay);
            continue;
        }

//-- physics step --------------------------------------------------------------
        int ticks = SDL_GetTicks();
        int delta_ticks = ticks - prev_ticks;
        prev_ticks = ticks;
        clamp_min(delta_ticks, 1);
        clamp_max(delta_ticks, 1000 / 15);

        float acx = 0, acy = 0;
        input.read(&acx, &acy, NULL);
        if (input_cal_cycle)
            input_cal_cycle = (input_calibration_sample(&user_set->input_calibration_data, &acx, &acy, NULL) < MAX_CALIBRATION_SAMPLES);
        input_calibration_adjust(&user_set->input_calibration_data, &acx, &acy, NULL);
        maze_set_forces(acx, acy, 0);
        GameState game_state = maze_step(delta_ticks);

        const dReal *R;
        int tk_px, tk_py, tk_pz;
        maze_get_ball(&tk_px, &tk_py, &tk_pz, &R);
        maze_get_animations(&keys_anim, &final_anim);
//------------------------------------------------------------------------------

        //restore the background
        ball_rect.w = game_config.ball_r * 2;
        ball_rect.h = game_config.ball_r * 2; //
        ball_rect.x = prev_px - game_config.ball_r;
        ball_rect.y = prev_py - game_config.ball_r;
        SDL_BlitSurface(render_pic, &ball_rect, screen, &ball_rect);

        UpdateBufAnimation();
        DrawBall(tk_px, tk_py, tk_pz, R, ballColor);

        //update the screen
        if (!redraw_all && !user_set->scrolling)
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
            clamp_min(min_px, 0);
            clamp_max(max_px, game_config.wnd_w - 1);
            clamp_min(min_py, 0);
            clamp_max(max_py, game_config.wnd_h - 1);
            SDL_UpdateRect(screen, min_px, min_py, max_px - min_px, max_py - min_py);
            UpdateScreenAnimation();
        }

        if (user_set->scrolling)
        {
            clamp_min(screen_rect.x, tk_px - disp_x + disp_scroll_border);
            clamp_max(screen_rect.x, tk_px - disp_scroll_border);
            clamp_min(screen_rect.y, tk_py - disp_y + disp_scroll_border);
            clamp_max(screen_rect.y, tk_py - disp_scroll_border);
            clamp(screen_rect.x, 0, game_config.wnd_w - disp_x);
            clamp(screen_rect.y, 0, game_config.wnd_h - disp_y);
            SDL_BlitSurface(screen, &screen_rect, disp, &disp_rect);
        }

        prev_px = tk_px;
        prev_py = tk_py;

//-- GUI -----------------------------------------------------------------------
        if (wasclick && !ingame && !show_settings)
        {
            char txt[32];
            sprintf(txt, "Level %d/%d", cur_level + 1, game_levels_count);
            SDL_FreeSurface(levelTextSurface);
            levelTextSurface = TTF_RenderText_Blended(font, txt, fontColor);
            levelTextLocation.x = (disp_x - levelTextSurface->w) / 2;
            SDL_BlitSurface(levelTextSurface, NULL, gui_surface, &levelTextLocation);

            if (cur_level > 0)
                SDL_BlitSurface(back_pic, NULL, gui_surface, &gui_rect_1);
            else
                SDL_BlitSurface(back_i_pic, NULL, gui_surface, &gui_rect_1);

            if (cur_level < game_levels_count - 1)
                SDL_BlitSurface(forward_pic, NULL, gui_surface, &gui_rect_2);
            else
                SDL_BlitSurface(forward_i_pic, NULL, gui_surface, &gui_rect_2);

            SDL_BlitSurface(settings_pic, NULL, gui_surface, &gui_rect_3);
            SDL_BlitSurface(exit_pic, NULL, gui_surface, &gui_rect_4);
            redraw_all = true;
        }
//------------------------------------------------------------------------------

        //update the whole screen if needed
        if (user_set->scrolling)
        {
            SDL_Flip(disp);
        }
        else if (redraw_all)
        {
            SDL_Flip(screen);
        }
        redraw_all = false;

        if (show_settings)
        {
            bool _video_set_modified = false;
            bool _input_set_modified = false;
            bool _vibro_set_modified = false;
            settings_show(&input_cal_cycle, &_video_set_modified, &_input_set_modified, &_vibro_set_modified);
            if (input_cal_cycle)
                input_calibration_reset();
            if (_video_set_modified)
                video_set_modified = true;
            if (_input_set_modified)
                SetInput();
            if (_vibro_set_modified)
                SetVibro();
            SDL_GetMouseState(&mouse.x, &mouse.y);
            ingame_changed = true;
        }

        switch (game_state)
        {
        case GAME_STATE_FAILED:
            RedrawDesk();
            maze_restart_level();
            ResetPrevPos();
            redraw_all = true;
            break;
        case GAME_STATE_SAVED:
            RedrawDesk();
            maze_reload_level();
            ResetPrevPos();
            redraw_all = true;
            break;
        case GAME_STATE_WIN:
            if (++cur_level >= game_levels_count) cur_level=0;
            RenderLevel();
            RedrawDesk();
            maze_set_level(cur_level);
            ResetPrevPos();
            redraw_all = true;
            break;
        default:
            break;
        }

        SDL_Delay(user_set->frame_delay);
    }
//==============================================================================

    if (video_set_modified)
    {
        user_set->scrolling = user_set_new.scrolling;
        user_set->geom_x = user_set_new.geom_x;
        user_set->geom_y = user_set_new.geom_y;
        user_set->bpp = user_set_new.bpp;
        user_set->fullscreen_mode = user_set_new.fullscreen_mode;
        user_set->frame_delay = user_set_new.frame_delay;
    }
    
    user_set->level = cur_level + 1;
    SaveUserSettings();

    settings_shutdown();

    SDL_FreeSurface(levelTextSurface);
    TTF_CloseFont(font);
    vibro.shutdown();
    input.shutdown();
}
