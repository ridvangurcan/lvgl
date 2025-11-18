# LVGL Performance Optimization Demo

## 📋 Genel Bakış

Bu demo, **düşük kaynaklı gömülü cihazlarda LVGL performans optimizasyonlarını** tamamen bağımsız, saf C kodu ile gösterir. LVGL kütüphanesine bağımlılığı yoktur ve standalone olarak derlenip çalıştırılabilir.

### Amaç

LVGL'nin düşük RAM ve düşük CPU gücüne sahip MCU'larda nasıl optimize edildiğini, farklı buffer stratejilerini, dirty area management'ı ve performance metrics'i öğretmek için tasarlanmıştır.

## 🎯 Kapsanan Konular

### 1. **Buffer Strategies**
- Full Double Buffering (300 KB RAM)
- Partial Single Buffering (6 KB RAM)
- Partial Double Buffering (12 KB RAM) - **BEST!**
- Direct Mode (0 KB RAM - not recommended)
- Memory trade-offs

### 2. **Dirty Area Management**
- Invalidation tracking
- Area joining optimization
- Draw call reduction
- Partial vs full redraw

### 3. **Performance Metrics**
- FPS (Frames Per Second)
- Render time
- Pixels drawn
- Flush calls
- Throughput (kpixels/sec)

### 4. **Optimization Scenarios**
- Single widget update (button click)
- Multiple widgets (area joining)
- Full screen redraw (screen transition)
- Animation (continuous updates)
- Memory comparison

## 🏗️ Simüle Edilen Optimizasyonlar

### Buffer Strategy Comparison

```
Full Double Buffer:
├─ RAM: 300 KB
├─ Flush: 1x per frame
└─ Speed: FASTEST (for full redraws)

Partial Single Buffer:
├─ RAM: 6 KB (50x less!)
├─ Flush: Nx per frame (N=strips)
└─ Speed: SLOW (CPU waits for flush)

Partial Double Buffer: ★ OPTIMAL ★
├─ RAM: 12 KB (25x less!)
├─ Flush: Nx per frame
├─ Speed: FAST (CPU/DMA parallel)
└─ Best RAM/performance balance
```

### Dirty Area Joining

```
Before Joining:
  Area 1: [10,10 - 100,50]    (3,731 px)
  Area 2: [80,30 - 180,70]    (4,141 px) ← OVERLAP!
  Total: 7,872 px, 2 draw calls

After Joining:
  Joined: [10,10 - 180,70]    (10,540 px)
  Extra pixels: 2,668 (+33%)
  Draw calls: 1 (-50%)

Trade-off: 33% more pixels, but 50% fewer draw calls!
If draw call overhead > pixel cost → JOIN wins!
```

## 🔧 Derleme ve Çalıştırma

### Gereksinimler
- GCC veya herhangi bir C derleyici
- Standart C kütüphanesi

### Derleme

```bash
cd design_and_documents/demos/performance
gcc lvgl_performance_demo.c -o demo
```

### Çalıştırma

```bash
./demo
```

### Çıktıyı Dosyaya Kaydetme

```bash
./demo > results.txt
```

## 📊 Örnek Çıktı

```
════════════════════════════════════════════════════════════════
  SCENARIO 1: Single Widget Update (Button Click)
════════════════════════════════════════════════════════════════

>> User clicks button at [100,100 - 200,140]

╔════════════════════════════════════════════════════════════════╗
║ Buffer Strategy: Partial Double Buffering
╚════════════════════════════════════════════════════════════════╝
  Display: 320x240 (16-bit color)
  Buffer count: 2
  Buffer size: 6400 bytes (6.2 KB)
  Total RAM: 12800 bytes (12.5 KB)
  Partial buffer: 10 rows (4.2% of screen height)
  RAM savings: 24.0x (vs full double buffer)

>> Rendering 1 dirty areas:
  Area 0:
  Coords [100,100 - 200,140] (101x41 = 4141 px)
    Splitting into 5 strips (10 rows each):
      Strip 0: Drawing to buffer 0...
        (Buffer 1 flushing in parallel via DMA)
        Flushing (2 ms)
      ...
    CPU/DMA parallelism saved 3 ms!

╔════════════════════════════════════════════════════════════════╗
║ Performance Metrics
╚════════════════════════════════════════════════════════════════╝
  Pixels drawn: 4141 (5.4% of screen)
  Draw calls: 1
  Flush calls: 5
  Render time: 7 ms
  FPS: 142.9
  Throughput: 591.6 kpixels/sec
  Rating: ★★★★★ EXCELLENT (smooth animation)
```

## 🎓 Öğrenme Yolu

### 1. İlk Okuma
Kodu yukarıdan aşağıya okuyun. Her bölüm detaylı açıklamalı:
- Configuration (`DISPLAY_HOR_RES`, `COLOR_DEPTH`)
- Area utilities (intersection, join, size)
- Dirty area management
- Buffer strategies
- Performance metrics
- Flush simulation
- Demo scenarios

