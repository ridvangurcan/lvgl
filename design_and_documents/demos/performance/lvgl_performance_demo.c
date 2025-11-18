/*
 * LVGL Performance Optimization Demo
 * ===================================
 *
 * Bu demo düşük kaynaklı gömülü cihazlarda LVGL performans optimizasyonlarını
 * gösterir. TAMAMEN STANDALONE - LVGL kütüphanesine bağımlılık YOK!
 *
 * Kapsanan Konular:
 * -----------------
 * 1. Buffer Strategies (Full vs Partial vs Double Partial)
 * 2. Dirty Area Tracking ve Joining
 * 3. Memory Usage Calculation
 * 4. Performance Metrics (FPS, Render Time, Throughput)
 * 5. Optimization Scenarios
 *
 * LVGL Kaynak Referansları:
 * -------------------------
 * - src/core/lv_refr.c - Refresh and dirty area management
 * - src/hal/lv_hal_disp.c - Display buffer management
 * - src/misc/lv_mem.c - Memory pool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/*******************************************************************************
 * CONFIGURATION
 ******************************************************************************/

#define DISPLAY_HOR_RES 320
#define DISPLAY_VER_RES 240
#define COLOR_DEPTH 16  // bits per pixel (RGB565)

#define BYTES_PER_PIXEL (COLOR_DEPTH / 8)
#define DISPLAY_BUFFER_SIZE (DISPLAY_HOR_RES * DISPLAY_VER_RES * BYTES_PER_PIXEL)

/*******************************************************************************
 * BASIC TYPES
 ******************************************************************************/

typedef struct {
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
} area_t;

typedef uint16_t color_t;  // RGB565

/*******************************************************************************
 * AREA UTILITIES
 ******************************************************************************/

// Get area width
static inline int16_t area_get_width(const area_t* area)
{
    return area->x2 - area->x1 + 1;
}

// Get area height
static inline int16_t area_get_height(const area_t* area)
{
    return area->y2 - area->y1 + 1;
}

// Get area size in pixels
static inline uint32_t area_get_size(const area_t* area)
{
    return (uint32_t)area_get_width(area) * area_get_height(area);
}

// Check if two areas overlap
static bool area_is_on(const area_t* a1, const area_t* a2)
{
    if(a1->x1 > a2->x2 || a2->x1 > a1->x2) return false;
    if(a1->y1 > a2->y2 || a2->y1 > a1->y2) return false;
    return true;
}

// Join two areas (bounding box)
static void area_join(area_t* res, const area_t* a1, const area_t* a2)
{
    res->x1 = (a1->x1 < a2->x1) ? a1->x1 : a2->x1;
    res->y1 = (a1->y1 < a2->y1) ? a1->y1 : a2->y1;
    res->x2 = (a1->x2 > a2->x2) ? a1->x2 : a2->x2;
    res->y2 = (a1->y2 > a2->y2) ? a1->y2 : a2->y2;
}

// Intersect two areas
static bool area_intersect(area_t* res, const area_t* a1, const area_t* a2)
{
    res->x1 = (a1->x1 > a2->x1) ? a1->x1 : a2->x1;
    res->y1 = (a1->y1 > a2->y1) ? a1->y1 : a2->y1;
    res->x2 = (a1->x2 < a2->x2) ? a1->x2 : a2->x2;
    res->y2 = (a1->y2 < a2->y2) ? a1->y2 : a2->y2;

    if(res->x1 > res->x2 || res->y1 > res->y2) {
        return false;
    }
    return true;
}

// Print area
static void area_print(const area_t* area, const char* name)
{
    printf("  %s [%d,%d - %d,%d] (%dx%d = %u px)\n",
           name,
           area->x1, area->y1, area->x2, area->y2,
           area_get_width(area), area_get_height(area),
           area_get_size(area));
}

/*******************************************************************************
 * DIRTY AREA MANAGEMENT
 *
 * LVGL only redraws changed areas (dirty areas).
 * Multiple dirty areas can be joined to reduce draw calls.
 *
 * LVGL Ref: src/core/lv_refr.c:133-195 (invalidation)
 *           src/core/lv_refr.c:237-287 (area joining)
 ******************************************************************************/

