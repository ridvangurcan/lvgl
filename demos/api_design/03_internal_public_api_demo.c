/*
 * INTERNAL VS PUBLIC API SEPARATION DEMO
 *
 * Bu demo, internal (dahili) ve public (harici) API ayrımının nasıl
 * yapıldığını ve neden önemli olduğunu gösterir.
 *
 * LVGL'de kullanılan strateji:
 * - Public API: lv_* prefix (kullanıcılar için, stabil)
 * - Internal API: _lv_* prefix (kütüphane içi, değişebilir)
 *
 * Gösterilen Konular:
 * 1. Prefix-based naming convention
 * 2. Public API stability (semantic versioning)
 * 3. Internal API flexibility (refactoring freedom)
 * 4. Documentation differences
 * 5. API surface reduction
 * 6. Clear contract definition
 *
 * Derleme: gcc 03_internal_public_api_demo.c -o api_separation_demo
 * Çalıştırma: ./api_separation_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * NAMING CONVENTION
 * ============================================================================
 *
 * Public API:  mylib_*    (kullanıcılar kullanır)
 * Internal API: _mylib_*  (sadece kütüphane içinde kullanılır)
 *
 * LVGL Örneği:
 *   lv_obj_create()         → Public
 *   _lv_obj_destruct()      → Internal
 *   lv_obj_set_width()      → Public
 *   _lv_inv_area()          → Internal
 */

/* ============================================================================
 * INTERNAL TYPES & DATA STRUCTURES
 * ============================================================================ */

// Internal - Kullanıcı görmemeli
typedef struct {
    void** items;
    size_t count;
    size_t capacity;
} _mylib_list_t;

// Internal - Memory pool
typedef struct {
    void* blocks[100];
    size_t used_count;
    size_t total_allocated;
    size_t total_freed;
} _mylib_mem_pool_t;

// Global internal state
static _mylib_mem_pool_t _mylib_memory_pool;
static bool _mylib_initialized = false;
static uint32_t _mylib_object_count = 0;

/* ============================================================================
 * INTERNAL FUNCTIONS (sadece kütüphane içinde kullanılır)
 * ============================================================================ */

/**
 * Internal: Kütüphaneyi başlat
 * @remarks Internal function, do not call directly
 */
void _mylib_init(void)
{
    if (_mylib_initialized) return;

    printf("[INTERNAL] _mylib_init() called\n");
    memset(&_mylib_memory_pool, 0, sizeof(_mylib_mem_pool_t));
    _mylib_initialized = true;
    _mylib_object_count = 0;
}

/**
 * Internal: Bellek tahsis et (internal pool'dan)
 * @remarks Internal function, do not call directly
 */
void* _mylib_malloc(size_t size)
{
    printf("[INTERNAL] _mylib_malloc(%zu bytes)\n", size);

    // Gerçek implementasyon malloc kullanır ama
    // kütüphane içinde tracking yapabilir
    void* ptr = malloc(size);

    if (ptr) {
        _mylib_memory_pool.total_allocated += size;
        // Pool'a kaydet (simplified)
        if (_mylib_memory_pool.used_count < 100) {
            _mylib_memory_pool.blocks[_mylib_memory_pool.used_count++] = ptr;
        }
    }

    return ptr;
}

/**
 * Internal: Belleği serbest bırak
 * @remarks Internal function, do not call directly
 */
void _mylib_free(void* ptr)
{
    if (!ptr) return;

    printf("[INTERNAL] _mylib_free(%p)\n", ptr);
    free(ptr);
    _mylib_memory_pool.total_freed += 1;  // Simplified
}

/**
 * Internal: Obje sayacını artır
 * @remarks Internal function, do not call directly
 */
void _mylib_object_increment(void)
{
    _mylib_object_count++;
    printf("[INTERNAL] Object count: %u\n", _mylib_object_count);
}

/**
 * Internal: Obje sayacını azalt
 * @remarks Internal function, do not call directly
 */
void _mylib_object_decrement(void)
{
    if (_mylib_object_count > 0) {
        _mylib_object_count--;
    }
    printf("[INTERNAL] Object count: %u\n", _mylib_object_count);
}

/**
 * Internal: List oluştur
 */
_mylib_list_t* _mylib_list_create(void)
{
    _mylib_list_t* list = (_mylib_list_t*)_mylib_malloc(sizeof(_mylib_list_t));
    if (!list) return NULL;

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;

    return list;
}