### 2. Kodu Çalıştırma
Demo'yu çalıştırın ve 5 farklı scenario'yu inceleyin:
1. **Scenario 1**: Single widget update → Partial buffering wins
2. **Scenario 2**: Multiple widgets → Area joining benefit
3. **Scenario 3**: Full screen redraw → Full buffer vs partial
4. **Scenario 4**: Animation → 60 frame simulation
5. **Scenario 5**: Memory comparison table

### 3. Parametreleri Değiştirme

**Display boyutunu değiştir**:
```c
#define DISPLAY_HOR_RES 480
#define DISPLAY_VER_RES 320
```

**Buffer boyutunu değiştir**:
```c
buffer_config_t config = get_buffer_config(
    BUFFER_STRATEGY_PARTIAL_DOUBLE,
    20  // 20 satır yerine 30, 40, etc. dene
);
```

**Join distance'ı değiştir**:
```c
#define JOIN_DISTANCE 10  // 5, 20, 50 ile dene
```

**Flush maliyetini değiştir**:
```c
#define FLUSH_OVERHEAD_MS 2   // SPI overhead
#define PIXELS_PER_MS 10000   // SPI throughput
```

### 4. LVGL Karşılaştırması

| Demo Fonksiyon | LVGL Fonksiyon | Dosya |
|----------------|----------------|-------|
| `dirty_area_add()` | `_lv_inv_area()` | `src/core/lv_refr.c:133` |
| `dirty_area_join()` | `refr_join_areas()` | `src/core/lv_refr.c:237` |
| `get_buffer_config()` | `lv_disp_draw_buf_init()` | `src/hal/lv_hal_disp.c` |
| `simulate_render()` | `lv_refr_now()` | `src/core/lv_refr.c:337` |
| `calculate_metrics()` | `monitor_cb` | `src/hal/lv_hal_disp.h` |

## 🧩 Kod Analizi

### Area Joining Logic

```c
void dirty_area_join(dirty_area_list_t* list)
{
    for(uint32_t i = 0; i < list->count; i++) {
        for(uint32_t j = i + 1; j < list->count; j++) {
            // Check overlap
            if(area_is_on(&list->areas[i], &list->areas[j])) {
                area_t joined_area;
                area_join(&joined_area, &list->areas[i], &list->areas[j]);

                // Join if economical
                if(size_joined < size_i + size_j + JOIN_DISTANCE^2) {
                    list->areas[i] = joined_area;
                    joined[j] = true;  // Mark for removal
                }
            }
        }
    }
    // Compact array (remove marked areas)
}
```

**LVGL'de aynı mantık**: `src/core/lv_refr.c:237-287`

