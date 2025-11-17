/**
 * @file 02_bit_packing_demo.c
 * @brief Bit Packing (Bit Paketleme) Stratejisi Demo
 *
 * Bu demo, küçük değerleri bit seviyesinde paketleyerek
 * bellek tasarrufu yapma stratejisini gösterir.
 *
 * KONSEPT:
 * - Boolean için 1 byte yerine 1 bit kullan
 * - Küçük enum'lar için gerektiği kadar bit kullan
 * - Birden fazla değeri tek bir integer'a sığdır
 *
 * COMPILE: gcc 02_bit_packing_demo.c -o bitpack_demo
 * RUN: ./bitpack_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ========================================================================
 * BIT PACKING OLMADAN (Baseline)
 * ======================================================================== */

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} text_align_t;  // 3 değer

typedef enum {
    STATE_IDLE,
    STATE_HOVERED,
    STATE_PRESSED,
    STATE_DISABLED
} widget_state_t;  // 4 değer

typedef struct {
    // Boolean flag'ler (her biri 1 byte!)
    uint8_t visible;        // 1 byte
    uint8_t enabled;        // 1 byte
    uint8_t clickable;      // 1 byte
    uint8_t draggable;      // 1 byte
    uint8_t focused;        // 1 byte
    uint8_t selected;       // 1 byte
    uint8_t dirty;          // 1 byte
    uint8_t layout_dirty;   // 1 byte

    // Enum'lar (her biri 4 byte!)
    text_align_t text_align;      // 4 byte (3 değer için)
    widget_state_t state;          // 4 byte (4 değer için)

    // Küçük sayılar
    uint8_t opacity;        // 1 byte (0-255)
    uint8_t layer;          // 1 byte (0-10 arası kullanılıyor)

} widget_unpacked;
// TOPLAM: 8×1 + 2×4 + 2×1 = 18 byte

/* ========================================================================
 * BIT PACKING İLE (Optimized)
 * ======================================================================== */

typedef struct {
    // Boolean flag'ler (8 flag = 1 byte!)
    uint8_t visible : 1;        // 1 bit
    uint8_t enabled : 1;        // 1 bit
    uint8_t clickable : 1;      // 1 bit
    uint8_t draggable : 1;      // 1 bit
    uint8_t focused : 1;        // 1 bit
    uint8_t selected : 1;       // 1 bit
    uint8_t dirty : 1;          // 1 bit
    uint8_t layout_dirty : 1;   // 1 bit
    // Toplam 8 bit = 1 byte

    // Enum'lar (gerektiği kadar bit)
    uint8_t text_align : 2;     // 2 bit (4 değere kadar)
    uint8_t state : 2;          // 2 bit (4 değere kadar)
    uint8_t reserved1 : 4;      // 4 bit (future use)
    // Toplam 8 bit = 1 byte

    // Küçük sayılar
    uint8_t opacity;            // 1 byte (0-255, tam kullanılıyor)
    uint8_t layer : 4;          // 4 bit (0-15, 0-10 kullanılıyor)
    uint8_t reserved2 : 4;      // 4 bit (future use)
    // 8 bit = 1 byte daha

} widget_packed;
// TOPLAM: 1 + 1 + 1 + 1 = 4 byte

/* ========================================================================
 * YARDIMCI FONKSİYONLAR
 * ======================================================================== */

const char* align_to_string(int align) {
    switch(align) {
        case ALIGN_LEFT: return "LEFT";
        case ALIGN_CENTER: return "CENTER";
        case ALIGN_RIGHT: return "RIGHT";
        default: return "UNKNOWN";
    }
}

const char* state_to_string(int state) {
    switch(state) {
        case STATE_IDLE: return "IDLE";
        case STATE_HOVERED: return "HOVERED";
        case STATE_PRESSED: return "PRESSED";
        case STATE_DISABLED: return "DISABLED";
        default: return "UNKNOWN";
    }
}

void print_unpacked_widget(widget_unpacked* w, int id) {
    printf("\n[UNPACKED Widget #%d]\n", id);
    printf("  Flags:      visible=%d, enabled=%d, clickable=%d\n",
           w->visible, w->enabled, w->clickable);
    printf("  Align:      %s\n", align_to_string(w->text_align));
    printf("  State:      %s\n", state_to_string(w->state));
    printf("  Opacity:    %d\n", w->opacity);
    printf("  Layer:      %d\n", w->layer);
    printf("  Size:       %zu bytes\n", sizeof(widget_unpacked));
}

