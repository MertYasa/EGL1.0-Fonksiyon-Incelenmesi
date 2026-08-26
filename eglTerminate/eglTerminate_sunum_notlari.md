# `eglTerminate` – Hocaya Anlatım Metni

## Fonksiyon Prototipi

```c
EGLBoolean eglTerminate(EGLDisplay dpy);
```

Hocam, `eglTerminate` fonksiyonunun temel amacı:

> Daha önce `eglInitialize` ile başlatılmış bir `EGLDisplay` üzerindeki EGL oturumunu sonlandırmak ve bu display ile ilişkili EGL kaynaklarını temizlemektir.

Bu fonksiyonu genel EGL yaşam döngüsünde:

```text
eglInitialize
    ↓
EGL kullanımı
    ↓
eglTerminate
```

şeklinde düşünebiliriz.

Yani:

```text
eglInitialize
→ EGLDisplay'i kullanıma hazır hale getirir.

eglTerminate
→ EGLDisplay'in EGL tarafındaki kullanımını sonlandırır.
```

Bu nedenle `eglTerminate`, EGL kullanımının kapanış aşamasındaki temel fonksiyonlardan biridir.

---

# Mental Model

Fonksiyonun mantığını şöyle düşünebiliriz:

```text
Native Display
      │
      ▼
  EGLDisplay
      │
      ├── EGLContext
      ├── EGLSurface
      ├── EGLConfig bilgileri
      └── EGL internal state
             │
             ▼
      eglTerminate(dpy)
             │
             ▼
   EGLDisplay uninitialized
```

Burada önemli nokta:

> `eglTerminate`, native pencereleme sistemini veya işletim sistemi kaynağını mutlaka yok etmez.

Temel olarak EGL katmanındaki bağlantıyı ve EGL'nin yönettiği kaynakları sonlandırır.

Örneğin:

```text
X11 Display
DRM file descriptor
GBM device
```

gibi native kaynakların ayrıca kendi API'leriyle kapatılması gerekebilir.

---

# 1. Parametre: `dpy`

Fonksiyon yalnızca bir parametre alır:

```c
EGLDisplay dpy
```

`dpy`, hangi EGL display bağlantısını sonlandırmak istediğimizi belirtir.

Yani bu parametre şu sorunun cevabıdır:

> "Hangi EGL oturumunu kapatıyorum?"

Örneğin:

```c
EGLDisplay dpy =
    eglGetDisplay(EGL_DEFAULT_DISPLAY);

eglInitialize(dpy, NULL, NULL);
```

ile bir EGLDisplay başlattığımızı düşünelim.

Programın sonunda:

```c
eglTerminate(dpy);
```

çağırarak bu display'in EGL tarafındaki oturumunu sonlandırırız.

---

# `dpy` İçin Temel Senaryolar

Bu fonksiyonda üç temel senaryoyu ayırmak önemlidir.

```text
1. Geçerli + initialized display
2. Geçerli ama uninitialized display
3. Geçersiz display
```

Bunların davranışları birbirinden farklıdır.

---

# Senaryo 1 — Geçerli ve Initialized Display

Örneğin:

```c
EGLDisplay dpy =
    eglGetDisplay(EGL_DEFAULT_DISPLAY);

eglInitialize(dpy, NULL, NULL);
```

daha sonra:

```c
EGLBoolean result =
    eglTerminate(dpy);
```

çağırıyoruz.

Bu durumda EGL:

- display'i uninitialized duruma geçirir,
- display'e bağlı EGL kaynaklarını temizler veya yok edilmek üzere işaretler,
- EGL tarafındaki oturumu kapatır.

Başarılı sonuç:

```text
EGL_TRUE
```

olur.

Yani:

```text
Initialized EGLDisplay
       │
       ▼
 eglTerminate
       │
       ▼
Uninitialized EGLDisplay
```

---

# Senaryo 2 — Geçerli Ama Zaten Uninitialized Display

Bu durum biraz ilginçtir.

Display handle geçerli olabilir ancak EGL bu display üzerinde artık initialize durumda olmayabilir.

Örneğin daha önce:

```c
eglTerminate(dpy);
```

çağrılmış olabilir.

Sonra tekrar:

```c
eglTerminate(dpy);
```

çağırıyoruz.

