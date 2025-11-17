/*
 * GETTER/SETTER PATTERN (ALICI/AYARLAYICI DESENİ) DEMO
 *
 * Bu demo, getter/setter pattern'inin nasıl kullanıldığını ve faydalarını gösterir.
 * LVGL'de yaygın olarak kullanılan bu teknik, nesne özelliklerine kontrollü
 * erişim sağlar.
 *
 * Gösterilen Pattern'ler:
 * 1. Basic get/set - Basit okuma/yazma
 * 2. add/clear pattern - Bayrak ekleme/çıkarma
 * 3. has/is pattern - Boolean sorgular
 * 4. Validation - Geçersiz değerleri reddetme
 * 5. Side effects - Değişikliğin yan etkileri
 * 6. Lazy computation - Gerektiğinde hesaplama
 * 7. Caching - Hesaplama sonuçlarını cache'leme
 *
 * Derleme: gcc 02_getter_setter_pattern_demo.c -o getter_setter_demo
 * Çalıştırma: ./getter_setter_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ============================================================================
 * BAD EXAMPLE: Doğrudan Field Erişimi (YAPILMAMALI)
 * ============================================================================ */

typedef struct {
    int width;
    int height;
    int x, y;
    bool visible;
} widget_bad_t;

void demonstrate_direct_access_problems(void)
{
    printf("========================================\n");
    printf("❌ KÖTÜ ÖRNEK: Doğrudan Field Erişimi\n");
    printf("========================================\n");

    widget_bad_t w;
    w.width = -100;      // ❌ Negatif değer! Validasyon yok
    w.height = 999999;   // ❌ Çok büyük! Sınır kontrolü yok
    w.x = 0;
    w.y = 0;
    w.visible = true;

    printf("Problemler:\n");
    printf("  ❌ Negatif width kabul edildi: %d\n", w.width);
    printf("  ❌ Aşırı büyük height kabul edildi: %d\n", w.height);
    printf("  ❌ Validasyon yok\n");
    printf("  ❌ Side effect yok (layout invalidation vs.)\n");
    printf("  ❌ Debugging zor\n");
    printf("  ❌ API değişikliğinde kullanıcı kodu bozulur\n");
    printf("\n");
}

/* ============================================================================
 * GOOD EXAMPLE: Getter/Setter Pattern
 * ============================================================================ */

// Flags (bitwise)
typedef enum {
    WIDGET_FLAG_VISIBLE    = 1 << 0,  // 0x01
    WIDGET_FLAG_ENABLED    = 1 << 1,  // 0x02
    WIDGET_FLAG_CLICKABLE  = 1 << 2,  // 0x04
    WIDGET_FLAG_SCROLLABLE = 1 << 3,  // 0x08
} widget_flag_t;

// State (bitwise)
typedef enum {
    WIDGET_STATE_DEFAULT  = 0,
    WIDGET_STATE_HOVERED  = 1 << 0,  // 0x01
    WIDGET_STATE_PRESSED  = 1 << 1,  // 0x02
    WIDGET_STATE_FOCUSED  = 1 << 2,  // 0x04
    WIDGET_STATE_DISABLED = 1 << 3,  // 0x08
} widget_state_t;

// Widget struct
typedef struct {
    int x, y;
    int width, height;
    uint32_t flags;
    uint32_t state;

    // Lazy computed (cache)
    int cached_area;
    bool area_cache_valid;

    // Side effect tracking
    bool layout_dirty;
    uint32_t modification_count;
} widget_t;

/* ----------------------------------------------------------------------------
 * PATTERN 1: Basic Setter (set_*)
 * ---------------------------------------------------------------------------- */

void widget_init(widget_t* w)
{
    if (!w) return;

    w->x = 0;
    w->y = 0;
    w->width = 100;
    w->height = 50;
    w->flags = WIDGET_FLAG_VISIBLE | WIDGET_FLAG_ENABLED;
    w->state = WIDGET_STATE_DEFAULT;
    w->cached_area = -1;
    w->area_cache_valid = false;
    w->layout_dirty = false;
    w->modification_count = 0;
}