#define INV_BUF_SIZE 16  // Max dirty areas (LVGL default: 16-32)
#define JOIN_DISTANCE 10  // Join areas if gap < 10 px

typedef struct {
    area_t areas[INV_BUF_SIZE];
    uint32_t count;
} dirty_area_list_t;

// Add dirty area
static void dirty_area_add(dirty_area_list_t* list, const area_t* area)
{
    if(list->count >= INV_BUF_SIZE) {
        printf("  [WARNING] Dirty area buffer full! Invalidating full screen.\n");
        list->count = 1;
        list->areas[0] = (area_t){0, 0, DISPLAY_HOR_RES-1, DISPLAY_VER_RES-1};
        return;
    }

    list->areas[list->count] = *area;
    list->count++;
}

// Join overlapping dirty areas
static void dirty_area_join(dirty_area_list_t* list)
{
    bool joined[INV_BUF_SIZE] = {false};
    uint32_t join_count = 0;

    printf("\n>> Dirty Area Joining:\n");

    for(uint32_t i = 0; i < list->count; i++) {
        if(joined[i]) continue;

        for(uint32_t j = i + 1; j < list->count; j++) {
            if(joined[j]) continue;

            // Check if areas overlap or are close
            if(area_is_on(&list->areas[i], &list->areas[j])) {
                area_t joined_area;
                area_join(&joined_area, &list->areas[i], &list->areas[j]);

                uint32_t size_i = area_get_size(&list->areas[i]);
                uint32_t size_j = area_get_size(&list->areas[j]);
                uint32_t size_joined = area_get_size(&joined_area);

                // Join if total size is not too much larger
                // LVGL uses JOIN_DISTANCE parameter
                if(size_joined < size_i + size_j + JOIN_DISTANCE * JOIN_DISTANCE) {
                    printf("  Joining area %u and %u:\n", i, j);
                    area_print(&list->areas[i], "Area 1");
                    area_print(&list->areas[j], "Area 2");
                    area_print(&joined_area, "Joined");

                    uint32_t extra_pixels = size_joined - size_i - size_j;
                    printf("    Extra pixels: %u (%.1f%% overhead)\n",
                           extra_pixels,
                           100.0 * extra_pixels / (size_i + size_j));
                    printf("    Draw calls: 2 → 1 (50%% reduction)\n");

                    list->areas[i] = joined_area;
                    joined[j] = true;
                    join_count++;
                }
            }
        }
    }

    // Compact array (remove joined areas)
    uint32_t write_idx = 0;
    for(uint32_t read_idx = 0; read_idx < list->count; read_idx++) {
        if(!joined[read_idx]) {
            list->areas[write_idx++] = list->areas[read_idx];
        }
    }
    list->count = write_idx;

    printf("  Total joins: %u, Final area count: %u\n", join_count, list->count);
}

/*******************************************************************************
 * BUFFER STRATEGIES
 ******************************************************************************/

typedef enum {
    BUFFER_STRATEGY_FULL_DOUBLE,     // Two full screen buffers
    BUFFER_STRATEGY_PARTIAL_SINGLE,  // One small partial buffer
    BUFFER_STRATEGY_PARTIAL_DOUBLE,  // Two small partial buffers (best!)
    BUFFER_STRATEGY_DIRECT,          // No buffer (not recommended)
} buffer_strategy_t;

typedef struct {
    buffer_strategy_t strategy;
    uint32_t buffer_size_bytes;  // Per buffer
    uint32_t buffer_count;       // 0, 1, or 2
    uint32_t partial_height;     // For partial buffering (rows)
} buffer_config_t;

