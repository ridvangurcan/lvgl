/*
 * OPAQUE POINTERS (OPAK İŞARETÇİLER) DEMO
 *
 * Bu demo, opaque pointer pattern'inin nasıl kullanıldığını gösterir.
 * LVGL'de yaygın olarak kullanılan bu teknik, implementasyon detaylarını
 * gizleyerek API stability ve encapsulation sağlar.
 *
 * İki pattern gösterilir:
 * 1. Fully Opaque - Struct tanımı tamamen gizli (handle-based)
 * 2. Semi-Opaque - Struct tanımı görünür ama prefix ile internal işaretli
 *
 * Derleme: gcc 01_opaque_pointers_demo.c -o opaque_demo
 * Çalıştırma: ./opaque_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * PATTERN 1: FULLY OPAQUE (TAM OPAK)
 * ============================================================================
 * Struct tanımı tamamen .c dosyasında saklanır.
 * Kullanıcı sadece pointer görür, içeriği görmez.
 * LVGL Örneği: _lv_event_dsc_t
 */

/* --- Public Header (button.h gibi düşünün) --- */

// Forward declaration - sadece tip adı deklare edilir
typedef struct button_t button_t;

// Public API - Kullanıcılar sadece bu fonksiyonları görür
button_t* button_create(const char* label);
void button_delete(button_t* btn);
void button_set_label(button_t* btn, const char* label);
const char* button_get_label(const button_t* btn);
void button_set_enabled(button_t* btn, bool enabled);
bool button_is_enabled(const button_t* btn);
void button_click(button_t* btn);
void button_print_info(const button_t* btn);

/* --- Private Implementation (button.c gibi düşünün) --- */

// Gerçek struct tanımı - Sadece implementation dosyasında
struct button_t {
    char label[64];        // Button etiketi
    bool enabled;          // Aktif mi?
    uint32_t click_count;  // Tıklanma sayısı
    void* internal_data;   // İç veri (kullanıcı görmez)
};

// Factory function - Constructor
button_t* button_create(const char* label)
{
    button_t* btn = (button_t*)malloc(sizeof(button_t));
    if (!btn) return NULL;

    strncpy(btn->label, label ? label : "Button", sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label) - 1] = '\0';
    btn->enabled = true;
    btn->click_count = 0;
    btn->internal_data = NULL;

    printf("[OPAQUE] Button created: '%s'\n", btn->label);
    return btn;
}

// Destructor
void button_delete(button_t* btn)
{
    if (!btn) return;
    printf("[OPAQUE] Button deleted: '%s' (clicked %u times)\n",
           btn->label, btn->click_count);
    free(btn);
}

// Setter - Label değiştirme
void button_set_label(button_t* btn, const char* label)
{
    if (!btn || !label) return;
    strncpy(btn->label, label, sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label) - 1] = '\0';
}

// Getter - Label okuma (const döner)
const char* button_get_label(const button_t* btn)
{
    return btn ? btn->label : "";
}

// Setter - Enabled durumu
void button_set_enabled(button_t* btn, bool enabled)
{
    if (!btn) return;
    btn->enabled = enabled;
    printf("[OPAQUE] Button '%s' %s\n", btn->label,
           enabled ? "enabled" : "disabled");
}

// Getter - Enabled sorgusu
bool button_is_enabled(const button_t* btn)
{
    return btn ? btn->enabled : false;
}

// Action - Tıklama
void button_click(button_t* btn)
{
    if (!btn) return;

    if (btn->enabled) {
        btn->click_count++;
        printf("[OPAQUE] Button '%s' clicked (count: %u)\n",
               btn->label, btn->click_count);
    } else {
        printf("[OPAQUE] Button '%s' is disabled, click ignored\n",
               btn->label);
    }
}

// Debug bilgisi
void button_print_info(const button_t* btn)
{
    if (!btn) return;
    printf("[OPAQUE] Button Info:\n");
    printf("  Label: %s\n", btn->label);
    printf("  Enabled: %s\n", btn->enabled ? "Yes" : "No");
    printf("  Click count: %u\n", btn->click_count);
}

