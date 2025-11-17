/*
 * DIRTY REGION TRACKING (KİRLİ BÖLGE TAKİBİ) DEMO
 *
 * Bu demo, ekranın sadece değişen (invalid) bölgelerini takip edip
 * sadece o bölgeleri çizme stratejisini gösterir.
 *
 * Gösterilen Konular:
 * 1. Full screen redraw vs dirty regions
 * 2. Invalid area tracking
 * 3. Region merging/coalescing
 * 4. Smart optimization (birleştirme ekonomik mi?)
 * 5. Performans karşılaştırması
 *
 * Derleme: gcc 01_dirty_region_tracking_demo.c -o dirty_demo
 * Çalıştırma: ./dirty_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ============================================================================
 * BASIC TYPES
 * ============================================================================ */

typedef int16_t coord_t;

// Rectangle (area)
typedef struct {
    coord_t x1, y1;  // Top-left
    coord_t x2, y2;  // Bottom-right
} area_t;

// Display
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320
#define INV_BUF_SIZE  32

typedef struct {
    area_t inv_areas[INV_BUF_SIZE];      // Invalid areas
    uint8_t inv_area_joined[INV_BUF_SIZE]; // Joined flags
    uint16_t inv_p;                       // Count
} display_t;

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

typedef struct {
    uint64_t pixels_drawn;
    uint32_t regions_tracked;
    uint32_t regions_merged;
    uint32_t draw_calls;
} stats_t;

static stats_t g_stats_full;
static stats_t g_stats_dirty;

/* ============================================================================
 * AREA UTILITIES
 * ============================================================================ */

// Calculate area size
static inline uint32_t area_get_size(const area_t* a)
{
    if(!a) return 0;
    if(a->x1 > a->x2 || a->y1 > a->y2) return 0;
    return (uint32_t)(a->x2 - a->x1 + 1) * (a->y2 - a->y1 + 1);
}

// Copy area
static inline void area_copy(area_t* dest, const area_t* src)
{
    if(!dest || !src) return;
    *dest = *src;
}

// Check if area is inside another
static bool area_is_in(const area_t* ain, const area_t* aholder)
{
    if(!ain || !aholder) return false;

    return ain->x1 >= aholder->x1 &&
           ain->y1 >= aholder->y1 &&
           ain->x2 <= aholder->x2 &&
           ain->y2 <= aholder->y2;
}

// Check if two areas overlap
static bool area_is_on(const area_t* a1, const area_t* a2)
{
    if(!a1 || !a2) return false;

    if(a1->x1 > a2->x2 || a2->x1 > a1->x2 ||
       a1->y1 > a2->y2 || a2->y1 > a1->y2) {
        return false;
    }
    return true;
}

// Join two areas (bounding box)
static void area_join(area_t* res, const area_t* a1, const area_t* a2)
{
    if(!res || !a1 || !a2) return;

    res->x1 = a1->x1 < a2->x1 ? a1->x1 : a2->x1;
    res->y1 = a1->y1 < a2->y1 ? a1->y1 : a2->y1;
    res->x2 = a1->x2 > a2->x2 ? a1->x2 : a2->x2;
    res->y2 = a1->y2 > a2->y2 ? a1->y2 : a2->y2;
}

// Intersect two areas (clip)
static bool area_intersect(area_t* res, const area_t* a1, const area_t* a2)
{
    if(!res || !a1 || !a2) return false;

    res->x1 = a1->x1 > a2->x1 ? a1->x1 : a2->x1;
    res->y1 = a1->y1 > a2->y1 ? a1->y1 : a2->y1;
    res->x2 = a1->x2 < a2->x2 ? a1->x2 : a2->x2;
    res->y2 = a1->y2 < a2->y2 ? a1->y2 : a2->y2;

    if(res->x1 > res->x2 || res->y1 > res->y2) {
        return false;  // No intersection
    }
    return true;
}

/* ============================================================================
 * DISPLAY MANAGEMENT
 * ============================================================================ */

static display_t g_disp;

void display_init(void)
{
    memset(&g_disp, 0, sizeof(display_t));
}

// Invalidate area (add to dirty regions)
void display_invalidate(const area_t* area)
{
    if(!area) return;

    // 1. Clip to screen
    area_t screen = {0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1};
    area_t clipped;

    if(!area_intersect(&clipped, area, &screen)) {
        return;  // Outside screen
    }

    // 2. Check if already covered
    for(uint16_t i = 0; i < g_disp.inv_p; i++) {
        if(area_is_in(&clipped, &g_disp.inv_areas[i])) {
            return;  // Already covered
        }
    }

    // 3. Add to buffer
    if(g_disp.inv_p < INV_BUF_SIZE) {
        area_copy(&g_disp.inv_areas[g_disp.inv_p], &clipped);
        g_disp.inv_area_joined[g_disp.inv_p] = 0;
        g_disp.inv_p++;
        g_stats_dirty.regions_tracked++;
    }
    else {
        // Buffer full, invalidate full screen
        g_disp.inv_p = 0;
        area_copy(&g_disp.inv_areas[0], &screen);
        g_disp.inv_area_joined[0] = 0;
        g_disp.inv_p = 1;
        printf("⚠ Buffer full, invalidated full screen\n");
    }
}

