#include "common_utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

static const char *egl_error_name(EGLint error) {
    switch (error) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        default: return "UNKNOWN_EGL_ERROR";
    }
}

static void print_egl_error(const char *call) {
    EGLint error = eglGetError();
    fprintf(stderr, "Error: %s failed (%s / 0x%04x)\n", call, egl_error_name(error), error);
}

static bool extension_supported(const char *extensions, const char *name) {
    const char *start = extensions;
    size_t name_len;

    if (!extensions || !name || !*name) {
        return false;
    }

    name_len = strlen(name);
    while ((start = strstr(start, name)) != NULL) {
        const char before = (start == extensions) ? ' ' : start[-1];
        const char after = start[name_len];
        if ((before == ' ' || before == '\0') && (after == ' ' || after == '\0')) {
            return true;
        }
        start += name_len;
    }

    return false;
}

static EGLDisplay get_gbm_egl_display(struct gbm_device *gbm_device) {
    EGLDisplay display = EGL_NO_DISPLAY;
    const char *client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

    if (extension_supported(client_extensions, "EGL_KHR_platform_gbm") ||
        extension_supported(client_extensions, "EGL_MESA_platform_gbm")) {
#ifdef EGL_VERSION_1_5
        display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbm_device, NULL);
        if (display != EGL_NO_DISPLAY) {
            return display;
        }
#endif

        {
            PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display_ext =
                (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
            if (get_platform_display_ext) {
                display = get_platform_display_ext(EGL_PLATFORM_GBM_KHR,
                                                   (void *)gbm_device,
                                                   NULL);
                if (display != EGL_NO_DISPLAY) {
                    return display;
                }
            }
        }
    }

    return eglGetDisplay((EGLNativeDisplayType)gbm_device);
}

static bool crtc_is_supported(drmModeRes *resources, drmModeEncoder *encoder, uint32_t crtc_id) {
    for (int i = 0; i < resources->count_crtcs; i++) {
        if (resources->crtcs[i] == crtc_id) {
            return (encoder->possible_crtcs & (1u << i)) != 0;
        }
    }

    return false;
}

static bool find_crtc_for_connector(int fd, drmModeRes *resources, drmModeConnector *connector, uint32_t *crtc_id) {
    drmModeEncoder *encoder = NULL;

    if (connector->encoder_id) {
        encoder = drmModeGetEncoder(fd, connector->encoder_id);
        if (encoder) {
            if (encoder->crtc_id && crtc_is_supported(resources, encoder, encoder->crtc_id)) {
                *crtc_id = encoder->crtc_id;
                drmModeFreeEncoder(encoder);
                return true;
            }
            drmModeFreeEncoder(encoder);
        }
    }

    for (int i = 0; i < connector->count_encoders; i++) {
        encoder = drmModeGetEncoder(fd, connector->encoders[i]);
        if (!encoder) {
            continue;
        }

        for (int j = 0; j < resources->count_crtcs; j++) {
            if (encoder->possible_crtcs & (1u << j)) {
                *crtc_id = resources->crtcs[j];
                drmModeFreeEncoder(encoder);
                return true;
            }
        }

        drmModeFreeEncoder(encoder);
    }

    return false;
}

static bool init_drm_output(AppState *state) {
    drmModeRes *resources = drmModeGetResources(state->drm_fd);

    if (!resources) {
        fprintf(stderr, "Error: Cannot get DRM resources (%s)\n", strerror(errno));
        return false;
    }

    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector *connector = drmModeGetConnector(state->drm_fd, resources->connectors[i]);
        if (!connector) {
            continue;
        }

        if (connector->connection == DRM_MODE_CONNECTED &&
            connector->count_modes > 0 &&
            find_crtc_for_connector(state->drm_fd, resources, connector, &state->crtc_id)) {
            state->connector_id = connector->connector_id;
            state->mode = connector->modes[0];
            drmModeFreeConnector(connector);
            drmModeFreeResources(resources);

            state->original_crtc = drmModeGetCrtc(state->drm_fd, state->crtc_id);
            if (!state->original_crtc) {
                fprintf(stderr, "Error: Cannot save original CRTC state (%s)\n", strerror(errno));
                return false;
            }

            return true;
        }

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);
    fprintf(stderr, "Error: No connected DRM connector with a supported CRTC was found.\n");
    return false;
}

