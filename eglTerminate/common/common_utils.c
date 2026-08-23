#include "common_utils.h"
#include <stdio.h>
#include <stdlib.h>

bool create_x11_window(NativeWindow* nw, int width, int height, const char* title) {
    nw->x_display = XOpenDisplay(NULL);
    if (!nw->x_display) {
        fprintf(stderr, "XOpenDisplay basarisiz. (DISPLAY ortami ayarli olmayabilir)\n");
        return false;
    }

    int screen = DefaultScreen(nw->x_display);
    Window root = RootWindow(nw->x_display, screen);

    nw->x_window = XCreateSimpleWindow(nw->x_display, root, 0, 0, width, height, 1,
                                        BlackPixel(nw->x_display, screen),
                                        WhitePixel(nw->x_display, screen));

    XStoreName(nw->x_display, nw->x_window, title);
    XMapWindow(nw->x_display, nw->x_window);
    XFlush(nw->x_display);

    return true;
}

void destroy_x11_window(NativeWindow* nw) {
    if (nw->x_display) {
        XDestroyWindow(nw->x_display, nw->x_window);
        XCloseDisplay(nw->x_display);
        nw->x_display = NULL;
    }
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