/**
 * Internal: List'e eleman ekle
 */
bool _mylib_list_add(_mylib_list_t* list, void* item)
{
    if (!list) return false;

    // Capacity kontrolü (simplified)
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        void** new_items = (void**)realloc(list->items, new_capacity * sizeof(void*));
        if (!new_items) return false;

        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count++] = item;
    return true;
}

/**
 * Internal: List'i yok et
 */
void _mylib_list_destroy(_mylib_list_t* list)
{
    if (!list) return;

    if (list->items) {
        free(list->items);
    }
    _mylib_free(list);
}

/**
 * Internal: Obje yıkımı (destructor)
 * Kullanıcı mylib_object_delete() kullanmalı, bunu değil
 */
void _mylib_object_destruct(void* obj)
{
    if (!obj) return;

    printf("[INTERNAL] _mylib_object_destruct() - cleanup internals\n");

    // İç temizlik işlemleri
    // - Event listener'ları temizle
    // - Child objeleri bilgilendir
    // - Resource'ları serbest bırak

    _mylib_object_decrement();
}

/**
 * Internal: Invalidation işlemi
 */
void _mylib_invalidate_area(int x, int y, int w, int h)
{
    printf("[INTERNAL] _mylib_invalidate_area(%d,%d,%dx%d) - mark for redraw\n",
           x, y, w, h);

    // Gerçek implementasyon:
    // - Dirty region listesine ekle
    // - Redraw timer'ı başlat
}

/* ============================================================================
 * PUBLIC API (kullanıcılar için stabil arayüz)
 * ============================================================================ */

// Opaque type - kullanıcı detayları görmez
typedef struct mylib_object_t mylib_object_t;

// Gerçek struct tanımı (internal)
struct mylib_object_t {
    int x, y;
    int width, height;
    char label[64];
    _mylib_list_t* children;  // Internal type kullanımı
    void* internal_data;
};

/**
 * Initialize the library
 *
 * @remarks This must be called before any other library function
 */
void mylib_init(void)
{
    printf("[PUBLIC] mylib_init() - User-facing initialization\n");
    _mylib_init();  // Internal init çağrılır
}

/**
 * Create a new object
 *
 * @param label Object label
 * @return Pointer to the created object, or NULL on failure
 */
mylib_object_t* mylib_object_create(const char* label)
{
    printf("[PUBLIC] mylib_object_create(\"%s\")\n", label ? label : "NULL");

    // Internal memory allocator kullan
    mylib_object_t* obj = (mylib_object_t*)_mylib_malloc(sizeof(mylib_object_t));
    if (!obj) return NULL;

    // Initialize
    obj->x = 0;
    obj->y = 0;
    obj->width = 100;
    obj->height = 50;
    strncpy(obj->label, label ? label : "Object", sizeof(obj->label) - 1);
    obj->label[sizeof(obj->label) - 1] = '\0';

    // Internal list oluştur
    obj->children = _mylib_list_create();
    obj->internal_data = NULL;

    // Internal counter artır
    _mylib_object_increment();

    return obj;
}

/**
 * Delete an object
 *
 * @param obj Object to delete
 */
void mylib_object_delete(mylib_object_t* obj)
{
    if (!obj) return;

    printf("[PUBLIC] mylib_object_delete() - User calls this\n");

    // Internal destructor çağır
    _mylib_object_destruct(obj);

    // Internal list yok et
    _mylib_list_destroy(obj->children);

    // Internal memory deallocator kullan
    _mylib_free(obj);
}

/**
 * Set object position
 *
 * @param obj Object
 * @param x X coordinate
 * @param y Y coordinate
 */
void mylib_object_set_pos(mylib_object_t* obj, int x, int y)
{
    if (!obj) return;

    printf("[PUBLIC] mylib_object_set_pos(%d, %d)\n", x, y);

    obj->x = x;
    obj->y = y;

    // Internal invalidation çağır
    _mylib_invalidate_area(obj->x, obj->y, obj->width, obj->height);
}

/**
 * Set object size
 *
 * @param obj Object
 * @param width Width
 * @param height Height
 */
void mylib_object_set_size(mylib_object_t* obj, int width, int height)
{
    if (!obj) return;

    printf("[PUBLIC] mylib_object_set_size(%d, %d)\n", width, height);

    obj->width = width;
    obj->height = height;

    // Internal invalidation
    _mylib_invalidate_area(obj->x, obj->y, obj->width, obj->height);
}

