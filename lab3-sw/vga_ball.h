#ifndef _VGA_BALL_H
#define _VGA_BALL_H

#include <linux/ioctl.h>
#include <stddef.h>

typedef struct {
    unsigned char red, green, blue;
} vga_ball_color_t;

typedef struct {
    unsigned char left, top, right, bottom;
} vga_ball_rect_t;

typedef struct __attribute__((packed)) {
  short x, y;
  unsigned char radius;
} vga_ball_circle_t;

typedef struct {
    short dx, dy;
    short next_bound;
    short corner;
    float vx, vy, fx, fy;
    float speed;
} vga_ball_dir_t;

typedef struct {
    vga_ball_color_t c_color;
    vga_ball_color_t bg_color;
    vga_ball_circle_t circle;
} vga_ball_arg_t;

#define VGA_BALL_MAGIC 'q'

/* Ioctl to update colors (circle and background) */
#define VGA_BALL_WRITE _IOW(VGA_BALL_MAGIC, 1, vga_ball_arg_t *)

/* Ioctl to read current device state */
#define VGA_BALL_READ  _IOR(VGA_BALL_MAGIC, 2, vga_ball_arg_t *)

/* New: Ioctl to update just the circle coordinates */
#define VGA_BALL_SET_CIRCLE _IOW(VGA_BALL_MAGIC, 3, vga_ball_circle_t *)

#endif