// Setter with validation
void widget_set_width(widget_t* w, int width)
{
    if (!w) return;

    // 1. Validation
    if (width < 0) {
        printf("⚠ Warning: Negative width rejected (%d)\n", width);
        return;
    }
    if (width > 10000) {
        printf("⚠ Warning: Width too large, clamping to 10000\n");
        width = 10000;
    }

    // 2. Set value
    w->width = width;

    // 3. Side effects
    w->area_cache_valid = false;  // Cache invalidate
    w->layout_dirty = true;        // Layout needs update
    w->modification_count++;       // Track changes

    printf("✓ Width set to %d\n", width);
}

void widget_set_height(widget_t* w, int height)
{
    if (!w) return;

    // Validation
    if (height < 0) {
        printf("⚠ Warning: Negative height rejected (%d)\n", height);
        return;
    }
    if (height > 10000) {
        printf("⚠ Warning: Height too large, clamping to 10000\n");
        height = 10000;
    }

    w->height = height;
    w->area_cache_valid = false;
    w->layout_dirty = true;
    w->modification_count++;

    printf("✓ Height set to %d\n", height);
}

// Compound setter (set multiple values at once)
void widget_set_size(widget_t* w, int width, int height)
{
    if (!w) return;
    printf("Setting size to %dx%d:\n", width, height);
    widget_set_width(w, width);   // Reuse validation logic
    widget_set_height(w, height);
}

void widget_set_pos(widget_t* w, int x, int y)
{
    if (!w) return;
    w->x = x;
    w->y = y;
    w->modification_count++;
    printf("✓ Position set to (%d,%d)\n", x, y);
}

/* ----------------------------------------------------------------------------
 * PATTERN 2: Basic Getter (get_*)
 * ---------------------------------------------------------------------------- */

int widget_get_width(const widget_t* w)
{
    return w ? w->width : 0;
}

int widget_get_height(const widget_t* w)
{
    return w ? w->height : 0;
}

void widget_get_pos(const widget_t* w, int* x, int* y)
{
    if (!w) return;
    if (x) *x = w->x;
    if (y) *y = w->y;
}

/* ----------------------------------------------------------------------------
 * PATTERN 3: Lazy Computation Getter
 * ---------------------------------------------------------------------------- */

// Area hesaplaması pahalı bir işlem olabilir, cache'leyelim
int widget_get_area(widget_t* w)
{
    if (!w) return 0;

    // Cache geçerli mi?
    if (w->area_cache_valid) {
        printf("  [Cache hit] Area: %d\n", w->cached_area);
        return w->cached_area;
    }

    // Hesapla ve cache'le
    printf("  [Cache miss] Computing area...\n");
    w->cached_area = w->width * w->height;
    w->area_cache_valid = true;

    return w->cached_area;
}

/* ----------------------------------------------------------------------------
 * PATTERN 4: Flag Operations (add_*, clear_*)
 * LVGL Pattern: lv_obj_add_flag, lv_obj_clear_flag
 * ---------------------------------------------------------------------------- */

void widget_add_flag(widget_t* w, widget_flag_t flag)
{
    if (!w) return;

    uint32_t old_flags = w->flags;
    w->flags |= flag;  // Bitwise OR

    printf("✓ Flag added: 0x%02X (flags: 0x%02X -> 0x%02X)\n",
           flag, old_flags, w->flags);

    // Side effect: VISIBLE flag değişirse redraw gerekir
    if ((flag & WIDGET_FLAG_VISIBLE) && !(old_flags & WIDGET_FLAG_VISIBLE)) {
        printf("  [Side effect] Widget became visible, redraw needed\n");
    }
}

