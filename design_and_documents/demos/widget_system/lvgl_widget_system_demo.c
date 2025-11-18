/**
 * ============================================================================
 * LVGL Widget System ve Lifecycle Mimari Demo
 * ============================================================================
 *
 * Bu demo, LVGL'nin widget altyapısını (lv_obj tabanlı sistem) tamamen
 * bağımsız bir şekilde gösterir. Gerçek çizim yapmaz, sadece printf ile
 * widget lifecycle ve event sistemini açıklar.
 *
 * Mimari Bileşenler:
 * 1. Base Widget (lv_obj benzeri)
 * 2. Widget Class System (Inheritance)
 * 3. Constructor/Destructor Lifecycle
 * 4. Event System (Observer Pattern)
 * 5. Event Bubbling (Chain of Responsibility)
 * 6. State Management
 * 7. Flag System
 * 8. Custom Widgets (Button, Label)
 *
 * Derleme: gcc lvgl_widget_system_demo.c -o demo && ./demo
 *
 * Design Pattern'ler:
 * - Prototype Pattern (Widget class system)
 * - Observer Pattern (Event callbacks)
 * - Chain of Responsibility (Event bubbling)
 * - Composite Pattern (Widget tree)
 * - Template Method Pattern (Constructor/destructor chain)
 * - State Pattern (Widget states)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// ============================================================================
// 1. TEMEL TİPLER VE ENUMS
// ============================================================================

/**
 * Widget States (Bit flags)
 * LVGL'deki lv_state_t'ye denk gelir
 */
typedef enum {
    WIDGET_STATE_DEFAULT  = 0x0000,
    WIDGET_STATE_CHECKED  = 0x0001,  // Bit 0: Checked (checkbox, switch)
    WIDGET_STATE_FOCUSED  = 0x0002,  // Bit 1: Focused
    WIDGET_STATE_PRESSED  = 0x0004,  // Bit 2: Pressed
    WIDGET_STATE_DISABLED = 0x0008,  // Bit 3: Disabled
    WIDGET_STATE_HOVERED  = 0x0010,  // Bit 4: Hovered (mouse over)
} widget_state_t;

/**
 * Widget Flags (Bit flags)
 * LVGL'deki lv_obj_flag_t'ye denk gelir
 */
typedef enum {
    WIDGET_FLAG_CLICKABLE      = (1 << 0),  // Widget tıklanabilir
    WIDGET_FLAG_CHECKABLE      = (1 << 1),  // Checked state'i var
    WIDGET_FLAG_SCROLLABLE     = (1 << 2),  // Scroll edilebilir
    WIDGET_FLAG_HIDDEN         = (1 << 3),  // Gizli
    WIDGET_FLAG_EVENT_BUBBLE   = (1 << 4),  // Event'leri parent'a ilet
} widget_flag_t;

/**
 * Event Codes
 * LVGL'deki lv_event_code_t'ye denk gelir
 */
typedef enum {
    // Input events
    EVENT_PRESSED,          // Widget basıldı
    EVENT_RELEASED,         // Widget bırakıldı
    EVENT_CLICKED,          // Widget tıklandı (press + release)
    EVENT_LONG_PRESSED,     // Uzun basış

    // State change events
    EVENT_VALUE_CHANGED,    // Değer değişti
    EVENT_FOCUSED,          // Focus alındı
    EVENT_DEFOCUSED,        // Focus kaybedildi

    // Lifecycle events
    EVENT_DELETE,           // Widget silinmeden önce
    EVENT_CHILD_CREATED,    // Child yaratıldı
    EVENT_CHILD_DELETED,    // Child silindi

    // Drawing events (simulated)
    EVENT_DRAW_MAIN,        // Ana çizim
    EVENT_DRAW_POST,        // Post çizim (overlay)

    EVENT_ALL,              // Tüm event'ler (wildcard)
} event_code_t;

/**
 * Coordinates
 */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} coords_t;

