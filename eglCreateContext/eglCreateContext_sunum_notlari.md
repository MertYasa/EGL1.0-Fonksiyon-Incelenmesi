# `eglCreateContext` – Hocaya Anlatım Metni

Aşağıdaki metin, `eglCreateContext` fonksiyonunu özellikle **parametreler ve farklı senaryolar** üzerinden anlaşılır şekilde anlatmak için hazırlanmıştır.

---

## Fonksiyon Prototipi

```c
EGLContext eglCreateContext(
    EGLDisplay dpy,
    EGLConfig config,
    EGLContext share_context,
    const EGLint *attrib_list
);
```

`eglCreateContext` fonksiyonunun temel görevi bir **rendering context**, yani çizim bağlamı oluşturmaktır.

Context bir pencere veya ekranda gördüğümüz görüntü değildir. Daha çok **OpenGL ES'in çalışma ortamı** olarak düşünülebilir.

Örneğin OpenGL tarafında hangi texture'ın bağlı olduğu, hangi ayarların aktif olduğu ve çeşitli grafik durumları context ile ilişkilidir.

Ancak context oluşturulduğu anda ekrana bir şey çizilmiş olmaz.

Kabaca süreç şöyledir:

```text
EGLDisplay
     ↓
EGLConfig
     ↓
eglCreateContext()
     ↓
EGLContext
     ↓
eglMakeCurrent()
     ↓
OpenGL ES komutları
```

Yani:

> `eglCreateContext` çalışma ortamını oluşturur, `eglMakeCurrent` ise o çalışma ortamını mevcut thread'e ve render yüzeylerine bağlar.

---

# 1. Parametre: `dpy`

```c
EGLDisplay dpy
```

`dpy`, context'in **hangi EGL display bağlantısına ait olduğunu** belirtir.

Burada display denildiğinde sadece fiziksel monitörü düşünmemek gerekir.

Daha doğru ifade:

> EGL ile alttaki native görüntüleme sistemi arasındaki bağlantıyı temsil eden handle'dır.

Örneğin:

```c
EGLDisplay display;

display = eglGetDisplay(...);
eglInitialize(display, ...);
```

Daha sonra:

```c
eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
```

şeklinde aynı display kullanılır.

## Senaryo 1 — Doğru display

```c
eglInitialize(dpy, ...);

eglCreateContext(
    dpy,
    config,
    EGL_NO_CONTEXT,
    NULL
);
```

Display geçerli ve initialize edilmişse, diğer parametrelerde problem yoksa context oluşturulabilir.

## Senaryo 2 — Geçersiz display

```c
eglCreateContext(
    EGL_NO_DISPLAY,
    config,
    EGL_NO_CONTEXT,
    NULL
);
```

Bu durumda:

```text
Dönüş:
EGL_NO_CONTEXT

Hata:
EGL_BAD_DISPLAY
```

## Senaryo 3 — Display var ama initialize edilmemiş

```c
EGLDisplay dpy = eglGetDisplay(...);
```

ancak:

```c
eglInitialize(dpy, ...);
```

çağrılmamışsa context oluşturma işlemi başarısız olur.

Hata:

```text
EGL_NOT_INITIALIZED
```

### `EGL_BAD_DISPLAY` ve `EGL_NOT_INITIALIZED` farkı

```text
EGL_BAD_DISPLAY
    ↓
Verilen display handle geçersiz.

EGL_NOT_INITIALIZED
    ↓
Display geçerli olabilir ama EGL bu display üzerinde başlatılmamış.
```

Kısaca biri **handle problemi**, diğeri **başlatma problemi**dir.

---

# 2. Parametre: `config`

```c
EGLConfig config
```

`config`, seçilmiş grafik konfigürasyonunu belirtir.

`dpy` ile farkı çok önemlidir.

```text
dpy
→ Hangi EGL görüntüleme bağlantısında çalışıyorum?

config
→ Hangi grafik konfigürasyonuyla çalışıyorum?
```

`EGLConfig` içinde örneğin:

- renk buffer özellikleri,
- depth buffer özellikleri,
- stencil özellikleri,
- desteklenen surface türleri,
- render edilebilecek API türleri

