# LVGL Draw Subsystem Demo

Bu proje, LVGL'nin `/src/draw` modülündeki **mimari**, **tasarım desenleri** ve **çizim tekniklerini** öğretici bir şekilde gösteren masaüstü demo uygulamasıdır.

## 🎯 Amaç

LVGL'nin draw subsystem'inin nasıl çalıştığını **interaktif** ve **görsel** bir şekilde anlamak:

- ✅ Strategy Pattern ile backend switching
- ✅ Factory Pattern ile widget creation
- ✅ State Pattern ile widget states
- ✅ Observer Pattern ile event system
- ✅ Dirty region tracking optimizasyonu
- ✅ Hardware abstraction layer (HAL) simülasyonu
- ✅ CPU vs GPU rendering karşılaştırması

## 📁 Proje Yapısı

```
lvgl_draw_demo/
├── main.py                    # Ana demo uygulaması
│
├── core/                      # Temel sistem (LVGL core benzeri)
│   ├── color.py              # Renk yönetimi (lv_color_t)
│   ├── area.py               # Alan/koordinat (lv_area_t)
│   ├── draw_context.py       # Draw context (lv_draw_ctx_t) - STRATEGY PATTERN
│   └── display.py            # Display HAL (lv_disp_drv_t) - OBSERVER PATTERN
│
├── backends/                  # Rendering backends (STRATEGY PATTERN)
│   ├── backend_sw.py         # Software rendering (CPU)
│   └── backend_gpu.py        # Simulated GPU rendering
│
├── draw/                      # Çizim primitifleri
│   └── primitives.py         # Bresenham, Midpoint Circle, Gradient, vb.
│
└── widgets/                   # Widget sistemi
    ├── widget_base.py        # Base widget (lv_obj_t) - STATE/OBSERVER PATTERN
    ├── button.py             # Button widget - STATE PATTERN demo
    └── label.py              # Label & Panel widgets
```

## 🎨 Tasarım Desenleri

### 1. Strategy Pattern (★★★★★)

**Kullanım:** Backend switching

```python
# DrawContext farklı backend'leri runtime'da kullanabilir
class DrawContext:
    def __init__(self, buffer, buf_area):
        self.draw_rect = None      # Function pointer
        self.draw_line = None
        self.draw_label = None
        # ... 12 more function pointers

# Backend initialization
SoftwareBackend.init_context(draw_ctx)   # CPU rendering
GPUBackend.init_context(draw_ctx)        # GPU rendering

# Kullanım - backend'den bağımsız
draw_ctx.draw_rect(draw_ctx, descriptor, area)
```

**LVGL Karşılığı:** `lv_draw_ctx_t` yapısı, `lv_draw_sw_init_ctx()`, `lv_draw_pxp_init_ctx()` vb.

### 2. Factory Pattern

**Kullanım:** Widget oluşturma

```python
# Widgets kolayca oluşturulur
button = Button(x, y, width, height, "Click Me")
label = Label(x, y, "Hello World")
panel = Panel(x, y, width, height)

# Hierarchy kurulabilir
panel.add_child(button)
panel.add_child(label)
```

**LVGL Karşılığı:** `lv_btn_create()`, `lv_label_create()`, image decoder factory

### 3. State Pattern (★★★★)

**Kullanım:** Widget state yönetimi

```python
class WidgetState:
    DEFAULT = 0
    HOVERED = 16
    PRESSED = 4
    DISABLED = 8

# State değiştirme
button.set_state(WidgetState.PRESSED, True)

# State'e göre görünüm
def get_current_color(self):
    if self.has_state(WidgetState.PRESSED):
        return DARK_BLUE
    elif self.has_state(WidgetState.HOVERED):
        return LIGHT_BLUE
    else:
        return BLUE
```

**LVGL Karşılığı:** `lv_state_t`, `lv_obj_add_state()`, `lv_obj_has_state()`

### 4. Observer Pattern

**Kullanım:** Event system

```python
# Event callback register
button.on_event('click', lambda w, d: print("Clicked!"))
button.on_event('hover', lambda w, d: print("Hovered!"))

# Event trigger
button.trigger_event('click', data=None)
```

**LVGL Karşılığı:** `lv_event_cb_t`, `lv_obj_add_event_cb()`, HAL callbacks

### 5. Composition Over Inheritance

**Kullanım:** GPU backend SW backend'i kullanır

```python
class GPUBackend:
    @staticmethod
    def init_context(draw_ctx):
        # 1. SW backend ile başla
        SoftwareBackend.init_context(draw_ctx)

        # 2. SW fonksiyonlarını backup'la
        draw_ctx._sw_draw_rect = draw_ctx.draw_rect

        # 3. GPU fonksiyonlarını override et
        draw_ctx.draw_rect = GPUBackend.draw_rect
```

**LVGL Karşılığı:** PXP/DMA2D backend'leri SW backend'i extend eder

## 🚀 Çizim Teknikleri

### 1. Bresenham Line Algorithm

