/**
 * @file 04_deferred_deletion_demo.c
 * @brief Deferred Deletion (Ertelenmiş Silme) Stratejisi Demo
 *
 * Bu demo, nesne silme işlemini erteleyen güvenlik stratejisini gösterir.
 *
 * KONSEPT:
 * - Callback içinde immediate deletion → use-after-free risk
 * - Deferred deletion → Callback bittikten sonra sil
 * - Async queue mekanizması
 *
 * COMPILE: gcc 04_deferred_deletion_demo.c -o deferred_demo
 * RUN: ./deferred_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * BASIT ASYNC CALL SİSTEMİ
 * ======================================================================== */

#define MAX_ASYNC_QUEUE 100

typedef void (*async_callback_t)(void*);

typedef struct {
    async_callback_t callback;
    void* param;
} async_call_t;

static async_call_t async_queue[MAX_ASYNC_QUEUE];
static int async_queue_len = 0;

/**
 * Async call queue'ya ekle
 */
void async_call(async_callback_t callback, void* param) {
    if (async_queue_len >= MAX_ASYNC_QUEUE) {
        printf("  [ASYNC] Queue full! ⚠️\n");
        return;
    }

    async_queue[async_queue_len].callback = callback;
    async_queue[async_queue_len].param = param;
    async_queue_len++;

    printf("  [ASYNC] Queued: %p (queue size: %d)\n", param, async_queue_len);
}

/**
 * Async queue'yu işle (ana döngüde çağrılır)
 */
void async_call_process(void) {
    printf("\n[ASYNC] Processing queue (%d items)...\n", async_queue_len);

    while (async_queue_len > 0) {
        async_queue_len--;
        async_call_t* call = &async_queue[async_queue_len];

        printf("  [ASYNC] Executing: %p\n", call->param);
        call->callback(call->param);
    }

    printf("[ASYNC] Queue empty ✓\n");
}

/* ========================================================================
 * BASIT EVENT SİSTEMİ
 * ======================================================================== */

typedef enum {
    EVENT_CLICK,
    EVENT_HOVER,
    EVENT_DELETE
} event_type_t;

typedef struct widget widget_t;

typedef void (*event_callback_t)(widget_t* widget, event_type_t event);

struct widget {
    int id;
    char name[32];
    bool deleted;  // Silme işareti
    event_callback_t callback;
};

/* ========================================================================
 * WIDGET MANAGEMENT
 * ======================================================================== */

/**
 * Widget oluştur
 */
widget_t* widget_create(int id, const char* name, event_callback_t callback) {
    widget_t* w = (widget_t*)calloc(1, sizeof(widget_t));
    w->id = id;
    strncpy(w->name, name, sizeof(w->name) - 1);
    w->deleted = false;
    w->callback = callback;

    printf("[CREATE] Widget #%d '%s' created\n", id, name);
    return w;
}

/**
 * IMMEDIATE DELETION (Tehlikeli!)
 */
void widget_delete_immediate(widget_t* w) {
    if (w->deleted) {
        printf("  [DELETE] Already deleted! ⚠️\n");
        return;
    }

    printf("  [DELETE] Deleting widget #%d '%s' immediately\n", w->id, w->name);

    // DELETE event gönder
    if (w->callback) {
        w->callback(w, EVENT_DELETE);
    }

    free(w);
    printf("  [DELETE] Widget freed ✓\n");
}

/**
 * ASYNC DELETION (Güvenli)
 */
static void widget_delete_async_cb(void* param) {
    widget_t* w = (widget_t*)param;

    printf("  [DELETE-ASYNC] Now deleting widget #%d '%s'\n", w->id, w->name);

    // DELETE event gönder
    if (w->callback) {
        w->callback(w, EVENT_DELETE);
    }

    free(w);
    printf("  [DELETE-ASYNC] Widget freed ✓\n");
}

void widget_delete_async(widget_t* w) {
    if (w->deleted) {
        printf("  [DELETE-ASYNC] Already marked for deletion! ⚠️\n");
        return;
    }

    printf("  [DELETE-ASYNC] Marking widget #%d '%s' for deletion\n", w->id, w->name);
    w->deleted = true;  // İşaretle

    // Async queue'ya ekle
    async_call(widget_delete_async_cb, w);
}