// ============================================================================
// 2. EVENT SYSTEM
// ============================================================================

// Forward declarations
typedef struct widget_t widget_t;
typedef struct event_t event_t;

/**
 * Event Callback Type
 */
typedef void (*event_cb_t)(event_t* e);

/**
 * Event Descriptor
 * Her widget birden fazla event callback'e sahip olabilir
 */
typedef struct {
    event_cb_t cb;          // Callback function
    event_code_t filter;    // Hangi event'ler için çalışacak
    void* user_data;        // Kullanıcı verisi
} event_dsc_t;

/**
 * Event Structure
 * LVGL'deki lv_event_t'ye denk gelir
 */
struct event_t {
    widget_t* target;           // Event'in kaynağı (ilk hedef)
    widget_t* current_target;   // Şu anda işleyen widget (bubbling için)
    event_code_t code;          // Event tipi
    void* param;                // Event parametresi (opsiyonel)
    void* user_data;            // Callback user data
    bool stop_bubbling;         // Bubbling'i durdur
    bool stop_processing;       // İşlemeyi tamamen durdur
};

// ============================================================================
// 3. WIDGET CLASS SYSTEM
// ============================================================================

/**
 * Widget Class (vtable benzeri yapı)
 * LVGL'deki lv_obj_class_t'ye denk gelir
 */
typedef struct widget_class_t {
    const char* name;                               // Class ismi
    const struct widget_class_t* base_class;        // Kalıtım (inheritance)

    // Virtual methods (function pointers)
    void (*constructor)(widget_t* widget);
    void (*destructor)(widget_t* widget);
    void (*event_handler)(widget_t* widget, event_t* e);

    // Instance size (türetilmiş class için)
    uint32_t instance_size;
} widget_class_t;

/**
 * Base Widget Structure
 * LVGL'deki lv_obj_t'ye denk gelir
 */
struct widget_t {
    const widget_class_t* class_p;  // Class descriptor

    // Hierarchy
    widget_t* parent;
    widget_t** children;
    uint32_t child_count;

    // Coordinates
    coords_t coords;

    // State ve Flags
    widget_state_t state;
    widget_flag_t flags;

    // Event callbacks (dynamic array)
    event_dsc_t* event_callbacks;
    uint32_t event_cb_count;

    // User data
    void* user_data;

    // Internal
    uint32_t id;  // Debug için unique ID
};

// Global widget ID counter
static uint32_t g_widget_id = 0;

// ============================================================================
// 4. WIDGET LIFECYCLE FUNCTIONS
// ============================================================================

/**
 * Widget yaratma (Factory Pattern)
 * LVGL'deki lv_obj_create()'e denk gelir
 */
widget_t* widget_create(widget_t* parent, const widget_class_t* class_p)
{
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ WIDGET CREATE (class: %s)\n", class_p->name);
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    // Memory allocation
    uint32_t size = class_p->instance_size;
    widget_t* widget = (widget_t*)malloc(size);
    if(!widget) {
        printf("  [ERROR] Memory allocation failed!\n");
        return NULL;
    }

    // Zero-initialize (LVGL stratejisi)
    memset(widget, 0, size);

    // Basic initialization
    widget->class_p = class_p;
    widget->parent = parent;
    widget->id = g_widget_id++;
    widget->state = WIDGET_STATE_DEFAULT;
    widget->flags = 0;

    printf("  Widget ID: %d\n", widget->id);
    printf("  Instance size: %d bytes\n", size);

    // Parent'a ekle
    if(parent) {
        parent->child_count++;
        parent->children = (widget_t**)realloc(parent->children,
                                              sizeof(widget_t*) * parent->child_count);
        parent->children[parent->child_count - 1] = widget;

        printf("  Added to parent (ID: %d), parent now has %d children\n",
               parent->id, parent->child_count);

        // CHILD_CREATED event gönder
        event_t e = {0};
        e.target = parent;
        e.current_target = parent;
        e.code = EVENT_CHILD_CREATED;
        e.param = widget;

        printf("  Sending EVENT_CHILD_CREATED to parent...\n");
        // widget_send_event() henüz tanımlı değil, constructor'dan sonra
    }

    // Constructor chain çağır (Template Method Pattern)
    printf("\n  ┌─ CONSTRUCTOR CHAIN ─────────────────────────────────────────┐\n");
    widget_construct_chain(widget);
    printf("  └─────────────────────────────────────────────────────────────┘\n");

    return widget;
}

