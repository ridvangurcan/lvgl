/**
 * @file 01_event_bubbling_demo.c
 * @brief Event Bubbling (Olay Kabarcıklanması) Demo
 *
 * Bu demo, olayların child'dan parent'a doğru "yükselmesini" (bubbling) gösterir.
 *
 * KONSEPT:
 * - Bir child widget'ta oluşan olay, parent widget'lara da ulaşabilir
 * - Parent, tüm child'ların olaylarını tek bir handler'da yakalayabilir
 * - Bubbling kontrol edilebilir (stop_bubbling)
 *
 * COMPILE: gcc 01_event_bubbling_demo.c -o bubbling_demo
 * RUN: ./bubbling_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * WIDGET HIYERARŞISI
 * ======================================================================== */

typedef struct widget widget_t;

typedef enum {
    EVENT_CLICK,
    EVENT_HOVER,
    EVENT_KEY_PRESS,
    EVENT_VALUE_CHANGED
} event_type_t;

typedef struct {
    event_type_t type;
    widget_t* target;           // Olayın kaynağı (asıl widget)
    widget_t* current_target;   // Şu anda işleyen widget
    void* param;
    bool stop_bubbling;         // Bubbling'i durdur
} event_t;

typedef void (*event_handler_t)(event_t* e);

struct widget {
    char name[32];
    widget_t* parent;
    widget_t* children[10];
    int child_count;
    event_handler_t handler;
    bool enable_bubbling;       // Bubbling aktif mi?
    void* user_data;
};

/* ========================================================================
 * WIDGET FONKSİYONLARI
 * ======================================================================== */

widget_t* widget_create(const char* name, widget_t* parent) {
    widget_t* w = (widget_t*)calloc(1, sizeof(widget_t));
    strncpy(w->name, name, sizeof(w->name) - 1);
    w->parent = parent;
    w->enable_bubbling = true;  // Varsayılan: bubbling aktif

    if (parent) {
        parent->children[parent->child_count++] = w;
    }

    return w;
}

void widget_set_event_handler(widget_t* w, event_handler_t handler) {
    w->handler = handler;
}

void widget_set_bubbling(widget_t* w, bool enable) {
    w->enable_bubbling = enable;
}

void widget_destroy(widget_t* w) {
    for (int i = 0; i < w->child_count; i++) {
        widget_destroy(w->children[i]);
    }
    free(w);
}

/* ========================================================================
 * EVENT SİSTEMİ - BUBBLING İLE
 * ======================================================================== */

/**
 * Event gönder - Bubbling desteğiyle
 */
void event_send(widget_t* target, event_type_t type, void* param) {
    const char* event_names[] = {"CLICK", "HOVER", "KEY_PRESS", "VALUE_CHANGED"};

    printf("\n[EVENT] Sending %s to '%s'\n", event_names[type], target->name);

    event_t e = {
        .type = type,
        .target = target,
        .current_target = target,
        .param = param,
        .stop_bubbling = false
    };

    widget_t* current = target;
    int level = 0;

    // Bubbling: Widget'tan parent'a doğru yüksel
    while (current != NULL) {
        e.current_target = current;

        printf("  [LEVEL %d] Processing at '%s'", level, current->name);

        if (current->handler) {
            printf(" → Handler EXISTS\n");
            current->handler(&e);

            if (e.stop_bubbling) {
                printf("  [STOP] Bubbling stopped at '%s'\n", current->name);
                break;
            }
        } else {
            printf(" → No handler\n");
        }

        // Bubbling kontrol
        if (!current->enable_bubbling) {
            printf("  [STOP] Bubbling disabled at '%s'\n", current->name);
            break;
        }

        current = current->parent;  // Parent'a yüksel
        level++;
    }

    printf("[EVENT] Event processing complete\n");
}

void event_stop_bubbling(event_t* e) {
    e->stop_bubbling = true;
}

/* ========================================================================
 * DEMO HANDLER'LAR
 * ======================================================================== */