// Calculate buffer configuration
static buffer_config_t get_buffer_config(buffer_strategy_t strategy,
                                         uint32_t partial_height)
{
    buffer_config_t config = {0};
    config.strategy = strategy;
    config.partial_height = partial_height;

    switch(strategy) {
        case BUFFER_STRATEGY_FULL_DOUBLE:
            config.buffer_size_bytes = DISPLAY_BUFFER_SIZE;
            config.buffer_count = 2;
            break;

        case BUFFER_STRATEGY_PARTIAL_SINGLE:
            config.buffer_size_bytes = DISPLAY_HOR_RES * partial_height * BYTES_PER_PIXEL;
            config.buffer_count = 1;
            break;

        case BUFFER_STRATEGY_PARTIAL_DOUBLE:
            config.buffer_size_bytes = DISPLAY_HOR_RES * partial_height * BYTES_PER_PIXEL;
            config.buffer_count = 2;
            break;

        case BUFFER_STRATEGY_DIRECT:
            config.buffer_size_bytes = 0;
            config.buffer_count = 0;
            break;
    }

    return config;
}

// Print buffer configuration
static void print_buffer_config(const buffer_config_t* config)
{
    const char* strategy_names[] = {
        "Full Double Buffering",
        "Partial Single Buffering",
        "Partial Double Buffering",
        "Direct Mode (No Buffer)"
    };

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ Buffer Strategy: %s\n", strategy_names[config->strategy]);
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    printf("  Display: %dx%d (%d-bit color)\n",
           DISPLAY_HOR_RES, DISPLAY_VER_RES, COLOR_DEPTH);
    printf("  Full framebuffer size: %u bytes (%.1f KB)\n",
           DISPLAY_BUFFER_SIZE, DISPLAY_BUFFER_SIZE / 1024.0);

    if(config->buffer_count > 0) {
        printf("  Buffer count: %u\n", config->buffer_count);
        printf("  Buffer size: %u bytes (%.1f KB)\n",
               config->buffer_size_bytes,
               config->buffer_size_bytes / 1024.0);
        printf("  Total RAM: %u bytes (%.1f KB)\n",
               config->buffer_size_bytes * config->buffer_count,
               config->buffer_size_bytes * config->buffer_count / 1024.0);

        if(config->strategy != BUFFER_STRATEGY_FULL_DOUBLE) {
            printf("  Partial buffer: %u rows (%.1f%% of screen height)\n",
                   config->partial_height,
                   100.0 * config->partial_height / DISPLAY_VER_RES);
            printf("  RAM savings: %.1fx (vs full double buffer)\n",
                   (float)(DISPLAY_BUFFER_SIZE * 2) /
                   (config->buffer_size_bytes * config->buffer_count));
        }
    } else {
        printf("  No buffer (direct mode)\n");
        printf("  RAM savings: MAXIMUM (but very slow!)\n");
    }
}

/*******************************************************************************
 * PERFORMANCE METRICS
 ******************************************************************************/

typedef struct {
    uint32_t pixels_drawn;
    uint32_t draw_calls;
    uint32_t flush_calls;
    uint32_t render_time_ms;
    float fps;
    float throughput_kpixels_per_sec;
} perf_metrics_t;

// Calculate performance metrics
static void calculate_metrics(perf_metrics_t* metrics,
                              const dirty_area_list_t* dirty_areas,
                              const buffer_config_t* buffer_config,
                              uint32_t render_time_ms)
{
    metrics->pixels_drawn = 0;
    metrics->draw_calls = dirty_areas->count;
    metrics->render_time_ms = render_time_ms;

    // Calculate total pixels
    for(uint32_t i = 0; i < dirty_areas->count; i++) {
        metrics->pixels_drawn += area_get_size(&dirty_areas->areas[i]);
    }

    // Calculate flush calls (depends on buffer strategy)
    if(buffer_config->strategy == BUFFER_STRATEGY_FULL_DOUBLE) {
        // Full buffer: 1 flush per draw call
        metrics->flush_calls = metrics->draw_calls;
    } else if(buffer_config->strategy == BUFFER_STRATEGY_PARTIAL_SINGLE ||
              buffer_config->strategy == BUFFER_STRATEGY_PARTIAL_DOUBLE) {
        // Partial buffer: multiple flushes per dirty area
        metrics->flush_calls = 0;
        for(uint32_t i = 0; i < dirty_areas->count; i++) {
            uint32_t area_height = area_get_height(&dirty_areas->areas[i]);
            uint32_t strips = (area_height + buffer_config->partial_height - 1) /
                            buffer_config->partial_height;
            metrics->flush_calls += strips;
        }
    } else {
        // Direct mode: 1 flush per pixel (extremely slow!)
        metrics->flush_calls = metrics->pixels_drawn;
    }

    // Calculate FPS and throughput
    if(render_time_ms > 0) {
        metrics->fps = 1000.0 / render_time_ms;
        metrics->throughput_kpixels_per_sec =
            (float)metrics->pixels_drawn / render_time_ms;
    } else {
        metrics->fps = 0;
        metrics->throughput_kpixels_per_sec = 0;
    }
}