/**
 * Get object position
 *
 * @param obj Object
 * @param x Pointer to store X coordinate
 * @param y Pointer to store Y coordinate
 */
void mylib_object_get_pos(const mylib_object_t* obj, int* x, int* y)
{
    if (!obj) return;
    if (x) *x = obj->x;
    if (y) *y = obj->y;
}

/**
 * Get library version
 *
 * @return Version string (e.g., "1.2.3")
 */
const char* mylib_get_version(void)
{
    return "1.0.0";  // Public API versiyonu
}

/**
 * Get library statistics (for debugging)
 *
 * @param total_objects Pointer to store total object count
 * @param total_allocated Pointer to store total allocated bytes
 */
void mylib_get_stats(uint32_t* total_objects, size_t* total_allocated)
{
    printf("[PUBLIC] mylib_get_stats() - Exposes some internal stats\n");

    if (total_objects) {
        *total_objects = _mylib_object_count;  // Internal counter
    }
    if (total_allocated) {
        *total_allocated = _mylib_memory_pool.total_allocated;  // Internal data
    }
}

/* ============================================================================
 * DEMONSTRATIONS
 * ============================================================================ */

void print_separator(const char* title)
{
    printf("\n========================================\n");
    printf("%s\n", title);
    printf("========================================\n");
}

void demonstrate_correct_usage(void)
{
    print_separator("✓ DOĞRU KULLANIM: Public API");

    // Public API kullanımı
    mylib_init();

    mylib_object_t* obj1 = mylib_object_create("Button");
    mylib_object_t* obj2 = mylib_object_create("Label");

    mylib_object_set_pos(obj1, 10, 20);
    mylib_object_set_size(obj1, 100, 50);

    mylib_object_set_pos(obj2, 120, 20);
    mylib_object_set_size(obj2, 200, 30);

    uint32_t obj_count;
    size_t allocated;
    mylib_get_stats(&obj_count, &allocated);
    printf("\nStats: %u objects, %zu bytes allocated\n", obj_count, allocated);

    mylib_object_delete(obj1);
    mylib_object_delete(obj2);

    printf("\n✓ Kullanıcı sadece public API kullandı\n");
    printf("✓ Internal detaylardan habersiz\n");
}

void demonstrate_wrong_usage(void)
{
    print_separator("❌ YANLIŞ KULLANIM: Internal API");

    printf("Kullanıcı internal API'leri kullanmamalı:\n\n");

    printf("❌ _mylib_init()                 // Public mylib_init() kullan\n");
    printf("❌ _mylib_malloc(100)            // malloc() kullan veya public API\n");
    printf("❌ _mylib_object_destruct(obj)   // Public mylib_object_delete() kullan\n");
    printf("❌ _mylib_invalidate_area(...)   // Otomatik çağrılır, manuel çağırma\n");

    printf("\nNeden kullanılmamalı?\n");
    printf("  • Internal API değişebilir (breaking changes)\n");
    printf("  • Dokümante edilmemiş\n");
    printf("  • Validation olmayabilir\n");
    printf("  • Side effect'ler eksik olabilir\n");
    printf("  • Binary uyumluluk garantisi yok\n");
}

void demonstrate_api_evolution(void)
{
    print_separator("API EVOLUTION (Version Değişiklikleri)");

    printf("═══ VERSION 1.0.0 ═══\n");
    printf("Public API:\n");
    printf("  mylib_init()                  ← Stabil\n");
    printf("  mylib_object_create()         ← Stabil\n");
    printf("  mylib_object_delete()         ← Stabil\n");
    printf("\nInternal API:\n");
    printf("  _mylib_init()                 ← İç detay\n");
    printf("  _mylib_malloc()               ← İç detay\n");

    printf("\n═══ VERSION 1.1.0 ═══\n");
    printf("Public API (DEĞİŞMEDİ!):\n");
    printf("  mylib_init()                  ← Aynı\n");
    printf("  mylib_object_create()         ← Aynı\n");
    printf("  mylib_object_delete()         ← Aynı\n");
    printf("\nInternal API (DEĞİŞTİ!):\n");
    printf("  _mylib_init_with_config()     ← İsim değişti\n");
    printf("  _mylib_malloc_aligned()       ← Parametre eklendi\n");
    printf("  _mylib_object_destruct_async()← Async yapıldı\n");

    printf("\n═══ VERSION 2.0.0 ═══\n");
    printf("Public API (BREAKING CHANGE - Major version bump):\n");
    printf("  mylib_init_ex()               ← Yeni parametre (breaking!)\n");
    printf("  mylib_object_create()         ← Aynı\n");
    printf("  mylib_object_delete()         ← Aynı\n");

    printf("\n✓ Internal API istediğimiz gibi değişebilir (minor version)\n");
    printf("✓ Public API değişirse major version artırılır\n");
    printf("✓ Semantic Versioning: MAJOR.MINOR.PATCH\n");
}

