# LVGL Draw Subsystem Mimari Analizi

## İçindekiler
1. [Genel Bakış](#genel-bakış)
2. [Mimari Yapı](#mimari-yapı)
3. [Tasarım Desenleri](#tasarım-desenleri)
4. [Çizim Teknikleri ve Yöntemleri](#çizim-teknikleri-ve-yöntemleri)
5. [Gerçek Zamanlı Çizim Stratejileri](#gerçek-zamanlı-çizim-stratejileri)
6. [Donanım İletişimi](#donanım-iletişimi)
7. [Backend Mimarisi](#backend-mimarisi)
8. [Performans Optimizasyonları](#performans-optimizasyonları)
9. [Veri Akışı](#veri-akışı)

---

## Genel Bakış

LVGL'nin `/src/draw` modülü, grafik kullanıcı arayüzü elementlerinin ekrana çizilmesinden sorumlu olan çekirdek alt sistemdir. Bu modül, donanımdan bağımsız bir soyutlama katmanı sağlayarak farklı rendering backend'lerinin (CPU tabanlı software rendering, GPU hızlandırma, DMA tabanlı çizim vb.) sorunsuz bir şekilde entegre edilmesini mümkün kılar.

### Modül Yapısı

```
src/draw/
├── lv_draw.h/c                 # Ana draw API ve başlatma
├── lv_draw_ctx.h               # Draw context yapısı (Strategy Pattern)
├── lv_draw_rect.h/c            # Dikdörtgen çizim API
├── lv_draw_label.h/c           # Metin rendering API
├── lv_draw_img.h/c             # Görüntü çizim API
├── lv_draw_line.h/c            # Çizgi çizim API
├── lv_draw_arc.h/c             # Yay/daire çizim API
├── lv_draw_triangle.h/c        # Üçgen çizim API
├── lv_draw_mask.h/c            # Maskeleme sistemi
├── lv_draw_layer.h/c           # Layer yönetimi
├── lv_draw_transform.h/c       # Transformasyon işlemleri
├── lv_img_decoder.h/c          # Görüntü decoder factory
├── lv_img_cache.h/c            # Görüntü önbellek yönetimi
│
├── sw/                         # Software rendering backend (CPU)
│   ├── lv_draw_sw.h/c
│   ├── lv_draw_sw_blend.h/c
│   ├── lv_draw_sw_gradient.h/c
│   ├── lv_draw_sw_dither.h/c
│   ├── lv_draw_sw_transform.h/c
│   └── ... (primitive çizimler)
│
├── nxp/                        # NXP GPU backends
│   ├── pxp/                    # PXP (Pixel Pipeline) backend
│   └── vglite/                 # VGLite GPU backend
│
├── stm32_dma2d/                # STM32 DMA2D hardware accelerator
├── arm2d/                      # ARM-2D acceleration library
├── gd32_ipa/                   # GigaDevice IPA hardware
└── sdl/                        # SDL2 GPU rendering backend
```

**Toplam Dosya Sayısı:** 98 dosya
**Backend Sayısı:** 7 farklı rendering backend
**Kod Satırı:** ~25,000+ satır (tüm backend'ler dahil)

---

## Mimari Yapı

### 1. Katmanlı Mimari

LVGL draw subsystem'i klasik bir katmanlı mimari takip eder:

```
┌─────────────────────────────────────────────┐
│   Widget Layer (Button, Label, Chart...)    │
│          (src/widgets/*)                     │
└──────────────────┬──────────────────────────┘
                   │ draw_rect(), draw_label()
┌──────────────────▼──────────────────────────┐
│      Draw API Layer (lv_draw_*.h)           │
│   - Donanımdan bağımsız API                 │
│   - Descriptor tabanlı çizim komutları       │
└──────────────────┬──────────────────────────┘
                   │ lv_draw_ctx_t function pointers
┌──────────────────▼──────────────────────────┐
│    Draw Context (Strategy Pattern)          │
│    lv_draw_ctx_t - 15 function pointer      │
│   - draw_rect, draw_img, draw_line...       │
└──────────────────┬──────────────────────────┘
                   │ Backend implementation
┌──────────────────▼──────────────────────────┐
│   Backend Implementations                    │
│  ┌────────┬────────┬────────┬─────────┐     │
│  │   SW   │  PXP   │ DMA2D  │  SDL    │     │
│  │  CPU   │  GPU   │  DMA   │ GPU/CPU │     │
│  └────────┴────────┴────────┴─────────┘     │
└──────────────────┬──────────────────────────┘
                   │ flush_cb(), buffer operations
┌──────────────────▼──────────────────────────┐
│  HAL Layer (lv_hal_disp.h)                  │
│  - lv_disp_drv_t (Display Driver)           │
│  - flush_cb: Buffer → Hardware transfer     │
│  - wait_cb, rounder_cb, monitor_cb          │
└──────────────────┬──────────────────────────┘
                   │ Platform-specific drivers
┌──────────────────▼──────────────────────────┐
│     Hardware (Display Controller)           │
│   SPI/RGB/MIPI DSI/HDMI/Framebuffer         │
└─────────────────────────────────────────────┘
```

### 2. Draw Context Yapısı

`lv_draw_ctx_t` yapısı, tüm draw subsystem'inin merkezi noktasıdır:

```c
typedef struct _lv_draw_ctx_t {
    // Buffer yönetimi
    void * buf;                      // Çizim buffer'ı
    lv_area_t * buf_area;           // Buffer'ın koordinatları
    const lv_area_t * clip_area;    // Geçerli clipping bölgesi
    bool render_with_alpha;          // Alpha channel kullanımı
    lv_color_format_t color_format;  // Hedef renk formatı

    // === FUNCTION POINTERS (Strategy Pattern) ===

    // Temel primitifler
    void (*draw_rect)(struct _lv_draw_ctx_t *, const lv_draw_rect_dsc_t *, const lv_area_t *);
    void (*draw_arc)(struct _lv_draw_ctx_t *, const lv_draw_arc_dsc_t *, ...);
    void (*draw_line)(struct _lv_draw_ctx_t *, const lv_draw_line_dsc_t *, ...);
    void (*draw_polygon)(struct _lv_draw_ctx_t *, const lv_draw_rect_dsc_t *, ...);

    // Görüntü rendering
    lv_res_t (*draw_img)(struct _lv_draw_ctx_t *, const lv_draw_img_dsc_t *, ...);
    void (*draw_img_decoded)(struct _lv_draw_ctx_t *, const lv_draw_img_dsc_t *, ...);
    void (*draw_transform)(struct _lv_draw_ctx_t *, const lv_area_t *, ...);

    // Metin rendering
    void (*draw_letter)(struct _lv_draw_ctx_t *, const lv_draw_label_dsc_t *, ...);

    // Arka plan ve buffer işlemleri
    void (*draw_bg)(struct _lv_draw_ctx_t *, const lv_draw_rect_dsc_t *, ...);
    void (*buffer_copy)(struct _lv_draw_ctx_t *, void *, lv_coord_t, ...);
    void (*buffer_convert)(struct _lv_draw_ctx_t *);

    // Senkronizasyon
    void (*wait_for_finish)(struct _lv_draw_ctx_t *);

    // Layer yönetimi
    struct _lv_draw_layer_ctx_t * (*layer_init)(...);
    void (*layer_adjust)(...);
    void (*layer_blend)(...);
    void (*layer_destroy)(...);
    size_t layer_instance_size;

} lv_draw_ctx_t;
```

**Dosya:** `src/draw/lv_draw.h:59-210`

---

## Tasarım Desenleri

### 1. Strategy Pattern (Dominant Pattern)

**Amaç:** Runtime'da farklı rendering algoritmalarını seçebilme yeteneği.

**Uygulama:**
- `lv_draw_ctx_t` yapısı 15 adet function pointer içerir
- Her backend bu pointer'ları kendi implementasyonlarıyla doldurur
- Caller kod backend'den bağımsızdır

**Örnek - SW Backend Initialization:**

```c
void lv_draw_sw_init_ctx(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
    lv_draw_sw_ctx_t * draw_sw_ctx = (lv_draw_sw_ctx_t *) draw_ctx;

    // Function pointer'ları SW implementasyonlarına yönlendir
    draw_sw_ctx->base_draw.draw_arc = lv_draw_sw_arc;
    draw_sw_ctx->base_draw.draw_rect = lv_draw_sw_rect;
    draw_sw_ctx->base_draw.draw_line = lv_draw_sw_line;
    draw_sw_ctx->base_draw.draw_img_decoded = lv_draw_sw_img_decoded;
    draw_sw_ctx->base_draw.draw_letter = lv_draw_sw_letter;
    draw_sw_ctx->base_draw.draw_polygon = lv_draw_sw_polygon;
    draw_sw_ctx->base_draw.draw_transform = lv_draw_sw_transform;
    draw_sw_ctx->base_draw.wait_for_finish = lv_draw_sw_wait_for_finish;
    draw_sw_ctx->base_draw.buffer_copy = lv_draw_sw_buffer_copy;
    draw_sw_ctx->base_draw.layer_init = lv_draw_sw_layer_create;
    // ...
    draw_sw_ctx->blend = lv_draw_sw_blend_basic;
}
```

**Dosya:** `src/draw/sw/lv_draw_sw.c:42-66`

**Avantajlar:**
- Uygulama kodu backend değişikliğinden etkilenmez
- Yeni backend eklemek mevcut kodu bozmaz
- Runtime'da backend değiştirilebilir
- Hibrit rendering mümkündür (bazı işlemler GPU, bazıları CPU)

### 2. Factory Pattern (Image Decoder System)

**Amaç:** Farklı görüntü formatlarını (PNG, JPG, BMP, custom) decode edebilme.

**Uygulama:**
- `lv_img_decoder_t` linked list yapısı
- Decoder'lar runtime'da register edilir
- Her decoder `info_cb` ve `open_cb` sağlar

```c
typedef struct _lv_img_decoder_t {
    lv_img_decoder_info_f_t info_cb;     // Format kontrolü
    lv_img_decoder_open_f_t open_cb;     // Decode işlemi
    lv_img_decoder_close_f_t close_cb;   // Cleanup
    // ...
    struct _lv_img_decoder_t * next;     // Linked list
} lv_img_decoder_t;
```

**Decoder Seçim Akışı:**

```
lv_img_decode()
    ↓
for each decoder in list
    ↓
    decoder->info_cb(src) → Destekliyor mu?
    ↓
    YES → decoder->open_cb(src, &dsc)
    ↓
    Return decoded image
```

**Dosya:** `src/draw/lv_img_decoder.h`

### 3. State Pattern (Mask State Machine)

Maskeleme işlemleri bir state machine olarak implemente edilmiştir:

```c
enum {
    LV_DRAW_MASK_RES_TRANSP,        // Tamamen şeffaf
    LV_DRAW_MASK_RES_FULL_COVER,    // Tamamen opak
    LV_DRAW_MASK_RES_CHANGED,       // Partial masking
    LV_DRAW_MASK_RES_UNKNOWN        // Henüz hesaplanmadı
};
```

**State Transitions:**

```
UNKNOWN → mask_cb() → { TRANSP | FULL_COVER | CHANGED }
                            ↓           ↓           ↓
                        Skip draw   Direct    Apply mask
                                    blit      & blend
```

**Dosya:** `src/draw/lv_draw_mask.h:36-41`

### 4. Observer/Callback Pattern (HAL Integration)

Display driver, event-driven callback'ler kullanır:

```c
typedef struct _lv_disp_drv_t {
    // Zorunlu callback
    void (*flush_cb)(struct _lv_disp_drv_t * disp_drv,
                     const lv_area_t * area,
                     lv_color_t * color_p);

    // İsteğe bağlı callback'ler
    void (*rounder_cb)(struct _lv_disp_drv_t *, lv_area_t *);
    void (*monitor_cb)(struct _lv_disp_drv_t *, uint32_t time, uint32_t px);
    void (*wait_cb)(struct _lv_disp_drv_t *);
    void (*clean_dcache_cb)(struct _lv_disp_drv_t *);
    void (*render_start_cb)(struct _lv_disp_drv_t *);
} lv_disp_drv_t;
```

**Dosya:** `src/hal/lv_hal_disp.h:80-151`

### 5. Template Method Pattern (Layer Allocation)

Layer oluşturma işlemi template method pattern kullanır:

```c
// Genel algoritma (template)
lv_draw_layer_ctx_t * lv_draw_layer_create(lv_draw_ctx_t * draw_ctx,
                                            const lv_area_t * layer_area,
                                            lv_draw_layer_flags_t flags)
{
    // 1. Ortak ön işlemler
    save_original_buffer();
    calculate_layer_size();

    // 2. Backend-specific initialization (polymorphic)
    layer_ctx = draw_ctx->layer_init(draw_ctx, layer_ctx, flags);

    // 3. Ortak son işlemler
    if(flags & LV_DRAW_LAYER_FLAG_CAN_SUBDIVIDE) {
        draw_ctx->layer_adjust(draw_ctx, layer_ctx, flags);
    }

    return layer_ctx;
}
```

**Dosya:** `src/draw/lv_draw_layer.h:40-59`

### 6. Composition Over Inheritance

Backend'ler inheritance yerine composition kullanır:

```c
typedef struct {
    lv_draw_ctx_t base_draw;    // Base context'i içerir
    void (*blend)(...);         // Ek backend-specific fonksiyonlar
} lv_draw_sw_ctx_t;
```

Avantajları:
- Runtime'da fonksiyon değiştirilebilir
- Multiple backend özelliklerini birleştirmek kolay
- C dilinde polymorphism sağlar

---

## Çizim Teknikleri ve Yöntemleri

### 1. Temel Primitifler

#### 1.1 Dikdörtgen Çizimi (Rectangle)

**Özellikler:**
- Border (kenarlık) desteği
- Köşe yuvarlatma (radius)
- Gradyan dolgu (linear, radial)
- Gölge efektleri
- Outline (dış çerçeve)

**Algoritma Akışı:**

```
lv_draw_rect()
    ↓
1. Clipping kontrolü → Görünür mü?
    ↓ (hayır)
    └→ Return (erken çıkış)
    ↓ (evet)
2. Shadow çiz (varsa)
    ↓
3. Background dolgu
    ├→ Solid color
    ├→ Gradient (LRU cache'den al)
    └→ Image pattern
    ↓
4. Border çiz (varsa)
    ├→ Düz kenarlık
    └→ Yuvarlatılmış köşeler (mask kullanarak)
    ↓
5. Outline çiz (varsa)
```

**Dosya:** `src/draw/sw/lv_draw_sw_rect.c`

**Gradient Optimizasyonu:**

Gradyan hesaplamaları CPU-intensive olduğu için LRU (Least Recently Used) cache kullanılır:

```c
typedef struct {
    lv_grad_t * cache[LV_GRAD_CACHE_DEF_SIZE];  // Default: 8 slot
    uint32_t cache_cnt;
    uint32_t cache_size;
} lv_grad_cache_t;
```

**Dosya:** `src/draw/sw/lv_draw_sw_gradient.c`

#### 1.2 Çizgi Çizimi (Line)

**Algoritma:** Bresenham's Line Algorithm

```c
LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_line(lv_draw_ctx_t * draw_ctx,
                                           const lv_draw_line_dsc_t * dsc,
                                           const lv_point_t * point1,
                                           const lv_point_t * point2)
{
    // Bresenham algoritması
    int32_t dx = abs(x2 - x1);
    int32_t dy = abs(y2 - y1);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx - dy;

    while(true) {
        set_pixel(x1, y1);
        if(x1 == x2 && y1 == y2) break;

        int32_t e2 = 2 * err;
        if(e2 > -dy) { err -= dy; x1 += sx; }
        if(e2 < dx)  { err += dx; y1 += sy; }
    }
}
```

**Performans:** `LV_ATTRIBUTE_FAST_MEM` ile TCRAM'e yerleştirilir.

**Dosya:** `src/draw/sw/lv_draw_sw_line.c`

#### 1.3 Yay/Daire Çizimi (Arc)

**Algoritma:** Midpoint Circle Algorithm (Bresenham benzeri)

```c
void draw_arc_segment(uint16_t radius, uint16_t start_angle, uint16_t end_angle)
{
    int32_t x = radius;
    int32_t y = 0;
    int32_t err = 0;

    while(x >= y) {
        // Simetrik noktaları işaretle
        plot_arc_points(center_x, center_y, x, y, start_angle, end_angle);

        y++;
        err += 1 + 2*y;
        if(2*(err-x) + 1 > 0) {
            x--;
            err += 1 - 2*x;
        }
    }
}
```

**Dosya:** `src/draw/sw/lv_draw_sw_arc.c`

### 2. Görüntü Rendering

#### 2.1 Image Decoding Pipeline

```
Image Source (File/Array/Symbol)
    ↓
lv_img_decoder_open()
    ↓
Format Detection (PNG/JPG/BMP/RAW)
    ↓
Decoder Factory → Appropriate Decoder
    ↓
Decode to Raw Buffer
    ↓
Cache Entry Created (if cacheable)
    ↓
draw_img_decoded()
    ↓
Transformation (if needed)
    ├→ Zoom
    ├→ Rotate
    └→ Pivot
    ↓
Blending & Rendering
```

**Dosya:** `src/draw/lv_img_decoder.c`

#### 2.2 Image Transformation

**Bilinear Interpolation** kullanılır:

```c
void lv_draw_sw_transform(lv_draw_ctx_t * draw_ctx,
                          const lv_area_t * dest_area,
                          const void * src_buf,
                          lv_coord_t src_w, lv_coord_t src_h,
                          const lv_draw_img_dsc_t * draw_dsc,
                          lv_img_cf_t cf,
                          lv_color_t * cbuf, lv_opa_t * abuf)
{
    // Transformation matrix hesaplama
    int32_t angle = draw_dsc->angle;
    int32_t zoom = draw_dsc->zoom;

    // Her hedef pixel için
    for(y = 0; y < dest_h; y++) {
        for(x = 0; x < dest_w; x++) {
            // Inverse transformation
            src_x = (x * cos - y * sin) / zoom;
            src_y = (x * sin + y * cos) / zoom;

            // Bilinear interpolation (4 komşu pixel)
            color = interpolate_4_pixels(src_buf, src_x, src_y);
            cbuf[y * dest_w + x] = color;
        }
    }
}
```

**Dosya:** `src/draw/sw/lv_draw_sw_transform.c`

### 3. Metin Rendering

**Font Rendering Pipeline:**

```
Character Code (UTF-8)
    ↓
Font Decoder (TTF/Custom/Bitmap)
    ↓
Glyph Bitmap Extraction
    ↓
Kerning Adjustment
    ↓
Anti-aliasing (1/2/4/8 bit)
    ↓
Color Application
    ↓
Blending
```

**Dosya:** `src/draw/sw/lv_draw_sw_letter.c`

### 4. Blending Modes

LVGL birden fazla blending mode destekler:

```c
typedef enum {
    LV_BLEND_MODE_NORMAL,        // Alpha blending
    LV_BLEND_MODE_ADDITIVE,      // Add colors
    LV_BLEND_MODE_SUBTRACTIVE,   // Subtract colors
    LV_BLEND_MODE_MULTIPLY,      // Multiply colors
} lv_blend_mode_t;
```

**Alpha Blending Formülü:**

```c
result = (src * alpha) + (dst * (255 - alpha)) / 255
```

**Optimizasyon:**
- SIMD kullanımı (ARM NEON, SSE vb.)
- Look-up table'lar (LUT)
- Batch processing

**Dosya:** `src/draw/sw/lv_draw_sw_blend.c`

### 5. Maskeleme (Masking)

Maskeleme 6 farklı tipte olabilir:

```c
enum {
    LV_DRAW_MASK_TYPE_LINE,      // Doğrusal mask
    LV_DRAW_MASK_TYPE_ANGLE,     // Açısal mask (pie chart için)
    LV_DRAW_MASK_TYPE_RADIUS,    // Yuvarlatılmış köşeler
    LV_DRAW_MASK_TYPE_FADE,      // Fade efekti
    LV_DRAW_MASK_TYPE_MAP,       // Bitmap mask
    LV_DRAW_MASK_TYPE_POLYGON,   // Polygon mask
};
```

**Mask Application Pipeline:**

```
1. Mask stack oluştur (max 16 mask)
    ↓
2. Her scan line için
    ├→ Her mask'i uygula
    ├→ Mask buffer hesapla (opacity array)
    ↓
3. Blend işlemi sırasında mask buffer'ı kullan
```

**Dosya:** `src/draw/lv_draw_mask.c`

---

## Gerçek Zamanlı Çizim Stratejileri

### 1. Dirty Region Tracking

LVGL, ekranın sadece değişen bölgelerini yeniden çizer:

```c
typedef struct _lv_disp_t {
    lv_area_t inv_areas[LV_INV_BUF_SIZE];  // Default: 32 alan
    uint8_t inv_area_joined[LV_INV_BUF_SIZE];
    uint16_t inv_p;                        // Area counter
} lv_disp_t;
```

**Algoritma:**

```
1. Widget değişikliği tespit edilir
    ↓
2. lv_obj_invalidate(obj) çağrılır
    ↓
3. Widget'ın alanı inv_areas[]'e eklenir
    ↓
4. Birleşebilecek alanlar birleştirilir (join)
    ↓
5. Render cycle'da sadece invalid alanlar çizilir
```

**Join Algoritması:**

```c
bool areas_joinable(lv_area_t * a, lv_area_t * b)
{
    uint32_t join_area = area_get_size(join(a, b));
    uint32_t a_area = area_get_size(a);
    uint32_t b_area = area_get_size(b);

    // %10'dan fazla boşluk olmasın
    if(join_area < (a_area + b_area) * 1.1) return true;
    return false;
}
```

**Dosya:** `src/hal/lv_hal_disp.h:186-189`, `src/core/lv_refr.c`

### 2. Buffering Stratejileri

#### 2.1 Single Buffering

```
┌─────────────┐
│  Buffer 1   │ ← Render & Display (aynı buffer)
└─────────────┘
```

**Avantaj:** Minimum RAM kullanımı
**Dezavantaj:** Tearing artifact'leri

#### 2.2 Double Buffering

```
Cycle 1:
┌─────────────┐     ┌─────────────┐
│  Buffer 1   │ ←   │  Buffer 2   │
│  (Display)  │     │  (Render)   │
└─────────────┘     └─────────────┘

Cycle 2:
┌─────────────┐     ┌─────────────┐
│  Buffer 1   │     │  Buffer 2   │ ←
│  (Render)   │     │  (Display)  │
└─────────────┘     └─────────────┘
```

**Swap mekanizması:**

```c
typedef struct _lv_disp_draw_buf_t {
    void * buf1;
    void * buf2;
    void * buf_act;                    // Active buffer
    volatile int flushing;             // Atomic flag
    volatile int flushing_last;
} lv_disp_draw_buf_t;
```

**Dosya:** `src/hal/lv_hal_disp.h:53-66`

#### 2.3 Partial Buffering

Ekran boyutundan küçük buffer kullanımı:

```
Full Screen: 800x480 = 384000 pixels
Buffer: 800x10 = 8000 pixels (1/48 of screen)

Rendering:
[====================] Line 0-9
                      [====================] Line 10-19
                                            [====================] Line 20-29
                                                                  ...
```

**Avantaj:** Çok düşük RAM kullanımı (örn: 800x10x2 = 16KB)
**Dezavantaj:** Daha fazla flush cycle

### 3. VSync ve Frame Rate Yönetimi

**Default Refresh Rate:**

```c
#define LV_DEF_REFR_PERIOD  33   // 33ms = ~30 FPS
```

**Refresh Cycle:**

```
Timer Tick (33ms)
    ↓
lv_disp_refr_timer()
    ↓
Check dirty areas
    ↓ (varsa)
lv_refr_area() → For each invalid area
    ↓
draw_ctx->draw_*() → Rendering
    ↓
disp_drv->flush_cb() → DMA/hardware transfer
    ↓
Wait for flush complete
    ↓
Swap buffers (if double buffer)
```

**Dosya:** `src/hal/lv_hal_disp.h:39`, `src/core/lv_refr.c`

### 4. Layer Subdivision (Memory Optimization)

Büyük layer'lar alt parçalara bölünebilir:

```c
typedef enum {
    LV_DRAW_LAYER_FLAG_NONE,
    LV_DRAW_LAYER_FLAG_HAS_ALPHA,
    LV_DRAW_LAYER_FLAG_CAN_SUBDIVIDE,  // Bu layer bölünebilir
} lv_draw_layer_flags_t;
```

**Subdivision Stratejisi:**

```
Requested Layer: 800x600 (1.44 MB @ 32bpp)
Available RAM: 256 KB

Algorithm:
1. Calculate max height: 256KB / (800*4) = 80 lines
2. Create layer: 800x80
3. Render partial layer
4. Blend to screen
5. Move to next 80 lines
6. Repeat until full layer rendered
```

**Dosya:** `src/draw/lv_draw_layer.h:30-34`

### 5. Asenkron Rendering (GPU)

GPU backend'ler asenkron işlem yapabilir:

```c
void lv_draw_gpu_rect(lv_draw_ctx_t * draw_ctx, ...)
{
    // 1. Setup GPU registers
    configure_gpu_rect(dsc, coords);

    // 2. Start GPU operation (non-blocking)
    gpu_start();

    // 3. Return immediately (async)
}

void lv_draw_gpu_wait_for_finish(lv_draw_ctx_t * draw_ctx)
{
    // 4. Wait until GPU completes
    while(gpu_is_busy());
}
```

**Flush sırasında bekleme:**

```c
void flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * px_map)
{
    draw_ctx->wait_for_finish(draw_ctx);  // GPU'yu bekle
    dma_transfer(px_map, lcd_buffer);     // DMA başlat
}
```

---

## Donanım İletişimi

### 1. HAL (Hardware Abstraction Layer) Mimarisi

```
┌──────────────────────────────────────────────┐
│         Application / Widgets                 │
└────────────────┬─────────────────────────────┘
                 │
┌────────────────▼─────────────────────────────┐
│           Draw Subsystem                      │
│    (Hardware agnostic rendering)             │
└────────────────┬─────────────────────────────┘
                 │
┌────────────────▼─────────────────────────────┐
│       lv_disp_drv_t (HAL Interface)          │
│  ┌──────────────────────────────────────┐   │
│  │ flush_cb() ← MANDATORY                │   │
│  │ rounder_cb()                          │   │
│  │ monitor_cb()                          │   │
│  │ wait_cb()                             │   │
│  │ clean_dcache_cb()                     │   │
│  └──────────────────────────────────────┘   │
└────────────────┬─────────────────────────────┘
                 │
┌────────────────▼─────────────────────────────┐
│      Platform-Specific Driver                 │
│  (SPI, RGB, MIPI DSI, Framebuffer)           │
└────────────────┬─────────────────────────────┘
                 │
┌────────────────▼─────────────────────────────┐
│          Display Hardware                     │
│  (LCD Controller, HDMI, VGA, etc.)           │
└──────────────────────────────────────────────┘
```

### 2. Display Driver Yapısı

```c
typedef struct _lv_disp_drv_t {
    // Çözünürlük
    lv_coord_t hor_res;
    lv_coord_t ver_res;

    // Buffer yönetimi
    lv_disp_draw_buf_t * draw_buf;

    // Rendering ayarları
    uint32_t direct_mode : 1;      // Screen-sized buffer
    uint32_t full_refresh : 1;     // Her frame'de full redraw
    uint32_t sw_rotate : 1;        // Software rotation
    uint32_t antialiasing : 1;     // AA enable
    uint32_t rotated : 2;          // 90/180/270 derece
    uint32_t screen_transp : 1;    // Transparent background
    uint32_t dpi : 10;             // Dots per inch

    lv_color_format_t color_format;  // RGB565, RGB888, ARGB8888, etc.

    // === CALLBACK FUNCTIONS ===

    // [ZORUNLU] Buffer'ı donanıma gönder
    void (*flush_cb)(struct _lv_disp_drv_t * disp_drv,
                     const lv_area_t * area,
                     lv_color_t * color_p);

    // [İSTEĞE BAĞLI] Invalid alanı yuvarla (monochrome LCD için)
    void (*rounder_cb)(struct _lv_disp_drv_t * disp_drv,
                       lv_area_t * area);

    // [İSTEĞE BAĞLI] Buffer temizleme
    void (*clear_cb)(struct _lv_disp_drv_t * disp_drv,
                     uint8_t * buf, uint32_t size);

    // [İSTEĞE BAĞLI] Performans monitörü
    void (*monitor_cb)(struct _lv_disp_drv_t * disp_drv,
                       uint32_t time_ms, uint32_t px_count);

    // [İSTEĞE BAĞLI] CPU bekleme/yield
    void (*wait_cb)(struct _lv_disp_drv_t * disp_drv);

    // [İSTEĞE BAĞLI] CPU cache temizleme
    void (*clean_dcache_cb)(struct _lv_disp_drv_t * disp_drv);

    // [İSTEĞE BAĞLI] Render başlangıç event
    void (*render_start_cb)(struct _lv_disp_drv_t * disp_drv);

    // Draw context
    lv_draw_ctx_t * draw_ctx;
    void (*draw_ctx_init)(struct _lv_disp_drv_t *, lv_draw_ctx_t *);
    void (*draw_ctx_deinit)(struct _lv_disp_drv_t *, lv_draw_ctx_t *);
    size_t draw_ctx_size;

} lv_disp_drv_t;
```

**Dosya:** `src/hal/lv_hal_disp.h:80-151`

### 3. Flush Callback İmplementasyonu

#### 3.1 SPI Display Örneği

```c
void spi_display_flush_cb(lv_disp_drv_t * disp_drv,
                          const lv_area_t * area,
                          lv_color_t * color_p)
{
    // 1. Set window (column/row address)
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    // 2. Calculate data size
    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    // 3. Transfer data via SPI
    spi_write_buffer((uint8_t *)color_p, size * sizeof(lv_color_t));

    // 4. Notify LVGL that flush is complete
    lv_disp_flush_ready(disp_drv);
}
```

#### 3.2 DMA Accelerated Flush

```c
void dma_display_flush_cb(lv_disp_drv_t * disp_drv,
                          const lv_area_t * area,
                          lv_color_t * color_p)
{
    uint32_t width = lv_area_get_width(area);
    uint32_t height = lv_area_get_height(area);

    // 1. Configure DMA
    dma_config.src_addr = (uint32_t)color_p;
    dma_config.dst_addr = (uint32_t)framebuffer + offset;
    dma_config.length = width * height * sizeof(lv_color_t);

    // 2. Start DMA (non-blocking)
    dma_start_transfer(&dma_config);

    // 3. DMA interrupt'ta flush_ready çağrılır
    // lv_disp_flush_ready() DMA ISR'da çağrılacak
}

void DMA_IRQHandler(void)
{
    if(dma_transfer_complete()) {
        lv_disp_flush_ready(&disp_drv);  // LVGL'e bildir
    }
}
```

#### 3.3 Framebuffer (Direct Mode)

```c
void framebuffer_flush_cb(lv_disp_drv_t * disp_drv,
                          const lv_area_t * area,
                          lv_color_t * color_p)
{
    // Direct mode: color_p zaten framebuffer içindedir
    // Hiçbir kopyalama gerekmez

    // Sadece cache temizle (eğer gerekiyorsa)
    if(disp_drv->clean_dcache_cb) {
        disp_drv->clean_dcache_cb(disp_drv);
    }

    lv_disp_flush_ready(disp_drv);
}
```

### 4. DMA2D Hardware Acceleration Örneği

STM32 DMA2D implementasyonu:

```c
void lv_gpu_stm32_dma2d_fill(lv_color_t * dest_buf,
                             lv_coord_t dest_stride,
                             const lv_area_t * fill_area,
                             lv_color_t color)
{
    // 1. Wait for previous DMA2D operation
    while(HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 0) != HAL_OK);

    // 2. Configure DMA2D for fill operation
    hlcd_dma2d.Init.Mode = DMA2D_R2M;  // Register to Memory
    hlcd_dma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
    hlcd_dma2d.Init.OutputOffset = dest_stride - width;

    // 3. Set fill color
    uint32_t dma2d_color = lv_color_to32(color);

    // 4. Start DMA2D transfer (non-blocking)
    HAL_DMA2D_Start(&hlcd_dma2d,
                    dma2d_color,           // Input color
                    (uint32_t)dest_buf,    // Output buffer
                    width, height);
}
```

**Dosya:** `src/draw/stm32_dma2d/lv_gpu_stm32_dma2d.c`

### 5. GPU Integration Pattern

```
┌─────────────────────────────────────────────┐
│     lv_draw_rect(draw_ctx, dsc, area)       │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
         ┌─────────────────────┐
         │  Size < threshold?  │
         └─────────┬───────────┘
                   │
        ┌──────────┴──────────┐
        │ YES               NO│
        ▼                     ▼
┌───────────────┐     ┌──────────────────┐
│  CPU Render   │     │   GPU Render     │
│  (SW backend) │     │ (Hardware accel) │
└───────────────┘     └──────────────────┘
        │                     │
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │  Blending/Output    │
        └─────────────────────┘
```

**Threshold Example:**

```c
// PXP GPU sadece büyük işlemler için kullanılır
#define MIN_GPU_SIZE  5000  // pixels

void lv_draw_pxp_rect(lv_draw_ctx_t * draw_ctx, ...)
{
    uint32_t area_size = lv_area_get_size(coords);

    if(area_size < MIN_GPU_SIZE) {
        // Küçük alanlar için CPU fallback
        lv_draw_sw_rect(draw_ctx, dsc, coords);
        return;
    }

    // PXP GPU kullan
    pxp_fill_rect(...);
}
```

**Dosya:** `src/draw/nxp/pxp/lv_gpu_nxp_pxp.c`

---

## Backend Mimarisi

### 1. Software Backend (SW)

**Özellikler:**
- Saf CPU rendering
- Platform bağımsız
- Fallback backend (her zaman kullanılabilir)
- SIMD optimizasyonları (ARM NEON, SSE)

**Dizin:** `src/draw/sw/`

**Ana Dosyalar:**
- `lv_draw_sw.c` - Backend initialization
- `lv_draw_sw_blend.c` - Blending engine (3000+ satır)
- `lv_draw_sw_rect.c` - Rectangle rendering
- `lv_draw_sw_img.c` - Image rendering
- `lv_draw_sw_transform.c` - Rotation/zoom
- `lv_draw_sw_gradient.c` - Gradient generation

**Performans Kritik Fonksiyonlar:**

```c
LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_blend_basic(...)
LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_img_decoded(...)
LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_line(...)
```

`LV_ATTRIBUTE_FAST_MEM` → TCRAM/ITCM'e yerleştirme (ARM Cortex-M7)

### 2. NXP PXP Backend

**PXP (Pixel Pipeline):** NXP i.MX RT serisi MCU'larda bulunan 2D GPU.

**Özellikler:**
- Hardware color format conversion
- Rotation (90/180/270)
- Scaling
- Alpha blending
- Color keying

**Backend Composition:**

```c
void lv_draw_pxp_ctx_init(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
    // 1. SW backend ile başla
    lv_draw_sw_init_ctx(drv, draw_ctx);

    // 2. PXP-accelerated fonksiyonları override et
    draw_ctx->draw_img_decoded = lv_draw_pxp_img_decoded;
    draw_ctx->blend = lv_draw_pxp_blend;

    // 3. SW fallback için orijinal pointer'ları sakla
    lv_draw_pxp_ctx_t * pxp_ctx = (lv_draw_pxp_ctx_t *)draw_ctx;
    pxp_ctx->base_draw.draw_rect = lv_draw_sw_rect;  // Fallback
}
```

**Dizin:** `src/draw/nxp/pxp/`

### 3. NXP VGLite Backend

**VGLite:** Vector graphics GPU (NXP i.MX RT1170 vb.)

**Özellikler:**
- Vector path rendering
- High-quality anti-aliasing
- Gradient fills
- Image filtering

**Dizin:** `src/draw/nxp/vglite/`

### 4. STM32 DMA2D Backend

**DMA2D:** STM32F4/F7/H7 serisindeki 2D grafik hızlandırıcı.

**Operasyonlar:**
- Memory-to-memory transfer
- Memory-to-memory with pixel format conversion
- Memory-to-memory with blending
- Register-to-memory (fill)

**Örnek:**

```c
void lv_draw_stm32_dma2d_blend(lv_draw_ctx_t * draw_ctx,
                               const lv_draw_sw_blend_dsc_t * dsc)
{
    if(dsc->blend_mode == LV_BLEND_MODE_NORMAL &&
       dsc->opa >= LV_OPA_MAX) {
        // DMA2D ile hızlı kopyalama
        dma2d_copy(dsc->src_buf, dsc->dst_buf, ...);
    } else {
        // Karmaşık blending için CPU fallback
        lv_draw_sw_blend(draw_ctx, dsc);
    }
}
```

**Dizin:** `src/draw/stm32_dma2d/`

### 5. ARM-2D Backend

**ARM-2D:** ARM tarafından sağlanan CPU-optimized 2D rendering library.

**Özellikler:**
- Helium (MVE) optimization (Cortex-M55/M85)
- NEON optimization (Cortex-A)
- Async tile processing

**Dizin:** `src/draw/arm2d/`

### 6. GD32 IPA Backend

**IPA (Image Pixel Accelerator):** GigaDevice MCU'lardaki grafik hızlandırıcı.

**Dizin:** `src/draw/gd32_ipa/`

### 7. SDL Backend

**SDL2:** Desktop platformlarda GPU acceleration.

**Özellikler:**
- OpenGL/Vulkan/Direct3D backend kullanımı
- Texture-based rendering
- Hardware-accelerated blending

**Avantajlar:**
- Platform bağımsız (Windows/Linux/macOS)
- GPU acceleration
- Debugging ve development için ideal

**Dizin:** `src/draw/sdl/`

**Texture Cache:**

```c
typedef struct {
    SDL_Texture * texture;
    void * userdata;
    lv_coord_t x_offset;
    lv_coord_t y_offset;
    /* ... */
} lv_draw_sdl_cache_key_magic_t;
```

Decoder'dan gelen görüntüler SDL texture'a çevrilerek cache'lenir.

**Dosya:** `src/draw/sdl/lv_draw_sdl_texture_cache.c`

### 8. Backend Selection Decision Tree

```
Application starts
    ↓
lv_init()
    ↓
lv_disp_drv_init(&disp_drv)
    ↓
User sets backend:
    ├→ disp_drv.draw_ctx_init = lv_draw_sw_init_ctx
    ├→ disp_drv.draw_ctx_init = lv_draw_pxp_init_ctx
    ├→ disp_drv.draw_ctx_init = lv_draw_dma2d_init_ctx
    ├→ disp_drv.draw_ctx_init = lv_draw_sdl_init_ctx
    └→ ... (other backends)
    ↓
lv_disp_drv_register(&disp_drv)
    ↓
draw_ctx_init() called
    ↓
Function pointers configured
    ↓
draw_ctx ready for use
```

---

## Performans Optimizasyonları

### 1. Fast Memory Placement

**16 kritik fonksiyon TCRAM'e yerleştirilir:**

```c
#define LV_ATTRIBUTE_FAST_MEM __attribute__((section(".ram_code")))

LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_blend_basic(...)
LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_blend_color(...)
LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_img_decoded(...)
LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_line(...)
// ... 12 more
```

**Hız kazancı:** ~20-30% (TCRAM vs Flash)

### 2. Clipping Optimization

```c
bool is_visible = _lv_area_intersect(&clipped_area, coords, draw_ctx->clip_area);
if(!is_visible) return;  // Erken çıkış
```

**Kazanç:** Görünmeyen widget'lar hiç işlenmez.

### 3. Mask Culling

```c
lv_draw_mask_res_t mask_res = lv_draw_mask_apply(mask_buf, ...);

if(mask_res == LV_DRAW_MASK_RES_TRANSP) {
    continue;  // Skip completely transparent pixels
}
else if(mask_res == LV_DRAW_MASK_RES_FULL_COVER) {
    // Fast path: no masking needed
    memcpy(dest, src, len);
}
else {
    // Slow path: apply mask
    blend_with_mask(dest, src, mask_buf, len);
}
```

### 4. Gradient LRU Cache

```c
lv_grad_t * grad = lv_gradient_get_from_cache(dsc);
if(grad == NULL) {
    grad = lv_gradient_calculate(dsc);
    lv_gradient_cache_add(grad);
}
```

**Cache size:** Default 8 gradyan
**Kazanç:** ~10x hız (önceden hesaplanmış gradyan)

### 5. SIMD Optimization Hooks

ARM NEON örneği:

```c
#if LV_USE_DRAW_SW_ASM == LV_DRAW_SW_ASM_NEON
    lv_color32_t * dest32 = (lv_color32_t *)dest;
    const lv_color32_t * src32 = (const lv_color32_t *)src;

    // 4 pixel'i aynı anda işle
    uint32x4_t src_vec = vld1q_u32((uint32_t *)src32);
    uint32x4_t dest_vec = vld1q_u32((uint32_t *)dest32);
    uint32x4_t result = vaddq_u32(src_vec, dest_vec);
    vst1q_u32((uint32_t *)dest32, result);
#else
    // Scalar fallback
    for(i = 0; i < len; i++) {
        dest[i] = src[i] + dest[i];
    }
#endif
```

### 6. GPU Threshold Logic

```c
#define GPU_SIZE_LIMIT  5000  // pixels

if(operation_size < GPU_SIZE_LIMIT) {
    cpu_render();  // Küçük işlemler için GPU overhead'i yüksek
} else {
    gpu_render();  // Büyük işlemler GPU'da hızlı
}
```

### 7. Layer Memory Subdivision

```c
uint32_t required_size = width * height * bytes_per_pixel;
uint32_t available_size = lv_mem_get_free_size();

if(required_size > available_size) {
    // Subdivide layer
    uint32_t max_height = available_size / (width * bytes_per_pixel);

    for(y = 0; y < height; y += max_height) {
        render_layer_part(y, min(max_height, height - y));
        blend_to_screen();
    }
}
```

**Kazanç:** Sınırsız boyutta layer rendering (örn: 256KB RAM ile 2MB layer)

### 8. Dirty Area Optimization

```c
// 32'den fazla dirty area varsa full refresh yap
if(disp->inv_p >= LV_INV_BUF_SIZE) {
    disp->inv_p = 1;
    disp->inv_areas[0] = full_screen_area;
}
```

**Sebep:** Çok fazla küçük alan, full redraw'dan yavaş olabilir.

---

## Veri Akışı

### End-to-End Rendering Pipeline

```
1. USER ACTION / TIMER EVENT
         ↓
2. WIDGET STATE CHANGE
   lv_obj_set_*() / lv_obj_invalidate()
         ↓
3. INVALIDATION
   inv_areas[] updated
         ↓
4. REFRESH TIMER (33ms)
   lv_disp_refr_timer()
         ↓
5. AREA PROCESSING
   for each dirty area:
         ↓
6. WIDGET TREE TRAVERSAL
   for each visible widget in area:
         ↓
7. DRAW DESCRIPTOR PREPARATION
   lv_draw_rect_dsc_t / lv_draw_img_dsc_t / etc.
         ↓
8. DRAW CONTEXT CALL
   draw_ctx->draw_rect(draw_ctx, dsc, coords)
         ↓
9. BACKEND RENDERING
   ├→ SW: CPU rendering
   ├→ GPU: Hardware accelerated
   └→ Hybrid: Mix of both
         ↓
10. MASKING (if applicable)
    Apply masks to rendered pixels
         ↓
11. BLENDING
    Alpha blend with existing buffer
         ↓
12. BUFFER COMPLETE
    Area fully rendered in draw_buf
         ↓
13. FLUSH CALLBACK
    disp_drv->flush_cb(area, color_p)
         ↓
14. HARDWARE TRANSFER
    ├→ SPI transfer
    ├→ DMA transfer
    ├→ Framebuffer copy
    └→ GPU blit
         ↓
15. FLUSH COMPLETE
    lv_disp_flush_ready()
         ↓
16. BUFFER SWAP (if double buffering)
    buf_act = (buf_act == buf1) ? buf2 : buf1
         ↓
17. NEXT AREA or FRAME COMPLETE
```

### Örnek: Button Press → Screen Update

```
User presses button
    ↓
lv_indev_read() detects touch
    ↓
lv_btn_event_cb(LV_EVENT_PRESSED)
    ↓
lv_obj_add_state(btn, LV_STATE_PRESSED)
    ↓
lv_obj_invalidate(btn)
    ↓
inv_areas[0] = btn->coords
    ↓
[Wait for next timer tick - 33ms]
    ↓
lv_refr_area(&inv_areas[0])
    ↓
lv_obj_draw(btn)
    ↓
lv_draw_rect(draw_ctx, &btn_dsc, &btn_coords)
    ↓
draw_ctx->draw_rect() → lv_draw_sw_rect()
    ↓
  1. Draw shadow
  2. Draw background (with gradient)
  3. Draw border
  4. Draw outline
    ↓
lv_draw_label(draw_ctx, &label_dsc, "OK")
    ↓
  For each character:
    draw_ctx->draw_letter()
    ↓
lv_draw_sw_blend() → Final blending
    ↓
flush_cb(&btn_area, draw_buf)
    ↓
SPI/DMA transfer to LCD
    ↓
lv_disp_flush_ready()
    ↓
User sees button in pressed state
```

---

## Sonuç ve Öneriler

### Mimari Güçlü Yönler

1. **Yüksek Seviye Soyutlama**: Widget kodları backend'den tamamen bağımsız
2. **Esneklik**: 7 farklı backend desteği, kolay yeni backend ekleme
3. **Performans**: Multi-level optimizasyon (SIMD, GPU, cache, clipping)
4. **Bellek Verimliliği**: Layer subdivision, partial buffering
5. **Gerçek Zamanlı**: Dirty region tracking, vsync, async rendering

### En İyi Pratikler

1. **Backend Seçimi**:
   - Embedded sistem: SW (CPU) veya platform-specific GPU (DMA2D, PXP)
   - Desktop development: SDL
   - High-end MCU: Hybrid (GPU + CPU fallback)

2. **Buffer Stratejisi**:
   - RAM < 64KB: Partial buffer (1/10 - 1/20 screen)
   - RAM 64-256KB: Partial buffer (1/4 - 1/2 screen)
   - RAM > 256KB: Double buffer (full screen)

3. **Performans İyileştirme**:
   - Anti-aliasing'i sadece gerektiğinde kullan
   - Gradient sayısını sınırla (cache size)
   - Karmaşık widget'ları layer'lara ayır
   - Statik içeriği cache'le

4. **Donanım Entegrasyonu**:
   - DMA kullan (blocking SPI yerine)
   - GPU threshold'u ayarla (küçük işlemler CPU)
   - Cache coherency'ye dikkat et (clean_dcache_cb)

### Gelecek İyileştirmeler

1. Vulkan/Metal backend (modern GPU)
2. Multi-threading support (parallel rendering)
3. Advanced caching (widget-level cache)
4. Hardware cursor support

---

## Referanslar

### Ana Dosyalar

| Dosya | Satır | Açıklama |
|-------|-------|----------|
| `src/draw/lv_draw.h` | 238 | Ana draw API |
| `src/draw/sw/lv_draw_sw.c` | 400+ | SW backend |
| `src/draw/sw/lv_draw_sw_blend.c` | 3000+ | Blending engine |
| `src/hal/lv_hal_disp.h` | 500+ | HAL interface |
| `src/core/lv_refr.c` | 2000+ | Refresh logic |

### Harici Kaynaklar

- [LVGL Documentation](https://docs.lvgl.io/)
- [ARM-2D Library](https://github.com/ARM-software/Arm-2D)
- [STM32 DMA2D Application Note](https://www.st.com/resource/en/application_note/dm00287603.pdf)
- [NXP PXP Reference Manual](https://www.nxp.com/docs/en/reference-manual/IMXRT1060RM.pdf)

---

**Doküman Versiyonu:** 1.0
**Oluşturulma Tarihi:** 2025-11-17
**LVGL Versiyonu:** v8.x/v9.x
**Yazar:** Claude AI (Anthropic)
