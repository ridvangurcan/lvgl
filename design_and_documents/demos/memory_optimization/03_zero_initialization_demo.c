/**
 * @file 03_zero_initialization_demo.c
 * @brief Zero-Initialization (Sıfır Başlatma) Stratejisi Demo
 *
 * Bu demo, belleği tahsis sonrası sıfırlayarak güvenli başlatma
 * yapma stratejisini gösterir.
 *
 * KONSEPT:
 * - Başlatılmamış bellek → Undefined Behavior
 * - Sıfırlama → Tüm alanlar güvenli başlangıç değerinde
 * - NULL pointer'lar, false flag'ler, 0 değerler
 *
 * COMPILE: gcc 03_zero_initialization_demo.c -o zero_init_demo
 * RUN: ./zero_init_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ========================================================================
 * ÖRNEK YAPILAR
 * ======================================================================== */

typedef struct {
    int* data_ptr;
    uint32_t count;
    bool initialized;
    uint8_t flags;
    char name[32];
} data_structure_t;

typedef struct node {
    int value;
    struct node* next;
    struct node* prev;
    bool is_head;
} list_node_t;

/* ========================================================================
 * DEMO 1: UNDEFINED BEHAVIOR PROBLEMI
 * ======================================================================== */

void demo_undefined_behavior() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 1: Undefined Behavior (Başlatılmamış Bellek)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[KÖTÜ ÖRNEK - Başlatılmamış Bellek]\n");

    // malloc ile tahsis et AMA sıfırlama!
    data_structure_t* bad_data = (data_structure_t*)malloc(sizeof(data_structure_t));

    printf("\n  Tahsis edildi ama sıfırlanmadı:\n");
    printf("  data_ptr:     %p  ← RASTGELE! ⚠️\n", (void*)bad_data->data_ptr);
    printf("  count:        %u  ← RASTGELE! ⚠️\n", bad_data->count);
    printf("  initialized:  %d  ← RASTGELE! ⚠️\n", bad_data->initialized);
    printf("  flags:        0x%02X  ← RASTGELE! ⚠️\n", bad_data->flags);
    printf("  name[0]:      '%c' (0x%02X)  ← RASTGELE! ⚠️\n",
           bad_data->name[0], (unsigned char)bad_data->name[0]);

    printf("\n  ⚠️  Tehlikeli Kullanım:\n");
    printf("      if (bad_data->initialized) { ... }  ← UNDEFINED!\n");
    printf("      if (bad_data->data_ptr != NULL) { ... }  ← UNDEFINED!\n");
    printf("      if (bad_data->count > 0) { ... }  ← UNDEFINED!\n");

    free(bad_data);

    printf("\n[İYİ ÖRNEK - Zero-Initialized]\n");

    // calloc ile tahsis et (otomatik sıfırlanır)
    data_structure_t* good_data = (data_structure_t*)calloc(1, sizeof(data_structure_t));

    printf("\n  Tahsis edildi ve sıfırlandı:\n");
    printf("  data_ptr:     %p  ← NULL ✓\n", (void*)good_data->data_ptr);
    printf("  count:        %u  ← 0 ✓\n", good_data->count);
    printf("  initialized:  %d  ← false (0) ✓\n", good_data->initialized);
    printf("  flags:        0x%02X  ← 0 ✓\n", good_data->flags);
    printf("  name[0]:      '\\0' (0x%02X)  ← NULL terminator ✓\n",
           (unsigned char)good_data->name[0]);

    printf("\n  ✓ Güvenli Kullanım:\n");
    printf("      if (good_data->initialized) { ... }  ← Her zaman false\n");
    printf("      if (good_data->data_ptr != NULL) { ... }  ← Her zaman false\n");
    printf("      if (good_data->count > 0) { ... }  ← Her zaman false\n");

    free(good_data);
}

/* ========================================================================
 * DEMO 2: FARKLI SIFIRLAMA YÖNTEMLERİ
 * ======================================================================== */