gibi özellikler bulunabilir.

Genellikle config şu şekilde seçilir:

```c
EGLConfig config;

eglChooseConfig(
    dpy,
    ...,
    &config,
    ...
);
```

## Senaryo 1 — Geçerli config

```c
eglCreateContext(
    dpy,
    config,
    EGL_NO_CONTEXT,
    NULL
);
```

Config geçerliyse işlem devam eder.

## Senaryo 2 — Geçersiz config

```c
eglCreateContext(
    dpy,
    invalidConfig,
    EGL_NO_CONTEXT,
    NULL
);
```

Sonuç:

```text
EGL_BAD_CONFIG
```

### Önemli ayrım

`EGLConfig`, context'in kendi framebuffer'ı değildir.

Daha doğru ifade:

> Context belirli bir `EGLConfig` kullanılarak oluşturulur ve daha sonra uyumlu `EGLSurface` üzerinde rendering yapılır.

---

# 3. Parametre: `share_context`

```c
EGLContext share_context
```

Bu parametre yeni oluşturulan context'in mevcut başka bir context ile **paylaşılabilir grafik kaynaklarını** paylaşıp paylaşmayacağını belirler.

İki temel senaryo vardır.

## Senaryo 1 — `EGL_NO_CONTEXT`

```c
eglCreateContext(
    dpy,
    config,
    EGL_NO_CONTEXT,
    NULL
);
```

Burada yeni context başka bir context'in paylaşım grubuna dahil edilmez.

Mantıksal olarak:

```text
Context A
   Texture A

Context B
   Texture B
```

Context'ler birbirinden bağımsızdır.

## Senaryo 2 — Başka bir context paylaşmak

Önce:

```c
EGLContext ctx1 =
    eglCreateContext(
        dpy,
        config,
        EGL_NO_CONTEXT,
        NULL
    );
```

Daha sonra:

```c
EGLContext ctx2 =
    eglCreateContext(
        dpy,
        config,
        ctx1,
        NULL
    );
```

Burada `ctx1`, `share_context` parametresidir.

Anlamı:

> `ctx2`, `ctx1` ile paylaşılabilir OpenGL ES nesnelerini ortak kullanabilsin.

Örneğin bazı texture veya buffer nesneleri yeniden oluşturulmadan kullanılabilir.

Ancak çok önemli bir ayrım vardır:

```text
Shared object ≠ Shared state
```

Yani context'lerin kendi OpenGL state'leri ayrıdır.

Mantıksal yapı:

```text
Context A
├── kendi state'i
└── shared resources ──────┐
                           │
Context B                  │
├── kendi state'i          │
└── shared resources ──────┘
```

Dolayısıyla:

> İki context paylaşım yapıyor diye her şey ortak olmaz.

Hangi nesnelerin paylaşılabildiği kullanılan OpenGL ES sürümüne ve API kurallarına bağlıdır.

---

## Farklı Display Senaryosu

Örneğin:

```text
Context A → Display 1

Yeni Context → Display 2
```

ve:

```c
eglCreateContext(
    display2,
    config2,
    contextA,
    NULL
);
```

çağrılırsa context'ler paylaşım açısından uyumsuz olabilir.

Bu durumda:

```text
EGL_BAD_MATCH
```

hatası alınabilir.

---

## `EGL_BAD_CONTEXT` ile `EGL_BAD_MATCH` farkı

### Geçersiz context handle

```c
eglCreateContext(
    dpy,
    config,
    invalidContext,
    NULL
);
```

Sonuç:

```text
EGL_BAD_CONTEXT
```

Anlamı:

> Verilen context handle geçerli değil.

### Context geçerli ama uyumsuz

Context gerçekten vardır ancak yeni context ile paylaşım yapamıyorsa:

```text
EGL_BAD_MATCH
```

Anlamı:

> Context geçerli ama bu iki context birbirleriyle paylaşım açısından uyumlu değil.

Kısaca:

```text
EGL_BAD_CONTEXT
→ Nesnenin kendisi geçersiz.

EGL_BAD_MATCH
→ Nesneler geçerli ama birbirleriyle uyumsuz.
```

---

# 4. Parametre: `attrib_list`

```c
const EGLint *attrib_list
```

Bu parametre context oluşturulurken istenen ek özellikleri belirtir.

Genel yapı:

```c
const EGLint attribs[] = {
    KEY, VALUE,
    KEY, VALUE,
    EGL_NONE
};
```

Liste:

```text
Attribute
Value
Attribute
Value
...
EGL_NONE
```

şeklinde ilerler.

`EGL_NONE`, listenin sonunu belirtir.

---

## EGL 1.0 açısından önemli durum

EGL 1.0 core spesifikasyonunda `eglCreateContext` için tanımlanmış standart context attribute'u yoktur.

Bu yüzden normal EGL 1.0 kullanımı:

```c
eglCreateContext(
    dpy,
    config,
    EGL_NO_CONTEXT,
    NULL
);
```

veya:

```c
const EGLint attribs[] = {
    EGL_NONE
};
```

şeklindedir.

---

# `EGL_CONTEXT_CLIENT_VERSION`

Daha sonraki EGL sürümlerinde örneğin:

```c
const EGLint attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};
```

kullanılabilir.

Burada:

```text
EGL_CONTEXT_CLIENT_VERSION → 2
```

OpenGL ES 2.x context talep edildiğini ifade eder.

Ancak önemli nokta:

> `EGL_CONTEXT_CLIENT_VERSION`, saf EGL 1.0 core özelliği değildir.

Bu özellik daha sonraki EGL sürümlerinde, özellikle EGL 1.3 ile OpenGL ES sürüm seçimi açısından standart hale gelmiştir.

---

# Dört Parametreyi Birbirinden Ayırmanın En Kolay Yolu

| Parametre | Cevap verdiği soru |
|---|---|
| `dpy` | Nerede context oluşturuyorum? |
| `config` | Hangi grafik konfigürasyonuyla oluşturuyorum? |
| `share_context` | Kimle kaynak paylaşacağım? |
| `attrib_list` | Context için hangi ek özellikleri istiyorum? |

Örneğin:

```c
eglCreateContext(
    dpy,
    config,
    shared,
    attribs
);
```

Türkçe olarak:

> `dpy` üzerindeki EGL ortamında, `config` konfigürasyonunu kullanarak, gerekirse `shared` context ile paylaşılabilir kaynakları ortak kullanan ve `attribs` içinde belirtilmiş özelliklere sahip yeni bir rendering context oluştur.

---

# Örnek Senaryolar

## Senaryo 1

```c
EGLContext ctx =
    eglCreateContext(
        dpy,
        config,
        EGL_NO_CONTEXT,
        NULL
    );
```

Burada:

- `dpy`: Hangi EGLDisplay üzerinde çalışılacağını belirler.
- `config`: Grafik konfigürasyonunu belirler.
- `EGL_NO_CONTEXT`: Başka context ile paylaşım yapılmaz.
- `NULL`: EGL 1.0 için ek context attribute'u talep edilmez.

---

## Senaryo 2

```c
EGLContext ctx2 =
    eglCreateContext(
        dpy,
        config,
        ctx1,
        NULL
    );
```

Bir önceki senaryodan farkı:

```text
EGL_NO_CONTEXT
yerine
ctx1
```

verilmiş olmasıdır.

Böylece `ctx2`, `ctx1` ile paylaşılabilir grafik kaynaklarının paylaşım grubuna katılabilir.

---

## Senaryo 3

```c
eglCreateContext(
    EGL_NO_DISPLAY,
    config,
    EGL_NO_CONTEXT,
    NULL
);
```

Hata:

```text
EGL_BAD_DISPLAY
```

---

## Senaryo 4

```c
eglCreateContext(
    dpy,
    invalidConfig,
    EGL_NO_CONTEXT,
    NULL
);
```

Hata:

```text
EGL_BAD_CONFIG
```

---

## Senaryo 5

```c
eglCreateContext(
    dpy,
    config,
    invalidContext,
    NULL
);
```

