/**
 * ============================================================================
 * LVGL Render Pipeline Mimari Demo
 * ============================================================================
 *
 * Bu demo, LVGL'nin render pipeline mimarisini tamamen bağımsız bir şekilde
 * gösterir. Gerçek çizim yapmaz, sadece printf ile pipeline adımlarını açıklar.
 *
 * Mimari Bileşenler:
 * 1. Dirty Region Tracking (Invalidation System)
 * 2. Object Tree (Parent-Child Hierarchy)
 * 3. Layout Calculation
 * 4. Draw Context (Backend Abstraction)
 * 5. Recursive Drawing
 * 6. Area Joining Optimization
 *
 * Derleme: gcc lvgl_render_pipeline_demo.c -o demo && ./demo
 *
 * Design Pattern'ler:
 * - Strategy Pattern (Backend abstraction via function pointers)
 * - Template Method Pattern (Recursive drawing)
 * - Observer Pattern (Invalidation system)
 * - Command Pattern (Draw descriptors)
 * - Composite Pattern (Object tree)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// 1. TEMEL TİPLER VE YAPILAR
// ============================================================================

/**
 * Area (Coordinate Rectangle)
 * LVGL'deki lv_area_t'ye denk gelir
 */
typedef struct {
    int32_t x1;  // Sol üst X
    int32_t y1;  // Sol üst Y
    int32_t x2;  // Sağ alt X
    int32_t y2;  // Sağ alt Y
} area_t;

/**
 * Color
 * Basitleştirilmiş renk yapısı
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

/**
 * Point
 */
typedef struct {
    int32_t x;
    int32_t y;
} point_t;

// ============================================================================
// 2. AREA UTILITY FUNCTIONS
// ============================================================================

/**
 * İki area'nın kesişimini hesaplar
 * @return true: kesişim var, false: yok
 */
bool area_intersect(area_t* res, const area_t* a1, const area_t* a2)
{
    res->x1 = (a1->x1 > a2->x1) ? a1->x1 : a2->x1;
    res->y1 = (a1->y1 > a2->y1) ? a1->y1 : a2->y1;
    res->x2 = (a1->x2 < a2->x2) ? a1->x2 : a2->x2;
    res->y2 = (a1->y2 < a2->y2) ? a1->y2 : a2->y2;

    // Geçerli kesişim kontrolü
    return (res->x1 <= res->x2) && (res->y1 <= res->y2);
}

/**
 * İki area'yı birleştirir (union)
 */
void area_join(area_t* res, const area_t* a1, const area_t* a2)
{
    res->x1 = (a1->x1 < a2->x1) ? a1->x1 : a2->x1;
    res->y1 = (a1->y1 < a2->y1) ? a1->y1 : a2->y1;
    res->x2 = (a1->x2 > a2->x2) ? a1->x2 : a2->x2;
    res->y2 = (a1->y2 > a2->y2) ? a1->y2 : a2->y2;
}

/**
 * Area'nın genişliğini döner
 */
int32_t area_get_width(const area_t* area)
{
    return area->x2 - area->x1 + 1;
}

/**
 * Area'nın yüksekliğini döner
 */
int32_t area_get_height(const area_t* area)
{
    return area->y2 - area->y1 + 1;
}

/**
 * Area'nın alanını (piksel sayısı) döner
 */
int32_t area_get_size(const area_t* area)
{
    return area_get_width(area) * area_get_height(area);
}

/**
 * İki area çakışıyor mu veya bitişik mi?
 */
bool area_is_on(const area_t* a1, const area_t* a2)
{
    // Sadeleştirilmiş: kesişim veya 1 piksel mesafede mi?
    return !(a1->x2 < a2->x1 - 1 ||
             a1->x1 > a2->x2 + 1 ||
             a1->y2 < a2->y1 - 1 ||
             a1->y1 > a2->y2 + 1);
}

/**
 * Area yazdır (debug)
 */
