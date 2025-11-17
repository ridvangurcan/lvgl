/*
 * CONST CORRECTNESS (CONST DOĞRULUĞU) DEMO
 *
 * Bu demo, `const` keyword'ünün doğru kullanımını ve faydalarını gösterir.
 * LVGL'de yaygın olarak kullanılan const correctness, API güvenliğini ve
 * derleyici optimizasyonlarını sağlar.
 *
 * Gösterilen Konular:
 * 1. Const parameters (read-only fonksiyon parametreleri)
 * 2. Const return types (değiştirilemez return değerleri)
 * 3. Const struct members (değişmez struct alanları)
 * 4. Const levels (const seviyeleri)
 * 5. Compiler optimizations
 * 6. Embedded benefits (ROM vs RAM)
 * 7. API intent documentation
 * 8. Compile-time error prevention
 *
 * Derleme: gcc 04_const_correctness_demo.c -o const_demo -Wall
 * Çalıştırma: ./const_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * PART 1: CONST PARAMETERS (Read-Only Objects)
 * ============================================================================ */

typedef struct {
    int x, y;
    int width, height;
    uint32_t flags;
} widget_t;

// ❌ BAD: Non-const parameter (fonksiyon sadece okuyorsa bile)
int bad_get_area(widget_t* w)
{
    return w->width * w->height;
}

// ✓ GOOD: Const parameter (fonksiyon sadece okuyor)
int good_get_area(const widget_t* w)
{
    if (!w) return 0;
    return w->width * w->height;
}

// ✓ GOOD: Const parameter ile yanlış kullanımı engelleme
bool widget_is_valid(const widget_t* w)
{
    if (!w) return false;

    // Sadece okur, değiştirmez
    if (w->width <= 0 || w->height <= 0) return false;

    // w->width = 100;  // ❌ DERLEME HATASI: const qualifier discarded

    return true;
}

// Setter - non-const parameter (çünkü değiştirir)
void widget_set_width(widget_t* w, int width)
{
    if (!w) return;
    w->width = width;  // ✓ OK: Non-const olduğu için değiştirebilir
}

// Getter - const parameter (çünkü sadece okur)
int widget_get_width(const widget_t* w)
{
    return w ? w->width : 0;
}

void demonstrate_const_parameters(void)
{
    printf("========================================\n");
    printf("CONST PARAMETERS (Sabit Parametreler)\n");
    printf("========================================\n");

    widget_t w = {0, 0, 100, 50, 0};

    printf("\nGetter ile okuma (const param):\n");
    int width = widget_get_width(&w);
    printf("  Width: %d\n", width);

    printf("\nSetter ile yazma (non-const param):\n");
    widget_set_width(&w, 200);
    printf("  Width: %d\n", widget_get_width(&w));

    printf("\nValidation (const param):\n");
    printf("  Is valid? %s\n", widget_is_valid(&w) ? "Yes" : "No");

    printf("\n✓ Const parameters API intent'ini belgelendirir\n");
    printf("✓ Yanlış kullanımı derleme zamanında yakalar\n");
}

/* ============================================================================
 * PART 2: CONST RETURN TYPES (Immutable Return Values)
 * ============================================================================ */

typedef struct {
    const char* name;  // Const pointer
    int id;
} object_class_t;

// Global class tanımları (değişmemeli)
static const object_class_t BUTTON_CLASS = {"Button", 1};
static const object_class_t LABEL_CLASS = {"Label", 2};
static const object_class_t IMAGE_CLASS = {"Image", 3};

typedef struct {
    const object_class_t* class_p;  // Const class pointer
    void* user_data;
} object_t;

// ✓ GOOD: Const pointer döner (class değiştirilemez)
const object_class_t* object_get_class(const object_t* obj)
{
    return obj ? obj->class_p : NULL;
}

