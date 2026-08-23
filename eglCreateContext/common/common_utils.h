#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Gerçek Sistem Kütüphaneleri (X11, EGL, GLES2)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

/* =========================================================================
 * MODÜLER YAPI: Ortak X11/EGL ve GLES2 Yardımcıları
 * Bu dosya, tüm test senaryolarının ortak kullandığı X11 pencere açma,
 * basit GLES2 shader derleme ve çizim fonksiyonlarını içerir.
 * ========================================================================= */

// Yardımcı Fonksiyon Prototipleri
Window create_x11_window(Display* x_dpy, int width, int height, const char* title);
void init_simple_shader();
void draw_triangle(float z, float r, float g, float b);
GLuint create_checkerboard_texture();

#endif // COMMON_UTILS_H
