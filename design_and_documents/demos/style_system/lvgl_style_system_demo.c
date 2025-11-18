/*
 * LVGL Style ve Theme Engine Architecture Demo
 * =============================================
 *
 * Bu demo LVGL'nin style ve theme sisteminin temel mimarisini gösterir.
 * TAMAMEN STANDALONE - LVGL kütüphanesine bağımlılık YOK!
 *
 * Kapsanan Konular:
 * -----------------
 * 1. Style Properties (lv_style_t benzeri)
 * 2. Selector System (Part + State)
 * 3. Style Cascading (Theme → Local → Inline)
 * 4. Style Inheritance (Parent → Child)
 * 5. Style Priority ve Override
 * 6. Theme Provider (Automatic Style Assignment)
 * 7. Transition System (State değişimi)
 *
 * LVGL Kaynak Referansları:
 * -------------------------
 * - src/misc/lv_style.h - Style structure
 * - src/misc/lv_style.c - Style operations
 * - src/core/lv_obj_style.c - Widget style management
 * - src/core/lv_theme.c - Theme provider
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * PART 1: BASIC TYPES
 ******************************************************************************/

// Color yapısı (RGB888)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

#define COLOR_RED     ((color_t){255, 0, 0})
#define COLOR_GREEN   ((color_t){0, 255, 0})
#define COLOR_BLUE    ((color_t){0, 0, 255})
#define COLOR_WHITE   ((color_t){255, 255, 255})
#define COLOR_BLACK   ((color_t){0, 0, 0})
#define COLOR_GRAY    ((color_t){128, 128, 128})
#define COLOR_ORANGE  ((color_t){255, 165, 0})

// Opacity tipi (0-255)
typedef uint8_t opacity_t;
#define OPACITY_TRANSP  0
#define OPACITY_COVER   255

/*******************************************************************************
 * PART 2: STYLE PROPERTIES
 *
 * LVGL'de 100+ property var. Bu demo'da en yaygın kullanılanları göstereceğiz.
 * LVGL Ref: src/misc/lv_style.h:155-280
 ******************************************************************************/

typedef enum {
    // Background properties
    STYLE_PROP_BG_COLOR = 0,
    STYLE_PROP_BG_OPA,
    STYLE_PROP_BG_RADIUS,

    // Border properties
    STYLE_PROP_BORDER_COLOR,
    STYLE_PROP_BORDER_WIDTH,
    STYLE_PROP_BORDER_OPA,

    // Padding properties
    STYLE_PROP_PAD_TOP,
    STYLE_PROP_PAD_BOTTOM,
    STYLE_PROP_PAD_LEFT,
    STYLE_PROP_PAD_RIGHT,

    // Text properties (INHERITED)
    STYLE_PROP_TEXT_COLOR,
    STYLE_PROP_TEXT_OPA,
    STYLE_PROP_TEXT_SIZE,

    // Shadow properties
    STYLE_PROP_SHADOW_WIDTH,
    STYLE_PROP_SHADOW_COLOR,
    STYLE_PROP_SHADOW_OPA,

    // Transform properties
    STYLE_PROP_TRANSFORM_SCALE_X,
    STYLE_PROP_TRANSFORM_SCALE_Y,

    // Animation/transition
    STYLE_PROP_TRANSITION_TIME,

    STYLE_PROP_COUNT
} style_prop_t;

// Hangi property'lerin inherit edildiğini belirten bitmask
// LVGL'de text, font, line space gibi property'ler inherit edilir
static const bool STYLE_PROP_INHERITABLE[STYLE_PROP_COUNT] = {
    [STYLE_PROP_TEXT_COLOR] = true,
    [STYLE_PROP_TEXT_OPA] = true,
    [STYLE_PROP_TEXT_SIZE] = true,
    // Diğerleri false (default)
};

// Property isimleri (debug için)
static const char* STYLE_PROP_NAMES[STYLE_PROP_COUNT] = {
    [STYLE_PROP_BG_COLOR] = "bg_color",
    [STYLE_PROP_BG_OPA] = "bg_opa",
    [STYLE_PROP_BG_RADIUS] = "bg_radius",
    [STYLE_PROP_BORDER_COLOR] = "border_color",
    [STYLE_PROP_BORDER_WIDTH] = "border_width",
    [STYLE_PROP_BORDER_OPA] = "border_opa",
    [STYLE_PROP_PAD_TOP] = "pad_top",
    [STYLE_PROP_PAD_BOTTOM] = "pad_bottom",
    [STYLE_PROP_PAD_LEFT] = "pad_left",
    [STYLE_PROP_PAD_RIGHT] = "pad_right",
    [STYLE_PROP_TEXT_COLOR] = "text_color",
    [STYLE_PROP_TEXT_OPA] = "text_opa",
    [STYLE_PROP_TEXT_SIZE] = "text_size",
    [STYLE_PROP_SHADOW_WIDTH] = "shadow_width",
    [STYLE_PROP_SHADOW_COLOR] = "shadow_color",
    [STYLE_PROP_SHADOW_OPA] = "shadow_opa",
    [STYLE_PROP_TRANSFORM_SCALE_X] = "scale_x",
    [STYLE_PROP_TRANSFORM_SCALE_Y] = "scale_y",
    [STYLE_PROP_TRANSITION_TIME] = "transition_time",
};

