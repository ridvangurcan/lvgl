/**
 * @file 02_event_filtering_demo.c
 * @brief Event Filtering (Olay Filtreleme) Demo
 *
 * KONSEPT:
 * - Handler'lar belirli event tiplerini dinleyebilir
 * - EVENT_ALL ile tüm event'ler yakalanabilir
 * - Filtre eşleşmezse handler çağrılmaz
 *
 * COMPILE: gcc 02_event_filtering_demo.c -o filtering_demo
 * RUN: ./filtering_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVENT_ALL = 0,
    EVENT_CLICK = 1,
    EVENT_HOVER = 2,
    EVENT_KEY_PRESS = 3,
    EVENT_VALUE_CHANGED = 4,
    EVENT_FOCUS = 5
} event_type_t;

typedef struct widget widget_t;

typedef struct {
    event_type_t type;
    widget_t* target;
    void* param;
} event_t;

typedef void (*event_handler_t)(event_t* e);

#define MAX_HANDLERS 10

typedef struct {
    event_handler_t callback;
    event_type_t filter;  // EVENT_CLICK, EVENT_ALL, etc.
    void* user_data;
} event_descriptor_t;

struct widget {
    char name[32];
    event_descriptor_t handlers[MAX_HANDLERS];
    int handler_count;
};

widget_t* widget_create(const char* name) {
    widget_t* w = (widget_t*)calloc(1, sizeof(widget_t));
    strncpy(w->name, name, sizeof(w->name) - 1);
    return w;
}

void widget_add_handler(widget_t* w, event_handler_t handler, event_type_t filter, void* user_data) {
    const char* filter_names[] = {"ALL", "CLICK", "HOVER", "KEY_PRESS", "VALUE_CHANGED", "FOCUS"};
    
    if (w->handler_count < MAX_HANDLERS) {
        w->handlers[w->handler_count].callback = handler;
        w->handlers[w->handler_count].filter = filter;
        w->handlers[w->handler_count].user_data = user_data;
        w->handler_count++;
        
        printf("  [REGISTER] Handler #%d for %s events\n", 
               w->handler_count, filter_names[filter]);
    }
}

void event_send(widget_t* target, event_type_t type, void* param) {
    const char* event_names[] = {"ALL", "CLICK", "HOVER", "KEY_PRESS", "VALUE_CHANGED", "FOCUS"};
    
    printf("\n[EVENT] Sending %s to '%s'\n", event_names[type], target->name);
    
    event_t e = {.type = type, .target = target, .param = param};
    int called = 0;
    
    for (int i = 0; i < target->handler_count; i++) {
        event_descriptor_t* desc = &target->handlers[i];
        
        // Filter matching
        if (desc->filter == EVENT_ALL || desc->filter == type) {
            printf("  [MATCH] Handler #%d (filter=%s) → CALLING\n",
                   i + 1, event_names[desc->filter]);
            desc->callback(&e);
            called++;
        } else {
            printf("  [SKIP] Handler #%d (filter=%s ≠ %s)\n",
                   i + 1, event_names[desc->filter], event_names[type]);
        }
    }
    
    printf("[EVENT] %d/%d handlers called\n", called, target->handler_count);
}

void click_handler(event_t* e) {
    printf("    [CLICK-HANDLER] Processing click on '%s'\n", e->target->name);
}

void hover_handler(event_t* e) {
    printf("    [HOVER-HANDLER] Processing hover on '%s'\n", e->target->name);
}

void all_handler(event_t* e) {
    const char* event_names[] = {"ALL", "CLICK", "HOVER", "KEY_PRESS", "VALUE_CHANGED", "FOCUS"};
    printf("    [ALL-HANDLER] Caught %s event on '%s'\n", 
           event_names[e->type], e->target->name);
}

void value_handler(event_t* e) {
    int* value = (int*)e->param;
    printf("    [VALUE-HANDLER] Value changed to %d on '%s'\n",
           value ? *value : 0, e->target->name);
}

void demo_single_filter() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 1: Single Event Filter\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* btn = widget_create("Button");
    
    printf("\n[SETUP] Registering click-only handler:\n");
    widget_add_handler(btn, click_handler, EVENT_CLICK, NULL);
    
    printf("\n[TEST 1] Send CLICK event:\n");
    event_send(btn, EVENT_CLICK, NULL);
    
    printf("\n[TEST 2] Send HOVER event:\n");
    event_send(btn, EVENT_HOVER, NULL);
    
    printf("\n[SONUÇ] Handler sadece CLICK'i yakaladı, HOVER'ı yakalamadı ✓\n");
    
    free(btn);
}

void demo_multiple_filters() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 2: Multiple Handlers with Different Filters\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* widget = widget_create("MultiWidget");
    
    printf("\n[SETUP] Registering 3 handlers:\n");
    widget_add_handler(widget, click_handler, EVENT_CLICK, NULL);
    widget_add_handler(widget, hover_handler, EVENT_HOVER, NULL);
    widget_add_handler(widget, value_handler, EVENT_VALUE_CHANGED, NULL);
    
    printf("\n[TEST 1] Send CLICK:\n");
    event_send(widget, EVENT_CLICK, NULL);
    
    printf("\n[TEST 2] Send HOVER:\n");
    event_send(widget, EVENT_HOVER, NULL);
    
    int value = 42;
    printf("\n[TEST 3] Send VALUE_CHANGED:\n");
    event_send(widget, EVENT_VALUE_CHANGED, &value);
    
    printf("\n[SONUÇ] Her handler kendi event tipini yakaladı ✓\n");
    
    free(widget);
}

void demo_event_all() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 3: EVENT_ALL Filter\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* widget = widget_create("AllWidget");
    
    printf("\n[SETUP] Registering EVENT_ALL handler:\n");
    widget_add_handler(widget, all_handler, EVENT_ALL, NULL);
    
    printf("\n[TEST 1] Send CLICK:\n");
    event_send(widget, EVENT_CLICK, NULL);
    
    printf("\n[TEST 2] Send HOVER:\n");
    event_send(widget, EVENT_HOVER, NULL);
    
    printf("\n[TEST 3] Send FOCUS:\n");
    event_send(widget, EVENT_FOCUS, NULL);
    
    printf("\n[SONUÇ] ALL handler TÜM event'leri yakaladı ✓\n");
    
    free(widget);
}

void demo_mixed_filters() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 4: Mixed Filters (Specific + ALL)\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* widget = widget_create("MixedWidget");
    
    printf("\n[SETUP] Registering mixed handlers:\n");
    widget_add_handler(widget, click_handler, EVENT_CLICK, NULL);
    widget_add_handler(widget, all_handler, EVENT_ALL, NULL);
    widget_add_handler(widget, hover_handler, EVENT_HOVER, NULL);
    
    printf("\n[TEST] Send CLICK:\n");
    event_send(widget, EVENT_CLICK, NULL);
    
    printf("\n[SONUÇ]\n");
    printf("  ✓ click_handler called (filter=CLICK)\n");
    printf("  ✓ all_handler called (filter=ALL)\n");
    printf("  ✗ hover_handler NOT called (filter=HOVER)\n");
    
    free(widget);
}

int main() {
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║     EVENT FILTERING (F İLTRELEME) DEMO        ║\n");
    printf("║                                               ║\n");
    printf("║  Konsept: Handler'lar event tipini filtreler ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    demo_single_filter();
    demo_multiple_filters();
    demo_event_all();
    demo_mixed_filters();
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                       ║\n");
    printf("║                                               ║\n");
    printf("║  ✓ Specific filter = Sadece o event          ║\n");
    printf("║  ✓ EVENT_ALL = Tüm event'ler                 ║\n");
    printf("║  ✓ Memory efficient = Gereksiz çağrı yok     ║\n");
    printf("║  ✓ Clean code = Ayrı handler, ayrı concern   ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    return 0;
}
