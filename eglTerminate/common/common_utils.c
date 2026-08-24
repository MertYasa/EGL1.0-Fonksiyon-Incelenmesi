#include "common_utils.h"
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

static int get_drm_resources(NativeWindow* nw) {
    drmModeRes *resources = drmModeGetResources(nw->drm_fd);
    if (!resources) {
        fprintf(stderr, "drmModeGetResources basarisiz.\n");
        return -1;
    }

    drmModeConnector *connector = NULL;
    for (int i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(nw->drm_fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector) {
        fprintf(stderr, "Aktif DRM connector bulunamadi.\n");
        drmModeFreeResources(resources);
        return -1;
    }

    nw->connector_id = connector->connector_id;
    nw->mode_info = connector->modes[0];

    drmModeEncoder *encoder = NULL;
    if (connector->encoder_id) {
        encoder = drmModeGetEncoder(nw->drm_fd, connector->encoder_id);
    }

    if (encoder) {
        nw->crtc_id = encoder->crtc_id;
        drmModeFreeEncoder(encoder);
    } else {
        // Fallback: Just pick the first CRTC
        nw->crtc_id = resources->crtcs[0];
    }

    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    return 0;
}

bool create_drm_window(NativeWindow* nw, int width, int height, const char* title) {
    // We try card0 or card1
    nw->drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (nw->drm_fd < 0) {
        nw->drm_fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    }

    if (nw->drm_fd < 0) {
        fprintf(stderr, "DRM cihazina erisilemedi (/dev/dri/card0 veya card1).\n");
        return false;
    }

    if (get_drm_resources(nw) < 0) {
        close(nw->drm_fd);
        return false;
    }

    // Use mode resolution instead of passed width/height if we want full screen KMS
    nw->gbm_device = gbm_create_device(nw->drm_fd);
    if (!nw->gbm_device) {
        fprintf(stderr, "GBM cihazi olusturulamadi.\n");
        close(nw->drm_fd);
        return false;
    }

    nw->gbm_surface = gbm_surface_create(nw->gbm_device,
                                         nw->mode_info.hdisplay,
                                         nw->mode_info.vdisplay,
                                         GBM_FORMAT_XRGB8888,
                                         GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!nw->gbm_surface) {
        fprintf(stderr, "GBM surface olusturulamadi.\n");
        gbm_device_destroy(nw->gbm_device);
        close(nw->drm_fd);
        return false;
    }

    printf("DRM/KMS ve GBM cihazi basariyla olusturuldu: %s\n", title);
    return true;
}

void destroy_drm_window(NativeWindow* nw) {
    if (nw->gbm_surface) {
        gbm_surface_destroy(nw->gbm_surface);
        nw->gbm_surface = NULL;
    }
    if (nw->gbm_device) {
        gbm_device_destroy(nw->gbm_device);
        nw->gbm_device = NULL;
    }
    if (nw->drm_fd >= 0) {
        close(nw->drm_fd);
        nw->drm_fd = -1;
    }
}

void drm_swap_buffers(EGLDisplay dpy, EGLSurface sfc, NativeWindow* nw) {
    eglSwapBuffers(dpy, sfc);

    struct gbm_bo *bo = gbm_surface_lock_front_buffer(nw->gbm_surface);
    if (!bo) {
        return;
    }

    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t pitch = gbm_bo_get_stride(bo);
    uint32_t fb_id;

    if (drmModeAddFB(nw->drm_fd, nw->mode_info.hdisplay, nw->mode_info.vdisplay,
                     24, 32, pitch, handle, &fb_id) == 0) {

        drmModeSetCrtc(nw->drm_fd, nw->crtc_id, fb_id, 0, 0, &nw->connector_id, 1, &nw->mode_info);
    }

    gbm_surface_release_buffer(nw->gbm_surface, bo);
}

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = malloc(infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            fprintf(stderr, "Shader derleme hatasi: %s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void draw_colorful_triangle() {
    const char* vShaderStr =
        "attribute vec4 vPosition;    \n"
        "attribute vec4 vColor;       \n"
        "varying vec4 fColor;         \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = vPosition;  \n"
        "   fColor = vColor;          \n"
        "}                            \n";

    const char* fShaderStr =
        "precision mediump float;     \n"
        "varying vec4 fColor;         \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_FragColor = fColor;    \n"
        "}                            \n";

    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vShaderStr);
    GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fShaderStr);

    GLuint programObject = glCreateProgram();
    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    glLinkProgram(programObject);

    glUseProgram(programObject);

    GLfloat vVertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };
    GLfloat vColors[] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f
    };

    GLuint positionLoc = glGetAttribLocation(programObject, "vPosition");
    GLuint colorLoc = glGetAttribLocation(programObject, "vColor");

    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(positionLoc);

    glVertexAttribPointer(colorLoc, 4, GL_FLOAT, GL_FALSE, 0, vColors);
    glEnableVertexAttribArray(colorLoc);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

const char* get_egl_error_str(EGLint error) {
    switch (error) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
        default: return "UNKNOWN_EGL_ERROR";
    }
}
