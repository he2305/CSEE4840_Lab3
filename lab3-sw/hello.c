#include <stdio.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <argp.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define abs(NUM) ((NUM) > 0 ? (NUM) : -(NUM))

int vga_ball_fd;
vga_ball_dir_t dir; // Global direction variable used by update functions

/* Read and print the background color */
void print_background_color() {
    vga_ball_arg_t vla;
    
    if (ioctl(vga_ball_fd, VGA_BALL_READ, &vla)) {
        perror("ioctl(VGA_BALL_READ) failed");
        return;
    }
    printf("%02x %02x %02x\n",
           vla.bg_color.red, vla.bg_color.green, vla.bg_color.blue);
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c)
{
    vga_ball_arg_t vla;
    vla.bg_color = *c;
    if (ioctl(vga_ball_fd, VGA_BALL_WRITE, &vla)) {
        perror("ioctl(VGA_BALL_WRITE) failed");
        return;
    }
}

/* Set the circle (position and radius) */
void set_circle(const vga_ball_circle_t *circle)
{
    if (ioctl(vga_ball_fd, VGA_BALL_SET_CIRCLE, circle)) {
        perror("ioctl(VGA_BALL_SET_CIRCLE) failed");
        return;
    }
}

/* (The rest of your functions like reset_circle(), update_circle(), etc. remain unchanged) */

struct arguments {
    vga_ball_circle_t circle;
    vga_ball_dir_t dir;
    vga_ball_color_t c_color;
    vga_ball_color_t bg_color;
    int c_random_color;
};

static char args_doc[] = "-x StartX -y StartY -u DirectionX -v DirectionY";
static char doc[] = "A bouncing ball program.";
static struct argp_option options[] = {
    {"startx" , 'x' , "STARTX" , 0 , "start x coordinate of circle"},
    {"starty" , 'y' , "STARTY" , 0 , "start y coordinate of circle"},
    {"dx" , 'u' , "DELTAX" , 0 , "start x velocity"},
    {"dy" , 'v' , "DELTAY" , 0 , "start y velocity"},
    {"radius" , 'r' , "RADIUS" , 0 , "circle radius"},
    {"speed" , 's' , "SPEED" , 0 , "circle speed"},
    {"color" , 'c' , "R,G,B" , 0 , "circle color"},
    {"background" , 'b' , "R,G,B" , 0 , "background color"},
    {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = state->input;
    int arr[3] = {0};
    switch (key) {
    case 'x': args->circle.x = atoi(arg); break;
    case 'y': args->circle.y = atoi(arg); break;
    case 'u': args->dir.vx = atof(arg); break;
    case 'v': args->dir.vy = atof(arg); break;
    case 'r': args->circle.radius = atoi(arg); break;
    case 's': 
        args->dir.speed = atof(arg);
        if(args->dir.speed <= 0){
            argp_failure(state, 1, 0, "invalid argument for -s. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    case 'c':
        if(parse_nums(arg, arr, 3) < 3){
            argp_failure(state, 1, 0, "invalid argument for -c. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        if(arr[0] > 255 || arr[1] > 255 || arr[2] > 255){
            argp_failure(state, 1, 0, "invalid R,G,B for -c. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->c_random_color = 0;
        args->c_color.red = arr[0];
        args->c_color.green = arr[1];
        args->c_color.blue = arr[2];
        break;
    case 'b':
        if(parse_nums(arg, arr, 3) < 3){
            argp_failure(state, 1, 0, "invalid argument for -b. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        if(arr[0] > 255 || arr[1] > 255 || arr[2] > 255){
            argp_failure(state, 1, 0, "invalid R,G,B for -b. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->bg_color.red = arr[0];
        args->bg_color.green = arr[1];
        args->bg_color.blue = arr[2];
        break;
    default: return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char **argv)
{
    vga_ball_arg_t vla;
    int i;

    struct arguments args;
    /* Set defaults:
       - Initialize the ball at the center of a 640x480 display.
       - Give it a radius of 16.
       - Set initial velocity to (3,4) (so it will bounce).
       - Set the ball color to blue.
       - Set background to white.
    */
    args.circle.x = 320;
    args.circle.y = 240;
    args.circle.radius = 16;
    args.dir.vx = 3.0;
    args.dir.vy = 4.0;
    args.dir.speed = 1.5;
    args.dir.fx = 0.0;
    args.dir.fy = 0.0;
    args.dir.next_bound = 0;
    args.dir.corner = 0;
    args.c_random_color = 0;  // Use fixed ball color
    args.c_color.red = 0x00;
    args.c_color.green = 0x00;
    args.c_color.blue = 0xff; // Blue ball
    args.bg_color.red = 0xff;
    args.bg_color.green = 0xff;
    args.bg_color.blue = 0xff; // White background

    argp_parse(&argp, argc, argv, 0, 0, &args);

    vga_ball_circle_t circle = args.circle;
    dir = args.dir;
    vla.c_color = args.c_color;
    vla.bg_color = args.bg_color;

    static const char filename[] = "/dev/vga_ball";

    printf("VGA ball Userspace program started\n");

    if ((vga_ball_fd = open(filename, O_RDWR)) == -1) {
        fprintf(stderr, "could not open %s\n", filename);
        return -1;
    }

    /* Initialize the ball's position and velocity */
    reset_circle(&circle, &dir);
    set_circle(&circle);

    /* Set initial colors (ball color and background) */
    if (ioctl(vga_ball_fd, VGA_BALL_WRITE, &vla)) {
        perror("ioctl(VGA_BALL_WRITE) failed");
    }
    printf("initial state: ");
    print_background_color();

    /* Main loop to update ball position */
    for (i = 0; i < 100; i++) {
        update_circle(&circle, &dir);
        set_circle(&circle);
        usleep(40000); // ~40ms per frame (~25 FPS)
    }
   
    printf("VGA BALL Userspace program terminating\n");
    return 0;
}
