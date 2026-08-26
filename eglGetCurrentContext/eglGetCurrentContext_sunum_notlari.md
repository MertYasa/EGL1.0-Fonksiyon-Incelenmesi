# `eglGetCurrentContext` – Hocaya Anlatım Metni

## Fonksiyon Prototipi

```c
EGLContext eglGetCurrentContext(void);
```

Hocam, `eglGetCurrentContext` fonksiyonunun temel amacı:

> Çağrıyı yapan thread üzerinde o anda aktif, yani **current** durumda olan `EGLContext` handle'ını döndürmektir.

Bu fonksiyon yeni bir context oluşturmaz.

Mevcut context'i değiştirmez.

Sadece:

> "Bu thread üzerinde şu anda hangi context aktif?"

sorusunun cevabını verir.

Eğer bu thread üzerinde aktif bir context yoksa:

```text
EGL_NO_CONTEXT
```

döner.

---

# Mental Model

Fonksiyonun mantığını şöyle düşünebiliriz:

```text
Thread A
   │
   ├── Current Context → Context X
   │
   └── eglGetCurrentContext()
              │
              ▼
          Context X
```

Başka bir thread için:

```text
Thread B
   │
   ├── Current Context → Context Y
   │
   └── eglGetCurrentContext()
              │
              ▼
          Context Y
```

Yani önemli nokta:

> `eglGetCurrentContext` global olarak sistemde hangi context'in aktif olduğunu sormaz.

Sadece çağrıyı yapan **mevcut thread'in current context'ini** sorar.

---

# Bu Fonksiyonun Parametresi Yok

Fonksiyon:

```c
eglGetCurrentContext(void);
```

şeklindedir.

Buradaki:

```c
void
```

şunu ifade eder:

> Fonksiyona herhangi bir parametre vermiyoruz.

Yani bu fonksiyon:

- `EGLDisplay` istemez,
- `EGLConfig` istemez,
- `EGLSurface` istemez,
- context handle istemez,
- attribute listesi istemez.

Bunun sebebi şudur:

> Hangi context'in current olduğu bilgisi zaten çağrıyı yapan thread'in EGL durumunda tutulmaktadır.

Dolayısıyla fonksiyonun dışarıdan bilgi almasına gerek yoktur.

---

# Fonksiyon Ne Döndürür?

Dönüş tipi:

```c
EGLContext
```

tir.

İki temel sonuç vardır.

## Senaryo 1 — Thread üzerinde aktif context var

Örneğin daha önce:

```c
eglMakeCurrent(
    dpy,
    drawSurface,
    readSurface,
    ctx
);
```

başarıyla çağrılmış olsun.

Sonrasında:

```c
EGLContext current =
    eglGetCurrentContext();
```

çağırırsak:

```text
current == ctx
```

olması beklenir.

Yani fonksiyon aktif context'in handle'ını döndürür.

---

## Senaryo 2 — Thread üzerinde aktif context yok

Eğer bu thread'e hiçbir context bağlanmamışsa:

```c
EGLContext current =
    eglGetCurrentContext();
```

sonucu:

```text
EGL_NO_CONTEXT
```

olur.

Bu bir hata olmak zorunda değildir.

Sadece:

> "Bu thread üzerinde current context yok."

anlamına gelir.

---

# En Önemli İlişki: `eglMakeCurrent`

`eglGetCurrentContext` fonksiyonunu anlamak için `eglMakeCurrent` ile ilişkisini anlamak gerekir.

`eglMakeCurrent`:

> Bir context'i belirli bir thread üzerinde current hale getirir.

`eglGetCurrentContext` ise:

> Hangi context'in current olduğunu sorgular.

Yani:

```text
eglMakeCurrent
      ↓
Context'i current yap

eglGetCurrentContext
      ↓
Current context hangisi?
```

Bunlar birbirinin tamamlayıcısıdır.

---

# Örnek

```c
EGLContext ctx = ...;

eglMakeCurrent(
    dpy,
    draw,
    read,
    ctx
);

EGLContext current =
    eglGetCurrentContext();
```

Burada:

```text
current == ctx
```

olmasını bekleriz.

Böylece context'in gerçekten current olduğunu doğrulayabiliriz.

---

# `current` Ne Demek?

Buradaki "current" kelimesini:

> "Bu thread tarafından şu anda kullanılan aktif rendering context"

olarak düşünebiliriz.