/**
 * Constructor zincirini çağırır (base → derived)
 * LVGL'deki lv_obj_construct()'a denk gelir
 */
void widget_construct_chain(widget_t* widget)
{
    const widget_class_t* original_class = widget->class_p;

    // BASE CLASS CONSTRUCTOR'INI ÖNCE ÇAĞIR (recursive)
    if(widget->class_p->base_class) {
        printf("    Calling base class (%s) constructor...\n",
               widget->class_p->base_class->name);
        widget->class_p = widget->class_p->base_class;
        widget_construct_chain(widget);  // RECURSIVE
    }

    // Orijinal class'ı geri yükle
    widget->class_p = original_class;

    // KENDI CONSTRUCTOR'INI ÇAĞIR
    if(widget->class_p->constructor) {
        printf("    Calling %s constructor...\n", widget->class_p->name);
        widget->class_p->constructor(widget);
    }
}

/**
 * Widget silme
 * LVGL'deki lv_obj_del()'e denk gelir
 */
void widget_delete(widget_t* widget)
{
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ WIDGET DELETE (ID: %d, class: %s)\n", widget->id, widget->class_p->name);
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    // DELETE event gönder
    event_t e = {0};
    e.target = widget;
    e.current_target = widget;
    e.code = EVENT_DELETE;
    printf("  Sending EVENT_DELETE...\n");
    widget_send_event(widget, &e);

    // Child'ları recursive sil
    if(widget->child_count > 0) {
        printf("  Deleting %d children recursively...\n", widget->child_count);
        for(uint32_t i = 0; i < widget->child_count; i++) {
            widget_delete(widget->children[i]);
        }
        free(widget->children);
    }

    // Destructor chain çağır (derived → base)
    printf("\n  ┌─ DESTRUCTOR CHAIN ──────────────────────────────────────────┐\n");
    widget_destruct_chain(widget);
    printf("  └─────────────────────────────────────────────────────────────┘\n");

    // Event callbacks'i serbest bırak
    if(widget->event_callbacks) {
        printf("  Freeing %d event callbacks...\n", widget->event_cb_count);
        free(widget->event_callbacks);
    }

    // Parent'tan çıkar
    if(widget->parent) {
        printf("  Removing from parent (ID: %d)...\n", widget->parent->id);
        widget_t* parent = widget->parent;

        // Parent'ın children listesinden çıkar
        for(uint32_t i = 0; i < parent->child_count; i++) {
            if(parent->children[i] == widget) {
                // Shift remaining children
                for(uint32_t j = i; j < parent->child_count - 1; j++) {
                    parent->children[j] = parent->children[j + 1];
                }
                parent->child_count--;
                break;
            }
        }

        // CHILD_DELETED event gönder
        event_t del_e = {0};
        del_e.target = parent;
        del_e.current_target = parent;
        del_e.code = EVENT_CHILD_DELETED;
        del_e.param = widget;
        widget_send_event(parent, &del_e);
    }

    // Widget belleğini serbest bırak
    printf("  Freeing widget memory...\n");
    free(widget);

    printf("  Widget deleted successfully.\n");
}

/**
 * Destructor zincirini çağırır (derived → base)
 * LVGL'deki _lv_obj_destruct()'a denk gelir
 */