/*******************************************************************************
 * PART 3: STYLE VALUE
 *
 * LVGL'de lv_style_value_t union kullanılır (color, int, ptr)
 * LVGL Ref: src/misc/lv_style.h:54-60
 ******************************************************************************/

typedef union {
    int32_t num;        // Integer values (padding, size, etc.)
    color_t color;      // Color values
    opacity_t opa;      // Opacity values
    void* ptr;          // Pointer values (font, image, etc.)
} style_value_t;

/*******************************************************************************
 * PART 4: STATE AND PART SYSTEM
 *
 * LVGL'de selector = part | state bitmaskleri
 * LVGL Ref: src/core/lv_obj.h:64-93 (states)
 *           src/core/lv_obj.h:95-118 (parts)
 ******************************************************************************/

// Widget state (bitmask)
typedef enum {
    STATE_DEFAULT   = 0x0000,
    STATE_CHECKED   = 0x0001,
    STATE_FOCUSED   = 0x0002,
    STATE_PRESSED   = 0x0004,
    STATE_DISABLED  = 0x0008,
    STATE_EDITED    = 0x0010,
    STATE_HOVERED   = 0x0020,
    STATE_USER_1    = 0x1000,  // Ek custom state'ler
    STATE_USER_2    = 0x2000,
    STATE_ANY       = 0xFFFF,
} widget_state_t;

// Widget part (bitmask)
typedef enum {
    PART_MAIN       = 0x00000000,  // Widget'ın ana kısmı
    PART_SCROLLBAR  = 0x00010000,  // Scrollbar
    PART_INDICATOR  = 0x00020000,  // Slider indicator, bar fill, vb.
    PART_KNOB       = 0x00030000,  // Slider knob, switch knob
    PART_SELECTED   = 0x00040000,  // Selected item
    PART_ITEMS      = 0x00050000,  // List items
    PART_CURSOR     = 0x00060000,  // Text cursor
    PART_ANY        = 0x000F0000,
} widget_part_t;

// Selector = Part | State
typedef uint32_t style_selector_t;

// Selector makroları (LVGL'deki gibi)
#define SELECTOR(part, state) ((style_selector_t)((part) | (state)))
#define SELECTOR_MAIN         SELECTOR(PART_MAIN, STATE_DEFAULT)
#define SELECTOR_MAIN_PRESSED SELECTOR(PART_MAIN, STATE_PRESSED)
#define SELECTOR_MAIN_FOCUSED SELECTOR(PART_MAIN, STATE_FOCUSED)

// Selector matching (bitmask karşılaştırma)
static bool selector_matches(style_selector_t selector, widget_part_t part, widget_state_t state)
{
    uint32_t selector_part = selector & 0xFFFF0000;
    uint32_t selector_state = selector & 0x0000FFFF;

    // Part match
    bool part_match = (selector_part == PART_ANY) || (selector_part == part);

    // State match (bitmask)
    bool state_match = (selector_state == STATE_ANY) ||
                       (selector_state == STATE_DEFAULT && state == STATE_DEFAULT) ||
                       ((selector_state & state) != 0);

    return part_match && state_match;
}

/*******************************************************************************
 * PART 5: STYLE STRUCTURE
 *
 * LVGL'de style, property-value pair'leri dynamic array olarak tutar.
 * Biz basitleştirme için fixed-size array kullanacağız.
 * LVGL Ref: src/misc/lv_style.h:129-138
 ******************************************************************************/

#define STYLE_MAX_PROPS 32

typedef struct {
    style_prop_t prop;
    style_value_t value;
} style_prop_entry_t;

typedef struct {
    style_prop_entry_t props[STYLE_MAX_PROPS];
    uint32_t prop_count;
} style_t;

// Style initialization
void style_init(style_t* style)
{
    memset(style, 0, sizeof(style_t));
    printf("  [STYLE] Initialized\n");
}

// Style set property
void style_set_prop(style_t* style, style_prop_t prop, style_value_t value)
{
    // Varsa güncelle
    for(uint32_t i = 0; i < style->prop_count; i++) {
        if(style->props[i].prop == prop) {
            style->props[i].value = value;
            return;
        }
    }

    // Yoksa ekle
    if(style->prop_count < STYLE_MAX_PROPS) {
        style->props[style->prop_count].prop = prop;
        style->props[style->prop_count].value = value;
        style->prop_count++;
    }
}