// Print performance metrics
static void print_metrics(const perf_metrics_t* metrics)
{
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ Performance Metrics\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    printf("  Pixels drawn: %u (%.1f%% of screen)\n",
           metrics->pixels_drawn,
           100.0 * metrics->pixels_drawn / (DISPLAY_HOR_RES * DISPLAY_VER_RES));
    printf("  Draw calls: %u\n", metrics->draw_calls);
    printf("  Flush calls: %u\n", metrics->flush_calls);
    printf("  Render time: %u ms\n", metrics->render_time_ms);
    printf("  FPS: %.1f\n", metrics->fps);
    printf("  Throughput: %.1f kpixels/sec\n", metrics->throughput_kpixels_per_sec);

    // Performance rating
    if(metrics->fps >= 30) {
        printf("  Rating: ★★★★★ EXCELLENT (smooth animation)\n");
    } else if(metrics->fps >= 20) {
        printf("  Rating: ★★★★☆ GOOD (acceptable)\n");
    } else if(metrics->fps >= 10) {
        printf("  Rating: ★★★☆☆ FAIR (usable)\n");
    } else {
        printf("  Rating: ★★☆☆☆ POOR (sluggish)\n");
    }
}

/*******************************************************************************
 * FLUSH SIMULATION
 *
 * Simulates display flush (SPI/I2C transfer) cost.
 * Real hardware: ~1-5 ms per flush depending on interface speed.
 ******************************************************************************/

#define FLUSH_OVERHEAD_MS 2   // Fixed overhead per flush call
#define PIXELS_PER_MS 10000   // Throughput (depends on SPI speed)

// Simulate flush cost
static uint32_t simulate_flush_cost(uint32_t pixel_count)
{
    uint32_t transfer_time = pixel_count / PIXELS_PER_MS;
    return FLUSH_OVERHEAD_MS + transfer_time;
}

/*******************************************************************************
 * RENDER SIMULATION
 ******************************************************************************/

static uint32_t simulate_render(dirty_area_list_t* dirty_areas,
                                buffer_config_t* buffer_config)
{
    printf("\n>> Rendering %u dirty areas:\n", dirty_areas->count);

    uint32_t total_time_ms = 0;

    for(uint32_t i = 0; i < dirty_areas->count; i++) {
        area_t* area = &dirty_areas->areas[i];
        printf("\n  Area %u:\n", i);
        area_print(area, "Coords");

        if(buffer_config->strategy == BUFFER_STRATEGY_FULL_DOUBLE) {
            // Full buffer: draw entire area in one flush
            printf("    Drawing to full buffer...\n");
            uint32_t flush_time = simulate_flush_cost(area_get_size(area));
            printf("    Flushing buffer (%u ms)\n", flush_time);
            total_time_ms += flush_time;

        } else if(buffer_config->strategy == BUFFER_STRATEGY_PARTIAL_SINGLE ||
                  buffer_config->strategy == BUFFER_STRATEGY_PARTIAL_DOUBLE) {
            // Partial buffer: split into strips
            uint32_t area_height = area_get_height(area);
            uint32_t strips = (area_height + buffer_config->partial_height - 1) /
                            buffer_config->partial_height;

            printf("    Splitting into %u strips (%u rows each):\n",
                   strips, buffer_config->partial_height);

            for(uint32_t s = 0; s < strips; s++) {
                uint32_t strip_y1 = area->y1 + s * buffer_config->partial_height;
                uint32_t strip_y2 = strip_y1 + buffer_config->partial_height - 1;
                if(strip_y2 > area->y2) strip_y2 = area->y2;

                area_t strip = {area->x1, strip_y1, area->x2, strip_y2};
                uint32_t strip_pixels = area_get_size(&strip);

                printf("      Strip %u: [%d,%d - %d,%d] (%u px)\n",
                       s, strip.x1, strip.y1, strip.x2, strip.y2, strip_pixels);

                if(buffer_config->strategy == BUFFER_STRATEGY_PARTIAL_DOUBLE) {
                    printf("        Drawing to buffer %u...\n", s % 2);
                    printf("        (Buffer %u flushing in parallel via DMA)\n", (s+1) % 2);
                } else {
                    printf("        Drawing to buffer...\n");
                }

                uint32_t flush_time = simulate_flush_cost(strip_pixels);
                printf("        Flushing (%u ms)\n", flush_time);
                total_time_ms += flush_time;
            }

            if(buffer_config->strategy == BUFFER_STRATEGY_PARTIAL_DOUBLE) {
                // CPU/DMA parallelism: reduce total time
                uint32_t saved_time = total_time_ms / 3;
                printf("    CPU/DMA parallelism saved %u ms!\n", saved_time);
                total_time_ms -= saved_time;
            }

        } else {
            // Direct mode: extremely slow
            printf("    Direct mode: flushing each pixel individually...\n");
            printf("    (This is VERY slow and not recommended!)\n");
            total_time_ms += simulate_flush_cost(area_get_size(area)) * 10;
        }
    }

    printf("\n  Total render time: %u ms\n", total_time_ms);
    return total_time_ms;
}

