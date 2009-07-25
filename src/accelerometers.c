/*  accelerometers.c
 *
 *  Accelerometer stuff
 *
 *  (c) 2009 Anton Olkhovik <ant007h@gmail.com>
 *
 *  Based on original accelerometers.c from OpenMooCow - accelerometer moobox simulator.
 *  (c) 2008 Thomas White <taw27@srcf.ucam.org>
 *  (c) 2008 Joachim Breitner <mail@joachim-breitner.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <math.h>

#include "paramsloader.h"
#include "accelerometers.h"
#include "types.h"

int finished=0;
SDL_Thread *thread;
double acx=0,acy=0,acz=0;
int recv_data=0;
Prompt arguments;

#define GET_DATA_INTERVAL 25000
#define WAIT_DATA_TIME 1500000
#define CALIBR_COUNT 60

typedef struct {

	int		fd;

	AccelType	type;

	int		x;
	int		y;
	int		z;

	/* Temporary values for use during reading */
	int		lx;
	int		ly;
	int		lz;

	/* Current state (driver dependent) */
	int		state;
	signed int	old_threshold;
	int     	threshold_changed;

} AccelHandle;

struct input_event {
	struct timeval time;
	uint16_t type;
	uint16_t code;
	int32_t value;
};
#define EV_SYN (0x00)
#define EV_REL (0x02)
#define EV_ABS (0x03)
#define SYN_REPORT (0x00)
#define REL_X (0x00)
#define REL_Y (0x01)
#define REL_Z (0x02)
#define ABS_X (0x00)
#define ABS_Y (0x01)
#define ABS_Z (0x02)

/* Try to open the threshold sysfs file for Freerunner's accelerometers.
 * Try methods for both old and new kernels */
static FILE *accelerometer_freerunner_open_threshold(const char *mode)
{
	FILE *fh;

	/* Try 2.6.24 method */
	fh = fopen("/sys/devices/platform/lis302dl.2/threshold", mode);
	if ( fh != NULL ) {
		return fh;
	}

	/* Try 2.6.28+ method */
	fh = fopen("/sys/class/i2c-adapter/i2c-0/0-0073/lis302dl.2/threshold", mode);
	if ( fh != NULL ) {
		return fh;
	}

	/* Try 2.6.29+ method */
	fh = fopen("/sys/class/i2c-adapter/i2c-0/0-0073/spi_s3c24xx_gpio.0/spi3.1/threshold", mode);
	if ( fh != NULL ) {
		return fh;
	}

        return NULL;
}

static void accelerometer_freerunner_try_threshold(AccelHandle *accel)
{

        FILE *fh;
	int rval;

        accel->old_threshold = -1;
        accel->threshold_changed = 0;

	/* Save old threshold value */
	fh = accelerometer_freerunner_open_threshold("r");
        if ( fh == NULL )
        {
            fprintf(stderr, "Accelerometer: can't open threshold file for reading.\n");
            return;
        }
        signed int trh = -1;
        rval = fscanf(fh, "%i", &trh);
	if ( rval != 1 )
        {
            fprintf(stderr, "Accelerometer: can't read threshold value.\n");
	}
        else
        {
            printf("Accelerometer: current threshold value is %i\n", trh);
            accel->old_threshold = trh;
        }
        fclose(fh);

        if (trh == 0)
        {
            printf("Accelerometer: threshold is not required to be changed.\n");
            return;
        }

	/* Set threshold to zero */
	fh = accelerometer_freerunner_open_threshold("w");
	if ( fh == NULL )
        {
            fprintf(stderr, "Accelerometer: can't open threshold file for writing.\n");
            return;
	}
	fprintf(fh, "0\n");
	fclose(fh);
        printf("Accelerometer: threshold disabled successfully.\n");
        accel->threshold_changed = 1;

}

static void accelerometer_freerunner_try_restore_threshold(AccelHandle *accel)
{

	FILE *fh;
	int rval;
	int new_threshold = 0;

	if ( (!(accel->threshold_changed)) ||
               (accel->old_threshold < 0) )
        {
		printf("Accelerometer: threshold is not required to be restored.\n");
                return;
	}

        printf("Accelerometer: trying to restore old threshold value.\n");

	fh = accelerometer_freerunner_open_threshold("r");
	if ( fh == NULL ) {
		fprintf(stderr, "Accelerometer: can't open threshold file for reading.\n");
		return;
	}
	rval = fscanf(fh, "%i", &new_threshold);
	if ( rval != 1 ) {
		fprintf(stderr, "Accelerometer: can't read new threshold\n");
                new_threshold = 0;
	}
	fclose(fh);

	/* Restore only if it hasn't been altered externally */
	if ( new_threshold != 0 ) {
		printf("Accelerometer: threshold has been altered externally - not restoring.\n");
		return;
	}

	/* Set it back to the old value */
	fh = accelerometer_freerunner_open_threshold("w");
	if ( fh == NULL ) {
		fprintf(stderr, "Accelerometer: can't open threshold file for writing.\n");
		return;
	}
	fprintf(fh, "%i\n", accel->old_threshold);
	fclose(fh);
	printf("Accelerometer: threshold restored successfully.\n");

}

