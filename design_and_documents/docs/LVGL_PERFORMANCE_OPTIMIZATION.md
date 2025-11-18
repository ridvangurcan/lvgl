# LVGL Performance Optimization Best Practices

**Hedef Cihazlar**: Düşük RAM (64KB-512KB), Düşük CPU (Cortex-M0/M3/M4), Kısıtlı Bant Genişliği

---

## İçindekiler

1. [Buffer Stratejileri](#1-buffer-stratejileri)
2. [Dirty Area Management](#2-dirty-area-management)
3. [Draw Pipeline Optimizasyonları](#3-draw-pipeline-optimizasyonları)
4. [Memory Management](#4-memory-management)
5. [CPU ve Cache Optimizasyonları](#5-cpu-ve-cache-optimizasyonları)
6. [Platform-Specific Optimizasyonlar](#6-platform-specific-optimizasyonlar)
7. [Benchmark ve Profiling](#7-benchmark-ve-profiling)
8. [Gerçek Dünya Örnekleri](#8-gerçek-dünya-örnekleri)

---

## 1. Buffer Stratejileri

### 1.1 Full Framebuffer (Double Buffering)

**Açıklama**: İki tam ekran boyutunda buffer kullanılır. Biri ekrana çizilirken diğeri arka planda hazırlanır.

```
Display (320x240, RGB565) = 153,600 bytes
Buffer 1: 153,600 bytes (working)
Buffer 2: 153,600 bytes (display)
Total: ~307 KB RAM
```

**Avantajlar**:
- ✅ Flickering yok (tam swap)
- ✅ Tearing yok
- ✅ En basit implementasyon

**Dezavantajlar**:
- ❌ Çok yüksek RAM kullanımı
- ❌ Düşük RAM'li cihazlar için uygun değil
- ❌ Swap maliyeti (memcpy veya pointer swap)

**LVGL Config**:
```c
// lv_conf.h
#define LV_COLOR_DEPTH 16
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 240];
static lv_color_t buf2[320 * 240];

lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 320 * 240);
```

**Kullanım Durumu**: RAM'in bol olduğu high-end MCU'lar (STM32F7, iMXRT)

---

### 1.2 Partial Buffering (Single Small Buffer)

**Açıklama**: Ekranın küçük bir parçası kadar buffer kullanılır. LVGL ekranı parçalara böler ve her parçayı sırayla çizer.

```
Display (320x240, RGB565) = 153,600 bytes
Buffer: 320 * 10 * 2 = 6,400 bytes (10 satırlık)
Total: ~6.4 KB RAM (24x daha az!)
```

**Avantajlar**:
- ✅ Çok düşük RAM kullanımı
- ✅ Küçük MCU'lar için ideal
- ✅ LVGL otomatik parçalama yapar

**Dezavantajlar**:
- ❌ Daha fazla flush call (SPI/I2C overhead)
- ❌ Her parça için draw tekrarı
- ❌ Dikkatli dirty area tracking gerekir

**LVGL Config**:
```c
// lv_conf.h
#define LV_COLOR_DEPTH 16
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 10];  // 10 satırlık buffer

lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 320 * 10);
```

**Performans İpuçları**:
- Buffer boyutu **display yüksekliğinin tam bölenini** seçin (örn: 240 için 10, 12, 15, 20, 30)
- Çok küçük buffer (1-2 satır) → fazla flush overhead
- Optimal: **10-30 satır** arası (6-20 KB RAM)

**LVGL Kaynak**: `src/core/lv_refr.c:337-425` (rendering loop with partial buffer)

---

### 1.3 Direct Mode (No Buffer)

**Açıklama**: Buffer kullanmadan doğrudan display'e çizilir.

**Avantajlar**:
- ✅ Sıfır RAM kullanımı
- ✅ En düşük memory footprint

**Dezavantajlar**:
- ❌ Flickering riski
- ❌ Tearing riski
- ❌ Çok yavaş (her pixel için SPI/I2C call)
- ❌ Karmaşık implementasyon

**LVGL Config**:
```c
// lv_conf.h
#define LV_VDB_SIZE 0  // Direct mode
```

**Kullanım Durumu**: **Önerilmez!** Sadece çok basit, statik UI için.

---

### 1.4 Hybrid: Double Partial Buffering

**En iyi performans/RAM dengesi!**

```
Buffer 1: 320 * 10 * 2 = 6,400 bytes
Buffer 2: 320 * 10 * 2 = 6,400 bytes
Total: ~12.8 KB RAM
```

**Nasıl Çalışır**:
1. Buffer 1'e bir parça çizilir
2. Buffer 1 DMA ile display'e flush edilirken...
3. ...CPU buffer 2'ye bir sonraki parçayı çizer (PARALEL!)
4. Swap ve tekrar

**Avantajlar**:
- ✅ Düşük RAM (12-20 KB)
- ✅ CPU ve DMA paralel çalışır
- ✅ Flush sırasında CPU idle olmaz
- ✅ En iyi throughput

**LVGL Config**:
```c
static lv_color_t buf1[320 * 10];
static lv_color_t buf2[320 * 10];

lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 320 * 10);
```

**LVGL Kaynak**: `src/core/lv_refr.c:427-478` (double buffering logic)

---

### 1.5 Buffer Boyutu Seçim Matrisi

| Display | Full Buffer | Partial (10 satır) | Partial (30 satır) | Double Partial |
|---------|-------------|-------------------|-------------------|---------------|
| 240x240 | 115 KB | 4.8 KB | 14.4 KB | 9.6 KB |
| 320x240 | 154 KB | 6.4 KB | 19.2 KB | 12.8 KB |
| 480x320 | 307 KB | 9.6 KB | 28.8 KB | 19.2 KB |
| 800x480 | 768 KB | 16 KB | 48 KB | 32 KB |

**Öneri**:
- **<128 KB RAM**: Partial (10-20 satır)
- **128-256 KB RAM**: Double Partial (20-30 satır)
- **>512 KB RAM**: Full Double Buffer

---

## 2. Dirty Area Management

### 2.1 Invalidation Tracking

LVGL sadece **değişen alanları** (dirty areas) yeniden çizer.

**Nasıl Çalışır**:
1. Widget değişir (state, color, text, etc.)
2. Widget invalidate edilir → dirty area kaydedilir
3. Refresh cycle'da sadece dirty area'lar çizilir

```c
// Widget değiştiğinde
lv_obj_set_x(obj, 100);
// → Internally calls:
_lv_inv_area(disp, &obj->coords);  // Mark dirty

// Refresh cycle
for(each dirty_area) {
    draw_area(dirty_area);  // Sadece bu alanı çiz
}
```

**LVGL Kaynak**: `src/core/lv_refr.c:133-195` (invalidation)

---

### 2.2 Area Joining (Optimization)

Birbirine yakın dirty area'lar **birleştirilerek** draw call sayısı azaltılır.

**Örnek**:
```
Area 1: [10,10 - 50,50]  (40x40 = 1,600 px)
Area 2: [45,45 - 100,100] (55x55 = 3,025 px)

Overlap var!
Joined: [10,10 - 100,100] (90x90 = 8,100 px)

Ayrı çizmek: 1,600 + 3,025 = 4,625 px + 2x draw call overhead
Birleştirip çizmek: 8,100 px + 1x draw call

Trade-off:
  Fazla pixel: 8,100 - 4,625 = 3,475 px (~75% artış)
  Daha az draw call: 2 → 1 (50% azalış)

  Eğer draw call overhead > extra pixel cost → JOIN!
```

**LVGL Logic**:
```c
// src/core/lv_refr.c:237-287
void refr_join_areas(void)
{
    for(i = 0; i < inv_p; i++) {
        for(j = i + 1; j < inv_p; j++) {
            if(area_is_on(&inv_areas[i], &inv_areas[j])) {
                area_join(&joined, &inv_areas[i], &inv_areas[j]);

                // Birleştirilmiş alan daha mı ekonomik?
                if(area_get_size(&joined) <
                   area_get_size(&inv_areas[i]) +
                   area_get_size(&inv_areas[j]) + JOIN_DISTANCE) {
                    // Evet, birleştir
                    inv_areas[i] = joined;
                    inv_area_joined[j] = 1;
                }
            }
        }
    }
}
```

**Tuning Parameters**:
```c
// lv_conf.h
#define LV_DISP_DEF_REFR_PERIOD 30  // 30ms refresh (33 FPS)
#define LV_INV_BUF_SIZE 16          // Max 16 dirty area

// Eğer >16 dirty area varsa, tüm ekran invalidate edilir (fallback)
```

**Best Practice**:
- `LV_INV_BUF_SIZE` çok küçük → sık sık full refresh (yavaş)
- `LV_INV_BUF_SIZE` çok büyük → fazla RAM + karmaşık joining
- **Optimal**: 8-32 arası

---

### 2.3 Partial Invalidation vs Full Redraw

**Partial Invalidation**:
```c
lv_obj_invalidate(btn);  // Sadece button'ı yeniden çiz
```
- Avantaj: Az CPU, az pixel
- Dezavantaj: Karmaşık tracking

**Full Redraw**:
```c
lv_obj_invalidate(lv_scr_act());  // Tüm ekranı yeniden çiz
```
- Avantaj: Basit, tutarlı
- Dezavantaj: Fazla CPU

**Ne Zaman Full Redraw?**
- Ekran değişimi (screen transition)
- Major UI update (birçok widget değişiyor)
- Dirty area > ekranın %50'si

**Ne Zaman Partial?**
- Tek widget update (button click, label text change)
- Animasyon (küçük alan)
- User input (keyboard, slider)

---

### 2.4 Smart Invalidation Strategies

**1. Delayed Invalidation** (Batching):
```c
// ❌ KÖTÜ: Her değişiklikte invalidate
for(int i = 0; i < 100; i++) {
    lv_obj_set_x(obj, i);  // 100x invalidation!
}

// ✅ İYİ: Son değişiklikten sonra invalidate
lv_obj_remove_style_all(obj);  // Auto-invalidate disabled
for(int i = 0; i < 100; i++) {
    lv_obj_set_x_internal(obj, i);  // No invalidate
}
lv_obj_invalidate(obj);  // Single invalidation
```

**2. Conditional Invalidation**:
```c
void lv_obj_set_x(lv_obj_t* obj, lv_coord_t x)
{
    if(obj->coords.x1 == x) return;  // No change, no invalidate

    lv_area_t old_coords = obj->coords;
    obj->coords.x1 = x;

    // Only invalidate if visible
    if(!lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        _lv_inv_area(disp, &old_coords);  // Old position
        _lv_inv_area(disp, &obj->coords); // New position
    }
}
```

**3. Hierarchical Invalidation**:
```c
// Child değiştiğinde parent'ı invalidate etme
// (Eğer parent overflow hidden değilse)

if(!lv_obj_has_flag(obj->parent, LV_OBJ_FLAG_OVERFLOW_VISIBLE)) {
    // Parent clips child, invalidate parent
    lv_obj_invalidate(obj->parent);
} else {
    // Invalidate only child
    lv_obj_invalidate(obj);
}
```

---

## 3. Draw Pipeline Optimizasyonları

### 3.1 Draw Call Batching

**Problem**: Her shape için ayrı draw call pahalı.

```c
// ❌ KÖTÜ: 100x draw call
for(int i = 0; i < 100; i++) {
    draw_rect(i * 10, 0, 10, 10, COLOR_RED);  // SPI transfer her seferinde
}

// ✅ İYİ: 1x draw call
lv_color_t buffer[100 * 10];
for(int i = 0; i < 100; i++) {
    fill_buffer(&buffer[i * 10], COLOR_RED, 10);
}
flush_buffer(buffer, 0, 0, 1000, 10);  // Tek SPI transfer
```

**LVGL Implementation**:
LVGL zaten draw descriptor'ları toplayıp batch ediyor:
```c
// src/draw/lv_draw.c:123-145
void lv_draw_rect(lv_draw_ctx_t* draw_ctx, const lv_draw_rect_dsc_t* dsc,
                  const lv_area_t* coords)
{
    // Draw call buffer'a eklenir
    draw_ctx->draw_rect(draw_ctx, dsc, coords);
    // Flush edilmez, sonraki call'lar da buffer'a eklenir
}

// Flush sadece buffer dolduğunda veya frame bittiğinde
void lv_refr_now(lv_disp_t* disp)
{
    // ... draw all objects ...
    draw_ctx->wait_for_finish(draw_ctx);  // Tek flush!
}
```

---

### 3.2 Clipping ve Culling

**Clipping**: Ekran dışındaki alanları çizme.

```c
// Her draw call'da clip check
void draw_rect(lv_area_t* coords)
{
    lv_area_t clipped;
    if(!lv_area_intersect(&clipped, coords, &screen_area)) {
        return;  // Tamamen ekran dışı, çizme!
    }

    // Sadece clipped alanı çiz
    draw_rect_internal(&clipped);
}
```

**Culling**: Görünmeyen objeleri draw pipeline'dan çıkarma.

```c
// Widget tree traversal sırasında
void refr_obj_and_children(lv_obj_t* obj)
{
    // Hidden check
    if(lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        return;  // Skip entire subtree!
    }

    // Clip check
    lv_area_t clip_area;
    if(!lv_area_intersect(&clip_area, &obj->coords, &dirty_area)) {
        return;  // Not in dirty area, skip!
    }

    // Draw obj
    lv_obj_draw(obj);

    // Draw children
    for(child in obj->children) {
        refr_obj_and_children(child);
    }
}
```

**LVGL Kaynak**: `src/core/lv_refr.c:647-725` (object culling)

---

### 3.3 Draw Order Optimization

**Z-index Sorting**: Önden arkaya çiz (painter's algorithm).

```c
// Depth-first tree traversal
void draw_tree(lv_obj_t* root)
{
    // 1. Draw parent background
    draw_background(root);

    // 2. Draw children (sorted by z-index)
    sort_children_by_zindex(root->children);
    for(child in root->children) {
        draw_tree(child);  // Recursive
    }

    // 3. Draw parent foreground (borders, scrollbars)
    draw_foreground(root);
}
```

**LVGL**: Automatic Z-index handling:
```c
lv_obj_set_z_index(obj, 10);  // Higher = front
```

---

### 3.4 GPU Acceleration

**Desteklenen işlemler**:
- Fill (solid color)
- Blit (copy area)
- Blend (alpha blending)
- Transform (rotate, scale) - bazı GPU'larda

**LVGL Config**:
```c
// lv_conf.h
#define LV_USE_GPU_STM32_DMA2D 1  // STM32 Chrom-ART
#define LV_USE_GPU_NXP_PXP 1      // iMXRT PXP
#define LV_USE_GPU_NXP_VGLite 1   // iMXRT VGLite
#define LV_USE_GPU_SDL 1          // SDL (desktop)
```

**Custom GPU Driver**:
```c
void my_gpu_fill(lv_draw_ctx_t* draw_ctx, const lv_draw_rect_dsc_t* dsc,
                 const lv_area_t* coords)
{
    // Setup DMA2D for fill
    DMA2D->CR = DMA2D_R2M;  // Register to memory
    DMA2D->OCOLR = color;
    DMA2D->OMAR = (uint32_t)dest_buffer;
    DMA2D->OOR = line_offset;
    DMA2D->NLR = (width << 16) | height;

    DMA2D->CR |= DMA2D_CR_START;

    // Non-blocking! CPU can do other work
}

// Register driver
draw_ctx->draw_rect = my_gpu_fill;
```

**Performans Kazancı**:
- Fill: **10-50x hızlı** (GPU vs CPU)
- Blit: **5-20x hızlı**
- Blend: **2-10x hızlı**

**LVGL Kaynak**: `src/draw/` (GPU backends)

---

## 4. Memory Management

### 4.1 Memory Pool (Static Allocation)

**Problem**: `malloc()` heap fragmentation'a yol açar, deterministik değil.

**Çözüm**: LVGL kendi memory pool'unu kullanır.

```c
// lv_conf.h
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (32 * 1024)  // 32 KB pool

// LVGL internal
static uint8_t mem_pool[LV_MEM_SIZE];

void* lv_mem_alloc(size_t size)
{
    // Custom allocator:
    // - First-fit
    // - Fragmentation tracking
    // - Defragmentation on demand
}
```

**Best Practice**:
```c
// ✅ İYİ: Pool'dan al, otomatik free
lv_obj_t* obj = lv_obj_create(parent);

// ❌ KÖTÜ: Manuel malloc (heap fragmentation!)
lv_obj_t* obj = malloc(sizeof(lv_obj_t));
```

**Pool Boyutu Hesaplama**:
```
Widgets: ~100 obj * 200 bytes = 20 KB
Styles: ~20 style * 100 bytes = 2 KB
Buffers: User data = 5 KB
Reserve: 5 KB
Total: 32 KB
```

**Monitoring**:
```c
lv_mem_monitor_t mon;
lv_mem_monitor(&mon);

printf("Used: %d / %d bytes (%.1f%%)\n",
       mon.used_cnt, mon.total_size,
       100.0 * mon.used_cnt / mon.total_size);
printf("Fragmentation: %.1f%%\n", mon.frag_pct);
```

---

### 4.2 Object Caching

**Font Glyph Cache**:
```c
// lv_conf.h
#define LV_FONT_FMT_TXT_CACHE_SIZE 256  // Cache 256 glyphs

// Frequently used characters cached in RAM
// (e.g., ASCII 32-127)
```

**Image Cache**:
```c
#define LV_IMG_CACHE_DEF_SIZE 4  // Cache 4 decoded images

// Decoded PNG/JPG cached to avoid re-decoding
```

**Trade-off**:
- Daha fazla cache → daha hızlı draw, daha fazla RAM
- Daha az cache → daha yavaş draw, daha az RAM

**Optimal**:
- Font cache: 128-512 glyph (2-8 KB)
- Image cache: 1-4 image (depends on image size)

---

### 4.3 Stack vs Heap

**Stack (Preferred for temp data)**:
```c
void draw_button(lv_obj_t* obj)
{
    lv_draw_rect_dsc_t rect_dsc;  // Stack: ~100 bytes
    lv_draw_rect_dsc_init(&rect_dsc);

    lv_draw_rect(draw_ctx, &rect_dsc, &obj->coords);
    // rect_dsc automatically freed when function returns
}
```

**Heap (For persistent data)**:
```c
lv_obj_t* btn = lv_btn_create(parent);  // Heap: ~200 bytes
// btn lives until lv_obj_del(btn)
```

**Embedded Tip**:
- Cortex-M genelde **8-16 KB stack**
- Recursive function'larda dikkat! (max depth * stack usage)
- LVGL max recursion depth: ~10-20 (widget tree)

---

## 5. CPU ve Cache Optimizasyonları

### 5.1 Cache-Friendly Data Structures

**Problem**: Cache miss pahalı (100+ cycles).

**Çözüm**: Sequential memory access.

```c
// ❌ KÖTÜ: Linked list (cache miss her node'da)
typedef struct node_t {
    void* data;
    struct node_t* next;  // Pointer chase!
} node_t;

for(node_t* n = head; n != NULL; n = n->next) {
    process(n->data);  // Cache miss at each iteration
}

// ✅ İYİ: Array (sequential access)
typedef struct {
    void* data[MAX_COUNT];
    uint32_t count;
} array_t;

for(uint32_t i = 0; i < arr.count; i++) {
    process(arr.data[i]);  // Cache-friendly!
}
```

**LVGL Example**:
```c
// Widget children stored as array (not linked list)
struct lv_obj_t {
    lv_obj_t** children;  // Array of pointers
    uint32_t child_cnt;
};

// Iteration is cache-friendly
for(uint32_t i = 0; i < obj->child_cnt; i++) {
    process_child(obj->children[i]);
}
```

---

### 5.2 Data Alignment

**ARM Cortex-M**: Unaligned access yavaş (veya fault).

```c
// ❌ KÖTÜ: Unaligned struct
typedef struct {
    uint8_t a;    // Offset 0
    uint32_t b;   // Offset 1 (UNALIGNED!)
    uint8_t c;    // Offset 5
} bad_struct_t;  // Size: 6 bytes

// ✅ İYİ: Aligned struct
typedef struct {
    uint32_t b;   // Offset 0 (aligned)
    uint8_t a;    // Offset 4
    uint8_t c;    // Offset 5
    uint16_t pad; // Padding
} good_struct_t;  // Size: 8 bytes (but faster access)
```

**LVGL**: Struct'lar zaten aligned:
```c
typedef struct {
    lv_coord_t x1;  // int16_t, 2-byte aligned
    lv_coord_t y1;
    lv_coord_t x2;
    lv_coord_t y2;
} lv_area_t;  // Total 8 bytes, 2-byte aligned
```

---

### 5.3 SIMD Optimization

**ARM Cortex-M4/M7**: DSP instructions (SIMD).

```c
// Normal loop: 4 cycles/pixel
for(int i = 0; i < count; i++) {
    buffer[i] = color;
}

// SIMD (Cortex-M4): 1 cycle/pixel
void fill_simd(uint32_t* buffer, uint32_t color, uint32_t count)
{
    uint32_t dual_color = (color << 16) | color;  // Pack 2 pixels

    for(int i = 0; i < count/2; i++) {
        buffer[i] = dual_color;  // 2 pixels at once!
    }
}
```

**LVGL**: Optional SIMD:
```c
// lv_conf.h
#define LV_USE_SIMD 1  // Enable SIMD optimizations

// src/draw/sw/lv_draw_sw_blend.c uses ARM DSP intrinsics
```

---

### 5.4 Floating Point Avoidance

**Problem**: Cortex-M0/M3 has no FPU (software emulation ~100x slower).

```c
// ❌ KÖTÜ: Float (slow on M0/M3)
float alpha = opacity / 255.0f;
uint8_t result = (uint8_t)(src * alpha + dst * (1.0f - alpha));

// ✅ İYİ: Fixed-point integer
uint32_t alpha = opacity;  // 0-255
uint32_t result = (src * alpha + dst * (255 - alpha)) / 255;
// Or better: >> 8 (if 256 instead of 255)
uint32_t result = (src * alpha + dst * (256 - alpha)) >> 8;
```

**LVGL**: All math in fixed-point:
```c
typedef int16_t lv_coord_t;  // Screen coordinates (integer)

// Opacity always 0-255 (uint8_t), never float
```

---

## 6. Platform-Specific Optimizasyonlar

### 6.1 DMA (Direct Memory Access)

**Problem**: CPU SPI transfer sırasında blocked.

```c
// ❌ KÖTÜ: Polling SPI (CPU busy-waits)
void flush_blocking(lv_color_t* buf, uint32_t size)
{
    for(uint32_t i = 0; i < size; i++) {
        SPI_SEND(buf[i]);  // CPU waits each byte
        while(!SPI_TX_READY);
    }
}
// CPU utilization: 100% (wasted on waiting)

// ✅ İYİ: DMA SPI (CPU free during transfer)
void flush_dma(lv_color_t* buf, uint32_t size)
{
    DMA_START(buf, size);
    // CPU immediately returns, can do other work!
}

void DMA_IRQ_Handler(void)
{
    // Transfer complete
    lv_disp_flush_ready(disp);
}
```

**LVGL Driver**:
```c
void my_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area,
                 lv_color_t* color_p)
{
    // Set display window
    set_display_window(area->x1, area->y1, area->x2, area->y2);

    // Start DMA transfer (non-blocking)
    HAL_SPI_Transmit_DMA(&hspi, (uint8_t*)color_p, size);

    // DON'T call lv_disp_flush_ready() here!
    // It will be called in DMA complete callback
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    lv_disp_flush_ready(&disp_drv);  // NOW it's ready
}
```

**Performance Gain**: CPU utilization: 100% → **20-50%** (CPU can draw next buffer)

---

### 6.2 RTOS Integration

**FreeRTOS**: LVGL as separate task.

```c
void lv_task(void* params)
{
    while(1) {
        lv_timer_handler();  // Process LVGL tasks
        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms sleep (200 FPS max)
    }
}

void main(void)
{
    lv_init();

    xTaskCreate(lv_task, "LVGL", 4096, NULL, 1, NULL);

    vTaskStartScheduler();
}
```

**Mutex Protection**:
```c
// ❌ KÖTÜ: Race condition
void user_task(void* params)
{
    lv_label_set_text(label, "Hello");  // UNSAFE! LVGL not thread-safe
}

// ✅ İYİ: Mutex
SemaphoreHandle_t lvgl_mutex;

void user_task(void* params)
{
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    lv_label_set_text(label, "Hello");  // Safe
    xSemaphoreGive(lvgl_mutex);
}

void lv_task(void* params)
{
    while(1) {
        xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
        lv_timer_handler();
        xSemaphoreGive(lvgl_mutex);

        vTaskDelay(5);
    }
}
```

---

### 6.3 Display Interface Optimization

**SPI Optimization**:
```c
// ❌ KÖTÜ: Slow SPI clock
SPI_InitStruct.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;  // 500 kHz

// ✅ İYİ: Fast SPI clock
SPI_InitStruct.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // 16 MHz

// Performance: 500 kHz → 16 MHz = 32x faster!
```

**Parallel RGB Interface** (best performance):
```c
// LTDC (STM32F7, H7) - direct RAM to display
// No SPI overhead, DMA burst transfer
// Bandwidth: ~100 MB/s vs SPI ~2 MB/s
```

**Interface Comparison**:
| Interface | Bandwidth | CPU Usage | Cost |
|-----------|-----------|-----------|------|
| SPI 8 MHz | 1 MB/s | High | Low |
| SPI 40 MHz | 5 MB/s | Medium | Low |
| Parallel 8-bit | 10 MB/s | Low (DMA) | Medium |
| RGB 16-bit | 50 MB/s | Very Low | High |
| LTDC (framebuffer) | 100 MB/s | Minimal | High |

---

## 7. Benchmark ve Profiling

### 7.1 Performance Metrics

**FPS (Frames Per Second)**:
```c
uint32_t frame_count = 0;
uint32_t last_time = 0;

void lv_task(void)
{
    lv_timer_handler();
    frame_count++;

    if(HAL_GetTick() - last_time >= 1000) {
        printf("FPS: %lu\n", frame_count);
        frame_count = 0;
        last_time = HAL_GetTick();
    }
}
```

**Render Time**:
```c
void my_monitor_cb(lv_disp_drv_t* disp_drv, uint32_t time_ms, uint32_t px_num)
{
    printf("Rendered %lu px in %lu ms (%.1f kpx/s)\n",
           px_num, time_ms, (float)px_num / time_ms);
}

disp_drv.monitor_cb = my_monitor_cb;
```

**Memory Usage**:
```c
lv_mem_monitor_t mon;
lv_mem_monitor(&mon);

printf("Heap: %d / %d bytes (%.1f%% used, %.1f%% frag)\n",
       mon.used_cnt, mon.total_size,
       100.0 * mon.used_cnt / mon.total_size,
       mon.frag_pct);
```

---

### 7.2 Profiling Hotspots

**Systick-based Profiling**:
```c
#define PROFILE_START() uint32_t t_start = DWT->CYCCNT
#define PROFILE_END(name) \
    printf("%s: %lu cycles\n", name, DWT->CYCCNT - t_start)

void lv_refr_now(lv_disp_t* disp)
{
    PROFILE_START();

    // ... invalidation ...
    PROFILE_END("Invalidation");

    PROFILE_START();
    // ... drawing ...
    PROFILE_END("Drawing");

    PROFILE_START();
    // ... flush ...
    PROFILE_END("Flush");
}
```

**DWT (Data Watchpoint and Trace)** (Cortex-M3/M4/M7):
```c
// Enable cycle counter
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0;
DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

// Read cycles
uint32_t cycles = DWT->CYCCNT;
```

---

### 7.3 Optimization Checklist

**Display**:
- [ ] Partial buffering kullanılıyor mu? (double partial ideal)
- [ ] Buffer boyutu optimize mi? (10-30 satır)
- [ ] SPI clock max hızda mı?
- [ ] DMA kullanılıyor mu?

**Draw Pipeline**:
- [ ] GPU acceleration aktif mi?
- [ ] Dirty area tracking çalışıyor mu?
- [ ] Area joining enabled mı?
- [ ] Clipping/culling çalışıyor mu?

**Memory**:
- [ ] Memory pool yeterli mi? (fragmentation < 10%)
- [ ] Font cache optimize mi?
- [ ] Image cache optimize mi?
- [ ] Stack overflow yok mu?

**CPU**:
- [ ] Float kullanımı minimize mi?
- [ ] SIMD enabled mı? (Cortex-M4+)
- [ ] Aligned access yapılıyor mu?
- [ ] Cache-friendly data structures mı?

**RTOS**:
- [ ] LVGL task priority doğru mu?
- [ ] Mutex protection var mı?
- [ ] Task stack yeterli mi?

---

## 8. Gerçek Dünya Örnekleri

### 8.1 Örnek: STM32F103 (Cortex-M3, 64 KB RAM, 72 MHz)

**Display**: 240x320 SPI (ILI9341)

**Optimal Config**:
```c
// lv_conf.h
#define LV_COLOR_DEPTH 16
#define LV_MEM_SIZE (24 * 1024)  // 24 KB for LVGL

// Partial double buffering: 10 satır
static lv_color_t buf1[240 * 10];  // 4.8 KB
static lv_color_t buf2[240 * 10];  // 4.8 KB

// Total RAM usage: 24 + 4.8 + 4.8 = 33.6 KB (~52% of 64 KB)
```

**Performance**:
- FPS: 15-25 (simple UI)
- Render time: 40-60 ms
- CPU usage: 40-60%

---

### 8.2 Örnek: ESP32 (Dual-Core, 520 KB RAM, 240 MHz)

**Display**: 320x240 SPI (ST7789)

**Optimal Config**:
```c
#define LV_COLOR_DEPTH 16
#define LV_MEM_SIZE (64 * 1024)

// Double partial buffering: 30 satır
static lv_color_t buf1[320 * 30];  // 19.2 KB
static lv_color_t buf2[320 * 30];  // 19.2 KB

// DMA SPI + FreeRTOS
// CPU0: LVGL task
// CPU1: User tasks
```

**Performance**:
- FPS: 30-60 (complex UI)
- Render time: 15-30 ms
- CPU usage: 20-40%

---

### 8.3 Örnek: STM32H7 (Cortex-M7, 1 MB RAM, 480 MHz)

**Display**: 800x480 RGB (LTDC)

**Optimal Config**:
```c
#define LV_COLOR_DEPTH 16
#define LV_MEM_SIZE (128 * 1024)

// Full double buffering (SDRAM)
static lv_color_t buf1[800 * 480];  // 768 KB
static lv_color_t buf2[800 * 480];  // 768 KB

// GPU (DMA2D) enabled
#define LV_USE_GPU_STM32_DMA2D 1
```

**Performance**:
- FPS: 60 (constant)
- Render time: 5-10 ms
- CPU usage: 5-15%

---

## 9. Özet ve Hızlı Referans

### Buffer Strategy Decision Tree

```
RAM < 128 KB?
├─ Yes → Partial buffering (10-20 satır)
│   ├─ DMA available? → Double partial
│   └─ No DMA → Single partial
└─ No → RAM < 512 KB?
    ├─ Yes → Double partial (30 satır)
    └─ No → Full double buffer + GPU
```

### Quick Wins (En etkili optimizasyonlar)

1. **DMA kullanın** (+50% CPU reduction)
2. **Double partial buffering** (+30% FPS)
3. **SPI clock max** (+2-5x throughput)
4. **GPU acceleration** (+10-50x draw speed)
5. **Dirty area tracking** (+20-80% FPS on partial updates)

### Performance Targets

| Device Class | Target FPS | Acceptable Render Time |
|--------------|------------|------------------------|
| Low-end (Cortex-M0) | 10-15 | <100 ms |
| Mid-range (Cortex-M3/M4) | 15-30 | <50 ms |
| High-end (Cortex-M7) | 30-60 | <16 ms |

---

## 10. Referanslar

- **LVGL Dökümanları**: https://docs.lvgl.io/master/overview/rendring.html
- **LVGL Forum**: https://forum.lvgl.io
- **LVGL GitHub**: https://github.com/lvgl/lvgl
- **Kaynak Kod**:
  - `src/core/lv_refr.c` - Rendering engine
  - `src/draw/` - Draw backends
  - `src/misc/lv_mem.c` - Memory management

---

**Versiyon**: 1.0
**Tarih**: 2025-11-18
**Yazar**: LVGL Performance Guide
**Lisans**: MIT
