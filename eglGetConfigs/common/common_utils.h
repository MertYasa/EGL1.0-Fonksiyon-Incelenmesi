#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <fcntl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

typedef struct {
    int drm_fd;
    struct gbm_device *gbm_device;
    struct gbm_surface *gbm_surface;
} AppState;

bool init_drm_and_gbm(AppState *state, int width, int height);
void cleanup_drm_and_gbm(AppState *state);

void init_simple_shader();
void draw_triangle(float z, float r, float g, float b);
GLuint create_checkerboard_texture();

#endif // COMMON_UTILS_H