AccelHandle *accelerometer_open()
{
	AccelHandle *accel;

	/* Initialise accelerometer data structure */
	accel = malloc(sizeof(AccelHandle));
	if ( accel == NULL ) return NULL;
	accel->x = 0;
	accel->y = 0;
	accel->z = 0;
	accel->lx = 0;
	accel->ly = 0;
	accel->lz = 0;
	accel->state = 0;
	accel->type = ACCEL_UNKNOWN;

        int cant_open = 0;
        struct stat st;

        /* Determine accelerometer type */

        if (!arguments.accel_set)
            printf("Accelerometer: starting auto-detection of device type\n");

        if ( !((arguments.accel_set) && (arguments.accel != ACCEL_HDAPS)) )
        {
            #define HDAPS_FILE "/dev/input/by-path/platform-hdaps-event-joystick"
            if (stat(HDAPS_FILE, &st) == 0)
            {
                accel->fd = open(HDAPS_FILE, O_RDONLY, O_NONBLOCK);
                if ( accel->fd != -1 ) {
                        //accel->type = ACCEL_HDAPS;
                        printf("Accelerometer: ThinkPad HDAPS detected\n");
                        printf("Accelerometer: this devices is not supported yet. Switching to keyboard input\n");
                        return accel;
                }
                else
                {
                        fprintf(stderr, "Accelerometer: ThinkPad HDAPS device file detected, but can't be opened. " \
                                        "Check access permissions\n");
                        cant_open = 1;
                }
            }
            else
            {
                if (arguments.accel_set)
                {
                    fprintf(stderr, "Accelerometer: ThinkPad HDAPS device file not found\n");
                    cant_open = 1;
                }
            }
        }

        if ( !((arguments.accel_set) && (arguments.accel != ACCEL_FREERUNNER)) )
        {
            #define FREERUNNER_FILE "/dev/input/event3"
            if (stat(FREERUNNER_FILE, &st) == 0)
            {
                accel->fd = open(FREERUNNER_FILE, O_RDONLY, O_NONBLOCK); //, O_RDONLY, 0);
                if ( accel->fd != -1 ) {

                        accel->type = ACCEL_FREERUNNER;
                        printf("Accelerometer: Neo Freerunner detected\n");
                        accelerometer_freerunner_try_threshold(accel);
                        return accel;
                }
                else
                {
                        fprintf(stderr, "Accelerometer: Neo Freerunner accelerometer file detected, but can't be opened. " \
                                        "Check access permissions\n");
                        cant_open = 1;
                }
            }
            else
            {
                if (arguments.accel_set)
                {
                    fprintf(stderr, "Accelerometer: Neo Freerunner accelerometer file not found\n");
                    cant_open = 1;
                }
            }
        }

	/* Try other types here */

        if ( !((arguments.accel_set) && (arguments.accel == ACCEL_UNKNOWN)) )
            if (cant_open == 0)
            {
                fprintf(stderr, "Accelerometer: couldn't determine accelerometer type\n");
            }
	fprintf(stderr, "Accelerometer: switching to keyboard input\n");
	return accel;
}

static void accelerometer_shutdown(AccelHandle *accel)
{
	if ( accel->type == ACCEL_FREERUNNER ) {
		accelerometer_freerunner_try_restore_threshold(accel);
	}
}

int accelerometer_moo_hdaps(AccelHandle *accel)
{
	struct input_event ev;
	size_t rval;

	rval = read(accel->fd, &ev, sizeof(ev));
	if ( rval != sizeof(ev) ) {
		fprintf(stderr, "Accelerometer: couldn't read data\n");
		return 0;
	}
	if (ev.type == EV_ABS && ev.code == REL_Y) {
		//ev.value
	}

        //TODO: setting normalized values to acx,acy,acz variables

	return 0;

	/*
 	fprintf(stderr, "Event: time %ld.%06ld, type %s, code %d, value %d\n",
		       ev.time.tv_sec, ev.time.tv_usec,
		       ev.type == EV_REL ? "REL" :
                       ev.type == EV_ABS ? "ABS" : "Other" ,
		       ev.code, ev.value);
	*/
}

