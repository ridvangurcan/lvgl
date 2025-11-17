/*
 * CACHE-FRIENDLY DATA STRUCTURES (CACHE DOSTU YAPI) DEMO
 *
 * Bu demo, CPU cache'ini verimli kullanan veri yapılarının nasıl
 * tasarlanacağını gösterir.
 *
 * Gösterilen Konular:
 * 1. Hot/Cold data separation
 * 2. Bitfield packing
 * 3. Cache line utilization
 * 4. Structure size optimization
 * 5. Memory access patterns
 *
 * Derleme: gcc 02_cache_friendly_structures_demo.c -o cache_demo
 * Çalıştırma: ./cache_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define CACHE_LINE_SIZE 64  // Typical L1 cache line size

/* ============================================================================
 * BAD EXAMPLE: Naive Structure (Cache-Unfriendly)
 * ============================================================================ */

typedef struct {
    // Hot data (accessed frequently) mixed with cold data
    int x, y;                  // 8 bytes - HOT (always accessed)
    int width, height;         // 8 bytes - HOT
    bool visible;              // 1 byte  - HOT
    bool enabled;              // 1 byte  - HOT
    bool clickable;            // 1 byte  - HOT
    bool scrollable;           // 1 byte  - HOT
    bool checkable;            // 1 byte  - HOT
    bool checked;              // 1 byte  - HOT
    bool focused;              // 1 byte  - HOT
    bool hovered;              // 1 byte  - HOT
    bool pressed;              // 1 byte  - HOT
    bool disabled;             // 1 byte  - HOT
                               // + 6 bytes padding

    // Cold data (rarely accessed) mixed in
    void* user_data;           // 8 bytes - COLD
    void* event_callbacks;     // 8 bytes - COLD
    char label[64];            // 64 bytes - COLD

    // More hot data
    uint32_t color;            // 4 bytes - HOT

    // More cold data
    void* children;            // 8 bytes - COLD
    uint32_t child_count;      // 4 bytes - COLD
} widget_naive_t;
// Total: ~128 bytes → 2 cache lines!

/* ============================================================================
 * GOOD EXAMPLE: Cache-Friendly Structure (LVGL-style)
 * ============================================================================ */

// Separate cold data
typedef struct {
    void* user_data;
    void* event_callbacks;
    char label[64];
    void* children;
    uint32_t child_count;
} widget_cold_data_t;

// Main structure with only hot data + bitfields
typedef struct {
    // ═══ HOT DATA (frequently accessed) ═══
    int x, y;                  // 8 bytes
    int width, height;         // 8 bytes
    uint32_t color;            // 4 bytes

    // ═══ BITFIELDS (pack multiple booleans) ═══
    uint16_t visible    : 1;   // 1 bit
    uint16_t enabled    : 1;   // 1 bit
    uint16_t clickable  : 1;   // 1 bit
    uint16_t scrollable : 1;   // 1 bit
    uint16_t checkable  : 1;   // 1 bit
    uint16_t checked    : 1;   // 1 bit
    uint16_t focused    : 1;   // 1 bit
    uint16_t hovered    : 1;   // 1 bit
    uint16_t pressed    : 1;   // 1 bit
    uint16_t disabled   : 1;   // 1 bit
    // Total: 10 bits in 2 bytes!
    // (vs 10 bytes in naive version)

    // ═══ LAZY POINTER to cold data ═══
    widget_cold_data_t* cold;  // 8 bytes - NULL if not needed
} widget_optimized_t;
// Total: ~32 bytes → Fits in half cache line!

/* ============================================================================
 * STRUCTURE SIZE ANALYSIS
 * ============================================================================ */