void widget_destruct_chain(widget_t* widget)
{
    // KENDI DESTRUCTOR'INI ÖNCE ÇAĞIR
    if(widget->class_p->destructor) {
        printf("    Calling %s destructor...\n", widget->class_p->name);
        widget->class_p->destructor(widget);
    }

    // BASE CLASS DESTRUCTOR'INI ÇAĞIR (recursive)
    if(widget->class_p->base_class) {
        printf("    Calling base class (%s) destructor...\n",
               widget->class_p->base_class->name);
        widget->class_p = widget->class_p->base_class;
        widget_destruct_chain(widget);  // RECURSIVE
    }
}

// ============================================================================
// 5. WIDGET UTILITY FUNCTIONS
// ============================================================================

/**
 * Widget pozisyonunu ayarla
 */
void widget_set_pos(widget_t* widget, int32_t x, int32_t y, int32_t w, int32_t h)
{
    printf("\n>>> SET POSITION (Widget ID: %d)\n", widget->id);
    printf("    Coords: [%d, %d, %dx%d]\n", x, y, w, h);

    widget->coords.x = x;
    widget->coords.y = y;
    widget->coords.width = w;
    widget->coords.height = h;
}

/**
 * State ekle
 */
void widget_add_state(widget_t* widget, widget_state_t state)
{
    printf("\n>>> ADD STATE (Widget ID: %d)\n", widget->id);
    printf("    Old state: 0x%04X\n", widget->state);

    widget->state |= state;

    printf("    New state: 0x%04X\n", widget->state);
}

/**
 * State çıkar
 */
void widget_clear_state(widget_t* widget, widget_state_t state)
{
    printf("\n>>> CLEAR STATE (Widget ID: %d)\n", widget->id);
    printf("    Old state: 0x%04X\n", widget->state);

    widget->state &= ~state;

    printf("    New state: 0x%04X\n", widget->state);
}

/**
 * State kontrolü
 */
bool widget_has_state(widget_t* widget, widget_state_t state)
{
    return (widget->state & state) == state;
}

/**
 * Flag ekle
 */
void widget_add_flag(widget_t* widget, widget_flag_t flag)
{
    printf("\n>>> ADD FLAG (Widget ID: %d)\n", widget->id);
    printf("    Old flags: 0x%04X\n", widget->flags);

    widget->flags |= flag;

    printf("    New flags: 0x%04X\n", widget->flags);
}

/**
 * Flag çıkar
 */
void widget_clear_flag(widget_t* widget, widget_flag_t flag)
{
    printf("\n>>> CLEAR FLAG (Widget ID: %d)\n", widget->id);
    printf("    Old flags: 0x%04X\n", widget->flags);

    widget->flags &= ~flag;

    printf("    New flags: 0x%04X\n", widget->flags);
}

/**
 * Flag kontrolü
 */
bool widget_has_flag(widget_t* widget, widget_flag_t flag)
{
    return (widget->flags & flag) == flag;
}

// ============================================================================
// 6. EVENT SYSTEM IMPLEMENTATION
// ============================================================================

/**
 * Event callback ekle (Observer Pattern)
 * LVGL'deki lv_obj_add_event_cb()'ye denk gelir
 */
void widget_add_event_cb(widget_t* widget, event_cb_t cb, event_code_t filter, void* user_data)
{
    printf("\n>>> ADD EVENT CALLBACK (Widget ID: %d)\n", widget->id);
    printf("    Filter: %d\n", filter);

    widget->event_cb_count++;
    widget->event_callbacks = (event_dsc_t*)realloc(widget->event_callbacks,
                                                    sizeof(event_dsc_t) * widget->event_cb_count);

    event_dsc_t* dsc = &widget->event_callbacks[widget->event_cb_count - 1];
    dsc->cb = cb;
    dsc->filter = filter;
    dsc->user_data = user_data;

    printf("    Callback added (total: %d callbacks)\n", widget->event_cb_count);
}

/**
 * Event gönder ve işle
 * LVGL'deki lv_event_send()'e denk gelir
 */
