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
} NativeDisplayContext;

// Yardımcı Fonksiyon Prototipleri
NativeDisplayContext* init_native_display();
struct gbm_surface* create_gbm_surface(struct gbm_device *gbm_dev, int width, int height);
void present_native_display(NativeDisplayContext* ctx, struct gbm_surface* surface);
void destroy_native_display(NativeDisplayContext* ctx);
void init_simple_shader();
void draw_triangle(float z, float r, float g, float b);
GLuint create_checkerboard_texture();

#endif // COMMON_UTILS_H
