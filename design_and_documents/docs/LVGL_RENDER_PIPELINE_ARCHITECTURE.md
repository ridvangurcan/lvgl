# LVGL Render Pipeline Mimarisi: Invalidation → Layout → Draw Dispatch → Backend

## İçindekiler
1. [Giriş](#giriş)
2. [Render Pipeline Genel Bakış](#render-pipeline-genel-bakış)
3. [Faz 1: Invalidation (Dirty Region Tracking)](#faz-1-invalidation-dirty-region-tracking)
4. [Faz 2: Layout Calculation](#faz-2-layout-calculation)
5. [Faz 3: Draw Dispatch](#faz-3-draw-dispatch)
6. [Faz 4: Backend Abstraction (GPU/CPU)](#faz-4-backend-abstraction-gpucpu)
7. [Design Pattern'lar](#design-patternlar)
8. [Low-Level Rendering Örnekleri](#low-level-rendering-örnekleri)
9. [Performance Optimizations](#performance-optimizations)

---

## Giriş

Bu doküman, **LVGL v8**'in render pipeline mimarisini low-level detaylarıyla inceler. Rendering süreci 4 ana fazdan oluşur:

```
┌─────────────────┐
│  Invalidation   │  ← Dirty region tracking
└────────┬────────┘
         ↓
┌─────────────────┐
│  Layout Update  │  ← Pozisyon hesaplama
└────────┬────────┘
         ↓
┌─────────────────┐
│  Draw Dispatch  │  ← Çizim emirlerini dağıt
└────────┬────────┘
         ↓
┌─────────────────┐
│  GPU/CPU Backend│  ← Platform-specific rendering
└─────────────────┘
```

**Hedef Kitle**: LVGL rendering mekanizmasını anlamak isteyen geliştiriciler, custom backend yazmak isteyenler, performans optimizasyonu yapacak yazılımcılar.

---

## Render Pipeline Genel Bakış

### Ana Refresh Döngüsü

```c
void _lv_disp_refr_timer(lv_timer_t * tmr)
{
    // 1. LAYOUT UPDATE: Tüm ekranların layout'unu güncelle
    lv_obj_update_layout(disp->act_scr);
    lv_obj_update_layout(disp->prev_scr);
    lv_obj_update_layout(disp->top_layer);
    lv_obj_update_layout(disp->sys_layer);

    // 2. DIRTY AREA JOIN: Çakışan area'ları birleştir
    lv_refr_join_area();

    // 3. RENDERING: Her dirty area için render
    refr_invalid_areas();

    // 4. CLEANUP: Dirty area listesini temizle
    disp->inv_p = 0;
}
```

**Kaynak**: `src/core/lv_refr.c:286-449`

### Pipeline Tetikleyicileri

Rendering pipeline şu durumlarda tetiklenir:

```c
// 1. Periyodik timer (varsayılan: 30ms)
lv_timer_create(_lv_disp_refr_timer, 30, disp);

// 2. Manuel invalidation
lv_obj_invalidate(obj);
  └─► _lv_inv_area(disp, &obj->coords);
       └─► disp->inv_p++;
            └─► lv_timer_resume(disp->refr_timer);

// 3. Stil/pozisyon değişiklikleri
lv_obj_set_pos(obj, x, y);
  └─► lv_obj_invalidate(obj);

lv_obj_add_state(obj, LV_STATE_PRESSED);
  └─► lv_obj_refresh_style(obj, ...);
       └─► lv_obj_invalidate(obj);
```

---

## Faz 1: Invalidation (Dirty Region Tracking)

### 1.1 Dirty Region Sistemi

LVGL, **sadece değişen alanları** yeniden çizer (partial rendering).

#### Display Invalidation State

```c
typedef struct _lv_disp_t {
    // Dirty region buffer
    lv_area_t inv_areas[LV_INV_BUF_SIZE];    // Invalidate edilmiş alanlar (varsayılan: 16)
    uint8_t inv_area_joined[LV_INV_BUF_SIZE]; // Birleştirilmiş area flag'leri
    uint16_t inv_p;                           // Dirty area sayısı

    bool rendering_in_progress;               // Render esnasında yeni invalidation önleme
    // ...
} lv_disp_t;
```

**Kaynak**: `src/hal/lv_hal_disp.h`

#### Invalidation İşlemi

```c
void _lv_inv_area(lv_disp_t * disp, const lv_area_t * area_p)
{
    if(!disp) disp = lv_disp_get_default();
    if(!lv_disp_is_invalidation_enabled(disp)) return;

    // Render esnasında yeni invalidation yasak (data race önleme)
    if(disp->rendering_in_progress) {
        LV_LOG_ERROR("detected modifying dirty areas in render");
        return;
    }

    // NULL parametresi = tüm listeyi temizle
    if(area_p == NULL) {
        disp->inv_p = 0;
        return;
    }

    // 1. Ekran sınırları ile kesişim kontrolü
    lv_area_t scr_area;
    scr_area.x1 = 0;
    scr_area.y1 = 0;
    scr_area.x2 = lv_disp_get_hor_res(disp) - 1;
    scr_area.y2 = lv_disp_get_ver_res(disp) - 1;

    lv_area_t com_area;
    bool suc = _lv_area_intersect(&com_area, area_p, &scr_area);
    if(suc == false) return;  // Ekran dışında

    // 2. Full refresh modu kontrolü
    if(disp->driver->full_refresh) {
        disp->inv_areas[0] = scr_area;  // Tüm ekranı invalidate et
        disp->inv_p = 1;
        if(disp->refr_timer) lv_timer_resume(disp->refr_timer);
        return;
    }

    // 3. Rounder callback (display alignment gereklilikleri)
    // Örnek: Bazı display'ler 8 piksel align gerektirir
    if(disp->driver->rounder_cb) {
        disp->driver->rounder_cb(disp->driver, &com_area);
    }

    // 4. Zaten kapsanmış mı kontrol et
    for(uint16_t i = 0; i < disp->inv_p; i++) {
        if(_lv_area_is_in(&com_area, &disp->inv_areas[i], 0)) {
            return;  // Bu area zaten daha büyük bir dirty area içinde
        }
    }

    // 5. Yeni dirty area kaydet
    if(disp->inv_p < LV_INV_BUF_SIZE) {
        lv_area_copy(&disp->inv_areas[disp->inv_p], &com_area);
    } else {
        // Buffer dolu - tüm ekranı invalidate et
        disp->inv_p = 0;
        lv_area_copy(&disp->inv_areas[disp->inv_p], &scr_area);
    }
    disp->inv_p++;

    // 6. Refresh timer'ı aktive et
    if(disp->refr_timer) lv_timer_resume(disp->refr_timer);
}
```

**Kaynak**: `src/core/lv_refr.c:205-260`

### 1.2 Dirty Area Joining (Optimizasyon)

Çakışan veya yakın area'ları birleştirerek render sayısını azaltır.

```c
static void lv_refr_join_area(void)
{
    uint32_t join_from;
    uint32_t join_in;
    lv_area_t joined_area;

    for(join_in = 0; join_in < disp_refr->inv_p; join_in++) {
        // Zaten birleştirilmiş area'ları atla
        if(disp_refr->inv_area_joined[join_in] != 0) continue;

        // Bu area ile birleştirilebilecek diğer area'ları bul
        for(join_from = 0; join_from < disp_refr->inv_p; join_from++) {
            // Kendisi veya zaten birleştirilmiş area'ları atla
            if(disp_refr->inv_area_joined[join_from] != 0 || join_in == join_from) {
                continue;
            }

            // Çakışma kontrolü (overlap veya adjacent)
            if(_lv_area_is_on(&disp_refr->inv_areas[join_in],
                             &disp_refr->inv_areas[join_from]) == false) {
                continue;
            }

            // İki area'yı birleştir
            _lv_area_join(&joined_area,
                         &disp_refr->inv_areas[join_in],
                         &disp_refr->inv_areas[join_from]);

            // Birleşik area daha küçük mü? (boşluk çok fazla değilse)
            if(lv_area_get_size(&joined_area) <
               (lv_area_get_size(&disp_refr->inv_areas[join_in]) +
                lv_area_get_size(&disp_refr->inv_areas[join_from]))) {

                // join_in'i genişlet
                lv_area_copy(&disp_refr->inv_areas[join_in], &joined_area);

                // join_from'u işaretle (render edilmeyecek)
                disp_refr->inv_area_joined[join_from] = 1;
            }
        }
    }
}
```

**Kaynak**: `src/core/lv_refr.c:475-507`

#### Joining Örneği

```
Öncesi:
┌──────┐     ┌──────┐
│ A1   │     │ A2   │
│ 10px │     │ 12px │
└──────┘     └──────┘
   ↓
Sonrası (join):
┌───────────────┐
│   Joined      │
│   25px        │
└───────────────┘
```

**Karar kriteri**: `joined_size < (A1_size + A2_size)`
- Eğer aralarındaki boşluk çok büyükse birleştirme
- Aksi halde ayrı render et (gereksiz piksel çizimi önlenir)

### 1.3 Invalidation Stratejileri

#### Partial Invalidation

```c
// Sadece değişen kısmı invalidate et
static void update_slider_indicator(lv_obj_t * slider, int32_t old_val, int32_t new_val)
{
    lv_area_t old_indicator_area;
    lv_area_t new_indicator_area;

    calculate_indicator_coords(slider, old_val, &old_indicator_area);
    calculate_indicator_coords(slider, new_val, &new_indicator_area);

    // Sadece farkı invalidate et
    lv_obj_invalidate_area(slider, &old_indicator_area);
    lv_obj_invalidate_area(slider, &new_indicator_area);

    // ❌ Kötü: lv_obj_invalidate(slider);  // Tüm slider'ı yeniden çizer
}
```

#### Full Screen Invalidation

```c
// Tüm ekranı invalidate et (screen transition, theme değişimi vb.)
lv_area_t scr_area = {0, 0, hor_res - 1, ver_res - 1};
_lv_inv_area(disp, &scr_area);
```

---

## Faz 2: Layout Calculation

### 2.1 Layout System Mimarisi

LVGL'de layout, **callback-based strategy pattern** kullanır.

#### Layout Descriptor

```c
typedef void (*lv_layout_update_cb_t)(lv_obj_t * obj, void * user_data);

typedef struct {
    lv_layout_update_cb_t cb;    // Layout callback
    void * user_data;             // Callback verisi
} lv_layout_dsc_t;
```

**Kaynak**: `src/core/lv_obj_pos.h:27-31`

#### Built-in Layout'lar

```c
// Flex layout
void lv_flex_layout(lv_obj_t * cont, void * user_data);

// Grid layout
void lv_grid_layout(lv_obj_t * cont, void * user_data);
```

### 2.2 Layout Update Pipeline

```c
void lv_obj_update_layout(lv_obj_t * obj)
{
    if(obj == NULL) return;

    // 1. Layout invalidation flag kontrolü
    if(!lv_obj_is_layout_positioned(obj)) {
        // Recursive update - child'lara kadar in
        uint32_t i;
        uint32_t child_cnt = lv_obj_get_child_cnt(obj);
        for(i = 0; i < child_cnt; i++) {
            lv_obj_update_layout(obj->spec_attr->children[i]);
        }

        // 2. Bu nesnenin layout callback'ini çağır
        if(obj->layout_dsc && obj->layout_dsc->cb) {
            obj->layout_dsc->cb(obj, obj->layout_dsc->user_data);
        }

        // 3. Layout invalidation flag'ini temizle
        obj->layout_inv = 0;
    }
}
```

### 2.3 Coordinate Calculation

Her nesne, **absolute coordinates** olarak saklanır:

```c
typedef struct _lv_obj_t {
    // ...
    lv_area_t coords;  // Absolute screen coordinates
    // coords.x1, y1: Sol üst köşe
    // coords.x2, y2: Sağ alt köşe
} lv_obj_t;
```

#### Coordinate Calculation Flow

```c
void lv_obj_set_pos(lv_obj_t * obj, lv_coord_t x, lv_coord_t y)
{
    // 1. Eski koordinatları kaydet (invalidation için)
    lv_area_t old_coords;
    lv_area_copy(&old_coords, &obj->coords);

    // 2. Parent'ın content area'sını al
    lv_area_t parent_area;
    lv_obj_get_content_coords(obj->parent, &parent_area);

    // 3. Yeni koordinatları hesapla
    lv_coord_t new_x1 = parent_area.x1 + x;
    lv_coord_t new_y1 = parent_area.y1 + y;
    lv_coord_t new_x2 = new_x1 + lv_obj_get_width(obj) - 1;
    lv_coord_t new_y2 = new_y1 + lv_obj_get_height(obj) - 1;

    obj->coords.x1 = new_x1;
    obj->coords.y1 = new_y1;
    obj->coords.x2 = new_x2;
    obj->coords.y2 = new_y2;

    // 4. Eski ve yeni alanları invalidate et
    lv_obj_invalidate_area(obj, &old_coords);
    lv_obj_invalidate(obj);

    // 5. CHILD_CHANGED event gönder (parent layout update tetikleyebilir)
    lv_event_send(obj->parent, LV_EVENT_CHILD_CHANGED, obj);
}
```

### 2.4 Flex Layout Örneği (Strategy Pattern)

```c
void lv_flex_layout(lv_obj_t * cont, void * user_data)
{
    LV_UNUSED(user_data);

    // 1. Flex parametrelerini al
    lv_flex_flow_t flex_flow = lv_obj_get_style_flex_flow(cont, 0);
    lv_flex_align_t main_place = lv_obj_get_style_flex_main_place(cont, 0);
    lv_flex_align_t cross_place = lv_obj_get_style_flex_cross_place(cont, 0);

    // 2. Child'ları topla
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    lv_obj_t ** children = cont->spec_attr->children;

    // 3. Main axis boyutunu hesapla
    lv_coord_t total_main_size = 0;
    for(uint32_t i = 0; i < child_cnt; i++) {
        if(lv_obj_has_flag(children[i], LV_OBJ_FLAG_IGNORE_LAYOUT)) continue;
        if(lv_obj_has_flag(children[i], LV_OBJ_FLAG_HIDDEN)) continue;

        if(flex_flow == LV_FLEX_FLOW_ROW) {
            total_main_size += lv_obj_get_width(children[i]);
        } else {
            total_main_size += lv_obj_get_height(children[i]);
        }
    }

    // 4. Spacing hesapla
    lv_coord_t gap = lv_obj_get_style_pad_gap(cont, 0);
    total_main_size += gap * (child_cnt - 1);

    // 5. Alignment'a göre başlangıç pozisyonunu hesapla
    lv_area_t cont_content;
    lv_obj_get_content_coords(cont, &cont_content);

    lv_coord_t main_pos;
    if(main_place == LV_FLEX_ALIGN_START) {
        main_pos = (flex_flow == LV_FLEX_FLOW_ROW) ? cont_content.x1 : cont_content.y1;
    }
    else if(main_place == LV_FLEX_ALIGN_END) {
        lv_coord_t cont_main_size = (flex_flow == LV_FLEX_FLOW_ROW) ?
                                    lv_area_get_width(&cont_content) :
                                    lv_area_get_height(&cont_content);
        main_pos = cont_main_size - total_main_size;
    }
    else if(main_place == LV_FLEX_ALIGN_CENTER) {
        lv_coord_t cont_main_size = (flex_flow == LV_FLEX_FLOW_ROW) ?
                                    lv_area_get_width(&cont_content) :
                                    lv_area_get_height(&cont_content);
        main_pos = (cont_main_size - total_main_size) / 2;
    }

    // 6. Her child'ı yerleştir
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = children[i];
        if(lv_obj_has_flag(child, LV_OBJ_FLAG_IGNORE_LAYOUT)) continue;
        if(lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;

        if(flex_flow == LV_FLEX_FLOW_ROW) {
            lv_obj_set_x(child, main_pos);
            main_pos += lv_obj_get_width(child) + gap;
        } else {
            lv_obj_set_y(child, main_pos);
            main_pos += lv_obj_get_height(child) + gap;
        }
    }
}
```

---

## Faz 3: Draw Dispatch

### 3.1 Draw Context: Abstraction Layer

**Draw context**, platform-agnostic bir abstraction layer'dır.

```c
typedef struct _lv_draw_ctx_t {
    // Buffer bilgileri
    void * buf;                      // Draw buffer pointer'ı
    lv_area_t * buf_area;            // Buffer'ın absolute koordinatları
    const lv_area_t * clip_area;     // Clip bölgesi (drawing sınırları)

    // Render mode
    bool render_with_alpha;          // Alpha channel eklenecek mi?
    lv_color_format_t color_format;  // Hedef color format

    // FUNCTION POINTERS (Backend tarafından doldurulur)
    void (*init_buf)(struct _lv_draw_ctx_t * draw_ctx);

    void (*draw_rect)(struct _lv_draw_ctx_t * draw_ctx,
                      const lv_draw_rect_dsc_t * dsc,
                      const lv_area_t * coords);

    void (*draw_arc)(struct _lv_draw_ctx_t * draw_ctx,
                     const lv_draw_arc_dsc_t * dsc,
                     const lv_point_t * center,
                     uint16_t radius,
                     uint16_t start_angle,
                     uint16_t end_angle);

    void (*draw_img_decoded)(struct _lv_draw_ctx_t * draw_ctx,
                            const lv_draw_img_dsc_t * dsc,
                            const lv_area_t * coords,
                            const uint8_t * map_p,
                            lv_img_cf_t color_format);

    lv_res_t (*draw_img)(struct _lv_draw_ctx_t * draw_ctx,
                        const lv_draw_img_dsc_t * draw_dsc,
                        const lv_area_t * coords,
                        const void * src);

    void (*draw_letter)(struct _lv_draw_ctx_t * draw_ctx,
                       const lv_draw_label_dsc_t * dsc,
                       const lv_point_t * pos_p,
                       uint32_t letter);

    void (*draw_line)(struct _lv_draw_ctx_t * draw_ctx,
                     const lv_draw_line_dsc_t * dsc,
                     const lv_point_t * point1,
                     const lv_point_t * point2);

    void (*draw_polygon)(struct _lv_draw_ctx_t * draw_ctx,
                        const lv_draw_rect_dsc_t * draw_dsc,
                        const lv_point_t points[],
                        uint16_t point_cnt);

    void (*draw_bg)(struct _lv_draw_ctx_t * draw_ctx,
                   const lv_draw_rect_dsc_t * draw_dsc,
                   const lv_area_t * coords);

    void (*wait_for_finish)(struct _lv_draw_ctx_t * draw_ctx);

    void (*buffer_copy)(struct _lv_draw_ctx_t * draw_ctx,
                       void * dest_buf, lv_coord_t dest_stride,
                       const lv_area_t * dest_area,
                       void * src_buf, lv_coord_t src_stride,
                       const lv_area_t * src_area);

    void (*buffer_convert)(struct _lv_draw_ctx_t * draw_ctx);

    // Layer support (advanced rendering)
    struct _lv_draw_layer_ctx_t * (*layer_init)(...);
    void (*layer_adjust)(...);
    void (*layer_blend)(...);
    void (*layer_destroy)(...);

} lv_draw_ctx_t;
```

**Kaynak**: `src/draw/lv_draw.h:59-200`

**Bu, Strategy Pattern'in mükemmel bir örneğidir**: Backend, function pointer'ları kendi implementasyonlarıyla doldurur.

### 3.2 Recursive Drawing

```c
void lv_obj_redraw(lv_draw_ctx_t * draw_ctx, lv_obj_t * obj)
{
    const lv_area_t * clip_area_ori = draw_ctx->clip_area;

    // 1. Nesnenin ext_draw_size ile genişletilmiş alanını hesapla
    lv_area_t obj_coords_ext;
    lv_obj_get_coords(obj, &obj_coords_ext);
    lv_coord_t ext_draw_size = _lv_obj_get_ext_draw_size(obj);
    lv_area_increase(&obj_coords_ext, ext_draw_size, ext_draw_size);

    // 2. Clip area ile kesişim kontrolü
    lv_area_t clip_coords_for_obj;
    bool should_draw = _lv_area_intersect(&clip_coords_for_obj,
                                          clip_area_ori,
                                          &obj_coords_ext);

    // OVERFLOW_VISIBLE flag'i varsa clip dışında da çizilmeli
    should_draw = should_draw || lv_obj_has_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    // 3. MAIN DRAWING PHASE
    if(should_draw) {
        draw_ctx->clip_area = &clip_coords_for_obj;

        lv_event_send(obj, LV_EVENT_DRAW_MAIN_BEGIN, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_MAIN, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_MAIN_END, draw_ctx);
    }

    // 4. CHILDREN DRAWING PHASE (Recursive)
    lv_area_t clip_coords_for_children;
    bool refr_children = true;

    if(lv_obj_has_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE)) {
        // Overflow visible: child'lar sınır dışına çıkabilir
        clip_coords_for_children = *clip_area_ori;
    } else {
        // Normal: child'ları nesne içinde clip et
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
            lv_obj_redraw(draw_ctx, child);  // RECURSIVE CALL
        }
    }

    // 5. POST DRAWING PHASE (Overlay, effects)
    if(should_draw) {
        draw_ctx->clip_area = &clip_coords_for_obj;

        lv_event_send(obj, LV_EVENT_DRAW_POST_BEGIN, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_POST, draw_ctx);
        lv_event_send(obj, LV_EVENT_DRAW_POST_END, draw_ctx);
    }

    // 6. Clip area'yı restore et
    draw_ctx->clip_area = clip_area_ori;
}
```

**Kaynak**: `src/core/lv_refr.c:129-196`

### 3.3 Draw Descriptor Pattern

Her çizim primitive'i kendi descriptor'ına sahiptir (Command Pattern).

#### Rectangle Descriptor

```c
typedef struct {
    lv_coord_t radius;             // Köşe yuvarlaklığı
    lv_blend_mode_t blend_mode;    // Blend modu (normal, additive, subtractive)

    // Border
    lv_color_t border_color;
    lv_coord_t border_width;
    lv_opa_t border_opa;           // Opacity (0-255)
    uint8_t border_post : 1;       // Border'ı son çiz (children'dan sonra)
    lv_border_side_t border_side;  // Hangi kenarlar (top, bottom, left, right)

    // Background
    lv_color_t bg_color;
    lv_grad_dsc_t bg_grad;         // Gradient descriptor
    lv_opa_t bg_opa;

    // Background image
    const void * bg_img_src;
    const void * bg_img_symbol_font;
    lv_color_t bg_img_recolor;
    lv_opa_t bg_img_opa;
    lv_opa_t bg_img_recolor_opa;
    uint8_t bg_img_tiled : 1;

    // Shadow
    lv_color_t shadow_color;
    lv_coord_t shadow_width;
    lv_coord_t shadow_ofs_x;
    lv_coord_t shadow_ofs_y;
    lv_coord_t shadow_spread;
    lv_opa_t shadow_opa;

    // Outline
    lv_color_t outline_color;
    lv_coord_t outline_width;
    lv_coord_t outline_pad;
    lv_opa_t outline_opa;
} lv_draw_rect_dsc_t;
```

**Kaynak**: `src/draw/lv_draw_rect.h`

#### Descriptor Kullanımı

```c
// 1. Descriptor initialize et
lv_draw_rect_dsc_t draw_dsc;
lv_draw_rect_dsc_init(&draw_dsc);

// 2. Parametreleri ayarla
draw_dsc.bg_color = lv_color_hex(0x0080FF);
draw_dsc.bg_opa = LV_OPA_COVER;
draw_dsc.radius = 10;
draw_dsc.border_width = 2;
draw_dsc.border_color = lv_color_hex(0xFFFFFF);
draw_dsc.shadow_width = 15;
draw_dsc.shadow_ofs_x = 5;
draw_dsc.shadow_ofs_y = 5;

// 3. Backend'e gönder (dispatch)
lv_area_t rect_area = {10, 10, 110, 60};
draw_ctx->draw_rect(draw_ctx, &draw_dsc, &rect_area);
```

---

## Faz 4: Backend Abstraction (GPU/CPU)

### 4.1 Backend Architecture

LVGL, **pluggable backend** mimarisine sahiptir.

#### Backend Türleri

```
lv_draw_ctx_t (Abstract base)
       ↓
    ┌──┴──┬──────┬──────────┬─────────┐
    │     │      │          │         │
   SW    SDL  STM32-DMA2D  NXP-PXP  ARM2D
  (CPU) (GPU)   (GPU)      (GPU)    (GPU)
```

### 4.2 Software (CPU) Backend

Varsayılan backend - pure C implementation.

```c
typedef struct {
    lv_draw_ctx_t base_draw;  // Base context (inheritance)

    // SW-specific method
    void (*blend)(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc);
} lv_draw_sw_ctx_t;
```

**Kaynak**: `src/draw/sw/lv_draw_sw.h:34-39`

#### SW Backend Initialization

```c
void lv_draw_sw_init_ctx(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
    lv_draw_sw_ctx_t * draw_sw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    // Base draw context initialize et
    lv_memzero(draw_sw_ctx, sizeof(lv_draw_sw_ctx_t));

    // Function pointer'ları SW implementasyonlarıyla doldur
    draw_ctx->init_buf = NULL;  // SW backend buffer init gerektirmez
    draw_ctx->draw_rect = lv_draw_sw_rect;
    draw_ctx->draw_arc = lv_draw_sw_arc;
    draw_ctx->draw_img_decoded = lv_draw_sw_img_decoded;
    draw_ctx->draw_letter = lv_draw_sw_letter;
    draw_ctx->draw_line = lv_draw_sw_line;
    draw_ctx->draw_polygon = lv_draw_sw_polygon;
    draw_ctx->draw_bg = lv_draw_sw_bg;
    draw_ctx->wait_for_finish = lv_draw_sw_wait_for_finish;
    draw_ctx->buffer_copy = lv_draw_sw_buffer_copy;
    draw_ctx->buffer_convert = lv_draw_sw_buffer_convert;
    draw_ctx->layer_init = lv_draw_sw_layer_create;
    draw_ctx->layer_adjust = lv_draw_sw_layer_adjust;
    draw_ctx->layer_blend = lv_draw_sw_layer_blend;
    draw_ctx->layer_destroy = lv_draw_sw_layer_destroy;

    // SW-specific blend method
    draw_sw_ctx->blend = lv_draw_sw_blend_basic;
}
```

**Kaynak**: `src/draw/sw/lv_draw_sw.c`

### 4.3 Low-Level Rectangle Drawing (SW Backend)

```c
void lv_draw_sw_rect(lv_draw_ctx_t * draw_ctx,
                     const lv_draw_rect_dsc_t * dsc,
                     const lv_area_t * coords)
{
    // 1. Clip area ile kesişim kontrolü
    lv_area_t clipped_coords;
    if(!_lv_area_intersect(&clipped_coords, coords, draw_ctx->clip_area)) {
        return;  // Tamamen clip dışında
    }

    // 2. SHADOW çiz (varsa)
    if(dsc->shadow_width > 0 && dsc->shadow_opa > LV_OPA_MIN) {
        draw_shadow(draw_ctx, dsc, coords);
    }

    // 3. BACKGROUND çiz
    if(dsc->bg_opa > LV_OPA_MIN) {
        lv_area_t bg_coords;
        lv_area_copy(&bg_coords, coords);

        // Border kadar içeri çek (border altında kalmasın)
        if(dsc->border_post == 0 && dsc->border_width > 0) {
            bg_coords.x1 += dsc->border_width;
            bg_coords.y1 += dsc->border_width;
            bg_coords.x2 -= dsc->border_width;
            bg_coords.y2 -= dsc->border_width;
        }

        // Gradient varsa
        if(dsc->bg_grad.dir != LV_GRAD_DIR_NONE) {
            draw_bg_gradient(draw_ctx, dsc, &bg_coords);
        } else {
            // Solid color
            draw_bg_color(draw_ctx, dsc, &bg_coords);
        }

        // Background image varsa
        if(dsc->bg_img_src) {
            draw_bg_img(draw_ctx, dsc, &bg_coords);
        }
    }

    // 4. BORDER çiz (post=0 ise önce, post=1 ise sonra)
    if(dsc->border_post == 0 && dsc->border_width > 0 && dsc->border_opa > LV_OPA_MIN) {
        draw_border(draw_ctx, dsc, coords);
    }

    // 5. OUTLINE çiz
    if(dsc->outline_width > 0 && dsc->outline_opa > LV_OPA_MIN) {
        draw_outline(draw_ctx, dsc, coords);
    }

    // 6. POST BORDER çiz (children çizildikten sonra)
    if(dsc->border_post == 1 && dsc->border_width > 0 && dsc->border_opa > LV_OPA_MIN) {
        draw_border(draw_ctx, dsc, coords);
    }
}
```

#### Background Color Fill (Blend)

```c
static void draw_bg_color(lv_draw_ctx_t * draw_ctx,
                         const lv_draw_rect_dsc_t * dsc,
                         const lv_area_t * coords)
{
    lv_draw_sw_ctx_t * draw_sw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    // Blend descriptor hazırla
    lv_draw_sw_blend_dsc_t blend_dsc;
    lv_memzero(&blend_dsc, sizeof(blend_dsc));

    blend_dsc.blend_area = coords;
    blend_dsc.color = dsc->bg_color;
    blend_dsc.opa = dsc->bg_opa;
    blend_dsc.blend_mode = dsc->blend_mode;

    // Radius varsa mask gerektirir
    if(dsc->radius > 0) {
        // Rounded corner için mask oluştur
        lv_draw_mask_radius_param_t mask_param;
        lv_draw_mask_radius_init(&mask_param, coords, dsc->radius, false);
        int16_t mask_id = lv_draw_mask_add(&mask_param, NULL);

        // Mask ile blend
        draw_sw_ctx->blend(draw_ctx, &blend_dsc);

        lv_draw_mask_remove_id(mask_id);
    } else {
        // Direkt blend (mask yok)
        draw_sw_ctx->blend(draw_ctx, &blend_dsc);
    }
}
```

### 4.4 Blending: Pixel-Level Operations

Blending, backend'in en kritik operasyonudur.

```c
void lv_draw_sw_blend_basic(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc)
{
    const lv_area_t * blend_area = dsc->blend_area;
    lv_area_t clipped_area;

    // 1. Clip area ile kesişim
    if(!_lv_area_intersect(&clipped_area, blend_area, draw_ctx->clip_area)) {
        return;
    }

    // 2. Buffer pointer hesapla
    lv_color_t * dest_buf = draw_ctx->buf;
    int32_t dest_stride = lv_area_get_width(draw_ctx->buf_area);

    // Blend area'nın buffer içindeki başlangıç offseti
    int32_t y_offset = clipped_area.y1 - draw_ctx->buf_area->y1;
    int32_t x_offset = clipped_area.x1 - draw_ctx->buf_area->x1;
    dest_buf += y_offset * dest_stride + x_offset;

    // 3. Blend parametreleri
    lv_color_t src_color = dsc->color;
    lv_opa_t opa = dsc->opa;
    lv_blend_mode_t blend_mode = dsc->blend_mode;

    // 4. Boyutlar
    lv_coord_t w = lv_area_get_width(&clipped_area);
    lv_coord_t h = lv_area_get_height(&clipped_area);

    // 5. BLENDING LOOP
    if(opa >= LV_OPA_MAX) {
        // Tam opak - direkt yaz (en hızlı)
        for(int32_t y = 0; y < h; y++) {
            for(int32_t x = 0; x < w; x++) {
                dest_buf[x] = src_color;
            }
            dest_buf += dest_stride;
        }
    }
    else if(opa > LV_OPA_MIN) {
        // Alpha blending gerekiyor
        for(int32_t y = 0; y < h; y++) {
            for(int32_t x = 0; x < w; x++) {
                dest_buf[x] = lv_color_mix(src_color, dest_buf[x], opa);
            }
            dest_buf += dest_stride;
        }
    }
    // opa == LV_OPA_MIN: Tamamen transparent, hiçbir şey yapma
}
```

**Kaynak**: `src/draw/sw/lv_draw_sw_blend.c`

#### Color Mixing (Alpha Blend)

```c
static inline lv_color_t lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix)
{
    lv_color_t ret;

#if LV_COLOR_DEPTH == 16
    // RGB565 format
    ret.ch.red   = LV_UDIV255((uint16_t)c1.ch.red * mix   + c2.ch.red * (255 - mix));
    ret.ch.green = LV_UDIV255((uint16_t)c1.ch.green * mix + c2.ch.green * (255 - mix));
    ret.ch.blue  = LV_UDIV255((uint16_t)c1.ch.blue * mix  + c2.ch.blue * (255 - mix));
#elif LV_COLOR_DEPTH == 32
    // ARGB8888 format
    ret.ch.red   = (uint32_t)((uint32_t)c1.ch.red * mix   + c2.ch.red * (255 - mix)) / 255;
    ret.ch.green = (uint32_t)((uint32_t)c1.ch.green * mix + c2.ch.green * (255 - mix)) / 255;
    ret.ch.blue  = (uint32_t)((uint32_t)c1.ch.blue * mix  + c2.ch.blue * (255 - mix)) / 255;
    ret.ch.alpha = 0xFF;
#endif

    return ret;
}
```

### 4.5 GPU Backend Örneği: STM32 DMA2D

STM32 DMA2D, hardware-accelerated blending sağlar.

```c
void lv_gpu_stm32_dma2d_init_ctx(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
    // SW backend ile başlat
    lv_draw_sw_init_ctx(drv, draw_ctx);

    // GPU-accelerated fonksiyonları override et
    draw_ctx->draw_rect = lv_draw_stm32_dma2d_rect;
    draw_ctx->draw_img_decoded = lv_draw_stm32_dma2d_img_decoded;
    draw_ctx->buffer_copy = lv_draw_stm32_dma2d_buffer_copy;

    // GPU context initialize
    lv_gpu_stm32_dma2d_ctx_t * dma2d_ctx = (lv_gpu_stm32_dma2d_ctx_t *)draw_ctx;
    dma2d_ctx->blend = lv_gpu_stm32_dma2d_blend;
}
```

**Kaynak**: `src/draw/stm32_dma2d/lv_gpu_stm32_dma2d.c`

#### DMA2D Blend (Hardware-Accelerated)

```c
void lv_gpu_stm32_dma2d_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc)
{
    const lv_area_t * blend_area = dsc->blend_area;

    // 1. Hardware register'larını ayarla
    DMA2D->CR = 0;  // Reset

    // 2. Output buffer konfigürasyonu
    DMA2D->OMAR = (uint32_t)draw_ctx->buf;  // Output memory address
    DMA2D->OOR = lv_area_get_width(draw_ctx->buf_area) - lv_area_get_width(blend_area);

    // 3. Foreground (source color) konfigürasyonu
    DMA2D->FGCOLR = dsc->color.full;  // Source color
    DMA2D->FGPFCCR = CM_RGB565;        // Pixel format

    // 4. Alpha blending ayarları
    DMA2D->FGPFCCR |= ((uint32_t)dsc->opa << 24);  // Alpha value

    // 5. Transfer boyutu
    DMA2D->NLR = (lv_area_get_width(blend_area) << 16) | lv_area_get_height(blend_area);

    // 6. DMA2D başlat (blocking mode)
    DMA2D->CR |= DMA2D_CR_START;

    // 7. Transfer tamamlanana kadar bekle
    while(DMA2D->CR & DMA2D_CR_START) {
        // Hardware busy
    }
}
```

**Performans Kazancı**:
- SW blend: ~50 cycles/pixel
- DMA2D blend: ~2 cycles/pixel (25x hızlı!)

### 4.6 Backend Selection

```c
void lv_disp_drv_init(lv_disp_drv_t * driver)
{
    lv_memzero(driver, sizeof(lv_disp_drv_t));

    // Backend seçimi (compile-time)
#if LV_USE_GPU_STM32_DMA2D
    driver->draw_ctx_init = lv_gpu_stm32_dma2d_init_ctx;
    driver->draw_ctx_deinit = lv_gpu_stm32_dma2d_deinit_ctx;
#elif LV_USE_GPU_NXP_PXP
    driver->draw_ctx_init = lv_gpu_nxp_pxp_init_ctx;
    driver->draw_ctx_deinit = lv_gpu_nxp_pxp_deinit_ctx;
#elif LV_USE_GPU_ARM2D
    driver->draw_ctx_init = lv_gpu_arm2d_init_ctx;
    driver->draw_ctx_deinit = lv_gpu_arm2d_deinit_ctx;
#else
    // Varsayılan: SW backend
    driver->draw_ctx_init = lv_draw_sw_init_ctx;
    driver->draw_ctx_deinit = lv_draw_sw_deinit_ctx;
#endif
}
```

---

## Design Pattern'lar

### 1. Strategy Pattern (Backend Abstraction)

```c
// Strategy interface
typedef struct _lv_draw_ctx_t {
    void (*draw_rect)(...);  // Strategy method
    void (*draw_arc)(...);
    void (*draw_img)(...);
    // ...
} lv_draw_ctx_t;

// Concrete strategies
lv_draw_sw_ctx_t     (CPU implementation)
lv_draw_dma2d_ctx_t  (STM32 GPU implementation)
lv_draw_pxp_ctx_t    (NXP GPU implementation)
```

### 2. Command Pattern (Draw Descriptors)

```c
// Command
lv_draw_rect_dsc_t draw_dsc;
draw_dsc.bg_color = ...;
draw_dsc.radius = ...;

// Execute
draw_ctx->draw_rect(draw_ctx, &draw_dsc, &coords);
```

### 3. Template Method Pattern (Recursive Drawing)

```c
lv_obj_redraw(draw_ctx, obj) {
    // Template:
    // 1. Draw main
    draw_main(obj);

    // 2. Draw children (recursive)
    for(each child) {
        lv_obj_redraw(draw_ctx, child);  // Recursive step
    }

    // 3. Draw post
    draw_post(obj);
}
```

### 4. Observer Pattern (Invalidation System)

```c
// Subject invalidate eder
lv_obj_set_pos(obj, x, y);
  └─► lv_obj_invalidate(obj);
       └─► _lv_inv_area(disp, &obj->coords);  // Observer'ları bilgilendir
            └─► lv_timer_resume(disp->refr_timer);  // Refresh trigger
```

### 5. Flyweight Pattern (Area Joining)

Çakışan area'ları birleştirerek hafıza/işlem tasarrufu:

```c
lv_refr_join_area();  // Shared state optimization
```

---

## Low-Level Rendering Örnekleri

### Örnek 1: Custom Draw Backend (Simple Framebuffer)

```c
typedef struct {
    lv_draw_ctx_t base;
    uint16_t * framebuffer;  // Direct framebuffer access
    uint32_t fb_width;
    uint32_t fb_height;
} my_draw_ctx_t;

void my_draw_rect(lv_draw_ctx_t * draw_ctx,
                  const lv_draw_rect_dsc_t * dsc,
                  const lv_area_t * coords)
{
    my_draw_ctx_t * my_ctx = (my_draw_ctx_t *)draw_ctx;

    // Clip kontrolü
    lv_area_t clipped;
    if(!_lv_area_intersect(&clipped, coords, draw_ctx->clip_area)) return;

    // Framebuffer offset hesapla
    uint16_t * fb = my_ctx->framebuffer;
    fb += clipped.y1 * my_ctx->fb_width + clipped.x1;

    // Color convert (LVGL color → RGB565)
    uint16_t color_rgb565 = lv_color_to16(dsc->bg_color);

    // Direct framebuffer write
    for(int y = clipped.y1; y <= clipped.y2; y++) {
        for(int x = clipped.x1; x <= clipped.x2; x++) {
            fb[x - clipped.x1] = color_rgb565;
        }
        fb += my_ctx->fb_width;
    }
}

void my_backend_init(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
    my_draw_ctx_t * my_ctx = (my_draw_ctx_t *)draw_ctx;

    // Function pointer'ları ayarla
    draw_ctx->draw_rect = my_draw_rect;
    draw_ctx->draw_arc = my_draw_arc;
    // ... diğer fonksiyonlar

    // Framebuffer pointer'ı kaydet
    my_ctx->framebuffer = (uint16_t *)drv->draw_buf->buf1;
    my_ctx->fb_width = drv->hor_res;
    my_ctx->fb_height = drv->ver_res;
}
```

### Örnek 2: Gradient Fill (Software)

```c
void draw_vertical_gradient(lv_draw_ctx_t * draw_ctx,
                           const lv_area_t * coords,
                           lv_color_t color_top,
                           lv_color_t color_bottom)
{
    lv_coord_t h = lv_area_get_height(coords);
    lv_coord_t w = lv_area_get_width(coords);

    // Buffer pointer
    lv_color_t * buf = draw_ctx->buf;
    int32_t stride = lv_area_get_width(draw_ctx->buf_area);

    // Offset hesapla
    int32_t y_offset = coords->y1 - draw_ctx->buf_area->y1;
    int32_t x_offset = coords->x1 - draw_ctx->buf_area->x1;
    buf += y_offset * stride + x_offset;

    // Her satır için interpolate
    for(lv_coord_t y = 0; y < h; y++) {
        // Mix ratio (0-255)
        uint8_t mix = (y * 255) / (h - 1);

        // Interpolated color
        lv_color_t line_color = lv_color_mix(color_bottom, color_top, mix);

        // Satırı doldur
        for(lv_coord_t x = 0; x < w; x++) {
            buf[x] = line_color;
        }

        buf += stride;
    }
}
```

### Örnek 3: Circle Drawing (Midpoint Algorithm)

```c
void draw_circle_midpoint(lv_draw_ctx_t * draw_ctx,
                         const lv_point_t * center,
                         lv_coord_t radius,
                         lv_color_t color)
{
    lv_color_t * buf = draw_ctx->buf;
    int32_t stride = lv_area_get_width(draw_ctx->buf_area);

    // Midpoint circle algorithm
    int32_t x = radius;
    int32_t y = 0;
    int32_t err = 0;

    while(x >= y) {
        // 8 octant'a piksel yaz
        put_pixel(buf, stride, center->x + x, center->y + y, color);
        put_pixel(buf, stride, center->x + y, center->y + x, color);
        put_pixel(buf, stride, center->x - y, center->y + x, color);
        put_pixel(buf, stride, center->x - x, center->y + y, color);
        put_pixel(buf, stride, center->x - x, center->y - y, color);
        put_pixel(buf, stride, center->x - y, center->y - x, color);
        put_pixel(buf, stride, center->x + y, center->y - x, color);
        put_pixel(buf, stride, center->x + x, center->y - y, color);

        // Next point
        if(err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if(err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

static inline void put_pixel(lv_color_t * buf, int32_t stride,
                             int32_t x, int32_t y, lv_color_t color)
{
    // Clip kontrolü yapılmalı (basitlik için atlandı)
    buf[y * stride + x] = color;
}
```

### Örnek 4: Anti-Aliased Line (Wu's Algorithm)

```c
void draw_line_antialiased(lv_draw_ctx_t * draw_ctx,
                          const lv_point_t * p1,
                          const lv_point_t * p2,
                          lv_color_t color)
{
    lv_color_t * buf = draw_ctx->buf;
    int32_t stride = lv_area_get_width(draw_ctx->buf_area);

    int32_t dx = p2->x - p1->x;
    int32_t dy = p2->y - p1->y;

    // Wu's line algorithm (simplified)
    if(abs(dx) > abs(dy)) {
        // Horizontal-ish line
        if(p1->x > p2->x) {
            // Swap points
            const lv_point_t * temp = p1;
            p1 = p2;
            p2 = temp;
        }

        float gradient = (float)dy / (float)dx;
        float y = p1->y;

        for(int32_t x = p1->x; x <= p2->x; x++) {
            int32_t yi = (int32_t)y;
            float frac = y - yi;

            // Upper pixel (1 - frac opacity)
            lv_opa_t opa1 = (1.0f - frac) * 255;
            lv_color_t c1 = lv_color_mix(color, buf[yi * stride + x], opa1);
            buf[yi * stride + x] = c1;

            // Lower pixel (frac opacity)
            lv_opa_t opa2 = frac * 255;
            lv_color_t c2 = lv_color_mix(color, buf[(yi + 1) * stride + x], opa2);
            buf[(yi + 1) * stride + x] = c2;

            y += gradient;
        }
    }
    // Vertical-ish line benzer şekilde...
}
```

---

## Performance Optimizations

### 1. Dirty Region Minimization

```c
// ❌ Kötü: Tüm widget'ı invalidate et
lv_obj_invalidate(slider);

// ✅ İyi: Sadece değişen kısmı invalidate et
lv_area_t old_indicator_area;
lv_area_t new_indicator_area;
calculate_indicator_area(slider, old_value, &old_indicator_area);
calculate_indicator_area(slider, new_value, &new_indicator_area);
lv_obj_invalidate_area(slider, &old_indicator_area);
lv_obj_invalidate_area(slider, &new_indicator_area);
```

### 2. Double Buffering

```c
lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[BUFFER_SIZE];
static lv_color_t buf2[BUFFER_SIZE];  // İkinci buffer

lv_disp_draw_buf_init(&draw_buf, buf1, buf2, BUFFER_SIZE);
```

**Avantaj**: Bir buffer render edilirken diğeri display'e gönderilir (paralel işlem).

### 3. DMA Transfer (Async Flush)

```c
void my_flush_cb(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    // DMA transfer başlat (non-blocking)
    start_dma_transfer(color_p, area);

    // HEMEN ready işaretle (DMA arka planda çalışıyor)
    lv_disp_flush_ready(drv);
}

// DMA complete interrupt'ında
void DMA_IRQHandler(void)
{
    // Transfer tamamlandı, bir sonraki buffer hazır
    lv_disp_flush_ready(disp_drv);
}
```

### 4. GPU Acceleration Checklist

```c
// GPU backend seçimi
#define LV_USE_GPU_STM32_DMA2D  1  // STM32
#define LV_USE_GPU_NXP_PXP      0  // NXP i.MX RT
#define LV_USE_GPU_ARM2D        0  // ARM Cortex-M
```

**Accelerated operations**:
- ✅ Solid color fill (DMA2D: 20x hızlı)
- ✅ Image copy/blend (DMA2D: 15x hızlı)
- ✅ Color format conversion
- ❌ Anti-aliasing (genellikle SW)
- ❌ Gradients (genellikle SW)

### 5. Render Budget

```c
// Maksimum render süresi ayarla
void lv_disp_set_render_budget(lv_disp_t * disp, uint32_t budget_ms)
{
    disp->render_budget_ms = budget_ms;
}

// Rendering loop'ta kontrol
if(lv_tick_elaps(render_start) > disp->render_budget_ms) {
    // Budget aşıldı, kalan area'ları sonraki frame'e bırak
    break;
}
```

**30 FPS için**: ~33ms frame time, render budget ~20ms olmalı (input, layout için zaman kalmalı).

---

## Özet

### LVGL Render Pipeline Fazları

| Faz | Fonksiyon | Açıklama | Süre (örnek) |
|-----|-----------|----------|--------------|
| **1. Invalidation** | `_lv_inv_area()` | Dirty region tracking | ~0.1ms |
| **2. Layout** | `lv_obj_update_layout()` | Pozisyon hesaplama | ~1-3ms |
| **3. Draw Dispatch** | `lv_obj_redraw()` | Çizim emirleri oluştur | ~0.5ms |
| **4. Backend Render** | `draw_ctx->draw_*()` | Pixel-level rendering | ~10-20ms |
| **5. Flush** | `flush_cb()` | Display'e gönder | ~5-10ms |

**Toplam**: ~17-35ms (30-60 FPS arası)

### Kritik Dosyalar

| Dosya | Açıklama |
|-------|----------|
| `src/core/lv_refr.c` | Invalidation ve main refresh loop |
| `src/core/lv_obj_pos.c` | Layout calculation |
| `src/draw/lv_draw.h` | Draw context (abstraction) |
| `src/draw/sw/lv_draw_sw.c` | Software backend |
| `src/draw/sw/lv_draw_sw_blend.c` | Blending operations |
| `src/draw/stm32_dma2d/*` | STM32 GPU backend |
| `src/draw/nxp/pxp/*` | NXP PXP GPU backend |

### Design Pattern Özeti

| Pattern | Kullanım | Konum |
|---------|----------|-------|
| **Strategy** | Backend abstraction | `lv_draw_ctx_t` |
| **Command** | Draw descriptors | `lv_draw_rect_dsc_t` |
| **Template Method** | Recursive drawing | `lv_obj_redraw()` |
| **Observer** | Invalidation system | `_lv_inv_area()` |
| **Flyweight** | Area joining | `lv_refr_join_area()` |

---

**Doküman Versiyonu**: 1.0
**LVGL Versiyonu**: v8.x
**Tarih**: 2025-11-18
**Dil**: Türkçe
