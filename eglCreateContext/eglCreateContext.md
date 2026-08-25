# EGL 1.0: eglCreateContext

```c
EGLContext eglCreateContext(EGLDisplay dpy,
                            EGLConfig config,
                            EGLContext share_context,
                            const EGLint *attrib_list);
```

`eglCreateContext`, OpenGL ES veya diğer Khronos API'lerinin komutlarını çalıştıracağı durumu (state machine) ve bellek alanını (rendering context) temsil eden soyut bir bağlam oluşturur.

## Mental Model

```text
       [İşletim Sistemi / Native Pencerelendirme (X11, DRM, vs.)]
                              |
                        Native Display
                              |
[EGL Katmanı]                 v
                      +---------------+
                      |  EGLDisplay   |
                      +-------+-------+
                              |
                      +-------v-------+
                      |   EGLConfig   | (Frame buffer yetenekleri: RGB565 vs)
                      +-------+-------+
                              |
        +---------------------+---------------------+
        |                                           |
+-------v-------+                           +-------v-------+
|  EGLContext   | (Main)                    |  EGLContext   | (Shared)
+-------+-------+                           +-------+-------+
        |  - OpenGL State                           |  - Kendi OpenGL State'i
        |  - Kendi VRAM Objeleri (FBO, PBO)         |
        |                                           |
        +--< Paylaşılan (Shared) VRAM Objeleri >----+ (Texture, VBO)
                    (VRAM Bellek Tasarrufu)
```

## Parametreler

### `dpy` (EGLDisplay)
Bağlantı kurulan fiziksel veya sanal ekranın (display) tanıtıcısıdır.

| Değer | Sonuç |
|---|---|
| Geçerli ve initialize edilmiş `EGLDisplay` | İşlem diğer parametreler de doğruysa devam eder. |
| `EGL_NO_DISPLAY` veya sahte display handle | Başarısız. `EGL_BAD_DISPLAY` hatası döner. |
| `eglInitialize` çağrılmamış display | Başarısız. `EGL_NOT_INITIALIZED` hatası döner. |

### `config` (EGLConfig)
Oluşturulacak bağlamın renk, derinlik (depth) ve stencil tampon gereksinimlerini belirleyen donanım konfigürasyonudur.

| Değer | Sonuç |
|---|---|
| `dpy` üzerinden `eglChooseConfig` ile alınmış geçerli `EGLConfig` | Başarılı. Context bu formatta render yapmak üzere ayarlanır. |
| Geçersiz veya uyuşmaz config handle | Başarısız. `EGL_BAD_CONFIG` hatası döner. |

### `share_context` (EGLContext)
Doku (texture) ve köşe tamponu (VBO) gibi VRAM nesnelerinin paylaşılacağı mevcut bir context. Güvenlik kritik sistemlerde bellek tasarrufu için hayati öneme sahiptir.

| Değer | Sonuç |
|---|---|
| `EGL_NO_CONTEXT` | İzolasyon. Context hiçbir VRAM verisini paylaşmaz, izole çalışır. |
| Geçerli bir `EGLContext` handle | Paylaşım. İki context texture gibi objelere ortak erişim sağlar. State'ler (viewport vb.) ayrı kalır. |
| Geçersiz handle veya farklı `dpy`'ye ait context | Başarısız. `EGL_BAD_CONTEXT` veya `EGL_BAD_MATCH` hatası döner. |

### `attrib_list` (const EGLint *)
Context oluşturulurken istenen ekstra özellikleri (API sürümü vb.) belirten anahtar-değer (key-value) çifti listesidir.

| Değer | Sonuç |
|---|---|
| `NULL` veya `{ EGL_NONE }` | EGL 1.0 standart kullanımı. Varsayılan (default) context (genellikle GLES 1.x) oluşturulur. |
| Geçerli attribute listesi (Örn: GLES 2.0 talebi) | İlgili gereksinimleri karşılayan bağlam oluşturulur. |
| Geçersiz/Desteklenmeyen attribute | Başarısız. `EGL_BAD_ATTRIBUTE` hatası döner. |