void widget_send_event(widget_t* widget, event_t* e)
{
    printf("\n┌─────────────────────────────────────────────────────────────────┐\n");
    printf("│ EVENT DISPATCH: %d (target: Widget ID %d)\n", e->code, widget->id);
    printf("└─────────────────────────────────────────────────────────────────┘\n");

    e->target = widget;
    e->current_target = widget;
    e->stop_bubbling = false;
    e->stop_processing = false;

    widget_process_event(widget, e);

    // EVENT BUBBLING (Chain of Responsibility Pattern)
    if(!e->stop_bubbling && widget->parent &&
       widget_has_flag(widget, WIDGET_FLAG_EVENT_BUBBLE)) {

        printf("\n  ┌─ EVENT BUBBLING TO PARENT ───────────────────────────────┐\n");
        printf("    Bubbling to parent (ID: %d)...\n", widget->parent->id);

        e->current_target = widget->parent;
        widget_process_event(widget->parent, e);

        printf("  └──────────────────────────────────────────────────────────┘\n");
    }
}

/**
 * Event'i işle (callback'leri çağır)
 */
void widget_process_event(widget_t* widget, event_t* e)
{
    printf("  Processing event on Widget ID %d...\n", widget->id);

    // Class event handler (virtual method)
    if(widget->class_p->event_handler && !e->stop_processing) {
        printf("    → Calling class event handler (%s)...\n", widget->class_p->name);
        widget->class_p->event_handler(widget, e);
    }

    // Instance event callbacks (observers)
    for(uint32_t i = 0; i < widget->event_cb_count && !e->stop_processing; i++) {
        event_dsc_t* dsc = &widget->event_callbacks[i];

        // Filter kontrolü
        if(dsc->filter == EVENT_ALL || dsc->filter == e->code) {
            printf("    → Calling instance callback #%d (filter: %d)...\n",
                   i, dsc->filter);

            e->user_data = dsc->user_data;
            dsc->cb(e);
        }
    }
}

/**
 * Event bubbling'i durdur
 */
void event_stop_bubbling(event_t* e)
{
    printf("      [EVENT] Bubbling stopped\n");
    e->stop_bubbling = true;
}

/**
 * Event işlemeyi tamamen durdur
 */
void event_stop_processing(event_t* e)
{
    printf("      [EVENT] Processing stopped\n");
    e->stop_processing = true;
}

// ============================================================================
// 7. BASE WIDGET CLASS
// ============================================================================

/**
 * Base widget constructor
 */
void base_widget_constructor(widget_t* widget)
{
    printf("      [BASE] Initializing base widget properties\n");
    // Varsayılan değerler zaten zero-init ile ayarlandı
}

/**
 * Base widget destructor
 */
void base_widget_destructor(widget_t* widget)
{
    printf("      [BASE] Cleaning up base widget\n");
}

/**
 * Base widget event handler
 */
void base_widget_event_handler(widget_t* widget, event_t* e)
{
    printf("      [BASE] Event %d received\n", e->code);

    if(e->code == EVENT_DRAW_MAIN) {
        printf("      [BASE] Drawing base widget background at [%d,%d %dx%d]\n",
               widget->coords.x, widget->coords.y,
               widget->coords.width, widget->coords.height);
    }
}

// Base widget class definition
const widget_class_t base_widget_class = {
    .name = "widget",
    .base_class = NULL,
    .constructor = base_widget_constructor,
    .destructor = base_widget_destructor,
    .event_handler = base_widget_event_handler,
    .instance_size = sizeof(widget_t),
};

// ============================================================================
// 8. BUTTON WIDGET
// ============================================================================

/**
 * Button widget (extends base widget)
 */
typedef struct {
    widget_t widget;  // Base class (FIRST MEMBER - inheritance trick)
    char label[64];   // Button text
    bool toggled;     // Toggle state
} button_t;

/**
 * Button constructor
 */