void widget_clear_flag(widget_t* w, widget_flag_t flag)
{
    if (!w) return;

    uint32_t old_flags = w->flags;
    w->flags &= ~flag;  // Bitwise AND NOT

    printf("✓ Flag cleared: 0x%02X (flags: 0x%02X -> 0x%02X)\n",
           flag, old_flags, w->flags);

    // Side effect
    if ((flag & WIDGET_FLAG_VISIBLE) && (old_flags & WIDGET_FLAG_VISIBLE)) {
        printf("  [Side effect] Widget became hidden, redraw needed\n");
    }
}

/* ----------------------------------------------------------------------------
 * PATTERN 5: Boolean Query (has_*, is_*)
 * LVGL Pattern: lv_obj_has_flag, lv_obj_has_state
 * ---------------------------------------------------------------------------- */

// Tüm flagler set mi? (AND semantiği)
bool widget_has_flag(const widget_t* w, widget_flag_t flag)
{
    if (!w) return false;
    return (w->flags & flag) == flag;
}

// Herhangi bir flag set mi? (OR semantiği)
bool widget_has_flag_any(const widget_t* w, widget_flag_t flag)
{
    if (!w) return false;
    return (w->flags & flag) != 0;
}

// Convenience wrappers
bool widget_is_visible(const widget_t* w)
{
    return widget_has_flag(w, WIDGET_FLAG_VISIBLE);
}

bool widget_is_enabled(const widget_t* w)
{
    return widget_has_flag(w, WIDGET_FLAG_ENABLED);
}

bool widget_is_clickable(const widget_t* w)
{
    return widget_has_flag(w, WIDGET_FLAG_CLICKABLE);
}

/* ----------------------------------------------------------------------------
 * PATTERN 6: State Operations (add_state, clear_state, has_state)
 * LVGL Pattern: lv_obj_add_state, lv_obj_clear_state, lv_obj_has_state
 * ---------------------------------------------------------------------------- */

void widget_add_state(widget_t* w, widget_state_t state)
{
    if (!w) return;
    w->state |= state;
    printf("✓ State added: 0x%02X (state: 0x%02X)\n", state, w->state);
}

void widget_clear_state(widget_t* w, widget_state_t state)
{
    if (!w) return;
    w->state &= ~state;
    printf("✓ State cleared: 0x%02X (state: 0x%02X)\n", state, w->state);
}

bool widget_has_state(const widget_t* w, widget_state_t state)
{
    if (!w) return false;
    return (w->state & state) == state;
}

widget_state_t widget_get_state(const widget_t* w)
{
    return w ? w->state : WIDGET_STATE_DEFAULT;
}

/* ----------------------------------------------------------------------------
 * PATTERN 7: Toggle Pattern
 * ---------------------------------------------------------------------------- */

void widget_toggle_flag(widget_t* w, widget_flag_t flag)
{
    if (!w) return;

    if (widget_has_flag(w, flag)) {
        widget_clear_flag(w, flag);
    } else {
        widget_add_flag(w, flag);
    }
}

/* ----------------------------------------------------------------------------
 * Debug & Info
 * ---------------------------------------------------------------------------- */

void widget_print_info(const widget_t* w)
{
    if (!w) return;

    printf("\nWidget Info:\n");
    printf("  Position: (%d, %d)\n", w->x, w->y);
    printf("  Size: %dx%d\n", w->width, w->height);
    printf("  Flags: 0x%02X [", w->flags);
    if (widget_is_visible(w)) printf(" VISIBLE");
    if (widget_is_enabled(w)) printf(" ENABLED");
    if (widget_is_clickable(w)) printf(" CLICKABLE");
    if (widget_has_flag(w, WIDGET_FLAG_SCROLLABLE)) printf(" SCROLLABLE");
    printf(" ]\n");
    printf("  State: 0x%02X [", w->state);
    if (widget_has_state(w, WIDGET_STATE_HOVERED)) printf(" HOVERED");
    if (widget_has_state(w, WIDGET_STATE_PRESSED)) printf(" PRESSED");
    if (widget_has_state(w, WIDGET_STATE_FOCUSED)) printf(" FOCUSED");
    if (widget_has_state(w, WIDGET_STATE_DISABLED)) printf(" DISABLED");
    printf(" ]\n");
    printf("  Modifications: %u\n", w->modification_count);
    printf("  Layout dirty: %s\n", w->layout_dirty ? "Yes" : "No");
}

