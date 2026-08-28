#include "common_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

#ifndef EGL_PLATFORM_GBM_MESA
#define EGL_PLATFORM_GBM_MESA 0x31D7
#endif

void init_native_window(NativeWindow* nw) {
    if (!nw) {
        return;
    }

    memset(nw, 0, sizeof(*nw));
    nw->drm_fd = -1;
}

static int crtc_index_for_id(drmModeRes *resources, uint32_t crtc_id) {
    for (int i = 0; i < resources->count_crtcs; i++) {
        if (resources->crtcs[i] == crtc_id) {
            return i;
        }
    }
    return -1;
}

static drmModeEncoder *find_usable_encoder(NativeWindow* nw, drmModeRes *resources,
                                           drmModeConnector *connector, uint32_t *crtc_id) {
    drmModeEncoder *encoder = NULL;

    if (connector->encoder_id) {
        encoder = drmModeGetEncoder(nw->drm_fd, connector->encoder_id);
        if (encoder && encoder->crtc_id) {
            int crtc_index = crtc_index_for_id(resources, encoder->crtc_id);
            if (crtc_index >= 0 && (encoder->possible_crtcs & (1u << crtc_index))) {
                *crtc_id = encoder->crtc_id;
                return encoder;
            }
        }
        if (encoder) {
            drmModeFreeEncoder(encoder);
            encoder = NULL;
        }
    }

    for (int i = 0; i < connector->count_encoders; i++) {
        encoder = drmModeGetEncoder(nw->drm_fd, connector->encoders[i]);
        if (!encoder) {
            continue;
        }

        for (int j = 0; j < resources->count_crtcs; j++) {
            if (encoder->possible_crtcs & (1u << j)) {
                *crtc_id = resources->crtcs[j];
                return encoder;
            }
        }

        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }

    return NULL;
}

static int get_drm_resources(NativeWindow* nw) {
    drmModeRes *resources = drmModeGetResources(nw->drm_fd);
    if (!resources) {
        fprintf(stderr, "drmModeGetResources basarisiz.\n");
        return -1;
    }

    drmModeConnector *connector = NULL;
    for (int i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(nw->drm_fd, resources->connectors[i]);
        if (!connector) {
            continue;
        }
        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0) {
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector) {
        fprintf(stderr, "Bagli ve aktif mode iceren DRM connector bulunamadi.\n");
        drmModeFreeResources(resources);
        return -1;
    }

    nw->connector_id = connector->connector_id;
    nw->mode_info = connector->modes[0];

    drmModeEncoder *encoder = find_usable_encoder(nw, resources, connector, &nw->crtc_id);
    if (!encoder) {
        fprintf(stderr, "Connector icin possible_crtcs maskesine uygun CRTC bulunamadi.\n");
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        return -1;
    }
    drmModeFreeEncoder(encoder);

    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    return 0;
}

bool create_drm_window(NativeWindow* nw, int width, int height, const char* title) {
    if (!nw) {
        fprintf(stderr, "NativeWindow pointer NULL; DRM window olusturulamadi.\n");
        return false;
    }

    init_native_window(nw);

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
        destroy_drm_window(nw);
        return false;
    }

    nw->old_crtc = drmModeGetCrtc(nw->drm_fd, nw->crtc_id);
    if (!nw->old_crtc) {
        fprintf(stderr, "Eski CRTC durumu kaydedilemedi; cikista restore denenemeyecek.\n");
    }

    if (width > 0 && height > 0 &&
        (width != nw->mode_info.hdisplay || height != nw->mode_info.vdisplay)) {
        printf("Bilgi: Istenen GBM surface boyutu %dx%d, aktif DRM mode %dx%d. KMS sunumu icin aktif mode boyutu kullaniliyor.\n",
               width, height, nw->mode_info.hdisplay, nw->mode_info.vdisplay);
    }

    // KMS sunumunda scanout icin aktif mode cozunurlugu kullanilir.
    nw->gbm_device = gbm_create_device(nw->drm_fd);
    if (!nw->gbm_device) {
        fprintf(stderr, "GBM cihazi olusturulamadi.\n");
        destroy_drm_window(nw);
        return false;
    }

    nw->gbm_surface = gbm_surface_create(nw->gbm_device,
                                         nw->mode_info.hdisplay,
                                         nw->mode_info.vdisplay,
                                         GBM_FORMAT_XRGB8888,
                                         GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!nw->gbm_surface) {
        fprintf(stderr, "GBM surface olusturulamadi.\n");
        destroy_drm_window(nw);
        return false;
    }

    printf("DRM/KMS ve GBM cihazi basariyla olusturuldu: %s\n", title);
    return true;
}