void area_print(const area_t* area, const char* prefix)
{
    printf("%s[%d,%d - %d,%d] (w=%d, h=%d, size=%d)\n",
           prefix,
           area->x1, area->y1, area->x2, area->y2,
           area_get_width(area),
           area_get_height(area),
           area_get_size(area));
}

// ============================================================================
// 3. DISPLAY VE INVALIDATION SYSTEM
// ============================================================================

#define INV_BUF_SIZE 16  // Maksimum dirty area sayısı

/**
 * Display
 * LVGL'deki lv_disp_t'ye denk gelir
 */
typedef struct {
    int32_t hor_res;           // Yatay çözünürlük
    int32_t ver_res;           // Dikey çözünürlük

    // Dirty region buffer (invalidation system)
    area_t inv_areas[INV_BUF_SIZE];
    uint8_t inv_area_joined[INV_BUF_SIZE];  // Birleştirilmiş mi flag'i
    uint16_t inv_count;                      // Dirty area sayısı

    bool rendering_in_progress;              // Render sırasında yeni invalidation önleme
} display_t;

// Global display instance
static display_t g_display;

/**
 * Display'i başlat
 */
void display_init(int32_t hor_res, int32_t ver_res)
{
    printf("\n========================================\n");
    printf("DISPLAY INITIALIZATION\n");
    printf("========================================\n");
    printf("Resolution: %dx%d\n", hor_res, ver_res);

    g_display.hor_res = hor_res;
    g_display.ver_res = ver_res;
    g_display.inv_count = 0;
    g_display.rendering_in_progress = false;

    memset(g_display.inv_area_joined, 0, sizeof(g_display.inv_area_joined));
}

/**
 * Area'yı invalidate et (dirty region olarak işaretle)
 * LVGL'deki _lv_inv_area()'ya denk gelir
 */
void invalidate_area(const area_t* area)
{
    printf("\n>>> INVALIDATE AREA: ");
    area_print(area, "");

    // Render esnasında yeni invalidation yasak
    if(g_display.rendering_in_progress) {
        printf("    [REJECTED] Rendering in progress!\n");
        return;
    }

    // Ekran sınırları ile kesişim kontrolü
    area_t screen_area = {0, 0, g_display.hor_res - 1, g_display.ver_res - 1};
    area_t clipped_area;

    if(!area_intersect(&clipped_area, area, &screen_area)) {
        printf("    [REJECTED] Outside screen bounds!\n");
        return;
    }

    // Zaten kapsanmış mı kontrol et
    for(uint16_t i = 0; i < g_display.inv_count; i++) {
        area_t test_area;
        if(area_intersect(&test_area, &clipped_area, &g_display.inv_areas[i])) {
            if(area_get_size(&test_area) == area_get_size(&clipped_area)) {
                printf("    [SKIPPED] Already covered by inv_areas[%d]\n", i);
                return;
            }
        }
    }

    // Buffer doldu mu?
    if(g_display.inv_count >= INV_BUF_SIZE) {
        printf("    [WARNING] Invalidation buffer full, invalidating whole screen!\n");
        g_display.inv_areas[0] = screen_area;
        g_display.inv_count = 1;
        return;
    }

    // Yeni dirty area ekle
    g_display.inv_areas[g_display.inv_count] = clipped_area;
    g_display.inv_count++;

    printf("    [ADDED] inv_areas[%d]\n", g_display.inv_count - 1);
}

/**
 * Dirty area'ları temizle
 */
void invalidate_clear(void)
{
    printf("\n>>> CLEAR INVALIDATION BUFFER\n");
    g_display.inv_count = 0;
    memset(g_display.inv_area_joined, 0, sizeof(g_display.inv_area_joined));
}

// ============================================================================
// 4. DRAW CONTEXT VE BACKEND ABSTRACTION
// ============================================================================

/**
 * Draw Rectangle Descriptor (Command Pattern)
 */