// Convenience setters
void style_set_bg_color(style_t* style, color_t color)
{
    style_set_prop(style, STYLE_PROP_BG_COLOR, (style_value_t){.color = color});
}

void style_set_bg_opa(style_t* style, opacity_t opa)
{
    style_set_prop(style, STYLE_PROP_BG_OPA, (style_value_t){.opa = opa});
}

void style_set_radius(style_t* style, int32_t radius)
{
    style_set_prop(style, STYLE_PROP_BG_RADIUS, (style_value_t){.num = radius});
}

void style_set_border_color(style_t* style, color_t color)
{
    style_set_prop(style, STYLE_PROP_BORDER_COLOR, (style_value_t){.color = color});
}

void style_set_border_width(style_t* style, int32_t width)
{
    style_set_prop(style, STYLE_PROP_BORDER_WIDTH, (style_value_t){.num = width});
}

void style_set_pad_all(style_t* style, int32_t pad)
{
    style_set_prop(style, STYLE_PROP_PAD_TOP, (style_value_t){.num = pad});
    style_set_prop(style, STYLE_PROP_PAD_BOTTOM, (style_value_t){.num = pad});
    style_set_prop(style, STYLE_PROP_PAD_LEFT, (style_value_t){.num = pad});
    style_set_prop(style, STYLE_PROP_PAD_RIGHT, (style_value_t){.num = pad});
}

void style_set_text_color(style_t* style, color_t color)
{
    style_set_prop(style, STYLE_PROP_TEXT_COLOR, (style_value_t){.color = color});
}

void style_set_text_size(style_t* style, int32_t size)
{
    style_set_prop(style, STYLE_PROP_TEXT_SIZE, (style_value_t){.num = size});
}

void style_set_shadow_width(style_t* style, int32_t width)
{
    style_set_prop(style, STYLE_PROP_SHADOW_WIDTH, (style_value_t){.num = width});
}

void style_set_shadow_color(style_t* style, color_t color)
{
    style_set_prop(style, STYLE_PROP_SHADOW_COLOR, (style_value_t){.color = color});
}

void style_set_transition_time(style_t* style, int32_t time_ms)
{
    style_set_prop(style, STYLE_PROP_TRANSITION_TIME, (style_value_t){.num = time_ms});
}

// Style get property
bool style_get_prop(const style_t* style, style_prop_t prop, style_value_t* out_value)
{
    for(uint32_t i = 0; i < style->prop_count; i++) {
        if(style->props[i].prop == prop) {
            *out_value = style->props[i].value;
            return true;
        }
    }
    return false;
}

/*******************************************************************************
 * PART 6: STYLE DESCRIPTOR
 *
 * Widget'lar birden fazla style'a sahip olabilir (her biri farklı selector için)
 * LVGL Ref: src/core/lv_obj.h:289-295
 ******************************************************************************/

typedef struct {
    style_t* style;
    style_selector_t selector;
} style_descriptor_t;

/*******************************************************************************
 * PART 7: WIDGET WITH STYLE SUPPORT
 ******************************************************************************/

#define WIDGET_MAX_STYLES 8

typedef struct widget_t widget_t;

struct widget_t {
    uint32_t id;
    widget_t* parent;
    widget_state_t state;

    // Local styles (widget'a özel atanan style'lar)
    style_descriptor_t local_styles[WIDGET_MAX_STYLES];
    uint32_t local_style_count;

    // Inline styles (doğrudan widget üzerinde set edilen property'ler)
    // LVGL'de en yüksek priority'ye sahiptir
    style_t inline_style;
};

// Widget oluştur
widget_t* widget_create(widget_t* parent, uint32_t id)
{
    widget_t* widget = (widget_t*)calloc(1, sizeof(widget_t));
    widget->id = id;
    widget->parent = parent;
    widget->state = STATE_DEFAULT;
    style_init(&widget->inline_style);

    printf("\n>>> CREATE WIDGET #%u\n", id);
    if(parent) {
        printf("    Parent: Widget #%u\n", parent->id);
    }

    return widget;
}

// Widget'a state ekle
void widget_add_state(widget_t* widget, widget_state_t state)
{
    widget_state_t old_state = widget->state;
    widget->state |= state;

    printf("\n>>> WIDGET #%u: ADD STATE\n", widget->id);
    printf("    Old state: 0x%04X\n", old_state);
    printf("    New state: 0x%04X\n", widget->state);
}

// Widget state'i kaldır
void widget_remove_state(widget_t* widget, widget_state_t state)
{
    widget_state_t old_state = widget->state;
    widget->state &= ~state;

    printf("\n>>> WIDGET #%u: REMOVE STATE\n", widget->id);
    printf("    Old state: 0x%04X\n", old_state);
    printf("    New state: 0x%04X\n", widget->state);
}

