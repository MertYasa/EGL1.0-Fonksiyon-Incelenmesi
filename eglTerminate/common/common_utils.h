#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdbool.h>

typedef struct {
    int drm_fd;
    struct gbm_device *gbm_device;
    struct gbm_surface *gbm_surface;
    uint32_t crtc_id;
    uint32_t connector_id;
    drmModeModeInfo mode_info;
} NativeWindow;

// DRM/KMS ve GBM penceresi oluşturma
bool create_drm_window(NativeWindow* nw, int width, int height, const char* title);

// DRM/KMS ve GBM penceresini yok etme
void destroy_drm_window(NativeWindow* nw);

// EGLSwapBuffers işleminden sonra fiziksel ekrana (KMS) yansıtma (page flip)
void drm_swap_buffers(EGLDisplay dpy, EGLSurface sfc, NativeWindow* nw);

// Basit bir GLES2 shader programı derleme ve üçgen çizme
void draw_colorful_triangle();

// EGL Hatalarını string olarak döndüren yardımcı fonksiyon
const char* get_egl_error_str(EGLint error);

#endif