Yani context sadece oluşturulmuş olabilir:

```c
EGLContext ctx =
    eglCreateContext(...);
```

Ama henüz:

```c
eglMakeCurrent(...)
```

ile thread'e bağlanmamış olabilir.

Bu durumda:

```c
eglGetCurrentContext();
```

çağrısı o `ctx` değerini döndürmez.

Çünkü context vardır ama **current değildir**.

---

# Oluşturulmuş Context ile Current Context Farkı

Bu ayrım çok önemlidir.

```text
eglCreateContext
      ↓
Context oluşturuldu.

Ama henüz current olmayabilir.
```

Sonrasında:

```text
eglMakeCurrent
      ↓
Context mevcut thread'e bağlandı.
```

Bundan sonra:

```text
eglGetCurrentContext
      ↓
O context'i döndürür.
```

Kısaca:

```text
Existence ≠ Current
```

Türkçesi:

> Bir context'in var olması, o context'in current olduğu anlamına gelmez.

---

# Thread-Local Davranış

Bu fonksiyonun en kritik özelliği budur.

`eglGetCurrentContext` sonucu **thread'e özeldir**.

Örneğin:

```text
Thread A
    ↓
Context A current

Thread B
    ↓
Context B current
```

Thread A içinde:

```c
eglGetCurrentContext();
```

çağrılırsa:

```text
Context A
```

döner.

Thread B içinde çağrılırsa:

```text
Context B
```

döner.

---

# Başka Thread'de Context Aktifse Ne Olur?

Şöyle düşünelim:

```text
Thread A
    ↓
Context X current

Thread B
    ↓
Current context yok
```

Thread B:

```c
eglGetCurrentContext();
```

çağırırsa:

```text
EGL_NO_CONTEXT
```

döner.

Context X sistemde vardır ve Thread A üzerinde aktiftir.

Ama Thread B bunu kendi current context'i olarak görmez.

Çünkü fonksiyon:

> "Sistemde herhangi bir aktif context var mı?"

diye sormaz.

Şunu sorar:

> "BENİM thread'imde aktif context var mı?"

Bu ayrım çok önemlidir.

---

# Aynı Context İki Thread'de Aynı Anda Current Olabilir mi?

Hayır.

Bir `EGLContext` aynı anda yalnızca bir thread üzerinde current olabilir.

Örneğin:

```text
Thread A ───────► Context X

Thread B ──X────► Context X
```

Context X Thread A üzerinde current iken Thread B aynı context'i eşzamanlı şekilde current yapamaz.

Önce mevcut thread üzerindeki bağlantının bırakılması gerekir.

---

# Context Nasıl Bırakılır?

Genellikle:

```c
eglMakeCurrent(
    dpy,
    EGL_NO_SURFACE,
    EGL_NO_SURFACE,
    EGL_NO_CONTEXT
);
```

kullanılarak thread üzerindeki current context kaldırılabilir.

Sonrasında:

```c
eglGetCurrentContext();
```

çağırırsak:

```text
EGL_NO_CONTEXT
```

dönmesini bekleriz.

---

# Senaryo 1 — Context Başarıyla Current Yapılmış

```c
eglMakeCurrent(
    dpy,
    draw,
    read,
    ctx
);

EGLContext current =
    eglGetCurrentContext();
```

Sonuç:

```text
current == ctx
```

Bu normal ve beklenen durumdur.

---

# Senaryo 2 — Henüz `eglMakeCurrent` Çağrılmamış

Context oluşturduk:

```c
EGLContext ctx =
    eglCreateContext(...);
```

Ama:

```c
eglMakeCurrent(...)
```

çağrılmadı.

Şimdi:

```c
eglGetCurrentContext();
```

çağırırsak:

```text
EGL_NO_CONTEXT
```

dönebilir.

Çünkü:

> Context oluşturulmuş olsa bile current değildir.

---

# Senaryo 3 — Context Unbind Edilmiş

Önce:

```c
eglMakeCurrent(
    dpy,
    draw,
    read,
    ctx
);
```

ile bağladık.

Sonra:

```c
eglMakeCurrent(
    dpy,
    EGL_NO_SURFACE,
    EGL_NO_SURFACE,
    EGL_NO_CONTEXT
);
```

ile bağlantıyı kaldırdık.

Artık:

```c
eglGetCurrentContext();
```

sonucu:

```text
EGL_NO_CONTEXT
```

olur.