// Widget'a local style ekle
void widget_add_style(widget_t* widget, style_t* style, style_selector_t selector)
{
    if(widget->local_style_count >= WIDGET_MAX_STYLES) {
        printf("    [WARNING] Max style count reached!\n");
        return;
    }

    widget->local_styles[widget->local_style_count].style = style;
    widget->local_styles[widget->local_style_count].selector = selector;
    widget->local_style_count++;

    printf("\n>>> WIDGET #%u: ADD LOCAL STYLE\n", widget->id);
    printf("    Selector: 0x%08X (part=0x%08X, state=0x%04X)\n",
           selector, selector & 0xFFFF0000, selector & 0x0000FFFF);
    printf("    Total local styles: %u\n", widget->local_style_count);
}

// Inline style set (en yüksek priority)
void widget_set_style_bg_color(widget_t* widget, color_t color)
{
    printf("\n>>> WIDGET #%u: SET INLINE STYLE (bg_color)\n", widget->id);
    printf("    Color: RGB(%u, %u, %u)\n", color.r, color.g, color.b);
    style_set_bg_color(&widget->inline_style, color);
}

void widget_set_style_pad_all(widget_t* widget, int32_t pad)
{
    printf("\n>>> WIDGET #%u: SET INLINE STYLE (padding)\n", widget->id);
    printf("    Padding: %d\n", pad);
    style_set_pad_all(&widget->inline_style, pad);
}

void widget_set_style_text_color(widget_t* widget, color_t color)
{
    printf("\n>>> WIDGET #%u: SET INLINE STYLE (text_color)\n", widget->id);
    printf("    Color: RGB(%u, %u, %u)\n", color.r, color.g, color.b);
    style_set_text_color(&widget->inline_style, color);
}

/*******************************************************************************
 * PART 8: THEME PROVIDER
 *
 * Theme, widget'lara otomatik olarak style atar.
 * LVGL Ref: src/core/lv_theme.c
 ******************************************************************************/

typedef struct {
    const char* name;

    // Default styles (tüm widget'lara uygulanır)
    style_t default_style;

    // Widget-specific styles
    style_t button_style;
    style_t button_pressed_style;
    style_t label_style;
    style_t container_style;
} theme_t;

// Theme initialization
void theme_init(theme_t* theme, const char* name)
{
    theme->name = name;
    style_init(&theme->default_style);
    style_init(&theme->button_style);
    style_init(&theme->button_pressed_style);
    style_init(&theme->label_style);
    style_init(&theme->container_style);

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ THEME INIT: %s\n", name);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
}

// Theme apply to widget (otomatik style assignment)
void theme_apply(theme_t* theme, widget_t* widget, const char* widget_type)
{
    printf("\n>>> THEME APPLY: %s theme to Widget #%u (%s)\n",
           theme->name, widget->id, widget_type);

    // Her widget'a default style ekle
    widget_add_style(widget, &theme->default_style, SELECTOR_MAIN);

    // Widget type'a göre özel style'lar ekle
    if(strcmp(widget_type, "button") == 0) {
        widget_add_style(widget, &theme->button_style, SELECTOR_MAIN);
        widget_add_style(widget, &theme->button_pressed_style, SELECTOR_MAIN_PRESSED);
        printf("    Applied: default + button + button_pressed styles\n");
    }
    else if(strcmp(widget_type, "label") == 0) {
        widget_add_style(widget, &theme->label_style, SELECTOR_MAIN);
        printf("    Applied: default + label styles\n");
    }
    else if(strcmp(widget_type, "container") == 0) {
        widget_add_style(widget, &theme->container_style, SELECTOR_MAIN);
        printf("    Applied: default + container styles\n");
    }
}

/*******************************************************************************
 * PART 9: STYLE RESOLUTION (Cascading + Inheritance + Priority)
 *
 * Bu LVGL style sisteminin kalbidir!
 * Priority sırası: INLINE > LOCAL > THEME > INHERITED > DEFAULT
 *
 * LVGL Ref: src/core/lv_obj_style.c:545-685 (lv_obj_get_style_prop)
 ******************************************************************************/

typedef enum {
    STYLE_RES_NOT_FOUND = 0,
    STYLE_RES_INLINE,
    STYLE_RES_LOCAL,
    STYLE_RES_INHERITED,
    STYLE_RES_DEFAULT,
} style_res_type_t;

typedef struct {
    bool found;
    style_value_t value;
    style_res_type_t source;
} style_res_t;

// Forward declaration
static style_res_t widget_get_style_prop(widget_t* widget, style_prop_t prop,
                                         widget_part_t part);