**Ne zaman join yapılır?**
- İki area overlap ediyor
- Joined area boyutu < (area1 + area2 + threshold)
- Threshold = `JOIN_DISTANCE²` (LVGL'de configurable)

**Örnek**:
```
Area 1: 1000 px
Area 2: 1000 px
Joined: 2500 px (overlap nedeniyle)

Extra pixels: 2500 - 2000 = 500 px
Draw calls: 2 → 1

Eğer 1 draw call > 500 pixel cost → JOIN!
```

### Buffer Strategy Selection

```c
buffer_config_t get_buffer_config(buffer_strategy_t strategy,
                                  uint32_t partial_height)
{
    switch(strategy) {
        case BUFFER_STRATEGY_FULL_DOUBLE:
            // 2 full-screen buffers
            buffer_size = DISPLAY_HOR_RES * DISPLAY_VER_RES * BPP;
            buffer_count = 2;
            break;

        case BUFFER_STRATEGY_PARTIAL_DOUBLE:
            // 2 partial buffers (N rows each)
            buffer_size = DISPLAY_HOR_RES * partial_height * BPP;
            buffer_count = 2;
            break;
    }
}
```

**LVGL'de**:
```c
// lv_conf.h
static lv_color_t buf1[DISPLAY_HOR_RES * PARTIAL_HEIGHT];
static lv_color_t buf2[DISPLAY_HOR_RES * PARTIAL_HEIGHT];

lv_disp_draw_buf_init(&draw_buf, buf1, buf2,
                      DISPLAY_HOR_RES * PARTIAL_HEIGHT);
```

**Partial height seçimi**:
- Çok küçük (1-5 satır) → çok fazla flush overhead
- Çok büyük (>50 satır) → fazla RAM
- **Optimal**: 10-30 satır (display yüksekliğinin tam böleni)

### Flush Simulation

```c
static uint32_t simulate_flush_cost(uint32_t pixel_count)
{
    uint32_t transfer_time = pixel_count / PIXELS_PER_MS;
    return FLUSH_OVERHEAD_MS + transfer_time;
}
```

**Gerçek SPI örneği**:
```c
// SPI 16 MHz, 16-bit color
// Throughput: 16,000,000 / 16 = 1,000,000 pixels/sec = 1000 px/ms

#define FLUSH_OVERHEAD_MS 1   // CS toggle, command send
#define PIXELS_PER_MS 1000

// 10,000 pixel flush:
// Time = 1 + (10000 / 1000) = 11 ms
```

**DMA Optimization**:
```c
// Without DMA: CPU blocked during transfer
time_total = time_draw + time_flush

// With DMA: CPU draws next buffer while flushing
time_total = max(time_draw, time_flush)

// Double buffering: CPU and DMA parallel!
// Speedup: ~2x (depending on draw/flush ratio)
```

### Performance Metrics Calculation

```c
void calculate_metrics(perf_metrics_t* metrics,
                      const dirty_area_list_t* dirty_areas,
                      const buffer_config_t* buffer_config,
                      uint32_t render_time_ms)
{
    // Total pixels
    metrics->pixels_drawn = sum(area_get_size(dirty_areas[i]));

    // Flush calls (depends on buffer strategy)
    if(PARTIAL_BUFFER) {
        for(each area) {
            strips = ceil(area_height / buffer_height);
            metrics->flush_calls += strips;
        }
    }

    // FPS
    metrics->fps = 1000.0 / render_time_ms;

    // Throughput
    metrics->throughput_kpixels_per_sec =
        (float)pixels_drawn / render_time_ms;
}
```

**LVGL monitoring**:
```c
void my_monitor_cb(lv_disp_drv_t* disp_drv, uint32_t time_ms,
                   uint32_t px_num)
{
    printf("Rendered %lu px in %lu ms\n", px_num, time_ms);
}

disp_drv.monitor_cb = my_monitor_cb;
```

## 💡 Önemli Çıkarımlar

### 1. Buffer Strategy Decision Tree

```
RAM < 64 KB?
├─ Yes → Partial Single (10 rows) = 6 KB
└─ No
    └─ RAM < 128 KB?
        ├─ Yes → Partial Double (10 rows) = 12 KB ★
        └─ No
            └─ RAM < 256 KB?
                ├─ Yes → Partial Double (30 rows) = 38 KB
                └─ No → Full Double = 300 KB (if fits)
```

### 2. Partial Buffer Height Selection

| Height (rows) | RAM (320px) | Flushes (240px screen) | Optimal For |
|---------------|-------------|------------------------|-------------|
| 5 | 3.2 KB | 48 | Extremely low RAM |
| 10 | 6.4 KB | 24 | Low RAM (<64 KB) |
| 20 | 12.8 KB | 12 | Mid RAM (64-128 KB) ★ |
| 30 | 19.2 KB | 8 | Mid-High RAM (128-256 KB) |
| 60 | 38.4 KB | 4 | High RAM (>256 KB) |
| 240 (full) | 153.6 KB | 1 | Very high RAM (>512 KB) |

**Rule of Thumb**: Buffer height = display_height / (8-16) flushes

### 3. Dirty Area Joining Benefits

**Scenario**: 2 overlapping buttons clicked

```
Without Joining:
  Area 1: 4000 px
  Area 2: 4000 px
  Total: 8000 px, 2 draw calls, 10 flushes

With Joining:
  Joined: 9000 px (+1000 extra)
  Total: 9000 px, 1 draw call, 5 flushes

Savings:
  Draw calls: 2 → 1 (50%)
  Flushes: 10 → 5 (50%)
  Extra cost: +1000 px (12.5%)

Net benefit: HUGE! (flush overhead >> extra pixels)
```

### 4. Partial vs Full Redraw

**Single widget update (5% of screen)**:
```
Partial: 4,000 px, 7 ms → 142 FPS ★
Full: 76,800 px, 80 ms → 12 FPS
Speedup: 11x
```

**Full screen transition**:
```
Partial: 76,800 px, 80 ms (multiple flushes)
Full: 76,800 px, 50 ms (single flush) ★
Speedup: 0.6x (full is better!)
```

**Conclusion**: Partial redraw for small changes, full redraw for major updates.

### 5. Animation Performance

**60 frame animation (50x50 object moving)**:
```
Partial Double Buffer (10 rows):
  Total: 150,000 px
  Time: 1000 ms
  FPS: 60 ★★★★★

Full Double Buffer:
  Total: 4,608,000 px (full screen each frame!)
  Time: 3000 ms
  FPS: 20 ★★★☆☆
```

**Conclusion**: Partial updates + dirty area tracking = smooth animation!

## 🐛 Yaygın Performans Sorunları

### 1. Çok Küçük Buffer

```c
// ❌ KÖTÜ: 1 satırlık buffer
buffer_config_t config = get_buffer_config(
    BUFFER_STRATEGY_PARTIAL_SINGLE, 1);

// Problem: 240 flush call per frame!
// Flush overhead: 240 * 2ms = 480 ms
// FPS: ~2 (unacceptable)

// ✅ İYİ: 10-20 satırlık buffer
buffer_config_t config = get_buffer_config(
    BUFFER_STRATEGY_PARTIAL_DOUBLE, 20);

// 12 flush per frame
// Flush overhead: 12 * 2ms = 24 ms
// FPS: ~40 (good!)
```

### 2. Join Distance Çok Küçük

```c
// ❌ KÖTÜ: JOIN_DISTANCE = 0
#define JOIN_DISTANCE 0

// Problem: Sadece tam overlap join edilir
// Çok fazla draw call

// ✅ İYİ: JOIN_DISTANCE = 10-20
#define JOIN_DISTANCE 10

// Yakın area'lar join edilir
// Draw call sayısı azalır
```

### 3. Her Widget Update'de Full Redraw

```c
// ❌ KÖTÜ: Button click'te tüm ekranı redraw
void on_button_click(lv_obj_t* btn)
{
    lv_obj_invalidate(lv_scr_act());  // Full screen!
}

// ✅ İYİ: Sadece button'ı invalidate
void on_button_click(lv_obj_t* btn)
{
    lv_obj_invalidate(btn);  // Just button
}
```

### 4. Single Buffer + DMA Yok

```c
// ❌ KÖTÜ: Single buffer + polling SPI
BUFFER_STRATEGY_PARTIAL_SINGLE + SPI_Transmit()

// Problem: CPU blocked during flush
// Throughput: 50%

// ✅ İYİ: Double buffer + DMA
BUFFER_STRATEGY_PARTIAL_DOUBLE + SPI_Transmit_DMA()

// CPU draws buffer 2 while buffer 1 flushes
// Throughput: ~100% (parallelism)
```

## 🎯 Pratik Egzersizler

### Egzersiz 1: Display Boyutu Değiştirme
480x320 display için optimal buffer strategy'yi bulun:
- Full double buffer RAM?
- Partial 20 satır RAM?
- Hangisi daha ekonomik?

### Egzersiz 2: Custom Scenario
Kendi UI scenario'nuzu ekleyin:
- 3 widget update (non-overlapping)
- Area joining benefit var mı?
- Hangi strategy en hızlı?

### Egzersiz 3: Flush Cost Tuning
SPI hızını değiştirin:
```c
// 8 MHz SPI
#define PIXELS_PER_MS 500

// 40 MHz SPI
#define PIXELS_PER_MS 2500
```
FPS nasıl değişiyor?

### Egzersiz 4: Memory Budget
64 KB total RAM, 32 KB LVGL için kullanılabilir:
- Optimal buffer strategy?
- Partial height?
- Expected FPS?

## 📚 Ek Kaynaklar

- **LVGL Performans Dökümanı**: `../docs/LVGL_PERFORMANCE_OPTIMIZATION.md`
- **LVGL Resmi Döküman**: https://docs.lvgl.io/master/overview/rendring.html
- **LVGL Forum**: https://forum.lvgl.io
- **Diğer Demo'lar**:
  - `../widget_system/` - Widget architecture
  - `../render_pipeline/` - Render pipeline
  - `../style_system/` - Style and theme

## ✅ Checklist

Demo'yu anladığınızı kontrol edin:

- [ ] Buffer strategy'leri arasındaki farklar?
- [ ] Partial buffering nasıl çalışır?
- [ ] Dirty area joining ne zaman faydalı?
- [ ] Double buffering + DMA avantajı?
- [ ] Flush overhead nedir?
- [ ] FPS nasıl hesaplanır?
- [ ] Partial vs full redraw ne zaman?
- [ ] Optimal buffer height seçimi?
- [ ] Memory/performance trade-off?
- [ ] LVGL kaynak kodunda eşdeğer fonksiyonlar?

## 🎉 Sonuç

Bu demo, LVGL'nin düşük kaynaklı cihazlarda nasıl optimize edildiğini gösterir. **Partial double buffering** en iyi RAM/performans dengesini sağlar.

**Önemli**: Bu bir öğretim aracıdır, production kodda LVGL'nin resmi API'lerini kullanın!

**Öğrendikleriniz**:
- ✅ Buffer stratejileri (Full, Partial, Double Partial)
- ✅ Dirty area management (tracking, joining)
- ✅ Performance metrics (FPS, throughput)
- ✅ Memory optimization (24-50x RAM savings)
- ✅ DMA parallelism (2x speedup)
- ✅ Real-world optimization scenarios

---

**Versiyon**: 1.0
**Tarih**: 2025-11-18
**Yazar**: LVGL Performance Demo
**Lisans**: MIT (Eğitim amaçlı)