## Geçerli Attribute Listesi

`attrib_list`, `{ anahtar, değer, ..., EGL_NONE }` formatında sonlanan bir dizidir.

| Attribute | Tip | Anlam |
|---|---|---|
| `EGL_CONTEXT_CLIENT_VERSION` | `EGLint` | İstenen OpenGL ES sürümü (Örn: `1` veya `2`). EGL 1.0 spesifikasyonunda standart olarak attribute alınmaz (Bölüm 3.6.1), ancak EGL 1.3 ve sonrası uzantılarla bu parametre GLES 2.x/3.x kullanımı için kritik hale gelmiştir. |
| `EGL_NONE` | `EGLint` | Liste sonlandırıcı belirteç. **Zorunludur.** |

*Not: `eglCreateContext` doğrudan buffer tiplerine (single buffered pixmap veya back buffered window) bağımlı değildir; bu uyumluluk yüzey (surface) oluşturulurken (`eglCreateWindowSurface`) ve bağlam yüzeye bağlanırken (`eglMakeCurrent`) denetlenir.*

## Ayrıntılar ve Yaşam Döngüsü

- **Thread Güvenliği (Thread Safety):** EGLContext aynı anda (concurrently) sadece bir iş parçacığında (thread) aktif (`current`) olabilir. Başka bir thread bu context'i `eglMakeCurrent` ile aktif etmek isterse, mevcut thread'in önce bağlamı serbest bırakması (unbind) gerekir. EGL spesifikasyonu, bir context'in birden fazla thread'de kullanılmasını yasaklar (`EGL_BAD_ACCESS` döner).
- **Yaşam Döngüsü:** Context oluşturulduğunda bellek tahsisi yapılır ancak ekrana çizim yapamaz. Çizim için `eglMakeCurrent` şarttır. Yok edilmesi ise `eglDestroyContext` ile yapılır.
- **Eşzamanlama (Synchronization):** `share_context` ile VRAM paylaşımı yapıldığında, OpenGL ES komutları otomatik olarak senkronize olmaz. İki farklı thread, paylaşılan bir dokuya (texture) aynı anda yazmaya/okumaya çalışırsa Undefined Behavior (tanımsız davranış) oluşur. Bu durumu engellemek için `glFinish()`, `eglWaitGL()` veya `eglWaitNative()` gibi açık eşzamanlama bariyerleri kullanılmalıdır.

## Hata Matrisi

Khronos spesifikasyonu (Bölüm 3.6.1) kuralı gereği: Fonksiyon başarısız olduğunda state machine'de hiçbir değişiklik olmaz, VRAM veya RAM sızıntısı yaşanmaz (No side effects). Hata durumunda `EGL_NO_CONTEXT` döner. Hatayı okumak için `eglGetError()` çağrılmalıdır.

| Durum | Sonuç (EGL Hata Kodu) |
|---|---|
| Geçerli parametreler ve yeterli donanım/bellek | Başarılı (Geçerli `EGLContext` döner) |
| `dpy` geçerli bir display değilse | `EGL_BAD_DISPLAY` |
| `dpy` EGL ile initialize edilmemişse | `EGL_NOT_INITIALIZED` |
| `config` geçersiz bir EGL konfigürasyonu ise | `EGL_BAD_CONFIG` |
| `share_context` geçersizse (`EGL_NO_CONTEXT` değilse) | `EGL_BAD_CONTEXT` |
| Context'ler paylaşılamıyorsa (uyuşmaz API veya donanım kısıtlaması) | `EGL_BAD_MATCH` |
| `attrib_list` geçersiz bir attribute içeriyorsa | `EGL_BAD_ATTRIBUTE` |
| İşletim sistemi veya GPU belleğinde yer kalmadıysa | `EGL_BAD_ALLOC` |