---

# Senaryo 4 — Context Başka Thread'de Current

```text
Thread A
    ↓
Context X current

Thread B
    ↓
eglGetCurrentContext()
```

Thread B'nin sonucu:

```text
EGL_NO_CONTEXT
```

olabilir.

Bu şu anlama gelmez:

> Context X yok.

Sadece şu anlama gelir:

> Context X, Thread B'nin current context'i değil.

---

# `EGL_NO_CONTEXT` Her Zaman Hata mı?

Hayır.

Bu fonksiyonda:

```text
EGL_NO_CONTEXT
```

çoğu zaman normal bir durum bilgisidir.

Örneğin:

- thread'e henüz context bağlanmamış olabilir,
- context unbind edilmiş olabilir,
- başka thread'de context aktif olabilir.

Dolayısıyla:

> `EGL_NO_CONTEXT` görmek tek başına EGL hatası olduğu anlamına gelmez.

Bu fonksiyonun çok önemli özelliklerinden biridir.

---

# `eglGetError()` ile İlişkisi

`eglGetCurrentContext` bir getter fonksiyonudur.

Mevcut state'i sorgular.

Normal kullanımında:

```c
EGLContext current =
    eglGetCurrentContext();
```

sonrasında `EGL_NO_CONTEXT` dönmesi:

> "Mutlaka hata oluştu."

anlamına gelmez.

Bu yüzden bunu `eglCreateContext` gibi fonksiyonlarla karıştırmamak gerekir.

Örneğin:

```text
eglCreateContext
      ↓
EGL_NO_CONTEXT
      ↓
Genellikle context oluşturma başarısızlığı

eglGetCurrentContext
      ↓
EGL_NO_CONTEXT
      ↓
Bu thread'de current context yok
```

Bu fark çok önemlidir.

---

# Parametre Olmamasının Mantığı

Hocamız:

> "Neden `eglGetCurrentContext` display parametresi almıyor?"

diye sorabilir.

Cevap:

> Çünkü fonksiyon belirli bir display üzerinde yeni bir işlem yapmıyor. Sadece çağrıyı yapan thread'in EGL current state'ini sorguluyor. Current context bilgisi zaten thread'e bağlı olarak tutulduğu için hangi display'in sorgulanacağını ayrıca vermemize gerek yok.

Yani:

```text
eglGetConfigs
→ Hangi display? → dpy gerekli.

eglCreateContext
→ Hangi display/config? → parametre gerekli.

eglGetCurrentContext
→ Bu thread'de şu anda ne current? → parametre gerekmez.
```

---

# `EGLContext`, `EGLSurface` ve `EGLDisplay` Farkı

Bu fonksiyonda özellikle bunları karıştırmamak gerekir.

```text
EGLDisplay
→ EGL'nin native display sistemiyle bağlantısı

EGLContext
→ OpenGL ES çalışma state'i / rendering context

EGLSurface
→ Render hedefi
```

`eglGetCurrentContext` sadece:

```text
EGLContext
```

döndürür.

Current surface'i öğrenmek için başka EGL getter fonksiyonları kullanılır.

---

# DRAW ve READ Surface İlişkisi

Bir context `eglMakeCurrent` ile bağlanırken genellikle:

```c
eglMakeCurrent(
    dpy,
    draw,
    read,
    ctx
);
```

şeklinde:

- bir draw surface,
- bir read surface,
- bir context

ile ilişkilendirilir.

Ancak:

```c
eglGetCurrentContext();
```

sadece context'i döndürür.

Şunları doğrudan döndürmez:

```text
draw surface
read surface
display
```

Dolayısıyla:

> Current context'i öğrenmek ile current surface'i öğrenmek aynı şey değildir.

---

# Doğrulama İçin Kullanım

Fonksiyonun pratik kullanım alanlarından biri, `eglMakeCurrent` sonrasında doğrulama yapmaktır.

Örneğin:

```c
if (!eglMakeCurrent(
        dpy,
        draw,
        read,
        ctx))
{
    // hata
}

EGLContext current =
    eglGetCurrentContext();

if (current != ctx)
{
    // Beklenen context current değil
}
```

Burada amaç:

> "Ben ctx context'ini current yapmaya çalıştım. Gerçekten current context bu mu?"

sorusuna cevap almaktır.

---

# Güvenli Kullanım Örneği