/* ============================================================================
 * DEMONSTRATIONS
 * ============================================================================ */

void print_separator(const char* title)
{
    printf("\n========================================\n");
    printf("%s\n", title);
    printf("========================================\n");
}

void demonstrate_validation(void)
{
    print_separator("AVANTAJ 1: Validation (Doğrulama)");

    widget_t w;
    widget_init(&w);

    printf("\nGeçersiz değer denemeleri:\n");
    widget_set_width(&w, -50);       // Rejected
    widget_set_height(&w, 999999);   // Clamped
    widget_set_width(&w, 200);       // Accepted

    printf("\n✓ Setter'lar geçersiz değerleri reddeder veya düzeltir\n");
    widget_print_info(&w);
}

void demonstrate_side_effects(void)
{
    print_separator("AVANTAJ 2: Side Effects (Yan Etkiler)");

    widget_t w;
    widget_init(&w);

    printf("\nBoyut değişikliği:\n");
    widget_set_size(&w, 150, 100);

    printf("\nCache invalidation:\n");
    printf("İlk area hesaplaması:\n");
    int area1 = widget_get_area(&w);

    printf("\nİkinci area hesaplaması (cache'den):\n");
    int area2 = widget_get_area(&w);

    printf("\nBoyut değişince cache invalidate olur:\n");
    widget_set_width(&w, 200);
    int area3 = widget_get_area(&w);  // Yeniden hesaplanır

    printf("\n✓ Setter'lar otomatik olarak cache'i invalidate eder\n");
    printf("✓ Layout dirty flag set edilir\n");
}

void demonstrate_flag_operations(void)
{
    print_separator("AVANTAJ 3: Flag Operations");

    widget_t w;
    widget_init(&w);

    printf("\nFlag ekleme:\n");
    widget_add_flag(&w, WIDGET_FLAG_CLICKABLE);
    widget_add_flag(&w, WIDGET_FLAG_SCROLLABLE);

    printf("\nFlag sorgulama:\n");
    printf("  Is clickable? %s\n", widget_is_clickable(&w) ? "Yes" : "No");
    printf("  Is scrollable? %s\n",
           widget_has_flag(&w, WIDGET_FLAG_SCROLLABLE) ? "Yes" : "No");

    printf("\nFlag temizleme:\n");
    widget_clear_flag(&w, WIDGET_FLAG_VISIBLE);

    printf("\nFlag toggle:\n");
    widget_toggle_flag(&w, WIDGET_FLAG_ENABLED);
    widget_toggle_flag(&w, WIDGET_FLAG_ENABLED);

    printf("\nMultiple flag kontrolü:\n");
    uint32_t both = WIDGET_FLAG_CLICKABLE | WIDGET_FLAG_SCROLLABLE;
    printf("  Has CLICKABLE AND SCROLLABLE? %s\n",
           widget_has_flag(&w, both) ? "Yes" : "No");
    printf("  Has CLICKABLE OR SCROLLABLE? %s\n",
           widget_has_flag_any(&w, both) ? "Yes" : "No");

    widget_print_info(&w);
}

