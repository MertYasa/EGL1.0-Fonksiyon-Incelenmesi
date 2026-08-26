# `eglGetConfigs` – Hocaya Anlatım Metni

## Fonksiyon Prototipi

```c
EGLBoolean eglGetConfigs(
    EGLDisplay dpy,
    EGLConfig *configs,
    EGLint config_size,
    EGLint *num_config
);
```

Hocam, `eglGetConfigs` fonksiyonunun temel amacı, belirli bir `EGLDisplay` üzerinde mevcut olan **EGLConfig konfigürasyonlarını elde etmektir**.

Bu fonksiyon yeni bir config oluşturmaz.

Var olan config'leri bize verir.

Yani bunu bir envanter sorgusu gibi düşünebiliriz:

```text
EGLDisplay
    │
    ▼
Sistemin desteklediği EGLConfig'ler
    │
    ▼
eglGetConfigs()
    │
    ├── Kaç tane config var?
    │
    └── Bu config'lerin handle'larını bana ver
```

Burada önemli nokta şudur:

> `eglGetConfigs` filtreleme yapmaz. Sistemde mevcut olan config'leri listeler.

Belirli özelliklere sahip bir config aramak istiyorsak, örneğin:

- 24-bit depth buffer,
- 8-bit alpha,
- window surface desteği,
- belirli renk formatı

gibi şartlarımız varsa `eglChooseConfig` daha uygundur.

Kısaca:

```text
eglGetConfigs
→ Bana mevcut config'leri göster.

eglChooseConfig
→ Benim şartlarıma uyan config'leri bul.
```

---

# Mental Model

Fonksiyonun mantığını şöyle düşünebiliriz:

```text
Native Display
     │
     ▼
EGLDisplay
     │
     ▼
+----------------------+
| EGLConfig Havuzu     |
+----------------------+
     │
     ├── EGLConfig #1
     │     RGB565
     │     Depth 16
     │
     ├── EGLConfig #2
     │     RGBA8888
     │     Depth 24
     │
     └── EGLConfig #N
             ...
```

`eglGetConfigs` bu havuza bakıp bize config handle'larını verir.

Ancak:

> Config'in bütün özelliklerini doğrudan `eglGetConfigs` söylemez.

Elimize gelen her `EGLConfig` için daha sonra:

```c
eglGetConfigAttrib(...)
```

kullanarak özelliklerini sorgulayabiliriz.

---

# 1. Parametre: `dpy`

```c
EGLDisplay dpy
```

İlk parametre hangi EGL display üzerindeki config'leri sorguladığımızı belirtir.

Bu parametreyi şöyle düşünebiliriz:

> "Hangi EGL ortamının config listesini istiyorum?"

Genellikle önce:

```c
EGLDisplay dpy = eglGetDisplay(...);
```

ile display elde edilir.

Ardından:

```c
eglInitialize(dpy, ...);
```

ile başlatılır.

Sonrasında:

```c
eglGetConfigs(dpy, ...);
```

çağrılabilir.

---

## Senaryo 1 — Geçerli ve initialize edilmiş display

Örneğin:

```c
EGLDisplay dpy = eglGetDisplay(...);
eglInitialize(dpy, ...);

eglGetConfigs(
    dpy,
    configs,
    config_size,
    &num_config
);
```

Diğer parametrelerde problem yoksa sorgu başarılı olabilir.

Fonksiyon:

```text
EGL_TRUE
```

döner.

---

## Senaryo 2 — `EGL_NO_DISPLAY`

```c
eglGetConfigs(
    EGL_NO_DISPLAY,
    configs,
    config_size,
    &num_config
);
```

Bu durumda geçerli bir display verilmediği için fonksiyon başarısız olur.

```text
Dönüş:
EGL_FALSE

Hata:
EGL_BAD_DISPLAY
```

---

## Senaryo 3 — Geçersiz display handle

Display parametresine geçerli olmayan bir handle verilirse yine:

```text
EGL_BAD_DISPLAY
```

hatası alınır.

---

## Senaryo 4 — Display geçerli ama initialize edilmemiş