Kaynak dosyadaki EGL 1.0 davranışına göre bu çağrı güvenlidir ve:

```text
EGL_TRUE
```

döner.

Yani ikinci terminate çağrısı:

> "Zaten kapalı olan EGL oturumunu tekrar kapatmaya çalışıyorsun."

gibi düşünülebilir.

Bu durumda yeni bir kaynak temizliği yapılmaz.

---

# Senaryo 3 — Geçersiz Display

Örneğin:

```c
eglTerminate(EGL_NO_DISPLAY);
```

çağırırsak:

```text
EGL_FALSE
```

döner.

Hata:

```text
EGL_BAD_DISPLAY
```

olur.

Buradaki problem:

> EGL'ye sonlandırılabilecek geçerli bir display handle verilmemiş olmasıdır.

---

# `EGL_BAD_DISPLAY` ile Uninitialized Display Farkı

Bu ayrım özellikle önemlidir.

```text
Geçersiz display
      ↓
EGL_BAD_DISPLAY
      ↓
EGL_FALSE
```

Ama:

```text
Geçerli display
ama uninitialized
      ↓
Hata yok
      ↓
EGL_TRUE
```

Yani:

> Uninitialized olmak ile geçersiz olmak aynı şey değildir.

Geçerli bir `EGLDisplay` handle'ı hâlâ var olabilir ama EGL oturumu kapalı olabilir.

---

# `eglInitialize` ile `eglTerminate` Farkı

Bu iki fonksiyonu birbirinin yaşam döngüsü karşılığı gibi düşünebiliriz.

```text
eglInitialize
      ↓
EGLDisplay'i başlat

eglTerminate
      ↓
EGLDisplay'in EGL oturumunu kapat
```

Şöyle:

```text
Uninitialized
      │
      │ eglInitialize
      ▼
 Initialized
      │
      │ eglTerminate
      ▼
Uninitialized
```

Bu şema fonksiyonun temel mantığını çok iyi anlatır.

---

# `eglTerminate` Context ve Surface'lere Ne Yapar?

Bu fonksiyonun önemli kısmı burada başlar.

Display ile ilişkili:

- `EGLContext`
- `EGLSurface`

gibi EGL kaynakları bulunabilir.

Ancak bunların hepsinin davranışı aynı değildir.

En önemli ayrım:

```text
Current değil
vs
Current
```

---

# Current Olmayan Nesneler

Bir context veya surface herhangi bir thread üzerinde current değilse:

```text
EGLContext
      ↓
Not Current
```

ve:

```text
EGLSurface
      ↓
Not Current
```

`eglTerminate` çağrıldığında bunlar temizlenebilir.

Mantık:

```text
Not Current EGL Object
        │
        ▼
  eglTerminate
        │
        ▼
     Destroy
```

---

# Current Olan Nesneler

Asıl önemli durum budur.

Bir context başka bir thread üzerinde current olabilir.

Örneğin:

```text
Thread A
    │
    ▼
Context X
CURRENT
```

Main thread ise:

```c
eglTerminate(dpy);
```

çağırabilir.

Bu durumda current context'i anında silmek tehlikeli olur.

Çünkü Thread A hâlâ o context'i kullanıyor olabilir.

Bu nedenle kaynak dosyadaki anlatıma göre nesne:

```text
Pending Destruction
```

yani:

> Bekleyen yıkım

durumuna geçebilir.

---

# Pending Destruction Nedir?

Şöyle düşünelim:

```text
Thread A
   │
   ▼
Context X
CURRENT
```

Başka thread:

```c
eglTerminate(dpy);
```

çağırıyor.

Context X'i hemen yok etmek yerine:

```text
Context X
     ↓
Pending Destruction
```

durumuna getiriyoruz.

Yani:

> "Bu nesne artık kapatılacak, ancak hâlâ current olduğu için hemen tamamen yok edemiyorum."

İlgili thread context'i bıraktığında kaynak tamamen temizlenebilir.

---

# Context Nasıl Unbind Edilir?

Current context'i thread'den ayırmak için:

```c
eglMakeCurrent(
    dpy,
    EGL_NO_SURFACE,
    EGL_NO_SURFACE,
    EGL_NO_CONTEXT
);
```

kullanılabilir.