// ✓ GOOD: Const string döner (değiştirilemez)
const char* object_get_name(const object_t* obj)
{
    if (!obj || !obj->class_p) return "";
    return obj->class_p->name;
}

// Const data array (lookup table)
static const uint8_t GAMMA_TABLE[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4,
    // ... (simplified)
};

// Const array döner
const uint8_t* get_gamma_table(void)
{
    return GAMMA_TABLE;
}

void demonstrate_const_return_types(void)
{
    printf("\n========================================\n");
    printf("CONST RETURN TYPES (Sabit Dönüş Tipleri)\n");
    printf("========================================\n");

    object_t btn;
    btn.class_p = &BUTTON_CLASS;
    btn.user_data = NULL;

    printf("\nClass bilgisi al (const döner):\n");
    const object_class_t* cls = object_get_class(&btn);
    printf("  Class name: %s\n", cls->name);
    printf("  Class ID: %d\n", cls->id);

    // cls->id = 999;  // ❌ DERLEME HATASI: assignment of read-only location

    printf("\nName al (const string):\n");
    const char* name = object_get_name(&btn);
    printf("  Name: %s\n", name);

    // name[0] = 'X';  // ❌ DERLEME HATASI: assignment of read-only location

    printf("\nLookup table (const array):\n");
    const uint8_t* gamma = get_gamma_table();
    printf("  Gamma[10]: %u\n", gamma[10]);

    // gamma[10] = 99;  // ❌ DERLEME HATASI: assignment of read-only location

    printf("\n✓ Const return types veriyi korur\n");
    printf("✓ Yanlışlıkla değiştirilmeyi engeller\n");
}

/* ============================================================================
 * PART 3: CONST STRUCT MEMBERS (Unchangeable Fields)
 * ============================================================================ */

typedef struct {
    const object_class_t* class_p;  // ← Const: Oluşturulduktan sonra değişmez
    int x, y;                        // ← Non-const: Değişebilir
    void* user_data;                 // ← Non-const: Değişebilir
} object_with_const_t;

void object_init(object_with_const_t* obj, const object_class_t* class_p)
{
    if (!obj) return;

    // Constructor'da bir kez set edilir
    *(const object_class_t**)&obj->class_p = class_p;  // Cast away const (sadece init'te!)

    obj->x = 0;
    obj->y = 0;
    obj->user_data = NULL;
}

void demonstrate_const_members(void)
{
    printf("\n========================================\n");
    printf("CONST STRUCT MEMBERS (Sabit Struct Alanları)\n");
    printf("========================================\n");

    object_with_const_t obj;
    object_init(&obj, &BUTTON_CLASS);

    printf("\nObject oluşturuldu:\n");
    printf("  Class: %s\n", obj.class_p->name);
    printf("  Position: (%d, %d)\n", obj.x, obj.y);

    printf("\nPosition değiştirilebilir:\n");
    obj.x = 100;
    obj.y = 50;
    printf("  Position: (%d, %d)\n", obj.x, obj.y);

    printf("\nAma class değiştirilemez:\n");
    // obj.class_p = &LABEL_CLASS;  // ❌ DERLEME HATASI: assignment of read-only member

    printf("✓ Const member yanlışlıkla değiştirilmeyi engeller\n");
    printf("✓ Object yaşam döngüsü boyunca class sabit kalır\n");
}

/* ============================================================================
 * PART 4: CONST LEVELS (Farklı Const Seviyeleri)
 * ============================================================================ */