Örneğin:

```c
EGLDisplay dpy = eglGetDisplay(...);
```

elde ettik ama:

```c
eglInitialize(dpy, ...);
```

çağırmadık.

Sonra:

```c
eglGetConfigs(
    dpy,
    configs,
    config_size,
    &num_config
);
```

çağırırsak:

```text
EGL_NOT_INITIALIZED
```

hatası oluşur.

---

# `EGL_BAD_DISPLAY` ile `EGL_NOT_INITIALIZED` farkı

Bu ayrım önemlidir.

```text
EGL_BAD_DISPLAY
      ↓
Verdiğim display handle geçersiz.

EGL_NOT_INITIALIZED
      ↓
Display olabilir ama EGL bu display üzerinde henüz başlatılmamış.
```

Yani:

> `BAD_DISPLAY` → handle problemi

> `NOT_INITIALIZED` → başlatma problemi

---

# 2. Parametre: `configs`

```c
EGLConfig *configs
```

Bu parametre çok önemlidir.

`configs`, EGL'nin bulduğu config handle'larını yazacağı **dizinin adresidir**.

Örneğin:

```c
EGLConfig configs[10];
```

oluşturduysak:

```c
eglGetConfigs(
    dpy,
    configs,
    10,
    &num_config
);
```

dediğimizde EGL bu diziye en fazla 10 tane `EGLConfig` handle'ı yazabilir.

Bunu şöyle düşünebiliriz:

```text
EGL'nin config havuzu

Config #1
Config #2
Config #3
Config #4
Config #5
    │
    ▼
eglGetConfigs()
    │
    ▼
configs[]
```

Yani `configs`:

> "Config'leri nereye yazacaksın?"

sorusunun cevabıdır.

---

# Çok Önemli: `configs = NULL`

Fonksiyonun en önemli özelliklerinden biri budur.

Şöyle çağırabiliriz:

```c
EGLint num_config;

eglGetConfigs(
    dpy,
    NULL,
    0,
    &num_config
);
```

Burada EGL'ye şunu söylüyoruz:

> "Config handle'larını şu anda bana verme. Önce sadece kaç tane config olduğunu söyle."

Bu durumda `configs` dizisine hiçbir şey yazılmaz.

Sadece:

```c
num_config
```

içine toplam config sayısı yazılır.

Örneğin sistemde 17 config varsa:

```text
num_config = 17
```

olabilir.

Bu özellik bize **iki adımlı sorgu** yapma imkânı sağlar.

---

# 3. Parametre: `config_size`

```c
EGLint config_size
```

Bu parametre:

> `configs` dizisine en fazla kaç tane `EGLConfig` yazılabileceğini belirtir.

Önemli nokta:

`config_size`, sistemde kaç config olduğunu söylemez.

Bu parametreyi **biz veriyoruz**.

Yani:

```text
config_size
→ Benim ayırdığım dizinin kapasitesi.

num_config
→ EGL'nin bana bildirdiği sonuç.
```

Bu ikisi kesinlikle karıştırılmamalıdır.

---

# Senaryo: `config_size = 10`

Örneğin:

```c
EGLConfig configs[10];
EGLint num_config;

eglGetConfigs(
    dpy,
    configs,
    10,
    &num_config
);
```

Burada diyoruz ki:

> "`configs` dizisine en fazla 10 tane config yazabilirsin."

---

# Senaryo A — Sistemde 6 config var

Bizim kapasitemiz:

```text
config_size = 10
```

Sistemde:

```text
6 config
```

var.

Sonuç:

```text
configs[0] → Config 1
configs[1] → Config 2
...
configs[5] → Config 6

num_config = 6
```

Dizinin kalan kısmına config yazılmaz.

---

# Senaryo B — Sistemde 10 config var

```text
config_size = 10
Sistemde = 10
```

Tamamı diziye sığar.

```text
num_config = 10
```

---

# Senaryo C — Sistemde 25 config var ama dizimiz 10 elemanlık

```c
EGLConfig configs[10];

eglGetConfigs(
    dpy,
    configs,
    10,
    &num_config
);
```