static bool present_front_buffer(AppState *state) {
    struct gbm_bo *bo;
    uint32_t handle;
    uint32_t stride;
    uint32_t fb_id = 0;

    bo = gbm_surface_lock_front_buffer(state->gbm_surface);
    if (!bo) {
        fprintf(stderr, "Error: gbm_surface_lock_front_buffer failed; DRM presentation skipped.\n");
        return false;
    }

    handle = gbm_bo_get_handle(bo).u32;
    stride = gbm_bo_get_stride(bo);

    if (drmModeAddFB(state->drm_fd,
                     gbm_bo_get_width(bo),
                     gbm_bo_get_height(bo),
                     24,
                     32,
                     stride,
                     handle,
                     &fb_id) != 0) {
        fprintf(stderr, "Error: drmModeAddFB failed (%s)\n", strerror(errno));
        gbm_surface_release_buffer(state->gbm_surface, bo);
        return false;
    }

    if (drmModeSetCrtc(state->drm_fd,
                       state->crtc_id,
                       fb_id,
                       0,
                       0,
                       &state->connector_id,
                       1,
                       &state->mode) != 0) {
        fprintf(stderr, "Error: drmModeSetCrtc failed (%s)\n", strerror(errno));
        drmModeRmFB(state->drm_fd, fb_id);
        gbm_surface_release_buffer(state->gbm_surface, bo);
        return false;
    }

    if (state->current_fb) {
        drmModeRmFB(state->drm_fd, state->current_fb);
    }
    if (state->current_bo) {
        gbm_surface_release_buffer(state->gbm_surface, state->current_bo);
    }

    state->current_fb = fb_id;
    state->current_bo = bo;
    return true;
}

void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void init_app_state(AppState *state) {
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->drm_fd = -1;
    state->connector_id = 0;
    state->crtc_id = 0;
    state->original_crtc = NULL;
    state->current_bo = NULL;
    state->current_fb = 0;
    state->egl_display = EGL_NO_DISPLAY;
    state->egl_context = EGL_NO_CONTEXT;
    state->egl_surface = EGL_NO_SURFACE;
}

