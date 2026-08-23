#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdbool.h>

// Struct to hold our window and EGL state
typedef struct {
    Display *x_display;
    Window x_window;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
} AppState;

// Initialize X11 window and EGL (creates display, config, surface, context)
bool init_egl_and_x11(AppState *state, int width, int height, const char* title);

// Setup a simple GLES2 shader and VBO for a colored triangle
void setup_triangle_drawing(void);

// Draw the triangle
void draw_triangle(void);

// Clean up X11 and EGL resources
void cleanup(AppState *state);

// Sleep for some milliseconds
void sleep_ms(int ms);

#endif // COMMON_UTILS_H