void demo_initialization_methods() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 2: Farklı Sıfırlama Yöntemleri\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[YÖNTEM 1: calloc()]\n");
    printf("  void* ptr = calloc(count, size);\n");
    printf("  ✓ Otomatik sıfırlanır\n");
    printf("  ✓ Kernel optimize edebilir\n");

    data_structure_t* data1 = (data_structure_t*)calloc(1, sizeof(data_structure_t));
    printf("  Sonuç: count=%u, flags=0x%02X ✓\n", data1->count, data1->flags);
    free(data1);

    printf("\n[YÖNTEM 2: malloc() + memset()]\n");
    printf("  void* ptr = malloc(size);\n");
    printf("  memset(ptr, 0, size);\n");
    printf("  ✓ Manuel kontrol\n");
    printf("  ✓ Custom allocator ile uyumlu\n");

    data_structure_t* data2 = (data_structure_t*)malloc(sizeof(data_structure_t));
    memset(data2, 0, sizeof(data_structure_t));
    printf("  Sonuç: count=%u, flags=0x%02X ✓\n", data2->count, data2->flags);
    free(data2);

    printf("\n[YÖNTEM 3: Struct initializer (C99)]\n");
    printf("  data_structure_t data = {0};  // Stack\n");
    printf("  ✓ Compile-time sıfırlama\n");
    printf("  ✓ Stack allocation için\n");

    data_structure_t data3 = {0};
    printf("  Sonuç: count=%u, flags=0x%02X ✓\n", data3.count, data3.flags);

    printf("\n[YÖNTEM 4: Designated initializers (C99)]\n");
    printf("  data_structure_t data = {\n");
    printf("      .data_ptr = NULL,\n");
    printf("      .count = 0,\n");
    printf("      // ...rest implicitly zero\n");
    printf("  };\n");

    data_structure_t data4 = {
        .data_ptr = NULL,
        .count = 0,
        .initialized = false
    };
    printf("  Sonuç: count=%u, flags=0x%02X ✓\n", data4.count, data4.flags);
}

/* ========================================================================
 * DEMO 3: NULL POINTER SAFETY
 * ======================================================================== */

void demo_null_pointer_safety() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 3: NULL Pointer Safety\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[SEnaryo: Linked List Node]\n");

    // Başlatılmamış node - TEHLİKELİ!
    printf("\n[KÖTÜ] malloc() + no init:\n");
    list_node_t* bad_node = (list_node_t*)malloc(sizeof(list_node_t));
    printf("  next pointer: %p  ← RASTGELE! ⚠️\n", (void*)bad_node->next);
    printf("  prev pointer: %p  ← RASTGELE! ⚠️\n", (void*)bad_node->prev);
    printf("\n  Tehlikeli kod:\n");
    printf("      if (node->next != NULL) {  ← UNDEFINED!\n");
    printf("          visit(node->next);     ← CRASH! ⚠️\n");
    printf("      }\n");
    free(bad_node);

    // Sıfırlanmış node - GÜVENLİ!
    printf("\n[İYİ] calloc() veya memset:\n");
    list_node_t* good_node = (list_node_t*)calloc(1, sizeof(list_node_t));
    printf("  next pointer: %p  ← NULL ✓\n", (void*)good_node->next);
    printf("  prev pointer: %p  ← NULL ✓\n", (void*)good_node->prev);
    printf("\n  Güvenli kod:\n");
    printf("      if (node->next != NULL) {  ← Her zaman false ✓\n");
    printf("          visit(node->next);     ← Çalışmaz (güvenli)\n");
    printf("      }\n");

    // NULL check örneği
    good_node->value = 42;
    if (good_node->next == NULL) {
        printf("\n  ✓ NULL check başarılı - node isolated\n");
    }

    // Liste sonuna ekleme - güvenli
    good_node->next = (list_node_t*)calloc(1, sizeof(list_node_t));
    good_node->next->value = 99;
    good_node->next->prev = good_node;

    printf("  ✓ İkinci node eklendi: value=%d\n", good_node->next->value);
    printf("  ✓ Backlink doğru: prev->value=%d\n", good_node->next->prev->value);

    free(good_node->next);
    free(good_node);
}

/* ========================================================================
 * DEMO 4: FLAG SAFETY
 * ======================================================================== */