void demonstrate_const_levels(void)
{
    printf("\n========================================\n");
    printf("CONST LEVELS (Const Seviyeleri)\n");
    printf("========================================\n");

    widget_t w = {0, 0, 100, 50, 0};

    printf("\n1. Pointer to non-const:\n");
    widget_t* p1 = &w;
    p1->width = 200;    // ✓ OK: İçerik değişebilir
    p1 = NULL;          // ✓ OK: Pointer değişebilir
    printf("  widget_t* p  → Her şey değişebilir\n");

    printf("\n2. Pointer to const (en yaygın):\n");
    const widget_t* p2 = &w;
    // p2->width = 300;    // ❌ HATA: İçerik değiştirilemez
    p2 = NULL;             // ✓ OK: Pointer değişebilir
    printf("  const widget_t* p  → İçerik değiştirilemez, pointer değişebilir\n");
    printf("  (LVGL'de en yaygın kullanım)\n");

    printf("\n3. Const pointer to non-const:\n");
    widget_t* const p3 = &w;
    p3->width = 400;    // ✓ OK: İçerik değişebilir
    // p3 = NULL;       // ❌ HATA: Pointer değiştirilemez
    printf("  widget_t* const p  → İçerik değişebilir, pointer değiştirilemez\n");

    printf("\n4. Const pointer to const:\n");
    const widget_t* const p4 = &w;
    // p4->width = 500; // ❌ HATA: İçerik değiştirilemez
    // p4 = NULL;       // ❌ HATA: Pointer değiştirilemez
    printf("  const widget_t* const p  → Hiçbir şey değiştirilemez\n");

    printf("\n5. Double const (pointer to const pointer to const):\n");
    printf("  const char* const* p  → Çok katmanlı const\n");
    printf("  LVGL'de nadiren kullanılır\n");
}

/* ============================================================================
 * PART 5: COMPILER OPTIMIZATIONS
 * ============================================================================ */

// Non-const global
int g_counter = 0;

// Const global (compiler optimize edebilir)
const int g_max_width = 1024;

void demonstrate_optimizations(void)
{
    printf("\n========================================\n");
    printf("COMPILER OPTIMIZATIONS\n");
    printf("========================================\n");

    printf("\nConst ile compiler optimizasyonları:\n");

    printf("\n1. Const local variable:\n");
    const int max = 100;
    int sum = 0;
    for (int i = 0; i < max; i++) {  // Compiler max'ı sabit olarak bilir
        sum += i;
    }
    printf("  Sum: %d (compiler loop'u optimize edebilir)\n", sum);

    printf("\n2. Const function parameter:\n");
    printf("  int get_area(const widget_t* w);\n");
    printf("  → Compiler w'nin değişmediğini bilir\n");
    printf("  → Pointer aliasing optimizasyonları yapabilir\n");

    printf("\n3. Const global (compile-time constant):\n");
    printf("  const int MAX_WIDTH = %d;\n", g_max_width);
    printf("  → Compiler sabiti kod içine gömebilir (inlining)\n");

    printf("\n4. Const array (lookup table):\n");
    printf("  const uint8_t TABLE[] = {...};\n");
    printf("  → Embedded sistemlerde ROM'a konulabilir\n");
    printf("  → RAM tasarrufu sağlar\n");

    printf("\n✓ Const optimizasyonları code size ve speed iyileştirir\n");
}

/* ============================================================================
 * PART 6: EMBEDDED BENEFITS (ROM vs RAM)
 * ============================================================================ */

// Non-const data → RAM'de saklanır
static uint8_t ram_buffer[1024];

// Const data → ROM'da saklanabilir (Flash)
static const uint8_t rom_lookup_table[256] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    // ... (256 element)
};

// Const struct → ROM'da
static const object_class_t ROM_CLASSES[] = {
    {"Button", 1},
    {"Label", 2},
    {"Image", 3},
    {"Slider", 4},
};