## Güvenli Kullanım Örneği

```c
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>

// Güvenlik kritik sistemler (Avionics) için Robust Context Oluşturma
EGLContext CreateSecureContext(EGLDisplay dpy, EGLConfig config, EGLContext shared_ctx) {
    // EGL 1.0 uyumlu null-terminated liste
    const EGLint attrib_list[] = {
        EGL_NONE // GLES 1.x / EGL 1.0 Default
    };

    // Context oluştur (Thread güvenli ve state-protected çağrı)
    EGLContext context = eglCreateContext(dpy, config, shared_ctx, attrib_list);

    // Error checking (Hata kontrolü) - Zero Side Effect kuralı denetimi
    if (context == EGL_NO_CONTEXT) {
        EGLint err = eglGetError();
        switch (err) {
            case EGL_BAD_DISPLAY:
                fprintf(stderr, "Kritik Hata: Gecersiz Display Handle.\n");
                break;
            case EGL_NOT_INITIALIZED:
                fprintf(stderr, "Kritik Hata: EGL sistemi baslatilmamis.\n");
                break;
            case EGL_BAD_CONFIG:
                fprintf(stderr, "Hata: Donanim konfigürasyonu gecersiz veya uyuşmuyor.\n");
                break;
            case EGL_BAD_ALLOC:
                fprintf(stderr, "OOM (Out of Memory): GPU veya Sistem bellegi yetersiz!\n");
                // Aviyonik sistemlerde bu durum watchdog timer veya soft-reset tetiklemelidir.
                break;
            case EGL_BAD_MATCH:
                fprintf(stderr, "Hata: Paylasimli context konfigürasyonu uyusmazligi.\n");
                break;
            case EGL_BAD_ATTRIBUTE:
                fprintf(stderr, "Hata: Desteklenmeyen EGL_ATTRIBUTE (API surumu vb.).\n");
                break;
            case EGL_BAD_CONTEXT:
                fprintf(stderr, "Hata: Paylasilmak istenen kaynak context gecersiz.\n");
                break;
            default:
                fprintf(stderr, "Bilinmeyen EGL hatasi: 0x%04x\n", err);
                break;
        }
        return EGL_NO_CONTEXT;
    }

    // Basarili: Bellek tahsisi ve state machine hazir.
    return context;
}
```

## Pratik Özet

- **State Yönetimi:** `eglCreateContext`, OpenGL'in çalışacağı boş beyni oluşturur; ancak gözleri (yüzey) ve elleri (current thread) yoktur. `eglMakeCurrent` çağırmadan OpenGL ES komutları işlenemez.
- **Bellek Optimizasyonu (Shared Context):** Uçuş paneli gibi çok ekranlı (PFD, ND, EICAS) sistemlerde her ekrana ayrı bağlam (context) açmak yerine, VRAM dokularını (harita verileri, simgeler, fontlar) `share_context` ile paylaşmak muazzam bellek tasarrufu sağlar.
- **Güvenlik Kritik İzolasyon:** Hata oluştuğunda sistem state'i kesinlikle bozulmaz (`Zero side-effect`). Bu durum, DO-178C gibi havacılık standartlarında öngörülebilir deterministik davranışların sağlanabilmesi için idealdir.
- **Thread Kısıtlaması:** Context bir kez bir iş parçacığına (thread) `eglMakeCurrent` ile bağlandığında (bind edildiğinde), bağ bitene kadar başka hiçbir thread o bağlama dokunamaz. İhlal edilmesi fatal hata ve `EGL_BAD_ACCESS` doğurur.
- **Hata Kontrolü:** Fonksiyonun dönüş değeri daima `EGL_NO_CONTEXT` sabiti ile kıyaslanmalı, `NULL` (0) kontrolü yapılmamalıdır. Hata alınması durumunda yalnızca `eglGetError()` durumu aydınlatabilir.