void demo_flag_safety() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 4: Flag Safety\n");
    printf("═══════════════════════════════════════════════════════════\n");

    typedef struct {
        bool is_valid;
        bool is_dirty;
        bool needs_redraw;
        uint8_t state_flags;
    } widget_flags_t;

    printf("\n[KÖTÜ] Başlatılmamış flag'ler:\n");
    widget_flags_t* bad_flags = (widget_flags_t*)malloc(sizeof(widget_flags_t));

    printf("  is_valid:     %d  ← RASTGELE! ⚠️\n", bad_flags->is_valid);
    printf("  is_dirty:     %d  ← RASTGELE! ⚠️\n", bad_flags->is_dirty);
    printf("  needs_redraw: %d  ← RASTGELE! ⚠️\n", bad_flags->needs_redraw);
    printf("  state_flags:  0x%02X  ← RASTGELE! ⚠️\n", bad_flags->state_flags);

    printf("\n  Tehlikeli kod:\n");
    printf("      if (widget->is_valid && !widget->is_dirty) {\n");
    printf("          skip_update();  ← Yanlış karar! ⚠️\n");
    printf("      }\n");

    free(bad_flags);

    printf("\n[İYİ] Zero-initialized flag'ler:\n");
    widget_flags_t* good_flags = (widget_flags_t*)calloc(1, sizeof(widget_flags_t));

    printf("  is_valid:     %d  ← false ✓\n", good_flags->is_valid);
    printf("  is_dirty:     %d  ← false ✓\n", good_flags->is_dirty);
    printf("  needs_redraw: %d  ← false ✓\n", good_flags->needs_redraw);
    printf("  state_flags:  0x%02X  ← 0 (no flags) ✓\n", good_flags->state_flags);

    printf("\n  Güvenli kod:\n");
    printf("      if (widget->is_valid && !widget->is_dirty) {\n");
    printf("          // false && true = false ✓\n");
    printf("          // Çalışmaz (beklendiği gibi)\n");
    printf("      }\n");

    // Şimdi flag'leri ayarla
    good_flags->is_valid = true;
    good_flags->needs_redraw = true;

    printf("\n  Flag'ler ayarlandıktan sonra:\n");
    printf("      is_valid=%d, needs_redraw=%d\n",
           good_flags->is_valid, good_flags->needs_redraw);

    if (good_flags->is_valid && good_flags->needs_redraw && !good_flags->is_dirty) {
        printf("      ✓ Redraw gerekli, data clean\n");
    }

    free(good_flags);
}

/* ========================================================================
 * DEMO 5: PARTIAL vs FULL INITIALIZATION
 * ======================================================================== */

void demo_partial_initialization() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 5: Partial vs Full Initialization\n");
    printf("═══════════════════════════════════════════════════════════\n");

    typedef struct {
        int id;
        int x, y;
        char* name;
        bool visible;
        uint32_t* buffer;
    } object_t;

    printf("\n[KÖTÜ] Partial initialization:\n");
    printf("  object_t obj;\n");
    printf("  obj.id = 1;\n");
    printf("  obj.x = 10;\n");
    printf("  // name, visible, buffer başlatılmadı! ⚠️\n");

    object_t bad_obj;
    bad_obj.id = 1;
    bad_obj.x = 10;
    // name, visible, buffer başlatılmadı!

    printf("\n  Sonuç:\n");
    printf("    id=%d ✓\n", bad_obj.id);
    printf("    x=%d ✓\n", bad_obj.x);
    printf("    name=%p  ← RASTGELE! ⚠️\n", (void*)bad_obj.name);
    printf("    visible=%d  ← RASTGELE! ⚠️\n", bad_obj.visible);
    printf("    buffer=%p  ← RASTGELE! ⚠️\n", (void*)bad_obj.buffer);

    printf("\n[İYİ] Full zero-init + specific values:\n");
    printf("  object_t obj = {0};  // Önce hepsini sıfırla\n");
    printf("  obj.id = 1;          // Sonra değerleri ayarla\n");
    printf("  obj.x = 10;\n");

    object_t good_obj = {0};  // Tümü sıfırlandı
    good_obj.id = 1;
    good_obj.x = 10;

    printf("\n  Sonuç:\n");
    printf("    id=%d ✓\n", good_obj.id);
    printf("    x=%d ✓\n", good_obj.x);
    printf("    name=%p  ← NULL ✓\n", (void*)good_obj.name);
    printf("    visible=%d  ← false ✓\n", good_obj.visible);
    printf("    buffer=%p  ← NULL ✓\n", (void*)good_obj.buffer);

    printf("\n  ✓ Güvenli NULL check'ler:\n");
    if (good_obj.name == NULL) {
        printf("    name is NULL - safe ✓\n");
    }
    if (good_obj.buffer == NULL) {
        printf("    buffer is NULL - safe ✓\n");
    }
}

/* ========================================================================
 * DEMO 6: PERFORMANCE CONSIDERATIONS
 * ======================================================================== */