void demonstrate_embedded_benefits(void)
{
    printf("\n========================================\n");
    printf("EMBEDDED BENEFITS (Gömülü Sistem Faydaları)\n");
    printf("========================================\n");

    printf("\nBellek kullanımı:\n\n");

    printf("Non-const data (RAM):\n");
    printf("  uint8_t buffer[1024];        → 1024 bytes RAM\n");
    printf("  - Read/Write erişim\n");
    printf("  - Değiştirilebilir\n");
    printf("  - Kıymetli RAM kullanır\n");

    printf("\nConst data (ROM/Flash):\n");
    printf("  const uint8_t table[256];    → 256 bytes ROM\n");
    printf("  - Read-only erişim\n");
    printf("  - Değiştirilemez\n");
    printf("  - RAM tasarrufu (0 byte RAM!)\n");

    printf("\nÖrnek embedded sistem:\n");
    printf("  RAM: 64 KB (kıymetli!)\n");
    printf("  ROM: 512 KB (bol)\n");
    printf("\n  Const kullanarak:\n");
    printf("  - Lookup tables ROM'da → RAM tasarrufu\n");
    printf("  - Strings ROM'da → RAM tasarrufu\n");
    printf("  - Class definitions ROM'da → RAM tasarrufu\n");

    printf("\nLVGL örneği:\n");
    printf("  const lv_font_t font;        → ROM'da\n");
    printf("  const lv_img_dsc_t image;    → ROM'da\n");
    printf("  const lv_style_t style;      → ROM'da\n");
    printf("  → Binlerce byte RAM tasarrufu!\n");

    printf("\n✓ Embedded sistemlerde const kritik önem taşır\n");
}

/* ============================================================================
 * PART 7: API INTENT DOCUMENTATION
 * ============================================================================ */

// Intent: "Bu fonksiyon sadece okur, değiştirmez"
int calculate_perimeter(const widget_t* w)
{
    if (!w) return 0;
    return 2 * (w->width + w->height);
}

// Intent: "Bu fonksiyon değiştirir"
void reset_widget(widget_t* w)
{
    if (!w) return;
    w->x = 0;
    w->y = 0;
    w->width = 100;
    w->height = 50;
}

// Intent: "Class'ı döner ama değiştirilemez"
const object_class_t* find_class_by_id(int id)
{
    for (size_t i = 0; i < sizeof(ROM_CLASSES) / sizeof(ROM_CLASSES[0]); i++) {
        if (ROM_CLASSES[i].id == id) {
            return &ROM_CLASSES[i];
        }
    }
    return NULL;
}

void demonstrate_api_intent(void)
{
    printf("\n========================================\n");
    printf("API INTENT DOCUMENTATION\n");
    printf("========================================\n");

    printf("\nConst API'nin intent'i açıkça belirtir:\n\n");

    printf("1. int calculate_perimeter(const widget_t* w);\n");
    printf("   Intent: \"Sadece okuyorum, değiştirmiyorum\"\n");
    printf("   → Kullanıcı güvenle çağırabilir\n");

    printf("\n2. void reset_widget(widget_t* w);\n");
    printf("   Intent: \"Widget'ı değiştireceğim\"\n");
    printf("   → Kullanıcı side effect bekler\n");

    printf("\n3. const object_class_t* get_class(...);\n");
    printf("   Intent: \"Class dönerim ama değiştirme\"\n");
    printf("   → Kullanıcı okuyabilir ama değiştiremez\n");

    printf("\n4. void process(const uint8_t* input, uint8_t* output);\n");
    printf("   Intent: \"input oku, output yaz\"\n");
    printf("   → Açık input/output ayrımı\n");

    printf("\n✓ Const olmadan intent belirsizdir\n");
    printf("✓ Dokümantasyon okumaya gerek kalmaz\n");
}

/* ============================================================================
 * PART 8: COMPILE-TIME ERROR PREVENTION
 * ============================================================================ */

