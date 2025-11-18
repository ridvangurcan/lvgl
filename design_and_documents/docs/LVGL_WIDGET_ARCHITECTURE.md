# LVGL Widget Altyapısı: Mimari ve Lifecycle Dokümantasyonu

## İçindekiler
1. [Giriş](#giriş)
2. [lv_obj Mimarisi ve Abstraction Modeli](#lv_obj-mimarisi-ve-abstraction-modeli)
3. [Widget Lifecycle ve Event-Driven Yapı](#widget-lifecycle-ve-event-driven-yapı)
4. [Invalidation → Rendering Pipeline](#invalidation--rendering-pipeline)
5. [Widget Inheritance Mekanizması](#widget-inheritance-mekanizması)
6. [Design Pattern'lar](#design-patternlar)
7. [Yeni Widget Geliştirme: Best Practices](#yeni-widget-geliştirme-best-practices)
8. [Kod Örnekleri](#kod-örnekleri)

---

## Giriş

Bu doküman, **LVGL v8** widget altyapısının detaylı teknik analizini sunar. LVGL'nin temel nesne sistemi (`lv_obj`), widget yaşam döngüsü, rendering mekanizması ve kalıtım yapısı ele alınmaktadır.

**Hedef Kitle**: LVGL ile widget geliştirmek isteyen yazılımcılar, framework mimarisini anlamak isteyen geliştiriciler.

**Ön Koşullar**: C programlama, temel GUI konseptleri, embedded sistemler.

---

## lv_obj Mimarisi ve Abstraction Modeli

### 1.1 lv_obj: Tüm Widget'ların Base Class'ı

LVGL'de **her widget bir `lv_obj_t`**'dir. Bu, framework'ün temel abstraction'ıdır.

#### Core Structure

```c
typedef struct _lv_obj_t {
    const lv_obj_class_t * class_p;      // Class descriptor (vtable benzeri)
    struct _lv_obj_t * parent;           // Parent nesne pointer'ı
    _lv_obj_spec_attr_t * spec_attr;     // Opsiyonel özellikler (lazy allocation)
    _lv_obj_style_t * styles;            // Stil listesi
    void * user_data;                    // Kullanıcı verisi
    lv_area_t coords;                    // Ekran koordinatları (x1,y1,x2,y2)
    lv_obj_flag_t flags;                 // Davranış flag'leri (32 bit)
    lv_state_t state;                    // UI durumu (16 bit)

    // Bit-packed fields (optimizasyon)
    uint16_t layout_inv : 1;             // Layout invalidation flag
    uint16_t scr_layout_inv : 1;         // Screen layout invalidation
    uint16_t skip_trans : 1;             // Transition'ları atla
    uint16_t style_cnt  : 6;             // Stil sayısı (max 64)
    uint16_t h_layout   : 1;             // Has layout
    uint16_t w_layout   : 1;             // Width layout
} lv_obj_t;
```

**Kaynak**: `src/core/lv_obj.h:174-191`

#### Temel Abstraction Prensipleri

##### a) Opaque Pointer Pattern

```c
// Header'da sadece forward declaration
struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

// Implementation detayları .c dosyasında gizli
```

**Faydalar**:
- ✅ Encapsulation: İç yapı kullanıcıdan gizli
- ✅ ABI Stability: İç değişiklikler API'yi bozmaz
- ✅ Controlled Access: Sadece getter/setter ile erişim

##### b) Lazy Allocation Strategy

Nadiren kullanılan özellikler dinamik tahsis edilir:

```c
typedef struct {
    struct _lv_obj_t ** children;        // Child dizisi (NULL ise çocuk yok)
    uint32_t child_cnt;                  // Child sayısı
    lv_group_t * group_p;                // Focus grubu
    struct _lv_event_dsc_t * event_dsc;  // Event handler'lar
    lv_point_t scroll;                   // Scroll offset
    lv_coord_t ext_click_pad;            // Ekstra tıklama alanı
    lv_coord_t ext_draw_size;            // Ekstra çizim alanı
    lv_scrollbar_mode_t scrollbar_mode : 2;
    lv_scroll_snap_t scroll_snap_x : 2;
    lv_scroll_snap_t scroll_snap_y : 2;
    lv_dir_t scroll_dir : 4;
    uint8_t event_dsc_cnt : 6;
    uint8_t layer_type : 2;
} _lv_obj_spec_attr_t;
```

**Kaynak**: `src/core/lv_obj.h:155-172`

**Tahsis Fonksiyonu**:

```c
void lv_obj_allocate_spec_attr(lv_obj_t * obj) {
    if(obj->spec_attr == NULL) {
        obj->spec_attr = lv_malloc(sizeof(_lv_obj_spec_attr_t));
        lv_memzero(obj->spec_attr, sizeof(_lv_obj_spec_attr_t));

        // Varsayılan değerler
        obj->spec_attr->scroll_dir = LV_DIR_ALL;
        obj->spec_attr->scrollbar_mode = LV_SCROLLBAR_MODE_AUTO;
    }
}
```

**Bellek Tasarrufu**:
- Temel `lv_obj_t`: ~60 bytes
- Tüm özelliklerle: ~150+ bytes
- **Çocuksuz, event'siz nesneler için %60 tasarruf**

### 1.2 Class System: OOP in C

LVGL, C dilinde sofistike bir class sistemi implement etmiş.

#### Class Descriptor

```c
typedef struct _lv_obj_class_t {
    const struct _lv_obj_class_t * base_class;    // Kalıtım pointer'ı
    void (*constructor_cb)(const lv_obj_class_t * class_p, lv_obj_t * obj);
    void (*destructor_cb)(const lv_obj_class_t * class_p, lv_obj_t * obj);
    void (*event_cb)(const lv_obj_class_t * class_p, lv_event_t * e);
    void * user_data;                              // Class-level user data
    lv_coord_t width_def;                          // Varsayılan genişlik
    lv_coord_t height_def;                         // Varsayılan yükseklik
    uint32_t editable : 2;                         // Düzenlenebilir mi?
    uint32_t group_def : 2;                        // Varsayılan grupta mı?
    uint32_t instance_size : 16;                   // Instance boyutu (bytes)
    uint32_t theme_inheritable : 1;                // Tema miras alınabilir mi?
} lv_obj_class_t;
```

**Kaynak**: `src/core/lv_obj_class.h:55-70`

#### Base Object Class

```c
const lv_obj_class_t lv_obj_class = {
    .base_class = NULL,                   // Zincirin en üstü
    .instance_size = sizeof(lv_obj_t),
    .constructor_cb = lv_obj_constructor,
    .destructor_cb = lv_obj_destructor,
    .event_cb = NULL,                     // Base'de yok
};
```

### 1.3 State ve Flag Sistemleri

#### State (Durum) - UI Görünümü

```c
typedef enum {
    LV_STATE_DEFAULT     = 0x0000,
    LV_STATE_CHECKED     = 0x0001,   // Bit 0: İşaretli (checkbox, switch)
    LV_STATE_FOCUSED     = 0x0002,   // Bit 1: Fokuslanmış
    LV_STATE_FOCUS_KEY   = 0x0004,   // Bit 2: Klavye ile fokus
    LV_STATE_EDITED      = 0x0008,   // Bit 3: Düzenleme modunda
    LV_STATE_HOVERED     = 0x0010,   // Bit 4: Mouse üzerinde
    LV_STATE_PRESSED     = 0x0020,   // Bit 5: Basılı
    LV_STATE_SCROLLED    = 0x0040,   // Bit 6: Kaydırılmış
    LV_STATE_DISABLED    = 0x0080,   // Bit 7: Devre dışı

    LV_STATE_USER_1      = 0x1000,   // Kullanıcı tanımlı
    LV_STATE_USER_2      = 0x2000,
    LV_STATE_USER_3      = 0x4000,
    LV_STATE_USER_4      = 0x8000,

    LV_STATE_ANY         = 0xFFFF,   // Wildcard
} lv_state_t;
```

**Kaynak**: `src/core/lv_obj.h:41-60`

**Kullanım**:

```c
// Çoklu durum kombinasyonu
lv_state_t combined = LV_STATE_FOCUSED | LV_STATE_PRESSED;

// Stil duruma göre değişir
lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), LV_STATE_PRESSED);
lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), LV_STATE_FOCUSED);
```

#### Flags (Davranış Özellikleri)

```c
typedef enum {
    LV_OBJ_FLAG_HIDDEN          = (1L << 0),   // Gizli
    LV_OBJ_FLAG_CLICKABLE       = (1L << 1),   // Tıklanabilir
    LV_OBJ_FLAG_CLICK_FOCUSABLE = (1L << 2),   // Tıkla-fokuslan
    LV_OBJ_FLAG_CHECKABLE       = (1L << 3),   // Checked state'i var
    LV_OBJ_FLAG_SCROLLABLE      = (1L << 4),   // Kaydırılabilir
    LV_OBJ_FLAG_SCROLL_ELASTIC  = (1L << 5),   // Elastik scroll
    LV_OBJ_FLAG_SCROLL_MOMENTUM = (1L << 6),   // Momentum scroll
    LV_OBJ_FLAG_SCROLL_ONE      = (1L << 7),   // Tek snap'lı scroll
    LV_OBJ_FLAG_SCROLL_CHAIN_HOR = (1L << 8),  // Yatay scroll zincirleme
    LV_OBJ_FLAG_SCROLL_CHAIN_VER = (1L << 9),  // Dikey scroll zincirleme
    LV_OBJ_FLAG_SCROLL_ON_FOCUS = (1L << 10),  // Fokuslanınca scroll
    LV_OBJ_FLAG_EVENT_BUBBLE    = (1L << 14),  // Event'leri parent'a ilet
    LV_OBJ_FLAG_GESTURE_BUBBLE  = (1L << 15),  // Gesture'ları parent'a ilet
    LV_OBJ_FLAG_ADV_HITTEST     = (1L << 16),  // Gelişmiş hit test
    LV_OBJ_FLAG_IGNORE_LAYOUT   = (1L << 17),  // Layout'u yoksay
    LV_OBJ_FLAG_FLOATING        = (1L << 18),  // Floating pozisyon
    LV_OBJ_FLAG_OVERFLOW_VISIBLE = (1L << 19), // Overflow görünür

    LV_OBJ_FLAG_USER_1          = (1L << 27),  // Kullanıcı için
    LV_OBJ_FLAG_USER_2          = (1L << 28),
    LV_OBJ_FLAG_USER_3          = (1L << 29),
    LV_OBJ_FLAG_USER_4          = (1L << 30),
} lv_obj_flag_t;
```

**Kaynak**: `src/core/lv_obj.h:89-122`

**State vs Flags**:
- **State**: Görsel durum (stil değişikliği tetikler)
- **Flags**: Davranış toggle'ları (event işleme, scroll, layout)

### 1.4 Part System: Composite Pattern

Widget'lar, bağımsız "part"lardan oluşur:

```c
typedef enum {
    LV_PART_MAIN         = 0x000000,   // Ana arka plan
    LV_PART_SCROLLBAR    = 0x010000,   // Scrollbar
    LV_PART_INDICATOR    = 0x020000,   // Gösterge (slider, bar, progress)
    LV_PART_KNOB         = 0x030000,   // Tutamaç (slider knob)
    LV_PART_SELECTED     = 0x040000,   // Seçim vurgusu
    LV_PART_ITEMS        = 0x050000,   // Bireysel öğeler (tablo hücreleri)
    LV_PART_TICKS        = 0x060000,   // Tick işaretleri
    LV_PART_CURSOR       = 0x070000,   // İmleç

    LV_PART_CUSTOM_FIRST = 0x080000,   // Özel part'lar için
    LV_PART_ANY          = 0x0F0000,   // Wildcard
} lv_part_t;
```

**Kaynak**: `src/core/lv_obj.h:68-83`

**Örnek: Slider Composition**

```
┌─────────────────────────────────┐
│  LV_PART_MAIN (track)           │
│  ┌─────────────────┐            │
│  │ LV_PART_INDICATOR│            │
│  └─────────────────┘  ●         │
│                   LV_PART_KNOB  │
└─────────────────────────────────┘
```

Her part bağımsız stillenebilir:

```c
// Main track
lv_obj_set_style_bg_color(slider, lv_color_hex(0xCCCCCC), LV_PART_MAIN);

// Indicator
lv_obj_set_style_bg_color(slider, lv_color_hex(0x0080FF), LV_PART_INDICATOR);

// Knob - basılı durumda opaklık
lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB | LV_STATE_PRESSED);
```

---

## Widget Lifecycle ve Event-Driven Yapı

### 2.1 Nesne Yaratma: İki Fazlı İnşa

LVGL, **Two-Phase Initialization** pattern kullanır.

#### Faz 1: Memory Allocation

```c
lv_obj_t * lv_obj_class_create_obj(const lv_obj_class_t * class_p, lv_obj_t * parent)
{
    // 1. Instance boyutunu belirle (kalıtım zincirini tara)
    uint32_t s = get_instance_size(class_p);

    // 2. Bellek tahsis et
    lv_obj_t * obj = lv_malloc(s);
    if(obj == NULL) return NULL;

    // 3. SIFIRLA (zero-initialization)
    lv_memzero(obj, s);

    // 4. Class ve parent pointer'larını set et
    obj->class_p = class_p;
    obj->parent = parent;

    // 5. Parent'ın children listesine ekle
    if(parent == NULL) {
        // Screen creation - display'e kaydet
        lv_disp_t * disp = lv_disp_get_default();
        disp->screen_cnt++;
        disp->screens = lv_realloc(disp->screens,
                                   sizeof(lv_obj_t *) * disp->screen_cnt);
        disp->screens[disp->screen_cnt - 1] = obj;
    } else {
        // Normal child - parent'a ekle
        lv_obj_allocate_spec_attr(parent);
        parent->spec_attr->child_cnt++;
        parent->spec_attr->children = lv_realloc(
            parent->spec_attr->children,
            sizeof(lv_obj_t *) * parent->spec_attr->child_cnt);
        parent->spec_attr->children[parent->spec_attr->child_cnt - 1] = obj;
    }

    return obj;
}
```

**Kaynak**: `src/core/lv_obj_class.c:43-102`

#### Faz 2: Initialization

```c
void lv_obj_class_init_obj(lv_obj_t * obj)
{
    // 1. Layout'u invalidate et
    lv_obj_mark_layout_as_dirty(obj);

    // 2. Stil refresh'i geçici olarak durdur (performans)
    lv_obj_enable_style_refresh(false);

    // 3. Tema uygula
    lv_theme_apply(obj);

    // 4. CONSTRUCTOR ZİNCİRİNİ ÇAĞIR (base → derived)
    lv_obj_construct(obj);

    // 5. Stil refresh'i etkinleştir ve stilleri hesapla
    lv_obj_enable_style_refresh(true);
    lv_obj_refresh_style(obj, LV_PART_ANY, LV_STYLE_PROP_ANY);

    // 6. Self-size hesapla (content boyutu)
    lv_obj_refresh_self_size(obj);

    // 7. Varsayılan gruba ekle (varsa)
    lv_group_t * def_group = lv_group_get_default();
    if(def_group && lv_obj_is_group_def(obj)) {
        lv_group_add_obj(def_group, obj);
    }

    // 8. Parent'a event gönder
    lv_obj_t * parent = lv_obj_get_parent(obj);
    if(parent) {
        lv_event_send(parent, LV_EVENT_CHILD_CHANGED, obj);
        lv_event_send(parent, LV_EVENT_CHILD_CREATED, obj);

        // 9. İlk invalidation
        lv_obj_invalidate(obj);
    }
}
```

**Kaynak**: `src/core/lv_obj_class.c:104-132`

#### Constructor Zinciri (Template Method Pattern)

```c
static void lv_obj_construct(lv_obj_t * obj)
{
    const lv_obj_class_t * original_class_p = obj->class_p;

    // Base class constructor'ını ÖNCE çağır (recursive)
    if(obj->class_p->base_class) {
        obj->class_p = obj->class_p->base_class;
        lv_obj_construct(obj);  // Recursive call
    }

    // Orijinal class'ı geri yükle
    obj->class_p = original_class_p;

    // Kendi constructor'ını çağır
    if(obj->class_p->constructor_cb) {
        obj->class_p->constructor_cb(obj->class_p, obj);
    }
}
```

**Kaynak**: `src/core/lv_obj_class.c:175-191`

**Zincir Sırası** (Button örneği):

```
lv_obj_constructor()           ← Base class (ilk)
    ↓
lv_btn_constructor()           ← Derived class (son)
```

#### Tam Yaratma Akışı

```
lv_btn_create(parent)
  │
  ├─► lv_obj_class_create_obj(&lv_btn_class, parent)
  │    ├─ Bellek tahsis: sizeof(lv_btn_t)
  │    ├─ Zero-initialize
  │    ├─ obj->class_p = &lv_btn_class
  │    ├─ obj->parent = parent
  │    └─ Parent'ın children dizisine ekle
  │
  └─► lv_obj_class_init_obj(obj)
       ├─ lv_theme_apply()
       ├─ lv_obj_construct()
       │   ├─ lv_obj_constructor()      [base]
       │   └─ lv_btn_constructor()      [derived]
       ├─ lv_obj_refresh_style()
       ├─ lv_obj_refresh_self_size()
       └─ LV_EVENT_CHILD_CREATED gönder
```

### 2.2 Nesne Silme: İki Fazlı Yıkım

#### Deletion Flow

```c
void lv_obj_del(lv_obj_t * obj)
{
    // 1. Display alanını invalidate et
    lv_obj_invalidate(obj);

    // 2. Core silme fonksiyonu
    obj_del_core(obj);
        // a. Tüm child'ları recursive sil
        // b. Destructor zinciri çağır (derived → base)
        // c. Group'tan çıkar
        // d. Tüm animasyonları kaldır
        // e. Event handler'ları serbest bırak
        // f. Stil listesini serbest bırak
        // g. Spec attr'ı serbest bırak
        // h. Nesne belleğini serbest bırak

    // 3. Parent'a bildir
    if(parent) {
        lv_event_send(parent, LV_EVENT_CHILD_DELETED, obj);
    }
}
```

#### Destructor Zinciri

```c
void _lv_obj_destruct(lv_obj_t * obj)
{
    // Kendi destructor'ını ÖNCE çağır
    if(obj->class_p->destructor_cb) {
        obj->class_p->destructor_cb(obj->class_p, obj);
    }

    // Base class destructor'ını çağır (recursive)
    if(obj->class_p->base_class) {
        obj->class_p = obj->class_p->base_class;
        _lv_obj_destruct(obj);  // Recursive
    }
}
```

**Kaynak**: `src/core/lv_obj_class.c:134-145`

**Sıra** (Constructor'ın tersi):

```
lv_btn_destructor()            ← Derived class (ilk)
    ↓
lv_obj_destructor()            ← Base class (son)
```

#### Güvenli Silme: Deferred Deletion

Event callback içinde silmek tehlikelidir:

```c
// ❌ TEHLİKELİ
static void button_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    if(e->code == LV_EVENT_CLICKED) {
        lv_obj_del(btn);  // Use-after-free riski!
    }
}

// ✅ GÜVENLİ
static void button_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    if(e->code == LV_EVENT_CLICKED) {
        lv_obj_del_async(btn);  // Async silme
    }
}
```

### 2.3 Event System: Observer Pattern

LVGL, sofistike bir event sistemi kullanır.

#### Event Tipleri (60+ event)

##### Input Events

```c
LV_EVENT_PRESSED              // İlk basış
LV_EVENT_PRESSING             // Basılı tutma
LV_EVENT_PRESS_LOST           // Basış kaybedildi
LV_EVENT_SHORT_CLICKED        // Kısa tık
LV_EVENT_LONG_PRESSED         // Uzun basış
LV_EVENT_CLICKED              // Normal tık
LV_EVENT_RELEASED             // Bırakma
LV_EVENT_SCROLL_BEGIN         // Scroll başlangıcı
LV_EVENT_SCROLL_END           // Scroll bitişi
LV_EVENT_FOCUSED              // Fokus alındı
LV_EVENT_DEFOCUSED            // Fokus kaybedildi
LV_EVENT_KEY                  // Klavye tuşu
LV_EVENT_GESTURE              // Gesture algılandı
```

##### Drawing Events

```c
LV_EVENT_DRAW_MAIN_BEGIN      // Ana çizim başlar
LV_EVENT_DRAW_MAIN            // Ana çizim
LV_EVENT_DRAW_MAIN_END        // Ana çizim biter
LV_EVENT_DRAW_POST_BEGIN      // Post çizim başlar
LV_EVENT_DRAW_POST            // Post çizim (overlay)
LV_EVENT_DRAW_POST_END        // Post çizim biter
LV_EVENT_DRAW_PART_BEGIN      // Part çizimi başlar
LV_EVENT_DRAW_PART_END        // Part çizimi biter
```

##### State Change Events

```c
LV_EVENT_VALUE_CHANGED        // Değer değişti
LV_EVENT_SIZE_CHANGED         // Boyut değişti
LV_EVENT_STYLE_CHANGED        // Stil değişti
LV_EVENT_LAYOUT_CHANGED       // Layout değişti
LV_EVENT_REFRESH              // Refresh gerekli
```

##### Lifecycle Events

```c
LV_EVENT_DELETE               // Nesne silinmeden önce
LV_EVENT_CHILD_CREATED        // Child yaratıldı
LV_EVENT_CHILD_DELETED        // Child silindi
LV_EVENT_SCREEN_LOADED        // Screen yüklendi
LV_EVENT_SCREEN_UNLOADED      // Screen kaldırıldı
```

#### Event Handler Kaydı

```c
// Tek event için
lv_obj_add_event_cb(obj, my_event_cb, LV_EVENT_CLICKED, user_data);

// Tüm event'ler için
lv_obj_add_event_cb(obj, my_event_cb, LV_EVENT_ALL, user_data);

// Event handler callback
static void my_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);        // Orijinal hedef
    lv_obj_t * current = lv_event_get_current_target(e); // Şu anki işleyici
    void * param = lv_event_get_param(e);
    void * user_data = lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        // Tıklama işle
    }
}
```

#### Event Dispatch: Üç Fazlı İşleme

```c
lv_res_t lv_event_send(lv_obj_t * obj, lv_event_code_t event_code, void * param)
{
    lv_event_t e;
    e.target = obj;              // Event'in kaynağı
    e.current_target = obj;       // Şu anda işleyen nesne
    e.code = event_code;
    e.param = param;

    // Nested event takibi için linked list
    e.prev = event_head;
    event_head = &e;

    lv_res_t res = event_send_core(&e);

    event_head = e.prev;
    return res;
}
```

```c
static lv_res_t event_send_core(lv_event_t * e) {
    lv_obj_t * obj = e->current_target;

    // FAZ 1: PREPROCESS - Ön işleme handler'ları
    //        (is_preprocess flag'i ile kayıtlı)
    for(each preprocess_handler) {
        handler(e);
        if(e->stop_processing) return LV_RES_INV;
    }

    // FAZ 2: CLASS HANDLER - Virtual method
    if(obj->class_p->event_cb) {
        obj->class_p->event_cb(obj->class_p, e);
        if(e->stop_processing) return LV_RES_INV;
    }

    // FAZ 3: REGULAR - Normal event callback'leri
    for(each regular_handler) {
        handler(e);
        if(e->stop_processing) return LV_RES_INV;
    }

    // FAZ 4: BUBBLING - Parent'a ilet (izin verilirse)
    if(res == LV_RES_OK && obj->parent && event_is_bubbled(e)) {
        e->current_target = obj->parent;
        res = event_send_core(e);  // Recursive bubbling
    }

    return res;
}
```

#### Event Bubbling (Chain of Responsibility)

```
┌─────────────────────────────┐
│  Screen                     │  ← Event bubble (parent'a çıkıyor)
│  ┌─────────────────────┐    │
│  │  Panel              │    │  ← Event bubble
│  │  ┌──────────────┐   │    │
│  │  │  Button      │   │    │  ← Event origin (clicked)
│  │  └──────────────┘   │    │
│  └─────────────────────┘    │
└─────────────────────────────┘
```

**Bubbling Kuralları**:

```c
static bool event_is_bubbled(lv_event_t * e) {
    // Her zaman bubble olan event'ler
    if(e->code == LV_EVENT_CHILD_CREATED ||
       e->code == LV_EVENT_CHILD_DELETED) {
        return true;
    }

    // Drawing event'leri ASLA bubble olmaz (performans)
    if(e->code >= LV_EVENT_DRAW_MAIN_BEGIN &&
       e->code <= LV_EVENT_DRAW_POST_END) {
        return false;
    }

    // LV_OBJ_FLAG_EVENT_BUBBLE flag'i kontrolü
    if(lv_obj_has_flag(e->current_target, LV_OBJ_FLAG_EVENT_BUBBLE)) {
        return true;
    }

    return false;
}
```

**Manuel Kontrol**:

```c
static void my_event_cb(lv_event_t * e) {
    // Event'i parent'a bubble etme
    lv_event_stop_bubbling(e);

    // İşlemeyi tamamen durdur
    lv_event_stop_processing(e);
}
```

---

## Invalidation → Rendering Pipeline

### 3.1 Dirty Region Tracking

LVGL, **sadece değişen alanları** yeniden çizer (performans optimizasyonu).

#### Invalidation Sistemi

```c
void lv_obj_invalidate(lv_obj_t * obj) {
    lv_obj_invalidate_area(obj, &obj->coords);
}

void lv_obj_invalidate_area(lv_obj_t * obj, const lv_area_t * area) {
    lv_disp_t * disp = lv_obj_get_disp(obj);
    _lv_inv_area(disp, area);  // Display'in dirty region listesine ekle
}
```

**Kaynak**: `src/core/lv_refr.c:205-250`

#### Dirty Area Yönetimi

```c
void _lv_inv_area(lv_disp_t * disp, const lv_area_t * area_p)
{
    // Ekran sınırları ile kesişim kontrol et
    lv_area_t scr_area = {0, 0, hor_res - 1, ver_res - 1};
    lv_area_t com_area;

    if(!_lv_area_intersect(&com_area, area_p, &scr_area)) {
        return;  // Ekran dışında, yoksay
    }

    // Full refresh modunda tüm ekranı invalidate et
    if(disp->driver->full_refresh) {
        disp->inv_areas[0] = scr_area;
        disp->inv_p = 1;
        return;
    }

    // Rounder callback (bazı display'ler 8 piksel align gerektirir)
    if(disp->driver->rounder_cb) {
        disp->driver->rounder_cb(disp->driver, &com_area);
    }

    // Bu alan zaten bir dirty area içinde mi?
    for(i = 0; i < disp->inv_p; i++) {
        if(_lv_area_is_in(&com_area, &disp->inv_areas[i], 0)) {
            return;  // Zaten kapsanıyor
        }
    }

    // Yeni dirty area kaydet
    if(disp->inv_p < LV_INV_BUF_SIZE) {
        disp->inv_areas[disp->inv_p] = com_area;
        disp->inv_p++;
    }
}
```

#### Dirty Area Birleştirme (Join)

Overlapping area'ları birleştir (optimizasyon):

```c
static void lv_refr_join_area(void)
{
    // Tüm dirty area çiftlerini karşılaştır
    for(uint32_t i = 0; i < disp->inv_p; i++) {
        for(uint32_t j = i + 1; j < disp->inv_p; j++) {
            // Çakışma veya yakınlık kontrolü
            if(areas_overlap_or_near(&disp->inv_areas[i],
                                     &disp->inv_areas[j])) {
                // Birleştir
                _lv_area_join(&disp->inv_areas[i],
                             &disp->inv_areas[i],
                             &disp->inv_areas[j]);

                // j'yi listeden kaldır (shift)
                for(uint32_t k = j; k < disp->inv_p - 1; k++) {
                    disp->inv_areas[k] = disp->inv_areas[k + 1];
                }
                disp->inv_p--;
                j--;  // Tekrar kontrol et
            }
        }
    }
}
```

### 3.2 Rendering Pipeline

#### Ana Refresh Döngüsü

```c
void _lv_disp_refr_timer(lv_timer_t * timer)
{
    lv_disp_t * disp = timer->user_data;

    // 1. Dirty area'ları birleştir
    lv_refr_join_area();

    // 2. Her dirty area için rendering yap
    for(uint32_t i = 0; i < disp->inv_p; i++) {
        refr_area(&disp->inv_areas[i]);
    }

    // 3. Dirty area listesini temizle
    disp->inv_p = 0;
}
```

#### Tek Bir Area'nın Rendering'i

```c
static void refr_area(const lv_area_t * area_p)
{
    // 1. Draw buffer hazırla
    lv_draw_ctx_t * draw_ctx = disp->driver->draw_ctx;
    draw_ctx->buf = draw_buf;
    draw_ctx->buf_area = area_p;
    draw_ctx->clip_area = area_p;

    // 2. En üstteki visible nesneyi bul
    lv_obj_t * top_obj = lv_refr_get_top_obj(area_p, lv_disp_get_scr_act(disp));

    // 3. Nesneyi ve child'larını çiz
    refr_obj_and_children(draw_ctx, top_obj);

    // 4. Flush callback (display'e gönder)
    draw_buf_flush(disp);
}
```

**Kaynak**: `src/core/lv_refr.c:54-63`

#### Recursive Object Drawing

```c
void lv_obj_redraw(lv_draw_ctx_t * draw_ctx, lv_obj_t * obj)
{
    const lv_area_t * clip_area_ori = draw_ctx->clip_area;
    lv_area_t clip_coords_for_obj;

    // 1. Nesnenin clip area'sını hesapla (coords + ext_draw_size)
    lv_area_t obj_coords_ext;
    lv_obj_get_coords(obj, &obj_coords_ext);
    lv_coord_t ext_draw_size = _lv_obj_get_ext_draw_size(obj);
    lv_area_increase(&obj_coords_ext, ext_draw_size, ext_draw_size);

    // 2. Orijinal clip area ile kesişim
    bool should_draw = _lv_area_intersect(&clip_coords_for_obj,
                                          clip_area_ori,
                                          &obj_coords_ext);

    // 3. MAIN DRAWING PHASE (nesnenin kendisi)
    if(should_draw || lv_obj_has_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE)) {
        draw_ctx->clip_area = &clip_coords_for_obj;

        lv_event_send(obj, LV_EVENT_DRAW_MAIN_BEGIN, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_MAIN, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_MAIN_END, draw_ctx);
    }

    // 4. CHILDREN DRAWING PHASE
    lv_area_t clip_coords_for_children;
    bool refr_children = true;

    if(lv_obj_has_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE)) {
        // Overflow visible: child'lar nesne sınırlarının dışına çıkabilir
        clip_coords_for_children = *clip_area_ori;
    } else {
        // Normal: child'ları nesne sınırları ile clip et
        if(!_lv_area_intersect(&clip_coords_for_children,
                               clip_area_ori,
                               &obj->coords)) {
            refr_children = false;
        }
    }

    if(refr_children) {
        draw_ctx->clip_area = &clip_coords_for_children;
        uint32_t child_cnt = lv_obj_get_child_cnt(obj);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = obj->spec_attr->children[i];
            lv_obj_redraw(draw_ctx, child);  // Recursive
        }
    }

    // 5. POST DRAWING PHASE (overlay, effects)
    if(should_draw) {
        draw_ctx->clip_area = &clip_coords_for_obj;

        lv_event_send(obj, LV_EVENT_DRAW_POST_BEGIN, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_POST, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_POST_END, draw_ctx);
    }

    // 6. Orijinal clip area'yı geri yükle
    draw_ctx->clip_area = clip_area_ori;
}
```

**Kaynak**: `src/core/lv_refr.c:129-196`

#### Drawing Flow Özeti

```
refr_area(dirty_area)
  │
  ├─► lv_refr_get_top_obj()  [Görünür en üst nesneyi bul]
  │
  └─► refr_obj_and_children(top_obj)
       │
       └─► lv_obj_redraw(draw_ctx, obj)  ← RECURSIVE
            │
            ├─► LV_EVENT_DRAW_MAIN_BEGIN
            ├─► LV_EVENT_DRAW_MAIN
            ├─► LV_EVENT_DRAW_MAIN_END
            │
            ├─► FOR EACH CHILD:
            │    └─► lv_obj_redraw(child)  ← RECURSIVE
            │
            ├─► LV_EVENT_DRAW_POST_BEGIN
            ├─► LV_EVENT_DRAW_POST
            └─► LV_EVENT_DRAW_POST_END
```

### 3.3 Invalidation Trigger'ları

Widget'lar şu durumlarda invalidate edilir:

```c
// Pozisyon değişikliği
lv_obj_set_pos(obj, x, y);
    └─► lv_obj_invalidate(obj);

// Boyut değişikliği
lv_obj_set_size(obj, w, h);
    └─► lv_obj_invalidate(obj);

// Stil değişikliği
lv_obj_set_style_bg_color(obj, color, selector);
    └─► lv_obj_invalidate(obj);

// State değişikliği (stil etkilenirse)
lv_obj_add_state(obj, LV_STATE_PRESSED);
    └─► lv_obj_refresh_style() → lv_obj_invalidate(obj);

// Değer değişikliği (slider, bar, vb.)
lv_slider_set_value(slider, value);
    └─► lv_obj_invalidate(obj);
```

---

## Widget Inheritance Mekanizması

### 4.1 Class Hierarchy

LVGL, **single inheritance** destekler.

#### Örnek Hierarchy: Slider

```
lv_obj                    (base class)
  ↓ base_class
lv_bar                    (bar widget)
  ↓ base_class
lv_slider                 (slider widget)
```

#### Class Tanımları

**Base Object**:

```c
const lv_obj_class_t lv_obj_class = {
    .base_class = NULL,
    .constructor_cb = lv_obj_constructor,
    .destructor_cb = lv_obj_destructor,
    .event_cb = NULL,
    .instance_size = sizeof(lv_obj_t),
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
};
```

**Button Class**:

```c
const lv_obj_class_t lv_btn_class = {
    .base_class = &lv_obj_class,          // Kalıtım
    .constructor_cb = lv_btn_constructor,
    .instance_size = sizeof(lv_btn_t),
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
};
```

**Kaynak**: `src/widgets/btn/lv_btn.c:30-37`

**Slider Class**:

```c
const lv_obj_class_t lv_slider_class = {
    .base_class = &lv_bar_class,          // Bar'dan miras alır
    .constructor_cb = lv_slider_constructor,
    .event_cb = lv_slider_event,
    .instance_size = sizeof(lv_slider_t),
    .editable = LV_OBJ_CLASS_EDITABLE_TRUE,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
};
```

**Kaynak**: `src/widgets/slider/lv_slider.c:45-52`

### 4.2 Instance Structures

#### Type Extension

```c
// Base type
typedef struct {
    lv_obj_t obj;           // FIRST MEMBER (inheritance trick)
    // lv_obj öğeleri buraya genişletilmiş gibi
} lv_obj_t;

// Derived type: Button
typedef struct {
    lv_obj_t obj;           // FIRST MEMBER = Base class
    // Button'a özel ek alan yok
} lv_btn_t;

// Derived type: Slider
typedef struct {
    lv_bar_t bar;           // FIRST MEMBER = Base class (bar)
    lv_area_t left_knob_area;   // Sol knob koordinatları
    lv_area_t right_knob_area;  // Sağ knob koordinatları
    lv_point_t pressed_point;   // Basılan nokta
    int32_t * value_to_set;     // Ayarlanacak değer pointer'ı
    uint8_t dragging : 1;       // Sürükleme durumu
    uint8_t left_knob_focus : 1;// Sol knob fokuslu mu?
} lv_slider_t;
```

**First Member Trick**: İlk üye base type olduğunda, pointer cast'i güvenli hale gelir:

```c
lv_slider_t * slider = lv_slider_create(parent);
lv_obj_t * obj = (lv_obj_t *)slider;  // ✅ Güvenli (memory layout uyumlu)

// Tüm lv_obj_* fonksiyonları slider ile çalışabilir
lv_obj_set_pos(obj, 10, 20);
```

### 4.3 Virtual Methods: Event Callbacks

Class-specific event handler'lar **virtual method** gibi çalışır.

#### Base Class Handler Çağırma

```c
static void lv_slider_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    lv_res_t res;

    // BASE CLASS EVENT HANDLER'INI ÇAĞIR (super.event())
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    // Kendi event işleme mantığı
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_slider_t * slider = (lv_slider_t *)obj;

    if(code == LV_EVENT_PRESSED) {
        lv_indev_get_point(lv_indev_get_act(), &slider->pressed_point);
    }
    else if(code == LV_EVENT_PRESSING) {
        update_knob_pos(obj, true);
    }
    else if(code == LV_EVENT_RELEASED) {
        slider->dragging = false;
        lv_obj_invalidate(obj);
    }
}
```

**Kaynak**: `src/widgets/slider/lv_slider.c:97-166`

#### Base Class Event Helper

```c
lv_res_t lv_obj_event_base(const lv_obj_class_t * class_p, lv_event_t * e)
{
    // Base class'ın event handler'ını bul ve çağır
    const lv_obj_class_t * base = class_p->base_class;

    while(base) {
        if(base->event_cb) {
            base->event_cb(base, e);
            if(e->stop_processing) return LV_RES_INV;
            break;
        }
        base = base->base_class;
    }

    return LV_RES_OK;
}
```

Bu sayede **C++'daki `super.method()` çağrısına** eşdeğer bir yapı elde edilir.

### 4.4 Type Checking (Runtime)

```c
bool lv_obj_has_class(const lv_obj_t * obj, const lv_obj_class_t * class_p)
{
    const lv_obj_class_t * obj_class = obj->class_p;

    // Kalıtım zincirini yukarı doğru tara
    while(obj_class) {
        if(obj_class == class_p) return true;
        obj_class = obj_class->base_class;
    }

    return false;
}
```

**Kullanım**:

```c
lv_obj_t * obj = lv_slider_create(parent);

lv_obj_has_class(obj, &lv_slider_class);  // true
lv_obj_has_class(obj, &lv_bar_class);     // true (base class)
lv_obj_has_class(obj, &lv_obj_class);     // true (base of base)
lv_obj_has_class(obj, &lv_btn_class);     // false
```

#### Assertion Macro

```c
#define LV_ASSERT_OBJ(obj_p, obj_class) \
    do { \
        LV_ASSERT_MSG(obj_p != NULL, "The object is NULL"); \
        LV_ASSERT_MSG(lv_obj_has_class(obj_p, obj_class), \
                      "Incompatible object type"); \
    } while(0)

// Widget fonksiyonlarında kullanım
void lv_slider_set_value(lv_obj_t * obj, int32_t value) {
    LV_ASSERT_OBJ(obj, &lv_slider_class);  // Type safety
    // ...
}
```

---

## Design Pattern'lar

### 5.1 Prototype Pattern (OOP in C)

Class-based sistem için function pointer'lar ve vtable benzeri yapı.

**Kullanım**: Class descriptor (`lv_obj_class_t`)

```c
typedef struct _lv_obj_class_t {
    const struct _lv_obj_class_t * base_class;    // Kalıtım
    void (*constructor_cb)(...);                   // Virtual constructor
    void (*destructor_cb)(...);                    // Virtual destructor
    void (*event_cb)(...);                         // Virtual event handler
    // ...
} lv_obj_class_t;
```

### 5.2 Template Method Pattern

Constructor/destructor zincirleri otomatik çağrılır.

```c
// Constructor template
lv_obj_construct(obj)
  ├─ base_class->constructor_cb()    [ilk]
  └─ derived_class->constructor_cb() [son]

// Destructor template
_lv_obj_destruct(obj)
  ├─ derived_class->destructor_cb()  [ilk]
  └─ base_class->destructor_cb()     [son]
```

### 5.3 Observer Pattern

Event sistem: Observer'lar (callback'ler) subject'i (widget) dinler.

```c
// Observer kaydet
lv_obj_add_event_cb(slider, slider_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

// Subject notify eder
lv_slider_set_value(slider, 50);
  └─► lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);
       └─► slider_changed_cb(...);
```

### 5.4 Chain of Responsibility

Event bubbling: Event, parent zincirinde yukarı doğru yayılır.

```
Button (clicked) → Panel → Screen
         ↓           ↓       ↓
       handler    handler  handler
```

### 5.5 Composite Pattern

Widget tree yapısı: Widget'lar child'lar içerebilir.

```c
lv_obj_t * panel = lv_obj_create(screen);
lv_obj_t * btn1 = lv_btn_create(panel);   // Child 1
lv_obj_t * btn2 = lv_btn_create(panel);   // Child 2
lv_obj_t * label = lv_label_create(btn1); // Child of child
```

### 5.6 Flyweight Pattern

Lazy allocation: Opsiyonel özellikler (`spec_attr`) ihtiyaç duyulduğunda tahsis edilir.

```c
if(obj->spec_attr == NULL) {
    lv_obj_allocate_spec_attr(obj);  // İlk kullanımda tahsis
}
```

### 5.7 Strategy Pattern

Part-based styling: Farklı part'lara farklı stil stratejileri uygulanır.

```c
lv_obj_set_style_bg_color(slider, color1, LV_PART_MAIN);
lv_obj_set_style_bg_color(slider, color2, LV_PART_INDICATOR);
lv_obj_set_style_bg_color(slider, color3, LV_PART_KNOB);
```

### 5.8 Factory Pattern

Class-based nesne yaratma.

```c
lv_obj_t * obj = lv_obj_class_create_obj(&lv_btn_class, parent);
```

---

## Yeni Widget Geliştirme: Best Practices

### 6.1 Widget Geliştirme Adımları

#### Adım 1: Class Descriptor Tanımlama

```c
// my_widget.h
#ifndef LV_MY_WIDGET_H
#define LV_MY_WIDGET_H

#include "../../core/lv_obj.h"

// Class descriptor (extern)
extern const lv_obj_class_t lv_my_widget_class;

// Instance structure
typedef struct {
    lv_obj_t obj;              // FIRST MEMBER (base class)
    int32_t my_value;          // Özel alan 1
    uint8_t my_flag : 1;       // Özel alan 2
} lv_my_widget_t;

// API
lv_obj_t * lv_my_widget_create(lv_obj_t * parent);
void lv_my_widget_set_value(lv_obj_t * obj, int32_t value);
int32_t lv_my_widget_get_value(const lv_obj_t * obj);

#endif
```

#### Adım 2: Class Implementation

```c
// my_widget.c
#include "my_widget.h"

#define MY_CLASS &lv_my_widget_class

// Forward declarations
static void lv_my_widget_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_my_widget_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_my_widget_event(const lv_obj_class_t * class_p, lv_event_t * e);

// Class descriptor tanımı
const lv_obj_class_t lv_my_widget_class = {
    .base_class = &lv_obj_class,               // Base class
    .constructor_cb = lv_my_widget_constructor,
    .destructor_cb = lv_my_widget_destructor,
    .event_cb = lv_my_widget_event,
    .width_def = 100,                          // Varsayılan genişlik
    .height_def = 50,                          // Varsayılan yükseklik
    .instance_size = sizeof(lv_my_widget_t),   // Instance boyutu
    .editable = LV_OBJ_CLASS_EDITABLE_FALSE,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
};
```

#### Adım 3: Constructor

```c
static void lv_my_widget_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_my_widget_t * my_widget = (lv_my_widget_t *)obj;

    // Özel alanları başlat
    my_widget->my_value = 0;
    my_widget->my_flag = 0;

    // Flag'leri ayarla
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    LV_TRACE_OBJ_CREATE("finished");
}
```

#### Adım 4: Destructor (Opsiyonel)

```c
static void lv_my_widget_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_my_widget_t * my_widget = (lv_my_widget_t *)obj;

    // Dinamik tahsisleri serbest bırak
    if(my_widget->allocated_data) {
        lv_free(my_widget->allocated_data);
        my_widget->allocated_data = NULL;
    }
}
```

#### Adım 5: Event Handler

```c
static void lv_my_widget_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    // Base class event handler'ı çağır
    lv_res_t res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_my_widget_t * my_widget = (lv_my_widget_t *)obj;

    if(code == LV_EVENT_DRAW_MAIN) {
        // Özel çizim mantığı
        lv_draw_ctx_t * draw_ctx = lv_event_get_param(e);
        draw_my_widget(draw_ctx, obj);
    }
    else if(code == LV_EVENT_CLICKED) {
        // Tıklama işle
        my_widget->my_value++;
        lv_obj_invalidate(obj);  // Yeniden çiz

        // VALUE_CHANGED event gönder
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
    else if(code == LV_EVENT_KEY) {
        uint32_t key = *((uint32_t *)lv_event_get_param(e));
        if(key == LV_KEY_ENTER) {
            // Enter tuşu işle
        }
    }
}
```

#### Adım 6: Create Function

```c
lv_obj_t * lv_my_widget_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}
```

#### Adım 7: Public API

```c
void lv_my_widget_set_value(lv_obj_t * obj, int32_t value)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_my_widget_t * my_widget = (lv_my_widget_t *)obj;

    if(my_widget->my_value == value) return;  // Değişiklik yok

    my_widget->my_value = value;
    lv_obj_invalidate(obj);  // Görünümü güncelle

    lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
}

int32_t lv_my_widget_get_value(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_my_widget_t * my_widget = (lv_my_widget_t *)obj;
    return my_widget->my_value;
}
```

### 6.2 Best Practices Checklist

#### Memory Management

✅ **Zero-initialize**: Constructor'da tüm alanlar başlatılmalı
```c
lv_memzero(&my_widget->data, sizeof(my_widget->data));
```

✅ **Cleanup**: Destructor'da tüm dinamik tahsisler temizlenmeli
```c
if(my_widget->buffer) lv_free(my_widget->buffer);
```

✅ **Lazy allocation**: Nadiren kullanılan özellikler için
```c
if(my_widget->extended_data == NULL) {
    my_widget->extended_data = lv_malloc(...);
}
```

#### Event Handling

✅ **Base class çağrısı**: Event handler'da base'i çağır
```c
lv_res_t res = lv_obj_event_base(MY_CLASS, e);
if(res != LV_RES_OK) return;
```

✅ **Invalidation**: Görünüm değişirse invalidate et
```c
lv_obj_invalidate(obj);
```

✅ **Event notification**: Durum değişirse event gönder
```c
lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
```

#### Drawing

✅ **Clip area respect**: Clip sınırlarını aşma
```c
lv_draw_rect(draw_ctx, &draw_dsc, &obj->coords);
```

✅ **Ext draw size**: Extra çizim alanı gerekirse bildir
```c
if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
    lv_coord_t * size = lv_event_get_param(e);
    *size = LV_MAX(*size, my_shadow_width);
}
```

#### Type Safety

✅ **Assertion**: Public API'lerde type check
```c
LV_ASSERT_OBJ(obj, MY_CLASS);
```

✅ **Const correctness**: Read-only fonksiyonlarda const
```c
int32_t lv_my_widget_get_value(const lv_obj_t * obj);
```

#### Performance

✅ **Erken return**: Gereksiz işlemden kaçın
```c
if(my_widget->my_value == value) return;  // Değişiklik yok
```

✅ **Partial invalidation**: Sadece değişen kısmı invalidate et
```c
lv_obj_invalidate_area(obj, &changed_area);  // Tüm nesne yerine
```

### 6.3 Örnek: Basit Progress Ring Widget

```c
// lv_progress_ring.h
typedef struct {
    lv_obj_t obj;
    int32_t value;          // 0-100
    lv_coord_t thickness;   // Ring kalınlığı
} lv_progress_ring_t;

extern const lv_obj_class_t lv_progress_ring_class;

lv_obj_t * lv_progress_ring_create(lv_obj_t * parent);
void lv_progress_ring_set_value(lv_obj_t * obj, int32_t value);
```

```c
// lv_progress_ring.c
#include "lv_progress_ring.h"
#include "../../draw/lv_draw_arc.h"

#define MY_CLASS &lv_progress_ring_class

static void lv_progress_ring_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_progress_ring_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_ring(lv_event_t * e);

const lv_obj_class_t lv_progress_ring_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_progress_ring_constructor,
    .event_cb = lv_progress_ring_event,
    .width_def = 100,
    .height_def = 100,
    .instance_size = sizeof(lv_progress_ring_t),
};

static void lv_progress_ring_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_progress_ring_t * ring = (lv_progress_ring_t *)obj;

    ring->value = 0;
    ring->thickness = 10;

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void lv_progress_ring_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_res_t res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DRAW_MAIN) {
        draw_ring(e);
    }
    else if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_obj_t * obj = lv_event_get_target(e);
        lv_progress_ring_t * ring = (lv_progress_ring_t *)obj;
        lv_coord_t * size = lv_event_get_param(e);
        *size = LV_MAX(*size, ring->thickness / 2);
    }
}

static void draw_ring(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_progress_ring_t * ring = (lv_progress_ring_t *)obj;
    lv_draw_ctx_t * draw_ctx = lv_event_get_param(e);

    lv_coord_t cx = obj->coords.x1 + lv_obj_get_width(obj) / 2;
    lv_coord_t cy = obj->coords.y1 + lv_obj_get_height(obj) / 2;
    lv_coord_t radius = (LV_MIN(lv_obj_get_width(obj),
                                 lv_obj_get_height(obj)) / 2) - ring->thickness;

    // Arka plan ring (gri)
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = lv_color_hex(0xCCCCCC);
    arc_dsc.width = ring->thickness;
    arc_dsc.rounded = 1;
    lv_draw_arc(draw_ctx, &arc_dsc, cx, cy, radius, 0, 360);

    // İlerleme ring (mavi)
    int32_t angle = (ring->value * 360) / 100;
    arc_dsc.color = lv_color_hex(0x0080FF);
    lv_draw_arc(draw_ctx, &arc_dsc, cx, cy, radius, 270, 270 + angle);
}

lv_obj_t * lv_progress_ring_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_progress_ring_set_value(lv_obj_t * obj, int32_t value)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_progress_ring_t * ring = (lv_progress_ring_t *)obj;

    if(ring->value == value) return;

    ring->value = LV_CLAMP(0, value, 100);
    lv_obj_invalidate(obj);

    lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
}
```

---

## Kod Örnekleri

### 7.1 Custom Event Handler

```c
static void my_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Button clicked!");

        // State değiştir
        if(lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        }
    }
    else if(code == LV_EVENT_LONG_PRESSED) {
        LV_LOG_USER("Button long pressed!");
        // Async sil
        lv_obj_del_async(btn);
    }
}

// Kullanım
lv_obj_t * btn = lv_btn_create(parent);
lv_obj_add_event_cb(btn, my_button_event_cb, LV_EVENT_ALL, NULL);
```

### 7.2 Custom Drawing

```c
static void custom_draw_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DRAW_POST) {
        lv_obj_t * obj = lv_event_get_target(e);
        lv_draw_ctx_t * draw_ctx = lv_event_get_param(e);

        // Özel çizim: X işareti
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = lv_color_hex(0xFF0000);
        line_dsc.width = 3;

        lv_point_t p1 = {obj->coords.x1, obj->coords.y1};
        lv_point_t p2 = {obj->coords.x2, obj->coords.y2};
        lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);

        p1.x = obj->coords.x2;
        p2.x = obj->coords.x1;
        lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);
    }
}

// Kullanım
lv_obj_t * obj = lv_obj_create(parent);
lv_obj_add_event_cb(obj, custom_draw_event_cb, LV_EVENT_DRAW_POST, NULL);
```

### 7.3 Event Bubbling Kontrolü

```c
static void panel_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * current = lv_event_get_current_target(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        if(current != target) {
            // Event bubbled from child
            LV_LOG_USER("Child was clicked: %p", (void*)target);
        } else {
            // Panel itself clicked
            LV_LOG_USER("Panel clicked directly");
        }

        // Daha fazla bubble'ı durdur
        lv_event_stop_bubbling(e);
    }
}

// Panel oluştur ve event bubble etkinleştir
lv_obj_t * panel = lv_obj_create(screen);
lv_obj_add_flag(panel, LV_OBJ_FLAG_EVENT_BUBBLE);
lv_obj_add_event_cb(panel, panel_event_cb, LV_EVENT_ALL, NULL);

// Child button'lar ekle
lv_obj_t * btn1 = lv_btn_create(panel);
lv_obj_t * btn2 = lv_btn_create(panel);
// Button event'leri panel'e bubble olacak
```

### 7.4 Partial Invalidation

```c
static void update_indicator(lv_obj_t * obj, int32_t new_value)
{
    my_widget_t * widget = (my_widget_t *)obj;

    // Eski indicator alanını kaydet
    lv_area_t old_indicator_area;
    calculate_indicator_area(widget, widget->value, &old_indicator_area);

    // Değeri güncelle
    widget->value = new_value;

    // Yeni indicator alanını hesapla
    lv_area_t new_indicator_area;
    calculate_indicator_area(widget, widget->value, &new_indicator_area);

    // Sadece değişen alanı invalidate et
    lv_obj_invalidate_area(obj, &old_indicator_area);
    lv_obj_invalidate_area(obj, &new_indicator_area);

    // Tüm widget'ı invalidate etmekten daha verimli
    // lv_obj_invalidate(obj);  // ❌ Gereksiz yere tüm nesneyi çizer
}
```

### 7.5 Tree Walking

```c
static lv_obj_tree_walk_res_t disable_all_buttons_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(user_data);

    if(lv_obj_check_type(obj, &lv_btn_class)) {
        lv_obj_add_state(obj, LV_STATE_DISABLED);
        LV_LOG_USER("Disabled button: %p", (void*)obj);
    }

    return LV_OBJ_TREE_WALK_NEXT;  // Devam et
}

// Tüm screen'deki button'ları disable et
lv_obj_tree_walk(lv_scr_act(), disable_all_buttons_cb, NULL);
```

---

## Özet

### LVGL Widget Altyapısının Temel Özellikleri

| Özellik | Açıklama |
|---------|----------|
| **Base Class** | `lv_obj_t` - tüm widget'ların temel tipi |
| **Inheritance** | Single inheritance, `base_class` pointer ile |
| **Polymorphism** | Virtual method'lar (event_cb, constructor_cb, destructor_cb) |
| **Memory** | Lazy allocation (spec_attr), zero-initialization |
| **Events** | Observer pattern, bubbling, 3-phase dispatch |
| **Rendering** | Dirty region tracking, recursive drawing |
| **Lifecycle** | Two-phase init/destruct, constructor/destructor chain |
| **Type Safety** | Runtime type checking (lv_obj_has_class) |

### Kritik Dosyalar

| Dosya | Açıklama |
|-------|----------|
| `src/core/lv_obj.h/c` | Base object implementation |
| `src/core/lv_obj_class.h/c` | Class system, lifecycle management |
| `src/core/lv_event.h/c` | Event dispatch ve handling |
| `src/core/lv_refr.h/c` | Rendering ve invalidation |
| `src/widgets/*/lv_*.c` | Widget örnekleri |

---

**Doküman Versiyonu**: 1.0
**LVGL Versiyonu**: v8.x
**Tarih**: 2025-11-18
**Dil**: Türkçe

