#include "common_utils.h"

Window create_x11_window(Display* x_dpy, int width, int height, const char* title) {
    int screen = DefaultScreen(x_dpy);
    Window root = RootWindow(x_dpy, screen);

    Window win = XCreateSimpleWindow(x_dpy, root, 0, 0, width, height, 0,
                                     BlackPixel(x_dpy, screen), WhitePixel(x_dpy, screen));

    XStoreName(x_dpy, win, title);
    XMapWindow(x_dpy, win);
    XFlush(x_dpy);

    return win;
}

static GLuint shader_program = 0;
static GLint pos_loc = -1;
static GLint color_loc = -1;

void init_simple_shader() {
    if (shader_program != 0) return;

    const char* vShaderStr =
        "attribute vec3 vPosition;\n"
        "uniform vec4 uColor;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "   gl_Position = vec4(vPosition, 1.0);\n"
        "   vColor = uColor;\n"
        "}\n";

    const char* fShaderStr =
        "precision mediump float;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "   gl_FragColor = vColor;\n"
        "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderStr, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderStr, NULL);
    glCompileShader(fragmentShader);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertexShader);
    glAttachShader(shader_program, fragmentShader);
    glLinkProgram(shader_program);

    pos_loc = glGetAttribLocation(shader_program, "vPosition");
    color_loc = glGetUniformLocation(shader_program, "uColor");
}

void draw_triangle(float z, float r, float g, float b) {
    if (shader_program == 0) init_simple_shader();

    glUseProgram(shader_program);

    GLfloat vertices[] = {
         0.0f,  0.5f, z,
        -0.5f, -0.5f, z,
         0.5f, -0.5f, z
    };

    glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(pos_loc);

    glUniform4f(color_loc, r, g, b, 1.0f);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

GLuint create_checkerboard_texture() {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // 2x2 Dama tahtası deseni (Sarı ve Siyah)
    GLubyte pixels[] = {
        255, 255,   0,   0,   0,   0,
          0,   0,   0, 255, 255,   0
    };

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture_id;
}
