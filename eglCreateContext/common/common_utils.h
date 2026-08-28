#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Gerçek Sistem Kütüphaneleri (DRM/KMS, GBM, EGL, GLES2)
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

/* =========================================================================
 * MODÜLER YAPI: Ortak DRM/KMS, GBM, EGL ve GLES2 Yardımcıları
 * Bu dosya, tüm test senaryolarının ortak kullandığı Native Display,
 * basit GLES2 shader derleme ve çizim fonksiyonlarını içerir.
 * ========================================================================= */

typedef struct {
    int fd;
    struct gbm_device *gbm_dev;
    uint32_t crtc_id;
    uint32_t connector_id;
    drmModeModeInfo mode_info;
    drmModeCrtc *saved_crtc;
} NativeDisplayContext;

// Yardımcı Fonksiyon Prototipleri
NativeDisplayContext* init_native_display();
NativeDisplayContext* init_native_display_at(int connected_index);
struct gbm_surface* create_gbm_surface(struct gbm_device *gbm_dev, int width, int height);
struct gbm_surface* create_egl_compatible_gbm_surface(EGLDisplay display, EGLConfig config, struct gbm_device *gbm_dev, int width, int height);
void present_native_display(NativeDisplayContext* ctx, struct gbm_surface* surface);
void present_native_display_for(NativeDisplayContext* ctx, struct gbm_surface* surface, unsigned int seconds);
void destroy_native_display(NativeDisplayContext* ctx);
const char* egl_error_name(EGLint error);
void log_egl_error(const char* operation);
void reset_common_gl_objects(void);
void init_simple_shader();
void draw_triangle(float z, float r, float g, float b);
void draw_quad(float x0, float y0, float x1, float y1, float z, float r, float g, float b);
GLuint create_checkerboard_texture();
void draw_textured_quad(GLuint texture_id);

#endif // COMMON_UTILS_H