typedef struct {
    color_t bg_color;
    uint8_t radius;        // Corner radius
    uint8_t border_width;
    color_t border_color;
} draw_rect_dsc_t;

/**
 * Draw Text Descriptor
 */
typedef struct {
    color_t color;
    const char* text;
} draw_text_dsc_t;

/**
 * Draw Context (Backend Abstraction - Strategy Pattern)
 * LVGL'deki lv_draw_ctx_t'ye denk gelir
 */
typedef struct _draw_ctx_t {
    // Buffer bilgileri
    void* buf;              // Simulated draw buffer
    area_t* buf_area;       // Buffer'ın absolute koordinatları
    const area_t* clip_area;  // Clip bölgesi

    // BACKEND FUNCTION POINTERS (Strategy Pattern)
    void (*draw_rect)(struct _draw_ctx_t* ctx, const draw_rect_dsc_t* dsc, const area_t* coords);
    void (*draw_text)(struct _draw_ctx_t* ctx, const draw_text_dsc_t* dsc, const area_t* coords);
} draw_ctx_t;

/**
 * Software Backend: Rectangle çizimi (simülasyon)
 */
void sw_draw_rect(draw_ctx_t* ctx, const draw_rect_dsc_t* dsc, const area_t* coords)
{
    printf("        [SW BACKEND] Drawing rectangle:\n");
    printf("            Coords: [%d,%d - %d,%d]\n",
           coords->x1, coords->y1, coords->x2, coords->y2);
    printf("            BG Color: RGB(%d, %d, %d)\n",
           dsc->bg_color.r, dsc->bg_color.g, dsc->bg_color.b);
    printf("            Radius: %d, Border: %dpx\n",
           dsc->radius, dsc->border_width);

    // Gerçek implementasyonda: pixel-by-pixel drawing
    // Burada sadece simülasyon
}

/**
 * Software Backend: Text çizimi (simülasyon)
 */
void sw_draw_text(draw_ctx_t* ctx, const draw_text_dsc_t* dsc, const area_t* coords)
{
    printf("        [SW BACKEND] Drawing text:\n");
    printf("            Coords: [%d,%d - %d,%d]\n",
           coords->x1, coords->y1, coords->x2, coords->y2);
    printf("            Text: \"%s\"\n", dsc->text);
    printf("            Color: RGB(%d, %d, %d)\n",
           dsc->color.r, dsc->color.g, dsc->color.b);
}

/**
 * Draw context'i software backend ile başlat
 */
void draw_ctx_init_sw(draw_ctx_t* ctx)
{
    printf("\n>>> DRAW CONTEXT INIT (Software Backend)\n");

    ctx->buf = NULL;  // Simulated buffer
    ctx->buf_area = NULL;
    ctx->clip_area = NULL;

    // Function pointer'ları SW implementation'larıyla doldur
    ctx->draw_rect = sw_draw_rect;
    ctx->draw_text = sw_draw_text;
}

// ============================================================================
// 5. OBJECT SYSTEM (COMPOSITE PATTERN)
// ============================================================================

// Forward declaration
typedef struct obj_t obj_t;
typedef struct obj_class_t obj_class_t;

/**
 * Object Class (vtable benzeri yapı)
 * LVGL'deki lv_obj_class_t'ye denk gelir
 */
struct obj_class_t {
    const char* name;
    const obj_class_t* base_class;  // Inheritance

    // Virtual methods (function pointers)
    void (*constructor)(obj_t* obj);
    void (*draw)(obj_t* obj, draw_ctx_t* ctx);
};

/**
 * Object (Base class)
 * LVGL'deki lv_obj_t'ye denk gelir
 */
struct obj_t {
    const obj_class_t* class_p;

    // Hierarchy
    obj_t* parent;
    obj_t** children;
    uint32_t child_count;

    // Coordinates
    area_t coords;

    // User data
    void* user_data;
};

