# LVGL Render Pipeline Architecture Demo

## 📋 Genel Bakış

Bu demo, **LVGL'nin render pipeline mimarisini** tamamen bağımsız, saf C kodu ile gösterir. LVGL kütüphanesine bağımlılığı yoktur ve standalone olarak derlenip çalıştırılabilir.

### Amaç

LVGL'nin karmaşık rendering mekanizmasını adım adım öğretmek için tasarlanmıştır. Gerçek grafik çizimi yapmaz, bunun yerine **printf ile her adımı detaylı açıklar**.

## 🎯 Kapsanan Konular

### 1. **Dirty Region Tracking (Invalidation System)**
- Sadece değişen alanların işaretlenmesi
- Ekran sınırları kontrolü
- Invalidation buffer yönetimi
- Render sırasında yeni invalidation önleme

### 2. **Area Joining Optimization**
- Çakışan dirty area'ların birleştirilmesi
- Piksel tasarrufu hesaplama
- Flyweight pattern optimizasyonu

### 3. **Recursive Drawing**
- Object tree'nin depth-first traversal'ı
- Clip area yönetimi
- Main/Children/Post draw fazları

### 4. **Backend Abstraction**
- Function pointer tabanlı backend seçimi
- Strategy pattern implementasyonu
- SW backend simülasyonu

### 5. **Object Hierarchy**
- Parent-child ilişkileri
- Composite pattern
- Widget inheritance (button, label)

## 🏗️ Mimari Bileşenler

```
┌─────────────────────────────────────────────────┐
│  Display (Invalidation Buffer)                 │
│  - inv_areas[16]                                │
│  - inv_count                                    │
└───────────────┬─────────────────────────────────┘
                │
                ↓
┌─────────────────────────────────────────────────┐
│  Render Pipeline                                │
│  1. Layout Update                               │
│  2. Area Joining                                │
│  3. Recursive Drawing                           │
│  4. Cleanup                                     │
└───────────────┬─────────────────────────────────┘
                │
                ↓
┌─────────────────────────────────────────────────┐
│  Draw Context (Backend Abstraction)             │
│  - draw_rect() → sw_draw_rect()                 │
│  - draw_text() → sw_draw_text()                 │
└───────────────┬─────────────────────────────────┘
                │
                ↓
┌─────────────────────────────────────────────────┐
│  Object Tree                                    │
│  screen                                         │
│    └─ panel                                     │
│        ├─ button1                               │
│        ├─ button2                               │
│        └─ label                                 │
└─────────────────────────────────────────────────┘
```

## 🔧 Derleme ve Çalıştırma

### Gereksinimler
- GCC veya herhangi bir C derleyici
- Standart C kütüphanesi (stdio, stdlib, string)

### Derleme

```bash
cd design_and_documents/demos/render_pipeline
gcc lvgl_render_pipeline_demo.c -o demo
```

### Çalıştırma

```bash
./demo
```

### Çıktıyı Dosyaya Kaydetme

```bash
./demo > output.txt
```

## 📊 Örnek Çıktı

Demo çalıştırıldığında şöyle bir çıktı üretir:

```
╔════════════════════════════════════════════════════════════════╗
║         LVGL Render Pipeline Architecture Demo                ║
║                                                                ║
║  Bu demo LVGL'nin render pipeline mimarisini gösterir:        ║
║  1. Invalidation (Dirty Region Tracking)                      ║
║  2. Area Joining Optimization                                 ║
║  3. Recursive Drawing                                         ║
║  4. Backend Abstraction (Strategy Pattern)                    ║
║  5. Object Hierarchy (Composite Pattern)                      ║
╚════════════════════════════════════════════════════════════════╝

========================================
DISPLAY INITIALIZATION
========================================
Resolution: 800x480

========================================
BUILDING OBJECT TREE
========================================

>>> CREATE OBJECT (class: obj)
    [CONSTRUCTOR] Base object #0
    New coords: [0,0 - 799,479] (w=800, h=480, size=384000)

>>> INVALIDATE AREA: [0,0 - 799,479]
    [ADDED] inv_areas[0]

...
```

## 🎓 Öğrenme Yolu

### 1. İlk Okuma
Kodu yukarıdan aşağıya okuyun. Her bölüm açıklamalı:
- Basic types (area_t, color_t)
- Area utility functions
- Display ve invalidation
- Draw context
- Object system
- Widgets
- Render pipeline
- Main demo

### 2. Kodu Çalıştırma
Demo'yu çalıştırın ve çıktıyı inceleyin. Her adımın ne yaptığını görün.

### 3. Değişiklikler Yapma
Kodu değiştirerek deneyin:

**Yeni widget ekleyin:**
```c
const obj_class_t my_widget_class = {
    .name = "my_widget",
    .base_class = &base_obj_class,
    .constructor = my_widget_constructor,
    .draw = my_widget_draw,
};
```

**Daha fazla obje oluşturun:**
```c
obj_t* button3 = obj_create(panel, &button_class);
obj_set_pos(button3, 70, 200, 100, 40);
```

**Area joining parametrelerini değiştirin:**
```c
// lv_refr_join_areas() fonksiyonunda
if(size_joined < (size_i + size_j) * 1.2) {  // %20 tolerans
    // Join yap
}
```

### 4. LVGL Karşılaştırması
LVGL kaynak kodunu (`src/core/lv_refr.c`) açın ve bu demo ile karşılaştırın:

| Demo Fonksiyon | LVGL Fonksiyon | Dosya |
|----------------|----------------|-------|
| `invalidate_area()` | `_lv_inv_area()` | `src/core/lv_refr.c` |
| `refr_join_areas()` | `lv_refr_join_area()` | `src/core/lv_refr.c` |
| `obj_draw_recursive()` | `lv_obj_redraw()` | `src/core/lv_refr.c` |
| `render_pipeline()` | `_lv_disp_refr_timer()` | `src/core/lv_refr.c` |
| `draw_ctx_t` | `lv_draw_ctx_t` | `src/draw/lv_draw.h` |
| `obj_t` | `lv_obj_t` | `src/core/lv_obj.h` |

## 🧩 Design Pattern'lar

Demo'da kullanılan design pattern'lar:

### 1. Strategy Pattern
```c
typedef struct {
    void (*draw_rect)(...);  // Strategy method
    void (*draw_text)(...);  // Strategy method
} draw_ctx_t;

// Backend selection
draw_ctx_init_sw(&ctx);  // SW backend
// veya
draw_ctx_init_gpu(&ctx); // GPU backend (örnek)
```

### 2. Command Pattern
```c
draw_rect_dsc_t dsc;  // Command
dsc.bg_color = ...;
dsc.radius = ...;

ctx->draw_rect(ctx, &dsc, &coords);  // Execute
```

### 3. Template Method Pattern
```c
void obj_draw_recursive(obj_t* obj, ...) {
    // Template:
    // 1. Main draw
    obj->class_p->draw(obj, ctx);

    // 2. Draw children (recursive)
    for(each child) {
        obj_draw_recursive(child, ...);
    }

    // 3. Post draw
}
```

### 4. Composite Pattern
```c
struct obj_t {
    obj_t* parent;      // Composite
    obj_t** children;   // Components
    uint32_t child_count;
};
```

### 5. Observer Pattern
```c
// Subject değişir
obj_set_pos(obj, x, y, w, h);
  └─► invalidate_area(&obj->coords);  // Observer'ları bilgilendir
       └─► render_pipeline();         // Reaction
```

## 📈 Performans Metrikleri

Demo, area joining optimizasyonunun etkisini gösterir:

```
Before join: 2 dirty areas
  Area 1: [70,70 - 169,109]   (size: 4000 pixels)
  Area 2: [200,70 - 299,109]  (size: 4000 pixels)

After join: 1 dirty area
  Joined: [70,70 - 299,109]   (size: 9200 pixels)

Saved: (4000 + 4000) - 9200 = -1200 pixels
(Boşluk çok, join yapma)
```

vs.

```
Before join: 2 dirty areas
  Area 1: [70,70 - 169,109]   (size: 4000 pixels)
  Area 2: [170,70 - 269,109]  (size: 4000 pixels)

After join: 1 dirty area
  Joined: [70,70 - 269,109]   (size: 8000 pixels)

Saved: (4000 + 4000) - 8000 = 0 pixels
(Bitişik, join yap!)
```

## 🔍 Detaylı Kod Analizi

### Invalidation Buffer Overflow Handling

```c
if(g_display.inv_count >= INV_BUF_SIZE) {
    // Buffer doldu - tüm ekranı invalidate et
    g_display.inv_areas[0] = screen_area;
    g_display.inv_count = 1;
}
```

**LVGL'de aynı mantık**: `src/core/lv_refr.c:254-257`

### Area Intersection Optimization

```c
bool area_intersect(area_t* res, const area_t* a1, const area_t* a2)
{
    res->x1 = (a1->x1 > a2->x1) ? a1->x1 : a2->x1;
    res->y1 = (a1->y1 > a2->y1) ? a1->y1 : a2->y1;
    res->x2 = (a1->x2 < a2->x2) ? a1->x2 : a2->x2;
    res->y2 = (a1->y2 < a2->y2) ? a1->y2 : a2->y2;

    return (res->x1 <= res->x2) && (res->y1 <= res->y2);
}
```