Bu çağrı mantıksal olarak:

> "Bu thread üzerinde artık hiçbir EGLContext ve surface current olmasın."

anlamına gelir.

Sonrasında `eglTerminate` yapılması daha temiz bir kapanış sağlar.

---

# Güvenli Kapanış Sırası

Hocaya en rahat şu sırayla anlatabilirim:

```text
1. Render işlemlerini bitir
        ↓
2. Context ve surface'leri unbind et
        ↓
3. eglTerminate(dpy)
        ↓
4. Native kaynakları ayrıca temizle
```

Kod olarak:

```c
eglMakeCurrent(
    dpy,
    EGL_NO_SURFACE,
    EGL_NO_SURFACE,
    EGL_NO_CONTEXT
);

eglTerminate(dpy);
```

sonrasında platforma göre:

```c
gbm_device_destroy(...);
close(...);
```

veya ilgili native API cleanup işlemleri yapılabilir.

---

# Native Kaynakları da Kapatır mı?

Çok önemli soru:

> "`eglTerminate` çağırınca X11 Display, DRM fd veya GBM device da otomatik kapanır mı?"

Genel cevap:

> Hayır, `eglTerminate` EGL'nin kendi display bağlantısını ve EGL kaynaklarını sonlandırır. Native kaynağın yaşam döngüsü ayrı olabilir.

Örneğin DRM/GBM tarafında:

```text
DRM fd
GBM device
```

EGL dışında oluşturulmuşsa bunların ayrıca:

```c
close(drm_fd);
gbm_device_destroy(gbm_dev);
```

gibi native fonksiyonlarla temizlenmesi gerekebilir.

Yani:

```text
eglTerminate
      ↓
EGL cleanup

Native cleanup
      ↓
Ayrı işlem
```

---

# EGLDisplay ile Native Display Aynı Şey mi?

Hayır.

Bu ayrımı da bilmek önemlidir.

```text
Native Display
      ↓
İşletim sistemi / pencere sistemi nesnesi

EGLDisplay
      ↓
EGL'nin native display üzerine kurduğu soyut bağlantı
```

Dolayısıyla:

```c
eglTerminate(dpy);
```

EGLDisplay'i sonlandırır.

Ama altında bulunan native display kaynağını otomatik olarak kapattığını varsaymamalıyız.

---

# Attribute Listesi Var mı?

Hayır.

Fonksiyon:

```c
eglTerminate(EGLDisplay dpy);
```

şeklindedir.

Yani:

- attribute listesi yok,
- config yok,
- context parametresi yok,
- surface parametresi yok.

Sadece:

```text
dpy
```

vardır.

Bunun nedeni:

> Terminate işlemi tek tek context veya surface değil, tüm EGLDisplay oturumunu hedef alır.

---

# `eglTerminate` Sonrasında Ne Olur?

Başarılı terminate sonrasında display:

```text
uninitialized
```

durumuna geçer.

Artık bu display ile çoğu EGL işlemini doğrudan yapamayız.

Örneğin:

```c
eglCreateContext(...)
```

veya:

```c
eglChooseConfig(...)
```

gibi işlemleri yapmadan önce display'in yeniden:

```c
eglInitialize(...)
```

ile başlatılması gerekir.

Genel akış:

```text
eglTerminate
      ↓
Uninitialized

eglCreateContext
      ↓
OLMAZ

Önce:
eglInitialize
      ↓
Sonra:
eglCreateContext
```

---

# `eglTerminate` Sonrası Tekrar Kullanmak İstersek

Display handle'ı hâlâ kullanılabilir bir EGLDisplay ise tekrar:

```c
eglInitialize(dpy, ...);
```

çağrılabilir.

Yani yaşam döngüsü teorik olarak:

```text
eglInitialize
      ↓
Kullan
      ↓
eglTerminate
      ↓
eglInitialize
      ↓
Tekrar kullan
```

şeklinde olabilir.

---

# Thread Konusu

`eglTerminate` thread açısından önemli bir fonksiyondur çünkü aynı display'e bağlı context başka thread'lerde current olabilir.

Örneğin:

```text
Thread A
    ↓
Context A current

Thread B
    ↓
Context B current

Main Thread
    ↓
eglTerminate(dpy)
```