// Global object counter (debug için)
static uint32_t g_obj_id_counter = 0;

/**
 * Base object constructor
 */
void base_obj_constructor(obj_t* obj)
{
    printf("    [CONSTRUCTOR] Base object #%d\n", g_obj_id_counter++);
}

/**
 * Base object draw
 */
void base_obj_draw(obj_t* obj, draw_ctx_t* ctx)
{
    printf("    [DRAW] Base object at ");
    area_print(&obj->coords, "");

    // Base object simple background çizer
    draw_rect_dsc_t dsc;
    dsc.bg_color = (color_t){200, 200, 200};
    dsc.radius = 0;
    dsc.border_width = 1;
    dsc.border_color = (color_t){100, 100, 100};

    ctx->draw_rect(ctx, &dsc, &obj->coords);
}

// Base object class
const obj_class_t base_obj_class = {
    .name = "obj",
    .base_class = NULL,
    .constructor = base_obj_constructor,
    .draw = base_obj_draw,
};

/**
 * Object yaratma
 */
obj_t* obj_create(obj_t* parent, const obj_class_t* class_p)
{
    printf("\n>>> CREATE OBJECT (class: %s)\n", class_p->name);

    obj_t* obj = (obj_t*)malloc(sizeof(obj_t));
    memset(obj, 0, sizeof(obj_t));

    obj->class_p = class_p;
    obj->parent = parent;
    obj->children = NULL;
    obj->child_count = 0;

    // Parent'a ekle
    if(parent) {
        parent->child_count++;
        parent->children = (obj_t**)realloc(parent->children,
                                           sizeof(obj_t*) * parent->child_count);
        parent->children[parent->child_count - 1] = obj;
        printf("    Added to parent (parent now has %d children)\n", parent->child_count);
    }

    // Constructor çağır
    if(class_p->constructor) {
        class_p->constructor(obj);
    }

    return obj;
}

/**
 * Object pozisyonunu ayarla
 */
void obj_set_pos(obj_t* obj, int32_t x, int32_t y, int32_t w, int32_t h)
{
    printf("\n>>> SET OBJECT POSITION\n");

    // Eski koordinatları invalidate et
    if(obj->coords.x2 > obj->coords.x1) {  // Daha önce set edilmiş mi?
        printf("    Invalidating old coords...\n");
        invalidate_area(&obj->coords);
    }

    // Yeni koordinatları ayarla
    obj->coords.x1 = x;
    obj->coords.y1 = y;
    obj->coords.x2 = x + w - 1;
    obj->coords.y2 = y + h - 1;

    printf("    New coords: [%d,%d - %d,%d]\n", x, y, x+w-1, y+h-1);

    // Yeni koordinatları invalidate et
    invalidate_area(&obj->coords);
}

/**
 * Object'i recursive olarak çiz
 * LVGL'deki lv_obj_redraw()'a denk gelir
 */
void obj_draw_recursive(obj_t* obj, draw_ctx_t* ctx, const area_t* clip_area)
{
    printf("\n    >>> DRAW OBJECT (class: %s)\n", obj->class_p->name);

    // Clip area ile kesişim kontrolü
    area_t clipped;
    if(!area_intersect(&clipped, &obj->coords, clip_area)) {
        printf("        [SKIPPED] Outside clip area\n");
        return;
    }

    ctx->clip_area = &clipped;

    // MAIN DRAWING PHASE
    printf("        PHASE: MAIN DRAW\n");
    if(obj->class_p->draw) {
        obj->class_p->draw(obj, ctx);
    }

    // CHILDREN DRAWING PHASE (Recursive)
    if(obj->child_count > 0) {
        printf("        PHASE: DRAW CHILDREN (%d children)\n", obj->child_count);
        for(uint32_t i = 0; i < obj->child_count; i++) {
            obj_draw_recursive(obj->children[i], ctx, clip_area);
        }
    }

    // POST DRAWING PHASE (overlay, effects)
    printf("        PHASE: POST DRAW (effects, overlay)\n");
}

