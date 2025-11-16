/**
 * @file 05_all_strategies_comparison.c
 * @brief Tüm Event Stratejilerinin Karşılaştırması
 *
 * Bu demo 4 stratejiyi birleştirerek gösterir:
 * 1. Bubbling
 * 2. Filtering
 * 3. Preprocessing
 * 4. Nested Events
 *
 * COMPILE: gcc 05_all_strategies_comparison.c -o comparison_demo
 * RUN: ./comparison_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVENT_ALL = 0,
    EVENT_CLICK,
    EVENT_HOVER,
    EVENT_VALUE_CHANGED
} event_type_t;

typedef struct widget widget_t;
typedef struct event event_t;

#define MAX_HANDLERS 10

typedef struct {
    void (*callback)(event_t* e);
    event_type_t filter;
    bool is_preprocess;
} handler_desc_t;

struct event {
    event_type_t type;
    widget_t* target;
    widget_t* current_target;
    void* param;
    bool stop_bubbling;
    bool stop_processing;
    bool deleted;
    struct event* prev;  // Nested event stack
};

struct widget {
    char name[32];
    widget_t* parent;
    widget_t* children[10];
    int child_count;
    handler_desc_t handlers[MAX_HANDLERS];
    int handler_count;
    bool enable_bubbling;
    int value;
};

static event_t* event_head = NULL;

widget_t* widget_create(const char* name, widget_t* parent) {
    widget_t* w = (widget_t*)calloc(1, sizeof(widget_t));
    strncpy(w->name, name, sizeof(w->name) - 1);
    w->parent = parent;
    w->enable_bubbling = true;
    if (parent) parent->children[parent->child_count++] = w;
    return w;
}

void widget_add_handler(widget_t* w, void (*handler)(event_t*), 
                        event_type_t filter, bool is_preprocess) {
    if (w->handler_count < MAX_HANDLERS) {
        w->handlers[w->handler_count].callback = handler;
        w->handlers[w->handler_count].filter = filter;
        w->handlers[w->handler_count].is_preprocess = is_preprocess;
        w->handler_count++;
    }
}

void event_send(widget_t* target, event_type_t type, void* param) {
    const char* names[] = {"ALL", "CLICK", "HOVER", "VALUE_CHANGED"};
    
    event_t e = {
        .type = type,
        .target = target,
        .current_target = target,
        .param = param,
        .stop_bubbling = false,
        .stop_processing = false,
        .deleted = false,
        .prev = event_head
    };
    
    event_head = &e;
    
    printf("\n[EVENT] %s on '%s'\n", names[type], target->name);
    
    widget_t* current = target;
    int level = 0;
    
    while (current && !e.stop_bubbling) {
        e.current_target = current;
        
        if (level > 0) {
            printf("  [BUBBLE] Level %d → '%s'\n", level, current->name);
        }
        
        // PREPROCESS PHASE
        for (int i = 0; i < current->handler_count && !e.stop_processing; i++) {
            handler_desc_t* h = &current->handlers[i];
            if (h->is_preprocess && (h->filter == EVENT_ALL || h->filter == type)) {
                printf("    [PRE] Handler %d\n", i + 1);
                h->callback(&e);
            }
        }
        
        if (e.stop_processing) break;
        
        // REGULAR PHASE
        for (int i = 0; i < current->handler_count && !e.stop_processing; i++) {
            handler_desc_t* h = &current->handlers[i];
            if (!h->is_preprocess && (h->filter == EVENT_ALL || h->filter == type)) {
                printf("    [REG] Handler %d\n", i + 1);
                h->callback(&e);
            }
        }
        
        if (e.stop_processing || !current->enable_bubbling) break;
        
        current = current->parent;
        level++;
    }
    
    event_head = e.prev;
}

void click_handler(event_t* e) {
    printf("      → Click handled on '%s'\n", e->current_target->name);
}

void all_handler(event_t* e) {
    const char* names[] = {"ALL", "CLICK", "HOVER", "VALUE_CHANGED"};
    printf("      → All handler caught %s\n", names[e->type]);
}

void preprocess_handler(event_t* e) {
    printf("      → Preprocess: validating...\n");
}

void nested_trigger(event_t* e) {
    printf("      → Triggering nested VALUE_CHANGED\n");
    event_send(e->target, EVENT_VALUE_CHANGED, NULL);
    printf("      → Returned from nested event\n");
}

void value_handler(event_t* e) {
    e->target->value++;
    printf("      → Value changed to %d\n", e->target->value);
}

void demo_combined() {
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║         COMBINED STRATEGIES DEMO             ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    printf("\n[SETUP] Creating hierarchy:\n");
    printf("  Window\n");
    printf("    └─ Panel\n");
    printf("         └─ Button\n");
    
    widget_t* window = widget_create("Window", NULL);
    widget_t* panel = widget_create("Panel", window);
    widget_t* button = widget_create("Button", panel);
    
    printf("\n[SETUP] Registering handlers:\n");
    printf("  Button:\n");
    printf("    - Preprocess handler (validation)\n");
    printf("    - Click handler (specific)\n");
    printf("    - Nested trigger handler\n");
    printf("  Panel:\n");
    printf("    - All handler (catches everything)\n");
    printf("  Window:\n");
    printf("    - Click handler (bubbled events)\n");
    
    widget_add_handler(button, preprocess_handler, EVENT_CLICK, true);   // Preprocess + Filter
    widget_add_handler(button, click_handler, EVENT_CLICK, false);       // Regular + Filter
    widget_add_handler(button, nested_trigger, EVENT_CLICK, false);      // Nested event trigger
    widget_add_handler(button, value_handler, EVENT_VALUE_CHANGED, false);
    widget_add_handler(panel, all_handler, EVENT_ALL, false);            // Filter: ALL
    widget_add_handler(window, click_handler, EVENT_CLICK, false);       // Bubbling target
    
    printf("\n[TEST 1] Button CLICK (all strategies):\n");
    event_send(button, EVENT_CLICK, NULL);
    
    printf("\n[TEST 2] Button HOVER (filtered out on button):\n");
    event_send(button, EVENT_HOVER, NULL);
    
    printf("\n═══════════════════════════════════════════════\n");
    printf("  STRATEGY COMPARISON TABLE\n");
    printf("═══════════════════════════════════════════════\n");
    
    printf("\n%-15s %-20s %-15s\n", "Strategy", "Purpose", "Used When");
    printf("%-15s %-20s %-15s\n", "--------", "-------", "---------");
    printf("%-15s %-20s %-15s\n", "Bubbling", "Propagation up", "Delegated handling");
    printf("%-15s %-20s %-15s\n", "Filtering", "Selective handling", "Specific events");
    printf("%-15s %-20s %-15s\n", "Preprocessing", "Before default", "Validation");
    printf("%-15s %-20s %-15s\n", "Nested Events", "Event in event", "Cascading updates");
    
    printf("\n═══════════════════════════════════════════════\n");
    printf("  PERFORMANCE COMPARISON\n");
    printf("═══════════════════════════════════════════════\n");
    
    printf("\nScenario: 100 buttons, 1 container\n\n");
    
    printf("WITHOUT Bubbling:\n");
    printf("  - 100 handler registrations\n");
    printf("  - Memory: 100 × handler_size\n");
    printf("  - Management: Complex\n\n");
    
    printf("WITH Bubbling:\n");
    printf("  - 1 handler registration\n");
    printf("  - Memory: 1 × handler_size\n");
    printf("  - Management: Simple\n");
    printf("  - Savings: 99%%\n\n");
    
    printf("WITHOUT Filtering:\n");
    printf("  - Handler called for ALL events\n");
    printf("  - Must check type manually\n");
    printf("  - CPU: Wasted on irrelevant events\n\n");
    
    printf("WITH Filtering:\n");
    printf("  - Handler called ONLY for specific event\n");
    printf("  - No manual checking\n");
    printf("  - CPU: Efficient\n\n");
    
    printf("WITHOUT Preprocessing:\n");
    printf("  - Can't intercept before default\n");
    printf("  - Validation after action\n");
    printf("  - Requires undo logic\n\n");
    
    printf("WITH Preprocessing:\n");
    printf("  - Intercept BEFORE default\n");
    printf("  - Validation before action\n");
    printf("  - No undo needed\n\n");
    
    printf("WITHOUT Nested Event Safety:\n");
    printf("  - Deletion during event = crash\n");
    printf("  - Complex state management\n");
    printf("  - Fragile code\n\n");
    
    printf("WITH Nested Event Safety:\n");
    printf("  - Deletion safe (marked)\n");
    printf("  - Event stack tracked\n");
    printf("  - Robust code\n");
    
    free(button); free(panel); free(window);
}

int main() {
    demo_combined();
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║  FINAL SUMMARY                                ║\n");
    printf("║                                               ║\n");
    printf("║  All 4 strategies work TOGETHER:             ║\n");
    printf("║                                               ║\n");
    printf("║  1. Filtering → Which events to handle       ║\n");
    printf("║  2. Preprocessing → Run before default       ║\n");
    printf("║  3. Bubbling → Propagate to parents          ║\n");
    printf("║  4. Nested Events → Events trigger events    ║\n");
    printf("║                                               ║\n");
    printf("║  Result: Powerful, flexible, safe!           ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    return 0;
}
