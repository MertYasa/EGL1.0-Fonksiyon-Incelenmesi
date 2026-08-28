#include "common_utils.h"
#include <errno.h>
#include <string.h>
#include <EGL/eglext.h>

#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

static uint32_t find_crtc_for_connector(int fd, const drmModeRes *resources, const drmModeConnector *connector) {
    if (connector->encoder_id) {
        drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoder_id);
        if (encoder) {
            uint32_t crtc_id = encoder->crtc_id;
            drmModeFreeEncoder(encoder);
            if (crtc_id) {
                return crtc_id;
            }
        }
    }

    for (int i = 0; i < connector->count_encoders; i++) {
        drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoders[i]);
        if (!encoder) {
            continue;
        }

        for (int j = 0; j < resources->count_crtcs; j++) {
            if (encoder->possible_crtcs & (1u << j)) {
                uint32_t crtc_id = resources->crtcs[j];
                drmModeFreeEncoder(encoder);
                return crtc_id;
            }
        }

        drmModeFreeEncoder(encoder);
    }

    return resources->count_crtcs > 0 ? resources->crtcs[0] : 0;
}

NativeDisplayContext* init_native_display(void) {
    return init_native_display_at(0);
}

NativeDisplayContext* init_native_display_at(int connected_index) {
    if (connected_index < 0) {
        connected_index = 0;
    }

    NativeDisplayContext* ctx = (NativeDisplayContext*)malloc(sizeof(NativeDisplayContext));
    if (!ctx) return NULL;

    ctx->fd = -1;
    ctx->gbm_dev = NULL;
    ctx->crtc_id = 0;
    ctx->connector_id = 0;
    ctx->saved_crtc = NULL;

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
    int connected_seen = 0;
    for (int i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(ctx->fd, resources->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0) {
            if (connected_seen == connected_index) {
                break;
            }
            connected_seen++;
        }
        if (connector) {
            drmModeFreeConnector(connector);
        }
        connector = NULL;
    }

    if (!connector && connected_index > 0) {
        printf("Uyari: %d numarali bagli ekran bulunamadi, 0 numarali ekran kullanilacak.\n", connected_index);
        for (int i = 0; i < resources->count_connectors; i++) {
            connector = drmModeGetConnector(ctx->fd, resources->connectors[i]);
            if (connector && connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0) {
                break;
            }
            if (connector) {
                drmModeFreeConnector(connector);
            }
            connector = NULL;
        }
    }

    if (!connector) {
        printf("Hata: Bagli ekran connector bulunamadi.\n");
        drmModeFreeResources(resources);
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    ctx->connector_id = connector->connector_id;
    ctx->mode_info = connector->modes[0];
    ctx->crtc_id = find_crtc_for_connector(ctx->fd, resources, connector);

    if (ctx->crtc_id == 0) {
        printf("Hata: CRTC bulunamadi.\n");
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    ctx->saved_crtc = drmModeGetCrtc(ctx->fd, ctx->crtc_id);

    printf("Native ekran: connector=%u, crtc=%u, mode=%dx%d\n",
           ctx->connector_id,
           ctx->crtc_id,
           ctx->mode_info.hdisplay,
           ctx->mode_info.vdisplay);

    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);

    ctx->gbm_dev = gbm_create_device(ctx->fd);
    if (!ctx->gbm_dev) {
        printf("Hata: GBM cihazi olusturulamadi.\n");
        if (ctx->saved_crtc) {
            drmModeFreeCrtc(ctx->saved_crtc);
        }
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

    struct gbm_surface *surf = gbm_surface_create(gbm_dev,
                                                  width,
                                                  height,
                                                  GBM_FORMAT_XRGB8888,
                                                  GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!surf) {
        printf("Hata: gbm_surface_create basarisiz oldu.\n");
    }
    return surf;
}

EGLDisplay get_egl_display_for_gbm(struct gbm_device *gbm_dev) {
    if (!gbm_dev) {
        return EGL_NO_DISPLAY;
    }

    const char *client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!client_extensions) {
        eglGetError();
    }

    if (client_extensions &&
        (strstr(client_extensions, "EGL_KHR_platform_gbm") ||
         strstr(client_extensions, "EGL_MESA_platform_gbm"))) {
        PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplay");
        if (!get_platform_display) {
            get_platform_display =
                (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
        }
        if (get_platform_display) {
            EGLDisplay display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm_dev, NULL);
            if (display != EGL_NO_DISPLAY) {
                return display;
            }
            eglGetError();
        }
    }

    return eglGetDisplay((EGLNativeDisplayType)gbm_dev);
}

struct gbm_surface* create_egl_compatible_gbm_surface(EGLDisplay display, EGLConfig config, struct gbm_device *gbm_dev, int width, int height) {
    if (!gbm_dev) {
        printf("Hata: GBM cihazi NULL, yuzey olusturulamadi.\n");
        return NULL;
    }

    EGLint native_visual_id = 0;
    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &native_visual_id) || native_visual_id == 0) {
        native_visual_id = GBM_FORMAT_XRGB8888;
    }

    printf("GBM yuzey formati: EGL_NATIVE_VISUAL_ID=0x%x\n", native_visual_id);

    struct gbm_surface *surf = gbm_surface_create(gbm_dev,
                                                  width,
                                                  height,
                                                  (uint32_t)native_visual_id,
                                                  GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!surf && native_visual_id != GBM_FORMAT_XRGB8888) {
        printf("Uyari: EGL native visual ile GBM yuzeyi acilamadi, XRGB8888 deneniyor.\n");
        surf = gbm_surface_create(gbm_dev,
                                  width,
                                  height,
                                  GBM_FORMAT_XRGB8888,
                                  GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    }

    if (!surf) {
        printf("Hata: EGL config ile uyumlu GBM yuzeyi olusturulamadi.\n");
    }

    return surf;
}

