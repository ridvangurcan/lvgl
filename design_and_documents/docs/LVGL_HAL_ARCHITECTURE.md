# LVGL Hardware Abstraction Layer (HAL) Mimarisi

## İçindekiler

1. [Giriş](#giriş)
2. [HAL Mimarisi Genel Bakış](#hal-mimarisi-genel-bakış)
3. [Display HAL (Ekran Sürücüsü)](#display-hal-ekran-sürücüsü)
4. [Input Device HAL (Giriş Cihazı)](#input-device-hal-giriş-cihazı)
5. [Tick HAL (Zaman Yönetimi)](#tick-hal-zaman-yönetimi)
6. [HAL Entegrasyonu](#hal-entegrasyonu)
7. [Best Practices](#best-practices)
8. [Sonuç](#sonuç)

---

## Giriş

**Hardware Abstraction Layer (HAL)**, donanım ile yazılım arasında soyutlama katmanı sağlayan bir mimari pattern'idir. LVGL'nin HAL mimarisi, farklı donanım platformlarında (STM32, ESP32, Linux, Windows, vb.) aynı kod tabanının çalışmasını sağlar.

### HAL'in Faydaları

```
✓ Platform Bağımsızlığı: Aynı UI kodu farklı MCU'larda çalışır
✓ Taşınabilirlik: Yeni platforma geçiş kolay
✓ Test Edilebilirlik: PC'de test, MCU'da deploy
✓ Bakım Kolaylığı: Donanım değişince sadece HAL güncellenir
✓ Modülerlik: Her donanım bileşeni ayrı soyutlanır
```

### LVGL HAL Bileşenleri

| Bileşen | Dosyalar | Amaç |
|---------|----------|------|
| **Display HAL** | `lv_hal_disp.h/c` | Ekran sürücüsü abstraction |
| **Input Device HAL** | `lv_hal_indev.h/c` | Touch, keyboard, encoder abstraction |
| **Tick HAL** | `lv_hal_tick.h/c` | Zaman/timer abstraction |
| **Main HAL** | `lv_hal.h` | Tüm HAL bileşenlerini içerir |

---

## HAL Mimarisi Genel Bakış

### Katmanlar

```
┌─────────────────────────────────────────────────┐
│          LVGL CORE (Platform Agnostic)          │
│  lv_obj, lv_event, lv_refr, lv_draw, ...      │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│              HARDWARE ABSTRACTION LAYER         │
│  ┌─────────────┬──────────────┬──────────────┐ │
│  │ Display HAL │ Input Device │   Tick HAL   │ │
│  │             │     HAL      │              │ │
│  └─────────────┴──────────────┴──────────────┘ │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│         HARDWARE DRIVERS (Platform Specific)     │
│  STM32 HAL, ESP-IDF, Linux FB, Windows GDI...  │
└─────────────────────────────────────────────────┘
```

### Ana Header

📁 `src/hal/lv_hal.h`
```c
#include "lv_hal_disp.h"   // Display abstraction
#include "lv_hal_indev.h"  // Input device abstraction
#include "lv_hal_tick.h"   // Tick/timing abstraction

// DPI hesaplama (Android-style)
#define _LV_DPX_CALC(dpi, n) ((n) == 0 ? 0 : LV_MAX((((dpi) * (n) + 80) / 160), 1))
#define LV_DPX(n) _LV_DPX_CALC(lv_disp_get_dpi(NULL), n)
```

**DPI Conversion**: Farklı çözünürlükteki ekranlarda tutarlı boyutlar
- 160 DPI ekranda: 1 DPX = 1 pixel
- 320 DPI ekranda: 1 DPX = 2 pixel

---

## Display HAL (Ekran Sürücüsü)

### Genel Bakış

Display HAL, LVGL'nin ekrana çizim yapmasını sağlar. Kullanıcı, ekran sürücüsü için callback fonksiyonları sağlar ve LVGL bu fonksiyonları çağırarak render sonuçlarını ekrana yazar.

### Ana Yapılar

#### 1. Display Draw Buffer

📁 `src/hal/lv_hal_disp.h:53-66`
```c
typedef struct _lv_disp_draw_buf_t {
    void * buf1;                    // İlk buffer (zorunlu)
    void * buf2;                    // İkinci buffer (opsiyonel, double buffering)

    // Internal (LVGL tarafından yönetilir)
    void * buf_act;                 // Aktif buffer
    uint32_t size;                  // Buffer boyutu (pixel sayısı)

    volatile int flushing;          // Flush işlemi devam ediyor mu?
    volatile int flushing_last;     // Son chunk mu flush ediliyor?
    volatile uint32_t last_area : 1;// Son alan mı render ediliyor?
    volatile uint32_t last_part : 1;// Alanın son parçası mı?
} lv_disp_draw_buf_t;
```

**Buffer Stratejileri**:

1. **Single Buffer**:
   ```
   buf1: [████████████] ← LVGL çizer, flush eder
   buf2: NULL

   Avantaj: Az RAM kullanır
   Dezavantaj: Yavaş (çizim bekler)
   ```

2. **Double Buffer**:
   ```
   buf1: [████████████] ← LVGL çizer
   buf2: [████████████] ← DMA flush eder

   Avantaj: Hızlı (paralel işlem)
   Dezavantaj: 2x RAM kullanır
   ```

3. **Partial Buffer**:
   ```
   Screen: 480x320 = 153,600 pixels
   Buffer: 480x10 = 4,800 pixels (32 strip)

   Avantaj: Az RAM (sadece %3)
   Dezavantaj: 32 flush çağrısı
   ```

#### 2. Display Driver

📁 `src/hal/lv_hal_disp.h:80-150`
```c
typedef struct _lv_disp_drv_t {
    // ═══ EKRAN BİLGİLERİ ═══
    lv_coord_t hor_res;              // Yatay çözünürlük (örn: 480)
    lv_coord_t ver_res;              // Dikey çözünürlük (örn: 320)

    lv_coord_t physical_hor_res;     // Fiziksel ekran genişliği
    lv_coord_t physical_ver_res;     // Fiziksel ekran yüksekliği
    lv_coord_t offset_x;             // X offset (virtual screen için)
    lv_coord_t offset_y;             // Y offset

    // ═══ BUFFER YÖNETİMİ ═══
    lv_disp_draw_buf_t * draw_buf;   // Draw buffer

    // ═══ AYARLAR (bitfields) ═══
    uint32_t direct_mode : 1;        // 1: Tam ekran buffer, absolute coord
    uint32_t full_refresh : 1;       // 1: Her zaman full redraw (dirty region yok)
    uint32_t sw_rotate : 1;          // 1: Software rotation (yavaş)
    uint32_t antialiasing : 1;       // 1: Anti-aliasing aktif
    uint32_t rotated : 2;            // Ekran rotasyonu (0/90/180/270)
    uint32_t screen_transp : 1;      // Transparent ekran desteği
    uint32_t dpi : 10;               // DPI (default: 130)

    lv_color_format_t color_format;  // Renk formatı (RGB565, RGB888, vb.)

    // ═══ CALLBACK FONKSİYONLARI ═══

    /**
     * ZORUNLU: Buffer'ı ekrana yaz
     * LVGL render bitince bu fonksiyonu çağırır
     * @param disp_drv Display driver
     * @param area Çizilecek alan (x1,y1,x2,y2)
     * @param color_p Pixel verisi
     */
    void (*flush_cb)(struct _lv_disp_drv_t * disp_drv,
                     const lv_area_t * area,
                     lv_color_t * color_p);

    /**
     * OPSİYONEL: Invalid alanı yuvarla
     * Örnek: Monochrome ekranda Y'yi 8'in katına yuvarla
     */
    void (*rounder_cb)(struct _lv_disp_drv_t * disp_drv,
                       lv_area_t * area);

    /**
     * OPSİYONEL: Buffer'ı temizle
     */
    void (*clear_cb)(struct _lv_disp_drv_t * disp_drv,
                     uint8_t * buf,
                     uint32_t size);

    /**
     * OPSİYONEL: Performans monitörü
     * Her refresh sonrası çağrılır
     * @param time Render + flush süresi (ms)
     * @param px Flush edilen pixel sayısı
     */
    void (*monitor_cb)(struct _lv_disp_drv_t * disp_drv,
                       uint32_t time,
                       uint32_t px);

    /**
     * OPSİYONEL: Bekleme callback'i
     * LVGL bir işlemin tamamlanmasını beklerken çağrılır
     * Örnek: Task yield, WDT feed
     */
    void (*wait_cb)(struct _lv_disp_drv_t * disp_drv);

    /**
     * OPSİYONEL: CPU cache temizleme
     * DMA kullanıyorsa gerekli
     */
    void (*clean_dcache_cb)(struct _lv_disp_drv_t * disp_drv);

    /**
     * OPSİYONEL: Driver güncelleme notification
     */
    void (*drv_update_cb)(struct _lv_disp_drv_t * disp_drv);

    /**
     * OPSİYONEL: Render başlangıcı notification
     */
    void (*render_start_cb)(struct _lv_disp_drv_t * disp_drv);

    // ═══ DİĞER AYARLAR ═══
    lv_color_t color_chroma_key;     // Transparent color (default: magenta)

    lv_draw_ctx_t * draw_ctx;        // Draw context
    void (*draw_ctx_init)(...);      // Draw context init
    void (*draw_ctx_deinit)(...);    // Draw context deinit
    size_t draw_ctx_size;            // Draw context boyutu

    #if LV_USE_USER_DATA
    void * user_data;                // Kullanıcı verisi
    #endif
} lv_disp_drv_t;
```

#### 3. Display Object

📁 `src/hal/lv_hal_disp.h:178-220` (yapıdan kısaltılmış)
```c
typedef struct _lv_disp_t {
    lv_disp_drv_t * driver;          // Driver pointer

    lv_timer_t * refr_timer;         // Refresh timer

    // ═══ SCREENS ═══
    struct _lv_obj_t ** screens;     // Tüm ekranlar (array)
    struct _lv_obj_t * act_scr;      // Aktif ekran
    struct _lv_obj_t * prev_scr;     // Önceki ekran
    struct _lv_obj_t * top_layer;    // Top layer (her zaman üstte)
    struct _lv_obj_t * sys_layer;    // System layer (cursor, vb.)

    // ═══ DIRTY REGIONS ═══
    lv_area_t inv_areas[LV_INV_BUF_SIZE];      // Invalid areas (32)
    uint8_t inv_area_joined[LV_INV_BUF_SIZE];  // Joined flags
    uint16_t inv_p;                             // Invalid area count

    // ═══ RENDERING STATE ═══
    uint32_t rendering_in_progress : 1;  // Render devam ediyor mu?
    int32_t inv_en_cnt;                  // Invalidation enable counter

    // ═══ THEME & STYLE ═══
    struct _lv_theme_t * theme;          // Aktif tema

    #if LV_USE_USER_DATA
    void * user_data;
    #endif
} lv_disp_t;
```

### Display HAL Kullanımı

#### Temel Kurulum

```c
#include "lvgl.h"

// 1. Draw buffer oluştur
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10];  // 10 satırlık buffer
static lv_color_t buf2[SCREEN_WIDTH * 10];  // Double buffering

void setup_display(void)
{
    // Draw buffer'ı başlat
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, SCREEN_WIDTH * 10);

    // 2. Display driver oluştur
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);  // Default değerlerle başlat

    // 3. Driver'ı yapılandır
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = my_flush_cb;  // ZORUNLU!
    disp_drv.draw_buf = &draw_buf;

    // OPSİYONEL callback'ler
    disp_drv.monitor_cb = my_monitor_cb;
    disp_drv.rounder_cb = my_rounder_cb;

    // 4. Display'i kaydet
    lv_disp_t * disp = lv_disp_drv_register(&disp_drv);
}

// ZORUNLU: Flush callback implementasyonu
void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // 1. Pixel verisini ekrana yaz
    int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            // Ekranınıza göre pixel yazma fonksiyonu
            set_pixel(x, y, *color_p);
            color_p++;
        }
    }

    // 2. LVGL'ye flush tamamlandı bildir (ZORUNLU!)
    lv_disp_flush_ready(disp_drv);
}
```

#### DMA ile Hızlı Flush

```c
void my_flush_cb_dma(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // Alan boyutunu hesapla
    int32_t width = area->x2 - area->x1 + 1;
    int32_t height = area->y2 - area->y1 + 1;
    int32_t size = width * height;

    // Ekranın ilgili alanını seç
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    // DMA ile veri transfer et (non-blocking)
    lcd_dma_transfer((uint16_t*)color_p, size);

    // DMA interrupt'ta lv_disp_flush_ready() çağrılacak!
}

// DMA complete interrupt
void DMA_IRQHandler(void)
{
    if(dma_transfer_complete()) {
        lv_disp_flush_ready(&disp_drv);  // LVGL'ye bildir
    }
}
```

#### Monitor Callback (Performans)

```c
void my_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time_ms, uint32_t pixels)
{
    float fps = 1000.0f / time_ms;
    float mpps = (pixels / 1000000.0f) / (time_ms / 1000.0f);  // Megapixels/sec

    printf("FPS: %.1f, Time: %u ms, Pixels: %u, MPps: %.2f\n",
           fps, time_ms, pixels, mpps);
}
```

### Display Rotation

```c
typedef enum {
    LV_DISP_ROT_NONE = 0,  // Rotasyon yok
    LV_DISP_ROT_90,        // 90° saat yönünde
    LV_DISP_ROT_180,       // 180°
    LV_DISP_ROT_270        // 270° (veya -90°)
} lv_disp_rot_t;

// Kullanım
disp_drv.rotated = LV_DISP_ROT_90;
disp_drv.sw_rotate = 1;  // Software rotation (yavaş ama esnek)
```

---

## Input Device HAL (Giriş Cihazı)

### Genel Bakış

Input Device HAL, kullanıcı giriş cihazlarını (touchscreen, mouse, keyboard, encoder) soyutlar.

### Desteklenen Cihaz Tipleri

📁 `src/hal/lv_hal_indev.h:62-68`
```c
typedef enum {
    LV_INDEV_TYPE_NONE,     // Başlatılmamış
    LV_INDEV_TYPE_POINTER,  // Touchscreen, mouse, trackpad
    LV_INDEV_TYPE_KEYPAD,   // Klavye, keypad
    LV_INDEV_TYPE_BUTTON,   // Hardware butonlar (ekran koordinatlarına atanmış)
    LV_INDEV_TYPE_ENCODER,  // Rotary encoder (sağ, sol, basma)
} lv_indev_type_t;
```

### Input Data Yapısı

📁 `src/hal/lv_hal_indev.h:77-85`
```c
typedef struct {
    lv_point_t point;           // POINTER için: (x, y) koordinat
    uint32_t key;               // KEYPAD için: basılan tuş (LV_KEY_UP, LV_KEY_ENTER, vb.)
    uint32_t btn_id;            // BUTTON için: buton ID
    int16_t enc_diff;           // ENCODER için: adım sayısı (+ sağ, - sol)

    lv_indev_state_t state;     // LV_INDEV_STATE_PRESSED veya RELEASED
    bool continue_reading;      // true ise read callback tekrar çağrılır
} lv_indev_data_t;
```

### Input Device Driver

📁 `src/hal/lv_hal_indev.h:88-130`
```c
typedef struct _lv_indev_drv_t {
    // ═══ CİHAZ TİPİ ═══
    lv_indev_type_t type;        // POINTER, KEYPAD, BUTTON, ENCODER

    // ═══ ZORUNLU CALLBACK ═══
    /**
     * Input cihazından veri oku
     * @param indev_drv Driver pointer
     * @param data Doldurulacak data struct
     */
    void (*read_cb)(struct _lv_indev_drv_t * indev_drv,
                    lv_indev_data_t * data);

    // ═══ OPSİYONEL CALLBACK ═══
    /**
     * Feedback (haptic, LED, vb.)
     * @param event LV_EVENT_CLICKED, LV_EVENT_FOCUSED, vb.
     */
    void (*feedback_cb)(struct _lv_indev_drv_t *, uint8_t event);

    #if LV_USE_USER_DATA
    void * user_data;
    #endif

    // ═══ BAĞLANTI ═══
    struct _lv_disp_t * disp;    // Hangi display'e bağlı
    lv_timer_t * read_timer;     // Periyodik okuma timer'ı

    // ═══ SCROLL AYARLARI ═══
    uint8_t scroll_limit;        // Kaç pixel kaydırmadan önce scroll (default: 10)
    uint8_t scroll_throw;        // Scroll throw slow-down % (default: 10)

    // ═══ GESTURE AYARLARI ═══
    uint8_t gesture_min_velocity;// Minimum hız (default: 3 px)
    uint8_t gesture_limit;       // Gesture threshold (default: 50 px)

    // ═══ LONG PRESS AYARLARI ═══
    uint16_t long_press_time;    // Long press süresi ms (default: 400)
    uint16_t long_press_repeat_time; // Repeat periyodu ms (default: 100)
} lv_indev_drv_t;
```

### Input Device Kullanımı

#### 1. Touchscreen (POINTER)

```c
void setup_touchscreen(void)
{
    // Input device driver oluştur
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    // Yapılandır
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchscreen_read;

    // Kaydet
    lv_indev_t * indev = lv_indev_drv_register(&indev_drv);
}

// Read callback
void touchscreen_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    // 1. Touchscreen'den veri oku (hardware-specific)
    bool is_pressed = touch_is_pressed();

    if(is_pressed) {
        // Basılı - koordinatları oku
        data->point.x = touch_get_x();
        data->point.y = touch_get_y();
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        // Basılı değil
        data->state = LV_INDEV_STATE_RELEASED;
    }

    // 2. continue_reading genelde false (bir okuma yeterli)
    data->continue_reading = false;
}
```

#### 2. Keyboard (KEYPAD)

```c
void setup_keyboard(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keyboard_read;

    lv_indev_t * indev = lv_indev_drv_register(&indev_drv);

    // Keyboard'u bir group'a atayarak kullan
    lv_group_t * g = lv_group_create();
    lv_indev_set_group(indev, g);
    lv_group_add_obj(g, textarea);  // Textarea'ya odaklan
}

void keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    uint32_t key = keyboard_get_key();  // Hardware-specific

    if(key != 0) {
        // Tuş basılı
        data->key = key;  // LV_KEY_UP, LV_KEY_DOWN, LV_KEY_ENTER, vb.
        data->state = LV_INDEV_STATE_PRESSED;
        last_key = key;
    }
    else if(last_key != 0) {
        // Tuş bırakıldı
        data->key = last_key;
        data->state = LV_INDEV_STATE_RELEASED;
        last_key = 0;
    }

    data->continue_reading = false;
}
```

**LVGL Key Codes**:
```c
LV_KEY_UP        // Yukarı
LV_KEY_DOWN      // Aşağı
LV_KEY_LEFT      // Sol
LV_KEY_RIGHT     // Sağ
LV_KEY_ESC       // Escape
LV_KEY_DEL       // Backspace
LV_KEY_BACKSPACE // Backspace
LV_KEY_ENTER     // Enter
LV_KEY_NEXT      // Tab (sonraki obje)
LV_KEY_PREV      // Shift+Tab (önceki obje)
LV_KEY_HOME      // Home
LV_KEY_END       // End
```

#### 3. Encoder (ENCODER)

```c
void setup_encoder(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read;

    lv_indev_t * indev = lv_indev_drv_register(&indev_drv);

    // Group ile kullan
    lv_group_t * g = lv_group_create();
    lv_indev_set_group(indev, g);
    lv_group_add_obj(g, slider);
}

void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    // 1. Encoder'dan değişikliği oku (hardware-specific)
    int16_t diff = encoder_get_diff();  // Pozitif: sağ, Negatif: sol
    encoder_reset_diff();

    // 2. Buton basıldı mı?
    bool btn_pressed = encoder_button_pressed();

    // 3. Data doldur
    data->enc_diff = diff;
    data->state = btn_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->continue_reading = false;
}
```

#### 4. Hardware Buttons (BUTTON)

```c
void setup_hardware_buttons(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_drv.read_cb = button_read;

    lv_indev_t * indev = lv_indev_drv_register(&indev_drv);

    // Butonları ekran koordinatlarına ata
    static const lv_point_t btn_points[3] = {
        {10, 10},   // Button 0: Sol üst köşe
        {100, 50},  // Button 1: Orta
        {200, 200}, // Button 2: Sağ alt
    };
    lv_indev_set_button_points(indev, btn_points);
}

void button_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_btn = 0;

    // Hangi buton basılı? (hardware-specific)
    int btn_id = get_pressed_button_id();  // 0, 1, 2, -1 (hiçbiri)

    if(btn_id >= 0) {
        data->btn_id = btn_id;
        data->state = LV_INDEV_STATE_PRESSED;
        last_btn = btn_id;
    }
    else if(last_btn != 0) {
        data->btn_id = last_btn;
        data->state = LV_INDEV_STATE_RELEASED;
        last_btn = 0;
    }

    data->continue_reading = false;
}
```

### Input State Management

```c
typedef enum {
    LV_INDEV_STATE_RELEASED = 0,  // Bırakıldı
    LV_INDEV_STATE_PRESSED        // Basılı
} lv_indev_state_t;
```

**State Transition**:
```
     ┌─────────────┐
     │  RELEASED   │ ← Initial state
     └──────┬──────┘
            │ Touch/Press
            ↓
     ┌─────────────┐
     │  PRESSED    │
     └──────┬──────┘
            │ Release
            ↓
     ┌─────────────┐
     │  RELEASED   │
     └─────────────┘
```

---

## Tick HAL (Zaman Yönetimi)

### Genel Bakış

Tick HAL, LVGL'ye milisaniye çözünürlüğünde zaman bilgisi sağlar. Animasyonlar, timer'lar ve input device timing'i için kritik öneme sahiptir.

### API

📁 `src/hal/lv_hal_tick.h:43-59`
```c
/**
 * Tick sayacını artır (periyodik çağrılmalı)
 * @param tick_period Bu fonksiyonun çağrılma periyodu (ms)
 */
void lv_tick_inc(uint32_t tick_period);

/**
 * Sistem başlangıcından beri geçen milisaniyeleri al
 * @return Geçen milisaniye sayısı
 */
uint32_t lv_tick_get(void);

/**
 * Belirli bir zaman damgasından beri geçen süreyi hesapla
 * @param prev_tick Önceki zaman damgası (lv_tick_get() return değeri)
 * @return Geçen milisaniye sayısı
 */
uint32_t lv_tick_elaps(uint32_t prev_tick);
```

### Tick HAL Kurulumu

#### Yöntem 1: lv_tick_inc() Kullanımı (Yaygın)

```c
// Sistem timer interrupt'ında (her 1 ms)
void SysTick_Handler(void)
{
    lv_tick_inc(1);  // 1 ms geçti
}

// Veya RTOS task'ında
void tick_task(void * param)
{
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1));
        lv_tick_inc(1);
    }
}
```

#### Yöntem 2: Custom Tick (lv_conf.h)

```c
// lv_conf.h içinde
#define LV_TICK_CUSTOM 1

// Kendi tick fonksiyonunuzu tanımlayın
uint32_t my_tick_get(void)
{
    return HAL_GetTick();  // STM32 HAL
    // veya
    return xTaskGetTickCount() * portTICK_PERIOD_MS;  // FreeRTOS
    // veya
    return millis();  // Arduino
}

#define LV_TICK_CUSTOM_INCLUDE "my_tick.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (my_tick_get())
```

### Tick Kullanım Örnekleri

#### 1. Elapsed Time Hesaplama

```c
uint32_t start_time = lv_tick_get();

// Uzun işlem...
heavy_computation();

uint32_t elapsed = lv_tick_elaps(start_time);
printf("İşlem %u ms sürdü\n", elapsed);
```

#### 2. Timeout İmplementasyonu

```c
uint32_t timeout_start = lv_tick_get();
uint32_t timeout_ms = 1000;  // 1 saniye

while(1) {
    if(is_operation_complete()) {
        break;  // Başarılı
    }

    if(lv_tick_elaps(timeout_start) > timeout_ms) {
        printf("Timeout!\n");
        return false;
    }

    // Bekle...
}
```

#### 3. Periyodik Task

```c
uint32_t last_update = 0;
uint32_t update_period = 100;  // 100 ms

void periodic_task(void)
{
    if(lv_tick_elaps(last_update) >= update_period) {
        // Periyodik güncelleme
        update_sensor_data();

        last_update = lv_tick_get();
    }
}
```

---

## HAL Entegrasyonu

### Tam Entegrasyon Örneği

```c
#include "lvgl.h"

// ═══ GLOBAL DEĞİŞKENLER ═══
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10];

static lv_indev_drv_t indev_drv;

// ═══ HAL KURULUMU ═══
void lvgl_hal_init(void)
{
    // 1. LVGL'yi başlat
    lv_init();

    // 2. Tick HAL'i kur
    // SysTick interrupt'ında lv_tick_inc(1) çağrılıyor

    // 3. Display HAL'i kur
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 10);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.monitor_cb = my_monitor_cb;

    lv_disp_t * disp = lv_disp_drv_register(&disp_drv);

    // 4. Input HAL'i kur
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchscreen_read;

    lv_indev_t * indev = lv_indev_drv_register(&indev_drv);
}

// ═══ MAIN LOOP ═══
int main(void)
{
    // Hardware init
    hardware_init();

    // LVGL HAL init
    lvgl_hal_init();

    // UI oluştur
    create_ui();

    // Main loop
    while(1) {
        lv_timer_handler();  // LVGL task handler (5-10 ms'de bir çağır)
        delay_ms(5);
    }
}

// ═══ SYSTICK INTERRUPT ═══
void SysTick_Handler(void)
{
    lv_tick_inc(1);  // Tick HAL
}
```

### Platform-Specific Örnekler

#### STM32 + HAL

```c
void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;

    // STM32 LTDC + DMA2D
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)color_p, LTDC_LAYER_1);

    lv_disp_flush_ready(disp_drv);
}
```

#### ESP32 + ESP-IDF

```c
void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    esp_lcd_panel_draw_bitmap(panel_handle,
                               area->x1, area->y1,
                               area->x2 + 1, area->y2 + 1,
                               color_p);

    lv_disp_flush_ready(disp_drv);
}
```

#### Linux Framebuffer

```c
static int fbfd = 0;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static uint8_t *fbp = NULL;

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            long location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                            (y + vinfo.yoffset) * finfo.line_length;
            *((uint16_t*)(fbp + location)) = color_p->full;
            color_p++;
        }
    }

    lv_disp_flush_ready(disp_drv);
}
```

---

## Best Practices

### Display HAL

```
✓ DMA Kullan: flush_cb'de DMA kullanarak paralel işlem yap
✓ Double Buffering: Yeterli RAM varsa 2 buffer kullan
✓ Partial Buffer: RAM kısıtlıysa 1/10 ekran boyutunda buffer
✓ Monitor Callback: FPS ve performansı takip et
✓ flush_ready: Her flush sonunda mutlaka çağır (DMA interrupt'ta)
✓ Cache Temizle: DMA kullanıyorsa dcache_clean callback kullan
```

### Input HAL

```
✓ Debouncing: Touchscreen/button için debouncing uygula
✓ Calibration: Touchscreen kalibrasyonu yap
✓ Continue Reading: Genelde false, çoklu touch için true
✓ Group Kullan: Keyboard/encoder için group mechanism kullan
✓ Feedback: Haptic/LED feedback için feedback_cb kullan
```

### Tick HAL

```
✓ 1 ms Çözünürlük: Mutlaka 1 ms çözünürlükte tick sağla
✓ Interrupt Güvenli: lv_tick_inc() interrupt-safe
✓ Overflow Safe: uint32_t overflow ~49 gün sonra, sorun değil
✓ RTOS Uyumlu: RTOS tick'ini kullanabilirsin
```

### Genel

```
✓ lv_timer_handler(): Main loop'ta 5-10 ms'de bir çağır
✓ Thread Safety: LVGL tek thread'de çalışır, mutex kullan
✓ Watchdog: Long operations'ta watchdog feed et
✓ Power Management: Sleep mode'da tick ve timer'ları durdur
```

---

## Sonuç

LVGL'nin Hardware Abstraction Layer mimarisi, platformbağımsız bir GUI framework oluşturmasını sağlar.

### Özet

| HAL Bileşeni | Amaç | Zorunlu Callback'ler |
|--------------|------|---------------------|
| **Display HAL** | Ekrana çizim | `flush_cb` |
| **Input Device HAL** | Kullanıcı girişi | `read_cb` |
| **Tick HAL** | Zaman yönetimi | `lv_tick_inc()` veya custom |

### Entegrasyon Adımları

1. ✅ `lv_init()` - LVGL'yi başlat
2. ✅ Tick HAL'i kur (SysTick veya custom)
3. ✅ Display buffer ve driver kur
4. ✅ Input device driver kur
5. ✅ Main loop'ta `lv_timer_handler()` çağır

### Platform Desteği

LVGL HAL sayesinde şu platformlarda çalışır:
- ✅ **MCU**: STM32, ESP32, NXP i.MX RT, Nordic nRF52, vb.
- ✅ **OS**: FreeRTOS, Zephyr, Linux, Windows, macOS
- ✅ **Board**: Arduino, Raspberry Pi, BeagleBone, vb.

**Sonuç**: Bir kez yazdığınız UI kodu, tüm platformlarda çalışır! 🚀

---

**Doküman Sürümü**: 1.0
**Tarih**: 2024
**Analiz Edilen LVGL Versiyonu**: v8.x
**İncelenen Dizin**: `/src/hal`