**LVGL'de aynı mantık**: `src/misc/lv_area.c`

### Recursive Drawing Flow

```
obj_draw_recursive(screen)
  ├─ draw(screen)
  ├─ obj_draw_recursive(panel)
  │   ├─ draw(panel)
  │   ├─ obj_draw_recursive(button1)
  │   │   └─ draw(button1)
  │   ├─ obj_draw_recursive(button2)
  │   │   └─ draw(button2)
  │   └─ obj_draw_recursive(label)
  │       └─ draw(label)
  └─ post_draw(screen)
```

## 💡 İleri Seviye Konular

### Custom Backend Yazma

```c
void gpu_draw_rect(draw_ctx_t* ctx, const draw_rect_dsc_t* dsc, const area_t* coords)
{
    printf("[GPU BACKEND] Hardware accelerated rectangle\n");
    // DMA2D, PXP, vb. kullanımı buraya
}

void draw_ctx_init_gpu(draw_ctx_t* ctx)
{
    ctx->draw_rect = gpu_draw_rect;  // Override
    ctx->draw_text = sw_draw_text;   // Fallback to SW
}
```

### Double Buffering Simülasyonu

```c
typedef struct {
    void* buf1;
    void* buf2;
    void* buf_act;  // Active buffer
} draw_buffer_t;

void swap_buffers(draw_buffer_t* buf) {
    buf->buf_act = (buf->buf_act == buf->buf1) ? buf->buf2 : buf->buf1;
}
```

### Layout Callback System

```c
typedef void (*layout_cb_t)(obj_t* obj);

typedef struct {
    layout_cb_t cb;
    void* user_data;
} layout_dsc_t;

void flex_layout(obj_t* obj) {
    // Flex layout algorithm
}

obj->layout_dsc = &flex_layout_dsc;
```

## 🐛 Yaygın Hatalar ve Çözümler

### 1. Invalidation Sırasında Render
```c
// ❌ HATALI
void user_action() {
    invalidate_area(&obj->coords);
    render_pipeline();  // Hemen render
    invalidate_area(&obj2->coords);  // Kaybolur!
}

// ✅ DOĞRU
void user_action() {
    invalidate_area(&obj->coords);
    invalidate_area(&obj2->coords);
    render_pipeline();  // Tek seferde render
}
```

### 2. Area Koordinat Hataları
```c
// ❌ HATALI
area_t area = {100, 100, 50, 50};  // x2 < x1

// ✅ DOĞRU
area_t area = {50, 50, 100, 100};  // x1 < x2, y1 < y2
```

### 3. Child'ı Parent'tan Önce Silme
```c
// ❌ HATALI
free(child);
free(parent);  // Parent hala child pointer'ına sahip

// ✅ DOĞRU
// Önce parent'tan çıkar, sonra free et
remove_child(parent, child);
free(child);
```

## 📚 Ek Kaynaklar

- **LVGL Resmi Dökümanları**: https://docs.lvgl.io
- **LVGL Kaynak Kodu**: https://github.com/lvgl/lvgl
- **Bu Projenin Diğer Dökümanları**:
  - `LVGL_WIDGET_ARCHITECTURE.md` - Widget lifecycle
  - `LVGL_RENDER_PIPELINE_ARCHITECTURE.md` - Detaylı render pipeline
  - `LVGL_CORE_DESIGN_PATTERNS.md` - Design patterns

## ✅ Checklist

Demo'yu anladığınızı kontrol edin:

- [ ] Invalidation buffer nasıl çalışıyor?
- [ ] Area joining ne zaman yapılır, ne zaman yapılmaz?
- [ ] Recursive drawing neden depth-first?
- [ ] Backend abstraction nasıl çalışıyor?
- [ ] Object hierarchy nasıl yönetiliyor?
- [ ] Clip area ne işe yarıyor?
- [ ] Draw descriptor pattern neden kullanılıyor?
- [ ] Hangi design pattern'lar var?

## 🎉 Sonuç

Bu demo, LVGL'nin render pipeline mimarisini basitleştirilmiş ama işlevsel bir şekilde gösterir. Gerçek LVGL kodu çok daha karmaşık olsa da, temel prensipler aynıdır.

**Önemli**: Bu bir öğretim aracıdır, production kodda LVGL'nin resmi API'lerini kullanın!

---

**Versiyon**: 1.0
**Tarih**: 2025-11-18
**Yazar**: Claude Code Demo
**Lisans**: MIT (Eğitim amaçlı)