Bu yüzden kapanış sırasında thread'lerin durumunu bilmek önemlidir.

En kontrollü yaklaşım:

> Önce rendering thread'lerini durdurmak veya context'lerini unbind ettirmek, ardından `eglTerminate` çağırmaktır.

---

# Eşzamanlama Konusu

GPU komutları CPU kodundan bağımsız olarak hâlâ çalışıyor olabilir.

Örneğin uygulama:

```text
glDraw...
glDraw...
glDraw...
```

komutları göndermiş olabilir.

CPU hemen ardından:

```c
eglTerminate(dpy);
```

çağırabilir.

Güvenli kapanış tasarımında bekleyen render işlemlerinin tamamlandığından emin olmak isteyebiliriz.

Kaynak metindeki örnekte:

```c
eglWaitGL();
```

ve gerektiğinde:

```c
eglWaitNative(...);
```

kullanımı ele alınmıştır.

Ancak temel kavram şudur:

> EGL kaynaklarını temizlemeden önce aktif render/native işlemlerinin durumunu kontrol etmek güvenli kapanış için önemlidir.

---

# Fonksiyonun Dönüş Değeri

Fonksiyonun dönüş tipi:

```c
EGLBoolean
```

dır.

Başarılıysa:

```text
EGL_TRUE
```

Başarısızsa:

```text
EGL_FALSE
```

döner.

Örneğin:

```c
EGLBoolean result =
    eglTerminate(dpy);

if (result == EGL_FALSE)
{
    EGLint err =
        eglGetError();
}
```

---

# Temel Hata Durumu

Bu fonksiyonda temel hata:

```text
EGL_BAD_DISPLAY
```

dir.

Örneğin:

```c
eglTerminate(EGL_NO_DISPLAY);
```

çağrısı:

```text
EGL_FALSE
```

döndürür ve:

```text
EGL_BAD_DISPLAY
```

hatasını oluşturur.

---

# Hata ile Normal Durumu Karıştırmamak

Şunu ayırmak gerekir:

```text
EGL_NO_DISPLAY
      ↓
Geçersiz handle
      ↓
EGL_BAD_DISPLAY
```

ile:

```text
Geçerli EGLDisplay
ama zaten uninitialized
      ↓
Normal durum
```

aynı değildir.

Bu fonksiyon için önemli senaryo farklarından biridir.

---

# Güvenli Kullanım Örneği

```c
#include <EGL/egl.h>
#include <stdio.h>

void safe_egl_cleanup(EGLDisplay dpy)
{
    if (dpy == EGL_NO_DISPLAY)
    {
        printf(
            "Gecersiz EGLDisplay.\n"
        );
        return;
    }

    // Current context/surface bağlarını kaldır.
    if (eglMakeCurrent(
            dpy,
            EGL_NO_SURFACE,
            EGL_NO_SURFACE,
            EGL_NO_CONTEXT) == EGL_FALSE)
    {
        printf(
            "Context unbind islemi basarisiz.\n"
        );
    }

    // EGL oturumunu sonlandır.
    if (eglTerminate(dpy) == EGL_FALSE)
    {
        EGLint err = eglGetError();

        printf(
            "eglTerminate basarisiz. Hata: 0x%04X\n",
            err
        );

        return;
    }

    printf(
        "EGL oturumu basariyla sonlandirildi.\n"
    );
}
```

Buradaki mantık:

```text
Current EGL state
      ↓
Unbind
      ↓
eglTerminate
      ↓
Uninitialized EGLDisplay
```

---

# `eglDestroyContext` ile `eglTerminate` Farkı

Hocanın sorması muhtemel önemli bir ayrımdır.

```text
eglDestroyContext
→ Belirli bir EGLContext'i yok eder.

eglTerminate
→ Tüm EGLDisplay oturumunu sonlandırır.
```

Örneğin:

```c
eglDestroyContext(
    dpy,
    ctx
);
```

sadece belirli context'i hedefler.

Ama:

```c
eglTerminate(dpy);
```

display seviyesinde kapanış yapar.

Kısaca:

```text
DestroyContext
→ Tek context

Terminate
→ Tüm EGL display oturumu
```

---

# `eglDestroySurface` ile `eglTerminate` Farkı

Aynı şekilde:

```text
eglDestroySurface
→ Tek surface'i yok eder.

eglTerminate
→ EGLDisplay seviyesindeki EGL oturumunu kapatır.
```

Bu yüzden program kapanışında kontrollü cleanup genellikle nesnelerin yaşam döngüsüne göre yapılmalıdır.

---

# `eglMakeCurrent` ile `eglTerminate` Farkı

```text
eglMakeCurrent
→ Bir context/surface bağını thread üzerinde ayarlar veya kaldırır.

eglTerminate
→ Display'in EGL oturumunu sonlandırır.
```

Örneğin:

```c
eglMakeCurrent(
    dpy,
    EGL_NO_SURFACE,
    EGL_NO_SURFACE,
    EGL_NO_CONTEXT
);
```

sadece current binding'i kaldırır.

EGL'yi tamamen kapatmaz.

Ardından:

```c
eglTerminate(dpy);
```

ile display sonlandırılır.

---

# `eglTerminate` Ne Yapmaz?

Fonksiyonu anlamanın en iyi yollarından biri ne yapmadığını bilmektir.

`eglTerminate`:

- yeni context oluşturmaz,
- yeni surface oluşturmaz,
- config seçmez,
- context'i current yapmaz,
- native pencere sistemini mutlaka kapatmaz,
- DRM file descriptor gibi native kaynakları otomatik olarak kapatmak zorunda değildir.

Yaptığı temel iş:

> Belirtilen EGLDisplay'in EGL oturumunu sonlandırmaktır.

---

# EGL Yaşam Döngüsündeki Yeri

Genel EGL akışında:

```text
eglGetDisplay
      ↓
eglInitialize
      ↓
eglGetConfigs / eglChooseConfig
      ↓
eglCreateContext
      ↓
eglCreateSurface
      ↓
eglMakeCurrent
      ↓
Rendering
      ↓
Context'i unbind et
      ↓
eglTerminate
      ↓
Native cleanup
```

Yani `eglTerminate`, EGL yaşam döngüsünün son tarafındadır.

---

# Hocanın Sorabileceği Hızlı Sorular

## `eglTerminate` ne yapar?

Belirtilen `EGLDisplay` üzerindeki EGL oturumunu sonlandırır ve display'i uninitialized duruma geçirir.

---

## Kaç parametre alır?

Bir tane:

```c
EGLDisplay dpy
```

---

## `dpy` neyi ifade eder?

Hangi EGLDisplay oturumunun sonlandırılacağını belirtir.

---

## Başarılı olursa ne döner?

```text
EGL_TRUE
```

---

## Geçersiz display verilirse?

```text
EGL_FALSE
```

ve:

```text
EGL_BAD_DISPLAY
```

---

## Zaten uninitialized display verilirse?

Kaynak dosyadaki EGL 1.0 anlatımına göre çağrı güvenlidir ve `EGL_TRUE` döner.

---

## `eglTerminate` ile `eglInitialize` ilişkisi nedir?

```text
eglInitialize
→ Başlatır.

eglTerminate
→ Sonlandırır.
```

---

## Current context hemen silinir mi?

Kaynak dosyadaki anlatıma göre current olan nesneler doğrudan yok edilmek yerine pending destruction durumunda kalabilir.

---

## Pending destruction ne demek?

Nesne yok edilmek üzere işaretlenmiştir ancak hâlâ bir thread üzerinde current olduğu için tamamen temizlenmesi ertelenmiştir.

---

## Context'i nasıl unbind ederim?

```c
eglMakeCurrent(
    dpy,
    EGL_NO_SURFACE,
    EGL_NO_SURFACE,
    EGL_NO_CONTEXT
);
```

---

## Native X11/DRM/GBM kaynaklarını da kapatır mı?

Hayır, bunların yaşam döngüsü EGL'den ayrı olabilir ve kendi native API'leriyle temizlenmeleri gerekebilir.

---

## `eglDestroyContext` ile farkı nedir?

```text
eglDestroyContext
→ Tek context.

eglTerminate
→ Tüm EGLDisplay oturumu.
```

---

# Hocanın Sorabileceği Senaryo 1

```c
eglInitialize(dpy, ...);

eglTerminate(dpy);
```

Ne olur?

Cevap:

> EGLDisplay'in EGL oturumu sonlandırılır, EGL tarafından yönetilen ilgili kaynaklar temizlenir veya uygun şekilde yok edilmek üzere işaretlenir ve display uninitialized duruma geçer.

---

# Hocanın Sorabileceği Senaryo 2

```c
eglTerminate(EGL_NO_DISPLAY);
```

Ne olur?

Cevap:

```text
EGL_FALSE
EGL_BAD_DISPLAY
```

---

# Hocanın Sorabileceği Senaryo 3

```c
eglTerminate(dpy);
eglTerminate(dpy);
```

İkinci çağrıda ne olur?

Cevap:

> İlk çağrı display'i uninitialized duruma getirir. Kaynak dosyadaki EGL 1.0 davranışına göre ikinci terminate çağrısı geçerli handle üzerinde hata oluşturmaz ve `EGL_TRUE` döner.

---

# Hocanın Sorabileceği Senaryo 4

Başka thread'de:

```text
Context X current
```

iken main thread:

```c
eglTerminate(dpy);
```

çağırıyor.

Ne olur?

Cevap:

> Current durumdaki EGL nesnelerinin yaşam döngüsü doğrudan current olmayan nesneler gibi değildir. Kaynak dosyadaki anlatıma göre bunlar pending destruction durumuna alınabilir ve context thread'den ayrıldıktan sonra tamamen temizlenebilir.

---

# Hocanın Sorabileceği Senaryo 5

`eglTerminate` sonrasında:

```c
eglCreateContext(...)
```

çağırırsak ne olur?

Cevap:

> Display artık uninitialized olduğu için önce yeniden `eglInitialize` çağrılmalıdır. Aksi durumda display üzerinde initialization gerektiren EGL çağrıları başarısız olabilir.

---

# En Kritik Şema

```text
                 eglTerminate(dpy)
                        │
                        ▼
                 dpy geçerli mi?
                  /            \
               Hayır           Evet
                │               │
                ▼               ▼
        EGL_BAD_DISPLAY    Initialized mı?
                            /          \
                          Evet         Hayır
                           │             │
                           ▼             ▼
                    EGL kaynakları    Zaten kapalı
                    temizle/pending    Güvenli çağrı
                           │
                           ▼
                    EGLDisplay
                    uninitialized
```

---

# Üç Fonksiyonu Birlikte Ezberleme

```text
eglInitialize
      ↓
EGL OTURUMUNU BAŞLAT

eglMakeCurrent
      ↓
CONTEXT'İ THREAD'E BAĞLA / AYIR

eglTerminate
      ↓
EGL OTURUMUNU SONLANDIR
```

---

# 30 Saniyelik Final Özeti

> `eglTerminate`, belirtilen `EGLDisplay` üzerindeki EGL oturumunu sonlandıran fonksiyondur. Tek parametresi `dpy`'dir ve hangi display'in kapatılacağını belirtir. Geçerli ve initialize edilmiş bir display verildiğinde EGL kaynakları temizlenir veya current durumlarına göre yok edilmek üzere işaretlenir ve display uninitialized hale gelir. Geçersiz display verilirse `EGL_FALSE` ve `EGL_BAD_DISPLAY` elde edilir. Current olan context ve surface'ler için pending destruction davranışı önemli olabilir; bu nedenle güvenli kapanışta önce context'i `eglMakeCurrent(..., EGL_NO_CONTEXT)` ile unbind etmek, ardından `eglTerminate` çağırmak mantıklıdır. Ayrıca `eglTerminate`, native X11, DRM veya GBM kaynaklarının tamamını otomatik olarak kapatmak zorunda değildir; native cleanup ayrı yapılmalıdır.

---

# Özellikle Öğrenilmesi Gereken 5 Ayrım

1. `eglInitialize` ↔ `eglTerminate`
2. Geçersiz `EGLDisplay` ↔ uninitialized ama geçerli `EGLDisplay`
3. Current olmayan nesne ↔ current/pending destruction nesnesi
4. `eglDestroyContext` ↔ `eglTerminate`
5. EGL kaynakları ↔ native sistem kaynakları

---

# Tek Satırlık Ezber

```text
eglTerminate =
"Bu EGLDisplay'in EGL oturumunu kapat ve uninitialized duruma getir."
```