/**
 * Event gönder
 */
void widget_send_event(widget_t* w, event_type_t event) {
    const char* event_names[] = {"CLICK", "HOVER", "DELETE"};

    if (w->deleted) {
        printf("  [EVENT] Widget deleted, skipping event ⚠️\n");
        return;
    }

    printf("  [EVENT] Sending %s to widget #%d '%s'\n",
           event_names[event], w->id, w->name);

    if (w->callback) {
        w->callback(w, event);
    }

    printf("  [EVENT] Event processing complete ✓\n");
}

/* ========================================================================
 * DEMO 1: IMMEDIATE DELETION PROBLEMI
 * ======================================================================== */

static void dangerous_callback(widget_t* w, event_type_t event) {
    printf("    [CALLBACK] Processing event for '%s'\n", w->name);

    if (event == EVENT_CLICK) {
        printf("    [CALLBACK] User clicked! Deleting self...\n");

        // TEHLİKE: Kendini hemen sil!
        widget_delete_immediate(w);

        // ⚠️ w artık FREED! Aşağıdaki kod use-after-free!
        printf("    [CALLBACK] After delete - accessing freed memory: id=%d ⚠️\n",
               w->id);  // UNDEFINED BEHAVIOR!
    }
}

void demo_immediate_deletion_problem() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 1: Immediate Deletion Problemi\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[SENARYO] Button tıklanınca kendini siler\n");

    widget_t* button = widget_create(1, "DangerButton", dangerous_callback);

    printf("\n[ACTION] Simulating click...\n");
    widget_send_event(button, EVENT_CLICK);

    // ⚠️ button artık freed!
    // Event sistem return edince button'a erişmeye çalışırsa CRASH!

    printf("\n[SONUÇ]\n");
    printf("  ⚠️  Callback içinde button freed\n");
    printf("  ⚠️  Event system return edince use-after-free risk\n");
    printf("  ⚠️  Callback içinde freed memory'ye erişildi\n");
}

/* ========================================================================
 * DEMO 2: DEFERRED DELETION ÇÖZÜMÜ
 * ======================================================================== */

static void safe_callback(widget_t* w, event_type_t event) {
    printf("    [CALLBACK] Processing event for '%s'\n", w->name);

    if (event == EVENT_CLICK) {
        printf("    [CALLBACK] User clicked! Scheduling deletion...\n");

        // GÜVENLİ: Async silme
        widget_delete_async(w);

        // ✓ w hala geçerli (henüz freed değil)
        printf("    [CALLBACK] After async delete - widget still valid: id=%d ✓\n",
               w->id);

        printf("    [CALLBACK] Accessing name: '%s' ✓\n", w->name);
    }
    else if (event == EVENT_DELETE) {
        printf("    [CALLBACK] DELETE event - cleanup resources\n");
    }
}

void demo_deferred_deletion_solution() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 2: Deferred Deletion Çözümü\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[SENARYO] Button tıklanınca async silme\n");

    widget_t* button = widget_create(2, "SafeButton", safe_callback);

    printf("\n[ACTION] Simulating click...\n");
    widget_send_event(button, EVENT_CLICK);

    printf("\n[MAIN LOOP] Event processing done, button still valid ✓\n");
    printf("  button->id = %d\n", button->id);
    printf("  button->name = '%s'\n", button->name);
    printf("  button->deleted = %d (marked for deletion)\n", button->deleted);

    printf("\n[MAIN LOOP] Processing async queue...\n");
    async_call_process();  // Şimdi gerçek silme yapılır

    printf("\n[SONUÇ]\n");
    printf("  ✓ Callback içinde button hala geçerli\n");
    printf("  ✓ Event system güvenle return etti\n");
    printf("  ✓ Ana döngüde silme yapıldı\n");
}

/* ========================================================================
 * DEMO 3: NESTED DELETION
 * ======================================================================== */

typedef struct {
    widget_t* parent;
    widget_t* child1;
    widget_t* child2;
} widget_hierarchy_t;

static widget_hierarchy_t* g_hierarchy = NULL;