// ============================================================================
// 6. CUSTOM WIDGET EXAMPLES
// ============================================================================

/**
 * Button Widget
 */
typedef struct {
    obj_t obj;  // Base class (inheritance)
    color_t color;
} button_t;

void button_constructor(obj_t* obj)
{
    printf("    [CONSTRUCTOR] Button widget\n");
    button_t* btn = (button_t*)obj;
    btn->color = (color_t){0, 128, 255};  // Blue
}

void button_draw(obj_t* obj, draw_ctx_t* ctx)
{
    printf("    [DRAW] Button widget at ");
    area_print(&obj->coords, "");

    button_t* btn = (button_t*)obj;

    draw_rect_dsc_t dsc;
    dsc.bg_color = btn->color;
    dsc.radius = 10;
    dsc.border_width = 2;
    dsc.border_color = (color_t){0, 64, 128};

    ctx->draw_rect(ctx, &dsc, &obj->coords);
}

const obj_class_t button_class = {
    .name = "button",
    .base_class = &base_obj_class,
    .constructor = button_constructor,
    .draw = button_draw,
};

/**
 * Label Widget
 */
typedef struct {
    obj_t obj;  // Base class
    const char* text;
} label_t;

void label_constructor(obj_t* obj)
{
    printf("    [CONSTRUCTOR] Label widget\n");
    label_t* label = (label_t*)obj;
    label->text = "Label";
}

void label_draw(obj_t* obj, draw_ctx_t* ctx)
{
    printf("    [DRAW] Label widget at ");
    area_print(&obj->coords, "");

    label_t* label = (label_t*)obj;

    draw_text_dsc_t dsc;
    dsc.color = (color_t){0, 0, 0};
    dsc.text = label->text;

    ctx->draw_text(ctx, &dsc, &obj->coords);
}

const obj_class_t label_class = {
    .name = "label",
    .base_class = &base_obj_class,
    .constructor = label_constructor,
    .draw = label_draw,
};

void label_set_text(obj_t* obj, const char* text)
{
    label_t* label = (label_t*)obj;
    printf("\n>>> SET LABEL TEXT: \"%s\"\n", text);
    label->text = text;

    // Text değişince invalidate et
    invalidate_area(&obj->coords);
}

// ============================================================================
// 7. RENDER PIPELINE
// ============================================================================

/**
 * Dirty area'ları birleştir (Flyweight Pattern optimizasyonu)
 * LVGL'deki lv_refr_join_area()'ya denk gelir
 */
void refr_join_areas(void)
{
    printf("\n========================================\n");
    printf("DIRTY AREA JOINING\n");
    printf("========================================\n");
    printf("Before join: %d dirty areas\n", g_display.inv_count);

    for(uint32_t i = 0; i < g_display.inv_count; i++) {
        if(g_display.inv_area_joined[i]) continue;

        printf("\n  Checking inv_areas[%d] for joining...\n", i);

        for(uint32_t j = 0; j < g_display.inv_count; j++) {
            if(g_display.inv_area_joined[j] || i == j) continue;

            // Çakışıyor mu veya bitişik mi?
            if(area_is_on(&g_display.inv_areas[i], &g_display.inv_areas[j])) {
                printf("    Found overlap with inv_areas[%d]\n", j);

                // Birleştir
                area_t joined;
                area_join(&joined, &g_display.inv_areas[i], &g_display.inv_areas[j]);

                // Birleşik alan daha küçük mü? (boşluk çok fazla değilse join)
                int32_t size_i = area_get_size(&g_display.inv_areas[i]);
                int32_t size_j = area_get_size(&g_display.inv_areas[j]);
                int32_t size_joined = area_get_size(&joined);

                if(size_joined < (size_i + size_j)) {
                    printf("    JOIN: %d + %d = %d pixels (saved %d pixels)\n",
                           size_i, size_j, size_joined, (size_i + size_j) - size_joined);

                    g_display.inv_areas[i] = joined;
                    g_display.inv_area_joined[j] = 1;  // Mark as joined
                } else {
                    printf("    SKIP: Joining would waste %d pixels\n",
                           size_joined - (size_i + size_j));
                }
            }
        }
    }

    // Joined olanları listeden çıkar
    uint32_t active_count = 0;
    for(uint32_t i = 0; i < g_display.inv_count; i++) {
        if(!g_display.inv_area_joined[i]) {
            active_count++;
        }
    }

    printf("\nAfter join: %d active dirty areas\n", active_count);
}