void demonstrate_compile_time_safety(void)
{
    printf("\n========================================\n");
    printf("COMPILE-TIME ERROR PREVENTION\n");
    printf("========================================\n");

    const widget_t w = {0, 0, 100, 50, 0};

    printf("\nDerleme zamanında yakalanan hatalar:\n\n");

    printf("1. Const objeyi değiştirme denemesi:\n");
    printf("   const widget_t w = {...};\n");
    printf("   w.width = 200;  // ❌ DERLEME HATASI\n");
    printf("   → Runtime'da değil, compile-time'da yakalanır!\n");

    printf("\n2. Const pointer'ı non-const fonksiyona geçme:\n");
    printf("   void modify(widget_t* w);  // Non-const param\n");
    printf("   const widget_t* cw = &w;\n");
    printf("   modify(cw);  // ❌ DERLEME HATASI (const qualifier discarded)\n");
    printf("   → Yanlışlıkla const objeyi değiştirmeyi engeller\n");

    printf("\n3. Const return'ü değiştirme:\n");
    printf("   const char* name = get_name();\n");
    printf("   name[0] = 'X';  // ❌ DERLEME HATASI\n");
    printf("   → String literal'ları korur\n");

    printf("\n4. Const array'i değiştirme:\n");
    printf("   const int arr[] = {1,2,3};\n");
    printf("   arr[0] = 10;  // ❌ DERLEME HATASI\n");
    printf("   → Lookup table'ları korur\n");

    printf("\n✓ Const hatları derleme zamanında yakalar\n");
    printf("✓ Runtime bug'larını önler\n");
    printf("✓ Debugging zamanından tasarruf\n");
}

/* ============================================================================
 * PART 9: REAL-WORLD EXAMPLE
 * ============================================================================ */

// Font struct (ROM'da saklanacak)
typedef struct {
    const char* name;
    uint8_t height;
    const uint8_t* bitmap_data;  // Const bitmap (ROM'da)
} font_t;

// Font bitmap data (ROM'da - const)
static const uint8_t FONT_ARIAL_12_BITMAP[] = {
    0xFF, 0x00, 0xAA, 0x55, 0xFF, 0x00, 0xAA, 0x55,
    // ... (binlerce byte)
};

// Font tanımı (ROM'da - const)
static const font_t FONT_ARIAL_12 = {
    .name = "Arial",
    .height = 12,
    .bitmap_data = FONT_ARIAL_12_BITMAP,
};

// Font getter (const döner)
const font_t* get_default_font(void)
{
    return &FONT_ARIAL_12;
}

// Text render (font sadece okunur)
void render_text(const char* text, const font_t* font, int x, int y)
{
    if (!text || !font) return;

    printf("Rendering '%s' at (%d,%d) with font '%s' (height: %d)\n",
           text, x, y, font->name, font->height);

    // Font'u oku ama değiştirme
    // font->height = 24;  // ❌ DERLEME HATASI
}

void demonstrate_real_world(void)
{
    printf("\n========================================\n");
    printf("REAL-WORLD EXAMPLE: Font Rendering\n");
    printf("========================================\n");

    printf("\nSenaryo: Embedded ekranda text render\n\n");

    const font_t* font = get_default_font();
    render_text("Hello World", font, 10, 20);

    printf("\nBellek analizi:\n");
    printf("  Font struct: %zu bytes → ROM'da\n", sizeof(font_t));
    printf("  Font bitmap: %zu bytes → ROM'da\n", sizeof(FONT_ARIAL_12_BITMAP));
    printf("  Font name: ~6 bytes → ROM'da\n");
    printf("  ═══════════════════════════════\n");
    printf("  Toplam RAM kullanımı: 0 bytes! ✓\n");
    printf("  (Const sayesinde hepsi ROM'da)\n");

    printf("\nConst olmadan:\n");
    printf("  Aynı data RAM'de olurdu\n");
    printf("  → ~1024 byte RAM kaybı\n");
    printf("  → 64KB RAM'li sistemde %1.6 kayıp!\n");

    printf("\n✓ LVGL binlerce font glyph const olarak saklar\n");
    printf("✓ Muazzam RAM tasarrufu sağlar\n");
}

/* ============================================================================
 * COMPARISON
 * ============================================================================ */