void button_constructor(widget_t* widget)
{
    printf("      [BUTTON] Initializing button widget\n");

    button_t* btn = (button_t*)widget;
    strcpy(btn->label, "Button");
    btn->toggled = false;

    // Button varsayılan flag'leri
    widget_add_flag(widget, WIDGET_FLAG_CLICKABLE);
    widget_add_flag(widget, WIDGET_FLAG_EVENT_BUBBLE);
}

/**
 * Button destructor
 */
void button_destructor(widget_t* widget)
{
    printf("      [BUTTON] Cleaning up button\n");
}

/**
 * Button event handler
 */
void button_event_handler(widget_t* widget, event_t* e)
{
    button_t* btn = (button_t*)widget;

    printf("      [BUTTON] Event %d received\n", e->code);

    if(e->code == EVENT_PRESSED) {
        printf("      [BUTTON] Button pressed: \"%s\"\n", btn->label);
        widget_add_state(widget, WIDGET_STATE_PRESSED);
    }
    else if(e->code == EVENT_RELEASED) {
        printf("      [BUTTON] Button released: \"%s\"\n", btn->label);
        widget_clear_state(widget, WIDGET_STATE_PRESSED);
    }
    else if(e->code == EVENT_CLICKED) {
        printf("      [BUTTON] Button clicked: \"%s\"\n", btn->label);

        // Checkable ise toggle yap
        if(widget_has_flag(widget, WIDGET_FLAG_CHECKABLE)) {
            btn->toggled = !btn->toggled;

            if(btn->toggled) {
                widget_add_state(widget, WIDGET_STATE_CHECKED);
            } else {
                widget_clear_state(widget, WIDGET_STATE_CHECKED);
            }

            // VALUE_CHANGED event gönder
            event_t ve = {0};
            ve.code = EVENT_VALUE_CHANGED;
            ve.param = &btn->toggled;
            widget_send_event(widget, &ve);
        }
    }
    else if(e->code == EVENT_DRAW_MAIN) {
        printf("      [BUTTON] Drawing button \"%s\" at [%d,%d %dx%d]\n",
               btn->label,
               widget->coords.x, widget->coords.y,
               widget->coords.width, widget->coords.height);

        if(widget_has_state(widget, WIDGET_STATE_PRESSED)) {
            printf("        (State: PRESSED)\n");
        }
        if(widget_has_state(widget, WIDGET_STATE_CHECKED)) {
            printf("        (State: CHECKED)\n");
        }
    }
}

// Button widget class definition
const widget_class_t button_class = {
    .name = "button",
    .base_class = &base_widget_class,
    .constructor = button_constructor,
    .destructor = button_destructor,
    .event_handler = button_event_handler,
    .instance_size = sizeof(button_t),
};

/**
 * Button helper: Label ayarla
 */
void button_set_label(widget_t* widget, const char* text)
{
    button_t* btn = (button_t*)widget;
    printf("\n>>> SET BUTTON LABEL (Widget ID: %d)\n", widget->id);
    printf("    New label: \"%s\"\n", text);

    strncpy(btn->label, text, sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label) - 1] = '\0';
}

// ============================================================================
// 9. LABEL WIDGET
// ============================================================================

/**
 * Label widget (extends base widget)
 */
typedef struct {
    widget_t widget;  // Base class
    char* text;       // Dynamic text
} label_t;

/**
 * Label constructor
 */
void label_constructor(widget_t* widget)
{
    printf("      [LABEL] Initializing label widget\n");

    label_t* label = (label_t*)widget;
    label->text = strdup("Label");
}

/**
 * Label destructor
 */
void label_destructor(widget_t* widget)
{
    printf("      [LABEL] Cleaning up label\n");

    label_t* label = (label_t*)widget;
    if(label->text) {
        free(label->text);
        label->text = NULL;
    }
}

/**
 * Label event handler
 */
