# Performance Techniques Demos (Performans Teknikleri Demoları)

Bu dizin, LVGL'de kullanılan performans optimizasyon tekniklerini gösteren standalone C demo programlarını içerir.

## İçerik

| Demo | Teknik | Kazanç |
|------|--------|--------|
| `01_dirty_region_tracking_demo.c` | **Dirty Region Tracking** | 80-95% daha az piksel |
| `02_cache_friendly_structures_demo.c` | **Cache-Friendly Structures** | 2-5x daha hızlı |
| `03_bit_based_filtering_demo.c` | **Bit-Based Filtering** | 10x daha hızlı flag check |
| `04_contiguous_memory_layout_demo.c` | **Contiguous Memory** | 3-50x daha hızlı access |
| `05_all_strategies_comparison.c` | **Tümü Birlikte** | 50-100x toplam kazanç |

## Derleme ve Çalıştırma

```bash
# Her demo bağımsız derlenebilir
gcc 01_dirty_region_tracking_demo.c -o demo1
./demo1

gcc 02_cache_friendly_structures_demo.c -o demo2
./demo2

gcc 03_bit_based_filtering_demo.c -o demo3
./demo3

gcc 04_contiguous_memory_layout_demo.c -o demo4
./demo4

gcc 05_all_strategies_comparison.c -o demo5
./demo5
```

## Demo Detayları

### 1. Dirty Region Tracking
- Sadece değişen bölgeleri çizme
- Smart region merging
- %80-95 pixel tasarrufu
- LVGL: 32 region buffer

### 2. Cache-Friendly Structures
- Hot/cold data separation
- Bitfield packing (10 bool → 2 byte)
- L1 cache'e sığacak boyut (32-48 byte)
- 2-5x daha hızlı access

### 3. Bit-Based Filtering
- 32 flag in 4 bytes
- Bitwise operations (1-3 cycle)
- 10x daha hızlı flag kontrolü
- Multiple flags tek operasyonda

### 4. Contiguous Memory Layout
- Array-based vs linked list
- O(1) access vs O(n)
- CPU prefetching aktif
- 3-50x daha hızlı

### 5. All Strategies Combined
- Tüm tekniklerin birlikte kullanımı
- Gerçekçi senaryo (100 widget)
- 50-100x toplam performans artışı

## Öğrenme Sırası

1. **Dirty Region Tracking** → Pixel tasarrufu
2. **Cache-Friendly Structures** → Bellek optimizasyonu
3. **Bit-Based Filtering** → Hızlı flag işlemleri
4. **Contiguous Memory** → Access pattern optimizasyonu
5. **All Strategies** → Sinerjik etki

## LVGL'de Kullanım

```c
// 1. Dirty Regions
lv_obj_invalidate(button);  // Sadece button'u işaretle

// 2. Cache-Friendly
typedef struct {
    lv_area_t coords;  // 8 byte (hot)
    uint32_t flags;    // 4 byte (hot + bitfield)
    // ...
    spec_attr_t* cold; // 8 byte (lazy, cold data)
} lv_obj_t;  // 48 bytes total

// 3. Bit Operations
if(obj->flags & LV_OBJ_FLAG_VISIBLE) { ... }  // 1-2 cycles

// 4. Contiguous Children
for(uint32_t i = 0; i < child_cnt; i++) {
    lv_obj_t* child = parent->children[i];  // Sequential!
}
```

## Performans Metrikleri

| Teknik | Naif | LVGL | Kazanç |
|--------|------|------|--------|
| **Pixels/Frame** | 153,600 | 5,000-10,000 | 15-30x |
| **Struct Size** | 128 byte | 48 byte | 2.7x |
| **Flag Check** | 10-20 cycle | 1-2 cycle | 10x |
| **Child Access** | O(n), 50 cycle | O(1), 2 cycle | 25x |

## Özet

Bu teknikler birlikte:
- ✅ 80-95% daha az pixel çizimi
- ✅ 60-75% daha az bellek
- ✅ 2-10x daha hızlı erişim
- ✅ L1 cache'te kalma

→ **Sonuç**: 16 MHz MCU'da 60 FPS!

---

**İlgili Döküman**: `../../docs/LVGL_PERFORMANCE_TECHNIQUES.md`