void destroy_drm_window(NativeWindow* nw) {
    if (!nw) {
        return;
    }

    if (nw->old_crtc && nw->drm_fd >= 0) {
        uint32_t *connectors = nw->old_crtc->mode_valid ? &nw->connector_id : NULL;
        int connector_count = nw->old_crtc->mode_valid ? 1 : 0;
        drmModeModeInfo *mode = nw->old_crtc->mode_valid ? &nw->old_crtc->mode : NULL;
        if (drmModeSetCrtc(nw->drm_fd, nw->old_crtc->crtc_id, nw->old_crtc->buffer_id,
                           nw->old_crtc->x, nw->old_crtc->y,
                           connectors, connector_count, mode) != 0) {
            fprintf(stderr, "Uyari: Eski CRTC durumu restore edilemedi.\n");
        }
    }

    if (nw->fb_id && nw->drm_fd >= 0) {
        drmModeRmFB(nw->drm_fd, nw->fb_id);
        nw->fb_id = 0;
    }
    if (nw->front_bo && nw->gbm_surface) {
        gbm_surface_release_buffer(nw->gbm_surface, nw->front_bo);
        nw->front_bo = NULL;
    }
    if (nw->old_crtc) {
        drmModeFreeCrtc(nw->old_crtc);
        nw->old_crtc = NULL;
    }
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

EGLDisplay get_egl_display_for_gbm(struct gbm_device *gbm_device) {
    EGLDisplay display = EGL_NO_DISPLAY;
    const char *client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    int has_khr_gbm = client_extensions &&
        strstr(client_extensions, "EGL_KHR_platform_gbm");
    int has_mesa_gbm = client_extensions &&
        strstr(client_extensions, "EGL_MESA_platform_gbm");
    int has_gbm_platform = has_khr_gbm || has_mesa_gbm;
    EGLenum gbm_platform = has_khr_gbm ? EGL_PLATFORM_GBM_KHR : EGL_PLATFORM_GBM_MESA;

#ifdef EGL_VERSION_1_5
    if (has_gbm_platform) {
        display = eglGetPlatformDisplay(gbm_platform, gbm_device, NULL);
        if (display != EGL_NO_DISPLAY) {
            printf("EGL GBM display eglGetPlatformDisplay ile alindi.\n");
            return display;
        }
        printf("Bilgi: eglGetPlatformDisplay GBM display dondurmedi, fallback denenecek.\n");
    }
#endif

    if (has_gbm_platform) {
        PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplayEXT =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
        if (getPlatformDisplayEXT) {
            display = getPlatformDisplayEXT(gbm_platform, gbm_device, NULL);
            if (display != EGL_NO_DISPLAY) {
                printf("EGL GBM display eglGetPlatformDisplayEXT ile alindi.\n");
                return display;
            }
            printf("Bilgi: eglGetPlatformDisplayEXT GBM display dondurmedi, eglGetDisplay fallback denenecek.\n");
        }
    }

    display = eglGetDisplay((EGLNativeDisplayType)gbm_device);
    if (display != EGL_NO_DISPLAY) {
        printf("EGL GBM display eglGetDisplay fallback ile alindi.\n");
    }
    return display;
}

bool drm_swap_buffers(EGLDisplay dpy, EGLSurface sfc, NativeWindow* nw) {
    if (!nw || !nw->gbm_surface || nw->drm_fd < 0) {
        fprintf(stderr, "DRM present icin NativeWindow hazir degil.\n");
        return false;
    }

    if (!eglSwapBuffers(dpy, sfc)) {
        EGLint err = eglGetError();
        fprintf(stderr, "eglSwapBuffers basarisiz: %s\n", get_egl_error_str(err));
        return false;
    }

    struct gbm_bo *bo = gbm_surface_lock_front_buffer(nw->gbm_surface);
    if (!bo) {
        fprintf(stderr, "gbm_surface_lock_front_buffer basarisiz; DRM present yapilamadi.\n");
        return false;
    }

    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t pitch = gbm_bo_get_stride(bo);
    uint32_t fb_id = 0;

    if (drmModeAddFB(nw->drm_fd, nw->mode_info.hdisplay, nw->mode_info.vdisplay,
                     24, 32, pitch, handle, &fb_id) != 0) {
        fprintf(stderr, "DRM framebuffer olusturulamadi.\n");
        gbm_surface_release_buffer(nw->gbm_surface, bo);
        return false;
    }

    if (drmModeSetCrtc(nw->drm_fd, nw->crtc_id, fb_id, 0, 0,
                       &nw->connector_id, 1, &nw->mode_info) != 0) {
        fprintf(stderr, "drmModeSetCrtc basarisiz; goruntu ekrana basilmadi.\n");
        drmModeRmFB(nw->drm_fd, fb_id);
        gbm_surface_release_buffer(nw->gbm_surface, bo);
        return false;
    }

    if (nw->fb_id) {
        drmModeRmFB(nw->drm_fd, nw->fb_id);
    }
    if (nw->front_bo) {
        gbm_surface_release_buffer(nw->gbm_surface, nw->front_bo);
    }

    nw->fb_id = fb_id;
    nw->front_bo = bo;
    return true;
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

bool draw_colorful_triangle() {
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
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader) {
            glDeleteShader(fragmentShader);
        }
        return false;
    }

    GLuint programObject = glCreateProgram();
    if (!programObject) {
        fprintf(stderr, "Shader programi olusturulamadi.\n");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    glLinkProgram(programObject);

    GLint linked = 0;
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = malloc(infoLen);
            if (infoLog) {
                glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
                fprintf(stderr, "Shader link hatasi: %s\n", infoLog);
                free(infoLog);
            }
        }
        glDeleteProgram(programObject);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

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

    GLint positionLoc = glGetAttribLocation(programObject, "vPosition");
    GLint colorLoc = glGetAttribLocation(programObject, "vColor");
    if (positionLoc < 0 || colorLoc < 0) {
        fprintf(stderr, "Shader attribute konumlari alinamadi.\n");
        glDeleteProgram(programObject);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(positionLoc);

    glVertexAttribPointer(colorLoc, 4, GL_FLOAT, GL_FALSE, 0, vColors);
    glEnableVertexAttribArray(colorLoc);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDeleteProgram(programObject);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
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