// Merge overlapping regions
void display_join_regions(void)
{
    for(uint32_t join_in = 0; join_in < g_disp.inv_p; join_in++) {
        if(g_disp.inv_area_joined[join_in] != 0) continue;

        for(uint32_t join_from = 0; join_from < g_disp.inv_p; join_from++) {
            if(g_disp.inv_area_joined[join_from] != 0 || join_in == join_from) {
                continue;
            }

            // Check if overlapping
            if(!area_is_on(&g_disp.inv_areas[join_in],
                          &g_disp.inv_areas[join_from])) {
                continue;
            }

            // Join them
            area_t joined;
            area_join(&joined,
                     &g_disp.inv_areas[join_in],
                     &g_disp.inv_areas[join_from]);

            // Only join if economical
            uint32_t size_separate = area_get_size(&g_disp.inv_areas[join_in]) +
                                     area_get_size(&g_disp.inv_areas[join_from]);
            uint32_t size_joined = area_get_size(&joined);

            if(size_joined < size_separate) {
                area_copy(&g_disp.inv_areas[join_in], &joined);
                g_disp.inv_area_joined[join_from] = 1;
                g_stats_dirty.regions_merged++;
            }
        }
    }
}

// Simulate drawing (count pixels)
void display_draw_regions(void)
{
    for(uint16_t i = 0; i < g_disp.inv_p; i++) {
        if(g_disp.inv_area_joined[i]) continue;

        uint32_t pixels = area_get_size(&g_disp.inv_areas[i]);
        g_stats_dirty.pixels_drawn += pixels;
        g_stats_dirty.draw_calls++;

        printf("  [Draw] Region %d: (%d,%d)-(%d,%d) → %u pixels\n",
               i,
               g_disp.inv_areas[i].x1, g_disp.inv_areas[i].y1,
               g_disp.inv_areas[i].x2, g_disp.inv_areas[i].y2,
               pixels);
    }
}

// Clear invalid areas
void display_clear(void)
{
    g_disp.inv_p = 0;
    memset(g_disp.inv_area_joined, 0, sizeof(g_disp.inv_area_joined));
}

/* ============================================================================
 * FULL SCREEN REDRAW (NAIVE)
 * ============================================================================ */

void full_screen_redraw(void)
{
    uint32_t pixels = SCREEN_WIDTH * SCREEN_HEIGHT;
    g_stats_full.pixels_drawn += pixels;
    g_stats_full.draw_calls++;

    printf("  [Full Redraw] %dx%d → %u pixels\n",
           SCREEN_WIDTH, SCREEN_HEIGHT, pixels);
}

/* ============================================================================
 * DEMONSTRATION SCENARIOS
 * ============================================================================ */

void print_separator(const char* title)
{
    printf("\n");
    printf("========================================\n");
    printf("%s\n", title);
    printf("========================================\n");
}

void scenario_button_click(void)
{
    print_separator("SENARYO 1: Button Tıklama");

    // Button: 100x50 at (50, 100)
    area_t button = {50, 100, 149, 149};

    printf("\nButton değişti: (%d,%d)-(%d,%d)\n",
           button.x1, button.y1, button.x2, button.y2);
    printf("Button boyutu: %dx%d = %u pixels\n",
           button.x2 - button.x1 + 1,
           button.y2 - button.y1 + 1,
           area_get_size(&button));

    printf("\n❌ FULL SCREEN REDRAW:\n");
    full_screen_redraw();

    printf("\n✓ DIRTY REGION TRACKING:\n");
    display_clear();
    display_invalidate(&button);
    display_draw_regions();

    uint32_t full_px = SCREEN_WIDTH * SCREEN_HEIGHT;
    uint32_t dirty_px = area_get_size(&button);
    float saving = (1.0f - (float)dirty_px / full_px) * 100.0f;

    printf("\n📊 Kazanç:\n");
    printf("  Full redraw: %u pixels\n", full_px);
    printf("  Dirty regions: %u pixels\n", dirty_px);
    printf("  Tasarruf: %.1f%%\n", saving);
}

