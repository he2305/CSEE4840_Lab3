/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

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
 
 /* Set the circle coordinates and radius */
 void set_circle(const vga_ball_circle_t *circle)
 {
     if (ioctl(vga_ball_fd, VGA_BALL_SET_CIRCLE, circle)) {
         perror("ioctl(VGA_BALL_SET_CIRCLE) failed");
         return;
     }
 }
 
 int parse_nums(const char *arg, int *arr, int max)
 {
     int *p = arr;
     int last_is_num = 0;
     while(*arg) {
         if(*arg >= '0' && *arg <= '9'){
             if(p == arr && !last_is_num)
                 *p = 0;
             *p = (*p) * 10 + *arg - '0';
             last_is_num = 1;
         }
         else{
             if(last_is_num){
                 ++p;
                 if(p - arr >= max)
                     return max;
             }
             last_is_num = 0;
         }
         ++arg;
     }
     if(last_is_num)
         ++p;
     return p - arr;
 }
 
 struct arguments{
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
     {"dx" , 'u' , "DELTAX" , 0 , "start x direction"},
     {"dy" , 'v' , "DELTAY" , 0 , "start y direction"},
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
         printf("Circle %u %u %u\n", args->c_color.red, args->c_color.green, args->c_color.blue);
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
         printf("Background %u %u %u\n", args->bg_color.red, args->bg_color.green, args->bg_color.blue);
         break;
     case ARGP_KEY_END:
         if((2 * args->circle.radius + abs(args->dir.dx) > 640) ||
            (2 * args->circle.radius + abs(args->dir.dy) > 480)){
             argp_failure(state, 1, 0, "invalid argument -u/-v/-r too large. See --help for more information");
             exit(ARGP_ERR_UNKNOWN);
         }
         break;
     default: return ARGP_ERR_UNKNOWN;
     }
     return 0;
 }
 
 /*
  * Fast inverse square root function.
  * https://github.com/RuralAnemone/quake-3-fast-inverse-sqrt/blob/main/src/main-og.c
  */
 float Q_rsqrt(float number) {
     long i;
     float x2, y;
     const float threehalfs = 1.5F;
  
     x2 = number * 0.5;
     y  = number;
     i  = * (long *) &y; 
     i  = 0x5f3759df - (i >> 1);
     y  = * (float *) &i;
     y  = y * (threehalfs - (x2 * y * y));
     return y;
 }
 
 void reset_circle(vga_ball_circle_t *circle, vga_ball_dir_t *dir)
 {
     // Correct position if out-of-bounds
     if(circle->x < circle->radius){
         circle->x = circle->radius;
         dir->dx = abs(dir->dx);
     }
     if(circle->x > 640 - circle->radius){
         circle->x = 640 - circle->radius;
         dir->dx = -abs(dir->dx);
     }
     if(circle->y < circle->radius){
         circle->y = circle->radius;
         dir->dy = abs(dir->dy);
     }
     if(circle->y > 480 - circle->radius){
         circle->y = 480 - circle->radius;
         dir->dy = -abs(dir->dy);
     }
     dir->next_bound = 0;
  
     float rlen = Q_rsqrt(dir->vx * dir->vx + dir->vy * dir->vy);
     dir->vx *= rlen * dir->speed;
     dir->vy *= rlen * dir->speed;
  
     // Reset fractional accumulators to ensure smooth movement
     dir->fx = 0;
     dir->fy = 0;
 }
 
 void update_circle(vga_ball_circle_t *circle, vga_ball_dir_t *dir)
 {
     dir->fx += dir->vx;
     dir->fy += dir->vy;
     dir->dx = (short) dir->fx;
     dir->dy = (short) dir->fy;
     dir->fx -= dir->dx;
     dir->fy -= dir->dy;
  
     /* Simple collision detection against screen bounds */
     if ((circle->x + dir->dx < circle->radius) || (circle->x + dir->dx > 640 - circle->radius))
         dir->vx = -dir->vx;
     if ((circle->y + dir->dy < circle->radius) || (circle->y + dir->dy > 480 - circle->radius))
         dir->vy = -dir->vy;
  
     circle->x += dir->dx;
     circle->y += dir->dy;
 }
  
 struct argp argp = {options, parse_opt, args_doc, doc};
 vga_ball_dir_t dir;
  
 int main(int argc, char **argv)
 {
     vga_ball_arg_t vla;
     int i;
  
     struct arguments args;
     args.circle.radius = 16;
     args.dir.dx = 0;
     args.dir.speed = 1.5;
     args.dir.dy = 0;
     args.c_random_color = 1;
     args.bg_color.red = args.bg_color.green = args.bg_color.blue = 0xff;
     args.c_color.red = 0xff;
     args.c_color.green = args.c_color.blue = 0;
  
     argp_parse(&argp, argc, argv, 0, 0, &args);
  
     /* If no starting position is provided, default to the center of the screen */
     if (args.circle.x == 0 && args.circle.y == 0) {
         args.circle.x = 320;
         args.circle.y = 240;
     }
  
     /* If the direction vector is zero, give it a default velocity */
     if (args.dir.vx == 0 && args.dir.vy == 0) {
         args.dir.vx = 1.0;
         args.dir.vy = 1.0;
         args.dir.fx = 0.0;
         args.dir.fy = 0.0;
     }
  
     vga_ball_circle_t circle = args.circle;
     dir = args.dir;
     vla.c_color = args.c_color;
     vla.bg_color = args.bg_color;
  
     static const char filename[] = "/dev/vga_ball";
  
     static const vga_ball_color_t colors[] = {
         { 0xff, 0x00, 0x00 }, /* Red */
         { 0x00, 0xff, 0x00 }, /* Green */
         { 0x00, 0x00, 0xff }, /* Blue */
         { 0xff, 0xff, 0x00 }, /* Yellow */
         { 0x00, 0xff, 0xff }, /* Cyan */
         { 0xff, 0x00, 0xff }, /* Magenta */
         { 0x80, 0x80, 0x80 }, /* Gray */
         { 0x00, 0x00, 0x00 }, /* Black */
         { 0xff, 0xff, 0xff }  /* White */
     };
  
 # define COLORS 9
  
     printf("VGA ball Userspace program started\n");
  
     if ((vga_ball_fd = open(filename, O_RDWR)) == -1) {
         fprintf(stderr, "could not open %s\n", filename);
         return -1;
     }
  
     /* Initialize ball position and direction */
     reset_circle(&circle, &dir);
     set_circle(&circle);
  
     /* Set initial colors */
     if (ioctl(vga_ball_fd, VGA_BALL_WRITE, &vla)) {
         perror("ioctl(VGA_BALL_WRITE) failed");
     }
     printf("initial state: ");
     print_background_color();
  
     int j = 0;
     int dr = 5;
  
     for (i = 0; i < 24; i++) {
         j++;
         if (args.c_random_color) {
             vla.c_color = colors[i % COLORS];
         }
         /* Update the background color */
         set_background_color(&colors[i % COLORS]);
         print_background_color();
  
         /* Optionally adjust the circle's radius (if desired) */
         if(circle.radius > 100)
             dr = -5;
         if(circle.radius < 10)
             dr = 5;
  
         update_circle(&circle, &dir);
  
         /* Update the ball’s position on the FPGA */
         set_circle(&circle);
  
         usleep(400000);
     }
    
     printf("VGA BALL Userspace program terminating\n");
     return 0;
 }
 