void label_event_handler(widget_t* widget, event_t* e)
{
    label_t* label = (label_t*)widget;

    printf("      [LABEL] Event %d received\n", e->code);

    if(e->code == EVENT_DRAW_MAIN) {
        printf("      [LABEL] Drawing label \"%s\" at [%d,%d %dx%d]\n",
               label->text,
               widget->coords.x, widget->coords.y,
               widget->coords.width, widget->coords.height);
    }
}

// Label widget class definition
const widget_class_t label_class = {
    .name = "label",
    .base_class = &base_widget_class,
    .constructor = label_constructor,
    .destructor = label_destructor,
    .event_handler = label_event_handler,
    .instance_size = sizeof(label_t),
};

/**
 * Label helper: Text ayarla
 */
void label_set_text(widget_t* widget, const char* text)
{
    label_t* label = (label_t*)widget;
    printf("\n>>> SET LABEL TEXT (Widget ID: %d)\n", widget->id);
    printf("    New text: \"%s\"\n", text);

    if(label->text) {
        free(label->text);
    }

    label->text = strdup(text);

    // VALUE_CHANGED event gönder
    event_t e = {0};
    e.code = EVENT_VALUE_CHANGED;
    e.param = label->text;
    widget_send_event(widget, &e);
}

// ============================================================================
// 10. DEMO SCENARIOS
// ============================================================================

/**
 * Global event callback örneği
 */
void global_event_cb(event_t* e)
{
    printf("\n    ┌─ GLOBAL CALLBACK ──────────────────────────────────────┐\n");
    printf("      Event: %d on Widget ID %d\n", e->code, e->current_target->id);

    if(e->code == EVENT_CLICKED) {
        printf("      Global: A widget was clicked!\n");
    }
    else if(e->code == EVENT_VALUE_CHANGED) {
        printf("      Global: A widget value changed!\n");
    }

    printf("    └────────────────────────────────────────────────────────┘\n");
}

/**
 * Button specific event callback
 */
void button_click_cb(event_t* e)
{
    if(e->code == EVENT_CLICKED) {
        button_t* btn = (button_t*)e->current_target;
        printf("\n    ┌─ BUTTON CLICK CALLBACK ────────────────────────────────┐\n");
        printf("      Button \"%s\" was clicked!\n", btn->label);
        printf("    └────────────────────────────────────────────────────────┘\n");
    }
}

/**
 * Render simulation
 */
void simulate_render(widget_t* widget)
{
    printf("\n");
    printf("████████████████████████████████████████████████████████████████\n");
    printf("█                    RENDER SIMULATION                        █\n");
    printf("████████████████████████████████████████████████████████████████\n");

    event_t draw_e = {0};
    draw_e.code = EVENT_DRAW_MAIN;

    widget_send_event(widget, &draw_e);

    // Recursive child rendering
    for(uint32_t i = 0; i < widget->child_count; i++) {
        simulate_render(widget->children[i]);
    }
}