Sistemde:

```text
25 config
```

olsun.

Ama bizim kapasitemiz:

```text
10
```

EGL sadece dizinin kapasitesi kadar config handle'ı döndürebilir.

Yani:

```text
Config #1
Config #2
...
Config #10
```

alınır.

Kalan config'ler bu çağrıda diziye yazılmaz.

Burada önemli fikir:

> `config_size`, buffer taşmasını engelleyen kapasite sınırıdır.

---

# `configs` ile `config_size` ilişkisi

Bu iki parametre birlikte düşünülmelidir.

```text
configs
   ↓
Config'lerin yazılacağı yer.

config_size
   ↓
O yere en fazla kaç config yazılabilir?
```

Örneğin:

```c
EGLConfig configs[5];
```

oluşturduysak:

```c
config_size = 5;
```

mantıklıdır.

Çünkü EGL'ye:

> "Benim sana ayırdığım alan 5 elemanlık."

diyoruz.

---

# 4. Parametre: `num_config`

```c
EGLint *num_config
```

Bu bir **output parametresidir**.

Yani fonksiyona bilgi vermekten çok, fonksiyonun bize bilgi vermesi için kullanılır.

Örneğin:

```c
EGLint num_config = 0;
```

tanımlarız ve:

```c
eglGetConfigs(
    dpy,
    configs,
    config_size,
    &num_config
);
```

şeklinde adresini veririz.

Fonksiyon işlem sonucunda elde ettiği sayıyı bu değişkene yazar.

---

# `num_config` neyi ifade eder?

Bu, `configs` parametresine göre değişir.

## Durum 1 — `configs != NULL`

Örneğin:

```c
eglGetConfigs(
    dpy,
    configs,
    10,
    &num_config
);
```

Burada `num_config`, bu çağrıda elde edilen config sayısını bildirir.

---

## Durum 2 — `configs == NULL`

```c
eglGetConfigs(
    dpy,
    NULL,
    0,
    &num_config
);
```

Burada config'leri istemiyoruz.

Sadece:

> "Sistemde toplam kaç tane config var?"

diye soruyoruz.

Dolayısıyla `num_config` toplam config sayısını öğrenmek için kullanılır.

---

# `config_size` ile `num_config` Arasındaki Fark

Bu fonksiyonun en kritik ayrımlarından biridir.

```text
config_size
     ↓
INPUT
     ↓
Ben EGL'ye söylüyorum.
"Dizimin kapasitesi bu kadar."

num_config
     ↓
OUTPUT
     ↓
EGL bana söylüyor.
"Bu kadar config elde edildi / mevcut."
```

Başka bir ifadeyle:

```text
config_size = kapasite
num_config  = sonuç
```

Bunu bilirsek fonksiyonun büyük kısmı çözülür.

---

# Dört Parametreyi Tek Cümlede Ayırmak

| Parametre | Sorduğu soru |
|---|---|
| `dpy` | Hangi EGLDisplay'in config'lerini istiyorum? |
| `configs` | Config handle'larını nereye yazacaksın? |
| `config_size` | Bu diziye en fazla kaç config yazabilirsin? |
| `num_config` | Sonuçta kaç config bulundu/yazıldı? |

Bunu daha kısa ezberlemek istersek:

```text
dpy
→ NEREDEN?

configs
→ NEREYE?

config_size
→ EN FAZLA KAÇ?

num_config
→ SONUÇ KAÇ?
```

---

# En Önemli Kullanım: İki Adımlı Sorgu

`eglGetConfigs` genellikle iki aşamalı kullanılabilir.

## 1. Adım — Önce sayıyı öğren

```c
EGLint total_configs = 0;

eglGetConfigs(
    dpy,
    NULL,
    0,
    &total_configs
);
```

Örneğin sonuç:

```text
total_configs = 17
```

olsun.

Bu aşamada henüz config handle'larını almadık.

Sadece kaç tane olduklarını öğrendik.

---

# 2. Adım — Yeterli alan ayır

Artık 17 tane config olduğunu bildiğimiz için:

```c
EGLConfig *configs =
    malloc(total_configs * sizeof(EGLConfig));
```

ile yeterli alan ayırabiliriz.

Sonra:

```c
eglGetConfigs(
    dpy,
    configs,
    total_configs,
    &total_configs
);
```

çağırabiliriz.

Böylece:

```text
1. çağrı
   ↓
Kaç config var?

2. çağrı
   ↓
Config'leri getir.
```

mantığı oluşur.

---

# Neden İki Adımlı Kullanıyoruz?

Çünkü baştan sistemde kaç config olduğunu bilmiyoruz.

Örneğin rastgele:

```c
EGLConfig configs[100];
```

oluşturmak mümkündür.

Ancak bu yaklaşım:

- gereksiz bellek ayırabilir,
- ihtiyaçtan küçük kapasite seçilirse tüm config'leri alamayabilir,
- sistemdeki gerçek config sayısını bilmeden tahmin yapmamıza neden olur.

İki adımlı yaklaşımda ise:

```text
Önce sayıyı öğren
        ↓
Tam gereken kadar alan ayır
        ↓
Config'leri getir
```

şeklinde daha kontrollü ilerleriz.

---

# Tam Örnek

```c
EGLBoolean SafeGetEGLConfigs(EGLDisplay dpy)
{
    EGLint total_configs = 0;
    EGLConfig *configs = NULL;

    // 1. Önce kaç config olduğunu öğren
    if (eglGetConfigs(
            dpy,
            NULL,
            0,
            &total_configs) != EGL_TRUE)
    {
        return EGL_FALSE;
    }

    if (total_configs == 0)
    {
        return EGL_TRUE;
    }

    // 2. Gereken kadar bellek ayır
    configs = malloc(
        total_configs * sizeof(EGLConfig)
    );

    if (configs == NULL)
    {
        return EGL_FALSE;
    }

    // 3. Gerçek config handle'larını al
    if (eglGetConfigs(
            dpy,
            configs,
            total_configs,
            &total_configs) != EGL_TRUE)
    {
        free(configs);
        return EGL_FALSE;
    }

    // configs artık kullanılabilir

    free(configs);

    return EGL_TRUE;
}
```

Buradaki süreç:

```text
dpy
 │
 ▼
eglGetConfigs(dpy, NULL, 0, &total)
 │
 ▼
Kaç config var?
 │
 ▼
malloc()
 │
 ▼
Yeterli dizi oluştur
 │
 ▼
eglGetConfigs(dpy, configs, total, &total)
 │
 ▼
Config handle'larını elde et
```

---

# `eglGetConfigs` ile `eglChooseConfig` Farkı

Hocanın sorması muhtemel önemli bir fark budur.

## `eglGetConfigs`

```text
Tüm config'leri getir.
```

Attribute filtresi almaz.

Örneğin:

```c
eglGetConfigs(...)
```

dediğimizde:

```text
Config 1
Config 2
Config 3
Config 4
...
```

gibi mevcut config'leri elde ederiz.

---

## `eglChooseConfig`

Burada ise belirli kriterler verebiliriz.

Mantıksal olarak:

```text
"Bana şunları sağlayan config'leri getir:

Red = 8 bit
Green = 8 bit
Blue = 8 bit
Depth = 24 bit
Window surface desteklesin."
```

Bunun için:

```c
eglChooseConfig(...)
```

kullanılır.

Kısaca:

```text
eglGetConfigs
→ HEPSİNİ GETİR

eglChooseConfig
→ ŞARTLARIMA UYANLARI GETİR
```

---

# Config Handle Nedir?

`eglGetConfigs` bize framebuffer'ın kendisini vermez.

Bize:

```c
EGLConfig
```

tipinde opaque handle'lar verir.

Örneğin:

```c
EGLConfig config;
```

Bu config'in özelliklerini öğrenmek istersek:

```c
eglGetConfigAttrib(
    dpy,
    config,
    EGL_DEPTH_SIZE,
    &depth
);
```

gibi sorgular yapabiliriz.

Yani süreç:

