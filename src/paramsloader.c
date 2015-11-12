/*  paramsloader.c
 *
 *  Config and level pack loader.
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <argtable2.h>
#include "types.h"
#include "paramsloader.h"

#define LOG_MODULE "Loader"
#include "logging.h"

static MazeConfig game_config = {0};
static Level *game_levels = NULL;
static int game_levels_count = 0;
static User user_set = {0};
static Prompt arguments = {0};

#define LEVELPACK_DEFAULT "main"
#define SAVE_DIR ".mokomaze"
#define CACHE_DIR "gfx_cache"
#define SAVE_FILE "user.json"
#define CONFIG_FILE MDIR "config.json"
static char *save_dir_full = NULL;
static char *cache_dir_full = NULL;
static char *save_file_full = NULL;
static bool can_save = false;

static JsonParser *parser = NULL;
static JsonNode *root_node = NULL;
static JsonObject *root_object = NULL;

//------------------------------------------------------------------------------

JsonNode* _json_object_get_member(JsonObject *obj, const char *member_name)
{
    return (obj ? json_object_get_member(obj, member_name) : NULL);
}
JsonArray* _json_node_get_array(JsonNode *node)
{
    if ((!node) || (JSON_NODE_TYPE(node) != JSON_NODE_ARRAY))
        return NULL;
    return json_node_get_array(node);
}
int _json_array_get_length(JsonArray *array)
{
    return (array ? json_array_get_length(array) : 0);
}
JsonNode* _json_array_get_element(JsonArray *array, int i)
{
    return (array ? json_array_get_element(array, i) : NULL);
}
JsonObject* _json_node_get_object(JsonNode *node)
{
    return (node ? json_node_get_object(node) : NULL);
}

#define JSON_GET_MEMBER_NODE JsonNode *member_node = _json_object_get_member(obj, member_name);
JsonObject* _json_object_get_member_object(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    return _json_node_get_object(member_node);
}
bool _json_object_get_member_boolean(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    if ((!member_node) || (json_node_get_value_type(member_node) != G_TYPE_BOOLEAN))
        return false;
    return json_node_get_boolean(member_node);
}
int _json_object_get_member_int(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    if (member_node)
    {
        GType vtype = json_node_get_value_type(member_node);
        if (vtype == G_TYPE_INT || vtype == G_TYPE_INT64)
            return json_node_get_int(member_node);
    }
    return 0;
}
double _json_object_get_member_double(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    if (member_node)
    {
        GType vtype = json_node_get_value_type(member_node);
        if (vtype == G_TYPE_DOUBLE)
            return json_node_get_double(member_node);
        else if (vtype == G_TYPE_INT64)
            return json_node_get_int(member_node);
    }
    return 0.0;
}
char* _json_object_dup_member_string(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    if ((!member_node) || (json_node_get_value_type(member_node) != G_TYPE_STRING))
        return NULL;
    return json_node_dup_string(member_node);
}

//------------------------------------------------------------------------------

void _json_object_set_member_boolean(JsonObject *obj, const char *member_name, bool val)
{
    JSON_GET_MEMBER_NODE
    if ((!member_node) || (json_node_get_value_type(member_node) != G_TYPE_BOOLEAN))
        return;
    json_node_set_boolean(member_node, val);
}
void _json_object_set_member_int(JsonObject *obj, const char *member_name, int val)
{
    JSON_GET_MEMBER_NODE
    if (member_node)
    {
        GType vtype = json_node_get_value_type(member_node);
        if (vtype == G_TYPE_INT || vtype == G_TYPE_INT64)
            json_node_set_int(member_node, val);
    }
}
void _json_object_set_member_double(JsonObject *obj, const char *member_name, double val)
{
    JSON_GET_MEMBER_NODE
    if ((!member_node) || (json_node_get_value_type(member_node) != G_TYPE_DOUBLE))
        return;
    json_node_set_double(member_node, val);
}
void _json_object_set_member_string(JsonObject *obj, const char *member_name, const char *val)
{
    JSON_GET_MEMBER_NODE
    if ((!member_node) || (json_node_get_value_type(member_node) != G_TYPE_STRING))
        return;
    json_node_set_string(member_node, val);
}

//------------------------------------------------------------------------------

bool StrToBool(char *str, bool free_str)
{
    bool res = false;
    if (str)
    {
        if (!strcmp(str, "true"))
            res = true;
        if (free_str)
            free(str);
    }
    return res;
}

FullscreenMode StrToFullscreenMode(char *str, bool free_str)
{
    FullscreenMode res = FULLSCREEN_NONE;
    if (str)
    {
        if (!strcmp(str, FULLSCREEN_INGAME_STR))
            res = FULLSCREEN_INGAME;
        else if (!strcmp(str, FULLSCREEN_ALWAYS_STR))
            res = FULLSCREEN_ALWAYS;
        if (free_str)
            free(str);
    }
    return res;
}

InputType StrToInputType(char *str, bool free_str)
{
    InputType res = INPUT_DUMMY;
    if (str)
    {
        if (!strcmp(str, INPUT_KEYBOARD_STR))
            res = INPUT_KEYBOARD;
        else if (!strcmp(str, INPUT_JOYSTICK_STR))
            res = INPUT_JOYSTICK;
        else if (!strcmp(str, INPUT_JOYSTICK_SDL_STR))
            res = INPUT_JOYSTICK_SDL;
        else if (!strcmp(str, INPUT_ACCEL_STR))
            res = INPUT_ACCEL;
        if (free_str)
            free(str);
    }
    return res;
}

VibroType StrToVibroType(char *str, bool free_str)
{
    VibroType res = VIBRO_DUMMY;
    if (str)
    {
        if (!strcmp(str, VIBRO_FREERUNNER_STR))
            res = VIBRO_FREERUNNER;
        if (free_str)
            free(str);
    }
    return res;
}

//------------------------------------------------------------------------------

bool load_json(const char *fname)
{
    GError *error = NULL;
    json_parser_load_from_file(parser, fname, &error);
    if (error)
    {
        log_error("Unable to parse: %s", error->message);
        g_error_free(error);
        return false;
    }
    root_node = json_parser_get_root(parser);
    root_object = _json_node_get_object(root_node);
    if (!root_object)
        log_error("Unable to get root element");
    return root_object;
}

bool load_levelpack()
{
    const char *fname = MDIR LEVELPACK_DEFAULT ".levelpack.json";
    log_info("Loading levelpack file `%s'", fname);
    if (!load_json(fname))
        return false;

    JsonObject *requirements_object = _json_object_get_member_object(root_object, "requirements");

    JsonObject *pack_window_object = _json_object_get_member_object(requirements_object, "window");
    game_config.wnd_w = _json_object_get_member_int(pack_window_object, "width");
    game_config.wnd_h = _json_object_get_member_int(pack_window_object, "height");

    JsonObject *ball_object = _json_object_get_member_object(requirements_object, "ball");
    game_config.ball_r = _json_object_get_member_int(ball_object, "radius");

    JsonObject *hole_object = _json_object_get_member_object(requirements_object, "hole");
    game_config.hole_r = _json_object_get_member_int(hole_object, "radius");

    JsonObject *key_object = _json_object_get_member_object(requirements_object, "key");
    game_config.key_r = _json_object_get_member_int(key_object, "radius");

    JsonObject *box_object = _json_object_get_member_object(requirements_object, "box");
    game_config.shadow = _json_object_get_member_int(box_object, "shadow");

    JsonNode *levels_node = _json_object_get_member(root_object, "levels");
    JsonArray *levels_array = _json_node_get_array(levels_node);
    int levels_count = _json_array_get_length(levels_array);
    game_levels_count = levels_count;
    game_levels = (Level*)calloc(levels_count, sizeof(Level));
    for (int i=0; i<levels_count; i++)
    {
        Level *level = &game_levels[i];

        JsonNode *level_node = _json_array_get_element(levels_array, i);
        JsonObject *level_object = _json_node_get_object(level_node);

        JsonNode *boxes_node = _json_object_get_member(level_object, "boxes");
        if (boxes_node)
        {
            JsonArray *boxes_array = _json_node_get_array(boxes_node);
            int boxes_count = _json_array_get_length(boxes_array);
            level->boxes_count = boxes_count;
            level->boxes = (Box*)malloc(sizeof(Box) * boxes_count);
            for (int j=0; j<boxes_count; j++)
            {
                JsonNode *box_node = _json_array_get_element(boxes_array, j);
                JsonObject *box_object = _json_node_get_object(box_node);
                Box *box = &level->boxes[j];
                box->x1 = _json_object_get_member_int(box_object, "x1");
                box->y1 = _json_object_get_member_int(box_object, "y1");
                box->x2 = _json_object_get_member_int(box_object, "x2");
                box->y2 = _json_object_get_member_int(box_object, "y2");
            }
        }

        JsonNode *holes_node = _json_object_get_member(level_object, "holes");
        if (holes_node)
        {
            JsonArray *holes_array = _json_node_get_array(holes_node);
            int holes_count = _json_array_get_length(holes_array);
            level->holes_count = holes_count;
            level->holes = (Point*)malloc(sizeof(Point) * holes_count);
            for (int j=0; j<holes_count; j++)
            {
                JsonNode *hole_node = _json_array_get_element(holes_array, j);
                JsonObject *hole_object = _json_node_get_object(hole_node);
                Point *hole = &level->holes[j];
                hole->x = _json_object_get_member_int(hole_object, "x");
                hole->y = _json_object_get_member_int(hole_object, "y");
            }
        }

        JsonNode *keys_node = _json_object_get_member(level_object, "keys");
        if (keys_node)
        {
            JsonArray *keys_array = _json_node_get_array(keys_node);
            int keys_count = _json_array_get_length(keys_array);
            level->keys_count = keys_count;
            level->keys = (Point*)malloc(sizeof(Point) * keys_count);
            for (int j=0; j<keys_count; j++)
            {
                JsonNode *key_node = _json_array_get_element(keys_array, j);
                JsonObject *key_object = _json_node_get_object(key_node);
                Point *key = &level->keys[j];
                key->x = _json_object_get_member_int(key_object, "x");
                key->y = _json_object_get_member_int(key_object, "y");
            }
        }

        JsonNode *fins_node = _json_object_get_member(level_object, "checkpoints");
        JsonArray *fins_array = _json_node_get_array(fins_node);
        int fins_count = _json_array_get_length(fins_array);
        level->fins_count = fins_count;
        level->fins = (Point*)malloc(sizeof(Point) * fins_count);
        for (int j=0; j<fins_count; j++)
        {
            JsonNode *fin_node = _json_array_get_element(fins_array, j);
            JsonObject *fin_object = _json_node_get_object(fin_node);
            Point *fin = &level->fins[j];
            fin->x = _json_object_get_member_int(fin_object, "x");
            fin->y = _json_object_get_member_int(fin_object, "y");
        }

        JsonObject *init_object = _json_object_get_member_object(level_object, "init");
        level->init.x = _json_object_get_member_int(init_object, "x");
        level->init.y = _json_object_get_member_int(init_object, "y");
    }

    log_info("%d game levels parsed", levels_count);
    return true;
}

#define CONFIG_FORMAT 1
bool load_config(const char *fname)
{
    log_info("Loading settings file `%s'", fname);
    if (!load_json(fname))
        return false;

    int config_format = _json_object_get_member_int(root_object, "config_format");
    if (config_format != CONFIG_FORMAT)
    {
        log_info("Mismatched config_format", fname);
        return false;
    }

    user_set.levelpack = _json_object_dup_member_string(root_object, "levelpack");
    if (!user_set.levelpack)
        user_set.levelpack = strdup(LEVELPACK_DEFAULT);

    user_set.level = _json_object_get_member_int(root_object, "level");
    user_set.geom_x = _json_object_get_member_int(root_object, "geom_x");
    user_set.geom_y = _json_object_get_member_int(root_object, "geom_y");
    user_set.bpp = _json_object_get_member_int(root_object, "bpp");
    user_set.scrolling = _json_object_get_member_boolean(root_object, "scrolling");
    user_set.frame_delay = _json_object_get_member_int(root_object, "frame_delay");
    user_set.ball_speed = (float)_json_object_get_member_double(root_object, "ball_speed");
    user_set.bump_min_speed = (float)_json_object_get_member_double(root_object, "bump_min_speed");
    user_set.bump_max_speed = (float)_json_object_get_member_double(root_object, "bump_max_speed");

    char *fullscreen_str = _json_object_dup_member_string(root_object, "fullscreen_mode");
    user_set.fullscreen_mode = StrToFullscreenMode(fullscreen_str, true);

    char *input_str = _json_object_dup_member_string(root_object, "input_type");
    user_set.input_type = StrToInputType(input_str, true);

    char *vibro_str = _json_object_dup_member_string(root_object, "vibro_type");
    user_set.vibro_type = StrToVibroType(vibro_str, true);

    JsonObject *input_calibration_data_object = _json_object_get_member_object(root_object, "input_calibration_data");
    user_set.input_calibration_data.swap_xy = _json_object_get_member_boolean(input_calibration_data_object, "swap_xy");
    user_set.input_calibration_data.invert_x = _json_object_get_member_boolean(input_calibration_data_object, "invert_x");
    user_set.input_calibration_data.invert_y = _json_object_get_member_boolean(input_calibration_data_object, "invert_y");
    user_set.input_calibration_data.cal_x = (float)_json_object_get_member_double(input_calibration_data_object, "cal_x");
    user_set.input_calibration_data.cal_y = (float)_json_object_get_member_double(input_calibration_data_object, "cal_y");
    user_set.input_calibration_data.sens = (float)_json_object_get_member_double(input_calibration_data_object, "sensitivity");

    JsonObject *vibro_freeerunner_data_object = _json_object_get_member_object(root_object, "vibro_freeerunner_data");
    user_set.vibro_freeerunner_data.duration = _json_object_get_member_int(vibro_freeerunner_data_object, "duration");

    JsonObject *input_joystick_data_object = _json_object_get_member_object(root_object, "input_joystick_data");
    user_set.input_joystick_data.fname = _json_object_dup_member_string(input_joystick_data_object, "fname");
    if (!user_set.input_joystick_data.fname)
        user_set.input_joystick_data.fname = strdup("");
    user_set.input_joystick_data.max_axis = (float)_json_object_get_member_double(input_joystick_data_object, "max_axis");
    user_set.input_joystick_data.interval = _json_object_get_member_int(input_joystick_data_object, "interval");

    JsonObject *input_joystick_sdl_data_object = _json_object_get_member_object(root_object, "input_joystick_sdl_data");
    user_set.input_joystick_sdl_data.number = _json_object_get_member_int(input_joystick_sdl_data_object, "number");
    user_set.input_joystick_sdl_data.max_axis = (float)_json_object_get_member_double(input_joystick_sdl_data_object, "max_axis");

    JsonObject *input_accel_data_object = _json_object_get_member_object(root_object, "input_accelerometer_data");
    user_set.input_accel_data.fname = _json_object_dup_member_string(input_accel_data_object, "fname");
    if (!user_set.input_accel_data.fname)
        user_set.input_accel_data.fname = strdup("");
    user_set.input_accel_data.max_axis = (float)_json_object_get_member_double(input_accel_data_object, "max_axis");
    user_set.input_accel_data.interval = _json_object_get_member_int(input_accel_data_object, "interval");

    return true;
}

bool load_params()
{
    char *pHome = getenv("HOME");
    if (!pHome)
    {
        can_save = false;
        log_warning("can't retrieve environment variable HOME");
        log_warning("user settings are not available");
    }
    else
    {
        can_save = true;
        save_dir_full = (char*)malloc(strlen(pHome) + strlen("/") + strlen(SAVE_DIR) + 1);
        sprintf(save_dir_full, "%s/%s", pHome, SAVE_DIR);
        cache_dir_full = (char*)malloc(strlen(save_dir_full) + strlen("/") + strlen(CACHE_DIR) + 1);
        sprintf(cache_dir_full, "%s/%s", save_dir_full, CACHE_DIR);
        save_file_full = (char*)malloc(strlen(save_dir_full) + strlen("/") + strlen(SAVE_FILE) + 1);
        sprintf(save_file_full, "%s/%s", save_dir_full, SAVE_FILE);
    }

    parser = json_parser_new();
    bool loaded = false;
    if (can_save)
        loaded = load_config(save_file_full);
    if (!loaded)
        loaded = load_config(CONFIG_FILE);
    if (loaded)
        loaded = load_levelpack();
    g_object_unref(parser);

    return loaded;
}

void SetJsonValues()
{
    _json_object_set_member_string(root_object, "levelpack", user_set.levelpack);
    _json_object_set_member_int(root_object, "level", user_set.level);
    _json_object_set_member_int(root_object, "geom_x", user_set.geom_x);
    _json_object_set_member_int(root_object, "geom_y", user_set.geom_y);
    _json_object_set_member_int(root_object, "bpp", user_set.bpp);
    _json_object_set_member_boolean(root_object, "scrolling", user_set.scrolling);
    _json_object_set_member_int(root_object, "frame_delay", user_set.frame_delay);
    _json_object_set_member_double(root_object, "ball_speed", user_set.ball_speed);
    _json_object_set_member_double(root_object, "bump_min_speed", user_set.bump_min_speed);
    _json_object_set_member_double(root_object, "bump_max_speed", user_set.bump_max_speed);

    char *fullscreen_mode_str = NULL;
    switch (user_set.fullscreen_mode)
    {
    case FULLSCREEN_INGAME:
        fullscreen_mode_str = FULLSCREEN_INGAME_STR;
        break;
    case FULLSCREEN_ALWAYS:
        fullscreen_mode_str = FULLSCREEN_ALWAYS_STR;
        break;
    default:
        fullscreen_mode_str = FULLSCREEN_NONE_STR;
        break;
    }
    _json_object_set_member_string(root_object, "fullscreen_mode", fullscreen_mode_str);

    char *input_type_str = NULL;
    switch (user_set.input_type)
    {
    case INPUT_KEYBOARD:
        input_type_str = INPUT_KEYBOARD_STR;
        break;
    case INPUT_JOYSTICK:
        input_type_str = INPUT_JOYSTICK_STR;
        break;
    case INPUT_JOYSTICK_SDL:
        input_type_str = INPUT_JOYSTICK_SDL_STR;
        break;
    case INPUT_ACCEL:
        input_type_str = INPUT_ACCEL_STR;
        break;
    default:
        input_type_str = INPUT_DUMMY_STR;
        break;
    }
    _json_object_set_member_string(root_object, "input_type", input_type_str);

    char *vibro_type_str = NULL;
    switch (user_set.vibro_type)
    {
    case VIBRO_FREERUNNER:
        vibro_type_str = VIBRO_FREERUNNER_STR;
        break;
    default:
        vibro_type_str = VIBRO_DUMMY_STR;
        break;
    }
    _json_object_set_member_string(root_object, "vibro_type", vibro_type_str);

    JsonObject *input_calibration_data_object = _json_object_get_member_object(root_object, "input_calibration_data");
    _json_object_set_member_boolean(input_calibration_data_object, "swap_xy", user_set.input_calibration_data.swap_xy);
    _json_object_set_member_boolean(input_calibration_data_object, "invert_x", user_set.input_calibration_data.invert_x);
    _json_object_set_member_boolean(input_calibration_data_object, "invert_y", user_set.input_calibration_data.invert_y);
    _json_object_set_member_double(input_calibration_data_object, "cal_x", user_set.input_calibration_data.cal_x);
    _json_object_set_member_double(input_calibration_data_object, "cal_y", user_set.input_calibration_data.cal_y);
    _json_object_set_member_double(input_calibration_data_object, "sensitivity", user_set.input_calibration_data.sens);

    JsonObject *vibro_freeerunner_data_object = _json_object_get_member_object(root_object, "vibro_freeerunner_data");
    _json_object_set_member_int(vibro_freeerunner_data_object, "duration", user_set.vibro_freeerunner_data.duration);

    JsonObject *input_joystick_data_object = _json_object_get_member_object(root_object, "input_joystick_data");
    _json_object_set_member_string(input_joystick_data_object, "fname", user_set.input_joystick_data.fname);
    _json_object_set_member_double(input_joystick_data_object, "max_axis", user_set.input_joystick_data.max_axis);
    _json_object_set_member_int(input_joystick_data_object, "interval", user_set.input_joystick_data.interval);

    JsonObject *input_joystick_sdl_data_object = _json_object_get_member_object(root_object, "input_joystick_sdl_data");
    _json_object_set_member_int(input_joystick_sdl_data_object, "number", user_set.input_joystick_sdl_data.number);
    _json_object_set_member_double(input_joystick_sdl_data_object, "max_axis", user_set.input_joystick_sdl_data.max_axis);

    JsonObject *input_accel_data_object = _json_object_get_member_object(root_object, "input_accelerometer_data");
    _json_object_set_member_string(input_accel_data_object, "fname", user_set.input_accel_data.fname);
    _json_object_set_member_double(input_accel_data_object, "max_axis", user_set.input_accel_data.max_axis);
    _json_object_set_member_int(input_accel_data_object, "interval", user_set.input_accel_data.interval);
}

void SaveUserSettings()
{
    bool err = !can_save;

    if (!err)
    {
        log_info("Saving user settings file `%s'", save_file_full);
        if (!TouchDir(save_dir_full))
            err = true;
    }

    if (!err)
    {
        parser = json_parser_new();
        if (load_json(CONFIG_FILE))
        {
            SetJsonValues();
            JsonGenerator *generator = json_generator_new();
            json_generator_set_root(generator, root_node);
            GError *error = NULL;
            // Doubles serialization might not work on some Ubuntu releases
            // https://bugs.launchpad.net/ubuntu/+source/json-glib/+bug/756426
            json_generator_to_file(generator, save_file_full, &error);
            if (error)
            {
                log_error("Unable to save: %s", error->message);
                g_error_free(error);
            }
            g_object_unref(generator);
        }
        g_object_unref(parser);
    }

    free(user_set.levelpack); //
    free(user_set.input_joystick_data.fname); //
    free(cache_dir_full);
    free(save_dir_full);
    free(save_file_full);
}

void parse_command_line(int argc, char *argv[])
{
    struct arg_str *input  = arg_str0("i","input","<type>", "input device type ('dummy', 'keyboard',");
    struct arg_rem *input1 = arg_rem (NULL, "'joystick', 'joystick_sdl' or 'accelerometer')");
    struct arg_str *vibro  = arg_str0("v","vibro","<type>", "vibro device type ('dummy' or 'freerunner')");
    struct arg_str *cal    = arg_str0("c","calibration","<option>", "perform input device calibration ('auto' option)");
    struct arg_rem *cal1   = arg_rem (NULL, "or reset calibration data ('reset' option)");
    struct arg_int *level  = arg_int0("l","level",NULL, "define level from which the game will be started");
    struct arg_int *geom_x = arg_int0("x",NULL,NULL, "set window width (0 = maximum)");
    struct arg_int *geom_y = arg_int0("y",NULL,NULL, "set window height (0 = maximum)");
    struct arg_int *bpp    = arg_int0("b","bpp",NULL, "set color depth (16/32 bits/pixel, 0 = auto detect)");
    struct arg_str *scroll = arg_str0("s","scrolling","<boolean>", "scroll game window if the level does not fit to it");
    struct arg_str *fscr   = arg_str0("f","fullscreen","<mode>", "set fullscreen mode ('none', 'ingame' or 'always')");
    struct arg_lit *help   = arg_lit0(NULL,"help", "print this help and exit");
    struct arg_end *end    = arg_end (20);
    void* argtable[] = {input,input1,vibro,cal,cal1,level,geom_x,geom_y,bpp,scroll,fscr,help,end};
    const char* progname = "mokomaze";
    int nerrors = 0;

    /* verify the argtable[] entries were allocated successfully */
    if (arg_nullcheck(argtable) != 0)
    {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exit(EXIT_FAILURE);
    }

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
    {
        printf("Mokomaze - ball-in-the-labyrinth game\n");
        printf("Usage: %s", progname);
        arg_print_syntax(stdout,argtable,"\n");
        arg_print_glossary(stdout,argtable,"  %-25s %s\n");
        exit(EXIT_SUCCESS);
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
    {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout,end,progname);
        printf("Try '%s --help' for more information.\n",progname);
        exit(EXIT_FAILURE);
    }

    /* normal case: take the command line options at face value */
    if (cal->count > 0)
    {
        const char *cal_option = cal->sval[0];
        if (!strcmp(cal_option, "auto"))
            arguments.cal_auto = true;
        else if (!strcmp(cal_option, "reset"))
            arguments.cal_reset = true;
    }

    if (level->count > 0)
    {
        arguments.level = level->ival[0];
        arguments.level_set = true;
    }

    if (input->count > 0)
    {
        arguments.input_set = true;
        char *input_name = (char*)input->sval[0];
        arguments.input_type = StrToInputType(input_name, false);
    }

    if (vibro->count > 0)
    {
        arguments.vibro_set = true;
        char *vibro_name = (char*)vibro->sval[0];
        arguments.vibro_type = StrToVibroType(vibro_name, false);
    }

    if (geom_x->count > 0 && geom_y->count > 0)
    {
        arguments.geom_x = geom_x->ival[0];
        arguments.geom_y = geom_y->ival[0];
        arguments.geom_set = true;
    }

    if (bpp->count > 0)
    {
        arguments.bpp = bpp->ival[0];
        arguments.bpp_set = true;
    }

    if (scroll->count > 0)
    {
        arguments.scrolling_set = true;
        char *scroll_str = (char*)scroll->sval[0];
        arguments.scrolling = StrToBool(scroll_str, false);
    }

    if (fscr->count > 0)
    {
        arguments.fullscreen_mode_set = true;
        char *fscr_str = (char*)fscr->sval[0];
        arguments.fullscreen_mode = StrToFullscreenMode(fscr_str, false);
    }

    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));
}

bool TouchDir(char *dir)
{
    bool ok = true;
    struct stat st;
    if (stat(dir, &st) != 0)
    {
        log_info("Directory `%s' does not exists. Trying to create.", dir);
        if (mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0)
        {
            log_info("Directory `%s' created successfully.", dir);
        }
        else
        {
            log_error("Can't create directory `%s'.", dir);
            ok = false;
        }
    }
    return ok;
}

//------------------------------------------------------------------------------

MazeConfig GetGameConfig()
{
    return game_config;
}

Level* GetGameLevels()
{
    return game_levels;
}

int GetGameLevelsCount()
{
    return game_levels_count;
}

User* GetUserSettings()
{
    return &user_set;
}

Prompt GetArguments()
{
    return arguments;
}

char* GetSaveDir()
{
    return save_dir_full;
}

char* GetCacheDir()
{
    return cache_dir_full;
}