void button_handler(event_t* e) {
    printf("    [BTN-HANDLER] Button '%s' clicked!\n", e->target->name);
    // Bubbling devam eder (stop_bubbling çağrılmadı)
}

void panel_handler(event_t* e) {
    printf("    [PANEL-HANDLER] Panel '%s' received event from '%s'\n",
           e->current_target->name, e->target->name);

    if (e->target != e->current_target) {
        printf("    [PANEL-HANDLER] This is a BUBBLED event! (originated from child)\n");
    }
}

void window_handler(event_t* e) {
    printf("    [WINDOW-HANDLER] Window '%s' received event from '%s'\n",
           e->current_target->name, e->target->name);

    if (e->target != e->current_target) {
        printf("    [WINDOW-HANDLER] This is a BUBBLED event! (originated from descendant)\n");
    }
}

void button_stop_handler(event_t* e) {
    printf("    [BTN-STOP-HANDLER] Button '%s' clicked! STOPPING BUBBLING\n",
           e->target->name);
    event_stop_bubbling(e);  // Bubbling'i durdur
}

/* ========================================================================
 * DEMO SENARYOLARI
 * ======================================================================== */

void demo_basic_bubbling() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 1: Temel Bubbling\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[HIYERARŞI]\n");
    printf("  Window\n");
    printf("    └─ Panel\n");
    printf("         └─ Button\n");

    // Hiyerarşi oluştur
    widget_t* window = widget_create("MainWindow", NULL);
    widget_t* panel = widget_create("ContentPanel", window);
    widget_t* button = widget_create("ClickButton", panel);

    // Handler'lar kaydet
    widget_set_event_handler(button, button_handler);
    widget_set_event_handler(panel, panel_handler);
    widget_set_event_handler(window, window_handler);

    printf("\n[TEST] Button click event:\n");
    event_send(button, EVENT_CLICK, NULL);

    printf("\n[SONUÇ]\n");
    printf("  ✓ Event button'da başladı\n");
    printf("  ✓ Panel'e bubble oldu (1 seviye yukarı)\n");
    printf("  ✓ Window'a bubble oldu (2 seviye yukarı)\n");

    widget_destroy(window);
}

void demo_stop_bubbling() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 2: Stop Bubbling\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[HIYERARŞI]\n");
    printf("  Window\n");
    printf("    └─ Panel\n");
    printf("         └─ Button (will STOP bubbling)\n");

    widget_t* window = widget_create("MainWindow", NULL);
    widget_t* panel = widget_create("ContentPanel", window);
    widget_t* button = widget_create("StopButton", panel);

    // Handler'lar kaydet
    widget_set_event_handler(button, button_stop_handler);  // Bu handler durduracak
    widget_set_event_handler(panel, panel_handler);
    widget_set_event_handler(window, window_handler);

    printf("\n[TEST] Button click event (with stop):\n");
    event_send(button, EVENT_CLICK, NULL);

    printf("\n[SONUÇ]\n");
    printf("  ✓ Event button'da başladı\n");
    printf("  ✓ Button handler stop_bubbling çağırdı\n");
    printf("  ✗ Panel handler çağrılmadı (bubbling durduruldu)\n");
    printf("  ✗ Window handler çağrılmadı (bubbling durduruldu)\n");

    widget_destroy(window);
}

void demo_disable_bubbling() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 3: Bubbling Disabled (Widget-level)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[HIYERARŞI]\n");
    printf("  Window\n");
    printf("    └─ Panel (bubbling DISABLED)\n");
    printf("         └─ Button\n");

    widget_t* window = widget_create("MainWindow", NULL);
    widget_t* panel = widget_create("ContentPanel", window);
    widget_t* button = widget_create("ClickButton", panel);

    // Panel'de bubbling'i kapat
    widget_set_bubbling(panel, false);

    widget_set_event_handler(button, button_handler);
    widget_set_event_handler(panel, panel_handler);
    widget_set_event_handler(window, window_handler);

    printf("\n[TEST] Button click event (panel bubbling disabled):\n");
    event_send(button, EVENT_CLICK, NULL);

    printf("\n[SONUÇ]\n");
    printf("  ✓ Event button'da başladı\n");
    printf("  ✓ Panel handler çağrıldı\n");
    printf("  ✗ Window handler çağrılmadı (panel'de bubbling disabled)\n");

    widget_destroy(window);
}

