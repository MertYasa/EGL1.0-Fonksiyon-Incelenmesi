# EGL 1.0: `eglGetConfigs`

```c
EGLBoolean eglGetConfigs(EGLDisplay dpy,
                         EGLConfig *configs,
                         EGLint config_size,
                         EGLint *num_config);
```

`eglGetConfigs`, daha önceden başlatılmış (initialized) bir EGL görüntüleme bağlantısı (`EGLDisplay`) üzerinden, sistemin donanımsal olarak desteklediği tüm geçerli framebuffer yapılandırmalarının (`EGLConfig`) listesini veya yalnızca mevcut konfigürasyonların toplam sayısını elde etmek için kullanılan temel bir EGL fonksiyonudur.

## Mental Model

EGL mimarisinde `eglGetConfigs` fonksiyonu, işletim sisteminin native pencereleme sistemi (X11, Wayland, DRM/GBM veya gömülü pencerelendirme sistemleri) ile EGL arasındaki uyumlu framebuffer formatlarının bir envanterini sunar.

```text
[İşletim Sistemi / Windowing System]        [EGL State Machine]
      Native Display (ör. X11 Display)  --->  EGLDisplay
                                                  |
                                                  +---> [EGLConfig Havuzu] (eglGetConfigs buradan okur)
                                                          |
                                                          +-- EGLConfig #1 (RGB565, Z16, Single Buffer)
                                                          |    +-- EGL_BUFFER_SIZE: 16
                                                          |    +-- EGL_DEPTH_SIZE: 16
                                                          |
                                                          +-- EGLConfig #2 (RGBA8888, Z24, Back Buffer)
                                                          |    +-- EGL_BUFFER_SIZE: 32
                                                          |    +-- EGL_DEPTH_SIZE: 24
                                                          |
                                                          +-- EGLConfig #N ...
```