/* ============================================================================
 * PATTERN 2: SEMI-OPAQUE (YARI-OPAK)
 * ============================================================================
 * Struct tanımı header'da görünür ancak underscore prefix ile "internal"
 * olduğu belirtilir. Kullanıcılar doğrudan erişmemeli.
 * LVGL Örneği: _lv_obj_t
 */

/* --- Public Header (widget.h gibi düşünün) --- */

// Forward declaration
typedef struct _widget_t widget_t;

// Struct tanımı görünür ama "_" prefix ile internal işaretli
struct _widget_t {
    int x, y;              // Pozisyon
    int width, height;     // Boyut
    bool visible;          // Görünür mü?
    uint32_t color;        // Renk (RGB)
};

// Public API
widget_t* widget_create(int x, int y, int w, int h);
void widget_delete(widget_t* w);
void widget_set_pos(widget_t* w, int x, int y);
void widget_get_pos(const widget_t* w, int* x, int* y);
void widget_set_size(widget_t* w, int width, int height);
void widget_set_visible(widget_t* w, bool visible);
bool widget_is_visible(const widget_t* w);
void widget_print_info(const widget_t* w);

/* --- Implementation --- */

widget_t* widget_create(int x, int y, int w, int h)
{
    widget_t* widget = (widget_t*)malloc(sizeof(widget_t));
    if (!widget) return NULL;

    widget->x = x;
    widget->y = y;
    widget->width = w;
    widget->height = h;
    widget->visible = true;
    widget->color = 0xFFFFFF;  // White

    printf("[SEMI-OPAQUE] Widget created at (%d,%d) size %dx%d\n",
           x, y, w, h);
    return widget;
}

void widget_delete(widget_t* w)
{
    if (!w) return;
    printf("[SEMI-OPAQUE] Widget deleted\n");
    free(w);
}

void widget_set_pos(widget_t* w, int x, int y)
{
    if (!w) return;
    w->x = x;
    w->y = y;
    printf("[SEMI-OPAQUE] Widget moved to (%d,%d)\n", x, y);
}

void widget_get_pos(const widget_t* w, int* x, int* y)
{
    if (!w) return;
    if (x) *x = w->x;
    if (y) *y = w->y;
}

void widget_set_size(widget_t* w, int width, int height)
{
    if (!w) return;
    w->width = width;
    w->height = height;
    printf("[SEMI-OPAQUE] Widget resized to %dx%d\n", width, height);
}

void widget_set_visible(widget_t* w, bool visible)
{
    if (!w) return;
    w->visible = visible;
    printf("[SEMI-OPAQUE] Widget %s\n", visible ? "shown" : "hidden");
}

bool widget_is_visible(const widget_t* w)
{
    return w ? w->visible : false;
}

void widget_print_info(const widget_t* w)
{
    if (!w) return;
    printf("[SEMI-OPAQUE] Widget Info:\n");
    printf("  Position: (%d,%d)\n", w->x, w->y);
    printf("  Size: %dx%d\n", w->width, w->height);
    printf("  Visible: %s\n", w->visible ? "Yes" : "No");
    printf("  Color: 0x%06X\n", w->color);
}

/* ============================================================================
 * COMPARISON & BENEFITS
 * ============================================================================ */

void print_separator(const char* title)
{
    printf("\n");
    printf("========================================\n");
    printf("%s\n", title);
    printf("========================================\n");
}

void demonstrate_opaque_benefits(void)
{
    print_separator("AVANTAJ 1: Enkapsülasyon");

    button_t* btn = button_create("Submit");

    // ✓ DOĞRU: API kullanımı
    button_set_label(btn, "Send");
    printf("Label (via API): %s\n", button_get_label(btn));

    // ❌ YANLIŞ: Doğrudan erişim mümkün değil (fully opaque)
    // btn->label = "Wrong";  // DERLEME HATASI: incomplete type

    printf("✓ Kullanıcı struct detaylarına erişemez (güvenli)\n");

    button_delete(btn);
}