// Style property resolution
static style_res_t widget_get_style_prop_impl(widget_t* widget, style_prop_t prop,
                                               widget_part_t part, bool allow_inherit)
{
    style_res_t res = {0};
    widget_state_t state = widget->state;

    // PRIORITY 1: INLINE STYLE (en yüksek priority)
    if(style_get_prop(&widget->inline_style, prop, &res.value)) {
        res.found = true;
        res.source = STYLE_RES_INLINE;
        return res;
    }

    // PRIORITY 2: LOCAL STYLES (selector matching ile)
    // Birden fazla style match edebilir, en son eklenen kazanır
    for(uint32_t i = 0; i < widget->local_style_count; i++) {
        style_descriptor_t* desc = &widget->local_styles[i];

        if(selector_matches(desc->selector, part, state)) {
            if(style_get_prop(desc->style, prop, &res.value)) {
                res.found = true;
                res.source = STYLE_RES_LOCAL;
                // Continue loop - sonraki style override edebilir
            }
        }
    }

    if(res.found) return res;

    // PRIORITY 3: INHERITED (sadece inheritable property'ler için)
    if(allow_inherit && STYLE_PROP_INHERITABLE[prop] && widget->parent) {
        res = widget_get_style_prop(widget->parent, prop, PART_MAIN);
        if(res.found) {
            res.source = STYLE_RES_INHERITED;
            return res;
        }
    }

    // PRIORITY 4: DEFAULT VALUES
    // Bu normalde compile-time constant'lar olur
    switch(prop) {
        case STYLE_PROP_BG_COLOR:
            res.value.color = COLOR_WHITE;
            res.found = true;
            res.source = STYLE_RES_DEFAULT;
            break;
        case STYLE_PROP_BG_OPA:
            res.value.opa = OPACITY_COVER;
            res.found = true;
            res.source = STYLE_RES_DEFAULT;
            break;
        case STYLE_PROP_TEXT_COLOR:
            res.value.color = COLOR_BLACK;
            res.found = true;
            res.source = STYLE_RES_DEFAULT;
            break;
        case STYLE_PROP_TEXT_SIZE:
            res.value.num = 16;
            res.found = true;
            res.source = STYLE_RES_DEFAULT;
            break;
        default:
            break;
    }

    return res;
}

static style_res_t widget_get_style_prop(widget_t* widget, style_prop_t prop,
                                         widget_part_t part)
{
    return widget_get_style_prop_impl(widget, prop, part, true);
}

// Debug: tüm resolved style'ları yazdır
void widget_print_resolved_styles(widget_t* widget, widget_part_t part)
{
    const char* source_names[] = {
        "NOT_FOUND", "INLINE", "LOCAL", "INHERITED", "DEFAULT"
    };

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ RESOLVED STYLES: Widget #%u (part=0x%08X, state=0x%04X)\n",
           widget->id, part, widget->state);
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    for(style_prop_t prop = 0; prop < STYLE_PROP_COUNT; prop++) {
        style_res_t res = widget_get_style_prop(widget, prop, part);

        if(res.found) {
            printf("  %-20s = ", STYLE_PROP_NAMES[prop]);

            // Property value yazdır
            switch(prop) {
                case STYLE_PROP_BG_COLOR:
                case STYLE_PROP_BORDER_COLOR:
                case STYLE_PROP_TEXT_COLOR:
                case STYLE_PROP_SHADOW_COLOR:
                    printf("RGB(%3u, %3u, %3u)",
                           res.value.color.r,
                           res.value.color.g,
                           res.value.color.b);
                    break;

                case STYLE_PROP_BG_OPA:
                case STYLE_PROP_BORDER_OPA:
                case STYLE_PROP_TEXT_OPA:
                case STYLE_PROP_SHADOW_OPA:
                    printf("%3u", res.value.opa);
                    break;

                default:
                    printf("%d", res.value.num);
                    break;
            }

            printf("  [%s", source_names[res.source]);
            if(STYLE_PROP_INHERITABLE[prop]) {
                printf(", inheritable");
            }
            printf("]\n");
        }
    }
}

/*******************************************************************************
 * PART 10: TRANSITION SYSTEM
 *
 * State değişiminde property'ler animate edilebilir.
 * LVGL Ref: src/misc/lv_anim.c, src/core/lv_obj_style.c:1080-1186
 ******************************************************************************/

typedef struct {
    widget_t* widget;
    style_prop_t prop;
    style_value_t start_value;
    style_value_t end_value;
    uint32_t duration_ms;
    uint32_t elapsed_ms;
} transition_t;