/*******************************************************************************
 * DEMO SCENARIOS
 ******************************************************************************/

static void print_header(const char* title)
{
    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("  %s\n", title);
    printf("════════════════════════════════════════════════════════════════\n");
}

// Scenario 1: Single widget update (partial invalidation)
static void demo_scenario_1(void)
{
    print_header("SCENARIO 1: Single Widget Update (Button Click)");

    printf("\n>> User clicks button at [100,100 - 200,140]\n");

    dirty_area_list_t dirty_areas = {0};
    dirty_area_add(&dirty_areas, &(area_t){100, 100, 200, 140});

    printf("  Dirty areas: %u\n", dirty_areas.count);
    area_print(&dirty_areas.areas[0], "Button");

    // Test all buffer strategies
    buffer_strategy_t strategies[] = {
        BUFFER_STRATEGY_FULL_DOUBLE,
        BUFFER_STRATEGY_PARTIAL_SINGLE,
        BUFFER_STRATEGY_PARTIAL_DOUBLE,
    };

    for(int i = 0; i < 3; i++) {
        buffer_config_t config = get_buffer_config(strategies[i], 10);
        print_buffer_config(&config);

        uint32_t render_time = simulate_render(&dirty_areas, &config);

        perf_metrics_t metrics;
        calculate_metrics(&metrics, &dirty_areas, &config, render_time);
        print_metrics(&metrics);
    }
}

// Scenario 2: Multiple widget updates (area joining)
static void demo_scenario_2(void)
{
    print_header("SCENARIO 2: Multiple Widget Updates (Area Joining)");

    printf("\n>> User interacts with UI:\n");
    printf("  - Button 1 clicked at [10,10 - 100,50]\n");
    printf("  - Button 2 clicked at [80,30 - 180,70] (overlaps!)\n");
    printf("  - Label updated at [10,80 - 150,110]\n");
    printf("  - Slider moved at [200,100 - 310,120]\n");

    dirty_area_list_t dirty_areas = {0};
    dirty_area_add(&dirty_areas, &(area_t){10, 10, 100, 50});
    dirty_area_add(&dirty_areas, &(area_t){80, 30, 180, 70});
    dirty_area_add(&dirty_areas, &(area_t){10, 80, 150, 110});
    dirty_area_add(&dirty_areas, &(area_t){200, 100, 310, 120});

    printf("\n>> Before joining:\n");
    printf("  Dirty areas: %u\n", dirty_areas.count);
    for(uint32_t i = 0; i < dirty_areas.count; i++) {
        char name[32];
        snprintf(name, sizeof(name), "Area %u", i);
        area_print(&dirty_areas.areas[i], name);
    }

    // Join overlapping areas
    dirty_area_join(&dirty_areas);

    printf("\n>> After joining:\n");
    printf("  Dirty areas: %u\n", dirty_areas.count);
    for(uint32_t i = 0; i < dirty_areas.count; i++) {
        char name[32];
        snprintf(name, sizeof(name), "Area %u", i);
        area_print(&dirty_areas.areas[i], name);
    }

    // Render with optimal strategy
    buffer_config_t config = get_buffer_config(BUFFER_STRATEGY_PARTIAL_DOUBLE, 20);
    print_buffer_config(&config);

    uint32_t render_time = simulate_render(&dirty_areas, &config);

    perf_metrics_t metrics;
    calculate_metrics(&metrics, &dirty_areas, &config, render_time);
    print_metrics(&metrics);
}