Hata:

```text
EGL_BAD_CONTEXT
```

---

## Senaryo 6

```c
eglCreateContext(
    display2,
    config2,
    contextFromDisplay1,
    NULL
);
```

Context geçerli olsa bile paylaşım için uyumsuzsa:

```text
EGL_BAD_MATCH
```

oluşabilir.

---

# `eglCreateContext` Sonrasında Ne Olur?

Context oluşturulduktan sonra henüz ekrana çizim yapılmaz.

Bir sonraki önemli işlem:

```c
eglMakeCurrent(
    dpy,
    drawSurface,
    readSurface,
    ctx
);
```

Bu fonksiyon context'i mevcut thread ve uygun surface'lerle ilişkilendirir.

Süreç:

```text
eglCreateContext
       ↓
Context'i oluştur

eglMakeCurrent
       ↓
Context'i kullanıma bağla

OpenGL ES komutları
       ↓
Çizim yap
```

Basit benzetme:

> `eglCreateContext` motoru oluşturur, `eglMakeCurrent` ise motoru araca takıp sürücünün kontrolüne verir.

---

# Context ile Surface Farkı

`EGLContext` ve `EGLSurface` aynı şey değildir.

```text
EGLContext
→ Grafik işlemlerinin çalışma ortamı ve state'i

EGLSurface
→ Render sonucunun ilişkilendirildiği çizim yüzeyi
```

Kısa hali:

```text
Context → Nasıl çiziyorum?
Surface → Nereye çiziyorum?
```

Bu ikisi `eglMakeCurrent` ile ilişkilendirilir.

---

# Thread Konusu

Bir context aynı anda birden fazla thread'de current olarak kullanılamaz.

Örneğin:

```text
Thread A ──────► Context X

Thread B ──X──► Context X
```

Context X, Thread A üzerinde current ise Thread B aynı anda onu current yapamaz.

Önce mevcut thread üzerindeki bağlantının bırakılması gerekir.

Burada önemli ayrım:

> `share_context` kullanmak, aynı context'i iki thread'de kullanmak değildir.

Daha doğru kullanım:

```text
Thread 1 → Context A
                │
                │ Shared resources
                │
Thread 2 → Context B
```

Yani thread'ler farklı context'ler kullanabilir ancak context'ler bazı grafik kaynaklarını paylaşabilir.

---

# Context Paylaşımı Neden Kullanılır?

Örneğin çok ekranlı bir sistemde:

```text
PFD
ND
EICAS
```

gibi farklı görüntüler bulunabilir.

Farklı context'lerde aynı:

- texture,
- font,
- sembol,
- grafik kaynağı

gereksiz yere tekrar oluşturulmak istenmeyebilir.

Uygun kaynakların paylaşılması bellek kullanımını azaltabilir.

Ancak:

> Shared context kullanmak bütün VRAM'in ortak olduğu anlamına gelmez.

Sadece kullanılan API'nin paylaşılabilir olarak tanımladığı nesneler paylaşılır.

---

# Fonksiyonun Başarı ve Hata Durumu

Fonksiyon başarılı olursa geçerli bir:

```c
EGLContext
```

handle döner.

Başarısız olursa:

```c
EGL_NO_CONTEXT
```

döner.

Sonrasında:

```c
eglGetError();
```

ile hata kodu okunur.

Örnek:

```c
EGLContext ctx =
    eglCreateContext(...);

if (ctx == EGL_NO_CONTEXT) {
    EGLint error = eglGetError();
}
```

---

# Hata Tablosu

| Hata | Anlamı |
|---|---|
| `EGL_BAD_DISPLAY` | Display handle geçersiz |
| `EGL_NOT_INITIALIZED` | Display var ama EGL başlatılmamış |
| `EGL_BAD_CONFIG` | Config geçersiz |
| `EGL_BAD_CONTEXT` | `share_context` olarak verilen context geçersiz |
| `EGL_BAD_MATCH` | Nesneler geçerli ancak birbirleriyle uyumsuz |
| `EGL_BAD_ATTRIBUTE` | Geçersiz veya desteklenmeyen attribute |
| `EGL_BAD_ALLOC` | Gerekli kaynak veya bellek ayrılamadı |