// ============================================================================
// 11. MAIN DEMO
// ============================================================================

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                ║\n");
    printf("║         LVGL Widget System ve Lifecycle Demo                  ║\n");
    printf("║                                                                ║\n");
    printf("║  Bu demo LVGL'nin widget altyapısını gösterir:                ║\n");
    printf("║  1. Widget Class System (Inheritance)                         ║\n");
    printf("║  2. Constructor/Destructor Lifecycle                          ║\n");
    printf("║  3. Event System (Observer Pattern)                           ║\n");
    printf("║  4. Event Bubbling (Chain of Responsibility)                  ║\n");
    printf("║  5. State ve Flag Management                                  ║\n");
    printf("║  6. Custom Widgets (Button, Label)                            ║\n");
    printf("║                                                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("                    SCENARIO 1: WIDGET TREE                    \n");
    printf("════════════════════════════════════════════════════════════════\n");

    // Screen (root widget)
    widget_t* screen = widget_create(NULL, &base_widget_class);
    widget_set_pos(screen, 0, 0, 800, 480);

    // Panel
    widget_t* panel = widget_create(screen, &base_widget_class);
    widget_set_pos(panel, 50, 50, 300, 200);

    // Button 1
    widget_t* button1 = widget_create(panel, &button_class);
    widget_set_pos(button1, 70, 70, 100, 40);
    button_set_label(button1, "Click Me");

    // Button 2 (checkable)
    widget_t* button2 = widget_create(panel, &button_class);
    widget_set_pos(button2, 70, 130, 100, 40);
    button_set_label(button2, "Toggle");
    widget_add_flag(button2, WIDGET_FLAG_CHECKABLE);

    // Label
    widget_t* label = widget_create(panel, &label_class);
    widget_set_pos(label, 190, 80, 100, 30);
    label_set_text(label, "Status: Ready");

    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("               SCENARIO 2: EVENT CALLBACKS                     \n");
    printf("════════════════════════════════════════════════════════════════\n");

    // Global callback (tüm event'ler için)
    widget_add_event_cb(button1, global_event_cb, EVENT_ALL, NULL);

    // Specific button click callback
    widget_add_event_cb(button1, button_click_cb, EVENT_CLICKED, NULL);

    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("            SCENARIO 3: SIMULATED USER INTERACTION             \n");
    printf("════════════════════════════════════════════════════════════════\n");

    // Button 1'e tıklama simülasyonu
    printf("\n[USER ACTION] Clicking Button 1...\n");
    event_t press_e = {0};
    press_e.code = EVENT_PRESSED;
    widget_send_event(button1, &press_e);

    event_t release_e = {0};
    release_e.code = EVENT_RELEASED;
    widget_send_event(button1, &release_e);

    event_t click_e = {0};
    click_e.code = EVENT_CLICKED;
    widget_send_event(button1, &click_e);

    // Button 2 toggle simülasyonu
    printf("\n\n[USER ACTION] Toggling Button 2...\n");
    event_t toggle_click = {0};
    toggle_click.code = EVENT_CLICKED;
    widget_send_event(button2, &toggle_click);

    printf("\n\n[USER ACTION] Toggling Button 2 again...\n");
    widget_send_event(button2, &toggle_click);

    // Label text güncelleme
    printf("\n\n[USER ACTION] Updating label text...\n");
    label_set_text(label, "Status: Active");

    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("                 SCENARIO 4: RENDERING                         \n");
    printf("════════════════════════════════════════════════════════════════\n");

    simulate_render(screen);

    printf("\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("              SCENARIO 5: WIDGET DELETION                      \n");
    printf("════════════════════════════════════════════════════════════════\n");

    // Tek bir widget silme
    printf("\n[USER ACTION] Deleting Button 2...\n");
    widget_delete(button2);

    // Tüm tree'yi silme (screen silinince tüm child'lar da silinir)
    printf("\n\n[USER ACTION] Deleting entire screen (recursive)...\n");
    widget_delete(screen);

    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      DEMO COMPLETED                            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Key Takeaways:\n");
    printf("  ✓ Widget inheritance via base_class pointer\n");
    printf("  ✓ Constructor/destructor chain (base → derived, derived → base)\n");
    printf("  ✓ Event system with multiple callbacks per widget\n");
    printf("  ✓ Event bubbling to parent (Chain of Responsibility)\n");
    printf("  ✓ State management with bit flags\n");
    printf("  ✓ Automatic parent-child hierarchy management\n");
    printf("\n");
    printf("LVGL Design Patterns Demonstrated:\n");
    printf("  • Prototype Pattern (widget class system)\n");
    printf("  • Observer Pattern (event callbacks)\n");
    printf("  • Chain of Responsibility (event bubbling)\n");
    printf("  • Composite Pattern (widget tree)\n");
    printf("  • Template Method Pattern (constructor/destructor chain)\n");
    printf("  • State Pattern (widget states)\n");
    printf("\n");

    return 0;
}