// Scenario 3: Full screen redraw (screen transition)
static void demo_scenario_3(void)
{
    print_header("SCENARIO 3: Full Screen Redraw (Screen Transition)");

    printf("\n>> User navigates to new screen\n");
    printf("  Entire display must be redrawn\n");

    dirty_area_list_t dirty_areas = {0};
    dirty_area_add(&dirty_areas, &(area_t){0, 0, DISPLAY_HOR_RES-1, DISPLAY_VER_RES-1});

    printf("\n  Dirty areas: %u\n", dirty_areas.count);
    area_print(&dirty_areas.areas[0], "Full screen");

    // Compare strategies for full redraw
    buffer_strategy_t strategies[] = {
        BUFFER_STRATEGY_FULL_DOUBLE,
        BUFFER_STRATEGY_PARTIAL_DOUBLE,
    };

    const char* strategy_desc[] = {
        "Full Double (best for full redraws)",
        "Partial Double (slower due to multiple flushes)"
    };

    for(int i = 0; i < 2; i++) {
        buffer_config_t config = get_buffer_config(strategies[i], 30);
        print_buffer_config(&config);
        printf("\n  %s\n", strategy_desc[i]);

        uint32_t render_time = simulate_render(&dirty_areas, &config);

        perf_metrics_t metrics;
        calculate_metrics(&metrics, &dirty_areas, &config, render_time);
        print_metrics(&metrics);
    }
}

// Scenario 4: Animation (continuous updates)
static void demo_scenario_4(void)
{
    print_header("SCENARIO 4: Animation (Moving Object)");

    printf("\n>> Object moves across screen (60 frames)\n");
    printf("  Object size: 50x50 pixels\n");
    printf("  Movement: horizontal, 5 px per frame\n");

    // Simulate 60 frames
    uint32_t total_pixels = 0;
    uint32_t total_render_time = 0;

    buffer_config_t config = get_buffer_config(BUFFER_STRATEGY_PARTIAL_DOUBLE, 10);

    printf("\n  Simulating animation...\n");

    for(int frame = 0; frame < 60; frame++) {
        dirty_area_list_t dirty_areas = {0};

        // Old position (clear)
        int x = frame * 5;
        dirty_area_add(&dirty_areas, &(area_t){x, 100, x+49, 149});

        // New position (draw)
        int new_x = (frame + 1) * 5;
        dirty_area_add(&dirty_areas, &(area_t){new_x, 100, new_x+49, 149});

        // Join if overlap
        dirty_area_join(&dirty_areas);

        for(uint32_t i = 0; i < dirty_areas.count; i++) {
            total_pixels += area_get_size(&dirty_areas.areas[i]);
        }

        uint32_t render_time = simulate_flush_cost(total_pixels / 60);
        total_render_time += render_time;
    }

    printf("  Completed 60 frames\n");
    printf("\n  Total pixels drawn: %u\n", total_pixels);
    printf("  Average pixels/frame: %u\n", total_pixels / 60);
    printf("  Total time: %u ms\n", total_render_time);
    printf("  Average FPS: %.1f\n", 60000.0 / total_render_time);

    if(total_render_time < 1000) {
        printf("  ★★★★★ Smooth animation! (60 FPS achieved)\n");
    } else if(total_render_time < 2000) {
        printf("  ★★★★☆ Good animation (30+ FPS)\n");
    } else {
        printf("  ★★★☆☆ Acceptable (but not smooth)\n");
    }
}