```python
def draw_line_bresenham(surface, x0, y0, x1, y1, color):
    """Integer aritmetiği ile düz çizgi çizimi"""
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    err = dx - dy

    while True:
        set_pixel(x0, y0, color)
        if x0 == x1 and y0 == y1: break

        e2 = 2 * err
        if e2 > -dy: err -= dy; x0 += sx
        if e2 < dx:  err += dx; y0 += sy
```

**LVGL:** `lv_draw_sw_line.c`

### 2. Midpoint Circle Algorithm

```python
def draw_circle_midpoint(surface, cx, cy, radius, color):
    """Integer aritmetiği ile çember çizimi"""
    x = radius
    y = 0
    err = 0

    while x >= y:
        # 8 simetrik noktayı çiz
        plot_8_points(cx, cy, x, y, color)

        y += 1
        err += 1 + 2 * y
        if 2 * (err - x) + 1 > 0:
            x -= 1
            err += 1 - 2 * x
```

**LVGL:** `lv_draw_sw_arc.c`

### 3. Gradient Fill

```python
def fill_rect_with_gradient(surface, x, y, w, h, color1, color2, vertical):
    """Renk geçişi ile dolgu"""
    for i in range(h if vertical else w):
        ratio = i / (h if vertical else w)
        r = int(color1.r + (color2.r - color1.r) * ratio)
        g = int(color1.g + (color2.g - color1.g) * ratio)
        b = int(color1.b + (color2.b - color1.b) * ratio)
        # Draw line with interpolated color
```

**LVGL:** `lv_draw_sw_gradient.c` - LRU cache ile optimize edilmiş

### 4. Alpha Blending

```python
def blend_pixel(dest, src_color, opacity):
    """Alpha blending formülü"""
    result = (src * opacity + dest * (255 - opacity)) / 255
    return result
```

**LVGL:** `lv_draw_sw_blend.c` - SIMD optimized

## 🔧 Gerçek Zamanlı Çizim Stratejileri

### 1. Dirty Region Tracking

```python
class InvalidAreaManager:
    """Sadece değişen alanları takip et"""

    def add_area(self, area):
        # Komşu alanları birleştir (join optimization)
        for existing in self.inv_areas:
            if self._should_join(area, existing):
                existing.join(area)
                return

        self.inv_areas.append(area)
```

**Optimizasyon:** Tüm ekranı çizmek yerine sadece değişen piksellerigallery çiz!

**LVGL:** `lv_disp_t->inv_areas[]` (default 32 alan)

### 2. Clipping

```python
# Sadece görünür alanı çiz
if not draw_ctx.is_area_visible(coords):
    return  # Erken çıkış
```

**LVGL:** `lv_draw_ctx_t->clip_area`

### 3. Backend Threshold

```python
# Küçük işlemler için GPU overhead'i yüksek
if area_size < GPU_THRESHOLD:
    cpu_render()  # SW fallback
else:
    gpu_render()  # Hardware acceleration
```

**LVGL:** PXP/DMA2D backend'leri tipik threshold: 5000 pixel

## 🖥️ Hardware Abstraction Layer (HAL)

### Display Driver

```python
class DisplayDriver:
    def __init__(self, width, height):
        self.draw_buf = create_buffer()

        # Callback functions (OBSERVER PATTERN)
        self.flush_cb = None           # Zorunlu
        self.monitor_cb = None         # İsteğe bağlı
        self.render_start_cb = None

    def flush(self, area):
        """Buffer'ı hardware'e flush et"""
        # 1. render_start_cb çağır
        # 2. Buffer → Screen transfer
        # 3. monitor_cb ile performance track et
```

**LVGL Karşılığı:**
- `lv_disp_drv_t`
- `flush_cb` → SPI/DMA transfer
- `monitor_cb` → FPS/performance tracking

## 📊 Backend Karşılaştırması

| Özellik | Software (CPU) | Simulated GPU |
|---------|----------------|---------------|
| **Implementasyon** | Saf Python | Optimized + Simulated delay |
| **Algoritma** | Bresenham, Midpoint | Native Pygame (optimized) |
| **Threshold** | - | 5000 pixel |
| **Fallback** | - | SW backend'e fallback |
| **Async** | Hayır (senkron) | Evet (simulated) |
| **LVGL Benzeri** | `lv_draw_sw` | `lv_draw_pxp`, `lv_draw_dma2d` |

## 🎮 Kullanım

### Gereksinimler

```bash
pip install pygame
```

### Çalıştırma

```bash
cd design_and_documents/lvgl_draw_demo
python main.py
```

### Kontroller

- **SPACE**: Backend değiştir (Software ↔ GPU)
- **Mouse**: Button'larla etkileşim (hover, click)
- **ESC**: Çıkış

## 📈 Performans Metrikleri

Demo gerçek zamanlı olarak şunları gösterir:

- **Frame Count**: Toplam render edilen frame sayısı
- **FPS**: Saniyedeki frame sayısı
- **Draw Stats**: Rectangle, Line, Circle çizim sayıları
- **Render Time**: Ortalama render süresi (ms)
- **Backend**: Aktif rendering backend

