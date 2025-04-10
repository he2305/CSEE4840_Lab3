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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int vga_ball_fd;

/* Read and print the background color */
void print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }  

  printf("Background color: %02x %02x %02x\n",
    vla.background.red, vla.background.green, vla.background.blue);
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c) {
  vga_ball_arg_t vla;
  vla.background = *c;

  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
    perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
    return;
  }

  printf("Background color: %02x %02x %02x\n",
    vla.background.red, vla.background.green, vla.background.blue);
}

/* Read and print the ball position */
void print_ball_position() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_POSITION, &vla)) {
      perror("ioctl(VGA_BALL_READ_POSITION) failed");
      return;
  }
  
  printf("Ball position: x = %d, y = %d\n", vla.position.x, vla.position.y);
}

/* Set the ball position */
void set_ball_position(unsigned short x_pos, unsigned short y_pos) {
  vga_ball_arg_t vla;
  
  vla.position.x = x_pos;
  vla.position.y = y_pos;
  
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_POSITION, &vla)) {
      perror("ioctl(VGA_BALL_WRITE_POSITION) failed");
      return;
  }

  printf("Ball position: x = %d, y = %d\n", vla.position.x, vla.position.y);
}

int main()
{
  vga_ball_arg_t vla;
  int i;
  static const char filename[] = "/dev/vga_ball";

  unsigned short ball_pos_x = 256;  /* Initial x position */
  unsigned short ball_pos_y = 128;  /* Initial y position */
  short ball_vel_x = 2;              /* X velocity (pixels per frame) */
  short ball_vel_y = 2;              /* Y velocity (pixels per frame) */
  
  /* Screen boundaries */
  const unsigned short X_MAX = 639;        /* Maximum x coordinate */
  const unsigned short Y_MAX = 479;         /* Maximum y coordinate */
  const unsigned short BALL_SIZE = 30;      /* Ball radius in pixels */

  static const vga_ball_color_t colors[] = {
    { 0xff, 0x00, 0x00 }, /* Red */
    { 0x00, 0xff, 0x00 }, /* Green */
    { 0x00, 0x00, 0xff }, /* Blue */
    { 0xff, 0xff, 0x00 }, /* Yellow */
    { 0x00, 0xff, 0xff }, /* Cyan */
    { 0xff, 0x00, 0xff }, /* Magenta */
    { 0x80, 0x80, 0x80 }, /* Gray */
    { 0x00, 0x00, 0x00 }, /* Black */
    { 0xff, 0xff, 0xff }, /* White */
    { 0xff, 0xa5, 0x00 }, /* Orange */
    { 0x80, 0x00, 0x80 }, /* Purple */
    { 0xa5, 0x2a, 0x2a }, /* Brown */
    { 0xff, 0xc0, 0xcb }, /* Pink */
    { 0x32, 0xcd, 0x32 }, /* Lime Green */
    { 0x80, 0x80, 0x00 }, /* Olive */
    { 0x00, 0x80, 0x80 }, /* Teal */
    { 0x00, 0x00, 0x80 }, /* Navy */
    { 0x80, 0x00, 0x00 }, /* Maroon */
    { 0xff, 0xd7, 0x00 }, /* Gold */
    { 0xc0, 0xc0, 0xc0 }, /* Silver */
    { 0x4b, 0x00, 0x82 }, /* Indigo */
    { 0xee, 0x82, 0xee }, /* Violet */
    { 0xf0, 0xe6, 0x8c }, /* Khaki */
    { 0xfa, 0x80, 0x72 }, /* Salmon */
    { 0x40, 0xe0, 0xd0 }, /* Turquoise */
    { 0xff, 0x7f, 0x50 }, /* Coral */
    { 0xe6, 0xe6, 0xfa }, /* Lavender */
    { 0x98, 0xff, 0x98 }, /* Mint Green */
    { 0xf5, 0xf5, 0xdc }, /* Beige */
    { 0xd2, 0x69, 0x1e }  /* Chocolate */
  };

  #define COLORS 30

  printf("VGA ball Userspace program started\n");

  if ((vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  printf("Initial state: \n");
  print_background_color();
  print_ball_position();
  
  srand(time(NULL));
  int rand_color = rand() % COLORS;
  print_background_color();
  
  printf("Starting animation\n");

  while (1) {
    ball_pos_x += ball_vel_x;
    ball_pos_y += ball_vel_y;
    
    if (ball_pos_x <= BALL_SIZE || ball_pos_x >= X_MAX - BALL_SIZE) {
      ball_vel_x = -ball_vel_x;
      
      rand_color = rand() % COLORS;
      set_background_color(&colors[rand_color]);
    }
    
    if (ball_pos_y <= BALL_SIZE || ball_pos_y >= Y_MAX - BALL_SIZE) {
      ball_vel_y = -ball_vel_y;
      
      rand_color = rand() % COLORS;
      set_background_color(&colors[rand_color]);
    }
    
    set_ball_position(ball_pos_x, ball_pos_y);
    
    usleep(25000);
  }
  
  printf("VGA BALL Userspace program terminating\n");
  return 0;
}