// Scenario 5: Memory comparison
static void demo_scenario_5(void)
{
    print_header("SCENARIO 5: Memory Usage Comparison");

    printf("\n>> Comparing buffer strategies for %dx%d display\n",
           DISPLAY_HOR_RES, DISPLAY_VER_RES);

    typedef struct {
        const char* name;
        buffer_strategy_t strategy;
        uint32_t partial_height;
    } strategy_info_t;

    strategy_info_t strategies[] = {
        {"Full Double Buffer", BUFFER_STRATEGY_FULL_DOUBLE, 0},
        {"Partial 10 rows", BUFFER_STRATEGY_PARTIAL_SINGLE, 10},
        {"Partial 20 rows", BUFFER_STRATEGY_PARTIAL_SINGLE, 20},
        {"Partial 30 rows", BUFFER_STRATEGY_PARTIAL_SINGLE, 30},
        {"Double Partial 10 rows", BUFFER_STRATEGY_PARTIAL_DOUBLE, 10},
        {"Double Partial 20 rows", BUFFER_STRATEGY_PARTIAL_DOUBLE, 20},
        {"Double Partial 30 rows", BUFFER_STRATEGY_PARTIAL_DOUBLE, 30},
    };

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ %-30s | %10s | %8s ║\n", "Strategy", "RAM (KB)", "vs Full");
    printf("╠════════════════════════════════════════════════════════════════╣\n");

    uint32_t full_buffer_ram = DISPLAY_BUFFER_SIZE * 2;

    for(int i = 0; i < 7; i++) {
        buffer_config_t config = get_buffer_config(strategies[i].strategy,
                                                   strategies[i].partial_height);

        uint32_t total_ram = config.buffer_size_bytes * config.buffer_count;
        float vs_full = (float)full_buffer_ram / total_ram;

        printf("║ %-30s | %10.1f | %6.1fx ║\n",
               strategies[i].name,
               total_ram / 1024.0,
               vs_full);
    }

    printf("╚════════════════════════════════════════════════════════════════╝\n");

    printf("\n>> Recommendation for different RAM sizes:\n");
    printf("  < 64 KB RAM:  Partial 10 rows (6.4 KB)\n");
    printf("  64-128 KB:    Double Partial 10-20 rows (12.8-25.6 KB)\n");
    printf("  128-256 KB:   Double Partial 30 rows (38.4 KB)\n");
    printf("  > 256 KB:     Full Double Buffer (307 KB) - if screen fits\n");
}

/*******************************************************************************
 * MAIN
 ******************************************************************************/

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                ║\n");
    printf("║         LVGL Performance Optimization Demo                    ║\n");
    printf("║                                                                ║\n");
    printf("║  Bu demo düşük kaynaklı cihazlarda LVGL performans            ║\n");
    printf("║  optimizasyonlarını gösterir:                                 ║\n");
    printf("║                                                                ║\n");
    printf("║  1. Buffer Strategies (Full vs Partial)                       ║\n");
    printf("║  2. Dirty Area Management                                     ║\n");
    printf("║  3. Performance Metrics                                       ║\n");
    printf("║  4. Memory Optimization                                       ║\n");
    printf("║                                                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    demo_scenario_1();  // Single widget
    demo_scenario_2();  // Multiple widgets + joining
    demo_scenario_3();  // Full screen
    demo_scenario_4();  // Animation
    demo_scenario_5();  // Memory comparison

    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      DEMO COMPLETED                            ║\n");
    printf("║                                                                ║\n");
    printf("║  Key Takeaways:                                                ║\n");
    printf("║  • Double Partial Buffering = Best performance/RAM balance     ║\n");
    printf("║  • Dirty Area Joining reduces draw calls significantly        ║\n");
    printf("║  • Partial updates >>> Full redraws for simple changes        ║\n");
    printf("║  • Choose buffer strategy based on available RAM              ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