## 🧪 Test Edilen Özellikler

### Widget States (STATE PATTERN)

- ✅ Default state
- ✅ Hovered state (mouse over)
- ✅ Pressed state (mouse down)
- ✅ Disabled state

### Çizim Teknikleri

- ✅ Solid fill
- ✅ Gradient fill (vertical/horizontal)
- ✅ Rounded corners (border-radius)
- ✅ Shadow effects
- ✅ Borders
- ✅ Text rendering

### Backend Features

- ✅ Runtime backend switching
- ✅ Threshold-based GPU fallback
- ✅ Async rendering simulation
- ✅ Composition pattern (GPU extends SW)

## 📚 LVGL Konsept Mapping

| Demo Konsept | LVGL Karşılığı | Dosya |
|--------------|----------------|-------|
| `DrawContext` | `lv_draw_ctx_t` | `src/draw/lv_draw.h` |
| `SoftwareBackend` | SW rendering | `src/draw/sw/` |
| `GPUBackend` | PXP/DMA2D/VGLite | `src/draw/nxp/`, `src/draw/stm32_dma2d/` |
| `DisplayDriver` | `lv_disp_drv_t` | `src/hal/lv_hal_disp.h` |
| `Widget` | `lv_obj_t` | `src/core/lv_obj.c` |
| `WidgetState` | `lv_state_t` | `src/core/lv_obj.h` |
| `InvalidAreaManager` | `inv_areas[]` | `src/core/lv_refr.c` |
| `DrawPrimitives` | SW primitives | `src/draw/sw/lv_draw_sw_*.c` |

## 💡 Öğrenme Noktaları

### 1. Strategy Pattern Avantajları

```python
# Backend değişikliği uygulama kodunu etkilemez
draw_ctx.draw_rect(...)  # Aynı API

# Runtime'da değiştirilebilir
SoftwareBackend.init_context(draw_ctx)  # CPU
GPUBackend.init_context(draw_ctx)       # GPU
```

### 2. Dirty Region Optimizasyonu

```python
# Kötü: Her frame'de tüm ekranı çiz
render_full_screen()  # 800x600 = 480,000 pixel

# İyi: Sadece değişen widget'ı çiz
invalidate(button.area)  # 160x50 = 8,000 pixel
# 60x performans kazancı!
```

### 3. State-Driven Rendering

```python
# State değişince otomatik re-render
button.set_state(PRESSED, True)  → invalidate() → redraw()

# Farklı state'ler farklı görünüm
color = get_color_for_state(state)
```

### 4. Composition > Inheritance

```python
# Bad: Inheritance
class GPUBackend(SoftwareBackend):
    pass  # Tightly coupled

# Good: Composition
class GPUBackend:
    def init(draw_ctx):
        SoftwareBackend.init(draw_ctx)  # Reuse
        draw_ctx.draw_rect = gpu_draw_rect  # Override
```

## 🔬 Genişletme Fikirleri

Demo'yu şu şekilde genişletebilirsiniz:

1. **Yeni Widget'lar**:
   - Slider
   - Checkbox
   - Chart
   - Image

2. **Yeni Backend**:
   - OpenGL backend
   - Vulkan backend

3. **Yeni Çizim Teknikleri**:
   - Anti-aliasing
   - Masking (polygon, fade)
   - Image transformation (rotate, zoom)

4. **Performans**:
   - Layer subdivision
   - Multi-threading
   - GPU profiling

## 📖 Referanslar

- [LVGL Documentation](https://docs.lvgl.io/)
- [LVGL Draw Subsystem Architecture](../draw_subsystem_architecture.md)
- [Design Patterns (GoF)](https://en.wikipedia.org/wiki/Design_Patterns)
- [Computer Graphics Algorithms](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm)

## 👨‍💻 Geliştirici Notları

### Backend Ekleme

Yeni backend eklemek için:

1. `backends/backend_mybackend.py` oluştur
2. `init_context()` implement et
3. Function pointer'ları doldur
4. `main.py`'de backend seçim ekle

```python
class MyBackend:
    @staticmethod
    def init_context(draw_ctx):
        draw_ctx.draw_rect = MyBackend.draw_rect
        draw_ctx.draw_line = MyBackend.draw_line
        # ...
```

### Yeni Widget Ekleme

1. `widgets/my_widget.py` oluştur
2. `Widget`'dan inherit et
3. `draw()` fonksiyonunu implement et

```python
class MyWidget(Widget):
    def __init__(self, x, y, width, height):
        super().__init__(x, y, width, height)

    def draw(self, draw_ctx):
        # Çizim kodu
        pass
```

## ⚖️ Lisans

Bu demo educational/öğretici amaçlıdır.
LVGL MIT lisansı altındadır.

---

**Hazırlayan:** Claude AI (Anthropic)
**Tarih:** 2025-11-17
**LVGL Versiyonu:** v8/v9 konseptleri