int calibr_val_count=0;
int calibrated=0;
double zero_asin_acx=0;
double zero_asin_acy=0;
double mid_acx=0;
double mid_acy=0;
int accelerometer_moo_freerunner(AccelHandle *accel)
{
	struct input_event ev;
	size_t rval;
	fd_set fds;
	struct timeval t;

	FD_ZERO(&fds);
	FD_SET(accel->fd, &fds);
	t.tv_sec = 0;
	t.tv_usec = 0;
	select(1+accel->fd, &fds, NULL, NULL, &t);

	if ( FD_ISSET(accel->fd, &fds) ) {
            
		rval = read(accel->fd, &ev, sizeof(ev));
		if ( rval != sizeof(ev) ) {
			fprintf(stderr, "Accelerometer: couldn't read data\n");
			return 0;
		}

	} else {
		return 0;	/* No data */
	}

	if ( ev.type == EV_REL ) {
		if ( ev.code == REL_X ) {
			accel->lx = ev.value;
                        recv_data = 1;
		}
		if ( ev.code == REL_Y ) {
			accel->ly = ev.value;
                        recv_data = 1;
		}
		if ( ev.code == REL_Z ) {
			accel->lz = ev.value;
                        recv_data = 1;
		}
	}

	if ( ev.type == EV_ABS ) {
		if ( ev.code == ABS_X ) {
			accel->lx = ev.value;
                        recv_data = 1;
		}
		if ( ev.code == ABS_Y ) {
			accel->ly = ev.value;
                        recv_data = 1;
		}
		if ( ev.code == ABS_Z ) {
			accel->lz = ev.value;
                        recv_data = 1;
		}
	}

	if ( ev.type == EV_SYN ) {
		if ( ev.code == SYN_REPORT ) {
			accel->x = accel->lx;
			accel->y = accel->ly;
			accel->z = accel->lz;
                        recv_data = 1;
		}
	}

	acx = accel->x/1000.0;
	acy = accel->y/1000.0;
	acz = accel->z/1000.0;

        if ((arguments.cal_auto) && (!calibrated))
            if (calibr_val_count < CALIBR_COUNT)
            {
                mid_acx += asin(acx);
                mid_acy += asin(acy);
                calibr_val_count++;
            }
            else
            {
                mid_acx /= (double)CALIBR_COUNT;
                mid_acy /= (double)CALIBR_COUNT;
                zero_asin_acx = mid_acx;
                zero_asin_acy = mid_acy;
                SetUserCalX(zero_asin_acx);
                SetUserCalY(zero_asin_acy);
                calibrated = 1;
                printf("Accelerometer: gravity calibrated (%.2f deg; %.2f deg)\n", zero_asin_acx*180.0/M_PI, zero_asin_acy*180.0/M_PI);
            }

        if ((zero_asin_acx!=0) || (zero_asin_acy!=0))
        {
            acx = sin(asin(acx)-zero_asin_acx);
            acy = sin(asin(acy)-zero_asin_acy);
        }

	return 0;
}

#define KBD_TILT 0.7
int accelerometer_kbd(AccelHandle *accel)
{
        double kbd_x=0, kbd_y=0;

        Uint8 *keystate = SDL_GetKeyState(NULL);
        if ( keystate[SDLK_UP] ) kbd_y = KBD_TILT;
        else
            if ( keystate[SDLK_DOWN] ) kbd_y = -KBD_TILT;
        if ( keystate[SDLK_LEFT] ) kbd_x = -KBD_TILT;
        else
            if ( keystate[SDLK_RIGHT] ) kbd_x = KBD_TILT;

        acx = kbd_x;
        acy = kbd_y;

        return 0;
}

int accelerometer_moo(AccelHandle *accel)
{
	switch ( accel->type ) {
		case ACCEL_UNKNOWN : {
			//return 0;
			return accelerometer_kbd(accel);
		}
		case ACCEL_FREERUNNER : {
			return accelerometer_moo_freerunner(accel);
		}
		case ACCEL_HDAPS : {
			return accelerometer_moo_hdaps(accel);
		}
		/* Add other types here. */
	}

	return 0;
}

/* The accelerometer work thread */
int accel_work(void *data)
{
	AccelHandle *accel;
	int *finished = data;

        arguments = GetArguments();

        zero_asin_acx = GetUserSettings().cal_x;
        zero_asin_acy = GetUserSettings().cal_y;
        if ((zero_asin_acx!=0) || (zero_asin_acy!=0))
        {
            printf("Accelerometer: gravity calibration data loaded (%.2f deg; %.2f deg)\n", zero_asin_acx*180.0/M_PI, zero_asin_acy*180.0/M_PI);
        }

	accel = accelerometer_open();

        recv_data = 0;
        int past_time = 0;
        int recv_stable;
        recv_stable =  ((accel->type != ACCEL_FREERUNNER) || (arguments.accel_set));

	while ( !(*finished) ) {
		accelerometer_moo(accel);
                if (!recv_stable)
                {
                    if (recv_data)
                    {
                        recv_stable = 1;
                    }
                    else
                    {
                        if (past_time > WAIT_DATA_TIME)
                        {
                            fprintf(stderr, "Accelerometer: no data recieved. Switching to keyboard input\n");
                            accelerometer_shutdown(accel);
                            accel->type = ACCEL_UNKNOWN;
                            recv_stable = 1;
                        }
                        else
                        {
                            past_time += GET_DATA_INTERVAL;
                        }
                    }
                }
		usleep(GET_DATA_INTERVAL);
	}

	accelerometer_shutdown(accel);

	return 0;
}

void accelerometer_start()
{
	finished = 0;
	thread = SDL_CreateThread(accel_work, &finished);
}

void accelerometer_stop()
{
	finished = 1;
	SDL_WaitThread(thread, NULL);
}

double getacx()
{
	return acx;
}
double getacy()
{
	return acy;
}
double getacz()
{
	return acz;
}
