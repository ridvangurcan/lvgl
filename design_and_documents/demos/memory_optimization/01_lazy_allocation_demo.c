/**
 * @file 01_lazy_allocation_demo.c
 * @brief Lazy Allocation (Tembel Tahsisat) Stratejisi Demo
 *
 * Bu demo, belleği sadece gerçekten gerektiğinde tahsis etme
 * stratejisini gösterir.
 *
 * KONSEPT:
 * - Temel nesne her zaman küçük (core data)
 * - İleri özellikler isteğe bağlı (optional data)
 * - Optional data sadece kullanılacaksa tahsis edilir
 *
 * COMPILE: gcc 01_lazy_allocation_demo.c -o lazy_demo
 * RUN: ./lazy_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * LAZY ALLOCATION OLMADAN (Baseline)
 * ======================================================================== */

// Tüm özellikler her zaman struct içinde
typedef struct {
    // Temel özellikler (her nesne kullanır)
    int id;
    int x, y;
    int width, height;

    // İleri özellikler (çoğu nesne kullanmaz!)
    char* children[10];        // 80 byte (10 pointer)
    int child_count;           // 4 byte
    void (*event_callbacks[5])(void*);  // 40 byte (5 pointer)
    int event_count;           // 4 byte
    char* tooltip;             // 8 byte
    void* user_data;           // 8 byte

} widget_without_lazy;  // TOPLAM: ~160 byte

/* ========================================================================
 * LAZY ALLOCATION İLE (Optimized)
 * ======================================================================== */

// İleri özellikler ayrı struct'ta
typedef struct {
    char* children[10];        // 80 byte
    int child_count;           // 4 byte
    void (*event_callbacks[5])(void*);  // 40 byte
    int event_count;           // 4 byte
    char* tooltip;             // 8 byte
    void* user_data;           // 8 byte
} widget_extended_attrs;  // TOPLAM: ~144 byte

// Ana widget - sadece temel özellikler
typedef struct {
    // Temel özellikler (her nesne kullanır)
    int id;
    int x, y;
    int width, height;

    // İleri özellikler pointer olarak (lazy allocated)
    widget_extended_attrs* ext;  // 8 byte pointer (içerik henüz yok!)

} widget_with_lazy;  // TOPLAM: ~32 byte (ext NULL ise)

/* ========================================================================
 * LAZY ALLOCATION FONKSİYONLARI
 * ======================================================================== */

/**
 * Extended attributes'u gerektiğinde tahsis et
 */
void widget_allocate_ext(widget_with_lazy* w) {
    if (w->ext != NULL) {
        return;  // Zaten tahsis edilmiş
    }

    printf("  [LAZY] Extended attributes tahsis ediliyor... (+144 byte)\n");
    w->ext = (widget_extended_attrs*)calloc(1, sizeof(widget_extended_attrs));
}

/**
 * Event callback ekle (extended attributes gerektirir)
 */
void widget_add_event(widget_with_lazy* w, void (*callback)(void*)) {
    widget_allocate_ext(w);  // Gerekirse tahsis et

    if (w->ext->event_count < 5) {
        w->ext->event_callbacks[w->ext->event_count++] = callback;
        printf("  [EVENT] Event callback eklendi (toplam: %d)\n", w->ext->event_count);
    }
}

/**
 * Child ekle (extended attributes gerektirir)
 */
void widget_add_child(widget_with_lazy* w, const char* child_name) {
    widget_allocate_ext(w);  // Gerekirse tahsis et

    if (w->ext->child_count < 10) {
        w->ext->children[w->ext->child_count++] = strdup(child_name);
        printf("  [CHILD] Child eklendi: '%s' (toplam: %d)\n",
               child_name, w->ext->child_count);
    }
}

/**
 * Tooltip ekle (extended attributes gerektirir)
 */
void widget_set_tooltip(widget_with_lazy* w, const char* tooltip) {
    widget_allocate_ext(w);  // Gerekirse tahsis et

    w->ext->tooltip = strdup(tooltip);
    printf("  [TOOLTIP] Tooltip ayarlandı: '%s'\n", tooltip);
}

/**
 * Widget oluştur
 */
widget_with_lazy* widget_create(int id, int x, int y, int w, int h) {
    widget_with_lazy* widget = (widget_with_lazy*)calloc(1, sizeof(widget_with_lazy));
    widget->id = id;
    widget->x = x;
    widget->y = y;
    widget->width = w;
    widget->height = h;
    widget->ext = NULL;  // Başlangıçta NULL - lazy!

    printf("\n[CREATE] Widget #%d oluşturuldu (%zu byte)\n", id, sizeof(widget_with_lazy));
    return widget;
}

/**
 * Widget'i serbest bırak
 */
void widget_destroy(widget_with_lazy* w) {
    if (w->ext != NULL) {
        // Extended attributes varsa temizle
        for (int i = 0; i < w->ext->child_count; i++) {
            free(w->ext->children[i]);
        }
        if (w->ext->tooltip) free(w->ext->tooltip);
        free(w->ext);
        printf("[DESTROY] Extended attributes serbest bırakıldı\n");
    }
    free(w);
    printf("[DESTROY] Widget #%d serbest bırakıldı\n\n", w->id);
}

/**
 * Widget bellek kullanımını hesapla
 */
size_t widget_memory_usage(widget_with_lazy* w) {
    size_t base = sizeof(widget_with_lazy);
    size_t ext = (w->ext != NULL) ? sizeof(widget_extended_attrs) : 0;
    return base + ext;
}

