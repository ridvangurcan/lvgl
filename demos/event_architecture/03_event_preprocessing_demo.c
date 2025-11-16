/**
 * @file 03_event_preprocessing_demo.c
 * @brief Event Preprocessing (Olay Ön-İşleme) Demo
 *
 * KONSEPT:
 * - Preprocess handler'lar varsayılan handler'dan ÖNCE çalışır
 * - Interception, validation, audit için kullanılır
 * - stop_processing ile varsayılan davranış önlenebilir
 *
 * COMPILE: gcc 03_event_preprocessing_demo.c -o preprocessing_demo
 * RUN: ./preprocessing_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVENT_CLICK,
    EVENT_VALUE_CHANGED,
    EVENT_KEY_PRESS
} event_type_t;

typedef struct widget widget_t;

typedef struct {
    event_type_t type;
    widget_t* target;
    void* param;
    bool stop_processing;
} event_t;

typedef void (*event_handler_t)(event_t* e);
typedef void (*class_handler_t)(widget_t* w, event_t* e);

#define MAX_HANDLERS 10

typedef struct {
    event_handler_t callback;
    event_type_t filter;
    bool is_preprocess;  // Preprocess flag
} event_descriptor_t;

struct widget {
    char name[32];
    event_descriptor_t handlers[MAX_HANDLERS];
    int handler_count;
    class_handler_t default_handler;  // Class default handler
    int value;  // Widget state
    bool enabled;
};

widget_t* widget_create(const char* name, class_handler_t default_handler) {
    widget_t* w = (widget_t*)calloc(1, sizeof(widget_t));
    strncpy(w->name, name, sizeof(w->name) - 1);
    w->default_handler = default_handler;
    w->enabled = true;
    w->value = 0;
    return w;
}

void widget_add_handler(widget_t* w, event_handler_t handler, 
                        event_type_t filter, bool is_preprocess) {
    if (w->handler_count < MAX_HANDLERS) {
        w->handlers[w->handler_count].callback = handler;
        w->handlers[w->handler_count].filter = filter;
        w->handlers[w->handler_count].is_preprocess = is_preprocess;
        w->handler_count++;
        
        printf("  [REGISTER] %s handler #%d\n",
               is_preprocess ? "PREPROCESS" : "REGULAR", w->handler_count);
    }
}

void event_send(widget_t* target, event_type_t type, void* param) {
    const char* event_names[] = {"CLICK", "VALUE_CHANGED", "KEY_PRESS"};
    
    printf("\n[EVENT] Sending %s to '%s'\n", event_names[type], target->name);
    
    event_t e = {.type = type, .target = target, .param = param, .stop_processing = false};
    
    // PHASE 1: PREPROCESS handlers
    printf("\n  [PHASE 1] Preprocess handlers:\n");
    for (int i = 0; i < target->handler_count; i++) {
        event_descriptor_t* desc = &target->handlers[i];
        if (desc->is_preprocess && desc->filter == type) {
            printf("    → Calling preprocess handler #%d\n", i + 1);
            desc->callback(&e);
            if (e.stop_processing) {
                printf("    ⏹️  stop_processing = true! Halting execution.\n");
                return;
            }
        }
    }
    
    // PHASE 2: CLASS DEFAULT handler
    printf("\n  [PHASE 2] Class default handler:\n");
    if (target->default_handler) {
        printf("    → Calling default handler\n");
        target->default_handler(target, &e);
        if (e.stop_processing) {
            printf("    ⏹️  stop_processing = true! Halting execution.\n");
            return;
        }
    } else {
        printf("    → No default handler\n");
    }
    
    // PHASE 3: REGULAR handlers
    printf("\n  [PHASE 3] Regular handlers:\n");
    for (int i = 0; i < target->handler_count; i++) {
        event_descriptor_t* desc = &target->handlers[i];
        if (!desc->is_preprocess && desc->filter == type) {
            printf("    → Calling regular handler #%d\n", i + 1);
            desc->callback(&e);
            if (e.stop_processing) {
                printf("    ⏹️  stop_processing = true! Halting execution.\n");
                return;
            }
        }
    }
    
    printf("\n[EVENT] All phases complete\n");
}

void event_stop_processing(event_t* e) {
    e->stop_processing = true;
}

// DEFAULT HANDLER (simulates framework behavior)
void button_default_handler(widget_t* w, event_t* e) {
    printf("      [DEFAULT] Button state changed (framework behavior)\n");
    w->value = !w->value;  // Toggle
    printf("      [DEFAULT] New value: %d\n", w->value);
}

// PREPROCESS HANDLERS
void audit_preprocess(event_t* e) {
    printf("      [AUDIT-PRE] Logging event before processing\n");
    printf("      [AUDIT-PRE] Target: %s\n", e->target->name);
}

void validate_preprocess(event_t* e) {
    printf("      [VALIDATE-PRE] Checking if widget is enabled\n");
    if (!e->target->enabled) {
        printf("      [VALIDATE-PRE] Widget disabled! Stopping event.\n");
        event_stop_processing(e);
    } else {
        printf("      [VALIDATE-PRE] Widget enabled, continuing...\n");
    }
}

void intercept_preprocess(event_t* e) {
    printf("      [INTERCEPT-PRE] Custom logic BEFORE default\n");
    printf("      [INTERCEPT-PRE] Current value: %d\n", e->target->value);
    printf("      [INTERCEPT-PRE] Preventing default behavior!\n");
    event_stop_processing(e);  // Prevent default
}

// REGULAR HANDLERS
void regular_handler(event_t* e) {
    printf("      [REGULAR] Responding to event AFTER default\n");
    printf("      [REGULAR] Final value: %d\n", e->target->value);
}

void demo_basic_preprocessing() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 1: Basic Preprocessing Order\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* btn = widget_create("Button", button_default_handler);
    
    printf("\n[SETUP] Registering handlers:\n");
    widget_add_handler(btn, audit_preprocess, EVENT_CLICK, true);   // Preprocess
    widget_add_handler(btn, regular_handler, EVENT_CLICK, false);   // Regular
    
    printf("\n[TEST] Click event:\n");
    event_send(btn, EVENT_CLICK, NULL);
    
    printf("\n[SONUÇ]\n");
    printf("  Execution order:\n");
    printf("    1. audit_preprocess (PRE) ✓\n");
    printf("    2. button_default_handler (DEFAULT) ✓\n");
    printf("    3. regular_handler (REGULAR) ✓\n");
    
    free(btn);
}

void demo_validation_preprocessing() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 2: Validation Preprocessing\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* btn = widget_create("Button", button_default_handler);
    
    printf("\n[SETUP] Registering validation preprocess handler:\n");
    widget_add_handler(btn, validate_preprocess, EVENT_CLICK, true);
    widget_add_handler(btn, regular_handler, EVENT_CLICK, false);
    
    printf("\n[TEST 1] Click when ENABLED:\n");
    btn->enabled = true;
    event_send(btn, EVENT_CLICK, NULL);
    
    printf("\n[TEST 2] Click when DISABLED:\n");
    btn->enabled = false;
    event_send(btn, EVENT_CLICK, NULL);
    
    printf("\n[SONUÇ]\n");
    printf("  Test 1: Preprocess passed, default ran ✓\n");
    printf("  Test 2: Preprocess stopped, default NOT ran ✓\n");
    
    free(btn);
}

void demo_interception() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 3: Interception (Prevent Default)\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* btn = widget_create("Button", button_default_handler);
    btn->value = 0;
    
    printf("\n[SETUP] Registering intercept preprocess:\n");
    widget_add_handler(btn, intercept_preprocess, EVENT_CLICK, true);
    widget_add_handler(btn, regular_handler, EVENT_CLICK, false);
    
    printf("\n[TEST] Click event:\n");
    printf("  Initial value: %d\n", btn->value);
    event_send(btn, EVENT_CLICK, NULL);
    printf("  Final value: %d\n", btn->value);
    
    printf("\n[SONUÇ]\n");
    printf("  Preprocess stopped processing ✓\n");
    printf("  Default handler NOT called ✓\n");
    printf("  Regular handler NOT called ✓\n");
    printf("  Value unchanged (0) ✓\n");
    
    free(btn);
}

void demo_multiple_preprocess() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 4: Multiple Preprocess Handlers\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* btn = widget_create("Button", button_default_handler);
    
    printf("\n[SETUP] Registering 2 preprocess + 1 regular:\n");
    widget_add_handler(btn, audit_preprocess, EVENT_CLICK, true);
    widget_add_handler(btn, validate_preprocess, EVENT_CLICK, true);
    widget_add_handler(btn, regular_handler, EVENT_CLICK, false);
    
    printf("\n[TEST] Click event:\n");
    event_send(btn, EVENT_CLICK, NULL);
    
    printf("\n[SONUÇ]\n");
    printf("  Both preprocess handlers ran BEFORE default ✓\n");
    
    free(btn);
}

int main() {
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║   EVENT PREPROCESSING (ÖN-İŞLEME) DEMO       ║\n");
    printf("║                                               ║\n");
    printf("║  Konsept: Handler'lar default'tan önce çalışır║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    demo_basic_preprocessing();
    demo_validation_preprocessing();
    demo_interception();
    demo_multiple_preprocess();
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                       ║\n");
    printf("║                                               ║\n");
    printf("║  ✓ Preprocess = ÖNCE çalışır                 ║\n");
    printf("║  ✓ Validation = Kontrol et, durdur           ║\n");
    printf("║  ✓ Interception = Default'u önle             ║\n");
    printf("║  ✓ Audit = Log, her şeyden önce              ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    return 0;
}