void transition_simulate(widget_t* widget, style_prop_t prop, widget_part_t part)
{
    style_res_t old_res = widget_get_style_prop(widget, prop, part);

    if(!old_res.found) return;

    // Transition time var mı?
    style_res_t trans_res = widget_get_style_prop(widget, STYLE_PROP_TRANSITION_TIME, part);
    if(!trans_res.found || trans_res.value.num == 0) return;

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ TRANSITION SIMULATION: Widget #%u\n", widget->id);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("  Property: %s\n", STYLE_PROP_NAMES[prop]);
    printf("  Duration: %d ms\n", trans_res.value.num);

    // Basit lineer interpolasyon simülasyonu
    int steps = 5;
    for(int i = 0; i <= steps; i++) {
        float progress = (float)i / steps;
        printf("    Progress: %3d%% - ", (int)(progress * 100));

        switch(prop) {
            case STYLE_PROP_BG_OPA:
            case STYLE_PROP_BORDER_OPA:
            case STYLE_PROP_TEXT_OPA:
            case STYLE_PROP_SHADOW_OPA: {
                // Opacity interpolation
                uint8_t interpolated = (uint8_t)(old_res.value.opa * (1.0f - progress) +
                                                 255 * progress);
                printf("Opacity = %u\n", interpolated);
                break;
            }

            default:
                printf("(interpolating...)\n");
                break;
        }
    }
}

/*******************************************************************************
 * DEMO SCENARIOS
 ******************************************************************************/

void print_header(const char* title)
{
    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("  %s\n", title);
    printf("════════════════════════════════════════════════════════════════\n");
}

void demo_scenario_1_basic_styles(void)
{
    print_header("SCENARIO 1: BASIC STYLE CREATION");

    printf("\n>> Creating basic styles with properties\n");

    style_t style1;
    style_init(&style1);
    style_set_bg_color(&style1, COLOR_BLUE);
    style_set_bg_opa(&style1, OPACITY_COVER);
    style_set_radius(&style1, 10);
    style_set_border_color(&style1, COLOR_WHITE);
    style_set_border_width(&style1, 2);
    style_set_pad_all(&style1, 15);

    printf("  Style 1 properties:\n");
    printf("    - bg_color: BLUE\n");
    printf("    - bg_opa: 255\n");
    printf("    - radius: 10\n");
    printf("    - border: WHITE, width=2\n");
    printf("    - padding: 15\n");

    printf("\n>> Getting property from style\n");
    style_value_t val;
    if(style_get_prop(&style1, STYLE_PROP_BG_RADIUS, &val)) {
        printf("  Retrieved radius: %d ✓\n", val.num);
    }
}

void demo_scenario_2_selectors(void)
{
    print_header("SCENARIO 2: SELECTOR MATCHING");

    printf("\n>> Testing selector matching (Part + State)\n");

    style_selector_t sel1 = SELECTOR(PART_MAIN, STATE_DEFAULT);
    style_selector_t sel2 = SELECTOR(PART_MAIN, STATE_PRESSED);
    style_selector_t sel3 = SELECTOR(PART_INDICATOR, STATE_DEFAULT);

    widget_state_t current_state = STATE_PRESSED;
    widget_part_t current_part = PART_MAIN;

    printf("  Current: part=PART_MAIN, state=STATE_PRESSED\n\n");

    printf("  Selector 1 (PART_MAIN | STATE_DEFAULT): %s\n",
           selector_matches(sel1, current_part, current_state) ? "MATCH" : "NO MATCH");
    printf("  Selector 2 (PART_MAIN | STATE_PRESSED): %s\n",
           selector_matches(sel2, current_part, current_state) ? "MATCH ✓" : "NO MATCH");
    printf("  Selector 3 (PART_INDICATOR | STATE_DEFAULT): %s\n",
           selector_matches(sel3, current_part, current_state) ? "MATCH" : "NO MATCH");
}

void demo_scenario_3_cascading(void)
{
    print_header("SCENARIO 3: STYLE CASCADING (Priority)");

    printf("\n>> Creating widget with multiple style sources\n");

    // Theme style (lowest priority)
    style_t theme_style;
    style_init(&theme_style);
    style_set_bg_color(&theme_style, COLOR_GRAY);
    style_set_text_color(&theme_style, COLOR_BLACK);
    style_set_pad_all(&theme_style, 10);
    printf("  Theme style: bg=GRAY, text=BLACK, pad=10\n");

    // Local style (medium priority)
    style_t local_style;
    style_init(&local_style);
    style_set_bg_color(&local_style, COLOR_BLUE);
    style_set_border_width(&local_style, 2);
    printf("  Local style: bg=BLUE, border_width=2\n");

    // Widget
    widget_t* widget = widget_create(NULL, 1);

    // Theme style ekle (default selector)
    widget_add_style(widget, &theme_style, SELECTOR_MAIN);

    // Local style ekle
    widget_add_style(widget, &local_style, SELECTOR_MAIN);

    // Inline style (highest priority)
    widget_set_style_bg_color(widget, COLOR_RED);
    printf("  Inline style: bg=RED\n");

    // Resolution
    printf("\n>> Style resolution (priority order):\n");
    printf("  Priority: INLINE > LOCAL > THEME > DEFAULT\n\n");

    style_res_t bg_res = widget_get_style_prop(widget, STYLE_PROP_BG_COLOR, PART_MAIN);
    style_res_t text_res = widget_get_style_prop(widget, STYLE_PROP_TEXT_COLOR, PART_MAIN);
    style_res_t pad_res = widget_get_style_prop(widget, STYLE_PROP_PAD_TOP, PART_MAIN);
    style_res_t border_res = widget_get_style_prop(widget, STYLE_PROP_BORDER_WIDTH, PART_MAIN);

    const char* sources[] = {"NOT_FOUND", "INLINE", "LOCAL", "INHERITED", "DEFAULT"};

    printf("  bg_color: RGB(%u,%u,%u) - Source: %s ✓\n",
           bg_res.value.color.r, bg_res.value.color.g, bg_res.value.color.b,
           sources[bg_res.source]);

    printf("  text_color: RGB(%u,%u,%u) - Source: %s ✓\n",
           text_res.value.color.r, text_res.value.color.g, text_res.value.color.b,
           sources[text_res.source]);

    printf("  pad_top: %d - Source: %s ✓\n",
           pad_res.value.num, sources[pad_res.source]);

    printf("  border_width: %d - Source: %s ✓\n",
           border_res.value.num, sources[border_res.source]);

    free(widget);
}

