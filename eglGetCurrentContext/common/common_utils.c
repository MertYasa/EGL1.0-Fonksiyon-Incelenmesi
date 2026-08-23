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

bool init_egl_and_x11(AppState *state, int width, int height, const char* title) {
    // 1. Initialize X11
    state->x_display = XOpenDisplay(NULL);
    if (state->x_display == NULL) {
        fprintf(stderr, "Error: Cannot connect to X server\n");
        return false;
    }

    Window root = DefaultRootWindow(state->x_display);
    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;

    state->x_window = XCreateWindow(
        state->x_display, root,
        0, 0, width, height, 0,
        CopyFromParent, InputOutput,
        CopyFromParent, CWEventMask,
        &swa);

    XStoreName(state->x_display, state->x_window, title);
    XMapWindow(state->x_display, state->x_window);
    XFlush(state->x_display);

    // 2. Initialize EGL
    state->egl_display = eglGetDisplay((EGLNativeDisplayType)state->x_display);
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

    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(state->egl_display, attribs, &config, 1, &num_configs) || num_configs == 0) {
        fprintf(stderr, "Error: eglChooseConfig failed\n");
        return false;
    }

    state->egl_surface = eglCreateWindowSurface(state->egl_display, config, (EGLNativeWindowType)state->x_window, NULL);
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
    if (state->x_display != NULL) {
        if (state->x_window != 0) XDestroyWindow(state->x_display, state->x_window);
        XCloseDisplay(state->x_display);
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