void demo_performance() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 6: Performance Considerations\n");
    printf("═══════════════════════════════════════════════════════════\n");

    const int COUNT = 10000;
    clock_t start, end;
    double cpu_time;

    // malloc only
    printf("\n[TEST 1] malloc() only (no zeroing):\n");
    start = clock();
    for (int i = 0; i < COUNT; i++) {
        data_structure_t* ptr = (data_structure_t*)malloc(sizeof(data_structure_t));
        free(ptr);
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("  %d allocation/free: %.3f ms\n", COUNT, cpu_time);

    // malloc + memset
    printf("\n[TEST 2] malloc() + memset():\n");
    start = clock();
    for (int i = 0; i < COUNT; i++) {
        data_structure_t* ptr = (data_structure_t*)malloc(sizeof(data_structure_t));
        memset(ptr, 0, sizeof(data_structure_t));
        free(ptr);
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("  %d allocation/zero/free: %.3f ms\n", COUNT, cpu_time);

    // calloc
    printf("\n[TEST 3] calloc():\n");
    start = clock();
    for (int i = 0; i < COUNT; i++) {
        data_structure_t* ptr = (data_structure_t*)calloc(1, sizeof(data_structure_t));
        free(ptr);
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("  %d allocation/zero/free: %.3f ms\n", COUNT, cpu_time);

    printf("\n[SONUÇ]\n");
    printf("  ✓ Zero-init overhead minimal (modern CPU'larda)\n");
    printf("  ✓ Güvenlik faydası overhead'ı fazlasıyla karşılar\n");
    printf("  ✓ calloc çekirdek seviyesinde optimize edilebilir\n");
}

/* ========================================================================
 * DEMO 7: BEST PRACTICES
 * ======================================================================== */

void demo_best_practices() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 7: Best Practices\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[PRACTICE 1] Her zaman sıfırla:\n");
    printf("  ✓ object_t* obj = calloc(1, sizeof(object_t));\n");
    printf("  veya\n");
    printf("  ✓ object_t* obj = malloc(sizeof(object_t));\n");
    printf("    memset(obj, 0, sizeof(object_t));\n");

    printf("\n[PRACTICE 2] Stack allocation için {0}:\n");
    printf("  ✓ object_t obj = {0};\n");

    printf("\n[PRACTICE 3] Sıfırdan farklı değerleri sonra ayarla:\n");
    printf("  ✓ object_t obj = {0};  // Önce sıfırla\n");
    printf("    obj.special_value = 42;  // Sonra ayarla\n");

    printf("\n[PRACTICE 4] Debug build'de validation:\n");
    printf("  #ifdef DEBUG\n");
    printf("    // Sıfırlanmış mı kontrol et\n");
    printf("    assert(obj->initialized == false);\n");
    printf("    assert(obj->ptr == NULL);\n");
    printf("  #endif\n");

    printf("\n[ANTI-PATTERN] Asla yapma:\n");
    printf("  ✗ object_t* obj = malloc(sizeof(object_t));\n");
    printf("    if (obj->field) { ... }  // ⚠️ UNDEFINED!\n");

    printf("\n  ✗ object_t obj;  // Başlatılmadı!\n");
    printf("    obj.id = 1;    // Sadece id ayarlandı\n");
    printf("    // obj.ptr hala rastgele! ⚠️\n");
}

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║      ZERO-INITIALIZATION (SIFIR BAŞLATMA) DEMO           ║\n");
    printf("║                                                           ║\n");
    printf("║  Konsept: Belleği tahsis sonrası sıfırla, güvenli başla  ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    demo_undefined_behavior();
    demo_initialization_methods();
    demo_null_pointer_safety();
    demo_flag_safety();
    demo_partial_initialization();
    demo_performance();
    demo_best_practices();

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                                   ║\n");
    printf("║                                                           ║\n");
    printf("║  ✓ Başlatılmamış bellek = Undefined Behavior             ║\n");
    printf("║  ✓ Zero-init = Tüm alanlar güvenli başlangıç değerinde   ║\n");
    printf("║  ✓ NULL pointer'lar, false flag'ler, 0 değerler          ║\n");
    printf("║  ✓ Minimal performance overhead                          ║\n");
    printf("║  ✓ Debug kolaylığı, öngörülebilir davranış               ║\n");
    printf("║                                                           ║\n");
    printf("║  📖 KURAL: Her allocation sonrası sıfırla!                ║\n");
    printf("║     calloc() veya memset(ptr, 0, size)                   ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