/* ========================================================================
 * DEMO CALLBACK
 * ======================================================================== */

void dummy_callback(void* data) {
    printf("    [CALLBACK] Event triggered!\n");
}

/* ========================================================================
 * DEMO SENARYOLARI
 * ======================================================================== */

void demo_simple_widgets() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 1: Basit Widget'lar (Extended özellik YOK)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    // 10 basit label gibi widget'lar
    widget_with_lazy* labels[10];

    for (int i = 0; i < 10; i++) {
        labels[i] = widget_create(i, i*10, i*10, 100, 20);
    }

    // Bellek kullanımı
    size_t total_mem = 0;
    for (int i = 0; i < 10; i++) {
        total_mem += widget_memory_usage(labels[i]);
    }

    printf("\n[SONUÇ] 10 basit widget:\n");
    printf("  - Her biri: %zu byte\n", sizeof(widget_with_lazy));
    printf("  - Toplam: %zu byte\n", total_mem);
    printf("  - Optimizasyonsuz olsaydı: %zu byte\n", 10 * sizeof(widget_without_lazy));
    printf("  - TASARRUF: %zu byte (%%%.0f)\n",
           10 * sizeof(widget_without_lazy) - total_mem,
           ((10.0 * sizeof(widget_without_lazy) - total_mem) /
            (10.0 * sizeof(widget_without_lazy))) * 100);

    // Temizlik
    for (int i = 0; i < 10; i++) {
        widget_destroy(labels[i]);
    }
}

void demo_complex_widget() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 2: Kompleks Widget (Extended özellikler VAR)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    widget_with_lazy* container = widget_create(100, 0, 0, 300, 200);

    printf("\n--- Child'lar ekleniyor ---\n");
    widget_add_child(container, "button1");  // İlk child - ext tahsis edilir!
    widget_add_child(container, "button2");
    widget_add_child(container, "label1");

    printf("\n--- Event callback'ler ekleniyor ---\n");
    widget_add_event(container, dummy_callback);  // ext zaten var, yeniden tahsis etmez
    widget_add_event(container, dummy_callback);

    printf("\n--- Tooltip ekleniyor ---\n");
    widget_set_tooltip(container, "Bu bir container widget");

    size_t mem = widget_memory_usage(container);
    printf("\n[SONUÇ] Kompleks widget:\n");
    printf("  - Base: %zu byte\n", sizeof(widget_with_lazy));
    printf("  - Extended: %zu byte\n", sizeof(widget_extended_attrs));
    printf("  - Toplam: %zu byte\n", mem);

    widget_destroy(container);
}

void demo_mixed_scenario() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 3: Karışık Senaryo (50 simple + 5 complex)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    widget_with_lazy* widgets[55];

    // 50 basit widget
    printf("\n[1/2] 50 basit widget oluşturuluyor...\n");
    for (int i = 0; i < 50; i++) {
        widgets[i] = widget_create(i, 0, 0, 100, 20);
    }

    // 5 kompleks widget
    printf("\n[2/2] 5 kompleks widget oluşturuluyor...\n");
    for (int i = 50; i < 55; i++) {
        widgets[i] = widget_create(i, 0, 0, 200, 100);
        widget_add_child(widgets[i], "child1");
        widget_add_child(widgets[i], "child2");
        widget_add_event(widgets[i], dummy_callback);
    }

    // Bellek analizi
    size_t simple_mem = 0, complex_mem = 0;
    for (int i = 0; i < 50; i++) {
        simple_mem += widget_memory_usage(widgets[i]);
    }
    for (int i = 50; i < 55; i++) {
        complex_mem += widget_memory_usage(widgets[i]);
    }

    size_t total_with_lazy = simple_mem + complex_mem;
    size_t total_without_lazy = 55 * sizeof(widget_without_lazy);

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  BELLEK KULLANIM ANALİZİ\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n[LAZY ALLOCATION İLE]\n");
    printf("  50 basit widget:    %zu byte (avg: %zu byte/widget)\n",
           simple_mem, simple_mem/50);
    printf("  5 kompleks widget:  %zu byte (avg: %zu byte/widget)\n",
           complex_mem, complex_mem/5);
    printf("  TOPLAM:             %zu byte\n", total_with_lazy);

    printf("\n[LAZY ALLOCATION OLMADAN]\n");
    printf("  55 widget:          %zu byte (avg: %zu byte/widget)\n",
           total_without_lazy, sizeof(widget_without_lazy));

    printf("\n[TASARRUF]\n");
    printf("  Fark:               %zu byte\n", total_without_lazy - total_with_lazy);
    printf("  Yüzde:              %.1f%%\n",
           ((double)(total_without_lazy - total_with_lazy) / total_without_lazy) * 100);

    // Temizlik
    for (int i = 0; i < 55; i++) {
        widget_destroy(widgets[i]);
    }
}

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║       LAZY ALLOCATION (TEMBEL TAHSİSAT) DEMO             ║\n");
    printf("║                                                           ║\n");
    printf("║  Konsept: Belleği sadece gerçekten gerektiğinde tahsis et║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    demo_simple_widgets();
    demo_complex_widget();
    demo_mixed_scenario();

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                                   ║\n");
    printf("║                                                           ║\n");
    printf("║  ✓ Basit nesneler minimal bellek kullanır                ║\n");
    printf("║  ✓ Kompleks nesneler sadece gerektiğinde genişler        ║\n");
    printf("║  ✓ Tipik senaryoda %%40-50 bellek tasarrufu               ║\n");
    printf("║  ✓ Ölçeklenebilir: Binlerce basit nesne mümkün           ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