void demonstrate_state_management(void)
{
    print_separator("AVANTAJ 4: State Management");

    widget_t w;
    widget_init(&w);

    printf("\nKullanıcı etkileşimi simülasyonu:\n");

    printf("\n1. Mouse hover:\n");
    widget_add_state(&w, WIDGET_STATE_HOVERED);

    printf("\n2. Mouse press:\n");
    widget_add_state(&w, WIDGET_STATE_PRESSED);

    printf("\n3. Mouse release (but still hovering):\n");
    widget_clear_state(&w, WIDGET_STATE_PRESSED);

    printf("\n4. Focus:\n");
    widget_add_state(&w, WIDGET_STATE_FOCUSED);

    printf("\n5. Mouse leave:\n");
    widget_clear_state(&w, WIDGET_STATE_HOVERED);

    printf("\nState sorguları:\n");
    printf("  Is pressed? %s\n",
           widget_has_state(&w, WIDGET_STATE_PRESSED) ? "Yes" : "No");
    printf("  Is focused? %s\n",
           widget_has_state(&w, WIDGET_STATE_FOCUSED) ? "Yes" : "No");

    widget_print_info(&w);
}

void demonstrate_lazy_computation(void)
{
    print_separator("AVANTAJ 5: Lazy Computation");

    widget_t w;
    widget_init(&w);

    printf("\nLazy computation - pahalı hesaplama sadece gerektiğinde yapılır:\n");

    printf("\nİlk get_area çağrısı (cache miss):\n");
    int area1 = widget_get_area(&w);

    printf("\nİkinci get_area çağrısı (cache hit):\n");
    int area2 = widget_get_area(&w);

    printf("\nÜçüncü get_area çağrısı (cache hit):\n");
    int area3 = widget_get_area(&w);

    printf("\nBoyut değiştir (cache invalidate):\n");
    widget_set_width(&w, 300);

    printf("\nDördüncü get_area çağrısı (cache miss, yeniden hesapla):\n");
    int area4 = widget_get_area(&w);

    printf("\n✓ Cache 3 gereksiz hesaplamayı önledi\n");
    printf("✓ Setter otomatik olarak cache'i invalidate etti\n");
}

void demonstrate_api_stability(void)
{
    print_separator("AVANTAJ 6: API Stability");

    printf("Getter/Setter kullanımında:\n");
    printf("✓ İç struct değişse API aynı kalır\n");
    printf("✓ Yeni validasyon kuralları eklenebilir\n");
    printf("✓ Side effect'ler eklenebilir/değiştirilebilir\n");
    printf("✓ Performans optimizasyonları yapılabilir\n");
    printf("\nÖrnek:\n");
    printf("  widget_set_width(w, 100);  // API değişmez\n");
    printf("\n  İmplementasyon değişebilir:\n");
    printf("  - Version 1: Sadece w->width = value\n");
    printf("  - Version 2: + Validation eklenir\n");
    printf("  - Version 3: + Cache invalidation eklenir\n");
    printf("  - Version 4: + Layout invalidation eklenir\n");
    printf("  - Version 5: + Event notification eklenir\n");
    printf("\n  Kullanıcı kodu değişmez! ✓\n");
}

void compare_with_direct_access(void)
{
    print_separator("KARŞILAŞTIRMA: Getter/Setter vs Doğrudan Erişim");

    printf("┌───────────────────────┬──────────────┬──────────────────┐\n");
    printf("│ Özellik               │ Direct Access│ Getter/Setter    │\n");
    printf("├───────────────────────┼──────────────┼──────────────────┤\n");
    printf("│ Validation            │ ❌           │ ✅               │\n");
    printf("│ Side Effects          │ ❌           │ ✅               │\n");
    printf("│ Lazy Computation      │ ❌           │ ✅               │\n");
    printf("│ Caching               │ ❌           │ ✅               │\n");
    printf("│ Debugging             │ ❌           │ ✅               │\n");
    printf("│ API Stability         │ ❌           │ ✅               │\n");
    printf("│ Thread Safety (add)   │ ❌           │ ✅               │\n");
    printf("│ Write Speed           │ ⭐⭐⭐⭐⭐    │ ⭐⭐⭐⭐          │\n");
    printf("│ Kod Boyutu            │ ⭐⭐⭐⭐⭐    │ ⭐⭐⭐           │\n");
    printf("└───────────────────────┴──────────────┴──────────────────┘\n");

    printf("\nDoğrudan Erişim:\n");
    printf("  w.width = 100;  // Hızlı ama güvensiz\n");
    printf("  - Validasyon yok\n");
    printf("  - Cache invalidation yok\n");
    printf("  - Side effect yok\n");

    printf("\nGetter/Setter:\n");
    printf("  widget_set_width(w, 100);  // Biraz yavaş ama güvenli\n");
    printf("  + Validasyon var\n");
    printf("  + Cache invalidation var\n");
    printf("  + Side effect var\n");
    printf("  + API stability var\n");
}

