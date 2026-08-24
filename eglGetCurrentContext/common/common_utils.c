#include "common_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

bool init_egl_and_drm(AppState *state, int width, int height, const char* title) {
    // 1. Initialize DRM and GBM
    state->drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (state->drm_fd < 0) {
        fprintf(stderr, "Error: Cannot open /dev/dri/card0\n");
        return false;
    }

    state->gbm_device = gbm_create_device(state->drm_fd);
    if (!state->gbm_device) {
        fprintf(stderr, "Error: Cannot create GBM device\n");
        close(state->drm_fd);
        return false;
    }

    state->gbm_surface = gbm_surface_create(state->gbm_device, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!state->gbm_surface) {
        fprintf(stderr, "Error: Cannot create GBM surface\n");
        gbm_device_destroy(state->gbm_device);
        close(state->drm_fd);
        return false;
    }

    // 2. Initialize EGL
    state->egl_display = eglGetDisplay((EGLNativeDisplayType)state->gbm_device);
    if (state->egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Error: eglGetDisplay failed\n");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(state->egl_display, &major, &minor)) {
        fprintf(stderr, "Error: eglInitialize failed\n");
        return false;
    }

    EGLint attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig config = NULL;
    EGLint num_configs;
    eglChooseConfig(state->egl_display, attribs, NULL, 0, &num_configs);
    if (num_configs == 0) {
        fprintf(stderr, "Error: No matching EGL configs found\n");
        return false;
    }

    EGLConfig *configs = malloc(num_configs * sizeof(EGLConfig));
    eglChooseConfig(state->egl_display, attribs, configs, num_configs, &num_configs);

    for (int i = 0; i < num_configs; i++) {
        EGLint visual_id;
        if (eglGetConfigAttrib(state->egl_display, configs[i], EGL_NATIVE_VISUAL_ID, &visual_id)) {
            if (visual_id == GBM_FORMAT_XRGB8888) {
                config = configs[i];
                break;
            }
        }
    }
    free(configs);

    if (config == NULL) {
        fprintf(stderr, "Error: Could not find EGL config matching GBM_FORMAT_XRGB8888\n");
        return false;
    }

    state->egl_surface = eglCreateWindowSurface(state->egl_display, config, (EGLNativeWindowType)state->gbm_surface, NULL);
    if (state->egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Error: eglCreateWindowSurface failed\n");
        return false;
    }

    EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    state->egl_context = eglCreateContext(state->egl_display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (state->egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Error: eglCreateContext failed\n");
        return false;
    }

    return true;
}

void cleanup(AppState *state) {
    if (state->egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(state->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (state->egl_context != EGL_NO_CONTEXT) eglDestroyContext(state->egl_display, state->egl_context);
        if (state->egl_surface != EGL_NO_SURFACE) eglDestroySurface(state->egl_display, state->egl_surface);
        eglTerminate(state->egl_display);
    }

    if (state->gbm_surface) {
        gbm_surface_destroy(state->gbm_surface);
    }
    if (state->gbm_device) {
        gbm_device_destroy(state->gbm_device);
    }
    if (state->drm_fd >= 0) {
        close(state->drm_fd);
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