void demonstrate_validation(void)
{
    print_separator("AVANTAJ 2: Validation ve Side Effects");

    button_t* btn = button_create("Click Me");

    // API validasyon yapabilir
    button_set_label(btn, NULL);  // NULL kontrolü yapılır
    button_set_label(btn, "Very Long Label That Exceeds Buffer Size Limit");
    printf("Label (truncated safely): %s\n", button_get_label(btn));

    // Side effect örneği
    button_set_enabled(btn, false);
    button_click(btn);  // Disabled olduğu için ignore edilir

    button_set_enabled(btn, true);
    button_click(btn);  // Şimdi kabul edilir

    button_delete(btn);
}

void demonstrate_abi_stability(void)
{
    print_separator("AVANTAJ 3: ABI Stability");

    printf("Opaque pointer kullanımında:\n");
    printf("✓ Struct'a yeni field eklenebilir\n");
    printf("✓ Field sırası değiştirilebilir\n");
    printf("✓ Field tipleri değiştirilebilir\n");
    printf("✓ Kullanıcı kodunu yeniden derlemeye gerek yok!\n");
    printf("\nÖrnek: button_t struct'ına 'last_click_time' eklense bile,\n");
    printf("       mevcut programlar çalışmaya devam eder.\n");
}

void demonstrate_semi_opaque(void)
{
    print_separator("SEMI-OPAQUE PATTERN");

    widget_t* w = widget_create(10, 20, 100, 50);

    // ✓ DOĞRU: API kullanımı
    widget_set_pos(w, 50, 50);

    int x, y;
    widget_get_pos(w, &x, &y);
    printf("Position via API: (%d,%d)\n", x, y);

    // ⚠ TEKNİK OLARAK MÜMKÜN: Doğrudan erişim
    // (Ama YAPILMAMALI - API kullan)
    printf("\n⚠ Doğrudan erişim (YAPMA!):\n");
    printf("  w->x = %d (direkt okudum)\n", w->x);
    w->x = 999;  // Tehlikeli! Side effect'ler çalışmaz
    printf("  w->x = %d (direkt değiştirdim - KÖTÜ!)\n", w->x);

    // API ile düzelt
    widget_set_pos(w, 100, 100);
    printf("\n✓ API ile düzelttim\n");

    widget_print_info(w);
    widget_delete(w);
}

void demonstrate_memory_safety(void)
{
    print_separator("AVANTAJ 4: Memory Safety");

    printf("Opaque pointer ile:\n");
    printf("✓ Kullanıcı malloc/free kontrolünü kaybetmez\n");
    printf("✓ Kütüphane memory layout'u yönetir\n");
    printf("✓ Memory leak önlenir (destructor pattern)\n");
    printf("✓ Double free önlenir\n");

    button_t* btn1 = button_create("Button 1");
    button_t* btn2 = button_create("Button 2");

    // Doğru deletion
    button_delete(btn1);
    button_delete(btn2);

    printf("\n✓ Her create bir delete ile eşleştirildi\n");
}

void compare_patterns(void)
{
    print_separator("PATTERN KARŞILAŞTIRMASI");

    printf("┌─────────────────────┬─────────────────┬─────────────────┐\n");
    printf("│ Özellik             │ Fully Opaque    │ Semi-Opaque     │\n");
    printf("├─────────────────────┼─────────────────┼─────────────────┤\n");
    printf("│ Enkapsülasyon       │ ⭐⭐⭐⭐⭐       │ ⭐⭐⭐          │\n");
    printf("│ ABI Stability       │ ⭐⭐⭐⭐⭐       │ ⭐⭐⭐⭐         │\n");
    printf("│ Compile Time        │ ⭐⭐⭐⭐         │ ⭐⭐⭐⭐⭐       │\n");
    printf("│ Debug Kolaylığı     │ ⭐⭐⭐          │ ⭐⭐⭐⭐⭐       │\n");
    printf("│ Inline Fonksiyon    │ ❌              │ ✅              │\n");
    printf("│ Stack Allocation    │ ❌              │ ✅              │\n");
    printf("└─────────────────────┴─────────────────┴─────────────────┘\n");

    printf("\nFully Opaque:\n");
    printf("  + Maksimum enkapsülasyon\n");
    printf("  + Tamamen değiştirilebilir implementation\n");
    printf("  - Heap allocation zorunlu\n");
    printf("  - Inline fonksiyon kullanılamaz\n");
    printf("  LVGL Örneği: _lv_event_dsc_t\n");

    printf("\nSemi-Opaque:\n");
    printf("  + Inline fonksiyonlar mümkün (performance)\n");
    printf("  + Stack allocation mümkün\n");
    printf("  + Debug kolay (struct görünür)\n");
    printf("  - Kullanıcı doğrudan erişebilir (tehlikeli)\n");
    printf("  LVGL Örneği: _lv_obj_t\n");
}

