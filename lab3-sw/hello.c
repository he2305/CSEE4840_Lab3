/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include <stdlib.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int vga_ball_fd;

/* Read and print the background color */
void print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("%02x %02x %02x\n",
	 vla.background.red, vla.background.green, vla.background.blue);
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}

void set_coords(char x, char y)
{
	vga_ball_arg_t vla;
	vla.x = x;
	vla.y = y;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_COORDS, &vla)) {
      perror("ioctl(VGA_BALL_SET_COORDS) failed");
      return;
  }
}

int main()
{
  vga_ball_arg_t vla;
  int i;
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

  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  printf("initial state: ");
  print_background_color();
  for (i = 0 ; i < 24 ; i++) {
    set_background_color(&colors[i % COLORS ]);
    print_background_color();
    usleep(40000);
  }
  printf("Begin of moving ball\n");
  srand(time(0));
  float x_vel = ((2.0 * rand() / RAND_MAX) + 1);
  float y_vel = ((2.0 * rand() / RAND_MAX) + 1);
  x_vel = ((rand() % 2) == 1 ? x_vel : -1 * x_vel);
  y_vel = ((rand() % 2) == 1 ? y_vel : -1 * y_vel);
  float x = (rand() % (H_SIZE - 5)) + 5;
  float y = (rand() % (V_SIZE - 5)) + 5;
  set_coords(x, y);
  printf("Initial vel %f %f\n", x_vel, y_vel);
  while(1) {
    usleep(100000);
    if (x + x_vel >= H_SIZE) {
      x = H_SIZE;
      x_vel *= -1;
    } else if (x + x_vel <= 0) {
      x = 0;
      x_vel *= -1;
    } else if (y + y_vel >= V_SIZE) {
      y = V_SIZE;
      y_vel *= -1;
    } else if (y + y_vel <= 0) {
      y = 0;
      y_vel *= -1;
    } else {
      x += x_vel;
      y += y_vel;
    }
    set_coords(x, y);
    printf("Coords are x: %f, y: %f, x_vel: %f, y_vel: %f\n", x, y, x_vel, y_vel);
  }
  /*
  for (i = 0; i < 100; i++) {
		set_coords(i % 16, i % 16);
		usleep(400000);
		}
  */
  
  printf("VGA BALL Userspace program terminating\n");
  return 0;
}