---

# Hocanın Sorabileceği Hızlı Sorular

## Context nedir?

Rendering API'nin çalışması için gereken state ve ilişkili grafik kaynaklarının bulunduğu çalışma ortamıdır.

## `dpy` ile `config` arasındaki fark nedir?

`dpy`, EGL görüntüleme bağlantısını; `config`, bu display üzerinde seçilen grafik konfigürasyonunu temsil eder.

## `EGL_NO_CONTEXT` verirsem ne olur?

Yeni context başka bir context'in paylaşım grubuna dahil edilmez.

## `share_context` verirsem bütün state ortak olur mu?

Hayır. Context state'leri ayrıdır. Yalnızca API'nin paylaşılabilir olarak tanımladığı nesneler paylaşılabilir.

## `EGL_BAD_CONTEXT` ile `EGL_BAD_MATCH` arasındaki fark nedir?

`EGL_BAD_CONTEXT`, context handle'ın geçersiz olduğunu belirtir.

`EGL_BAD_MATCH`, context'lerin geçerli olduğunu ancak birbirleriyle uyumlu olmadığını belirtir.

## `NULL` attrib_list ne demek?

EGL 1.0 açısından standart bir ek context attribute'u talep edilmediğini ifade eder.

## `EGL_CONTEXT_CLIENT_VERSION, 2` EGL 1.0'da var mı?

Hayır. Saf EGL 1.0 core spesifikasyonunda standart değildir. Daha sonraki EGL sürümlerinde OpenGL ES sürümü seçmek için standart hale gelmiştir.

## Context oluşturulduktan hemen sonra OpenGL ES komutu çalıştırabilir miyim?

Genellikle hayır. Önce uygun surface'lerle birlikte `eglMakeCurrent` kullanılarak context current hale getirilmelidir.

## Context ile Surface aynı şey mi?

Hayır.

```text
Context → Nasıl çiziyorum?
Surface → Nereye çiziyorum?
```

---

# 30 Saniyelik Final Özeti

> `eglCreateContext` bir OpenGL ES rendering context oluşturur. Dört temel parametresi vardır. `dpy` hangi EGL display üzerinde çalışacağımızı, `config` hangi grafik konfigürasyonunu kullanacağımızı, `share_context` başka bir context ile paylaşılabilir grafik kaynaklarının ortak kullanılıp kullanılmayacağını, `attrib_list` ise varsa context oluşturma özelliklerini belirler. EGL 1.0'da standart context attribute'u bulunmadığı için `attrib_list` normalde `NULL` veya `EGL_NONE` olur. Fonksiyon başarılı olduğunda `EGLContext`, başarısız olduğunda `EGL_NO_CONTEXT` döner. Oluşturulan context tek başına çizim yapmaz; OpenGL ES komutlarını çalıştırmadan önce `eglMakeCurrent` ile thread ve uygun surface'lerle ilişkilendirilmesi gerekir.

---

# En Kritik Şema

```text
                eglCreateContext
                       │
        ┌──────────────┼──────────────┬───────────────┐
        │              │              │               │
       dpy           config       share_context   attrib_list
        │              │              │               │
        ▼              ▼              ▼               ▼
     NEREDE?      HANGİ GRAFİK     KİMLE          HANGİ EK
                  YAPISIYLA?      PAYLAŞACAĞIM?   ÖZELLİKLER?
        │              │              │               │
        ▼              ▼              ▼               ▼
 BAD_DISPLAY      BAD_CONFIG     BAD_CONTEXT     BAD_ATTRIBUTE
 NOT_INITIALIZED                 BAD_MATCH
```

## Özellikle öğrenilmesi gereken 5 ayrım

1. `dpy` ↔ `config`
2. `context` ↔ `surface`
3. `EGL_NO_CONTEXT` ↔ geçerli `share_context`
4. `EGL_BAD_CONTEXT` ↔ `EGL_BAD_MATCH`
5. EGL 1.0 ↔ EGL 1.3 attribute farkı