void demo_scenario_4_inheritance(void)
{
    print_header("SCENARIO 4: STYLE INHERITANCE");

    printf("\n>> Creating parent-child widget hierarchy\n");

    // Parent widget
    widget_t* parent = widget_create(NULL, 1);
    widget_set_style_text_color(parent, COLOR_BLUE);
    widget_set_style_bg_color(parent, COLOR_GRAY);

    printf("  Parent widget #1:\n");
    printf("    - text_color: BLUE (inline)\n");
    printf("    - bg_color: GRAY (inline)\n");

    // Child widget
    widget_t* child = widget_create(parent, 2);

    printf("\n>> Resolving styles on child widget\n");
    printf("  Note: text_color is INHERITABLE, bg_color is NOT\n\n");

    style_res_t text_res = widget_get_style_prop(child, STYLE_PROP_TEXT_COLOR, PART_MAIN);
    style_res_t bg_res = widget_get_style_prop(child, STYLE_PROP_BG_COLOR, PART_MAIN);

    const char* sources[] = {"NOT_FOUND", "INLINE", "LOCAL", "INHERITED", "DEFAULT"};

    printf("  text_color: RGB(%u,%u,%u) - Source: %s ✓ (inherited from parent)\n",
           text_res.value.color.r, text_res.value.color.g, text_res.value.color.b,
           sources[text_res.source]);

    printf("  bg_color: RGB(%u,%u,%u) - Source: %s ✓ (default, not inherited)\n",
           bg_res.value.color.r, bg_res.value.color.g, bg_res.value.color.b,
           sources[bg_res.source]);

    // Child override
    widget_set_style_text_color(child, COLOR_RED);
    printf("\n>> Child overrides text_color to RED\n");

    text_res = widget_get_style_prop(child, STYLE_PROP_TEXT_COLOR, PART_MAIN);
    printf("  text_color: RGB(%u,%u,%u) - Source: %s ✓ (overridden)\n",
           text_res.value.color.r, text_res.value.color.g, text_res.value.color.b,
           sources[text_res.source]);

    free(parent);
    free(child);
}

void demo_scenario_5_theme(void)
{
    print_header("SCENARIO 5: THEME PROVIDER");

    printf("\n>> Creating custom theme\n");

    theme_t theme;
    theme_init(&theme, "MyTheme");

    // Default style (all widgets)
    style_set_text_color(&theme.default_style, COLOR_BLACK);
    style_set_text_size(&theme.default_style, 14);
    printf("  Default style: text=BLACK, size=14\n");

    // Button style
    style_set_bg_color(&theme.button_style, COLOR_BLUE);
    style_set_radius(&theme.button_style, 8);
    style_set_pad_all(&theme.button_style, 10);
    printf("  Button style: bg=BLUE, radius=8, pad=10\n");

    // Button pressed style
    style_set_bg_color(&theme.button_pressed_style, COLOR_GREEN);
    style_set_shadow_width(&theme.button_pressed_style, 5);
    printf("  Button pressed style: bg=GREEN, shadow=5\n");

    // Label style
    style_set_text_color(&theme.label_style, COLOR_GRAY);
    printf("  Label style: text=GRAY\n");

    // Apply theme to widgets
    printf("\n>> Applying theme to widgets\n");

    widget_t* button = widget_create(NULL, 10);
    theme_apply(&theme, button, "button");

    widget_t* label = widget_create(NULL, 11);
    theme_apply(&theme, label, "label");

    // Print resolved styles
    widget_print_resolved_styles(button, PART_MAIN);

    // Simulate button press
    widget_add_state(button, STATE_PRESSED);
    widget_print_resolved_styles(button, PART_MAIN);

    free(button);
    free(label);
}

