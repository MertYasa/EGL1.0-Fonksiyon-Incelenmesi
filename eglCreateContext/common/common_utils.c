#include "common_utils.h"

NativeDisplayContext* init_native_display() {
    NativeDisplayContext* ctx = (NativeDisplayContext*)malloc(sizeof(NativeDisplayContext));
    if (!ctx) return NULL;

    ctx->fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (ctx->fd < 0) {
        printf("Hata: /dev/dri/card0 acilamadi.\n");
        free(ctx);
        return NULL;
    }

    drmModeRes *resources = drmModeGetResources(ctx->fd);
    if (!resources) {
        printf("Hata: DRM kaynaklari alinamadi.\n");
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    drmModeConnector *connector = NULL;
    for (int i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(ctx->fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector) {
        printf("Hata: Bagli ekran (connector) bulunamadi.\n");
        drmModeFreeResources(resources);
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    ctx->connector_id = connector->connector_id;
    ctx->mode_info = connector->modes[0];
    ctx->crtc_id = resources->crtcs[0];

    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);

    ctx->gbm_dev = gbm_create_device(ctx->fd);
    if (!ctx->gbm_dev) {
        printf("Hata: GBM cihazi olusturulamadi.\n");
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    return ctx;
}

struct gbm_surface* create_gbm_surface(struct gbm_device *gbm_dev, int width, int height) {
    if (!gbm_dev) {
        printf("Hata: GBM cihazi NULL, yuzey olusturulamadi.\n");
        return NULL;
    }
    struct gbm_surface *surf = gbm_surface_create(gbm_dev, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!surf) {
        printf("Hata: gbm_surface_create basarisiz oldu.\n");
    }
    return surf;
}

void present_native_display(NativeDisplayContext* ctx, struct gbm_surface* surface) {
    if (!ctx || !surface) {
        printf("Hata: Gecersiz context veya surface (present_native_display).\n");
        return;
    }

    struct gbm_bo *bo = gbm_surface_lock_front_buffer(surface);
    if (!bo) {
        printf("Hata: gbm_surface_lock_front_buffer basarisiz (EGL cizim yapamamis olabilir).\n");
        return;
    }

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    uint32_t fb_id;
    int ret = drmModeAddFB(ctx->fd, width, height, 24, 32, stride, handle, &fb_id);
    if (ret) {
        printf("Hata: FB olusturulamadi.\n");
        gbm_surface_release_buffer(surface, bo);
        return;
    }

    drmModeSetCrtc(ctx->fd, ctx->crtc_id, fb_id, 0, 0, &ctx->connector_id, 1, &ctx->mode_info);
    gbm_surface_release_buffer(surface, bo);
}

void destroy_native_display(NativeDisplayContext* ctx) {
    if (ctx) {
        if (ctx->gbm_dev) gbm_device_destroy(ctx->gbm_dev);
        if (ctx->fd >= 0) close(ctx->fd);
        free(ctx);
    }
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