void scenario_slider_drag(void)
{
    print_separator("SENARYO 2: Slider Sürükleme");

    // Slider track: 200x10
    // Knob: 20x30 (moving)
    area_t old_knob = {50, 100, 69, 129};
    area_t new_knob = {100, 100, 119, 129};

    printf("\nSlider knob hareket etti:\n");
    printf("  Eski pozisyon: (%d,%d)-(%d,%d)\n",
           old_knob.x1, old_knob.y1, old_knob.x2, old_knob.y2);
    printf("  Yeni pozisyon: (%d,%d)-(%d,%d)\n",
           new_knob.x1, new_knob.y1, new_knob.x2, new_knob.y2);

    printf("\n❌ FULL SCREEN REDRAW:\n");
    full_screen_redraw();

    printf("\n✓ DIRTY REGION TRACKING:\n");
    display_clear();
    display_invalidate(&old_knob);  // Eski pozisyon temizlenmeli
    display_invalidate(&new_knob);  // Yeni pozisyon çizilmeli
    printf("  %d bölge takip ediliyor\n", g_disp.inv_p);

    display_join_regions();  // Merge if economical
    printf("  %d bölge birleştirildi\n", g_stats_dirty.regions_merged);

    display_draw_regions();

    uint32_t full_px = SCREEN_WIDTH * SCREEN_HEIGHT;
    uint32_t dirty_px = 0;
    for(uint16_t i = 0; i < g_disp.inv_p; i++) {
        if(!g_disp.inv_area_joined[i]) {
            dirty_px += area_get_size(&g_disp.inv_areas[i]);
        }
    }
    float saving = (1.0f - (float)dirty_px / full_px) * 100.0f;

    printf("\n📊 Kazanç:\n");
    printf("  Full redraw: %u pixels\n", full_px);
    printf("  Dirty regions: %u pixels\n", dirty_px);
    printf("  Tasarruf: %.1f%%\n", saving);
}

void scenario_multiple_updates(void)
{
    print_separator("SENARYO 3: Çoklu Güncelleme");

    // Multiple widgets updated
    area_t areas[] = {
        {10, 10, 59, 59},      // Icon
        {70, 10, 169, 59},     // Label
        {10, 70, 109, 119},    // Button 1
        {120, 70, 219, 119},   // Button 2
        {10, 130, 209, 179},   // Progress bar
    };
    int count = sizeof(areas) / sizeof(areas[0]);

    printf("\n%d widget güncellendi:\n", count);
    for(int i = 0; i < count; i++) {
        printf("  %d: (%d,%d)-(%d,%d) → %u pixels\n",
               i + 1,
               areas[i].x1, areas[i].y1,
               areas[i].x2, areas[i].y2,
               area_get_size(&areas[i]));
    }

    printf("\n❌ FULL SCREEN REDRAW:\n");
    full_screen_redraw();

    printf("\n✓ DIRTY REGION TRACKING:\n");
    display_clear();
    for(int i = 0; i < count; i++) {
        display_invalidate(&areas[i]);
    }
    printf("  %d bölge takip ediliyor\n", g_disp.inv_p);

    display_join_regions();
    printf("  %d bölge birleştirildi\n", g_stats_dirty.regions_merged);

    display_draw_regions();

    uint32_t full_px = SCREEN_WIDTH * SCREEN_HEIGHT;
    uint32_t dirty_px = 0;
    for(uint16_t i = 0; i < g_disp.inv_p; i++) {
        if(!g_disp.inv_area_joined[i]) {
            dirty_px += area_get_size(&g_disp.inv_areas[i]);
        }
    }
    float saving = (1.0f - (float)dirty_px / full_px) * 100.0f;

    printf("\n📊 Kazanç:\n");
    printf("  Full redraw: %u pixels\n", full_px);
    printf("  Dirty regions: %u pixels (%d bölge)\n", dirty_px, g_disp.inv_p);
    printf("  Tasarruf: %.1f%%\n", saving);
}