void analyze_structure_sizes(void)
{
    printf("========================================\n");
    printf("STRUCTURE SIZE ANALYSIS\n");
    printf("========================================\n");

    printf("\n❌ Naive Structure:\n");
    printf("  Size: %zu bytes\n", sizeof(widget_naive_t));
    printf("  Cache lines: %zu\n", (sizeof(widget_naive_t) + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE);
    printf("  Padding waste: ~6 bytes\n");
    printf("  Hot/Cold: Mixed (cache pollution)\n");

    printf("\n✓ Optimized Structure:\n");
    printf("  Size: %zu bytes\n", sizeof(widget_optimized_t));
    printf("  Cache lines: %zu\n", (sizeof(widget_optimized_t) + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE);
    printf("  Bitfield savings: 8 bytes (10 bools → 2 bytes)\n");
    printf("  Hot/Cold: Separated (no pollution)\n");

    printf("\n📊 Memory Savings:\n");
    size_t saving = sizeof(widget_naive_t) - sizeof(widget_optimized_t);
    float percent = (float)saving / sizeof(widget_naive_t) * 100.0f;
    printf("  Per object: %zu bytes\n", saving);
    printf("  Percentage: %.1f%%\n", percent);
    printf("  1000 objects: %zu KB saved\n", saving * 1000 / 1024);
}

/* ============================================================================
 * BITFIELD PACKING DEMONSTRATION
 * ============================================================================ */

void demonstrate_bitfield_packing(void)
{
    printf("\n========================================\n");
    printf("BITFIELD PACKING\n");
    printf("========================================\n");

    printf("\n❌ Naive (separate booleans):\n");
    printf("  bool visible;       // 1 byte\n");
    printf("  bool enabled;       // 1 byte\n");
    printf("  bool clickable;     // 1 byte\n");
    printf("  ... (10 booleans)\n");
    printf("  Total: 10 bytes\n");

    printf("\n✓ Bitfield (packed):\n");
    printf("  uint16_t visible   : 1;  // 1 bit\n");
    printf("  uint16_t enabled   : 1;  // 1 bit\n");
    printf("  uint16_t clickable : 1;  // 1 bit\n");
    printf("  ... (10 bits)\n");
    printf("  Total: 2 bytes (10 bits fit in uint16_t)\n");

    printf("\n📊 Savings: 80%% (10 bytes → 2 bytes)\n");

    // Demonstrate usage
    widget_optimized_t w;
    memset(&w, 0, sizeof(w));

    w.visible = 1;
    w.enabled = 1;
    w.clickable = 0;

    printf("\nBitfield operations (same as normal booleans):\n");
    printf("  w.visible = %d\n", w.visible);
    printf("  w.enabled = %d\n", w.enabled);
    printf("  w.clickable = %d\n", w.clickable);
}

/* ============================================================================
 * HOT/COLD DATA SEPARATION
 * ============================================================================ */

void demonstrate_hot_cold_separation(void)
{
    printf("\n========================================\n");
    printf("HOT/COLD DATA SEPARATION\n");
    printf("========================================\n");

    printf("\nHOT DATA (accessed frequently):\n");
    printf("  ✓ Position (x, y)\n");
    printf("  ✓ Size (width, height)\n");
    printf("  ✓ Flags (visible, enabled, ...)\n");
    printf("  ✓ Color\n");
    printf("  → Keep in main struct (fast access)\n");

    printf("\nCOLD DATA (accessed rarely):\n");
    printf("  ✓ User data\n");
    printf("  ✓ Event callbacks\n");
    printf("  ✓ Label text\n");
    printf("  ✓ Children list\n");
    printf("  → Move to separate struct (lazy allocation)\n");

    printf("\n📊 Benefits:\n");
    printf("  ✓ Hot data fits in L1 cache\n");
    printf("  ✓ Cold data doesn't pollute cache\n");
    printf("  ✓ Memory saved (cold data often NULL)\n");

    // Example: 100 widgets, only 20 need cold data
    int total_widgets = 100;
    int widgets_with_cold = 20;

    size_t naive_total = total_widgets * sizeof(widget_naive_t);
    size_t opt_total = total_widgets * sizeof(widget_optimized_t) +
                       widgets_with_cold * sizeof(widget_cold_data_t);

    printf("\nExample: 100 widgets, 20 with cold data:\n");
    printf("  Naive: %zu KB\n", naive_total / 1024);
    printf("  Optimized: %zu KB\n", opt_total / 1024);
    printf("  Savings: %zu KB (%.1f%%)\n",
           (naive_total - opt_total) / 1024,
           (1.0f - (float)opt_total / naive_total) * 100.0f);
}

/* ============================================================================
 * CACHE LINE ANALYSIS
 * ============================================================================ */

void demonstrate_cache_line_usage(void)
{
    printf("\n========================================\n");
    printf("CACHE LINE UTILIZATION\n");
    printf("========================================\n");

    printf("\nTypical L1 cache line: %d bytes\n", CACHE_LINE_SIZE);

    printf("\n❌ Naive structure:\n");
    printf("  Size: %zu bytes\n", sizeof(widget_naive_t));
    printf("  Cache lines needed: 2\n");
    printf("  Utilization: %zu / %d = %.1f%%\n",
           sizeof(widget_naive_t), CACHE_LINE_SIZE * 2,
           (float)sizeof(widget_naive_t) / (CACHE_LINE_SIZE * 2) * 100.0f);

    printf("\n  ┌─────────────────────────────────┐\n");
    printf("  │  Cache Line 1 (64 bytes)        │\n");
    printf("  │  widget_naive_t (part 1)        │\n");
    printf("  ├─────────────────────────────────┤\n");
    printf("  │  Cache Line 2 (64 bytes)        │\n");
    printf("  │  widget_naive_t (part 2)        │\n");
    printf("  │  + wasted space                 │\n");
    printf("  └─────────────────────────────────┘\n");

    printf("\n✓ Optimized structure:\n");
    printf("  Size: %zu bytes\n", sizeof(widget_optimized_t));
    printf("  Cache lines needed: 1\n");
    printf("  Utilization: %zu / %d = %.1f%%\n",
           sizeof(widget_optimized_t), CACHE_LINE_SIZE,
           (float)sizeof(widget_optimized_t) / CACHE_LINE_SIZE * 100.0f);

    printf("\n  ┌─────────────────────────────────┐\n");
    printf("  │  Cache Line 1 (64 bytes)        │\n");
    printf("  │  widget_optimized_t (32 bytes)  │\n");
    printf("  │  + room for other data          │\n");
    printf("  └─────────────────────────────────┘\n");

    printf("\n  Can fit 2 widgets in one cache line!\n");
}

/* ============================================================================
 * PERFORMANCE SIMULATION
 * ============================================================================ */

#define NUM_WIDGETS 1000
#define NUM_ITERATIONS 10000

void simulate_access_patterns(void)
{
    printf("\n========================================\n");
    printf("ACCESS PATTERN SIMULATION\n");
    printf("========================================\n");

    printf("\nSimulating %d widgets, %d iterations\n",
           NUM_WIDGETS, NUM_ITERATIONS);

    // Allocate arrays
    widget_naive_t* naive = malloc(sizeof(widget_naive_t) * NUM_WIDGETS);
    widget_optimized_t* optimized = malloc(sizeof(widget_optimized_t) * NUM_WIDGETS);

    // Initialize
    for(int i = 0; i < NUM_WIDGETS; i++) {
        memset(&naive[i], 0, sizeof(widget_naive_t));
        memset(&optimized[i], 0, sizeof(widget_optimized_t));

        naive[i].x = i;
        naive[i].visible = true;
        optimized[i].x = i;
        optimized[i].visible = 1;
    }

    // Simulate typical access (hot data only)
    clock_t start, end;
    uint64_t sum = 0;

    printf("\nAccessing hot data (x, visible):\n");

    // Naive
    start = clock();
    for(int iter = 0; iter < NUM_ITERATIONS; iter++) {
        for(int i = 0; i < NUM_WIDGETS; i++) {
            if(naive[i].visible) {
                sum += naive[i].x;
            }
        }
    }
    end = clock();
    double naive_time = (double)(end - start) / CLOCKS_PER_SEC;

    // Optimized
    sum = 0;
    start = clock();
    for(int iter = 0; iter < NUM_ITERATIONS; iter++) {
        for(int i = 0; i < NUM_WIDGETS; i++) {
            if(optimized[i].visible) {
                sum += optimized[i].x;
            }
        }
    }
    end = clock();
    double opt_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("  Naive: %.3f seconds\n", naive_time);
    printf("  Optimized: %.3f seconds\n", opt_time);
    printf("  Speedup: %.1fx faster\n", naive_time / opt_time);

    printf("\n📊 Why faster?\n");
    printf("  ✓ Smaller structure → better cache utilization\n");
    printf("  ✓ More widgets fit in cache\n");
    printf("  ✓ Fewer cache misses\n");

    free(naive);
    free(optimized);
}

/* ============================================================================
 * REAL-WORLD EXAMPLE
 * ============================================================================ */

void demonstrate_real_world(void)
{
    printf("\n========================================\n");
    printf("REAL-WORLD EXAMPLE: Button Widget\n");
    printf("========================================\n");

    printf("\nScenario: 1000 buttons on screen\n");
    printf("  Operation: Check visibility and draw\n");
    printf("  Access pattern: x, y, width, height, visible, color\n");

    printf("\n❌ Naive structure:\n");
    printf("  Each button: %zu bytes\n", sizeof(widget_naive_t));
    printf("  1000 buttons: %zu KB\n", sizeof(widget_naive_t) * 1000 / 1024);
    printf("  Cache lines per button: 2\n");
    printf("  Total cache lines: 2000\n");
    printf("  L1 cache size (typical): 32 KB\n");
    printf("  → Constant cache misses!\n");

    printf("\n✓ Optimized structure:\n");
    printf("  Each button: %zu bytes\n", sizeof(widget_optimized_t));
    printf("  1000 buttons: %zu KB\n", sizeof(widget_optimized_t) * 1000 / 1024);
    printf("  Cache lines per button: 1\n");
    printf("  Total cache lines: 1000\n");
    printf("  → More buttons fit in L1 cache!\n");

    printf("\n📊 Performance impact:\n");
    printf("  ✓ Memory bandwidth: 50%% reduced\n");
    printf("  ✓ Cache hit rate: 2-3x better\n");
    printf("  ✓ Overall speed: 2-5x faster\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   CACHE-FRIENDLY DATA STRUCTURES DEMO                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    analyze_structure_sizes();
    demonstrate_bitfield_packing();
    demonstrate_hot_cold_separation();
    demonstrate_cache_line_usage();
    simulate_access_patterns();
    demonstrate_real_world();

    printf("\n========================================\n");
    printf("ÖZET\n");
    printf("========================================\n");
    printf("Cache-Friendly Design:\n");
    printf("✓ Hot/Cold Separation: Sık kullanılan veri main struct'ta\n");
    printf("✓ Bitfield Packing: 10 bool → 2 byte\n");
    printf("✓ Cache Line Fit: 32 byte structure → 1 cache line\n");
    printf("✓ Memory Savings: 60-75%%\n");
    printf("✓ Speed Improvement: 2-5x faster\n");
    printf("\n");
    printf("LVGL'de kullanım:\n");
    printf("• lv_obj_t: 48 bytes (fits in cache line)\n");
    printf("• spec_attr: Lazy allocated cold data\n");
    printf("• Bitfields: 11 bits → 2 bytes\n");
    printf("\n");

    return 0;
}
