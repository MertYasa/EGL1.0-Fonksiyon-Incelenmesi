# EGL 1.0: eglGetCurrentContext

```c
EGLContext eglGetCurrentContext(void);
```

`eglGetCurrentContext`, çağrıyı yapan thread (iş parçacığı) için o anda aktif olan (state machine'e bağlanmış) `EGLContext` nesnesinin handle (tanıtıcı) değerini döndürür. Bu fonksiyon, herhangi bir argüman almadan doğrudan thread-local bellek üzerinden durumu okuduğu için sistem durumunu sorgulamada en düşük maliyetli EGL fonksiyonlarından biridir.

## Mental Model

```text
İşletim Sistemi (OS) - Thread Local Storage (TLS)
 └── İş Parçacığı (Thread ID: 0x1234)
      ├── Aktif EGL State Makinesi
      │    └── EGLContext (Current Context - eglGetCurrentContext() ile sorgulanır)
      │         ├── EGLDisplay (Örn: DRM/GBM Display veya X11 Display)
      │         ├── EGLSurface (DRAW - Çizim Hedefi)
      │         │    └── NativeWindowType (Örn: gbm_surface veya Window)
      │         └── EGLSurface (READ - Okuma Hedefi)
      │              └── NativePixmapType / NativeWindowType
      └── [Alternatif] EGL_NO_CONTEXT (Hiçbir context bağlanmamış durumu)
```

## Parametreler

Bu fonksiyon spesifikasyona göre hiçbir parametre almaz (`void`).

### void

| Değer | Sonuç |
|---|---|
| `void` (Parametre yok) | Thread'e bağlı geçerli bir `EGLContext` varsa o döndürülür, aksi takdirde `EGL_NO_CONTEXT` döner. |

*Not: Fonksiyon parametre almadığı için NativeWindowType veya NativePixmapType gibi platforma bağımlı tipleri doğrudan etkilemez veya kabul etmez. Ancak dönen context'in dolaylı olarak bağlı olduğu yüzeyler (surface) X11'de Window, DRM/GBM sistemlerinde ise `gbm_surface` gibi yapılara tekabül eder.*

## Geçerli Attribute Listesi ve Tampon (Buffer) Tipleri

`eglGetCurrentContext` fonksiyonu konfigürasyon veya flag dizisi almaz. Ancak dönen context'in durumunu değerlendirirken tampon yapıları büyük önem taşır:

| Attribute / Durum | Tip | Anlam |
|---|---|---|
| Konfigürasyon Listesi | Yok | Fonksiyon attribute dizisi kabul etmez. |
| Back Buffered (Çift Tamponlu) | Surface Durumu | Dönen context bir Window yüzeyine bağlıysa, çizimler arka tampona (back buffer) yapılır ve `eglSwapBuffers` beklenir. |
| Single Buffered (Tek Tamponlu) | Surface Durumu | Dönen context bir Pixmap veya Pbuffer (off-screen) yüzeyine bağlıysa, çizimler anında gerçekleşebilir ve frame eşzamanlaması farklı yönetilir. |

## Ayrıntılar ve Yaşam Döngüsü

**Thread Güvenliği (Thread Safety):**
EGL standartlarına göre thread güvenliği son derece katıdır. Bir `EGLContext` aynı anda sadece **bir** thread üzerinde current (aktif) olabilir. Eğer aynı context başka bir thread'de zaten aktifse ve o thread'de `eglMakeCurrent` çağrılmazsa, farklı bir thread üzerinden bu context'e erişilemez. `eglGetCurrentContext` fonksiyonu thread-safe'tir ve yalnızca çağıran thread'in belleğine bakar, diğer thread'leri bloklamaz (non-blocking).

**Eşzamanlama (Synchronization):**
Dönen context üzerinden çizim komutları gönderilirken, OpenGL ile Native API (Örn: DRM veya X11 2D render komutları) arasında eşzamanlama gerekiyorsa `eglWaitGL` (OpenGL komutlarının bitmesini bekler) ve `eglWaitNative` (Native render komutlarının bitmesini bekler) kullanılmalıdır. Native objeler (Window/Pixmap) üzerinde OpenGL harici bir API işlem yapacaksa, context üzerinde `glFinish()` çağırmak da alternatif bir donanım bazlı eşzamanlama garantisidir.

## Hata Matrisi

Khronos spesifikasyonlarına göre `eglGetCurrentContext`, state machine üzerinde hiçbir değişiklik yapmayan **yan etkisiz (side-effect free)** bir okuma (getter) operasyonudur. Başarısızlık durumunda dahi EGL error flag'lerini modifiye etmez.

| Durum | Sonuç (EGL Hata Kodu) |
|---|---|
| EGL hiç initialize edilmediyse | `EGL_NO_CONTEXT` döner, `eglGetError()` değişmez. |
| Başka thread'de context aktif, mevcut thread'de boşsa | `EGL_NO_CONTEXT` döner, `eglGetError()` değişmez. |
| `eglMakeCurrent` başarıyla çalıştıysa | Aktif `EGLContext` handle'ı döner, hata kodu değişmez. |

*Önemli Not: Bu fonksiyon hiçbir zaman `EGL_BAD_MATCH`, `EGL_BAD_NATIVE_WINDOW` gibi hatalar fırlatmaz. Bir şeylerin ters gittiğini sadece dönen değerin `EGL_NO_CONTEXT` olmasından anlayabilirsiniz.*

## Güvenli Kullanım Örneği

Aşağıdaki örnekte, fonksiyonun DRM/GBM veya gömülü sistemler fark etmeksizin nasıl güvenli kullanılacağı gösterilmiştir:

```c
#include <EGL/egl.h>
#include <stdio.h>

// Context'in dogru sekilde current (aktif) olup olmadigini test eden guvenli fonksiyon
void perform_safe_rendering(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    // 1. Context'i aktif yapmayi dene
    if (!eglMakeCurrent(dpy, draw, read, ctx)) {
        EGLint err = eglGetError();
        fprintf(stderr, "Kritik Hata: eglMakeCurrent basarisiz oldu. Hata Kodu: 0x%04X\n", err);
        return;
    }

    // 2. State machine'in gercekten context'i kabul ettigini dogrula (Cifte Kontrol)
    EGLContext current_ctx = eglGetCurrentContext();
    
    if (current_ctx == EGL_NO_CONTEXT) {
        // eglMakeCurrent basarili donse bile spesifikasyon disi donanim surucusu hatasi
        fprintf(stderr, "Fatal: eglMakeCurrent basarili oldu fakat aktif context yok!\n");
        // Guvenli cikis (Memory leak ve undefined behavior onleme)
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return;
    }

    if (current_ctx != ctx) {
        fprintf(stderr, "Fatal: Aktif context (%p), beklenen context (%p) ile eslesmiyor!\n", 
                (void*)current_ctx, (void*)ctx);
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return;
    }

    // 3. Guvenli bolge: Context kesin olarak bu thread'e bagli
    printf("Context basariyla dogrulandi (%p). Cizim islemleri basliyor...\n", (void*)current_ctx);
    
    // glClear(GL_COLOR_BUFFER_BIT) vs...
    
    // 4. Eşzamanlama (Opsiyonel ama güvenlik kritik sistemlerde önerilir)
    // eglWaitGL();
    
    // 5. Cizim bittikten sonra context'i guvenli sekilde ayir (Cleanup)
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}
```

## Pratik Özet

- **Maliyet ve Yan Etki:** Çok ucuz (düşük maliyetli) ve tamamen yan etkisiz (side-effect free) bir çağrıdır. `eglGetError()` state'ini asla kirletmez.
- **Thread Local Davranış:** Sonuç tamamen thread-spesifiktir. Her thread sadece kendi aktif context'ini görebilir.
- **Doğrulama (Validation):** `eglMakeCurrent` sonrası sistemin (özellikle gömülü GPU sürücülerinin) gerçekten state değişikliğini yansıtıp yansıtmadığını çift kontrol (double-check) etmek için kritik bir diagnostik aracıdır.
- **Platform Bağımsızlığı:** Dönen değer `EGLContext` olup X11, Wayland, QNX veya DRM/GBM sistemlerindeki pencerelendirme mimarilerinden bağımsız, sadece EGL ve OpenGL ES'in dahili durumunu temsil eder.