```c
#include <EGL/egl.h>
#include <stdio.h>

void perform_safe_rendering(
    EGLDisplay dpy,
    EGLSurface draw,
    EGLSurface read,
    EGLContext ctx)
{
    if (!eglMakeCurrent(
            dpy,
            draw,
            read,
            ctx))
    {
        EGLint err = eglGetError();

        fprintf(
            stderr,
            "eglMakeCurrent basarisiz. Hata: 0x%04X\n",
            err
        );

        return;
    }

    EGLContext current =
        eglGetCurrentContext();

    if (current == EGL_NO_CONTEXT)
    {
        fprintf(
            stderr,
            "Aktif context bulunamadi.\n"
        );

        return;
    }

    if (current != ctx)
    {
        fprintf(
            stderr,
            "Current context beklenen context degil.\n"
        );

        return;
    }

    printf(
        "Dogru context current durumda.\n"
    );

    // OpenGL ES çizimleri burada yapılabilir.

    eglMakeCurrent(
        dpy,
        EGL_NO_SURFACE,
        EGL_NO_SURFACE,
        EGL_NO_CONTEXT
    );
}
```

Buradaki süreç:

```text
eglMakeCurrent
      ↓
Context'i current yap

eglGetCurrentContext
      ↓
Current context'i oku

current == ctx ?
      ↓
EVET
      ↓
Render işlemlerine devam et
```

---

# `eglGetCurrentContext` Ne Yapmaz?

Fonksiyonu daha iyi anlamak için ne yapmadığını bilmek önemlidir.

`eglGetCurrentContext`:

- yeni context oluşturmaz,
- context'i current yapmaz,
- context'i destroy etmez,
- display seçmez,
- config seçmez,
- surface oluşturmaz,
- render komutu göndermez,
- context state'ini değiştirmez.

Sadece:

> Mevcut thread'in current context'ini sorgular.

---

# Fonksiyonun EGL Akışındaki Yeri

Genel EGL akışını düşünelim:

```text
eglGetDisplay
      ↓
eglInitialize
      ↓
eglChooseConfig / eglGetConfigs
      ↓
eglCreateContext
      ↓
eglCreateSurface
      ↓
eglMakeCurrent
      ↓
eglGetCurrentContext
      ↓
OpenGL ES çizimi
```

Burada:

```text
eglCreateContext
→ Context'i oluşturur.

eglMakeCurrent
→ Context'i thread'e bağlar.

eglGetCurrentContext
→ Hangi context'in bağlı olduğunu sorgular.
```

Bu üç fonksiyonun farkını bilmek önemlidir.

---

# `eglCreateContext` ile `eglGetCurrentContext` Farkı

```text
eglCreateContext
→ YENİ context oluşturur.

eglGetCurrentContext
→ Zaten current olan context'i getirir.
```

Birincisi oluşturma işlemi yapar.

İkincisi sadece sorgulama yapar.

---

# `eglMakeCurrent` ile `eglGetCurrentContext` Farkı

```text
eglMakeCurrent
→ State'i değiştirir.
→ Bir context'i current yapar.

eglGetCurrentContext
→ State'i değiştirmez.
→ Current olan context'i okur.
```

En kolay ayrım budur:

```text
Make → Ayarla
Get  → Oku
```

---

# Hocanın Sorabileceği Hızlı Sorular

## `eglGetCurrentContext` ne yapar?

Çağrıyı yapan thread üzerinde current olan `EGLContext` handle'ını döndürür.

---

## Parametre alıyor mu?

Hayır.

Fonksiyon prototipi:

```c
eglGetCurrentContext(void);
```

şeklindedir.

---

## Neden `EGLDisplay` parametresi almıyor?

Çünkü belirli bir display üzerinde işlem yapmıyor. Çağrıyı yapan thread'in current EGL state'ini sorguluyor.

---

## Context varsa mutlaka onu döndürür mü?

Hayır.

Context'in sadece oluşturulmuş olması yeterli değildir.

O context'in bu thread üzerinde **current** olması gerekir.

---

## Context'i current yapan fonksiyon hangisi?

```c
eglMakeCurrent
```

---

## `eglGetCurrentContext()` ne zaman `EGL_NO_CONTEXT` döndürür?

Bu thread üzerinde aktif/current context yoksa.

---

## Başka thread'de context varsa onu görür mü?

Hayır.

Fonksiyon thread-local davranır.

Her thread yalnızca kendi current context'ini görür.

---