/**
 * Bir dirty area'yı render et
 */
void refr_area(obj_t* root_obj, const area_t* area, draw_ctx_t* ctx)
{
    printf("\n========================================\n");
    printf("RENDERING DIRTY AREA\n");
    printf("========================================\n");
    area_print(area, "Area: ");

    // Draw context'e area bilgilerini set et
    static area_t buf_area;  // Simulated buffer area
    buf_area = *area;

    ctx->buf_area = &buf_area;
    ctx->clip_area = area;

    // Root object'ten başlayarak recursive draw
    printf("\nStarting recursive drawing from root...\n");
    obj_draw_recursive(root_obj, ctx, area);
}

/**
 * Tüm dirty area'ları render et
 */
void refr_invalid_areas(obj_t* root_obj, draw_ctx_t* ctx)
{
    printf("\n========================================\n");
    printf("RENDER INVALID AREAS\n");
    printf("========================================\n");

    if(g_display.inv_count == 0) {
        printf("No dirty areas to render.\n");
        return;
    }

    g_display.rendering_in_progress = true;

    // Her dirty area için render
    for(uint32_t i = 0; i < g_display.inv_count; i++) {
        if(!g_display.inv_area_joined[i]) {
            refr_area(root_obj, &g_display.inv_areas[i], ctx);
        }
    }

    g_display.rendering_in_progress = false;
}

/**
 * Ana render pipeline fonksiyonu
 * LVGL'deki _lv_disp_refr_timer()'a denk gelir
 */
void render_pipeline(obj_t* root_obj)
{
    printf("\n\n");
    printf("████████████████████████████████████████████████████████████████\n");
    printf("█                  RENDER PIPELINE START                      █\n");
    printf("████████████████████████████████████████████████████████████████\n");

    // 1. LAYOUT UPDATE (şu an basit, sadece bilgi)
    printf("\n========================================\n");
    printf("PHASE 1: LAYOUT UPDATE\n");
    printf("========================================\n");
    printf("(In real LVGL: lv_obj_update_layout() would run here)\n");
    printf("Layout calculation completed.\n");

    // 2. DIRTY AREA JOINING
    printf("\n========================================\n");
    printf("PHASE 2: DIRTY AREA OPTIMIZATION\n");
    printf("========================================\n");
    refr_join_areas();

    // 3. RENDERING
    printf("\n========================================\n");
    printf("PHASE 3: RENDERING\n");
    printf("========================================\n");

    draw_ctx_t ctx;
    draw_ctx_init_sw(&ctx);

    refr_invalid_areas(root_obj, &ctx);

    // 4. CLEANUP
    printf("\n========================================\n");
    printf("PHASE 4: CLEANUP\n");
    printf("========================================\n");
    invalidate_clear();

    printf("\n████████████████████████████████████████████████████████████████\n");
    printf("█                  RENDER PIPELINE END                        █\n");
    printf("████████████████████████████████████████████████████████████████\n");
}