void compare_const_usage(void)
{
    printf("\n========================================\n");
    printf("KARŞILAŞTIRMA: Const vs Non-Const\n");
    printf("========================================\n");

    printf("┌──────────────────────────┬─────────────┬─────────────┐\n");
    printf("│ Özellik                  │ Non-Const   │ Const       │\n");
    printf("├──────────────────────────┼─────────────┼─────────────┤\n");
    printf("│ Compile-time Safety      │ ❌          │ ✅          │\n");
    printf("│ Compiler Optimization    │ ⭐⭐        │ ⭐⭐⭐⭐⭐   │\n");
    printf("│ API Intent               │ Belirsiz    │ Açık        │\n");
    printf("│ ROM Placement (embed)    │ ❌          │ ✅          │\n");
    printf("│ RAM Tasarrufu            │ ❌          │ ✅          │\n");
    printf("│ Thread Safety (help)     │ ❌          │ ✅          │\n");
    printf("│ Self-documenting         │ ❌          │ ✅          │\n");
    printf("│ Accidental Modification  │ Mümkün      │ Engellenir  │\n");
    printf("└──────────────────────────┴─────────────┴─────────────┘\n");

    printf("\nNon-const:\n");
    printf("  void process(widget_t* w)  // w değişir mi? Belirsiz\n");

    printf("\nConst:\n");
    printf("  void process(const widget_t* w)  // Kesinlikle değişmez\n");

    printf("\n✓ Const her zaman tercih edilmeli (readonly ise)\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║         CONST CORRECTNESS (CONST DOĞRULUĞU) DEMO          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    demonstrate_const_parameters();
    demonstrate_const_return_types();
    demonstrate_const_members();
    demonstrate_const_levels();
    demonstrate_optimizations();
    demonstrate_embedded_benefits();
    demonstrate_api_intent();
    demonstrate_compile_time_safety();
    demonstrate_real_world();
    compare_const_usage();

    printf("\n========================================\n");
    printf("ÖZET\n");
    printf("========================================\n");
    printf("Const Correctness:\n");
    printf("✓ Compile-time Safety - Hatalar derleme zamanında\n");
    printf("✓ Compiler Optimizations - Daha hızlı kod\n");
    printf("✓ API Intent - Fonksiyon ne yapar açık\n");
    printf("✓ ROM Placement - Embedded'de RAM tasarrufu\n");
    printf("✓ Self-documenting - Kod kendi kendini açıklar\n");
    printf("✓ Thread Safety - Immutable data güvenli\n");
    printf("\n");
    printf("LVGL'de const kullanımı:\n");
    printf("• Getter parameters: const lv_obj_t* obj\n");
    printf("• Return types: const lv_obj_class_t*\n");
    printf("• Struct members: const lv_obj_class_t* class_p\n");
    printf("• Font data: const lv_font_t font\n");
    printf("• Images: const lv_img_dsc_t img\n");
    printf("\n");
    printf("Best Practice:\n");
    printf("→ Getter'lar const param alsın\n");
    printf("→ Read-only return const olsun\n");
    printf("→ Lookup table'lar const olsun\n");
    printf("→ Değişmeyen struct member'lar const olsun\n");
    printf("\n");

    return 0;
}

/*
 * COMPILER WARNINGS TEST:
 *
 * Aşağıdaki kodları aktifleştirirseniz compiler uyarı/hata verir:
 *
 * void test_const_violations(void) {
 *     const widget_t w = {0, 0, 100, 50, 0};
 *
 *     // w.width = 200;  // error: assignment of member in read-only object
 *
 *     const widget_t* cw = &w;
 *     // widget_set_width(cw, 300);  // warning: discards 'const' qualifier
 *
 *     const char* name = "Test";
 *     // name[0] = 'X';  // warning: assignment of read-only location
 * }
 *
 * Bu uyarılar const correctness'in çalıştığını gösterir!
 */