```text
eglGetConfigs
      ↓
EGLConfig handle'larını elde et
      ↓
eglGetConfigAttrib
      ↓
Her config'in özelliklerini öğren
```

---

# Örnek Config Özellikleri

Bir config'in örneğin şu özellikleri olabilir:

```text
EGL_RED_SIZE     = 8
EGL_GREEN_SIZE   = 8
EGL_BLUE_SIZE    = 8
EGL_ALPHA_SIZE   = 8
EGL_DEPTH_SIZE   = 24
EGL_STENCIL_SIZE = 8
```

Başka bir config:

```text
RGB565
Depth = 16
Stencil = 0
```

olabilir.

`eglGetConfigs` bunlar arasında seçim yapmaz.

Sadece config handle'larını getirir.

---

# Surface Türleriyle İlişkisi

Bir `EGLConfig`, hangi tür surface'leri desteklediği hakkında özelliklere de sahiptir.

Örneğin:

```text
EGL_WINDOW_BIT
EGL_PIXMAP_BIT
EGL_PBUFFER_BIT
```

Bu bilgi:

```c
eglGetConfigAttrib(
    dpy,
    config,
    EGL_SURFACE_TYPE,
    &surface_type
);
```

ile sorgulanabilir.

Bu nedenle `eglGetConfigs` sonucunda gelen config'lerin hepsinin aynı özelliklere sahip olduğunu düşünmemeliyiz.

---

# Fonksiyonun Dönüş Değeri

Fonksiyonun dönüş tipi:

```c
EGLBoolean
```

dır.

Başarılı olursa:

```text
EGL_TRUE
```

Başarısız olursa:

```text
EGL_FALSE
```

döner.

Örneğin:

```c
if (eglGetConfigs(
        dpy,
        configs,
        config_size,
        &num_config) != EGL_TRUE)
{
    EGLint error = eglGetError();
}
```

---

# Hata Mantığı

Temel hata durumları:

| Durum | Sonuç |
|---|---|
| Her şey doğru | `EGL_TRUE` |
| Display geçersiz | `EGL_FALSE` + `EGL_BAD_DISPLAY` |
| Display initialize edilmemiş | `EGL_FALSE` + `EGL_NOT_INITIALIZED` |

Bu fonksiyonda hata mantığını şu şekilde ezberleyebiliriz:

```text
Display yok/geçersiz
        ↓
EGL_BAD_DISPLAY

Display var ama initialize değil
        ↓
EGL_NOT_INITIALIZED
```

---

# `configs = NULL` Bir Hata Değildir

Bu nokta özellikle önemlidir.

Normalde pointer parametresine `NULL` vermek bize hata gibi gelebilir.

Ancak burada:

```c
configs = NULL
```

özel ve kullanışlı bir anlam taşır.

Şunu ifade eder:

> "Config dizisini istemiyorum. Sadece kaç config olduğunu söyle."

Örneğin:

```c
EGLint count;

eglGetConfigs(
    dpy,
    NULL,
    0,
    &count
);
```

geçerli ve bilinçli bir kullanım şeklidir.

---

# `num_config` Neden Pointer?

Fonksiyon:

```c
EGLBoolean
```

döndüğü için dönüş değeri zaten:

```text
Başarılı mı?
Başarısız mı?
```

bilgisini vermek için kullanılır.

Peki config sayısını nasıl alacağız?

Bu yüzden:

```c
EGLint *num_config
```

output parametresi kullanılır.

Yani:

```text
Fonksiyon dönüşü
      ↓
İşlem başarılı mı?

num_config
      ↓
Kaç config elde edildi?
```

Bu ikisinin görevleri farklıdır.

---

# Hocanın Verebileceği Örnek

Diyelim:

```c
EGLConfig configs[5];
EGLint num;

EGLBoolean result =
    eglGetConfigs(
        dpy,
        configs,
        5,
        &num
    );
```

Hocamız:

> "Burada her parametre ne yapıyor?"

derse:

Şöyle cevaplarım:

> `dpy`, hangi EGLDisplay'in config'lerini sorguladığımı belirliyor. `configs`, elde edilen EGLConfig handle'larının yazılacağı dizidir. `5`, yani `config_size`, bu diziye en fazla beş config yazılabileceğini söylüyor. `&num` ise fonksiyonun elde ettiği config sayısını yazacağı output parametresidir. İşlem başarılı olursa fonksiyon `EGL_TRUE`, başarısız olursa `EGL_FALSE` döner.

---

# Hocanın Sorabileceği Senaryo 1

```c
EGLConfig configs[5];

eglGetConfigs(
    dpy,
    configs,
    5,
    &num
);
```

Sistemde 3 config varsa ne olur?

Cevap:

> Üç config de diziye yazılabilir çünkü kapasitemiz 5'tir. Sonuç sayısı da 3 olur.

```text
config_size = 5
mevcut = 3

num = 3
```

---

# Hocanın Sorabileceği Senaryo 2

Sistemde 20 config var ama:

```c
EGLConfig configs[5];
```

ayırdık.

```c
eglGetConfigs(
    dpy,
    configs,
    5,
    &num
);
```

Bu durumda:

> Diziye kapasitesinden fazla eleman yazılmaz. Bu çağrıda en fazla beş config handle'ı alınabilir.

Önemli olan:

```text
config_size
```

buffer'ın kapasite sınırıdır.

---

# Hocanın Sorabileceği Senaryo 3

```c
eglGetConfigs(
    dpy,
    NULL,
    0,
    &num
);
```

Burada ne oluyor?

Cevap:

> Config handle'larını istemiyorum. Sadece sistemde kaç tane config bulunduğunu öğreniyorum. Sonuç `num` değişkenine yazılıyor.

Bu kullanım iki adımlı sorgunun ilk aşamasıdır.

---

# Hocanın Sorabileceği Senaryo 4

```c
eglGetConfigs(
    EGL_NO_DISPLAY,
    NULL,
    0,
    &num
);
```

Burada:

```text
EGL_FALSE
```

döner.

Hata:

```text
EGL_BAD_DISPLAY
```

olur.

Çünkü hangi EGLDisplay üzerinde sorgu yapılacağı belli değildir.

---

# Hocanın Sorabileceği Senaryo 5

```c
EGLDisplay dpy = eglGetDisplay(...);

// eglInitialize çağrılmadı

eglGetConfigs(
    dpy,
    NULL,
    0,
    &num
);
```

Burada display handle bulunmasına rağmen EGL başlatılmamıştır.

Dolayısıyla:

```text
EGL_NOT_INITIALIZED
```

hatası beklenir.

---

# En Kritik Parametre İlişkisi

Bu fonksiyonda özellikle üç parametre birlikte düşünülmelidir:

```text
configs
config_size
num_config
```

Şöyle:

```text
          configs
             │
             ▼
       [ ............ ]
       Config dizisi
             ▲
             │
       config_size
       "Kapasite kaç?"
             │
             ▼
        eglGetConfigs
             │
             ▼
        num_config
      "Sonuçta kaç?"
```

Kısaca:

```text
configs
→ VERİNİN YAZILACAĞI YER

config_size
→ YERİN KAPASİTESİ

num_config
→ ELDE EDİLEN SONUÇ
```

---

# `eglGetConfigs` Ne Yapmaz?

Fonksiyonu anlamanın bir başka yolu da ne yapmadığını bilmektir.

`eglGetConfigs`:

- yeni framebuffer oluşturmaz,
- yeni context oluşturmaz,
- surface oluşturmaz,
- OpenGL ES çizimi yapmaz,
- config'leri bizim kriterlerimize göre filtrelemez,
- config özelliklerini doğrudan açıklamaz.

Yaptığı temel iş:

> Mevcut `EGLConfig` handle'larını elde etmemizi sağlamaktır.

---

# EGL Akışındaki Yeri

Genel bir EGL başlangıç akışında fonksiyonun yerini şöyle düşünebiliriz:

```text
Native Display
      │
      ▼
eglGetDisplay
      │
      ▼
EGLDisplay
      │
      ▼
eglInitialize
      │
      ▼
eglGetConfigs / eglChooseConfig
      │
      ▼
EGLConfig
      │
      ├───────────────┐
      ▼               ▼
eglCreateContext   eglCreate...Surface
      │               │
      └───────┬───────┘
              ▼
       eglMakeCurrent
              │
              ▼
        OpenGL ES çizimi
```

Yani `eglGetConfigs`, EGL başlangıç sürecinin oldukça erken aşamasındadır.

Önce hangi grafik yapılandırmalarının mevcut olduğunu öğreniriz, sonra bunlardan birini kullanarak context ve surface oluşturma aşamasına geçebiliriz.

---

# Hocanın Sorabileceği Hızlı Sorular

## `eglGetConfigs` ne yapar?

Belirli bir `EGLDisplay` üzerindeki mevcut `EGLConfig` handle'larını veya bunların sayısını elde etmek için kullanılır.

---

## Yeni config oluşturur mu?

Hayır.

Var olan config'leri sorgular.

---

## `eglGetConfigs` ile `eglChooseConfig` farkı nedir?

```text
eglGetConfigs
→ Mevcut config'leri listeler.

eglChooseConfig
→ Verdiğim kriterlere uyan config'leri seçer.
```

---

## `configs` nedir?

Config handle'larının yazılacağı dizinin adresidir.

---

## `configs = NULL` olabilir mi?

Evet.

Bu durumda config dizisi alınmaz; sadece config sayısı öğrenilebilir.

---

## `config_size` nedir?

`configs` dizisine en fazla kaç config yazılabileceğini belirten kapasitedir.

---

## `num_config` nedir?

Fonksiyonun config sayısını yazdığı output parametresidir.

---

## `config_size` ile `num_config` farkı nedir?

```text
config_size
→ Biz fonksiyona veririz.
→ Kapasite.

num_config
→ Fonksiyon bize verir.
→ Sonuç.
```

---

## Display geçersizse?

```text
EGL_BAD_DISPLAY
```

---

## Display initialize edilmemişse?

```text
EGL_NOT_INITIALIZED
```

---

## Config özelliklerini nasıl öğrenirim?

`eglGetConfigAttrib` ile.

---

# 30 Saniyelik Final Özeti

> `eglGetConfigs`, initialize edilmiş bir `EGLDisplay` üzerinde bulunan EGLConfig'leri elde etmek için kullanılır. Dört parametresi vardır. `dpy`, hangi display'in sorgulandığını belirtir. `configs`, config handle'larının yazılacağı dizidir. `config_size`, bu diziye en fazla kaç config yazılabileceğini belirler. `num_config` ise elde edilen config sayısının yazıldığı output parametresidir. `configs` parametresini `NULL` verirsek config'leri almadan yalnızca toplam sayıyı öğrenebiliriz; bu nedenle fonksiyon iki adımlı sorgu için uygundur. `eglGetConfigs` filtreleme yapmaz; belirli özelliklere göre config seçmek için `eglChooseConfig` kullanılır.

---

# En Kritik Şema

```text
                     eglGetConfigs
                           │
        ┌──────────────────┼──────────────────┬─────────────────┐
        │                  │                  │                 │
       dpy              configs         config_size       num_config
        │                  │                  │                 │
        ▼                  ▼                  ▼                 ▼
   NEREDEN?            NEREYE?          EN FAZLA KAÇ?       SONUÇ KAÇ?
        │                  │                  │                 │
        ▼                  ▼                  ▼                 ▼
 EGLDisplay         EGLConfig dizisi      Kapasite           Output
```

---

# Özellikle Öğrenilmesi Gereken 5 Ayrım

1. `eglGetConfigs` ↔ `eglChooseConfig`
2. `configs` ↔ `num_config`
3. `config_size` ↔ `num_config`
4. `EGL_BAD_DISPLAY` ↔ `EGL_NOT_INITIALIZED`
5. `configs = NULL` kullanımının hata değil, **yalnızca sayıyı öğrenme yöntemi** olması

---

# Tek Satırlık Ezber

```text
dpy = nereden,
configs = nereye,
config_size = en fazla kaç,
num_config = sonuç kaç.
```