void demonstrate_api_surface(void)
{
    print_separator("API SURFACE (Yüzey Alanı)");

    printf("Kütüphane içinde 50 fonksiyon olabilir:\n\n");

    printf("PUBLIC API (15 fonksiyon):\n");
    printf("  mylib_init()                    ✓ Dokümante\n");
    printf("  mylib_object_create()           ✓ Dokümante\n");
    printf("  mylib_object_delete()           ✓ Dokümante\n");
    printf("  mylib_object_set_pos()          ✓ Dokümante\n");
    printf("  mylib_object_get_pos()          ✓ Dokümante\n");
    printf("  ... (10 more)\n");

    printf("\nINTERNAL API (35 fonksiyon):\n");
    printf("  _mylib_init()                   ✗ Dokümante değil\n");
    printf("  _mylib_malloc()                 ✗ Dokümante değil\n");
    printf("  _mylib_free()                   ✗ Dokümante değil\n");
    printf("  _mylib_list_create()            ✗ Dokümante değil\n");
    printf("  _mylib_object_destruct()        ✗ Dokümante değil\n");
    printf("  _mylib_invalidate_area()        ✗ Dokümante değil\n");
    printf("  ... (29 more)\n");

    printf("\nAvantajlar:\n");
    printf("  ✓ Kullanıcı sadece 15 fonksiyon öğrenir (35 değil!)\n");
    printf("  ✓ Dokümantasyon yükü 70%% azalır\n");
    printf("  ✓ API berraklığı artar\n");
    printf("  ✓ Yanlış kullanım azalır\n");
}

void demonstrate_refactoring_freedom(void)
{
    print_separator("REFACTORING FREEDOM (Yeniden Yapılandırma Özgürlüğü)");

    printf("Internal API değişiklikleri kullanıcıyı etkilemez:\n\n");

    printf("Senaryo: Memory allocator'ı değiştiriyoruz\n\n");

    printf("═══ ÖNCESİ ═══\n");
    printf("void _mylib_malloc(size_t size) {\n");
    printf("    return malloc(size);  // Standard malloc\n");
    printf("}\n");

    printf("\n═══ SONRASI ═══\n");
    printf("void _mylib_malloc(size_t size) {\n");
    printf("    return custom_pool_alloc(&pool, size);  // Custom allocator\n");
    printf("}\n");

    printf("\n✓ Public API etkilenmez:\n");
    printf("  mylib_object_create()  // Aynı çalışır\n");
    printf("  mylib_object_delete()  // Aynı çalışır\n");

    printf("\n✓ Kullanıcı kodunu yeniden derlemeye bile gerek yok!\n");

    printf("\n═══ BAŞKA BİR SENARYO ═══\n");
    printf("Internal invalidation algoritmasını değiştiriyoruz:\n\n");

    printf("void _mylib_invalidate_area(...) {\n");
    printf("    // Version 1: Immediate redraw\n");
    printf("    // Version 2: Batch invalidation\n");
    printf("    // Version 3: Dirty region merging\n");
    printf("}\n");

    printf("\n✓ mylib_object_set_pos() API'si değişmez\n");
    printf("✓ Kullanıcı yeni optimizasyondan faydalanır\n");
}