int present_native_display(NativeDisplayContext* ctx, struct gbm_surface* surface) {
    return present_native_display_for(ctx, surface, 0);
}

int present_native_display_for(NativeDisplayContext* ctx, struct gbm_surface* surface, unsigned int seconds) {
    if (!ctx || !surface) {
        printf("Hata: Gecersiz context veya surface (present_native_display).\n");
        return 0;
    }

    struct gbm_bo *bo = gbm_surface_lock_front_buffer(surface);
    if (!bo) {
        printf("Hata: gbm_surface_lock_front_buffer basarisiz (EGL cizim yapamamis olabilir).\n");
        return 0;
    }

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t format = gbm_bo_get_format(bo);

    uint32_t fb_id = 0;
    uint32_t handles[4] = { handle, 0, 0, 0 };
    uint32_t strides[4] = { stride, 0, 0, 0 };
    uint32_t offsets[4] = { 0, 0, 0, 0 };
    int ret = drmModeAddFB2(ctx->fd, width, height, format, handles, strides, offsets, &fb_id, 0);

    if (ret && format == GBM_FORMAT_XRGB8888) {
        ret = drmModeAddFB(ctx->fd, width, height, 24, 32, stride, handle, &fb_id);
    }

    if (ret) {
        printf("Hata: FB olusturulamadi. format=0x%x, errno=%d (%s)\n",
               format,
               errno,
               strerror(errno));
        gbm_surface_release_buffer(surface, bo);
        return 0;
    }

    ret = drmModeSetCrtc(ctx->fd, ctx->crtc_id, fb_id, 0, 0, &ctx->connector_id, 1, &ctx->mode_info);
    if (ret) {
        printf("Hata: drmModeSetCrtc basarisiz oldu.\n");
        drmModeRmFB(ctx->fd, fb_id);
        gbm_surface_release_buffer(surface, bo);
        return 0;
    }

    if (seconds > 0) {
        sleep(seconds);
    }

    if (ctx->saved_crtc) {
        uint32_t connector_id = ctx->connector_id;
        uint32_t *connectors = ctx->saved_crtc->buffer_id ? &connector_id : NULL;
        int connector_count = ctx->saved_crtc->buffer_id ? 1 : 0;
        drmModeModeInfo *mode = ctx->saved_crtc->mode_valid ? &ctx->saved_crtc->mode : NULL;

        drmModeSetCrtc(ctx->fd,
                       ctx->saved_crtc->crtc_id,
                       ctx->saved_crtc->buffer_id,
                       ctx->saved_crtc->x,
                       ctx->saved_crtc->y,
                       connectors,
                       connector_count,
                       mode);
    }

    drmModeRmFB(ctx->fd, fb_id);
    gbm_surface_release_buffer(surface, bo);
    return 1;
}