void demo_scenario_6_transition(void)
{
    print_header("SCENARIO 6: TRANSITION (Animation)");

    printf("\n>> Creating widget with transition\n");

    style_t normal_style;
    style_init(&normal_style);
    style_set_bg_opa(&normal_style, OPACITY_COVER);
    style_set_transition_time(&normal_style, 300);  // 300ms transition

    style_t pressed_style;
    style_init(&pressed_style);
    style_set_bg_opa(&pressed_style, 128);  // Half transparent when pressed

    widget_t* button = widget_create(NULL, 20);
    widget_add_style(button, &normal_style, SELECTOR_MAIN);
    widget_add_style(button, &pressed_style, SELECTOR_MAIN_PRESSED);

    printf("  Normal: bg_opa=255\n");
    printf("  Pressed: bg_opa=128\n");
    printf("  Transition: 300ms\n");

    printf("\n>> Simulating state change: DEFAULT → PRESSED\n");
    widget_add_state(button, STATE_PRESSED);

    transition_simulate(button, STYLE_PROP_BG_OPA, PART_MAIN);

    free(button);
}

void demo_scenario_7_complete_example(void)
{
    print_header("SCENARIO 7: COMPLETE EXAMPLE");

    printf("\n>> Building realistic UI with theme and custom styles\n");

    // 1. Create theme
    theme_t dark_theme;
    theme_init(&dark_theme, "DarkTheme");

    style_set_bg_color(&dark_theme.default_style, COLOR_BLACK);
    style_set_text_color(&dark_theme.default_style, COLOR_WHITE);
    style_set_text_size(&dark_theme.default_style, 16);

    style_set_bg_color(&dark_theme.container_style, (color_t){32, 32, 32});
    style_set_border_color(&dark_theme.container_style, (color_t){64, 64, 64});
    style_set_border_width(&dark_theme.container_style, 1);
    style_set_pad_all(&dark_theme.container_style, 20);

    style_set_bg_color(&dark_theme.button_style, COLOR_BLUE);
    style_set_radius(&dark_theme.button_style, 6);
    style_set_pad_all(&dark_theme.button_style, 12);
    style_set_transition_time(&dark_theme.button_style, 200);

    style_set_bg_color(&dark_theme.button_pressed_style, (color_t){0, 128, 255});
    style_set_shadow_width(&dark_theme.button_pressed_style, 8);
    style_set_shadow_color(&dark_theme.button_pressed_style, COLOR_BLUE);

    // 2. Create widget hierarchy
    widget_t* container = widget_create(NULL, 100);
    theme_apply(&dark_theme, container, "container");

    widget_t* button1 = widget_create(container, 101);
    theme_apply(&dark_theme, button1, "button");

    widget_t* label1 = widget_create(button1, 102);
    theme_apply(&dark_theme, label1, "label");

    // 3. Custom overrides
    printf("\n>> Adding custom inline styles\n");
    widget_set_style_bg_color(button1, COLOR_ORANGE);

    // 4. Print final resolved styles
    printf("\n>> Container styles (themed):\n");
    widget_print_resolved_styles(container, PART_MAIN);

    printf("\n>> Button styles (themed + custom override):\n");
    widget_print_resolved_styles(button1, PART_MAIN);

    printf("\n>> Label styles (themed + inherited from button):\n");
    widget_print_resolved_styles(label1, PART_MAIN);

    // 5. State change with transition
    printf("\n>> User presses button:\n");
    widget_add_state(button1, STATE_PRESSED);
    widget_print_resolved_styles(button1, PART_MAIN);

    free(container);
    free(button1);
    free(label1);
}

/*******************************************************************************
 * MAIN
 ******************************************************************************/

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                ║\n");
    printf("║         LVGL Style ve Theme System Architecture Demo          ║\n");
    printf("║                                                                ║\n");
    printf("║  Bu demo LVGL'nin style sistem mimarisini gösterir:           ║\n");
    printf("║  1. Style Properties (lv_style_t)                             ║\n");
    printf("║  2. Selector System (Part + State)                            ║\n");
    printf("║  3. Style Cascading (Priority)                                ║\n");
    printf("║  4. Style Inheritance                                         ║\n");
    printf("║  5. Theme Provider                                            ║\n");
    printf("║  6. Transition System                                         ║\n");
    printf("║                                                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    demo_scenario_1_basic_styles();
    demo_scenario_2_selectors();
    demo_scenario_3_cascading();
    demo_scenario_4_inheritance();
    demo_scenario_5_theme();
    demo_scenario_6_transition();
    demo_scenario_7_complete_example();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      DEMO COMPLETED                            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
