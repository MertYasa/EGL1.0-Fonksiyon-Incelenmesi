#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdbool.h>

typedef struct {
    Display* x_display;
    Window x_window;
} NativeWindow;

// X11 penceresi oluşturma
bool create_x11_window(NativeWindow* nw, int width, int height, const char* title);

// X11 penceresini yok etme
void destroy_x11_window(NativeWindow* nw);

// Basit bir GLES2 shader programı derleme ve üçgen çizme
void draw_colorful_triangle();

// EGL Hatalarını string olarak döndüren yardımcı fonksiyon
const char* get_egl_error_str(EGLint error);

#endif