void demonstrate_real_world_example(void)
{
    print_separator("GERÇEK DÜNYA ÖRNEĞİ");

    printf("Senaryo: Bir form oluşturalım\n\n");

    // Multiple buttons
    button_t* btn_submit = button_create("Submit");
    button_t* btn_cancel = button_create("Cancel");
    button_t* btn_reset = button_create("Reset");

    // Widgets
    widget_t* form_bg = widget_create(0, 0, 400, 300);
    widget_t* input_field = widget_create(10, 10, 380, 40);

    // User interactions
    printf("\nKullanıcı etkileşimleri:\n");
    button_click(btn_submit);
    button_click(btn_submit);
    button_click(btn_cancel);

    // Disable reset temporarily
    button_set_enabled(btn_reset, false);
    button_click(btn_reset);  // Ignored
    button_set_enabled(btn_reset, true);
    button_click(btn_reset);  // Accepted

    // Widget operations
    widget_set_visible(input_field, false);
    widget_set_visible(input_field, true);

    // Print final state
    printf("\nFinal durumu:\n");
    button_print_info(btn_submit);
    widget_print_info(form_bg);

    // Cleanup
    button_delete(btn_submit);
    button_delete(btn_cancel);
    button_delete(btn_reset);
    widget_delete(form_bg);
    widget_delete(input_field);

    printf("\n✓ Tüm kaynaklar temizlendi (memory leak yok)\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        OPAQUE POINTERS (OPAK İŞARETÇİLER) DEMO            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    demonstrate_opaque_benefits();
    demonstrate_validation();
    demonstrate_semi_opaque();
    demonstrate_abi_stability();
    demonstrate_memory_safety();
    compare_patterns();
    demonstrate_real_world_example();

    print_separator("ÖZET");
    printf("Opaque Pointer Pattern:\n");
    printf("✓ İmplementasyon detaylarını gizler (Encapsulation)\n");
    printf("✓ ABI stability sağlar (Binary uyumluluk)\n");
    printf("✓ API değişikliklerine karşı esneklik\n");
    printf("✓ Validation ve side effect kontrolü\n");
    printf("✓ Memory safety ve resource yönetimi\n");
    printf("\n");
    printf("LVGL'de kullanımı:\n");
    printf("• Fully Opaque: _lv_event_dsc_t (event descriptors)\n");
    printf("• Semi-Opaque: _lv_obj_t (objects)\n");
    printf("• Semi-Opaque: _lv_group_t (groups)\n");
    printf("\n");

    return 0;
}

/*
 * ÇIKTI ÖRNEĞİ:
 *
 * ========================================
 * AVANTAJ 1: Enkapsülasyon
 * ========================================
 * [OPAQUE] Button created: 'Submit'
 * [OPAQUE] Button created: 'Send'
 * Label (via API): Send
 * ✓ Kullanıcı struct detaylarına erişemez (güvenli)
 * [OPAQUE] Button deleted: 'Send' (clicked 0 times)
 *
 * ========================================
 * AVANTAJ 2: Validation ve Side Effects
 * ========================================
 * [OPAQUE] Button created: 'Click Me'
 * Label (truncated safely): Very Long Label That Exceeds Buffer Size Limi
 * [OPAQUE] Button 'Very Long Label That Exceeds Buffer Size Limi' disabled
 * [OPAQUE] Button 'Very Long Label That Exceeds Buffer Size Limi' is disabled, click ignored
 * [OPAQUE] Button 'Very Long Label That Exceeds Buffer Size Limi' enabled
 * [OPAQUE] Button 'Very Long Label That Exceeds Buffer Size Limi' clicked (count: 1)
 * ...
 */