static void parent_callback(widget_t* w, event_type_t event) {
    printf("    [PARENT-CB] Event for '%s'\n", w->name);

    if (event == EVENT_DELETE) {
        printf("    [PARENT-CB] Parent being deleted, deleting children...\n");

        if (g_hierarchy) {
            if (g_hierarchy->child1) {
                printf("    [PARENT-CB] Deleting child1...\n");
                widget_delete_async(g_hierarchy->child1);
            }
            if (g_hierarchy->child2) {
                printf("    [PARENT-CB] Deleting child2...\n");
                widget_delete_async(g_hierarchy->child2);
            }
        }
    }
}

static void child_callback(widget_t* w, event_type_t event) {
    printf("    [CHILD-CB] Event for '%s'\n", w->name);

    if (event == EVENT_DELETE) {
        printf("    [CHILD-CB] Cleaning up child\n");
        // Parent hala geçerli (async deletion sayesinde)
        if (g_hierarchy && g_hierarchy->parent) {
            printf("    [CHILD-CB] Parent still accessible: '%s' ✓\n",
                   g_hierarchy->parent->name);
        }
    }
}

void demo_nested_deletion() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 3: Nested Deletion (Parent-Child)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[SETUP] Creating parent with 2 children\n");

    g_hierarchy = (widget_hierarchy_t*)calloc(1, sizeof(widget_hierarchy_t));

    g_hierarchy->parent = widget_create(10, "ParentWidget", parent_callback);
    g_hierarchy->child1 = widget_create(11, "Child1", child_callback);
    g_hierarchy->child2 = widget_create(12, "Child2", child_callback);

    printf("\n[ACTION] Deleting parent (will delete children too)...\n");
    widget_delete_async(g_hierarchy->parent);

    printf("\n[MAIN LOOP] Processing async deletions...\n");
    async_call_process();

    printf("\n[SONUÇ]\n");
    printf("  ✓ Parent deletion callback'i child'ları sildi\n");
    printf("  ✓ Child deletion callback'lerinde parent hala erişilebilirdi\n");
    printf("  ✓ Nested deletion güvenle tamamlandı\n");

    free(g_hierarchy);
    g_hierarchy = NULL;
}

/* ========================================================================
 * DEMO 4: DELETION DURING ITERATION
 * ======================================================================== */

#define MAX_WIDGETS 10

typedef struct {
    widget_t* widgets[MAX_WIDGETS];
    int count;
} widget_list_t;

static widget_list_t* g_list = NULL;

static void list_item_callback(widget_t* w, event_type_t event) {
    printf("    [ITEM-CB] Event for '%s'\n", w->name);

    if (event == EVENT_CLICK) {
        printf("    [ITEM-CB] Item clicked, scheduling deletion\n");
        widget_delete_async(w);
    }
}

void demo_deletion_during_iteration() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 4: Deletion During Iteration\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[SETUP] Creating list of 5 widgets\n");

    g_list = (widget_list_t*)calloc(1, sizeof(widget_list_t));

    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "ListItem%d", i);
        g_list->widgets[i] = widget_create(20 + i, name, list_item_callback);
        g_list->count++;
    }

    printf("\n[ACTION] Iterating and clicking items (item 2 will self-delete)...\n");

    for (int i = 0; i < g_list->count; i++) {
        widget_t* w = g_list->widgets[i];

        if (w->deleted) {
            printf("\n[ITER #%d] Widget '%s' already marked for deletion, skip ⚠️\n",
                   i, w->name);
            continue;
        }

        printf("\n[ITER #%d] Processing widget '%s'\n", i, w->name);

        if (i == 2) {
            printf("[ITER #%d] Clicking item 2 (will self-delete)...\n", i);
            widget_send_event(w, EVENT_CLICK);
        }

        // ✓ w hala geçerli (async deletion)
        printf("[ITER #%d] After event, widget still valid: id=%d ✓\n", i, w->id);
    }

    printf("\n[ITER] Iteration complete, all widgets still in list ✓\n");

    printf("\n[MAIN LOOP] Processing async deletions...\n");
    async_call_process();

    printf("\n[CLEANUP] Deleting remaining widgets...\n");
    for (int i = 0; i < g_list->count; i++) {
        if (!g_list->widgets[i]->deleted) {
            widget_delete_async(g_list->widgets[i]);
        }
    }
    async_call_process();

    printf("\n[SONUÇ]\n");
    printf("  ✓ Iterator sırasında widget silinse bile iteration devam etti\n");
    printf("  ✓ Deleted flag ile silinen widget'lar skip edildi\n");
    printf("  ✓ Ana döngüde gerçek silme yapıldı\n");

    free(g_list);
    g_list = NULL;
}