void print_packed_widget(widget_packed* w, int id) {
    printf("\n[PACKED Widget #%d]\n", id);
    printf("  Flags:      visible=%d, enabled=%d, clickable=%d\n",
           w->visible, w->enabled, w->clickable);
    printf("  Align:      %s\n", align_to_string(w->text_align));
    printf("  State:      %s\n", state_to_string(w->state));
    printf("  Opacity:    %d\n", w->opacity);
    printf("  Layer:      %d\n", w->layer);
    printf("  Size:       %zu bytes\n", sizeof(widget_packed));
}

/* ========================================================================
 * BİT OPERASYON DEM

OLARI
 * ======================================================================== */

void demo_bitfield_basics() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 1: Bitfield Temelleri\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[UNPACKED - Normal Approach]\n");
    printf("  struct {\n");
    printf("      uint8_t visible;     // 1 byte\n");
    printf("      uint8_t enabled;     // 1 byte\n");
    printf("      uint8_t clickable;   // 1 byte\n");
    printf("      // ... 5 more booleans\n");
    printf("  };  // 8 bytes\n");

    printf("\n[PACKED - Bitfield Approach]\n");
    printf("  struct {\n");
    printf("      uint8_t visible : 1;     // 1 bit\n");
    printf("      uint8_t enabled : 1;     // 1 bit\n");
    printf("      uint8_t clickable : 1;   // 1 bit\n");
    printf("      // ... 5 more bits\n");
    printf("  };  // 1 byte (8 bits)\n");

    printf("\n[TASARRUF] 8 bytes → 1 byte = %%87.5 azalma!\n");
}

void demo_widget_comparison() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 2: Widget Karşılaştırması\n");
    printf("═══════════════════════════════════════════════════════════\n");

    // Unpacked widget
    widget_unpacked w_unpacked;
    w_unpacked.visible = 1;
    w_unpacked.enabled = 1;
    w_unpacked.clickable = 1;
    w_unpacked.draggable = 0;
    w_unpacked.focused = 0;
    w_unpacked.selected = 0;
    w_unpacked.dirty = 1;
    w_unpacked.layout_dirty = 0;
    w_unpacked.text_align = ALIGN_CENTER;
    w_unpacked.state = STATE_HOVERED;
    w_unpacked.opacity = 200;
    w_unpacked.layer = 5;

    print_unpacked_widget(&w_unpacked, 1);

    // Packed widget (aynı veriler)
    widget_packed w_packed;
    memset(&w_packed, 0, sizeof(w_packed));  // Sıfırla
    w_packed.visible = 1;
    w_packed.enabled = 1;
    w_packed.clickable = 1;
    w_packed.draggable = 0;
    w_packed.focused = 0;
    w_packed.selected = 0;
    w_packed.dirty = 1;
    w_packed.layout_dirty = 0;
    w_packed.text_align = ALIGN_CENTER;
    w_packed.state = STATE_HOVERED;
    w_packed.opacity = 200;
    w_packed.layer = 5;

    print_packed_widget(&w_packed, 1);

    printf("\n[KARŞILAŞTIRMA]\n");
    printf("  Unpacked: %zu bytes\n", sizeof(widget_unpacked));
    printf("  Packed:   %zu bytes\n", sizeof(widget_packed));
    printf("  Tasarruf: %zu bytes (%%%.1f)\n",
           sizeof(widget_unpacked) - sizeof(widget_packed),
           ((double)(sizeof(widget_unpacked) - sizeof(widget_packed)) /
            sizeof(widget_unpacked)) * 100);
}

void demo_array_of_widgets() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 3: 100 Widget Dizisi\n");
    printf("═══════════════════════════════════════════════════════════\n");

    const int COUNT = 100;

    printf("\n[UNPACKED] 100 widget:\n");
    printf("  %d × %zu bytes = %zu bytes\n",
           COUNT, sizeof(widget_unpacked), COUNT * sizeof(widget_unpacked));

    printf("\n[PACKED] 100 widget:\n");
    printf("  %d × %zu bytes = %zu bytes\n",
           COUNT, sizeof(widget_packed), COUNT * sizeof(widget_packed));

    size_t saved = COUNT * (sizeof(widget_unpacked) - sizeof(widget_packed));
    printf("\n[TASARRUF]\n");
    printf("  %zu bytes (%.1f KB)\n", saved, saved / 1024.0);
    printf("  %%%.1f azalma\n",
           ((double)(sizeof(widget_unpacked) - sizeof(widget_packed)) /
            sizeof(widget_unpacked)) * 100);
}

