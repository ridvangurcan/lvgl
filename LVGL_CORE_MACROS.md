# LVGL Core Modülü: Makro Kullanımı ve Teknikleri

## İçindekiler
1. [Giriş](#giriş)
2. [Makro Kategorileri](#makro-kategorileri)
3. [Tip Güvenliği Makroları](#tip-güvenliği-makroları)
4. [Utility Makroları](#utility-makroları)
5. [Konfigürasyon Makroları](#konfigürasyon-makroları)
6. [Bit Manipülasyon Makroları](#bit-manipülasyon-makroları)
7. [API Makroları](#api-makroları)
8. [Kod Üretim Makroları](#kod-üretim-makroları)
9. [Debug ve Logging Makroları](#debug-ve-logging-makroları)
10. [Gelişmiş Makro Teknikleri](#gelişmiş-makro-teknikleri)
11. [Makro Best Practice'leri](#makro-best-practiceleri)
12. [Özet ve Çıkarımlar](#özet-ve-çıkarımlar)

---

## Giriş

Bu doküman, LVGL (Light and Versatile Graphics Library) kütüphanesinin `/src/core` modülünde kullanılan C preprocessor makrolarını detaylı olarak inceler.

**Makrolar neden önemli?**
- ✅ **Sıfır maliyet abstraksiyonu** - Compile-time'da çözülür, runtime overhead yok
- ✅ **Tip güvenliği** - C'nin zayıf type system'ini güçlendirir
- ✅ **Kod tekrarını önler** - DRY (Don't Repeat Yourself) prensibi
- ✅ **Conditional compilation** - Platform/feature bazlı derleme
- ✅ **Debug/Release farklılaştırması** - Geliştirme vs production optimizasyonu

**LVGL'de Makro Kullanım İstatistikleri:**
- **60+ farklı makro kategorisi** core modülde
- **Type safety, utility, configuration, debug** gibi çeşitli amaçlar
- **Zero-cost abstractions** - Release build'de sıfır overhead
- **Compile-time polymorphism** - Token pasting ile

---

## Makro Kategorileri

LVGL core modülünde kullanılan makrolar şu kategorilere ayrılabilir:

| Kategori | Amaç | Örnek Makrolar |
|----------|------|----------------|
| **Type Safety** | Tip kontrolü, validasyon | `LV_ASSERT_OBJ`, `LV_ASSERT_NULL` |
| **Utility** | Genel yardımcı fonksiyonlar | `LV_MIN`, `LV_MAX`, `LV_CLAMP` |
| **Configuration** | Conditional compilation | `LV_USE_ASSERT_OBJ`, `LV_USE_LOG` |
| **Bit Manipulation** | Flag ve state yönetimi | `LV_OBJ_FLAG_*`, `LV_STATE_*` |
| **API Helpers** | API kolaylıkları | `MY_CLASS`, `LV_DPX` |
| **Code Generation** | Token pasting, stringification | `LV_CONCAT`, `LV_STRINGIFY` |
| **Debug/Logging** | Geliştirme araçları | `LV_LOG_TRACE`, `EVENT_TRACE` |

---

## Tip Güvenliği Makroları

### 1. **LV_ASSERT_OBJ** - Nesne Validasyonu

**Konum:** `src/core/lv_obj.h:388-396`

C dilinin zayıf type system'ini güçlendiren en kritik makro. Runtime'da nesne geçerliliğini kontrol eder.

#### Tanım:

```c
#if LV_USE_ASSERT_OBJ
    #define LV_ASSERT_OBJ(obj_p, obj_class) \
        do { \
            LV_ASSERT_MSG(obj_p != NULL, "The object is NULL"); \
            LV_ASSERT_MSG(lv_obj_has_class(obj_p, obj_class) == true, \
                         "Incompatible object type. " \
                         "Expected: %s. Current: %s", \
                         (obj_class)->name, \
                         obj_p->class_p ? obj_p->class_p->name : "undefined"); \
            LV_ASSERT_MSG(lv_obj_is_valid(obj_p) == true, \
                         "The object is invalid, deleted or corrupted."); \
        } while(0)
#else
    #define LV_ASSERT_OBJ(obj_p, obj_class) do{}while(0)
#endif
```

#### Üç Katmanlı Validasyon:

1. **NULL pointer kontrolü**: `obj_p != NULL`
2. **Tip uyumluluğu kontrolü**: Nesne doğru class'tan mı? (kalıtım hiyerarşisi dahil)
3. **Validite kontrolü**: Nesne silinmemiş/corrupt olmamış mı?

#### Kullanım Örnekleri:

```c
// lv_slider.c'den örnek
void lv_slider_set_value(lv_obj_t * obj, int32_t value) {
    LV_ASSERT_OBJ(obj, &lv_slider_class);  // Slider mi kontrol et

    lv_slider_t * slider = (lv_slider_t *)obj;
    slider->value = value;
    // ...
}

// lv_event.c'den örnek
lv_res_t lv_event_send(lv_obj_t * obj, lv_event_code_t event_code, void * param) {
    if(obj == NULL) return LV_RES_OK;
    LV_ASSERT_OBJ(obj, &lv_obj_class);  // En azından base obj olmalı

    // Event gönderme işlemi...
}
```

#### Faydaları:

- ✅ **Erken hata yakalama**: Yanlış tip kullanımı hemen anlaşılır
- ✅ **Hiyerarşi desteği**: Parent class kabul edilir (polimorfizm)
- ✅ **Dangling pointer tespiti**: Silinen nesnelere erişim engellenir
- ✅ **Sıfır maliyet (release)**: Production build'de tamamen kaldırılabilir

---

### 2. **LV_ASSERT_NULL** - NULL Pointer Kontrolü

**Konum:** `src/misc/lv_assert.h:45-50`

```c
#if LV_USE_ASSERT_NULL
    #define LV_ASSERT_NULL(p) LV_ASSERT_MSG(p != NULL, "NULL pointer")
#else
    #define LV_ASSERT_NULL(p) do{}while(0)
#endif
```

#### Kullanım:

```c
void lv_obj_set_parent(lv_obj_t * obj, lv_obj_t * parent) {
    LV_ASSERT_NULL(obj);     // obj NULL olamaz
    LV_ASSERT_NULL(parent);  // parent NULL olamaz

    obj->parent = parent;
}
```

---

### 3. **LV_ASSERT_MSG** - Mesajlı Assertion

**Konum:** `src/misc/lv_assert.h:37-43`

```c
#if LV_USE_ASSERT_MSG
    #define LV_ASSERT_MSG(expr, msg) \
        do { \
            if(!(expr)) { \
                LV_LOG_ERROR("Asserted at expression: %s", #expr); \
                lv_assert_handler(__FILE__, __LINE__, msg); \
            } \
        } while(0)
#else
    #define LV_ASSERT_MSG(expr, msg) do{}while(0)
#endif
```

#### Özellikler:

- **Stringification**: `#expr` ile expression'ı string'e çevirir
- **File/Line bilgisi**: `__FILE__`, `__LINE__` otomatik eklenir
- **Özel mesaj**: Hata durumunda detaylı açıklama

---

## Utility Makroları

### 1. **LV_MIN / LV_MAX** - Karşılaştırma

**Konum:** `src/misc/lv_math.h:121-127`

```c
#define LV_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LV_MAX(a, b) ((a) > (b) ? (a) : (b))
```

#### ⚠️ Önemli Nokta: Parametre Parantezleme

```c
// Yanlış (parantez yok):
#define BAD_MIN(a, b) (a < b ? a : b)

int x = BAD_MIN(5 + 3, 10);
// Expands: (5 + 3 < 10 ? 5 + 3 : 10)
// Result: 5 + 3 = 8  ❌ YANLIŞ!

// Doğru (parantezli):
int x = LV_MIN(5 + 3, 10);
// Expands: ((5 + 3) < (10) ? (5 + 3) : (10))
// Result: 8  ✅ DOĞRU!
```

#### Yan Etki Problemi:

```c
int a = 5, b = 10;
int min = LV_MIN(a++, b++);
// Expands: ((a++) < (b++) ? (a++) : (b++))
// a ve b iki kez artırılır! ⚠️

// Çözüm: Yan etkili expression'lar kullanma
int temp_a = a++;
int temp_b = b++;
int min = LV_MIN(temp_a, temp_b);
```

---

### 2. **LV_CLAMP** - Değer Sınırlama

**Konum:** `src/misc/lv_math.h:129`

```c
#define LV_CLAMP(min, val, max) (LV_MAX(min, (LV_MIN(val, max))))
```

#### Makro Composition:

`LV_CLAMP`, `LV_MIN` ve `LV_MAX`'i kullanarak oluşturulmuş - **makro composition** örneği.

```
LV_CLAMP(0, -5, 100)
  ↓
LV_MAX(0, LV_MIN(-5, 100))
  ↓
LV_MAX(0, -5)
  ↓
0
```

#### Kullanım Örnekleri:

```c
// Opacity sınırlama (0-255)
uint8_t opa = LV_CLAMP(0, user_input, 255);

// Slider değeri sınırlama
int32_t value = LV_CLAMP(slider->min_value, raw_value, slider->max_value);

// RGB renk bileşeni
uint8_t red = LV_CLAMP(0, calculated_red, 255);
```

---

### 3. **LV_ABS** - Mutlak Değer

**Konum:** `src/misc/lv_math.h:135`

```c
#define LV_ABS(x) ((x) > 0 ? (x) : (-(x)))
```

---

### 4. **LV_DPX** - DPI Scaling

**Konum:** `src/hal/lv_hal.h:30-35` (yaklaşık)

DPI (Dots Per Inch) ölçeklendirmesi - Android'deki DIP (Density Independent Pixels) konseptine benzer.

```c
#if LV_DPI_DEF != 0
    #define LV_DPX(n) ((n) == 0 ? 0 : LV_MAX((n * LV_DPI_DEF / 160), 1))
#else
    #define LV_DPX(n) (n)
#endif
```

#### Mantık:

- **Referans DPI**: 160 (mdpi - medium density)
- **Ölçeklendirme**: `pixel = (dp * current_dpi) / 160`
- **Minimum**: En az 1 pixel (0 olmadığı sürece)

#### Kullanım Örnekleri:

```c
// Varsayılan nesne boyutları
#define LV_OBJ_DEF_WIDTH    LV_DPX(100)   // 100 DIP
#define LV_OBJ_DEF_HEIGHT   LV_DPX(50)    // 50 DIP

// 160 DPI ekranda:  100 pixel
// 320 DPI ekranda:  200 pixel (2x)
// 480 DPI ekranda:  300 pixel (3x)

// Padding değerleri
lv_obj_set_style_pad_all(obj, LV_DPX(10), 0);  // 10 DIP padding
```

#### Faydaları:

- ✅ **Fiziksel boyut tutarlılığı**: Farklı DPI'larda aynı fiziksel boyut
- ✅ **Otomatik ölçeklendirme**: Kod değişikliği gerektirmez
- ✅ **Derleme zamanı hesaplama**: Sabit değerler için

---

### 5. **LV_UNUSED** - Kullanılmayan Parametre

**Konum:** `src/misc/lv_types.h:35`

```c
#define LV_UNUSED(x) (void)(x)
```

#### Kullanım:

```c
static void my_event_cb(lv_event_t * e) {
    LV_UNUSED(e);  // Compiler warning'i engelle

    // Event kullanılmıyor ama callback signature gereği parametre gerekli
}
```

Compiler'ın "unused parameter" warning'ini engeller.

---

## Konfigürasyon Makroları

### 1. **Conditional Compilation Pattern**

LVGL'de yaygın olarak kullanılan pattern:

```c
#if LV_USE_FEATURE_X
    // Feature enabled - kod dahil edilir
    #define FEATURE_X_MACRO(...) /* implementation */
#else
    // Feature disabled - empty macro (zero cost)
    #define FEATURE_X_MACRO(...) do{}while(0)
#endif
```

#### Örnekler:

**Assertion Sistemi:**
```c
#if LV_USE_ASSERT_OBJ
    #define LV_ASSERT_OBJ(obj, class) /* validation code */
#else
    #define LV_ASSERT_OBJ(obj, class) do{}while(0)
#endif
```

**User Data:**
```c
typedef struct _lv_obj_t {
    const lv_obj_class_t * class_p;
    struct _lv_obj_t * parent;

#if LV_USE_USER_DATA
    void * user_data;  // Sadece enabled ise struct'a dahil
#endif

    // ...
} lv_obj_t;
```

**Animation:**
```c
#if LV_USE_ANIMATION
    void lv_obj_del_delayed(lv_obj_t * obj, uint32_t delay_ms);
#else
    #define lv_obj_del_delayed(obj, delay) lv_obj_del(obj)
#endif
```

#### Faydaları:

- ✅ **Kod boyutu optimizasyonu**: Kullanılmayan özellikler derlenmez
- ✅ **Bellek tasarrufu**: Struct'larda gereksiz alanlar yok
- ✅ **Debug vs Release**: Farklı build'ler için farklı davranış
- ✅ **Platform özelleştirmesi**: Her platform için özel konfigürasyon

---

### 2. **LV_COLOR_DEPTH** - Compile-Time Polymorphism

**Konum:** `src/misc/lv_color.h`

```c
#if LV_COLOR_DEPTH == 32
    #define LV_COLOR_SIZE 4
    typedef struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    } lv_color32_t;
    typedef lv_color32_t lv_color_t;

#elif LV_COLOR_DEPTH == 16
    #define LV_COLOR_SIZE 2
    typedef struct {
        uint16_t blue : 5;
        uint16_t green : 6;
        uint16_t red : 5;
    } lv_color16_t;
    typedef lv_color16_t lv_color_t;

#elif LV_COLOR_DEPTH == 8
    #define LV_COLOR_SIZE 1
    typedef uint8_t lv_color8_t;
    typedef lv_color8_t lv_color_t;
#endif
```

**Tek kod, farklı platform'lar** - compile-time polymorphism.

---

### 3. **Coordinate Type System**

**Konum:** `src/misc/lv_area.h:259-274`

```c
// Özel coordinate değerleri
#define LV_SIZE_CONTENT  0x2000  // Auto-size
#define LV_COORD_MAX     0x3FFF  // Maximum değer
#define LV_COORD_MIN     (-0x4000) // Minimum değer

// Percentage encoding
#define _LV_COORD_TYPE_SHIFT       13
#define _LV_COORD_TYPE_MASK        0x7
#define _LV_COORD_TYPE_PX_SPEC     0x0
#define _LV_COORD_TYPE_SPEC        0x1
#define _LV_COORD_TYPE_PX_PCT      0x2

#define LV_PCT(x)  (((int32_t)(x) << _LV_COORD_TYPE_SHIFT) | \
                    (_LV_COORD_TYPE_PX_PCT << 13))
```

#### Kullanım:

```c
// Pixel değeri
lv_obj_set_width(obj, 100);  // 100 pixel

// Percentage değeri
lv_obj_set_width(obj, LV_PCT(50));  // Parent'ın %50'si

// Content size
lv_obj_set_width(obj, LV_SIZE_CONTENT);  // İçeriğe göre
```

#### Nasıl Çalışır?

```
LV_PCT(50)
  ↓
(50 << 13) | (0x2 << 13)
  ↓
0x6000 | 0x4000
  ↓
0xA000  // Encoded percentage value
```

Koordinat değerinin yüksek bitleri **tip bilgisi** taşır - clever encoding!

---

## Bit Manipülasyon Makroları

### 1. **LV_OBJ_FLAG_*** - Object Flags

**Konum:** `src/core/lv_obj.h:90-121`

```c
typedef enum {
    LV_OBJ_FLAG_HIDDEN              = (1L << 0),   // 0x00000001
    LV_OBJ_FLAG_CLICKABLE           = (1L << 1),   // 0x00000002
    LV_OBJ_FLAG_CLICK_FOCUSABLE     = (1L << 2),   // 0x00000004
    LV_OBJ_FLAG_CHECKABLE           = (1L << 3),   // 0x00000008
    LV_OBJ_FLAG_SCROLLABLE          = (1L << 4),   // 0x00000010
    LV_OBJ_FLAG_SCROLL_ELASTIC      = (1L << 5),   // 0x00000020
    LV_OBJ_FLAG_SCROLL_MOMENTUM     = (1L << 6),   // 0x00000040
    LV_OBJ_FLAG_SCROLL_ONE          = (1L << 7),   // 0x00000080
    LV_OBJ_FLAG_SCROLL_CHAIN_HOR    = (1L << 8),   // 0x00000100
    LV_OBJ_FLAG_SCROLL_CHAIN_VER    = (1L << 9),   // 0x00000200

    // Kombine flag'ler
    LV_OBJ_FLAG_SCROLL_CHAIN = (LV_OBJ_FLAG_SCROLL_CHAIN_HOR |
                                 LV_OBJ_FLAG_SCROLL_CHAIN_VER),

    // ... 30+ flag toplam
} lv_obj_flag_t;
```

#### Bit Maskeleme Teknikleri:

**Flag Ekleme:**
```c
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f) {
    obj->flags |= f;  // OR ile ekleme
}

// Kullanım
lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
```

**Flag Kaldırma:**
```c
void lv_obj_clear_flag(lv_obj_t * obj, lv_obj_flag_t f) {
    obj->flags &= ~f;  // AND NOT ile kaldırma
}
```

**Flag Kontrolü:**
```c
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f) {
    return (obj->flags & f) == f;  // AND ile kontrol
}
```

#### Bitwise Operations Hatırlatma:

```
flags = 0b00001010  (SCROLLABLE | CLICK_FOCUSABLE)

// Add CLICKABLE (0x02 = 0b00000010)
flags |= 0x02
  ↓
0b00001010 | 0b00000010 = 0b00001010  // Zaten var, değişmez

// Add HIDDEN (0x01 = 0b00000001)
flags |= 0x01
  ↓
0b00001010 | 0b00000001 = 0b00001011

// Remove SCROLLABLE (0x10 = 0b00010000)
flags &= ~0x10
  ↓
0b00001011 & 0b11101111 = 0b00001011  // Zaten yok, değişmez

// Check CLICKABLE
(flags & 0x02) == 0x02
  ↓
(0b00001011 & 0b00000010) == 0b00000010
  ↓
0b00000010 == 0b00000010  → true ✅
```

---

### 2. **LV_STATE_*** - Object States

**Konum:** `src/core/lv_obj.h:380-395`

```c
typedef enum {
    LV_STATE_DEFAULT     = 0x0000,
    LV_STATE_CHECKED     = 0x0001,
    LV_STATE_FOCUSED     = 0x0002,
    LV_STATE_FOCUS_KEY   = 0x0004,
    LV_STATE_EDITED      = 0x0008,
    LV_STATE_HOVERED     = 0x0010,
    LV_STATE_PRESSED     = 0x0020,
    LV_STATE_SCROLLED    = 0x0040,
    LV_STATE_DISABLED    = 0x0080,

    LV_STATE_USER_1      = 0x1000,  // Kullanıcı tanımlı
    LV_STATE_USER_2      = 0x2000,
    LV_STATE_USER_3      = 0x4000,
    LV_STATE_USER_4      = 0x8000,

    LV_STATE_ANY         = 0xFFFF,  // Wildcard
} lv_state_t;
```

#### State vs Flag Farkı:

| Özellik | State | Flag |
|---------|-------|------|
| **Amaç** | Görsel durum (UI feedback) | Davranış konfigürasyonu |
| **Örnek** | PRESSED, FOCUSED, HOVERED | CLICKABLE, SCROLLABLE |
| **Stil Etkisi** | Evet - stil değişir | Hayır - davranış değişir |
| **Geçicilik** | Geçici (user interaction) | Kalıcı (configuration) |

#### Kullanım:

```c
// State-dependent styling
lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), LV_STATE_PRESSED);
lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), LV_STATE_FOCUSED);

// State kontrolü
if(lv_obj_has_state(btn, LV_STATE_PRESSED)) {
    // Buton basılı durumda
}
```

---

### 3. **LV_PART_*** - Visual Parts

**Konum:** `src/core/lv_obj_style.h`

```c
typedef enum {
    LV_PART_MAIN        = 0x000000,  // Ana bölüm
    LV_PART_SCROLLBAR   = 0x010000,  // Scrollbar
    LV_PART_INDICATOR   = 0x020000,  // Gösterge (slider, progress)
    LV_PART_KNOB        = 0x030000,  // Tutamaç (slider)
    LV_PART_SELECTED    = 0x040000,  // Seçili item
    LV_PART_ITEMS       = 0x050000,  // Bireysel item'lar
    LV_PART_TICKS       = 0x060000,  // Tick işaretleri
    LV_PART_CURSOR      = 0x070000,  // İmleç
    LV_PART_CUSTOM_FIRST= 0x080000,  // Özel part'lar

    LV_PART_ANY         = 0x0F0000,  // Wildcard
} lv_part_t;
```

#### Style Selector Encoding:

Style selector, **state** ve **part**'ı tek bir değerde encode eder:

```c
typedef uint32_t lv_style_selector_t;

// Encoding
lv_style_selector_t selector = LV_PART_INDICATOR | LV_STATE_PRESSED;
//                              ↑ High 8 bits      ↑ Low 16 bits

// Decoding
#define LV_STYLE_SELECTOR_PART(sel)   ((sel) & 0x0F0000)
#define LV_STYLE_SELECTOR_STATE(sel)  ((sel) & 0x00FFFF)
```

#### Kullanım Örneği:

```c
// Slider'ın indicator'ına, pressed state için stil uygula
lv_obj_set_style_bg_color(slider,
                          lv_color_hex(0xFF0000),
                          LV_PART_INDICATOR | LV_STATE_PRESSED);

// Decoding example
lv_style_selector_t sel = LV_PART_KNOB | LV_STATE_FOCUSED;
lv_part_t part = LV_STYLE_SELECTOR_PART(sel);     // = LV_PART_KNOB
lv_state_t state = LV_STYLE_SELECTOR_STATE(sel);  // = LV_STATE_FOCUSED
```

**32 bit'te hem part hem state - verimli encoding!**

---

## API Makroları

### 1. **MY_CLASS** - Class Reference

**Konum:** Her `.c` implementation dosyasında

```c
// lv_obj.c'de
#define MY_CLASS &lv_obj_class

// lv_slider.c'de
#define MY_CLASS &lv_slider_class

// lv_btn.c'de
#define MY_CLASS &lv_btn_class
```

#### Kullanım:

```c
void lv_slider_set_value(lv_obj_t * obj, int32_t value) {
    LV_ASSERT_OBJ(obj, MY_CLASS);  // MY_CLASS = &lv_slider_class

    // Implementation...
}
```

#### Faydaları:

- ✅ **DRY**: Sınıf referansı tek yerde
- ✅ **Okunabilirlik**: `MY_CLASS` daha anlaşılır
- ✅ **Refactoring**: Sınıf adı değişirse tek yerden düzelt

---

### 2. **LV_HOR_RES / LV_VER_RES** - Display Resolution

**Konum:** `src/hal/lv_hal_disp.h`

```c
#define LV_HOR_RES  lv_disp_get_hor_res(lv_disp_get_default())
#define LV_VER_RES  lv_disp_get_ver_res(lv_disp_get_default())
```

#### Kullanım:

```c
// Ekran merkezine yerleştir
lv_obj_set_pos(obj,
               (LV_HOR_RES - lv_obj_get_width(obj)) / 2,
               (LV_VER_RES - lv_obj_get_height(obj)) / 2);

// Tam ekran
lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);
```

---

### 3. **LV_GC_ROOT** - Garbage Collection Roots

**Konum:** `src/misc/lv_gc.h:43-66`

X-Macro pattern kullanımı:

```c
#define LV_GC_ROOTS(prefix) \
    prefix lv_ll_t _lv_task_ll;     \
    prefix lv_ll_t _lv_disp_ll;     \
    prefix lv_ll_t _lv_indev_ll;    \
    prefix lv_ll_t _lv_anim_ll;     \
    prefix lv_ll_t _lv_group_ll;    \
    prefix lv_ll_t _lv_img_cache_ll;
```

#### X-Macro Kullanımı:

```c
// Declaration
typedef struct {
    LV_GC_ROOTS(extern )  // Expands to: extern lv_ll_t _lv_task_ll; ...
} lv_global_t;

// Definition
lv_global_t lv_global = {
    LV_GC_ROOTS( )  // Expands to: lv_ll_t _lv_task_ll; ...
};
```

**Tek tanımlama, çoklu kullanım** - kod tekrarını önler.

---

## Kod Üretim Makroları

### 1. **LV_CONCAT** - Token Pasting

**Konum:** `src/misc/lv_types.h:77-81`

```c
#define _LV_CONCAT(x, y) x ## y
#define LV_CONCAT(x, y) _LV_CONCAT(x, y)  // İki seviye!
```

#### Neden İki Seviye?

Tek seviye token pasting, makro parametrelerini expand etmez:

```c
// Yanlış (tek seviye)
#define CONCAT_BAD(x, y) x ## y

#define PREFIX foo
CONCAT_BAD(PREFIX, _bar)
  ↓
PREFIX_bar  // PREFIX expand olmadı! ❌

// Doğru (iki seviye)
#define _CONCAT(x, y) x ## y
#define CONCAT(x, y) _CONCAT(x, y)

#define PREFIX foo
CONCAT(PREFIX, _bar)
  ↓
_CONCAT(foo, _bar)  // Önce PREFIX expand oldu
  ↓
foo_bar  // Sonra concat ✅
```

#### Kullanım Örnekleri:

**Color Depth Polymorphism:**
```c
#define LV_COLOR_DEPTH 32

#define LV_COLOR_SET_R(c, v) \
    LV_CONCAT(LV_COLOR_SET_R, LV_COLOR_DEPTH)(c, v)

// Expands to: LV_COLOR_SET_R32(c, v)
```

**Unique Variable Names:**
```c
// Her satırda benzersiz değişken adı oluştur
#define UNIQUE_VAR(prefix) LV_CONCAT(prefix, __LINE__)

int UNIQUE_VAR(temp) = 5;  // temp42 (satır 42'de)
int UNIQUE_VAR(temp) = 10; // temp43 (satır 43'te)
```

---

### 2. **LV_STRINGIFY** - Stringification

**Konum:** `src/misc/lv_types.h:83-85`

```c
#define _LV_STRINGIFY(x) #x
#define LV_STRINGIFY(x) _LV_STRINGIFY(x)
```

#### Kullanım:

```c
#define VERSION 1.2.3

// Tek seviye - expand etmez
#define STRINGIFY_BAD(x) #x
STRINGIFY_BAD(VERSION)  → "VERSION"  ❌

// İki seviye - expand eder
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
STRINGIFY(VERSION)  → "1.2.3"  ✅
```

#### Assertion'larda Kullanım:

```c
#define LV_ASSERT_MSG(expr, msg) \
    if(!(expr)) { \
        LV_LOG_ERROR("Failed: %s at %s:%d", \
                     #expr,          /* Expression as string */ \
                     __FILE__,       /* File name */ \
                     __LINE__);      /* Line number */ \
    }

// Kullanım
LV_ASSERT_MSG(x > 0, "X must be positive");

// Hata mesajı:
// "Failed: x > 0 at main.c:42"
```

---

### 3. **Compile-Time Function Selection**

Token pasting ile compile-time polymorphism:

```c
// Color depth'e göre fonksiyon seçimi
#define LV_COLOR_MAKE(r, g, b) \
    LV_CONCAT3(lv_color_make, LV_COLOR_DEPTH, _bit)(r, g, b)

#if LV_COLOR_DEPTH == 32
    lv_color_t lv_color_make32_bit(uint8_t r, uint8_t g, uint8_t b);
#elif LV_COLOR_DEPTH == 16
    lv_color_t lv_color_make16_bit(uint8_t r, uint8_t g, uint8_t b);
#endif

// Kullanım
lv_color_t red = LV_COLOR_MAKE(255, 0, 0);
// Expands to: lv_color_make32_bit(255, 0, 0) veya
//             lv_color_make16_bit(255, 0, 0)
```

**Sıfır runtime overhead - compile-time'da çözülür!**

---

## Debug ve Logging Makroları

### 1. **LV_LOG_*** - Leveled Logging

**Konum:** `src/misc/lv_log.h:89-147`

```c
#if LV_USE_LOG

    // Log seviyeleri
    enum {
        LV_LOG_LEVEL_TRACE,
        LV_LOG_LEVEL_INFO,
        LV_LOG_LEVEL_WARN,
        LV_LOG_LEVEL_ERROR,
        LV_LOG_LEVEL_USER,
        LV_LOG_LEVEL_NONE,
    };

    // Variadic macros
    #define LV_LOG_TRACE(...) _lv_log_add(LV_LOG_LEVEL_TRACE, \
                                          __FILE__, __LINE__, \
                                          __func__, __VA_ARGS__)
    #define LV_LOG_INFO(...)  _lv_log_add(LV_LOG_LEVEL_INFO, ...)
    #define LV_LOG_WARN(...)  _lv_log_add(LV_LOG_LEVEL_WARN, ...)
    #define LV_LOG_ERROR(...) _lv_log_add(LV_LOG_LEVEL_ERROR, ...)

#else
    #define LV_LOG_TRACE(...) do{}while(0)
    #define LV_LOG_INFO(...)  do{}while(0)
    #define LV_LOG_WARN(...)  do{}while(0)
    #define LV_LOG_ERROR(...) do{}while(0)
#endif
```

#### Özellikler:

- **Variadic arguments**: `__VA_ARGS__` ile printf-style formatting
- **Otomatik metadata**: File, line, function bilgisi
- **Compile-time filtering**: Disabled ise sıfır kod
- **Runtime filtering**: Log level kontrolü

#### Kullanım:

```c
LV_LOG_TRACE("Entering function with value: %d", value);
LV_LOG_INFO("Display initialized: %dx%d", width, height);
LV_LOG_WARN("Object count exceeding threshold: %d", count);
LV_LOG_ERROR("Failed to allocate memory: %zu bytes", size);
```

---

### 2. **Module-Specific Traces**

**Konumlar:**
- `EVENT_TRACE` → `src/core/lv_event.c:43`
- `REFR_TRACE` → `src/core/lv_refr.c:90`
- `INDEV_TRACE` → `src/core/lv_indev.c:55`

```c
// lv_event.c'de
#if LV_LOG_TRACE_EVENT
    #define EVENT_TRACE(...) LV_LOG_TRACE(__VA_ARGS__)
#else
    #define EVENT_TRACE(...) do{}while(0)
#endif

// Kullanım
EVENT_TRACE("Event sent: code=%d, target=%p", event_code, obj);
```

#### Granular Control:

Modül bazında trace açıp kapatabilme:

```c
// lv_conf.h'de
#define LV_LOG_TRACE_EVENT   1  // Event trace enabled
#define LV_LOG_TRACE_REFR    0  // Refresh trace disabled
#define LV_LOG_TRACE_INDEV   1  // Input trace enabled
```

---

### 3. **LV_ASSERT_HANDLER** - Custom Assertion Handler

```c
#if LV_USE_ASSERT_HANDLER
    void lv_assert_handler(const char * file, int line, const char * msg) {
        // Özel assertion handling
        // Örnek: LED yak, buzzer çal, debug port'a yaz
        error_led_on();
        debug_printf("ASSERT: %s:%d - %s\n", file, line, msg);
        while(1);  // Hang for debugging
    }
#endif
```

Embedded sistemlerde debugging için kritik.

---

## Gelişmiş Makro Teknikleri

### 1. **do-while(0) Pattern**

Makroları her context'te güvenli hale getirir:

```c
// YANLIŞ: Braces olmadan
#define BAD_MACRO(x) \
    statement1(x); \
    statement2(x);

if(condition)
    BAD_MACRO(val);  // Sadece statement1 if'in içinde!
else
    something();

// DOĞRU: do-while(0) ile
#define GOOD_MACRO(x) do { \
    statement1(x); \
    statement2(x); \
} while(0)

if(condition)
    GOOD_MACRO(val);  // Tüm macro if'in içinde ✅
else
    something();
```

#### Neden while(0)?

- ✅ **Tek statement gibi davranır**: Semicolon gerektir
- ✅ **Her yerde kullanılabilir**: if/else, switch, etc.
- ✅ **Compiler optimize eder**: Loop overhead yok

#### LVGL'deki Kullanımlar:

```c
#define LV_ASSERT_MSG(expr, msg) do { \
    if(!(expr)) { \
        LV_LOG_ERROR(...); \
        lv_assert_handler(...); \
    } \
} while(0)

#define LV_LOG_TRACE(...) do { \
    _lv_log_add(LV_LOG_LEVEL_TRACE, ...); \
} while(0)
```

---

### 2. **Variadic Macros** - __VA_ARGS__

C99'dan beri mevcut - değişken sayıda argüman:

```c
#define DEBUG_PRINT(fmt, ...) \
    printf("[DEBUG] " fmt "\n", __VA_ARGS__)

// Kullanım
DEBUG_PRINT("Value: %d", x);
// Expands: printf("[DEBUG] " "Value: %d" "\n", x);

DEBUG_PRINT("X=%d, Y=%d", x, y);
// Expands: printf("[DEBUG] " "X=%d, Y=%d" "\n", x, y);
```

#### GNU Extension: ##__VA_ARGS__

Virgül silme - argüman yoksa:

```c
#define DEBUG_PRINT(fmt, ...) \
    printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
    //                          ↑ Ekstra ##

// Argüman olmadan
DEBUG_PRINT("Hello");
// Expands: printf("[DEBUG] " "Hello" "\n");  ✅
// ## removes trailing comma
```

#### LVGL'de Kullanım:

```c
#define LV_LOG_ERROR(fmt, ...) \
    _lv_log_add(LV_LOG_LEVEL_ERROR, \
                __FILE__, __LINE__, __func__, \
                fmt, ##__VA_ARGS__)
```

---

### 3. **X-Macros Pattern**

Kod tekrarını önleyen güçlü pattern:

```c
// Enum tanımı
#define COLOR_LIST \
    X(RED, 0xFF0000) \
    X(GREEN, 0x00FF00) \
    X(BLUE, 0x0000FF) \
    X(WHITE, 0xFFFFFF) \
    X(BLACK, 0x000000)

// Enum generation
enum {
    #define X(name, value) COLOR_##name,
    COLOR_LIST
    #undef X
};
// Expands:
// enum {
//     COLOR_RED,
//     COLOR_GREEN,
//     COLOR_BLUE,
//     COLOR_WHITE,
//     COLOR_BLACK,
// };

// String table generation
const char * color_names[] = {
    #define X(name, value) #name,
    COLOR_LIST
    #undef X
};
// Expands:
// const char * color_names[] = {
//     "RED", "GREEN", "BLUE", "WHITE", "BLACK",
// };

// Value table generation
const uint32_t color_values[] = {
    #define X(name, value) value,
    COLOR_LIST
    #undef X
};
```

**Tek liste tanımı, üç farklı kullanım!**

---

### 4. **Compile-Time Assertions**

Static assert - derleme zamanı kontrolü:

```c
// C11 standardı
_Static_assert(sizeof(lv_obj_t) <= 128, "Object too large!");

// Macro wrapper
#define LV_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

LV_STATIC_ASSERT(LV_COLOR_DEPTH == 16 || LV_COLOR_DEPTH == 32,
                 "Unsupported color depth");
```

**Runtime değil, compile-time kontrol - sıfır overhead!**

---

### 5. **Macro Overloading** - Argument Count Based

Argüman sayısına göre farklı makro seçimi:

```c
// Helper macros
#define GET_MACRO_3(_1, _2, _3, NAME, ...) NAME

// Overloaded macro
#define SET_COLOR(...) GET_MACRO_3(__VA_ARGS__, \
                                    set_color_3, \
                                    set_color_2, \
                                    set_color_1)(__VA_ARGS__)

#define set_color_1(obj)           set_color_impl(obj, WHITE, 255)
#define set_color_2(obj, col)      set_color_impl(obj, col, 255)
#define set_color_3(obj, col, opa) set_color_impl(obj, col, opa)

// Kullanım
SET_COLOR(btn);                  // Calls set_color_1
SET_COLOR(btn, RED);             // Calls set_color_2
SET_COLOR(btn, RED, 128);        // Calls set_color_3
```

---

## Makro Best Practice'leri

### 1. **Parametre Parantezleme**

```c
// YANLIŞ
#define SQUARE(x) x * x

int result = SQUARE(3 + 2);
// Expands: 3 + 2 * 3 + 2 = 3 + 6 + 2 = 11  ❌

// DOĞRU
#define SQUARE(x) ((x) * (x))

int result = SQUARE(3 + 2);
// Expands: ((3 + 2) * (3 + 2)) = 25  ✅
```

**Kural:** Her parametre ve tüm expression parantez içinde.

---

### 2. **Yan Etki Önleme**

```c
// YANLIŞ - Yan etki problemi
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int x = 5, y = 10;
int max = MAX(x++, y++);  // x ve y iki kez artırılır! ⚠️

// ÇÖZÜM 1: Inline function (C99+)
static inline int max(int a, int b) {
    return a > b ? a : b;
}

// ÇÖZÜM 2: GNU Statement Expression
#define MAX(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b; \
})
```

**Kural:** Makro parametreleri birden fazla kez değerlendirilebilir - yan etkili expression'lardan kaçın.

---

### 3. **İsimlendirme Konvansiyonları**

```c
// Public API - ALL CAPS
#define LV_MIN(a, b)        ((a) < (b) ? (a) : (b))
#define LV_COLOR_MAKE(r, g, b)

// Internal API - Leading underscore
#define _LV_CONCAT(x, y)    x ## y
#define _lv_log_add(...)

// Helper macros - UPPER_CASE
#define GET_MACRO_3(...)
```

**Kural:**
- Public → `LV_NAME`
- Internal → `_LV_NAME` veya `_lv_name`
- Constants → `LV_CONST_NAME`

---

### 4. **do-while(0) Kullanımı**

```c
// Çok satırlı makro
#define COMPLEX_MACRO(x) do { \
    statement1(x); \
    statement2(x); \
    statement3(x); \
} while(0)

// Tek satırlık makro - do-while gerekmez
#define SIMPLE_MACRO(x) simple_func(x)

// Disabled macro - do-while(0) kullan
#define DISABLED_MACRO(...) do{}while(0)
```

**Kural:** Multi-statement makrolar için her zaman `do { ... } while(0)`.

---

### 5. **Header Guard'lar**

```c
// Traditional header guard
#ifndef LV_OBJ_H
#define LV_OBJ_H

// Header content...

#endif /* LV_OBJ_H */

// Modern alternative (non-standard but widely supported)
#pragma once

// Header content...
```

LVGL traditional guard kullanıyor - daha portable.

---

### 6. **Include Guards vs Forward Declarations**

```c
// lv_obj.h
#ifndef LV_OBJ_H
#define LV_OBJ_H

// Forward declaration - circular dependency'yi önler
struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

// Tam tanım
struct _lv_obj_t {
    // ...
};

#endif
```

---

## Özet ve Çıkarımlar

### Makro Kategorileri ve Kullanım Alanları

| Kategori | Kullanım Sıklığı | Ana Fayda | Örnek |
|----------|------------------|-----------|-------|
| **Type Safety** | Yüksek | Runtime validation | `LV_ASSERT_OBJ` |
| **Utility** | Çok Yüksek | Kod kısalığı | `LV_MIN`, `LV_MAX` |
| **Configuration** | Orta | Platform esnekliği | `LV_USE_*` |
| **Bit Manipulation** | Yüksek | Bellek verimliliği | `LV_OBJ_FLAG_*` |
| **Code Generation** | Orta | Compile-time polymorphism | `LV_CONCAT` |
| **Debug/Logging** | Yüksek | Development support | `LV_LOG_*` |

---

### LVGL'de Makro Kullanımının Güçlü Yönleri

#### 1. **Sıfır Maliyet Abstraksiyonu**

```c
// Debug build
#define LV_ASSERT_OBJ(obj, class) /* full validation */

// Release build
#define LV_ASSERT_OBJ(obj, class) do{}while(0)  // ZERO CODE!
```

**Sonuç:** Development'ta tam validation, production'da sıfır overhead.

#### 2. **Compile-Time Polymorphism**

```c
#define LV_COLOR_MAKE(r, g, b) \
    LV_CONCAT3(lv_color_make, LV_COLOR_DEPTH, _bit)(r, g, b)

// LV_COLOR_DEPTH=16 → lv_color_make16_bit()
// LV_COLOR_DEPTH=32 → lv_color_make32_bit()
```

**Sonuç:** Tek API, farklı implementasyonlar - runtime overhead yok.

#### 3. **Flexible Configuration**

```c
// Özellik bazlı derleme
#if LV_USE_ANIMATION
    // Animation kodu
#endif

#if LV_USE_GPU
    // GPU acceleration kodu
#endif
```

**Sonuç:** Her platform için optimize edilmiş binary.

#### 4. **Type Safety in C**

```c
LV_ASSERT_OBJ(obj, &lv_slider_class);
// Runtime'da tip kontrolü - C++'taki dynamic_cast gibi
```

**Sonuç:** C'nin zayıf type system'i güçlendirilmiş.

---

### Öğrenilecek Teknikler

#### 🎯 **do-while(0) Pattern**
Multi-statement makroları güvenli hale getirir. Her C programcısının bilmesi gereken temel pattern.

#### 🎯 **Token Pasting ve Stringification**
`##` ve `#` operatörleri ile compile-time code generation. Meta-programming için kritik.

#### 🎯 **Variadic Macros**
`__VA_ARGS__` ile esnek API'ler (logging, assertion, vb.).

#### 🎯 **X-Macros Pattern**
Kod tekrarını önleyen güçlü pattern. Enum + string table + dispatch table gibi related data'yı sync tutar.

#### 🎯 **Conditional Compilation**
Platform/feature bazlı derleme. Embedded sistemler için hayati.

#### 🎯 **Compile-Time Assertions**
`_Static_assert` ile hataları compile-time'da yakala.

---

### Makro Kullanımında Dikkat Edilecekler

#### ⚠️ **Yan Etki Problemi**
```c
MAX(a++, b++)  // Parametreler birden fazla kez evaluate edilir!
```
**Çözüm:** Inline function kullan veya dokümante et.

#### ⚠️ **Operator Precedence**
```c
#define BAD(x) x * 2
BAD(3 + 4)  // = 3 + 4 * 2 = 11 ❌

#define GOOD(x) ((x) * 2)
GOOD(3 + 4)  // = ((3 + 4) * 2) = 14 ✅
```
**Çözüm:** Her parametre parantez içinde.

#### ⚠️ **Debugging Zorluğu**
Makrolar expand olduktan sonra debugger'da görünmez.
**Çözüm:** Inline function alternatifi düşün (modern compiler'lar optimize eder).

#### ⚠️ **Namespace Pollution**
```c
#define MAX(a, b)  // Tüm kodda MAX rezerve edilir!
```
**Çözüm:** Prefix kullan (`LV_MAX`).

---

### Modern C vs Macro Tradeoffs

| Özellik | Macro | Inline Function |
|---------|-------|-----------------|
| **Type Safety** | ❌ Yok | ✅ Var |
| **Debugging** | ❌ Zor | ✅ Kolay |
| **Conditional Compilation** | ✅ Var | ❌ Yok |
| **Generic (Type Agnostic)** | ✅ Evet | ❌ Hayır (C11 _Generic ile kısmen) |
| **Side Effects** | ⚠️ Dikkat gerekir | ✅ Güvenli |
| **Code Size** | ✅ Minimal | ✅ Minimal (modern compiler) |
| **Compile Time** | ⚠️ Yavaşlatabilir | ✅ Hızlı |

---

### Sonuç

LVGL'deki makro kullanımı, **C preprocessor'ın güçlü ve doğru kullanımına mükemmel bir örnektir**.

**Başarılı olduğu alanlar:**
- ✅ Sıfır overhead abstractions
- ✅ Compile-time polymorphism
- ✅ Platform flexibility
- ✅ Type safety enhancement
- ✅ Debug/release differentiation

**Dikkatli kullanılan yerler:**
- ✅ Parametre parantezleme - her zaman
- ✅ do-while(0) pattern - çok satırlı makrolar için
- ✅ Naming conventions - namespace collision önleme
- ✅ Documentation - yan etki uyarıları

Bu makro teknikleri, **embedded system programming için hayati** olup, LVGL'nin başarısında önemli rol oynuyor.

---

## Ek Kaynaklar

### Anahtar Dosyalar

**Assertion ve Validation:**
- `src/misc/lv_assert.h:37-73` - Assertion sistemi
- `src/core/lv_obj.h:388-396` - Object validation

**Utility ve Math:**
- `src/misc/lv_math.h:121-137` - Math utilities
- `src/misc/lv_types.h:77-85` - Token pasting/stringify

**Logging:**
- `src/misc/lv_log.h:89-147` - Logging sistemi

**Color System:**
- `src/misc/lv_color.h:155-163` - Color macros

**Core Object System:**
- `src/core/lv_obj.h:90-121` - Object flags
- `src/core/lv_obj.h:380-395` - Object states

**Module Traces:**
- `src/core/lv_event.c:43` - EVENT_TRACE
- `src/core/lv_refr.c:90` - REFR_TRACE
- `src/core/lv_indev.c:55` - INDEV_TRACE

---

### C Preprocessor Referansları

- **GCC Preprocessor Manual**: https://gcc.gnu.org/onlinedocs/cpp/
- **C99 Standard (ISO/IEC 9899:1999)**: Preprocessor directive specifications
- **X-Macros Pattern**: https://en.wikipedia.org/wiki/X_Macro

---

**Doküman Tarihi**: 2025-11-16
**Analiz Edilen Modül**: `/src/core` ve ilgili `/src/misc` header'ları
**LVGL Versiyonu**: v8.3.5+
