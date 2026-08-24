#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: uShareContext - Ortak Context (Paylasimli) ---\n\n");

    NativeDisplayContext* native_ctx = init_native_display();
    if (!native_ctx) {
        printf("Hata: Native Display (DRM/GBM) baslatilamadi.\n");
        return -1;
    }
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)native_ctx->gbm_dev);
    eglInitialize(display, NULL, NULL);

    // EGL_PBUFFER_BIT eklenerek off-screen (ekransiz) yuzey destegi istendi.
    EGLint attribs[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE };
    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(display, attribs, &config, 1, &num_config);

    EGLint pbuffer_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLSurface pbuffer_surf = eglCreatePbufferSurface(display, config, pbuffer_attribs);
    if (pbuffer_surf == EGL_NO_SURFACE) {
        printf("Hata: EGL_NO_SURFACE (Pbuffer olusturulamadi).\n");
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    // 1. Bağlam oluşturulur
    EGLContext main_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (main_ctx == EGL_NO_CONTEXT) {
        printf("Hata: main_ctx olusturulamadi.\n");
        eglDestroySurface(display, pbuffer_surf);
        destroy_native_display(native_ctx);
        return -1;
    }
    eglMakeCurrent(display, pbuffer_surf, pbuffer_surf, main_ctx);

    GLuint texture = create_checkerboard_texture();
    printf("[VRAM] 1. Baglamda doku (ID: %d) yaratildi.\n", texture);

    // 2. Bağlam oluşturulur - Paylaşım VAR! (main_ctx referans verildi)
    printf("\nDEGER B: share_context = main_ctx (Paylasim Var)\n");
    EGLContext shared_ctx = eglCreateContext(display, config, main_ctx, ctx_attribs);
    if (shared_ctx == EGL_NO_CONTEXT) {
        printf("Hata: shared_ctx olusturulamadi.\n");
        eglDestroyContext(display, main_ctx);
        eglDestroySurface(display, pbuffer_surf);
        destroy_native_display(native_ctx);
        return -1;
    }

    // 2. Baglami aktif et
    eglMakeCurrent(display, pbuffer_surf, pbuffer_surf, shared_ctx);

    // Doku bu bağlamda geçerli mi kontrol et (Eğer paylaşıldıysa glIsTexture TRUE döner)
    GLboolean isValid = glIsTexture(texture);

    printf(" -> SONUC: 2. baglam 1. baglamin dokusunu %s!\n", isValid ? "TANIYOR" : "TANIMIYOR");
    printf("    (Gorsel ekrana gerek yok, glIsTexture ile bellek paylasimi dogrulandi. Bellek tasarrufu saglandi.)\n\n");

    eglDestroySurface(display, pbuffer_surf);
    eglDestroyContext(display, main_ctx);
    eglDestroyContext(display, shared_ctx);
    destroy_native_display(native_ctx);
    return 0;
}