bool init_egl_and_drm(AppState *state, int width, int height, const char* title) {
    EGLint major, minor;
    EGLint total_configs = 0;
    EGLConfig config = NULL;
    EGLConfig *configs = NULL;
    EGLint num_configs = 0;
    bool ok = false;
    (void)title;

    if (!state) {
        fprintf(stderr, "Error: AppState pointer is NULL; initialization skipped safely.\n");
        return false;
    }

    init_app_state(state);

    // 1. Initialize DRM and GBM
    state->drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (state->drm_fd < 0) {
        fprintf(stderr, "Error: Cannot open /dev/dri/card0 (%s)\n", strerror(errno));
        goto out;
    }

    if (!init_drm_output(state)) {
        goto out;
    }

    state->gbm_device = gbm_create_device(state->drm_fd);
    if (!state->gbm_device) {
        fprintf(stderr, "Error: Cannot create GBM device\n");
        goto out;
    }

    if (width != state->mode.hdisplay || height != state->mode.vdisplay) {
        fprintf(stderr,
                "Info: Requested %dx%d, using active DRM mode %dx%d for scanout.\n",
                width,
                height,
                state->mode.hdisplay,
                state->mode.vdisplay);
    }

    state->gbm_surface = gbm_surface_create(state->gbm_device, state->mode.hdisplay, state->mode.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!state->gbm_surface) {
        fprintf(stderr, "Error: Cannot create GBM surface\n");
        goto out;
    }

    // 2. Initialize EGL
    state->egl_display = get_gbm_egl_display(state->gbm_device);
    if (state->egl_display == EGL_NO_DISPLAY) {
        print_egl_error("eglGetPlatformDisplay/eglGetDisplay");
        goto out;
    }

    if (!eglInitialize(state->egl_display, &major, &minor)) {
        print_egl_error("eglInitialize");
        goto out;
    }

    EGLint attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    if (!eglGetConfigs(state->egl_display, NULL, 0, &total_configs)) {
        print_egl_error("eglGetConfigs");
        goto out;
    }
    if (total_configs == 0) {
        fprintf(stderr, "Info: eglGetConfigs succeeded, but this display reported 0 configs.\n");
        goto out;
    }

    if (!eglChooseConfig(state->egl_display, attribs, NULL, 0, &num_configs)) {
        print_egl_error("eglChooseConfig(count)");
        goto out;
    }
    if (num_configs == 0) {
        fprintf(stderr, "Info: eglChooseConfig succeeded, but no config matched the requested GLES2/GBM attributes.\n");
        goto out;
    }

    configs = malloc((size_t)num_configs * sizeof(EGLConfig));
    if (!configs) {
        fprintf(stderr, "Error: Cannot allocate EGL config list\n");
        goto out;
    }

    if (!eglChooseConfig(state->egl_display, attribs, configs, num_configs, &num_configs)) {
        print_egl_error("eglChooseConfig(list)");
        goto out;
    }

    for (int i = 0; i < num_configs; i++) {
        EGLint visual_id;
        if (eglGetConfigAttrib(state->egl_display, configs[i], EGL_NATIVE_VISUAL_ID, &visual_id)) {
            if (visual_id == GBM_FORMAT_XRGB8888) {
                config = configs[i];
                break;
            }
        }
    }

    if (config == NULL) {
        fprintf(stderr, "Error: Could not find EGL config matching GBM_FORMAT_XRGB8888\n");
        goto out;
    }

    state->egl_surface = eglCreateWindowSurface(state->egl_display, config, (EGLNativeWindowType)state->gbm_surface, NULL);
    if (state->egl_surface == EGL_NO_SURFACE) {
        print_egl_error("eglCreateWindowSurface");
        goto out;
    }

    EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    state->egl_context = eglCreateContext(state->egl_display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (state->egl_context == EGL_NO_CONTEXT) {
        print_egl_error("eglCreateContext");
        goto out;
    }

    ok = true;

out:
    free(configs);
    if (!ok) {
        cleanup(state);
    }
    return ok;
}

bool make_current_checked(AppState *state, EGLSurface draw, EGLSurface read, EGLContext context, const char *description) {
    if (!state || state->egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Error: eglMakeCurrent skipped because EGL display is not valid.\n");
        return false;
    }

    if (!eglMakeCurrent(state->egl_display, draw, read, context)) {
        if (description) {
            fprintf(stderr, "Error: %s\n", description);
        }
        print_egl_error("eglMakeCurrent");
        return false;
    }

    return true;
}

bool swap_buffers_checked(AppState *state) {
    if (!state || state->egl_display == EGL_NO_DISPLAY || state->egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Error: eglSwapBuffers skipped because EGL display/surface is not valid.\n");
        return false;
    }

    if (!eglSwapBuffers(state->egl_display, state->egl_surface)) {
        print_egl_error("eglSwapBuffers");
        fprintf(stderr, "Info: Swap failed; front-buffer lock and DRM presentation must be skipped.\n");
        return false;
    }

    return present_front_buffer(state);
}

void cleanup(AppState *state) {
    if (!state) {
        return;
    }

    if (state->egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(state->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (state->egl_context != EGL_NO_CONTEXT) eglDestroyContext(state->egl_display, state->egl_context);
        if (state->egl_surface != EGL_NO_SURFACE) eglDestroySurface(state->egl_display, state->egl_surface);
        eglTerminate(state->egl_display);
        state->egl_display = EGL_NO_DISPLAY;
        state->egl_context = EGL_NO_CONTEXT;
        state->egl_surface = EGL_NO_SURFACE;
    }

    if (state->original_crtc && state->drm_fd >= 0) {
        int restore_result;
        if (state->original_crtc->mode_valid) {
            restore_result = drmModeSetCrtc(state->drm_fd,
                                            state->original_crtc->crtc_id,
                                            state->original_crtc->buffer_id,
                                            state->original_crtc->x,
                                            state->original_crtc->y,
                                            &state->connector_id,
                                            1,
                                            &state->original_crtc->mode);
        } else {
            restore_result = drmModeSetCrtc(state->drm_fd,
                                            state->original_crtc->crtc_id,
                                            0,
                                            0,
                                            0,
                                            NULL,
                                            0,
                                            NULL);
        }

        if (restore_result != 0) {
            fprintf(stderr, "Warning: Could not restore original CRTC state (%s)\n", strerror(errno));
        }
    }

    if (state->current_fb && state->drm_fd >= 0) {
        drmModeRmFB(state->drm_fd, state->current_fb);
        state->current_fb = 0;
    }

    if (state->current_bo && state->gbm_surface) {
        gbm_surface_release_buffer(state->gbm_surface, state->current_bo);
        state->current_bo = NULL;
    }

    if (state->original_crtc) {
        drmModeFreeCrtc(state->original_crtc);
        state->original_crtc = NULL;
    }

    if (state->gbm_surface) {
        gbm_surface_destroy(state->gbm_surface);
        state->gbm_surface = NULL;
    }
    if (state->gbm_device) {
        gbm_device_destroy(state->gbm_device);
        state->gbm_device = NULL;
    }
    if (state->drm_fd >= 0) {
        close(state->drm_fd);
        state->drm_fd = -1;
    }
}

// Global variables for shader and vbo
GLuint program;
GLuint position_loc;
GLuint color_loc;
GLuint vbo;

void setup_triangle_drawing(void) {
    const char *v_shader_src =
        "attribute vec4 a_position;   \n"
        "attribute vec4 a_color;      \n"
        "varying vec4 v_color;        \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = a_position; \n"
        "   v_color = a_color;        \n"
        "}                            \n";

    const char *f_shader_src =
        "precision mediump float;     \n"
        "varying vec4 v_color;        \n"
        "void main()                  \n"
        "{                            \n"
        "  gl_FragColor = v_color;    \n"
        "}                            \n";

    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &v_shader_src, NULL);
    glCompileShader(v_shader);

    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, &f_shader_src, NULL);
    glCompileShader(f_shader);

    program = glCreateProgram();
    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);
    glLinkProgram(program);

    position_loc = glGetAttribLocation(program, "a_position");
    color_loc = glGetAttribLocation(program, "a_color");

    // Interleaved vertex data: x, y, z, r, g, b, a
    GLfloat vertices[] = {
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void draw_triangle(void) {
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (const void*)0);

    glEnableVertexAttribArray(color_loc);
    glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (const void*)(3 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(position_loc);
    glDisableVertexAttribArray(color_loc);
}