void demo_bitfield_operations() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 4: Bitfield Operasyonları\n");
    printf("═══════════════════════════════════════════════════════════\n");

    widget_packed w;
    memset(&w, 0, sizeof(w));

    printf("\n[OKUMA/YAZMA]\n");

    // Boolean flag operasyonları
    w.visible = 1;
    printf("  w.visible = 1  → OK (%d)\n", w.visible);

    w.enabled = 0;
    printf("  w.enabled = 0  → OK (%d)\n", w.enabled);

    // Enum operasyonları
    w.state = STATE_PRESSED;
    printf("  w.state = PRESSED  → OK (%s)\n", state_to_string(w.state));

    // Toggle işlemi
    w.visible = !w.visible;
    printf("  w.visible = !w.visible  → %d\n", w.visible);

    // Karşılaştırma
    if (w.state == STATE_PRESSED) {
        printf("  if (w.state == PRESSED)  → TRUE ✓\n");
    }

    printf("\n[OVERFLOW TESTİ]\n");

    // Layer: 4 bit = max 15
    w.layer = 10;
    printf("  w.layer = 10  → OK (%d)\n", w.layer);

    w.layer = 15;
    printf("  w.layer = 15  → OK (%d)\n", w.layer);

    w.layer = 20;  // Overflow! (20 mod 16 = 4)
    printf("  w.layer = 20  → OVERFLOW! (%d) ⚠️\n", w.layer);
    printf("  (4 bit sadece 0-15 aralığını tutar)\n");

    // Align: 2 bit = max 3
    w.text_align = 3;
    printf("\n  w.text_align = 3  → OK (%d)\n", w.text_align);

    w.text_align = 5;  // Overflow! (5 mod 4 = 1)
    printf("  w.text_align = 5  → OVERFLOW! (%d) ⚠️\n", w.text_align);
    printf("  (2 bit sadece 0-3 aralığını tutar)\n");
}

void demo_memory_layout() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DEMO 5: Bellek Yerleşimi (Memory Layout)\n");
    printf("═══════════════════════════════════════════════════════════\n");

    widget_packed w;
    memset(&w, 0, sizeof(w));

    w.visible = 1;
    w.enabled = 1;
    w.clickable = 0;
    w.draggable = 1;
    w.focused = 0;
    w.selected = 1;
    w.dirty = 0;
    w.layout_dirty = 1;

    printf("\n[BYTE 0 - Flag'ler]\n");
    printf("  Bit 0 (visible):      %d\n", w.visible);
    printf("  Bit 1 (enabled):      %d\n", w.enabled);
    printf("  Bit 2 (clickable):    %d\n", w.clickable);
    printf("  Bit 3 (draggable):    %d\n", w.draggable);
    printf("  Bit 4 (focused):      %d\n", w.focused);
    printf("  Bit 5 (selected):     %d\n", w.selected);
    printf("  Bit 6 (dirty):        %d\n", w.dirty);
    printf("  Bit 7 (layout_dirty): %d\n", w.layout_dirty);
    printf("  Binary: 1010 0101\n");

    w.text_align = ALIGN_CENTER;  // 01
    w.state = STATE_PRESSED;       // 10

    printf("\n[BYTE 1 - Enum'lar]\n");
    printf("  Bit 0-1 (text_align): %d (%s)\n",
           w.text_align, align_to_string(w.text_align));
    printf("  Bit 2-3 (state):      %d (%s)\n",
           w.state, state_to_string(w.state));
    printf("  Bit 4-7 (reserved):   0\n");
    printf("  Binary: 0000 1001\n");

    w.opacity = 255;

    printf("\n[BYTE 2 - Opacity]\n");
    printf("  Opacity: %d\n", w.opacity);
    printf("  Binary: 1111 1111\n");

    w.layer = 5;

    printf("\n[BYTE 3 - Layer]\n");
    printf("  Bit 0-3 (layer):    %d\n", w.layer);
    printf("  Bit 4-7 (reserved): 0\n");
    printf("  Binary: 0000 0101\n");

    printf("\n[TOPLAM: 4 BYTE]\n");
}