## `EGL_NO_CONTEXT` mutlaka hata mıdır?

Hayır.

Bu thread'de current context olmadığını ifade eden normal bir sonuç olabilir.

---

## `eglGetCurrentContext` state değiştirir mi?

Hayır.

Salt-okuma yapan bir getter fonksiyonudur.

---

## `eglCreateContext` ile farkı ne?

```text
eglCreateContext
→ Context oluşturur.

eglGetCurrentContext
→ Current context'i sorgular.
```

---

## `eglMakeCurrent` ile farkı ne?

```text
eglMakeCurrent
→ Context'i current hale getirir.

eglGetCurrentContext
→ Hangi context'in current olduğunu söyler.
```

---

# Hocanın Sorabileceği Senaryo 1

```c
EGLContext ctx =
    eglCreateContext(...);

EGLContext current =
    eglGetCurrentContext();
```

`eglMakeCurrent` çağrılmamışsa ne olur?

Cevap:

> `ctx` oluşturulmuş olsa bile current değildir. Bu thread üzerinde başka current context yoksa `eglGetCurrentContext()` `EGL_NO_CONTEXT` döndürür.

---

# Hocanın Sorabileceği Senaryo 2

```c
eglMakeCurrent(
    dpy,
    draw,
    read,
    ctx
);

EGLContext current =
    eglGetCurrentContext();
```

Ne beklenir?

Cevap:

```text
current == ctx
```

olması beklenir.

---

# Hocanın Sorabileceği Senaryo 3

```c
eglMakeCurrent(
    dpy,
    EGL_NO_SURFACE,
    EGL_NO_SURFACE,
    EGL_NO_CONTEXT
);

EGLContext current =
    eglGetCurrentContext();
```

Ne döner?

Cevap:

```text
EGL_NO_CONTEXT
```

Çünkü context thread'den ayrılmıştır.

---

# Hocanın Sorabileceği Senaryo 4

```text
Thread A → Context X current
Thread B → Current context yok
```

Thread B:

```c
eglGetCurrentContext();
```

çağırırsa?

Cevap:

```text
EGL_NO_CONTEXT
```

Çünkü Context X yalnızca Thread A üzerinde current'dır.

---

# En Kritik Şema

```text
                 eglGetCurrentContext()
                           │
                           ▼
                  "Bu thread'de
                   current context
                     hangisi?"
                           │
              ┌────────────┴────────────┐
              │                         │
              ▼                         ▼
       Context mevcut            Context mevcut değil
       ve current                veya current değil
              │                         │
              ▼                         ▼
        EGLContext                  EGL_NO_CONTEXT
```

---

# Üç Fonksiyonu Birlikte Ezberleme

```text
eglCreateContext
      ↓
CONTEXT OLUŞTUR

eglMakeCurrent
      ↓
CONTEXT'İ THREAD'E BAĞLA

eglGetCurrentContext
      ↓
ŞU AN HANGİ CONTEXT BAĞLI?
```

Bu üçlü mantığı bilirsek konu çok daha kolay anlaşılır.

---

# 30 Saniyelik Final Özeti

> `eglGetCurrentContext`, çağrıyı yapan thread üzerinde o anda current olan `EGLContext` handle'ını döndüren parametresiz bir getter fonksiyonudur. Yeni context oluşturmaz ve EGL state'ini değiştirmez. Eğer daha önce `eglMakeCurrent` ile bir context bu thread'e başarıyla bağlanmışsa o context döner. Eğer bu thread üzerinde current context yoksa `EGL_NO_CONTEXT` döner. Fonksiyon thread-local çalıştığı için başka bir thread üzerinde current olan context'i göremez. Bu nedenle bir context'in sadece oluşturulmuş olması yeterli değildir; `eglGetCurrentContext` tarafından döndürülmesi için ilgili thread üzerinde current olması gerekir.

---

# Özellikle Öğrenilmesi Gereken 5 Ayrım

1. **Context var** ↔ **Context current**
2. `eglCreateContext` ↔ `eglGetCurrentContext`
3. `eglMakeCurrent` ↔ `eglGetCurrentContext`
4. **Bu thread'in context'i** ↔ **Başka thread'in context'i**
5. `EGL_NO_CONTEXT` ↔ gerçek API hatası

---

# Tek Satırlık Ezber

```text
eglGetCurrentContext =
"Benim thread'imde şu anda hangi context aktif?"
```