void demonstrate_clear_contract(void)
{
    print_separator("CLEAR CONTRACT (Açık Sözleşme)");

    printf("Public API = Kullanıcı ile kütüphane arasındaki sözleşme\n\n");

    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│                  KULLANICI KODU                     │\n");
    printf("└─────────────────────────────────────────────────────┘\n");
    printf("                         │\n");
    printf("                         │ mylib_object_create()\n");
    printf("                         │ mylib_object_set_pos()\n");
    printf("                         │ mylib_object_delete()\n");
    printf("                         ▼\n");
    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│              PUBLIC API (Contract)                  │\n");
    printf("│  - Stabil                                           │\n");
    printf("│  - Dokümante                                        │\n");
    printf("│  - Semantic versioning                              │\n");
    printf("└─────────────────────────────────────────────────────┘\n");
    printf("                         │\n");
    printf("                         ▼\n");
    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│          INTERNAL IMPLEMENTATION                    │\n");
    printf("│  - _mylib_malloc()                                  │\n");
    printf("│  - _mylib_object_destruct()                         │\n");
    printf("│  - _mylib_invalidate_area()                         │\n");
    printf("│  - İstediğimiz gibi değiştirebiliriz                │\n");
    printf("└─────────────────────────────────────────────────────┘\n");

    printf("\nSözleşme kuralları:\n");
    printf("  ✓ Public API: Breaking change = Major version bump\n");
    printf("  ✓ Public API: New feature = Minor version bump\n");
    printf("  ✓ Public API: Bug fix = Patch version bump\n");
    printf("  ✓ Internal API: İstediğimiz gibi değişir\n");
}

void compare_approaches(void)
{
    print_separator("KARŞILAŞTIRMA");

    printf("┌──────────────────────┬──────────────┬──────────────────┐\n");
    printf("│ Özellik              │ Ayrım Yok    │ Internal/Public  │\n");
    printf("├──────────────────────┼──────────────┼──────────────────┤\n");
    printf("│ API Berraklığı       │ ⭐⭐         │ ⭐⭐⭐⭐⭐        │\n");
    printf("│ Refactoring Kolaylığı│ ⭐⭐         │ ⭐⭐⭐⭐⭐        │\n");
    printf("│ Dokümantasyon        │ ⭐⭐         │ ⭐⭐⭐⭐⭐        │\n");
    printf("│ API Stability        │ ⭐⭐         │ ⭐⭐⭐⭐⭐        │\n");
    printf("│ Kullanım Kolaylığı   │ ⭐⭐⭐       │ ⭐⭐⭐⭐⭐        │\n");
    printf("│ Yanlış Kullanım      │ Yüksek       │ Düşük            │\n");
    printf("│ Öğrenme Eğrisi       │ Zor          │ Kolay            │\n");
    printf("└──────────────────────┴──────────────┴──────────────────┘\n");

    printf("\nAyrım Olmadan:\n");
    printf("  • Kullanıcı tüm fonksiyonları görür (50+)\n");
    printf("  • Hangisini kullanacağını bilemez\n");
    printf("  • Yanlış fonksiyon kullanabilir\n");
    printf("  • Dokümantasyon karmaşık\n");
    printf("  • API değişiklikleri her zaman breaking\n");

    printf("\nInternal/Public Ayrımı:\n");
    printf("  • Kullanıcı sadece 15 public fonksiyon görür\n");
    printf("  • Ne kullanacağı açık\n");
    printf("  • Yanlış kullanamaz (internal gizli)\n");
    printf("  • Dokümantasyon sade\n");
    printf("  • Internal değişse sorun yok\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║    INTERNAL VS PUBLIC API SEPARATION (DAHİLİ-HARİCİ)      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    demonstrate_correct_usage();
    demonstrate_wrong_usage();
    demonstrate_api_evolution();
    demonstrate_api_surface();
    demonstrate_refactoring_freedom();
    demonstrate_clear_contract();
    compare_approaches();

    print_separator("ÖZET");
    printf("Internal vs Public API Separation:\n");
    printf("✓ Clear Contract - API sözleşmesi açık\n");
    printf("✓ API Surface Reduction - Daha az fonksiyon öğren\n");
    printf("✓ Refactoring Freedom - İç yapıyı değiştir\n");
    printf("✓ Documentation - Sadece public dokümante et\n");
    printf("✓ Stability - Public API stabil, internal esnek\n");
    printf("✓ Wrong Usage Prevention - Yanlış kullanımı engelle\n");
    printf("\n");
    printf("LVGL'de Adlandırma Konvansiyonu:\n");
    printf("• lv_*     → Public API (kullanıcılar için)\n");
    printf("• _lv_*    → Internal API (kütüphane içi)\n");
    printf("\n");
    printf("LVGL Örnekleri:\n");
    printf("  Public:   lv_init(), lv_obj_create(), lv_obj_del()\n");
    printf("  Internal: _lv_refr_init(), _lv_obj_destruct(), _lv_inv_area()\n");
    printf("\n");

    return 0;
}