/* ========================================================================
 * DEMO 5: DELAYED DELETION (Simulated)
 * ======================================================================== */

typedef struct {
    widget_t* widget;
    int delay_frames;
} delayed_deletion_t;

#define MAX_DELAYED_QUEUE 100

static delayed_deletion_t delayed_queue[MAX_DELAYED_QUEUE];
static int delayed_queue_len = 0;

void widget_delete_delayed(widget_t* w, int delay_frames) {
    if (delayed_queue_len >= MAX_DELAYED_QUEUE) {
        printf("  [DELAYED] Queue full! ⚠️\n");
        return;
    }

    printf("  [DELAYED] Scheduling deletion in %d frames\n", delay_frames);

    delayed_queue[delayed_queue_len].widget = w;
    delayed_queue[delayed_queue_len].delay_frames = delay_frames;
    delayed_queue_len++;

    w->deleted = true;  // İşaretle
}

void delayed_deletion_process() {
    for (int i = 0; i < delayed_queue_len; i++) {
        delayed_queue[i].delay_frames--;

        if (delayed_queue[i].delay_frames <= 0) {
            printf("  [DELAYED] Delay expired, deleting widget #%d\n",
                   delayed_queue[i].widget->id);

            widget_delete_async_cb(delayed_queue[i].widget);

            // Remove from queue
            for (int j = i; j < delayed_queue_len - 1; j++) {
                delayed_queue[j] = delayed_queue[j + 1];
            }
            delayed_queue_len--;
            i--;
        }
    }
}

void demo_delayed_deletion() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 5: Delayed Deletion (Animation)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[SENARYO] Popup fade-out animation (3 frames)\n");

    widget_t* popup = widget_create(30, "PopupDialog", NULL);

    printf("\n[ACTION] Closing popup with fade-out...\n");
    printf("  (Animation: 3 frames fade-out, then delete)\n");

    widget_delete_delayed(popup, 3);  // 3 frame sonra sil

    printf("\n[MAIN LOOP] Simulating frames...\n");

    for (int frame = 1; frame <= 5; frame++) {
        printf("\n  [FRAME %d] Rendering...\n", frame);

        if (popup->deleted && delayed_queue_len > 0) {
            int delay = delayed_queue[0].delay_frames;
            printf("    Popup fading out (opacity=%d%%)...\n",
                   (delay * 100) / 3);
        }

        delayed_deletion_process();
    }

    printf("\n[SONUÇ]\n");
    printf("  ✓ Popup 3 frame boyunca fade-out yaptı\n");
    printf("  ✓ Frame 4'te silme yapıldı\n");
    printf("  ✓ Smooth UX sağlandı\n");
}

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║       DEFERRED DELETION (ERTELENMİŞ SİLME) DEMO          ║\n");
    printf("║                                                           ║\n");
    printf("║  Konsept: Callback içinde silme yapma, ertele            ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    demo_immediate_deletion_problem();
    demo_deferred_deletion_solution();
    demo_nested_deletion();
    demo_deletion_during_iteration();
    demo_delayed_deletion();

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                                   ║\n");
    printf("║                                                           ║\n");
    printf("║  IMMEDIATE DELETION:                                      ║\n");
    printf("║  ✓ Main loop içinde kullan                               ║\n");
    printf("║  ✗ Callback/event handler içinde kullanma!               ║\n");
    printf("║                                                           ║\n");
    printf("║  ASYNC DELETION:                                          ║\n");
    printf("║  ✓ Callback içinde güvenli                               ║\n");
    printf("║  ✓ Event handler içinde güvenli                          ║\n");
    printf("║  ✓ Iterator sırasında güvenli                            ║\n");
    printf("║  ✓ Use-after-free önlenir                                ║\n");
    printf("║                                                           ║\n");
    printf("║  DELAYED DELETION:                                        ║\n");
    printf("║  ✓ Animation sonrası silme                               ║\n");
    printf("║  ✓ Smooth UX için                                        ║\n");
    printf("║  ✓ Fade-out, slide-out vb.                               ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