void scenario_smart_merging(void)
{
    print_separator("SENARYO 4: Akıllı Birleştirme");

    // Two close areas
    area_t area1 = {10, 10, 59, 59};   // 50x50 = 2,500 px
    area_t area2 = {70, 10, 119, 59};  // 50x50 = 2,500 px
    // Gap: 10 pixels

    printf("\nİki yakın alan:\n");
    printf("  Alan 1: (%d,%d)-(%d,%d) → %u pixels\n",
           area1.x1, area1.y1, area1.x2, area1.y2,
           area_get_size(&area1));
    printf("  Alan 2: (%d,%d)-(%d,%d) → %u pixels\n",
           area2.x1, area2.y1, area2.x2, area2.y2,
           area_get_size(&area2));

    // Calculate if merging is economical
    area_t merged;
    area_join(&merged, &area1, &area2);

    uint32_t separate = area_get_size(&area1) + area_get_size(&area2);
    uint32_t joined = area_get_size(&merged);

    printf("\nBirleştirme analizi:\n");
    printf("  Ayrı çizim: %u + %u = %u pixels\n",
           area_get_size(&area1), area_get_size(&area2), separate);
    printf("  Birleşmiş: (%d,%d)-(%d,%d) → %u pixels\n",
           merged.x1, merged.y1, merged.x2, merged.y2, joined);

    if(joined < separate) {
        printf("  ✓ Birleştir (ekonomik: %u < %u)\n", joined, separate);
    } else {
        printf("  ❌ Birleştirme (pahalı: %u > %u)\n", joined, separate);
    }

    printf("\n✓ DIRTY REGION TRACKING (Smart Merging):\n");
    display_clear();
    display_invalidate(&area1);
    display_invalidate(&area2);
    printf("  Başlangıç: %d bölge\n", g_disp.inv_p);

    display_join_regions();
    printf("  Birleştirme sonrası: %d bölge\n", g_disp.inv_p - g_stats_dirty.regions_merged);

    display_draw_regions();
}

void benchmark_comparison(void)
{
    print_separator("BENCHMARK: Performans Karşılaştırması");

    // Reset stats
    memset(&g_stats_full, 0, sizeof(stats_t));
    memset(&g_stats_dirty, 0, sizeof(stats_t));

    // Simulate 100 frames with typical updates
    int frames = 100;
    printf("\n%d frame simülasyonu (tipik kullanım):\n", frames);

    for(int i = 0; i < frames; i++) {
        // Full screen approach
        full_screen_redraw();

        // Dirty regions approach
        display_clear();

        // Typical update: 2-3 small widgets
        area_t a1 = {rand() % 400, rand() % 250, 0, 0};
        a1.x2 = a1.x1 + 50;
        a1.y2 = a1.y1 + 50;

        area_t a2 = {rand() % 400, rand() % 250, 0, 0};
        a2.x2 = a2.x1 + 80;
        a2.y2 = a2.y1 + 30;

        display_invalidate(&a1);
        display_invalidate(&a2);
        display_join_regions();

        for(uint16_t j = 0; j < g_disp.inv_p; j++) {
            if(!g_disp.inv_area_joined[j]) {
                g_stats_dirty.pixels_drawn += area_get_size(&g_disp.inv_areas[j]);
                g_stats_dirty.draw_calls++;
            }
        }
    }

    printf("\n📊 Sonuçlar (%d frame):\n", frames);
    printf("\n  Full Screen Redraw:\n");
    printf("    Toplam pixel: %llu\n", (unsigned long long)g_stats_full.pixels_drawn);
    printf("    Draw calls: %u\n", g_stats_full.draw_calls);
    printf("    Avg pixels/frame: %.0f\n",
           (double)g_stats_full.pixels_drawn / frames);

    printf("\n  Dirty Region Tracking:\n");
    printf("    Toplam pixel: %llu\n", (unsigned long long)g_stats_dirty.pixels_drawn);
    printf("    Draw calls: %u\n", g_stats_dirty.draw_calls);
    printf("    Avg pixels/frame: %.0f\n",
           (double)g_stats_dirty.pixels_drawn / frames);
    printf("    Regions tracked: %u\n", g_stats_dirty.regions_tracked);
    printf("    Regions merged: %u\n", g_stats_dirty.regions_merged);

    float reduction = (1.0f - (float)g_stats_dirty.pixels_drawn /
                      g_stats_full.pixels_drawn) * 100.0f;

    printf("\n  🎯 Performans Kazancı:\n");
    printf("    Pixel reduction: %.1f%%\n", reduction);
    printf("    Speed improvement: %.1fx daha hızlı\n",
           (float)g_stats_full.pixels_drawn / g_stats_dirty.pixels_drawn);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    srand(time(NULL));

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   DIRTY REGION TRACKING (KİRLİ BÖLGE TAKİBİ) DEMO         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    printf("\nEkran: %dx%d = %u pixels\n",
           SCREEN_WIDTH, SCREEN_HEIGHT,
           SCREEN_WIDTH * SCREEN_HEIGHT);

    display_init();

    scenario_button_click();
    scenario_slider_drag();
    scenario_multiple_updates();
    scenario_smart_merging();
    benchmark_comparison();

    print_separator("ÖZET");
    printf("Dirty Region Tracking:\n");
    printf("✓ Sadece değişen alanları çiz\n");
    printf("✓ Smart merging ile optimize et\n");
    printf("✓ Tipik kazanç: 80-95%% daha az piksel\n");
    printf("✓ LVGL'de: 32 bölge buffer\n");
    printf("✓ Ekonomik birleştirme stratejisi\n");
    printf("\n");

    return 0;
}
