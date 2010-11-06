/*  paramsloader.c
 *
 *  Config and levelpack loader
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "types.h"
#include "json.h"
#include "logging.h"

Config game_config;
Level* game_levels;
User user_set;
Prompt arguments;
int game_levels_count;
int vibro_enabled;
int vibro_force=100;
int vibro_interval=100;
char *exec_init, *exec_final;

#define SAVE_DIR ".mokomaze"
#define SAVE_FILE "user.json"
char *save_dir_full;
char *save_file_full;
int can_save = 0;


int load_file_to_mem(const char *filename, char **result)
{
    int size = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        *result = NULL;
        return -1; // opening fail
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *result = (char *) malloc(size + 1);
    if (size != fread(*result, sizeof (char), size, f))
    {
        free(*result);
        return -2; // reading fail
    }
    fclose(f);
    (*result)[size] = 0;
    return size;
}

int GetNodesCount(json_t* node)
{
    int co=0;
    while (node)
    {
        co++;
        node = node->next;
    }
    return co;
}

void load_params()
{

    char *pHome = getenv("HOME");
    if (!pHome)
    {
        can_save=0;
        log_warning("File_loader: can't retrieve environment variable HOME");
        log_warning("File_loader: savegame data not available");
    }
    else
    {
        can_save=1;
        save_dir_full = (char*)malloc(strlen(pHome) + strlen("/") + strlen(SAVE_DIR) + 1);
        strcpy(save_dir_full, pHome);
        strcat(save_dir_full, "/");
        strcat(save_dir_full, SAVE_DIR);
        save_file_full = (char*)malloc(strlen(save_dir_full) + strlen("/") + strlen(SAVE_FILE) + 1);
        strcpy(save_file_full, save_dir_full);
        strcat(save_file_full, "/");
        strcat(save_file_full, SAVE_FILE);
    }

    char* cfg;
    char* lvl;
    char* usr = NULL;
    load_file_to_mem(MDIR "config.json", &cfg);
    load_file_to_mem(MDIR "main.levelpack.json", &lvl);
    if (can_save)
    {
        if (load_file_to_mem(save_file_full, &usr) < 0)
        {
            log_info("File_loader: savegame file not found");
        }
    }

//    log_debug("%s",cfg);
//    log_debug("%s",lvl);

    char *document;;
    json_t *root;

//-----------------------------------------------------------
    document = cfg;
    //log_debug("Parsing the document...");
    json_parse_document(&root, document);
    //log_debug("Printing the document tree...");
    //json_render_tree(root);

    //log_debug("wnd_w");
    //log_debug("%s", root->child->child->child->child->text);
    game_config.wnd_w = atoi(root->child->child->child->child->text);
    //log_debug("wnd_h");
    //log_debug("%s", root->child->child->child-> next-> child->text);
    game_config.wnd_h = atoi(root->child->child->child-> next-> child->text);
    game_config.f_delay = atoi(root->child->child->child-> next->next-> child->text);
    game_config.fullscreen = atoi(root->child->child->child-> next->next->next-> child->text);

    //log_debug("ball_rad");
    //log_debug("%s", root->child-> next-> child->child->child->text);
    game_config.ball_r = atoi(root->child-> next-> child->child->child->text);

    //log_debug("hole_rad");
    //log_debug("%s", root->child-> next->next-> child->child->child->text);
    game_config.hole_r = atoi(root->child-> next->next-> child->child->child->text);

    game_config.key_r = atoi(root->child-> next->next->next-> child->child->child->text);

    vibro_enabled  = atoi(root->child-> next->next->next->next-> child->child-> child->text);
    vibro_force    = atoi(root->child-> next->next->next->next-> child->child-> next-> child->text);
    vibro_interval = atoi(root->child-> next->next->next->next-> child->child-> next->next-> child->text);
    if (vibro_force<30) vibro_force=30;
    if (vibro_force>100) vibro_force=100;
    if (vibro_interval<30) vibro_interval=30;
    if (vibro_interval>150) vibro_interval=150;

    exec_init  = strdup(root->child-> next->next->next->next->next-> child->child->child->text);
    exec_final = strdup(root->child-> next->next->next->next->next-> child->child-> next-> child->text);

    json_free_value(&root);
//-----------------------------------------------------------
//-----------------------------------------------------------
 
    document = lvl;
    //log_debug("Parsing the document...");
    json_parse_document(&root, document);
    //log_debug("Printing the document tree...");
    //json_render_tree(root);

    json_t *level = root->child->   next->next->next->   child->child;
    int levels_count = GetNodesCount(level);
    game_levels_count = levels_count;
    log_info("File_loader: %d game levels parsed", game_levels_count);
    game_levels = (Level*)malloc(sizeof(Level) * levels_count);
    int i=0;
    int j=0;
    while (level)
    {
        json_t *box = level->child->  next->  child->child;
        int boxes_count = GetNodesCount(box);
        game_levels[i].boxes_count = boxes_count;
        game_levels[i].boxes = (Box*)malloc(sizeof(Box) * boxes_count);
        j=0;
        while (box)
        {
            game_levels[i].boxes[j].x1 = atoi(box->child->child->text);
            game_levels[i].boxes[j].y1 = atoi(box->child-> next-> child->text);
            game_levels[i].boxes[j].x2 = atoi(box->child-> next->next-> child->text);
            game_levels[i].boxes[j].y2 = atoi(box->child-> next->next->next-> child->text);
            box = box->next;
            j++;
        }

        json_t *hole = level->child-> next->next-> child->child;
        int holes_count = GetNodesCount(hole);
        game_levels[i].holes_count = holes_count;
        game_levels[i].holes = (Point*)malloc(sizeof(Point) * holes_count);
        j=0;
        while (hole)
        {
            game_levels[i].holes[j].x = atoi(hole->child->child->text);
            game_levels[i].holes[j].y = atoi(hole->child-> next-> child->text);
            hole = hole->next;
            j++;
        }

        json_t *fin = level->child-> next->next->next-> child->child;
        int fins_count = GetNodesCount(fin);
        game_levels[i].fins_count = fins_count;
        game_levels[i].fins = (Point*)malloc(sizeof(Point) * fins_count);
        j=0;
        while (fin)
        {
            game_levels[i].fins[j].x = atoi(fin->child->child->text);
            game_levels[i].fins[j].y = atoi(fin->child-> next-> child->text);
            fin = fin->next;
            j++;
        }

        json_t *init = level->child-> next->next->next->next-> child;
        game_levels[i].init.x = atoi(init->child->child->text);
        game_levels[i].init.y = atoi(init->child-> next-> child->text);

        if (level->child-> next->next->next->next->next)
        {
            json_t *key = level->child-> next->next->next->next->next-> child->child;
            int keys_count = GetNodesCount(key);
            game_levels[i].keys_count = keys_count;
            game_levels[i].keys = (Point*)malloc(sizeof(Point) * keys_count);
            j=0;
            while (key)
            {
                game_levels[i].keys[j].x = atoi(key->child->child->text);
                game_levels[i].keys[j].y = atoi(key->child-> next-> child->text);
                key = key->next;
                j++;
            }
        }
        else
        {
            game_levels[i].keys_count = 0;
        }

        level = level->next;
        i++;
    }
    
    json_free_value(&root);
//==============================================================================

    int savegame_error = 0;
    if (usr)
    {
        document = usr;
        if (json_parse_document(&root, document) != JSON_OK)
        {
            savegame_error = 1;
        }
        else
        {
            json_t *save_section = json_find_first_label(root, "save");
            if ((save_section) && (save_section->child))
            {
                json_t *levelpack_section = json_find_first_label(save_section->child, "levelpack");
                if ((levelpack_section) && (levelpack_section->child) && (levelpack_section->child->text))
                {
                    strcpy(user_set.levelpack, levelpack_section->child->text);
                }
                else
                {
                    savegame_error = 1;
                }

                json_t *level_section = json_find_first_label(save_section->child, "level");
                if ((level_section) && (level_section->child) && (level_section->child->text))
                {
                    user_set.level = atoi(level_section->child->text);
                }
                else
                {
                    savegame_error = 1;
                }

                json_t *cal_x_section = json_find_first_label(save_section->child, "calibration_x");
                if ((cal_x_section) && (cal_x_section->child) && (cal_x_section->child->text))
                {
                    user_set.cal_x = atof(cal_x_section->child->text);
                }
                else
                {
                    savegame_error = 1;
                }

                json_t *cal_y_section = json_find_first_label(save_section->child, "calibration_y");
                if ((cal_y_section) && (cal_y_section->child) && (cal_y_section->child->text))
                {
                    user_set.cal_y = atof(cal_y_section->child->text);
                }
                else
                {
                    savegame_error = 1;
                }
            }
            else
            {
                savegame_error = 1;
            }
            
            // clean up
            json_free_value(&root);
        }
        
        if (savegame_error)
        {
            log_error("File_loader: error parsing savegame file");
        }
    }

    if ( (!usr) || (savegame_error) )
    {
        strcpy(user_set.levelpack, "main");
        user_set.level = 1;
    }

    if (arguments.cal_reset)
    {
        user_set.cal_x = 0;
        user_set.cal_y = 0;
    }

    if (cfg) free(cfg);
    if (lvl) free(lvl);
    if (usr) free(usr);
}

#define USERSET_TEMPLATE "{\n\t\"save\": {\n\t\t\"levelpack\": \"main\",\n\t\t\"level\"    : %d,\n\t\t\"calibration_x\": %f,\n\t\t\"calibration_y\": %f\n\t}\n}\n"
void SaveLevel(int n)
{
    if (!can_save) return;

    struct stat st;
    if (stat(save_dir_full, &st) != 0)
    {
        log_info("File_saver: directory '%s' not exists. Trying to create.", save_dir_full);
        if (mkdir(save_dir_full, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
        {
            log_error("File_saver: can't create");
            free(save_dir_full); free(save_file_full);
            return;
        }
        log_info("File_saver: created successfully");
    }

    FILE *f = fopen(save_file_full, "w");
    if (!f)
    {
            log_error("File_saver: can't open savegame file '%s' for writing", save_file_full);
            free(save_dir_full); free(save_file_full);
            return;
    }
    fprintf(f, USERSET_TEMPLATE, n, user_set.cal_x, user_set.cal_y);
    fclose(f);

    free(save_dir_full); free(save_file_full);
    return;
}

void print_help()
{
    log("Usage: mokomaze [-i input_device_type] [-l level_number] [-c option]");
    log("-i, --input");
    log("    It is used to determine input device type manually. Currently");
    log("    supported values is 'freerunner' (to use Freerunner's");
    log("    accelerometer) and 'keyboard'.");
    log("-l, --level");
    log("    Specifies a level from which the game will be started.");
    log("    Levels are counted from 1.");
    log("-c, --calibrate");
    log("    Performs the accelerometer calibration ('auto' option)");
    log("    or resets the calibration data ('reset' option).");
    log("Examples:");
    log("    mokomaze");
    log("    mokomaze -l 5");
    log("    mokomaze -i freerunner");
    log("    mokomaze -i keyboard -l 2");
    log("    mokomaze -c auto");
    log("    mokomaze -c reset");
}

void parse_command_line(int argc, char *argv[])
{
    arguments.level_set = 0;
    arguments.accel_set = 0;
    for (int i=1; i<argc; i++)
    {
        if ( (!strcmp(argv[i],"-h")) || (!strcmp(argv[i],"--help")) )
        {
            log("Mokomaze - ball-in-the-labyrinth game");
            print_help();
            exit(0);
        }
        else if ( (!strcmp(argv[i],"-c")) || (!strcmp(argv[i],"--calibrate")) )
        {
            if (i+1<argc)
            {
                char *option = argv[i+1];
                if (!strcmp(option, "auto"))
                {
                    arguments.cal_auto = 1;
                }
                else if (!strcmp(option, "reset"))
                {
                    arguments.cal_reset = 1;
                }
                else
                {
                    log_warning("Arguments_parser: unknown calibration parameter '%s'", option);
                    return;
                }
                i++;
            }
            else
            {
                log_warning("Arguments_parser: calibration parameter undefined");
                return;
            }
        }
        else if ( (!strcmp(argv[i],"-l")) || (!strcmp(argv[i],"--level")) )
        {
            if (i+1<argc)
            {
                arguments.level = atoi(argv[i+1]);
                arguments.level_set = 1;
                i++;
            }
            else
            {
                log_warning("Arguments_parser: level number undefined");
                return;
            }
        }
        else if ( (!strcmp(argv[i],"-i")) || (!strcmp(argv[i],"--input")) )
        {
            if (i+1<argc)
            {
                int known = 1;
                char *accel_name = argv[i+1];
                if (!strcmp(accel_name, "keyboard"))
                {
                    arguments.accel = ACCEL_UNKNOWN;
                }
                else if (!strcmp(accel_name, "freerunner"))
                {
                    arguments.accel = ACCEL_FREERUNNER;
                }
                else if (!strcmp(accel_name, "hdaps"))
                {
                    arguments.accel = ACCEL_HDAPS;
                }
                else
                {
                    log_warning("Arguments_parser: unknown input device type '%s'", accel_name);
                    known = 0;
                    return;
                }
                
                if (known) arguments.accel_set = 1;
                i++;
            }
            else
            {
                log_warning("Arguments_parser: input device type undefined");
                return;
            }
        }
        else
        {
            log_warning("Arguments_parser: unknown argument '%s'", argv[i]);
            return;
        }
    }
}

//-----------------------------------------------------------------------------

Config GetGameConfig()
{
    return game_config;
}

Level* GetGameLevels()
{
    return game_levels;
}

User GetUserSettings()
{
    return user_set;
}

Prompt GetArguments()
{
    return arguments;
}

int GetGameLevelsCount()
{
    return game_levels_count;
}

int GetVibroEnabled()
{
    return vibro_enabled;
}

char* GetExecInit()
{
    return exec_init;
}
char* GetExecFinal()
{
    return exec_final;
}

void SetUserCalX(float x)
{
    user_set.cal_x = x;
}
void SetUserCalY(float y)
{
    user_set.cal_y = y;
}

int GetVibroForce()
{
    return vibro_force;
}
int GetVibroInterval()
{
    return vibro_interval;
}
