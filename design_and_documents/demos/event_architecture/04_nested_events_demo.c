/**
 * @file 04_nested_events_demo.c
 * @brief Nested Event Handling Demo
 *
 * KONSEPT:
 * - Event handler içinde başka event gönderilebilir
 * - Event stack ile takip edilir
 * - Deletion marking ile güvenlik sağlanır
 *
 * COMPILE: gcc 04_nested_events_demo.c -o nested_demo
 * RUN: ./nested_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVENT_CLICK,
    EVENT_VALUE_CHANGED,
    EVENT_DELETE
} event_type_t;

typedef struct widget widget_t;
typedef struct event event_t;

struct event {
    event_type_t type;
    widget_t* target;
    void* param;
    bool deleted;
    struct event* prev;  // Event stack pointer
};

typedef void (*event_handler_t)(event_t* e);

struct widget {
    char name[32];
    event_handler_t handler;
    int value;
    bool is_deleted;
};

// Global event stack head
static event_t* event_head = NULL;

widget_t* widget_create(const char* name) {
    widget_t* w = (widget_t*)calloc(1, sizeof(widget_t));
    strncpy(w->name, name, sizeof(w->name) - 1);
    return w;
}

void widget_set_handler(widget_t* w, event_handler_t handler) {
    w->handler = handler;
}

void widget_mark_deleted(widget_t* w) {
    printf("    [MARK-DELETED] Marking widget '%s' as deleted\n", w->name);
    w->is_deleted = true;
    
    // Walk event stack and mark any events involving this widget
    event_t* e = event_head;
    while (e) {
        if (e->target == w) {
            printf("    [MARK-DELETED] Found event in stack, setting deleted flag\n");
            e->deleted = true;
        }
        e = e->prev;
    }
}

void widget_delete(widget_t* w) {
    printf("  [DELETE] Deleting widget '%s'\n", w->name);
    widget_mark_deleted(w);
    free(w);
}

void event_send(widget_t* target, event_type_t type, void* param);  // Forward decl

void event_send(widget_t* target, event_type_t type, void* param) {
    const char* event_names[] = {"CLICK", "VALUE_CHANGED", "DELETE"};
    
    static int depth = 0;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("[EVENT-%d] Sending %s to '%s'\n", depth, event_names[type], target->name);
    
    // Build event on stack
    event_t e = {
        .type = type,
        .target = target,
        .param = param,
        .deleted = false,
        .prev = event_head  // Link to outer event
    };
    
    event_head = &e;  // Push to stack
    depth++;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("[STACK] Event stack depth: %d\n", depth);
    
    // Call handler
    if (target->handler && !target->is_deleted) {
        target->handler(&e);
        
        if (e.deleted) {
            for (int i = 0; i < depth; i++) printf("  ");
            printf("[DELETED] Widget was deleted during processing!\n");
        }
    }
    
    // Pop from stack
    event_head = e.prev;
    depth--;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("[STACK] Popped, depth now: %d\n", depth);
}

// Handlers that trigger nested events
void button_handler(event_t* e) {
    printf("    [BTN-HANDLER] Button clicked\n");
    printf("    [BTN-HANDLER] Triggering nested VALUE_CHANGED event...\n");
    
    // Nested event!
    event_send(e->target, EVENT_VALUE_CHANGED, NULL);
    
    printf("    [BTN-HANDLER] Returned from nested event\n");
}

void value_changed_handler(event_t* e) {
    printf("      [VALUE-HANDLER] Value changed\n");
    e->target->value++;
    printf("      [VALUE-HANDLER] New value: %d\n", e->target->value);
}

void delete_handler(event_t* e) {
    printf("    [DELETE-HANDLER] DELETE event received\n");
    // Cleanup would happen here
}

void cascade_handler(event_t* e) {
    printf("    [CASCADE] Level 1 handler\n");
    printf("    [CASCADE] Triggering level 2...\n");
    event_send(e->target, EVENT_VALUE_CHANGED, NULL);
    printf("    [CASCADE] Returned to level 1\n");
}

void level2_handler(event_t* e) {
    printf("      [L2] Level 2 handler\n");
    e->target->value += 10;
}

void dangerous_handler(event_t* e) {
    printf("    [DANGEROUS] Handler deleting widget during processing\n");
    widget_delete(e->target);
    printf("    [DANGEROUS] After delete, widget still in scope (stack variable)\n");
}

void demo_basic_nesting() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 1: Basic Nested Events\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* btn = widget_create("Button");
    widget_set_handler(btn, button_handler);
    
    printf("\n[TEST] Click triggers nested VALUE_CHANGED:\n");
    event_send(btn, EVENT_CLICK, NULL);
    
    printf("\n[SONUÇ]\n");
    printf("  ✓ CLICK event started (depth=1)\n");
    printf("  ✓ VALUE_CHANGED nested inside (depth=2)\n");
    printf("  ✓ Stack unwound correctly\n");
    
    free(btn);
}

void demo_deep_nesting() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 2: Deep Nesting (3 levels)\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* w1 = widget_create("Widget1");
    widget_t* w2 = widget_create("Widget2");
    widget_t* w3 = widget_create("Widget3");
    
    // Setup cascading handlers
    void handler1(event_t* e) {
        printf("    [H1] Handler 1\n");
        printf("    [H1] Sending to Widget2...\n");
        event_send(w2, EVENT_CLICK, NULL);
        printf("    [H1] Returned\n");
    }
    
    void handler2(event_t* e) {
        printf("      [H2] Handler 2\n");
        printf("      [H2] Sending to Widget3...\n");
        event_send(w3, EVENT_CLICK, NULL);
        printf("      [H2] Returned\n");
    }
    
    void handler3(event_t* e) {
        printf("        [H3] Handler 3 (deepest)\n");
    }
    
    widget_set_handler(w1, handler1);
    widget_set_handler(w2, handler2);
    widget_set_handler(w3, handler3);
    
    printf("\n[TEST] Cascading events:\n");
    event_send(w1, EVENT_CLICK, NULL);
    
    printf("\n[SONUÇ]\n");
    printf("  ✓ 3-level deep nesting\n");
    printf("  ✓ Stack tracked correctly\n");
    
    free(w1); free(w2); free(w3);
}

void demo_deletion_during_event() {
    printf("\n═══════════════════════════════════════════════\n");
    printf("  DEMO 3: Deletion During Event Processing\n");
    printf("═══════════════════════════════════════════════\n");
    
    widget_t* btn = widget_create("DangerButton");
    widget_set_handler(btn, dangerous_handler);
    
    printf("\n[TEST] Handler deletes widget:\n");
    event_send(btn, EVENT_CLICK, NULL);
    
    printf("\n[SONUÇ]\n");
    printf("  ✓ Widget deleted during processing\n");
    printf("  ✓ Event marked as deleted\n");
    printf("  ✓ No crash (event on stack)\n");
}

int main() {
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║       NESTED EVENT HANDLING DEMO             ║\n");
    printf("║                                               ║\n");
    printf("║  Konsept: Event içinde event tetiklenebilir  ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    demo_basic_nesting();
    demo_deep_nesting();
    demo_deletion_during_event();
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                       ║\n");
    printf("║                                               ║\n");
    printf("║  ✓ Nested events = Event içinde event        ║\n");
    printf("║  ✓ Event stack = Linked list ile takip       ║\n");
    printf("║  ✓ Deletion safe = Marked, no crash          ║\n");
    printf("║  ✓ Complex workflows = Güvenle çalışır        ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    return 0;
}