int swap_and_present_native_display(EGLDisplay display, EGLSurface egl_surface, NativeDisplayContext* ctx, struct gbm_surface* gbm_surface, unsigned int seconds) {
    if (!eglSwapBuffers(display, egl_surface)) {
        log_egl_error("eglSwapBuffers");
        printf("Hata: Swap basarisiz oldugu icin GBM front buffer ekrana basilmadi.\n");
        return 0;
    }

    return present_native_display_for(ctx, gbm_surface, seconds);
}

void destroy_native_display(NativeDisplayContext* ctx) {
    if (ctx) {
        if (ctx->saved_crtc) drmModeFreeCrtc(ctx->saved_crtc);
        if (ctx->gbm_dev) gbm_device_destroy(ctx->gbm_dev);
        if (ctx->fd >= 0) close(ctx->fd);
        free(ctx);
    }
}

const char* egl_error_name(EGLint error) {
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
        default: return "BILINMEYEN_EGL_HATASI";
    }
}

void log_egl_error(const char* operation) {
    EGLint err = eglGetError();
    printf("Hata: %s basarisiz oldu. EGL hata kodu: %s (0x%04x)\n",
           operation,
           egl_error_name(err),
           err);
}

static GLuint shader_program = 0;
static GLint pos_loc = -1;
static GLint color_loc = -1;
static GLuint texture_program = 0;
static GLint tex_pos_loc = -1;
static GLint tex_uv_loc = -1;
static GLint tex_sampler_loc = -1;

void reset_common_gl_objects(void) {
    shader_program = 0;
    pos_loc = -1;
    color_loc = -1;
    texture_program = 0;
    tex_pos_loc = -1;
    tex_uv_loc = -1;
    tex_sampler_loc = -1;
}

void init_simple_shader(void) {
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

void draw_quad(float x0, float y0, float x1, float y1, float z, float r, float g, float b) {
    if (shader_program == 0) init_simple_shader();

    glUseProgram(shader_program);

    GLfloat vertices[] = {
        x0, y0, z,
        x1, y0, z,
        x0, y1, z,
        x1, y0, z,
        x1, y1, z,
        x0, y1, z
    };

    glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(pos_loc);
    glUniform4f(color_loc, r, g, b, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

GLuint create_checkerboard_texture(void) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

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

static void init_texture_shader(void) {
    if (texture_program != 0) return;

    const char* vShaderStr =
        "attribute vec3 vPosition;\n"
        "attribute vec2 vTexCoord;\n"
        "varying vec2 texCoord;\n"
        "void main() {\n"
        "   gl_Position = vec4(vPosition, 1.0);\n"
        "   texCoord = vTexCoord;\n"
        "}\n";

    const char* fShaderStr =
        "precision mediump float;\n"
        "varying vec2 texCoord;\n"
        "uniform sampler2D uTexture;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(uTexture, texCoord);\n"
        "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderStr, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderStr, NULL);
    glCompileShader(fragmentShader);

    texture_program = glCreateProgram();
    glAttachShader(texture_program, vertexShader);
    glAttachShader(texture_program, fragmentShader);
    glLinkProgram(texture_program);

    tex_pos_loc = glGetAttribLocation(texture_program, "vPosition");
    tex_uv_loc = glGetAttribLocation(texture_program, "vTexCoord");
    tex_sampler_loc = glGetUniformLocation(texture_program, "uTexture");
}

void draw_textured_quad(GLuint texture_id) {
    init_texture_shader();

    glUseProgram(texture_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glUniform1i(tex_sampler_loc, 0);

    GLfloat vertices[] = {
        -0.7f, -0.7f, 0.0f,  0.0f, 0.0f,
         0.7f, -0.7f, 0.0f,  1.0f, 0.0f,
        -0.7f,  0.7f, 0.0f,  0.0f, 1.0f,
         0.7f, -0.7f, 0.0f,  1.0f, 0.0f,
         0.7f,  0.7f, 0.0f,  1.0f, 1.0f,
        -0.7f,  0.7f, 0.0f,  0.0f, 1.0f
    };

    glVertexAttribPointer(tex_pos_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vertices);
    glEnableVertexAttribArray(tex_pos_loc);
    glVertexAttribPointer(tex_uv_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vertices + 3);
    glEnableVertexAttribArray(tex_uv_loc);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
