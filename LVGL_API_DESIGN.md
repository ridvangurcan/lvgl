# LVGL API Tasarımı ve En İyi Uygulamalar

## İçindekiler

1. [Giriş](#giriş)
2. [Opaque Pointers (Opak İşaretçiler)](#1-opaque-pointers-opak-işaretçiler)
3. [Getter/Setter Pattern (Alıcı/Ayarlayıcı Deseni)](#2-gettersetter-pattern-alıcıayarlayıcı-deseni)
4. [Internal vs Public API Separation (Dahili-Harici API Ayrımı)](#3-internal-vs-public-api-separation-dahili-harici-api-ayrımı)
5. [Const Correctness (Const Doğruluğu)](#4-const-correctness-const-doğruluğu)
6. [Stratejilerin Birlikte Kullanımı](#stratejilerin-birlikte-kullanımı)
7. [Sonuç](#sonuç)

---

## Giriş

Bu doküman, LVGL (Light and Versatile Graphics Library) kütüphanesinde kullanılan profesyonel API tasarım stratejilerini inceler. LVGL, C dilinde yazılmış olmasına rağmen, modern ve güvenli bir API tasarımı sunar. Bu tasarım, aşağıdaki temel prensiplere dayanır:

- **Enkapsülasyon**: İmplementasyon detaylarını gizleme
- **Tip Güvenliği**: Derleme zamanında hata yakalama
- **API Berraklığı**: Kullanımı kolay ve anlaşılır arayüzler
- **Geriye Dönük Uyumluluk**: İç implementasyonu değiştirirken API'yi koruma

LVGL'nin `/src/core` dizininde bu stratejilerin nasıl uygulandığını detaylı olarak inceleyeceğiz.

---

## 1. Opaque Pointers (Opak İşaretçiler)

### 1.1. Kavram

**Opaque Pointer** (Opak İşaretçi), içeriği kullanıcıdan gizlenmiş bir veri yapısına işaret eden pointer'dır. Kullanıcı bu pointer'ı kullanabilir ancak içindeki alanlara doğrudan erişemez.

### 1.2. Faydaları

```
✓ İmplementasyon detaylarını gizler
✓ ABI (Application Binary Interface) kararlılığı sağlar
✓ Struct değişikliklerinde yeniden derleme gerektirmez
✓ Kullanıcıyı iç yapıdan korur
✓ Kütüphane geliştiricilerine esneklik sağlar
```

### 1.3. LVGL'de Kullanımı

LVGL, iki tür opaque pointer stratejisi kullanır:

#### Pattern A: Tamamen Opak (Fully Opaque)

Struct tanımı tamamen `.c` dosyasında saklanır, header'da sadece forward declaration bulunur.

**Örnek 1: Event Descriptor**

📁 **Header**: `src/core/lv_event.h:31`
```c
// Sadece forward declaration
struct _lv_event_dsc_t;
```

📁 **Implementation**: `src/core/lv_event.c:20-24`
```c
// Gerçek tanım sadece .c dosyasında
typedef struct _lv_event_dsc_t {
    lv_event_cb_t cb;           // Callback fonksiyonu
    void * user_data;           // Kullanıcı verisi
    lv_event_code_t filter : 8; // Event filtresi (bitfield)
} lv_event_dsc_t;
```

**Kullanım**: `src/core/lv_event.h:156`
```c
// Kullanıcı sadece pointer alır, içeriğe erişemez
struct _lv_event_dsc_t * lv_obj_add_event_cb(
    struct _lv_obj_t * obj,
    lv_event_cb_t event_cb,
    lv_event_code_t filter,
    void * user_data
);
```

#### Pattern B: Yarı-Opak (Semi-Opaque)

Struct tanımı header'da görünür ancak `_lv_` prefix ile internal olduğu belirtilir.

**Örnek 2: Object Structure**

📁 **Forward Declaration**: `src/core/lv_obj.h:35`
```c
struct _lv_obj_t;  // İlk deklare edilir
```

📁 **Full Definition**: `src/core/lv_obj.h:174-191`
```c
// Tanım header'da ama underscore ile internal işaretli
typedef struct _lv_obj_t {
    const lv_obj_class_t * class_p;      // Sınıf işaretçisi
    struct _lv_obj_t * parent;            // Ebeveyn nesne
    _lv_obj_spec_attr_t * spec_attr;      // Özel özellikler (lazy)
    _lv_obj_style_t * styles;             // Stiller

    #if LV_USE_USER_DATA
    void * user_data;                     // Kullanıcı verisi
    #endif

    lv_area_t coords;                     // Koordinatlar
    lv_obj_flag_t flags;                  // Bayraklar
    lv_state_t state;                     // Durum

    // Bit alanları (memory optimization)
    uint16_t layout_inv : 1;
    uint16_t scr_layout_inv : 1;
    uint16_t skip_trans : 1;
    uint16_t style_cnt  : 6;
    uint16_t h_layout   : 1;
    uint16_t w_layout   : 1;
} lv_obj_t;
```

**Neden Header'da?**
- Bazı inline fonksiyonlar bu struct'a erişim gerektirir
- Performance critical operasyonlar için
- Ancak `_lv_` prefix ile "internal" olduğu belirtilir

**Örnek 3: Group Structure**

📁 **Definition**: `src/core/lv_group.h:59-78`
```c
typedef struct _lv_group_t {
    lv_ll_t obj_ll;                       // Obje listesi (linked list)
    struct _lv_obj_t ** obj_focus;        // Odaklanılan obje
    lv_group_focus_cb_t focus_cb;         // Odak callback'i
    lv_group_edge_cb_t  edge_cb;          // Kenar callback'i

    #if LV_USE_USER_DATA
    void * user_data;
    #endif

    // Bit alanları ile bellek optimizasyonu
    uint8_t frozen : 1;           // Dondurulmuş mu?
    uint8_t editing : 1;          // Düzenleme modunda mı?
    uint8_t refocus_policy : 1;   // Yeniden odaklanma politikası
    uint8_t wrap : 1;             // Döngüsel navigasyon?
} lv_group_t;
```

### 1.4. Opaque Pointer ile API Tasarımı

LVGL, opaque pointer'ları kullanarak şu şekilde bir API tasarımı oluşturur:

```c
// Kullanıcı perspektifi:
lv_obj_t * obj = lv_obj_create(NULL);  // Pointer alır
lv_obj_set_width(obj, 100);             // Pointer ile çalışır
lv_obj_set_height(obj, 50);             // İç detayları bilmez

// obj->coords.x1 = 10;  // ❌ YAPILMAMALI (ama mümkün çünkü semi-opaque)
lv_obj_set_x(obj, 10);   // ✓ DOĞRU - API kullan
```

### 1.5. Diğer Örnekler

📁 `src/core/lv_obj_class.h:29-31` - Forward declarations:
```c
struct _lv_obj_t;
struct _lv_obj_class_t;
struct _lv_event_t;
```

📁 `src/core/lv_obj_tree.h:30-31` - Tree yapısı için:
```c
struct _lv_obj_t;
struct _lv_obj_class_t;
```

📁 `src/core/lv_obj_pos.h:25` - Position modülü:
```c
struct _lv_obj_t;
```

### 1.6. Avantajlar ve Zorluklar

| Avantaj | Açıklama |
|---------|----------|
| **ABI Stability** | Struct değişse bile binary uyumluluk korunur |
| **Encapsulation** | İç detaylar gizlidir, kullanıcı yanlış kullanamaz |
| **Flexibility** | İç implementasyon serbestçe değiştirilebilir |
| **Compile Time** | Header değişmediği için yeniden derleme azalır |

| Zorluk | Çözüm |
|--------|-------|
| Debug zorluğu | Debug build'de assertion'lar ve logging |
| Memory yönetimi | Kütüphane allocation/free kontrol eder |
| Type casting | Strict type checking ile güvenlik sağlanır |

---

## 2. Getter/Setter Pattern (Alıcı/Ayarlayıcı Deseni)

### 2.1. Kavram

**Getter/Setter Pattern**, nesne özelliklerine doğrudan erişim yerine fonksiyonlar aracılığıyla erişim sağlayan bir desendir.

```c
// ❌ Doğrudan erişim (kötü)
obj->width = 100;

// ✓ Setter ile erişim (iyi)
lv_obj_set_width(obj, 100);
```

### 2.2. Faydaları

```
✓ Validation (doğrulama) imkanı
✓ Side effects (yan etkiler) yönetimi
✓ Lazy computation (tembel hesaplama)
✓ API değişikliklerinde esneklik
✓ Debugging ve logging kolaylığı
✓ Thread safety eklenebilir
```

### 2.3. LVGL'de Setter Fonksiyonları

LVGL, tutarlı bir adlandırma konvansiyonu kullanır:

#### Temel Setter Pattern'leri

**Pattern 1: `lv_obj_set_*` - Değer ayarlama**

📁 `src/core/lv_obj_pos.h:47-69`
```c
// Pozisyon ayarlama
void lv_obj_set_pos(struct _lv_obj_t * obj, lv_coord_t x, lv_coord_t y);
void lv_obj_set_x(struct _lv_obj_t * obj, lv_coord_t x);
void lv_obj_set_y(struct _lv_obj_t * obj, lv_coord_t y);

// Boyut ayarlama
void lv_obj_set_size(struct _lv_obj_t * obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_width(struct _lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_height(struct _lv_obj_t * obj, lv_coord_t h);
```

**Pattern 2: `lv_obj_add_*` - Bayrak/özellik ekleme**

📁 `src/core/lv_obj.h:236-243`
```c
// Flag ekleme (bitwise OR)
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f);

// State ekleme
void lv_obj_add_state(lv_obj_t * obj, lv_state_t state);
```

**Pattern 3: `lv_obj_clear_*` - Bayrak/özellik temizleme**

📁 `src/core/lv_obj.h:236-260`
```c
// Flag temizleme (bitwise AND NOT)
void lv_obj_clear_flag(lv_obj_t * obj, lv_obj_flag_t f);

// State temizleme
void lv_obj_clear_state(lv_obj_t * obj, lv_state_t state);
```

#### Setter İmplementasyon Örneği

📁 `src/core/lv_obj.c:324-334`
```c
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);  // 1. Validasyon

    bool was_on_layout = lv_obj_is_layout_positioned(obj);

    obj->flags |= f;  // 2. Değer değiştir

    // 3. Side effects - flag değişikliklerinin sonuçları
    if(!was_on_layout && lv_obj_is_layout_positioned(obj)) {
        // Layout flag eklendiyse, parent'ı güncelle
        lv_obj_mark_layout_as_dirty(lv_obj_get_parent(obj));
        lv_obj_mark_layout_as_dirty(obj);
    }

    // 4. Flag HIDDEN eklenirse invalidation gerekir
    if((f & LV_OBJ_FLAG_HIDDEN) != 0) {
        lv_obj_invalidate(obj);
    }
}
```

**Setter'ın Sağladığı Faydalar:**
1. ✓ **Assertion** - Geçersiz pointer kontrolü
2. ✓ **Lazy evaluation** - Layout sadece gerekirse güncellenir
3. ✓ **Side effects** - HIDDEN flag eklenince ekranı invalidate eder
4. ✓ **Encapsulation** - Kullanıcı internal flag manipülasyonundan habersiz

#### Style Setter'ları (Auto-Generated)

LVGL, style API'lerini otomatik generate eder:

📁 `src/core/lv_obj_style_gen.c:3-59`
```c
// Makrolar ile otomatik üretilmiş setter'lar
void lv_obj_set_style_width(struct _lv_obj_t * obj,
                            lv_coord_t value,
                            lv_style_selector_t selector)
{
    lv_obj_set_local_style_prop(obj, LV_STYLE_WIDTH,
                                (lv_style_value_t) {.num = (int32_t)value},
                                selector);
}

void lv_obj_set_style_height(struct _lv_obj_t * obj,
                             lv_coord_t value,
                             lv_style_selector_t selector)
{
    lv_obj_set_local_style_prop(obj, LV_STYLE_HEIGHT,
                                (lv_style_value_t) {.num = (int32_t)value},
                                selector);
}

void lv_obj_set_style_x(struct _lv_obj_t * obj,
                        lv_coord_t value,
                        lv_style_selector_t selector)
{
    lv_obj_set_local_style_prop(obj, LV_STYLE_X,
                                (lv_style_value_t) {.num = (int32_t)value},
                                selector);
}
```

**Avantaj**: Yüzlerce style property için manuel kod yazmak yerine, tek bir template'ten generate edilir.

#### Parent Setter

📁 `src/core/lv_obj_tree.h:88`
```c
void lv_obj_set_parent(struct _lv_obj_t * obj, struct _lv_obj_t * parent);
```

Bu setter, complex side effects içerir:
- Eski parent'tan nesneyi çıkarır
- Yeni parent'a ekler
- Layout'u invalidate eder
- Koordinatları yeniden hesaplar

### 2.4. LVGL'de Getter Fonksiyonları

#### Temel Getter Pattern'leri

**Pattern 1: `lv_obj_get_*` - Değer okuma**

📁 `src/core/lv_obj.h:299`
```c
lv_state_t lv_obj_get_state(const lv_obj_t * obj);
```

📁 `src/core/lv_obj.c:398-403` - İmplementasyon:
```c
lv_state_t lv_obj_get_state(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);  // Validasyon
    return obj->state;              // Basit okuma
}
```

**Pattern 2: Pozisyon/Boyut Getter'ları**

📁 `src/core/lv_obj_pos.h:210-290`
```c
// Koordinatları okuma
void lv_obj_get_coords(const struct _lv_obj_t * obj, lv_area_t * coords);
lv_coord_t lv_obj_get_x(const struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_y(const struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_width(const struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_height(const struct _lv_obj_t * obj);

// Content boyutları (padding dahil değil)
lv_coord_t lv_obj_get_content_width(const struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_content_height(const struct _lv_obj_t * obj);
```

**Pattern 3: Class Getter**

📁 `src/core/lv_obj.h:360`
```c
const lv_obj_class_t * lv_obj_get_class(const lv_obj_t * obj);
```

📁 `src/core/lv_obj.c:459-462`
```c
const lv_obj_class_t * lv_obj_get_class(const lv_obj_t * obj)
{
    return obj->class_p;  // const pointer döner
}
```

#### Inline Getter'lar (Performance)

Bazı getter'lar performans için inline yapılır:

📁 `src/core/lv_obj_style_gen.h:4-100`
```c
// Auto-generated inline getter'lar
static inline lv_coord_t lv_obj_get_style_width(
    const struct _lv_obj_t * obj,
    uint32_t part)
{
    lv_style_value_t v = lv_obj_get_style_prop(obj, part, LV_STYLE_WIDTH);
    return (lv_coord_t)v.num;
}

static inline lv_coord_t lv_obj_get_style_height(
    const struct _lv_obj_t * obj,
    uint32_t part)
{
    lv_style_value_t v = lv_obj_get_style_prop(obj, part, LV_STYLE_HEIGHT);
    return (lv_coord_t)v.num;
}

static inline const lv_font_t * lv_obj_get_style_text_font(
    const struct _lv_obj_t * obj,
    uint32_t part)
{
    lv_style_value_t v = lv_obj_get_style_prop(obj, part, LV_STYLE_TEXT_FONT);
    return (const lv_font_t *)v.ptr;
}
```

**Avantaj**: Sık kullanılan getter'lar function call overhead'i olmadan çalışır.

#### Scroll Getter'ları

📁 `src/core/lv_obj_scroll.h:126-172`
```c
// Scroll pozisyonu okuma
lv_coord_t lv_obj_get_scroll_x(const struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_y(const struct _lv_obj_t * obj);

// Scroll sınırları
lv_coord_t lv_obj_get_scroll_top(struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_bottom(struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_left(struct _lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_right(struct _lv_obj_t * obj);
```

### 2.5. Boolean Query Pattern (has_* pattern)

LVGL, boolean sorgular için `has_*` prefix kullanır:

**Pattern 1: Flag Sorguları**

📁 `src/core/lv_obj.h:284-292`
```c
// Tüm flagler set mi? (AND)
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f);

// Herhangi bir flag set mi? (OR)
bool lv_obj_has_flag_any(const lv_obj_t * obj, lv_obj_flag_t f);
```

📁 `src/core/lv_obj.c:384-396` - İmplementasyon:
```c
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return (obj->flags & f) == f ? true : false;  // Tüm bitler set mi?
}

bool lv_obj_has_flag_any(const lv_obj_t * obj, lv_obj_flag_t f)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return (obj->flags & f) ? true : false;  // Herhangi bir bit set mi?
}
```

**Pattern 2: State Sorguları**

📁 `src/core/lv_obj.h:307`
```c
bool lv_obj_has_state(const lv_obj_t * obj, lv_state_t state);
```

**Pattern 3: Type Checking**

📁 `src/core/lv_obj.h:344-353`
```c
// Tam tip kontrolü
bool lv_obj_check_type(const lv_obj_t * obj, const lv_obj_class_t * class_p);

// Inheritance kontrolü (base class'lar dahil)
bool lv_obj_has_class(const lv_obj_t * obj, const lv_obj_class_t * class_p);
```

### 2.6. Getter/Setter Kullanım Örnekleri

```c
// Obje oluştur
lv_obj_t * btn = lv_obj_create(lv_scr_act());

// Setter'larla yapılandır
lv_obj_set_size(btn, 120, 50);                    // Boyut
lv_obj_set_pos(btn, 10, 10);                       // Pozisyon
lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);      // Flag ekle
lv_obj_add_state(btn, LV_STATE_CHECKED);          // State ekle

// Getter'larla oku
lv_coord_t w = lv_obj_get_width(btn);             // width = 120
lv_coord_t h = lv_obj_get_height(btn);            // height = 50
lv_state_t s = lv_obj_get_state(btn);             // state içinde CHECKED var

// Boolean sorguları
bool is_clickable = lv_obj_has_flag(btn, LV_OBJ_FLAG_CLICKABLE);  // true
bool is_checked = lv_obj_has_state(btn, LV_STATE_CHECKED);         // true

// Style getter/setter
lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0);
lv_color_t bg = lv_obj_get_style_bg_color(btn, 0);
```

### 2.7. Getter/Setter Pattern Avantajları

| Avantaj | Örnek |
|---------|-------|
| **Validation** | `lv_obj_set_width()` negatif değerleri reddedebilir |
| **Side Effects** | `lv_obj_set_parent()` layout'u otomatik günceller |
| **Lazy Computation** | `lv_obj_get_content_width()` padding'i hesaplar |
| **Caching** | Getter sonuçları cache'lenebilir |
| **Debugging** | Breakpoint koyma ve logging kolay |
| **API Stability** | İç struct değişse API aynı kalır |

---

## 3. Internal vs Public API Separation (Dahili-Harici API Ayrımı)

### 3.1. Kavram

API'leri iki kategoriye ayırma stratejisidir:

- **Public API**: Kullanıcıların kullanması gereken, stabil, dokümante edilmiş fonksiyonlar
- **Internal API**: Kütüphane içinde kullanılan, değişebilir, dokümante edilmemiş fonksiyonlar

### 3.2. Neden Gereklidir?

```
✓ API yüzeyini küçültür (daha az öğrenme)
✓ Geriye dönük uyumluluk kolaylaşır
✓ İç değişikliklerde özgürlük sağlar
✓ Kullanıcı yanlış fonksiyon kullanmaz
✓ Dokümantasyon yükü azalır
```

### 3.3. LVGL'nin Adlandırma Konvansiyonu

LVGL, **prefix-based** bir strateji kullanır:

| Prefix | Anlamı | Kullanım |
|--------|--------|----------|
| `lv_*` | **Public** | Kullanıcılar için, stabil API |
| `_lv_*` | **Internal** | Sadece kütüphane içi, değişebilir |

**Kural:**
```c
// ✓ Public - Kullanıcılar kullanabilir
lv_obj_t * lv_obj_create(lv_obj_t * parent);

// ❌ Internal - Kullanıcılar kullanmamalı
void _lv_obj_destruct(lv_obj_t * obj);
```

### 3.4. Internal Fonksiyon Örnekleri

#### Object Lifecycle (Dahili)

📁 `src/core/lv_obj_class.h:86`
```c
// Internal - Obje yıkımı (kullanıcı lv_obj_del kullanmalı)
void _lv_obj_destruct(struct _lv_obj_t * obj);
```

#### Refresh System (Dahili)

📁 `src/core/lv_refr.h:48`
```c
// Internal - Refresh sistemini başlatır
void _lv_refr_init(void);
```

📁 `src/core/lv_refr.h:72`
```c
// Internal - Ekran alanını invalidate eder
void _lv_inv_area(lv_disp_t * disp, const lv_area_t * area_p);
```

📁 `src/core/lv_refr.h:105`
```c
// Internal - Display refresh timer callback
void _lv_disp_refr_timer(lv_timer_t * timer);
```

#### Event System (Dahili)

📁 `src/core/lv_event.h:236`
```c
// Internal - Silinen objeleri event stack'te işaretle
void _lv_event_mark_deleted(struct _lv_obj_t * obj);
```

📁 `src/core/lv_event.c:151-159` - İmplementasyon:
```c
void _lv_event_mark_deleted(lv_obj_t * obj)
{
    // Event zincirinde bu objeyi "deleted" olarak işaretle
    lv_event_t * e = event_head;
    while(e) {
        if(e->current_target == obj || e->target == obj) {
            e->deleted = 1;  // Silindi işareti
        }
        e = e->prev;
    }
}
```

**Neden Internal?**
- Event dispatch sırasında çağrılır
- Kullanıcının direkt kullanması gereksiz ve tehlikeli
- API değişikliklerinde özgürce değiştirilebilir

#### Group System (Dahili)

📁 `src/core/lv_group.h:94`
```c
// Internal - Group sistemini başlatır
void _lv_group_init(void);
```

📁 `src/core/lv_group.h:90-94` - Dokümantasyon:
```c
/**
 * Init. the group module
 * @remarks Internal function, do not call directly.
 */
void _lv_group_init(void);
```

**Önemli**: LVGL, internal fonksiyonların dokümantasyonunda açıkça uyarır.

#### Style System (Dahili)

📁 `src/core/lv_obj_style.h:71`
```c
// Internal - Style sistemini başlatır
void _lv_obj_style_init(void);
```

📁 `src/core/lv_obj_style.h:186`
```c
// Internal - Style transition oluşturur
void _lv_obj_style_create_transition(
    struct _lv_obj_t * obj,
    lv_part_t part,
    lv_state_t prev_state,
    lv_state_t new_state,
    uint32_t time,
    uint32_t delay
);
```

#### Input Device (Dahili)

📁 `src/core/lv_indev_scroll.h:34-40`
```c
// Internal - Scroll işleme
void _lv_indev_scroll_handler(_lv_indev_proc_t * proc);

// Internal - Scroll throw (관성 스크롤) işleme
void _lv_indev_scroll_throw_handler(_lv_indev_proc_t * proc);
```

### 3.5. Public Fonksiyon Örnekleri

#### Object Creation (Public)

📁 `src/core/lv_obj.h:224`
```c
// Public - Kullanıcılar obje oluşturmak için kullanır
lv_obj_t * lv_obj_create(lv_obj_t * parent);
```

#### Library Initialization (Public)

📁 `src/core/lv_obj.h:202`
```c
// Public - LVGL'i başlatır
void lv_init(void);
```

#### Event Sending (Public)

📁 `src/core/lv_event.h:158`
```c
// Public - Event gönderimi
lv_res_t lv_event_send(
    struct _lv_obj_t * obj,
    lv_event_code_t event_code,
    void * param
);
```

#### Object Deletion (Public)

📁 `src/core/lv_obj.h:269`
```c
// Public - Obje silme
void lv_obj_del(lv_obj_t * obj);
```

**Internal karşılığı**: `_lv_obj_destruct()` kullanılır ama kullanıcı `lv_obj_del()` kullanır.

### 3.6. Internal Type Naming

LVGL, internal tipler için de `_lv_` prefix kullanır:

📁 `src/core/lv_obj.h:172`
```c
// Internal type - Özel özellikler struct'ı
typedef struct {
    struct _lv_obj_t ** children;        // Çocuk objeler
    uint32_t child_cnt;                  // Çocuk sayısı
    lv_group_t * group_p;                // Group işaretçisi
    struct _lv_event_dsc_t * event_dsc;  // Event descriptor'lar

    lv_point_t scroll;                   // Scroll pozisyonu
    lv_coord_t ext_click_pad;            // Genişletilmiş click alanı
    lv_coord_t ext_draw_size;            // Genişletilmiş draw alanı

    // Bit alanları (memory optimization)
    lv_scrollbar_mode_t scrollbar_mode : 2;
    lv_scroll_snap_t scroll_snap_x : 2;
    lv_scroll_snap_t scroll_snap_y : 2;
    lv_dir_t scroll_dir : 4;
    uint8_t layer_type : 2;

} _lv_obj_spec_attr_t;  // ← Underscore ile internal işaretli
```

### 3.7. Header Organization

LVGL, internal ve public API'leri bazen ayrı header'larda tutar:

```
lv_obj.h          → Public API
lv_obj_private.h  → Private/Internal declarations (varsa)
```

### 3.8. Stratejinin Avantajları

| Avantaj | Açıklama |
|---------|----------|
| **Clear Contract** | Kullanıcı hangi API'yi kullanacağını bilir |
| **Refactoring Safety** | Internal API değişse kullanıcı etkilenmez |
| **Documentation** | Sadece public API dokümante edilir |
| **Testing** | Public API test edilir, internal değişebilir |
| **Versioning** | Public API semantic versioning takip eder |

### 3.9. Kullanım Örnekleri

```c
// ✓ DOĞRU - Public API kullan
void my_app(void)
{
    lv_init();                              // Public init

    lv_obj_t * obj = lv_obj_create(NULL);  // Public create
    lv_obj_set_size(obj, 100, 50);         // Public setter

    lv_event_send(obj, LV_EVENT_CLICKED, NULL);  // Public event

    lv_obj_del(obj);                       // Public delete
}

// ❌ YANLIŞ - Internal API kullanma
void my_app_wrong(void)
{
    _lv_refr_init();                       // ❌ Internal init

    lv_obj_t * obj = lv_obj_create(NULL);
    _lv_obj_destruct(obj);                 // ❌ Internal destruct

    _lv_inv_area(NULL, NULL);              // ❌ Internal invalidate
}
```

### 3.10. Internal API'lerin Değişkenliği

Internal API'ler versiyon güncellemelerinde değişebilir:

```c
// Version 8.0
void _lv_refr_init(void);

// Version 8.1 - İsim değişebilir
void _lv_refr_initialize(lv_disp_t * disp);

// Version 8.2 - Parametre eklenebilir
void _lv_refr_initialize(lv_disp_t * disp, uint32_t flags);
```

**Public API** ise stabil kalır:
```c
// Version 8.0, 8.1, 8.2 - Hep aynı
void lv_init(void);
```

---

## 4. Const Correctness (Const Doğruluğu)

### 4.1. Kavram

**Const Correctness**, `const` keyword'ünü doğru kullanarak:
- Read-only veriyi koruma
- İstenmeyen değişiklikleri derleme zamanında yakalama
- API'nin intent'ini belgeleme

stratejisidir.

### 4.2. Faydaları

```
✓ Compiler optimizasyonları
✓ Bug'ları derleme zamanında yakalar
✓ API intent'i belgelenmiş olur
✓ Thread safety analizi kolaylaşır
✓ Const data ROM'a konulabilir (embedded)
```

### 4.3. LVGL'de Const Kullanımları

#### Pattern 1: Const Parameters (Read-Only Objects)

Fonksiyon objeyi değiştirmeyecekse, `const` kullanılır:

📁 `src/core/lv_obj.h:284-292`
```c
// Object sadece okunur, değiştirilmez
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag_any(const lv_obj_t * obj, lv_obj_flag_t f);
```

📁 `src/core/lv_obj.h:299-307`
```c
// State sorguları - sadece okur
lv_state_t lv_obj_get_state(const lv_obj_t * obj);
bool lv_obj_has_state(const lv_obj_t * obj, lv_state_t state);
```

📁 `src/core/lv_obj.h:344-367`
```c
// Type checking - objeyi değiştirmez
bool lv_obj_check_type(const lv_obj_t * obj, const lv_obj_class_t * class_p);
bool lv_obj_has_class(const lv_obj_t * obj, const lv_obj_class_t * class_p);
const lv_obj_class_t * lv_obj_get_class(const lv_obj_t * obj);
bool lv_obj_is_valid(const lv_obj_t * obj);
```

**Avantaj**: Derleyici şöyle bir hatayı yakalar:

```c
void my_function(const lv_obj_t * obj)
{
    lv_obj_set_width(obj, 100);  // ❌ HATA: const obj'yi değiştirmeye çalışıyor

    lv_coord_t w = lv_obj_get_width(obj);  // ✓ OK: Sadece okuyor
}
```

#### Pattern 2: Position Getters ile Const

📁 `src/core/lv_obj_pos.c:481-554`
```c
void lv_obj_get_coords(const lv_obj_t * obj, lv_area_t * coords);
lv_coord_t lv_obj_get_x(const lv_obj_t * obj);
lv_coord_t lv_obj_get_y(const lv_obj_t * obj);
lv_coord_t lv_obj_get_width(const lv_obj_t * obj);
lv_coord_t lv_obj_get_height(const lv_obj_t * obj);
```

#### Pattern 3: Inline Getters ile Const

📁 `src/core/lv_obj.h:378-381`
```c
// DPI hesaplama - obj sadece okunur
static inline lv_coord_t lv_obj_dpx(const lv_obj_t * obj, lv_coord_t n)
{
    return _LV_DPX_CALC(lv_disp_get_dpi(lv_obj_get_disp(obj)), n);
}
```

### 4.4. Const Return Types

Fonksiyon döndürdüğü veriyi değiştirilemez yapmak için `const` döner:

#### Pattern 1: Class Pointer Dönme

📁 `src/core/lv_obj.h:360`
```c
// const pointer döner - class değiştirilemez
const lv_obj_class_t * lv_obj_get_class(const lv_obj_t * obj);
```

📁 `src/core/lv_obj.c:459-462` - İmplementasyon:
```c
const lv_obj_class_t * lv_obj_get_class(const lv_obj_t * obj)
{
    return obj->class_p;  // class_p zaten const lv_obj_class_t *
}
```

**Kullanım:**
```c
const lv_obj_class_t * cls = lv_obj_get_class(obj);

// cls->instance_size = 100;  // ❌ HATA: const class değiştirilemez
uint32_t size = cls->instance_size;  // ✓ OK: Okuyabilirsin
```

#### Pattern 2: Double Const (Pointer to Const + Const Pointer)

📁 `src/core/lv_obj_style_gen.h:214-226`
```c
// Gradient descriptor döner - değiştirilemez
static inline const lv_grad_dsc_t * lv_obj_get_style_bg_grad(
    const struct _lv_obj_t * obj,
    uint32_t part)
{
    lv_style_value_t v = lv_obj_get_style_prop(obj, part, LV_STYLE_BG_GRAD);
    return (const lv_grad_dsc_t *)v.ptr;
}

// Image source döner - değiştirilemez
static inline const void * lv_obj_get_style_bg_img_src(
    const struct _lv_obj_t * obj,
    uint32_t part)
{
    lv_style_value_t v = lv_obj_get_style_prop(obj, part, LV_STYLE_BG_IMG_SRC);
    return (const void *)v.ptr;
}
```

#### Pattern 3: Font Pointer

📁 `src/core/lv_obj_style_gen.h:490`
```c
// Font döner - değiştirilemez
static inline const lv_font_t * lv_obj_get_style_text_font(
    const struct _lv_obj_t * obj,
    uint32_t part)
{
    lv_style_value_t v = lv_obj_get_style_prop(obj, part, LV_STYLE_TEXT_FONT);
    return (const lv_font_t *)v.ptr;
}
```

#### Pattern 4: Transition Descriptor

📁 `src/core/lv_obj_style_gen.h:568`
```c
// Transition descriptor döner
static inline const lv_style_transition_dsc_t * lv_obj_get_style_transition(
    const struct _lv_obj_t * obj,
    uint32_t part)
{
    lv_style_value_t v = lv_obj_get_style_prop(obj, part, LV_STYLE_TRANSITION);
    return (const lv_style_transition_dsc_t *)v.ptr;
}
```

### 4.5. Const in Event System

📁 `src/core/lv_event.h:314`
```c
// Event'ten eski boyutu al - değiştirilemez
const lv_area_t * lv_event_get_old_size(lv_event_t * e);
```

### 4.6. Const Struct Members

Struct içinde değişmemesi gereken alanlar `const` yapılır:

#### Pattern 1: Object Class Pointer

📁 `src/core/lv_obj.h:175`
```c
typedef struct _lv_obj_t {
    const lv_obj_class_t * class_p;  // ← Class oluşturulduktan sonra değişmez
    struct _lv_obj_t * parent;        // Parent değişebilir
    _lv_obj_spec_attr_t * spec_attr;  // Spec attr değişebilir
    // ...
} lv_obj_t;
```

**Avantaj**: Yanlışlıkla class değiştirilmesi engellenir:

```c
lv_obj_t * obj = lv_obj_create(NULL);

// obj->class_p = &my_class;  // ❌ HATA: const member değiştirilemez
```

#### Pattern 2: Class Hierarchy

📁 `src/core/lv_obj_class.h:55-70`
```c
typedef struct _lv_obj_class_t {
    const struct _lv_obj_class_t * base_class;  // ← Base class const

    // Callback'ler const class pointer alır
    void (*constructor_cb)(const struct _lv_obj_class_t * class_p,
                          struct _lv_obj_t * obj);

    void (*destructor_cb)(const struct _lv_obj_class_t * class_p,
                         struct _lv_obj_t * obj);

    void (*event_cb)(const struct _lv_obj_class_t * class_p,
                    struct _lv_event_t * e);

    void * user_data;
    const char * name;                          // ← İsim const
    uint32_t instance_size;
    uint32_t editable : 2;
    uint32_t group_def : 2;
    uint32_t base_class_cnt : 6;
} lv_obj_class_t;
```

**Önemli**: Callback'ler `const` class pointer alır, böylece callback class'ı değiştiremez.

### 4.7. Const Correctness in Function Parameters

Birden fazla `const` parametreli fonksiyonlar:

#### Pattern 1: Area Invalidation

📁 `src/core/lv_obj_pos.h:375`
```c
void lv_obj_invalidate_area(
    const struct _lv_obj_t * obj,   // Object değişmez
    const lv_area_t * area          // Area değişmez
);
```

#### Pattern 2: Text Alignment Calculation

📁 `src/core/lv_obj_style.h:305`
```c
lv_text_align_t lv_obj_calculate_style_text_align(
    const struct _lv_obj_t * obj,   // Object değişmez
    lv_part_t part,
    const char * txt                // Text değişmez
);
```

#### Pattern 3: Style Replacement

📁 `src/core/lv_obj_style.h:92`
```c
bool lv_obj_replace_style(
    struct _lv_obj_t * obj,          // Object değişebilir
    const lv_style_t * old_style,    // Eski style değişmez
    const lv_style_t * new_style,    // Yeni style değişmez
    lv_style_selector_t selector
);
```

### 4.8. Const Correctness in Internal Functions

Internal fonksiyonlar da const correctness kullanır:

📁 `src/core/lv_obj_class.c:29-196`
```c
// Internal helper - class'ı değiştirmez
static uint32_t get_instance_size(const lv_obj_class_t * class_p)
{
    const lv_obj_class_t * base = class_p;
    uint32_t size = 0;

    // Class hierarchy'yi dolaş
    while(base) {
        size = base->instance_size;
        base = base->base_class;  // base_class zaten const
    }

    return size;
}
```

### 4.9. Const Correctness Seviyeleri

LVGL'de farklı const seviyeleri kullanılır:

```c
// Seviye 1: Pointer to const
const lv_obj_t * obj;          // obj'nin içeriği değiştirilemez
obj = another_obj;             // ✓ Pointer kendisi değişebilir

// Seviye 2: Const pointer
lv_obj_t * const obj;          // obj pointer'ı değiştirilemez
obj->width = 100;              // ✓ İçerik değişebilir

// Seviye 3: Const pointer to const
const lv_obj_t * const obj;    // Ne pointer ne içerik değişebilir

// LVGL'de en yaygın: Seviye 1
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f);
```

### 4.10. Const ile API Intent Belgeleme

```c
// Intent: "Bu fonksiyon sadece okur, değiştirmez"
lv_coord_t lv_obj_get_width(const lv_obj_t * obj);

// Intent: "Bu fonksiyon objeyi değiştirir"
void lv_obj_set_width(lv_obj_t * obj, lv_coord_t w);

// Intent: "Class'ı okur, ama değiştirmez, const döner"
const lv_obj_class_t * lv_obj_get_class(const lv_obj_t * obj);
```

### 4.11. Embedded Systemlerde Const Avantajı

```c
// const data ROM'a konulabilir (flash memory)
const lv_font_t font_arial_16 = {
    .dsc = &font_arial_16_dsc,
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    // ...
};

// Non-const data RAM'de olmalı
lv_obj_t obj = {
    .coords = {0, 0, 100, 50},
    // ...
};
```

**Avantaj**: Embedded sistemlerde RAM çok kıymetlidir, const data flash'ta tutulur.

### 4.12. Const Correctness Best Practices

| Practice | Örnek |
|----------|-------|
| **Getter → const param** | `int get_value(const obj_t *obj)` |
| **Setter → non-const param** | `void set_value(obj_t *obj, int val)` |
| **Read-only return → const** | `const char * get_name()` |
| **Callback param → const** | `void cb(const event_t *e)` |
| **Lookup table → const** | `const uint8_t table[] = {...}` |

---

## Stratejilerin Birlikte Kullanımı

LVGL'de bu 4 strateji birlikte kullanılarak güçlü bir API oluşturulur:

### Örnek: Object Creation & Configuration

```c
// 1. Opaque Pointer - Kullanıcı struct detaylarını görmez
lv_obj_t * btn = lv_obj_create(lv_scr_act());

// 2. Getter/Setter - Doğrudan field erişimi yerine API kullan
lv_obj_set_size(btn, 100, 50);               // Setter
lv_coord_t w = lv_obj_get_width(btn);        // Getter

// 3. Internal API - Kullanıcı görmez/kullanmaz
// _lv_obj_destruct(btn);  // ❌ YAPMA - Internal
lv_obj_del(btn);           // ✓ YAP - Public

// 4. Const Correctness - Read-only işlemler
const lv_obj_class_t * cls = lv_obj_get_class(btn);  // const döner
bool has_flag = lv_obj_has_flag(btn, LV_OBJ_FLAG_CLICKABLE);  // const param
```

### Implementasyon Tarafında

```c
// lv_obj.h - Public header
typedef struct _lv_obj_t lv_obj_t;  // ← Opaque forward declaration

// Public API
lv_obj_t * lv_obj_create(lv_obj_t * parent);                    // ← Public
void lv_obj_set_width(lv_obj_t * obj, lv_coord_t w);           // ← Setter
lv_coord_t lv_obj_get_width(const lv_obj_t * obj);             // ← Getter + const

// Internal API
void _lv_obj_destruct(lv_obj_t * obj);                         // ← Internal

// lv_obj.c - Implementation
struct _lv_obj_t {                   // ← Gerçek tanım
    const lv_obj_class_t * class_p;  // ← Const member
    lv_coord_t width;
    // ...
};

lv_coord_t lv_obj_get_width(const lv_obj_t * obj)  // ← Const param
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return obj->width;
}
```

### API Tasarımı Checklist

Yeni bir API fonksiyonu eklerken:

```
□ Opaque mı yoksa exposed struct mı? (Genelde opaque tercih et)
□ Getter mi setter mi? (İsimlendirme: get_*/set_*)
□ Public mi internal mi? (Prefix: lv_* vs _lv_*)
□ Parametreler const olmalı mı? (Read-only ise const)
□ Return type const olmalı mı? (Değiştirilmemeli ise const)
□ Dokümantasyon eklendi mi?
□ Validasyon/assertion var mı?
```

---

## Sonuç

LVGL'nin `/src/core` modülü, profesyonel C API tasarımının mükemmel bir örneğidir. Bu dokümanda incelenen 4 strateji:

### 1. Opaque Pointers
- İmplementasyon detaylarını gizler
- ABI stability sağlar
- Kullanıcıyı internal değişikliklerden korur

### 2. Getter/Setter Pattern
- Validation ve side effect kontrolü
- API stability
- Lazy computation imkanı

### 3. Internal vs Public API Separation
- Açık API kontratı
- Refactoring özgürlüğü
- Dokümantasyon kolaylığı

### 4. Const Correctness
- Derleme zamanı güvenlik
- Compiler optimizasyonları
- API intent belgeleme

Bu stratejiler birlikte kullanıldığında, **sürdürülebilir, güvenli ve kullanımı kolay** bir API ortaya çıkar.

### Öğrenilen Dersler

C dilinde modern API tasarımı için:

1. **Enkapsülasyon için** opaque pointer kullan
2. **Kontrol için** getter/setter pattern kullan
3. **Berraklık için** public/internal ayırımı yap
4. **Güvenlik için** const correctness uygula

LVGL, bu prensipleri uygulayarak embedded sistemler için en popüler GUI kütüphanelerinden biri haline gelmiştir.

---

**Doküman Sürümü**: 1.0
**Tarih**: 2024
**Analiz Edilen LVGL Versiyonu**: v8.x
**İncelenen Dizin**: `/src/core`
