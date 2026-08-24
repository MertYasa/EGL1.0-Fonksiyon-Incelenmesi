#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <fcntl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdbool.h>

// Struct to hold our window and EGL state
typedef struct {
    int drm_fd;
    struct gbm_device *gbm_device;
    struct gbm_surface *gbm_surface;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
} AppState;

// Initialize DRM/GBM and EGL (creates display, config, surface, context)
bool init_egl_and_drm(AppState *state, int width, int height, const char* title);

// Setup a simple GLES2 shader and VBO for a colored triangle
void setup_triangle_drawing(void);

// Draw the triangle
void draw_triangle(void);

// Clean up DRM/GBM and EGL resources
void cleanup(AppState *state);

// Sleep for some milliseconds
void sleep_ms(int ms);

#endif // COMMON_UTILS_H