void demonstrate_real_world_example(void)
{
    print_separator("GERÇEK DÜNYA ÖRNEĞİ");

    printf("Senaryo: Responsive UI - Pencere boyutu değişince widget'lar yeniden boyutlandırılır\n\n");

    widget_t widgets[3];

    printf("1. Widget'ları oluştur:\n");
    for (int i = 0; i < 3; i++) {
        widget_init(&widgets[i]);
        widget_set_pos(&widgets[i], i * 110, 0);
        widget_set_size(&widgets[i], 100, 50);
        widgets[i].modification_count = 0;  // Reset counter
    }

    printf("\n2. Pencere boyutu değişti - Widget'ları resize et:\n");
    for (int i = 0; i < 3; i++) {
        widget_set_width(&widgets[i], 150);  // Otomatik invalidation
    }

    printf("\n3. Bir widget'ı disable et:\n");
    widget_clear_flag(&widgets[1], WIDGET_FLAG_ENABLED);
    widget_add_state(&widgets[1], WIDGET_STATE_DISABLED);

    printf("\n4. Hover effect:\n");
    widget_add_state(&widgets[0], WIDGET_STATE_HOVERED);

    printf("\n5. Final durumu:\n");
    for (int i = 0; i < 3; i++) {
        printf("\n--- Widget %d ---\n", i);
        widget_print_info(&widgets[i]);
    }

    printf("\n✓ Tüm değişiklikler güvenli ve kontrollü yapıldı\n");
    printf("✓ Side effect'ler otomatik çalıştı\n");
    printf("✓ Validation her adımda yapıldı\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     GETTER/SETTER PATTERN (ALICI/AYARLAYICI DESENİ)       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    demonstrate_direct_access_problems();
    demonstrate_validation();
    demonstrate_side_effects();
    demonstrate_flag_operations();
    demonstrate_state_management();
    demonstrate_lazy_computation();
    demonstrate_api_stability();
    compare_with_direct_access();
    demonstrate_real_world_example();

    print_separator("ÖZET");
    printf("Getter/Setter Pattern:\n");
    printf("✓ Validation - Geçersiz değerleri reddet\n");
    printf("✓ Side Effects - Cache invalidation, layout update\n");
    printf("✓ Lazy Computation - Pahalı hesaplama sadece gerektiğinde\n");
    printf("✓ Caching - Tekrarlanan hesaplamaları önle\n");
    printf("✓ API Stability - İç değişse API değişmez\n");
    printf("✓ Debugging - Breakpoint ve logging kolay\n");
    printf("✓ Thread Safety - Mutex eklenebilir\n");
    printf("\n");
    printf("LVGL'de kullanılan pattern'ler:\n");
    printf("• set_* - Değer ayarlama (lv_obj_set_width)\n");
    printf("• get_* - Değer okuma (lv_obj_get_width)\n");
    printf("• add_* - Flag/state ekleme (lv_obj_add_flag)\n");
    printf("• clear_* - Flag/state temizleme (lv_obj_clear_flag)\n");
    printf("• has_* - Boolean sorgu (lv_obj_has_flag)\n");
    printf("• is_* - Boolean durum (lv_obj_is_visible)\n");
    printf("\n");

    return 0;
}