Bu fonksiyon havuz üzerinde herhangi bir filtreleme veya sıralama yapmaz (bu görev `eglChooseConfig`'e aittir); yalnızca sistemin native konfigürasyonlarına karşılık gelen tüm EGLConfig handle'larını olduğu gibi kullanıcıya sunar.

## Parametreler

### `dpy` (EGLDisplay)

EGL ortamını temsil eden opaque (kapalı) bir handle'dır. İşletim sisteminin native display'i (örneğin X11 `Display*` veya DRM dosya tanımlayıcısı) kullanılarak `eglGetDisplay` ile elde edilir ve `eglInitialize` ile başlatılmış olmalıdır.

| Değer | Sonuç |
|---|---|
| Geçerli ve Başlatılmış `EGLDisplay` | İşlem devam eder, diğer argümanlar da doğruysa sorgu başarılıdır. |
| `EGL_NO_DISPLAY` | Fonksiyon `EGL_FALSE` döner. `eglGetError()` -> `EGL_BAD_DISPLAY` üretir. |
| Geçersiz Handle | Fonksiyon `EGL_FALSE` döner. `eglGetError()` -> `EGL_BAD_DISPLAY` üretir. |
| Başlatılmamış (Uninitialized) Display | Fonksiyon `EGL_FALSE` döner. `eglGetError()` -> `EGL_NOT_INITIALIZED` üretir. |

### `configs` (EGLConfig*)

EGL konfigürasyonlarının handle'larının (tanımlayıcılarının) yazılacağı bellek bölgesini (diziyi) işaret eder.

| Değer | Sonuç |
|---|---|
| Geçerli Bellek Adresi | Sistemdeki mevcut konfigürasyonlar (en fazla `config_size` kadar) bu diziye kopyalanır. |
| `NULL` | Konfigürasyonlar kopyalanmaz. Sadece toplam config sayısı hesaplanıp `num_config` adresine yazılır (2 adımlı sorgu paterni). |

### `config_size` (EGLint)

`configs` dizisine kopyalanabilecek maksimum `EGLConfig` sayısını belirten tam sayıdır. 

| Değer | Sonuç |
|---|---|
| `0` | Eğer `configs` geçerli bir dizi ise hiçbir veri kopyalanmaz, `num_config` değeri `0` olur. (Fakat `configs` `NULL` ise EGL standardınca dikkate alınmaz). |
| Toplam Config < `config_size` | Sistemdeki tüm config'ler diziye yazılır. `num_config` içine kopyalanan miktar yazılır. Kalan dizi kapasitesi değiştirilmez. |
| Toplam Config > `config_size` | EGL yalnızca `config_size` kadar config'i diziye kopyalar. Taşanlar göz ardı edilir, herhangi bir hata üretilmez. `num_config` değeri `config_size` değerine eşit olur. |

### `num_config` (EGLint*)

EGL'nin "Bu diziye kaç konfigürasyon yazdım?" (eğer `configs != NULL` ise) veya "Sistemde toplam kaç konfigürasyon var?" (eğer `configs == NULL` ise) sorusuna cevabını ilettiği çıkış (output) parametresidir.

| Değer | Sonuç |
|---|---|
| Geçerli `EGLint*` adresi | Elde edilen sayı bu bellek adresine başarıyla yazılır. |
| `NULL` | Khronos standardına göre kesinlikle yasaktır. Çökme (segmentation fault) veya `EGL_BAD_PARAMETER` üretir. |

## Geçerli Attribute Listesi ve Tampon (Buffer) Tipleri

`eglGetConfigs` fonksiyonu herhangi bir nitelik (attribute) listesi argümanı almaz. Sistemin desteklediği tüm EGL konfigürasyonlarını filtresiz döndürür. Elde edilen her bir `EGLConfig`, `eglGetConfigAttrib` kullanılarak tek tek incelenebilir. 

Elde edilecek konfigürasyonlar donanımın desteklediği aşağıdaki tampon tiplerine sahip olabilir ve aralarındaki davranış farklılıkları kritik öneme sahiptir:
*   **Back Buffered (Çift Tamponlu):** Pencere yüzeyleri (Window Surfaces) için uygundur. Ekranda yırtılma (tearing) olmaması için `eglSwapBuffers` işlemi gerektirir.
*   **Single Buffered (Tek Tamponlu):** Pixmap yüzeyleri (Pixmap Surfaces) için uygundur. Çizilen her şey anında native belleğe yansır.

Hangisinin hangi tampon tipini desteklediğini anlamak için dönen config'lerin `EGL_SURFACE_TYPE` niteliği (`EGL_WINDOW_BIT`, `EGL_PIXMAP_BIT`, `EGL_PBUFFER_BIT`) kontrol edilmelidir.

## Ayrıntılar ve Yaşam Döngüsü

**Thread Güvenliği (Thread Safety):** 
EGL'de konfigürasyon elde etme işlemleri (örneğin `eglGetConfigs`) thread-safe'tir. Birden fazla thread aynı `EGLDisplay` üzerinden eşzamanlı (concurrent) olarak bu fonksiyonu çağırabilir. Veri EGL state machine'ini modifiye etmeyip sadece okunup döndürüldüğü için bir yarış koşulu yaratmaz. 

**Eşzamanlama (Synchronization):** 
Bu fonksiyon donanıma veya Native render motoruna komut (command buffer) göndermez, sürücünün (driver) önceden oluşturduğu statik konfigürasyon listesini okur. Bu yüzden `glFinish()`, `eglWaitGL()` veya `eglWaitNative()` gibi komutların çağrılmasına gerek yoktur. 

**Yaşam Döngüsü:** 
Döndürülen `EGLConfig` handle'ları, o handle'ları barındıran `EGLDisplay` var olduğu sürece geçerlidir. Display `eglTerminate` ile sonlandırıldığında bu config'ler geçersizleşir ve bir daha kullanılamazlar.

## Hata Matrisi

EGL spesifikasyonunun katı kuralları gereği `eglGetConfigs` başarısız olduğunda **hiçbir yan etki (side effect) bırakmamalıdır.** Argümanlardaki bellek alanları değiştirilmez ve state machine mevcut durumunu aynen korur.

| Durum | Sonuç (Fonksiyon Dönüşü) | EGL Hata Kodu (`eglGetError()`) |
|---|---|---|
| Her şey geçerli ve başarılı | `EGL_TRUE` | `EGL_SUCCESS` |
| `dpy` geçerli bir display değil veya `EGL_NO_DISPLAY` | `EGL_FALSE` | `EGL_BAD_DISPLAY` |
| `dpy` henüz `eglInitialize` ile başlatılmamış | `EGL_FALSE` | `EGL_NOT_INITIALIZED` |
| `num_config` parametresinin `NULL` olması | `EGL_FALSE` (veya Crash) | `EGL_BAD_PARAMETER` |

## Güvenli Kullanım Örneği

Aşağıdaki C kodu, EGL spesifikasyonuna tam uyumlu, hafıza sızıntısı (memory leak) riskini sıfırlayan, modern EGL yazılımlarındaki **İki Adımlı Sorgu (Two-Step Query)** yaklaşımını gösterir.

```c
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Sistemdeki tüm konfigürasyonları elde edip ekrana sayısını yazdıran güvenli fonksiyon.
 * Başarılı olursa EGL_TRUE, aksi halde EGL_FALSE döner.
 */
EGLBoolean SafeGetEGLConfigs(EGLDisplay dpy) {
    EGLint total_configs = 0;
    EGLConfig *configs = NULL;

    // 1. ADIM: Sadece toplam config sayısını sor (configs = NULL geçirerek)
    if (eglGetConfigs(dpy, NULL, 0, &total_configs) != EGL_TRUE) {
        fprintf(stderr, "[Hata] EGL config sayısı okunamadı. Hata: 0x%04X\n", eglGetError());
        return EGL_FALSE;
    }

    if (total_configs == 0) {
        printf("[Bilgi] EGL başlatılmış ancak desteklenen konfigürasyon yok.\n");
        return EGL_TRUE; // Bu bir donanım limitidir, API hatası değil.
    }

    // 2. ADIM: Dönen sayı kadar heap'ten dinamik bellek tahsis et
    configs = (EGLConfig*) malloc(total_configs * sizeof(EGLConfig));
    if (configs == NULL) {
        fprintf(stderr, "[Kritik Hata] Bellek yetersizliği (Out of memory).\n");
        return EGL_FALSE;
    }

    // 3. ADIM: Ayrılan belleği gerçek konfigürasyonlarla doldur
    if (eglGetConfigs(dpy, configs, total_configs, &total_configs) != EGL_TRUE) {
        fprintf(stderr, "[Hata] EGL config verileri okunamadı. Hata: 0x%04X\n", eglGetError());
        free(configs);
        return EGL_FALSE;
    }

    printf("Başarılı: Sistemden %d adet EGLConfig çekildi.\n", total_configs);

    // TODO: Burada 'configs' dizisi eglGetConfigAttrib ile filtrelenip
    // projenin ihtiyaç duyduğu Back Buffered veya pBuffer özellikli config seçilebilir.

    // İşlem bittikten sonra ayrılan belleği daima temizle (Memory Leak önleme)
    free(configs);
    
    return EGL_TRUE;
}
```

## Pratik Özet

*   **Amaç ve Sınırlar:** `eglGetConfigs`, veritabanındaki tüm konfigürasyonları hiçbir filtre uygulamadan ham (raw) formatta listeler. Özel ihtiyaçlar (ör. Z-Buffer'ı 24 bit olan configler) aranıyorsa her zaman `eglChooseConfig` tercih edilmelidir.
*   **İki Adımlı Sorgu (Best Practice):** Gömülü ve safety-critical sistemlerde rastgele boyutlu statik diziler (ör. `EGLConfig arr[100]`) gereksiz bellek israfına veya array taşmasına yol açabilir. Her zaman önce `configs = NULL` ile sayıyı öğrenin ve dinamik olarak yeterli alan ayırın.
*   **Null Pointer Kısıtı:** `num_config` argümanı hiçbir senaryoda (sadece sayım yaparken bile) `NULL` olamaz, adres geçmek zorunludur. Aksi takdirde uygulamanız EGL standardını ihlal eder ve crash olabilir.
*   **Yan Etkisizlik:** Fonksiyon salt-okunur (read-only) tabanlı bir işlemdir. Başarısız çağrılar hiçbir state değişikliğine veya mevcut bellek alanında bozulmaya neden olmaz.
