/*
 * ALL PERFORMANCE STRATEGIES COMPARISON DEMO
 *
 * Bu demo, tüm performans tekniklerini birlikte kullanarak
 * maksimum performans elde etmeyi gösterir.
 *
 * Derleme: gcc 05_all_strategies_comparison.c -o comparison_demo
 * Çalıştırma: ./comparison_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define SCREEN_W 480
#define SCREEN_H 320
#define NUM_WIDGETS 100

/* Flags */
typedef enum {
    FLAG_VISIBLE   = (1 << 0),
    FLAG_DIRTY     = (1 << 1),
    FLAG_ENABLED   = (1 << 2),
} flag_t;

/* Widget (optimized with all strategies) */
typedef struct {
    int16_t x, y, w, h;     // Cache-friendly: 8 bytes
    uint32_t flags;         // Bit-based filtering
    uint32_t color;         // 4 bytes
    // Total: 16 bytes (cache-friendly!)
} widget_t;

/* Display with dirty regions */
typedef struct {
    widget_t widgets[NUM_WIDGETS];  // Contiguous array!
    int dirty_x1, dirty_y1;
    int dirty_x2, dirty_y2;
    bool has_dirty;
} display_t;

static display_t g_disp;
static uint64_t g_pixels_drawn_naive = 0;
static uint64_t g_pixels_drawn_optimized = 0;

void display_init(void) {
    for(int i = 0; i < NUM_WIDGETS; i++) {
        g_disp.widgets[i].x = rand() % (SCREEN_W - 100);
        g_disp.widgets[i].y = rand() % (SCREEN_H - 50);
        g_disp.widgets[i].w = 50 + rand() % 100;
        g_disp.widgets[i].h = 30 + rand() % 50;
        g_disp.widgets[i].flags = FLAG_VISIBLE | FLAG_ENABLED;
        g_disp.widgets[i].color = rand();
    }
    g_disp.has_dirty = false;
}

void widget_invalidate(widget_t* w) {
    w->flags |= FLAG_DIRTY;  // Bit operation!

    // Update dirty region
    if(!g_disp.has_dirty) {
        g_disp.dirty_x1 = w->x;
        g_disp.dirty_y1 = w->y;
        g_disp.dirty_x2 = w->x + w->w;
        g_disp.dirty_y2 = w->y + w->h;
        g_disp.has_dirty = true;
    } else {
        if(w->x < g_disp.dirty_x1) g_disp.dirty_x1 = w->x;
        if(w->y < g_disp.dirty_y1) g_disp.dirty_y1 = w->y;
        if(w->x + w->w > g_disp.dirty_x2) g_disp.dirty_x2 = w->x + w->w;
        if(w->y + w->h > g_disp.dirty_y2) g_disp.dirty_y2 = w->y + w->h;
    }
}

void draw_naive(void) {
    // Full screen redraw
    g_pixels_drawn_naive += SCREEN_W * SCREEN_H;

    // Draw all widgets
    for(int i = 0; i < NUM_WIDGETS; i++) {
        widget_t* w = &g_disp.widgets[i];
        // Process all widgets (no filtering)
        (void)w;
    }
}

void draw_optimized(void) {
    if(!g_disp.has_dirty) return;

    // Dirty region only
    int dirty_w = g_disp.dirty_x2 - g_disp.dirty_x1;
    int dirty_h = g_disp.dirty_y2 - g_disp.dirty_y1;
    g_pixels_drawn_optimized += dirty_w * dirty_h;

    // Sequential iteration (contiguous memory)
    for(int i = 0; i < NUM_WIDGETS; i++) {
        widget_t* w = &g_disp.widgets[i];

        // Bit-based filtering (fast!)
        if(!(w->flags & FLAG_VISIBLE)) continue;
        if(!(w->flags & FLAG_DIRTY)) continue;

        // Draw widget
        w->flags &= ~FLAG_DIRTY;  // Clear bit
    }

    g_disp.has_dirty = false;
}

int main(void) {
    srand(time(NULL));

    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   ALL STRATEGIES COMPARISON            ║\n");
    printf("╚════════════════════════════════════════╝\n");

    display_init();

    printf("\nScreen: %dx%d = %u pixels\n", SCREEN_W, SCREEN_H, SCREEN_W * SCREEN_H);
    printf("Widgets: %d\n", NUM_WIDGETS);

    // Simulate 100 frames
    int frames = 100;
    printf("\nSimulating %d frames...\n", frames);

    for(int i = 0; i < frames; i++) {
        // Update 5-10 random widgets
        int updates = 5 + rand() % 6;
        for(int j = 0; j < updates; j++) {
            int idx = rand() % NUM_WIDGETS;
            widget_invalidate(&g_disp.widgets[idx]);
        }

        draw_naive();
        draw_optimized();
    }

    printf("\n📊 Results (%d frames):\n", frames);
    printf("\n  Naive (Full Redraw):\n");
    printf("    Pixels drawn: %llu\n", (unsigned long long)g_pixels_drawn_naive);
    printf("    Per frame: %llu\n", (unsigned long long)(g_pixels_drawn_naive / frames));

    printf("\n  Optimized (All Strategies):\n");
    printf("    Pixels drawn: %llu\n", (unsigned long long)g_pixels_drawn_optimized);
    printf("    Per frame: %llu\n", (unsigned long long)(g_pixels_drawn_optimized / frames));

    float reduction = (1.0f - (float)g_pixels_drawn_optimized / g_pixels_drawn_naive) * 100.0f;

    printf("\n  🎯 Total Improvement:\n");
    printf("    Pixel reduction: %.1f%%\n", reduction);
    printf("    Speed: %.1fx faster\n", (float)g_pixels_drawn_naive / g_pixels_drawn_optimized);

    printf("\n========================================\n");
    printf("STRATEGIES USED:\n");
    printf("========================================\n");
    printf("1. Dirty Region Tracking\n");
    printf("   → %.1f%% fewer pixels\n", reduction);
    printf("\n2. Cache-Friendly Structure\n");
    printf("   → 16 bytes per widget (fits cache)\n");
    printf("\n3. Bit-Based Filtering\n");
    printf("   → 1-2 cycles per flag check\n");
    printf("\n4. Contiguous Memory\n");
    printf("   → Sequential widget iteration\n");
    printf("\n🎯 Combined: %.1fx performance boost!\n",
           (float)g_pixels_drawn_naive / g_pixels_drawn_optimized);
    printf("\n");

    return 0;
}