/* ========================================================================
 * BIT MASKELEME MANÜELİ İMPLEMENTASYON (COMPARISON)
 * ======================================================================== */

void demo_manual_bitpacking() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  BONUS: Manuel Bit Maskeleme\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n[C Bitfield vs Manuel Bit Manipulation]\n");
    printf("\nC Bitfield (Compiler yapar):\n");
    printf("  struct {\n");
    printf("      uint8_t visible : 1;\n");
    printf("      uint8_t enabled : 1;\n");
    printf("  };\n");
    printf("  w.visible = 1;  // Kolay!\n");

    printf("\nManuel (Kendin yaparsın):\n");
    printf("  uint8_t flags;\n");
    printf("  #define FLAG_VISIBLE 0x01  // 0000 0001\n");
    printf("  #define FLAG_ENABLED 0x02  // 0000 0010\n");
    printf("  \n");
    printf("  // Set bit\n");
    printf("  flags |= FLAG_VISIBLE;   // OR ile ekle\n");
    printf("  \n");
    printf("  // Clear bit\n");
    printf("  flags &= ~FLAG_VISIBLE;  // AND NOT ile sil\n");
    printf("  \n");
    printf("  // Check bit\n");
    printf("  if (flags & FLAG_VISIBLE) { ... }\n");

    // Örnek
    uint8_t flags = 0;

    printf("\n[MANUAL BIT OPERATIONS]\n");
    printf("  flags başlangıç: 0x%02X (binary: ", flags);
    for (int i = 7; i >= 0; i--) {
        printf("%d", (flags >> i) & 1);
        if (i == 4) printf(" ");
    }
    printf(")\n");

    // Set visible
    #define FLAG_VISIBLE 0x01
    flags |= FLAG_VISIBLE;
    printf("  flags |= VISIBLE: 0x%02X (binary: ", flags);
    for (int i = 7; i >= 0; i--) {
        printf("%d", (flags >> i) & 1);
        if (i == 4) printf(" ");
    }
    printf(")\n");

    // Set enabled
    #define FLAG_ENABLED 0x02
    flags |= FLAG_ENABLED;
    printf("  flags |= ENABLED: 0x%02X (binary: ", flags);
    for (int i = 7; i >= 0; i--) {
        printf("%d", (flags >> i) & 1);
        if (i == 4) printf(" ");
    }
    printf(")\n");

    // Clear visible
    flags &= ~FLAG_VISIBLE;
    printf("  flags &= ~VISIBLE: 0x%02X (binary: ", flags);
    for (int i = 7; i >= 0; i--) {
        printf("%d", (flags >> i) & 1);
        if (i == 4) printf(" ");
    }
    printf(")\n");

    printf("\n[SONUÇ]\n");
    printf("  ✓ C bitfield daha okunabilir\n");
    printf("  ✓ Manuel daha kontrollü (endianness, alignment)\n");
    printf("  ✓ Her ikisi de aynı bellek verimliliği\n");
}

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║          BIT PACKING (BIT PAKETLEME) DEMO                ║\n");
    printf("║                                                           ║\n");
    printf("║  Konsept: Küçük değerleri bit seviyesinde paketle        ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    demo_bitfield_basics();
    demo_widget_comparison();
    demo_array_of_widgets();
    demo_bitfield_operations();
    demo_memory_layout();
    demo_manual_bitpacking();

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  SONUÇ:                                                   ║\n");
    printf("║                                                           ║\n");
    printf("║  ✓ Boolean'lar: 8 byte → 1 byte (%%87.5 tasarruf)         ║\n");
    printf("║  ✓ Küçük enum'lar: 4 byte → 2 bit                        ║\n");
    printf("║  ✓ Widget örneği: 18 byte → 4 byte (%%77 tasarruf)        ║\n");
    printf("║  ✓ Cache efficiency: Küçük struct = daha hızlı           ║\n");
    printf("║                                                           ║\n");
    printf("║  ⚠️  Dikkat:                                               ║\n");
    printf("║  - Overflow riski (değer aralığı sınırlı)                ║\n");
    printf("║  - Pointer alamazsın (&w.visible HATA!)                  ║\n");
    printf("║  - Thread-safety sorunları olabilir                      ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