void demo_delegated_handling() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 4: Delegated Event Handling (Practical Use Case)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[SENARYO]\n");
    printf("  Bir container 10 button içeriyor\n");
    printf("  Container'a TEK bir handler kayıtlı\n");
    printf("  Tüm button click'leri container'da yakalanıyor\n");

    widget_t* container = widget_create("ButtonContainer", NULL);

    // 10 button oluştur - HİÇBİRİNE handler kaydetme!
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "Button%d", i);
        widget_create(name, container);
        // Not: Handler yok! Container yakalayacak
    }

    // Container'a TEK handler
    widget_set_event_handler(container, panel_handler);

    printf("\n[TEST 1] Click Button3:\n");
    event_send(container->children[3], EVENT_CLICK, NULL);

    printf("\n[TEST 2] Click Button7:\n");
    event_send(container->children[7], EVENT_CLICK, NULL);

    printf("\n[SONUÇ]\n");
    printf("  ✓ 10 button, 0 handler registration on buttons\n");
    printf("  ✓ 1 handler on container catches all\n");
    printf("  ✓ Memory efficient!\n");
    printf("  ✓ e->target tells which button was clicked\n");

    widget_destroy(container);
}

void demo_multi_level_bubbling() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 5: Multi-Level Bubbling\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[HIYERARŞI]\n");
    printf("  Screen\n");
    printf("    └─ Window\n");
    printf("         └─ Panel\n");
    printf("              └─ Container\n");
    printf("                   └─ Button\n");

    widget_t* screen = widget_create("Screen", NULL);
    widget_t* window = widget_create("Window", screen);
    widget_t* panel = widget_create("Panel", window);
    widget_t* container = widget_create("Container", panel);
    widget_t* button = widget_create("DeepButton", container);

    // Sadece screen ve button'a handler
    widget_set_event_handler(button, button_handler);
    widget_set_event_handler(screen, window_handler);

    printf("\n[TEST] Deep button click:\n");
    event_send(button, EVENT_CLICK, NULL);

    printf("\n[SONUÇ]\n");
    printf("  ✓ Event 5 seviye yukarı bubble oldu\n");
    printf("  ✓ Ara widget'lar handler'sız olsa bile bubbling devam etti\n");
    printf("  ✓ Screen (en üst) event'i yakaladı\n");

    widget_destroy(screen);
}

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║           EVENT BUBBLING (KABARCIKLANMA) DEMO            ║\n");
    printf("║                                                           ║\n");
    printf("║  Konsept: Olaylar child'dan parent'a doğru yükselir      ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    demo_basic_bubbling();
    demo_stop_bubbling();
    demo_disable_bubbling();
    demo_delegated_handling();
    demo_multi_level_bubbling();

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                                   ║\n");
    printf("║                                                           ║\n");
    printf("║  ✓ Event bubbling = Child'dan parent'a propagation       ║\n");
    printf("║  ✓ Delegated handling = Parent tüm child'ları yakalar    ║\n");
    printf("║  ✓ Kontrol edilebilir = stop_bubbling() ile durdur       ║\n");
    printf("║  ✓ Memory efficient = Tek handler, çok widget            ║\n");
    printf("║                                                           ║\n");
    printf("║  📖 AVANTAJLAR:                                            ║\n");
    printf("║     - Az handler registration                            ║\n");
    printf("║     - Kolay child yönetimi                               ║\n");
    printf("║     - Tutarlı event handling                             ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