// ============================================================================
// 8. MAIN DEMO
// ============================================================================

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                ║\n");
    printf("║         LVGL Render Pipeline Architecture Demo                ║\n");
    printf("║                                                                ║\n");
    printf("║  Bu demo LVGL'nin render pipeline mimarisini gösterir:        ║\n");
    printf("║  1. Invalidation (Dirty Region Tracking)                      ║\n");
    printf("║  2. Area Joining Optimization                                 ║\n");
    printf("║  3. Recursive Drawing                                         ║\n");
    printf("║  4. Backend Abstraction (Strategy Pattern)                    ║\n");
    printf("║  5. Object Hierarchy (Composite Pattern)                      ║\n");
    printf("║                                                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    // Display başlat
    display_init(800, 480);

    // Object tree oluştur
    printf("\n");
    printf("========================================\n");
    printf("BUILDING OBJECT TREE\n");
    printf("========================================\n");

    obj_t* screen = obj_create(NULL, &base_obj_class);
    obj_set_pos(screen, 0, 0, 800, 480);

    obj_t* panel = obj_create(screen, &base_obj_class);
    obj_set_pos(panel, 50, 50, 300, 200);

    obj_t* button1 = obj_create(panel, &button_class);
    obj_set_pos(button1, 70, 70, 100, 40);

    obj_t* button2 = obj_create(panel, &button_class);
    obj_set_pos(button2, 200, 70, 100, 40);

    obj_t* label = obj_create(panel, &label_class);
    obj_set_pos(label, 70, 150, 200, 30);
    label_set_text(label, "Hello LVGL Pipeline!");

    // İLK RENDER
    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("                    INITIAL RENDER                             \n");
    printf("════════════════════════════════════════════════════════════════\n");
    render_pipeline(screen);

    // SİMÜLE EDİLMİŞ USER ACTION: Button rengini değiştir
    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("           SIMULATED USER ACTION: Change Button Color          \n");
    printf("════════════════════════════════════════════════════════════════\n");

    button_t* btn = (button_t*)button1;
    btn->color = (color_t){255, 0, 0};  // Red
    invalidate_area(&button1->coords);

    render_pipeline(screen);

    // SİMÜLE EDİLMİŞ USER ACTION: Label text değiştir
    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("           SIMULATED USER ACTION: Change Label Text            \n");
    printf("════════════════════════════════════════════════════════════════\n");

    label_set_text(label, "Updated Text!");

    render_pipeline(screen);

    // SİMÜLE EDİLMİŞ USER ACTION: İki button'u aynı anda değiştir (area joining test)
    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("      SIMULATED USER ACTION: Change Two Adjacent Buttons       \n");
    printf("                  (Testing Area Joining)                       \n");
    printf("════════════════════════════════════════════════════════════════\n");

    ((button_t*)button1)->color = (color_t){0, 255, 0};  // Green
    ((button_t*)button2)->color = (color_t){0, 0, 255};  // Blue

    invalidate_area(&button1->coords);
    invalidate_area(&button2->coords);

    render_pipeline(screen);

    // ÖZET
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      DEMO COMPLETED                            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Key Takeaways:\n");
    printf("  ✓ Invalidation System: Only changed areas are marked dirty\n");
    printf("  ✓ Area Joining: Overlapping dirty areas are merged\n");
    printf("  ✓ Recursive Drawing: Object tree is drawn depth-first\n");
    printf("  ✓ Backend Abstraction: Function pointers enable pluggable backends\n");
    printf("  ✓ Object Hierarchy: Parent-child relationships managed automatically\n");
    printf("\n");
    printf("LVGL Design Patterns Demonstrated:\n");
    printf("  • Strategy Pattern (draw_ctx_t function pointers)\n");
    printf("  • Command Pattern (draw descriptors)\n");
    printf("  • Template Method Pattern (recursive drawing)\n");
    printf("  • Observer Pattern (invalidation triggering render)\n");
    printf("  • Composite Pattern (object tree)\n");
    printf("  • Flyweight Pattern (area joining optimization)\n");
    printf("\n");

    // Cleanup (basitleştirilmiş)
    // Gerçek uygulamada tüm nesneler ve bellek serbest bırakılır

    return 0;
}
