# LVGL Style ve Theme System Architecture Demo

## 📋 Genel Bakış

Bu demo, **LVGL'nin style ve theme sistem mimarisini** tamamen bağımsız, saf C kodu ile gösterir. LVGL kütüphanesine bağımlılığı yoktur ve standalone olarak derlenip çalıştırılabilir.

### Amaç

LVGL'nin karmaşık style ve theme mekanizmasını adım adım öğretmek için tasarlanmıştır. Gerçek grafik çizimi yapmaz, bunun yerine **printf ile her adımı detaylı açıklar**.

## 🎯 Kapsanan Konular

### 1. **Style Properties (lv_style_t)**
- Property types (color, opacity, size, etc.)
- Property value union
- Property get/set operations
- Property storage structure

### 2. **Selector System**
- Part system (MAIN, INDICATOR, KNOB, etc.)
- State system (DEFAULT, PRESSED, FOCUSED, DISABLED)
- Selector matching (Part + State bitmask)
- Wildcard selectors (ANY)

### 3. **Style Cascading**
- Multiple style sources
- Priority order (INLINE > LOCAL > THEME > INHERITED > DEFAULT)
- Style override mechanism
- Style merging

### 4. **Style Inheritance**
- Inheritable properties (text properties)
- Non-inheritable properties (background, border)
- Parent-to-child propagation
- Override behavior

### 5. **Theme Provider**
- Theme structure
- Automatic style assignment
- Widget-type specific styling
- Theme-based UI consistency

### 6. **Transition System**
- Property animation on state change
- Transition duration
- Linear interpolation
- Smooth visual feedback

## 🏗️ Mimari Bileşenler

```
┌──────────────────────────────────────────────────────────────┐
│  Style System Architecture                                   │
│                                                               │
│  ┌─────────────┐                                             │
│  │  style_t    │  Property storage                           │
│  ├─────────────┤                                             │
│  │ - props[]   │  Array of property-value pairs              │
│  │ - count     │                                             │
│  └─────────────┘                                             │
│         │                                                     │
│         │ referenced by                                      │
│         ↓                                                     │
│  ┌──────────────────┐                                        │
│  │ style_descriptor │  Style + Selector                      │
│  ├──────────────────┤                                        │
│  │ - style*         │  Pointer to style                      │
│  │ - selector       │  Part | State (32-bit bitmask)         │
│  └──────────────────┘                                        │
│         │                                                     │
│         │ attached to                                        │
│         ↓                                                     │
│  ┌──────────────────┐                                        │
│  │    widget_t      │  Widget with multiple styles          │
│  ├──────────────────┤                                        │
│  │ - state          │  Current state (PRESSED, FOCUSED,...)  │
│  │ - local_styles[] │  Array of style descriptors            │
│  │ - inline_style   │  Highest priority style                │
│  │ - parent*        │  For inheritance                       │
│  └──────────────────┘                                        │
│         │                                                     │
│         │ styles applied by                                  │
│         ↓                                                     │
│  ┌──────────────────┐                                        │
│  │     theme_t      │  Theme provider                        │
│  ├──────────────────┤                                        │
│  │ - default_style  │  All widgets                           │
│  │ - button_style   │  Button specific                       │
│  │ - label_style    │  Label specific                        │
│  │ - ...            │                                         │
│  └──────────────────┘                                        │
└──────────────────────────────────────────────────────────────┘
```

## 🔧 Derleme ve Çalıştırma

### Gereksinimler
- GCC veya herhangi bir C derleyici
- Standart C kütüphanesi (stdio, stdlib, string, stdbool, stdint)

### Derleme

```bash
cd design_and_documents/demos/style_system
gcc lvgl_style_system_demo.c -o demo
```

### Çalıştırma

```bash
./demo
```

### Çıktıyı Dosyaya Kaydetme

```bash
./demo > output.txt
```

## 📊 Örnek Çıktı

Demo çalıştırıldığında şöyle bir çıktı üretir:

```
╔════════════════════════════════════════════════════════════════╗
║         LVGL Style ve Theme System Architecture Demo          ║
║                                                                ║
║  Bu demo LVGL'nin style sistem mimarisini gösterir:           ║
║  1. Style Properties (lv_style_t)                             ║
║  2. Selector System (Part + State)                            ║
║  3. Style Cascading (Priority)                                ║
║  4. Style Inheritance                                         ║
║  5. Theme Provider                                            ║
║  6. Transition System                                         ║
╚════════════════════════════════════════════════════════════════╝

════════════════════════════════════════════════════════════════
  SCENARIO 3: STYLE CASCADING (Priority)
════════════════════════════════════════════════════════════════

>> Style resolution (priority order):
  Priority: INLINE > LOCAL > THEME > DEFAULT

  bg_color: RGB(255,0,0) - Source: INLINE ✓
  text_color: RGB(0,0,0) - Source: LOCAL ✓
  pad_top: 10 - Source: LOCAL ✓
  border_width: 2 - Source: LOCAL ✓
```

## 🎓 Öğrenme Yolu

### 1. İlk Okuma
Kodu yukarıdan aşağıya okuyun. Her bölüm açıklamalı:
- Basic types (color_t, opacity_t)
- Style properties enum
- Style value union
- State and part enums
- Selector system
- Style structure
- Widget with style support
- Cascading resolver
- Inheritance mechanism
- Theme provider
- Transition system
- Demo scenarios

### 2. Kodu Çalıştırma
Demo'yu çalıştırın ve çıktıyı inceleyin. 7 farklı scenario göreceksiniz:
1. **Basic Styles**: Style oluşturma ve property set/get
2. **Selectors**: Part + State matching
3. **Cascading**: Priority-based style resolution
4. **Inheritance**: Parent → Child property propagation
5. **Theme**: Automatic style assignment
6. **Transition**: State change animation
7. **Complete Example**: Gerçekçi UI örneği

### 3. Değişiklikler Yapma

**Yeni property ekleyin:**
```c
typedef enum {
    // ...existing props...
    STYLE_PROP_OUTLINE_WIDTH,
    STYLE_PROP_OUTLINE_COLOR,
    STYLE_PROP_OUTLINE_PAD,
    STYLE_PROP_COUNT
} style_prop_t;

void style_set_outline_width(style_t* style, int32_t width)
{
    style_set_prop(style, STYLE_PROP_OUTLINE_WIDTH,
                   (style_value_t){.num = width});
}
```

**Yeni state ekleyin:**
```c
typedef enum {
    // ...existing states...
    STATE_SCROLLED  = 0x0040,
    STATE_FOCUSED_IN = 0x0080,
    STATE_ANY       = 0xFFFF,
} widget_state_t;
```

**Custom theme oluşturun:**
```c
void create_light_theme(theme_t* theme)
{
    theme_init(theme, "LightTheme");

    // Default: white background, black text
    style_set_bg_color(&theme->default_style, COLOR_WHITE);
    style_set_text_color(&theme->default_style, COLOR_BLACK);

    // Button: green background
    style_set_bg_color(&theme->button_style, COLOR_GREEN);
    style_set_radius(&theme->button_style, 10);

    // Button pressed: darker green
    style_set_bg_color(&theme->button_pressed_style,
                      (color_t){0, 180, 0});
}
```

### 4. LVGL Karşılaştırması

| Demo Fonksiyon | LVGL Fonksiyon | Dosya |
|----------------|----------------|-------|
| `style_init()` | `lv_style_init()` | `src/misc/lv_style.c` |
| `style_set_prop()` | `lv_style_set_prop()` | `src/misc/lv_style.c` |
| `style_get_prop()` | `lv_style_get_prop()` | `src/misc/lv_style.c` |
| `widget_add_style()` | `lv_obj_add_style()` | `src/core/lv_obj_style.c` |
| `widget_get_style_prop()` | `lv_obj_get_style_prop()` | `src/core/lv_obj_style.c` |
| `selector_matches()` | Internal matching logic | `src/core/lv_obj_style.c` |
| `theme_apply()` | `lv_theme_apply()` | `src/core/lv_theme.c` |
| `style_t` | `lv_style_t` | `src/misc/lv_style.h` |
| `style_prop_t` | `lv_style_prop_t` | `src/misc/lv_style.h` |

## 🧩 Design Pattern'lar

### 1. Strategy Pattern (Style Resolution)

```c
// Strategy: Style resolver farklı kaynaklardan property alır
static style_res_t widget_get_style_prop_impl(widget_t* widget,
                                               style_prop_t prop,
                                               widget_part_t part,
                                               bool allow_inherit)
{
    // Strategy 1: Try inline style
    if(style_get_prop(&widget->inline_style, prop, &res.value)) {
        res.source = STYLE_RES_INLINE;
        return res;
    }

    // Strategy 2: Try local styles
    for(each local_style) {
        if(selector_matches(...)) {
            res.source = STYLE_RES_LOCAL;
        }
    }

    // Strategy 3: Try inheritance
    if(allow_inherit && INHERITABLE[prop]) {
        res = widget_get_style_prop(widget->parent, prop, PART_MAIN);
        res.source = STYLE_RES_INHERITED;
    }

    // Strategy 4: Use default
    res.source = STYLE_RES_DEFAULT;
    return res;
}
```

**LVGL'de aynı pattern**: `src/core/lv_obj_style.c:545-685`

### 2. Flyweight Pattern (Property Storage)

```c
// Flyweight: Property-value pairs intrinsic data olarak tutulur
// Birden fazla widget aynı style objesini paylaşabilir

style_t shared_button_style;  // FLYWEIGHT: shared across widgets
style_init(&shared_button_style);
style_set_bg_color(&shared_button_style, COLOR_BLUE);

// Widget 1 ve 2 aynı style'ı paylaşır (memory efficient)
widget_add_style(button1, &shared_button_style, SELECTOR_MAIN);
widget_add_style(button2, &shared_button_style, SELECTOR_MAIN);

// Her widget kendi inline_style'ına sahip (extrinsic data)
widget_set_style_bg_color(button1, COLOR_RED);  // Only button1 changes
```

**LVGL'de aynı mantık**: Theme style'ları tüm widget'lar arasında paylaşılır

### 3. Composite Pattern (Style Layering)

```c
// Composite: Birden fazla style layer'ı bir araya gelir
typedef struct {
    style_t* style;
    style_selector_t selector;
} style_descriptor_t;  // Leaf

typedef struct widget_t {
    style_descriptor_t local_styles[WIDGET_MAX_STYLES];  // Composite
    style_t inline_style;                                 // Leaf
} widget_t;

// Resolution: Tüm layer'ları birleştir (compose)
style_res_t widget_get_style_prop(widget_t* widget, style_prop_t prop, ...)
{
    // Layer 1: inline
    // Layer 2: local styles
    // Layer 3: inherited
    // Layer 4: default
    // Composite result
}
```

### 4. Observer Pattern (State Change → Transition)

```c
// Subject: widget state
// Observer: transition system

void widget_add_state(widget_t* widget, widget_state_t state)
{
    widget->state |= state;  // State change

    // NOTIFY OBSERVERS
    // In real LVGL, this triggers transition animations
    transition_simulate(widget, STYLE_PROP_BG_OPA, PART_MAIN);
}
```

### 5. Template Method Pattern (Style Apply)

```c
// Template method: Theme apply için genel akış
void theme_apply(theme_t* theme, widget_t* widget, const char* type)
{
    // STEP 1: Apply default (always) - template
    widget_add_style(widget, &theme->default_style, SELECTOR_MAIN);

    // STEP 2: Apply widget-specific (hook - varies by type)
    if(type == "button") {
        widget_add_style(widget, &theme->button_style, SELECTOR_MAIN);
        widget_add_style(widget, &theme->button_pressed_style,
                        SELECTOR_MAIN_PRESSED);
    }
    else if(type == "label") {
        widget_add_style(widget, &theme->label_style, SELECTOR_MAIN);
    }
    // HOOK: type-specific logic
}
```

## 🔍 Detaylı Kod Analizi

### Property Storage: Dynamic Array vs Fixed Array

**LVGL** (dynamic):
```c
// src/misc/lv_style.h
typedef struct {
    lv_style_value_t * values_and_props;  // Dynamic array
    uint16_t prop_cnt;
    uint8_t has_group;
} lv_style_t;

// Properties eklendikçe realloc ile büyür
```

**Demo** (fixed - basitleştirme):
```c
#define STYLE_MAX_PROPS 32

typedef struct {
    style_prop_entry_t props[STYLE_MAX_PROPS];  // Fixed array
    uint32_t prop_count;
} style_t;
```

**Trade-off**: LVGL memory efficient, Demo implementation simple.

### Selector Matching Logic

```c
// 32-bit selector: [16-bit part | 16-bit state]
typedef uint32_t style_selector_t;

#define SELECTOR(part, state) ((style_selector_t)((part) | (state)))

// Examples:
SELECTOR(PART_MAIN, STATE_DEFAULT)   = 0x00000000
SELECTOR(PART_MAIN, STATE_PRESSED)   = 0x00000004
SELECTOR(PART_INDICATOR, STATE_DEFAULT) = 0x00020000

// Matching:
static bool selector_matches(style_selector_t selector,
                             widget_part_t part,
                             widget_state_t state)
{
    uint32_t selector_part = selector & 0xFFFF0000;   // Upper 16 bits
    uint32_t selector_state = selector & 0x0000FFFF;  // Lower 16 bits

    // Part exact match OR wildcard
    bool part_match = (selector_part == PART_ANY) ||
                      (selector_part == part);

    // State bitmask match
    // STATE_PRESSED (0x04) matches selector with PRESSED bit set
    bool state_match = (selector_state == STATE_ANY) ||
                       (selector_state == STATE_DEFAULT &&
                        state == STATE_DEFAULT) ||
                       ((selector_state & state) != 0);  // Bitmask test

    return part_match && state_match;
}
```

**Örnek**: Widget state = `STATE_PRESSED | STATE_FOCUSED` (0x0006)
- Selector `STATE_PRESSED` (0x0004) → **MATCH** (0x0006 & 0x0004 = 0x0004 ≠ 0)
- Selector `STATE_DEFAULT` (0x0000) → **NO MATCH**
- Selector `STATE_DISABLED` (0x0008) → **NO MATCH** (0x0006 & 0x0008 = 0)

**LVGL'de aynı logic**: `src/core/lv_obj_style.c:_lv_obj_style_get_prop()`

### Cascading Priority Order

```c
// Priority hierarchy (highest to lowest):
// 1. INLINE     - widget->inline_style
// 2. LOCAL      - widget->local_styles[] (selector-matched)
// 3. INHERITED  - widget->parent (for inheritable props)
// 4. DEFAULT    - compile-time constants

style_res_t widget_get_style_prop_impl(...)
{
    // 1. Check inline (highest priority)
    if(style_get_prop(&widget->inline_style, prop, &res.value)) {
        return INLINE;
    }

    // 2. Check local styles (match selectors)
    for(i = 0; i < widget->local_style_count; i++) {
        if(selector_matches(local_styles[i].selector, part, state)) {
            if(style_get_prop(local_styles[i].style, prop, &res.value)) {
                found = true;  // Continue - later style can override
            }
        }
    }
    if(found) return LOCAL;

    // 3. Check inheritance (only for inheritable props)
    if(STYLE_PROP_INHERITABLE[prop] && widget->parent) {
        res = widget_get_style_prop(widget->parent, prop, PART_MAIN);
        if(res.found) return INHERITED;
    }

    // 4. Use default
    return DEFAULT;
}
```

**Gerçek dünya örneği**:
```c
// Theme sets bg_color = GRAY
theme_style.bg_color = GRAY;
widget_add_style(widget, &theme_style, SELECTOR_MAIN);

// Local style overrides to BLUE
local_style.bg_color = BLUE;
widget_add_style(widget, &local_style, SELECTOR_MAIN);

// Inline overrides to RED (highest priority)
widget_set_style_bg_color(widget, RED);

// Final result: RED (INLINE wins)
```

### Inheritance Mechanism

```c
// Inheritable properties flagged at compile time
static const bool STYLE_PROP_INHERITABLE[STYLE_PROP_COUNT] = {
    [STYLE_PROP_TEXT_COLOR] = true,
    [STYLE_PROP_TEXT_OPA] = true,
    [STYLE_PROP_TEXT_SIZE] = true,
    // All others default to false
};

// Resolution checks inheritance
if(allow_inherit && STYLE_PROP_INHERITABLE[prop] && widget->parent) {
    // RECURSIVE: Ask parent for property
    res = widget_get_style_prop(widget->parent, prop, PART_MAIN);

    if(res.found) {
        res.source = STYLE_RES_INHERITED;
        return res;
    }
}
```

**Örnek**:
```
Container (text_color = BLUE)
 └─ Button (no text_color set)
     └─ Label (no text_color set)

Label.text_color resolution:
1. Check inline: NOT FOUND
2. Check local: NOT FOUND
3. Check inherited:
   → Ask Button: NOT FOUND
   → Ask Container: FOUND (BLUE)
4. Result: BLUE (INHERITED from grandparent)
```

**Neden text properties inherit edilir?**
Tipografi tutarlılığı için. Bir container'ın font/color ayarları tüm child'larına uygulanır.

**LVGL'de hangi property'ler inheritable?**
- `src/misc/lv_style.h:282-295` - `LV_STYLE_PROP_INH_START` ile işaretli
- Text font, color, letter spacing
- Line height, spacing
- Border side (partial)

### Transition Simulation

```c
void transition_simulate(widget_t* widget, style_prop_t prop,
                        widget_part_t part)
{
    // Get current value
    style_res_t old_res = widget_get_style_prop(widget, prop, part);

    // Get transition duration
    style_res_t trans_res = widget_get_style_prop(widget,
                                 STYLE_PROP_TRANSITION_TIME, part);

    if(!trans_res.found || trans_res.value.num == 0) {
        return;  // No transition
    }

    // Simulate linear interpolation over time
    int steps = 5;
    for(int i = 0; i <= steps; i++) {
        float progress = (float)i / steps;  // 0.0 → 1.0

        // Interpolate based on property type
        switch(prop) {
            case STYLE_PROP_BG_OPA: {
                uint8_t start = old_res.value.opa;
                uint8_t end = 255;  // Target value
                uint8_t current = start + (end - start) * progress;
                printf("Opacity = %u\n", current);
                break;
            }
            // ...other property types...
        }
    }
}
```

**LVGL'de gerçek implementation**:
- `src/misc/lv_anim.c` - Animation engine
- `src/core/lv_obj_style.c:1080-1186` - Style transition
- Bezier easing curves
- Non-linear interpolation
- Multiple properties simultaneously

## 💡 İleri Seviye Konular

### Multiple Selector Styles on Same Widget

```c
// Widget can have different styles for different states
widget_t* button = widget_create(NULL, 1);

style_t normal_style;
style_set_bg_color(&normal_style, COLOR_BLUE);

style_t pressed_style;
style_set_bg_color(&pressed_style, COLOR_GREEN);

style_t focused_style;
style_set_border_color(&focused_style, COLOR_ORANGE);
style_set_border_width(&focused_style, 3);

// Add all styles with different selectors
widget_add_style(button, &normal_style, SELECTOR_MAIN);
widget_add_style(button, &pressed_style, SELECTOR_MAIN_PRESSED);
widget_add_style(button, &focused_style, SELECTOR_MAIN_FOCUSED);

// When state changes, different styles activate
button->state = STATE_DEFAULT;   // Uses normal_style
button->state = STATE_PRESSED;   // Uses normal_style + pressed_style
button->state = STATE_FOCUSED;   // Uses normal_style + focused_style
button->state = STATE_PRESSED | STATE_FOCUSED;  // Uses all three!
```

### Part-Specific Styling

```c
// Slider widget with multiple parts
widget_t* slider = widget_create(NULL, 1);

style_t main_style;
style_set_bg_color(&main_style, COLOR_GRAY);

style_t indicator_style;
style_set_bg_color(&indicator_style, COLOR_BLUE);

style_t knob_style;
style_set_bg_color(&knob_style, COLOR_WHITE);
style_set_radius(&knob_style, 999);  // Circle

// Different styles for different parts
widget_add_style(slider, &main_style, SELECTOR(PART_MAIN, STATE_DEFAULT));
widget_add_style(slider, &indicator_style,
                SELECTOR(PART_INDICATOR, STATE_DEFAULT));
widget_add_style(slider, &knob_style,
                SELECTOR(PART_KNOB, STATE_DEFAULT));

// Get style for specific part
style_res_t main_bg = widget_get_style_prop(slider,
                                           STYLE_PROP_BG_COLOR,
                                           PART_MAIN);        // GRAY

style_res_t indicator_bg = widget_get_style_prop(slider,
                                                 STYLE_PROP_BG_COLOR,
                                                 PART_INDICATOR);  // BLUE
```

### Theme Variants

```c
// Create multiple theme variants
typedef enum {
    THEME_VARIANT_LIGHT,
    THEME_VARIANT_DARK,
    THEME_VARIANT_HIGH_CONTRAST,
} theme_variant_t;

void theme_init_variant(theme_t* theme, theme_variant_t variant)
{
    switch(variant) {
        case THEME_VARIANT_LIGHT:
            style_set_bg_color(&theme->default_style, COLOR_WHITE);
            style_set_text_color(&theme->default_style, COLOR_BLACK);
            break;

        case THEME_VARIANT_DARK:
            style_set_bg_color(&theme->default_style, COLOR_BLACK);
            style_set_text_color(&theme->default_style, COLOR_WHITE);
            break;

        case THEME_VARIANT_HIGH_CONTRAST:
            style_set_bg_color(&theme->default_style, COLOR_BLACK);
            style_set_text_color(&theme->default_style, COLOR_WHITE);
            style_set_border_color(&theme->default_style, COLOR_WHITE);
            style_set_border_width(&theme->default_style, 3);
            break;
    }
}
```

### Style Property Caching

LVGL optimization:
```c
// Problem: Style resolution is expensive (cascade, inheritance, etc.)
// Solution: Cache recently resolved properties

typedef struct {
    style_prop_t prop;
    widget_part_t part;
    widget_state_t state;
    style_value_t value;
    bool valid;
} style_cache_entry_t;

style_cache_entry_t style_cache[CACHE_SIZE];

style_res_t widget_get_style_prop_cached(widget_t* widget,
                                         style_prop_t prop,
                                         widget_part_t part)
{
    // Check cache first
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cache[i].valid &&
           cache[i].prop == prop &&
           cache[i].part == part &&
           cache[i].state == widget->state) {
            return cache[i].value;  // CACHE HIT
        }
    }

    // Cache miss - do expensive resolution
    style_res_t res = widget_get_style_prop_impl(widget, prop, part);

    // Update cache
    cache[cache_index] = (style_cache_entry_t){
        .prop = prop,
        .part = part,
        .state = widget->state,
        .value = res.value,
        .valid = true
    };

    return res;
}

// Invalidate cache when state changes
void widget_add_state(widget_t* widget, widget_state_t state)
{
    widget->state |= state;

    // INVALIDATE CACHE
    for(int i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = false;
    }
}
```

## 🐛 Yaygın Hatalar ve Çözümler

### 1. Inline Style Kullanırken Theme Override Edilemiyor

```c
// ❌ HATALI - Inline en yüksek priority'ye sahip
widget_t* btn = widget_create(NULL, 1);
theme_apply(&my_theme, btn, "button");  // bg = BLUE (theme)

widget_set_style_bg_color(btn, COLOR_RED);  // bg = RED (inline)

// Artık theme style'ı değiştiremezsiniz!
style_set_bg_color(&my_theme.button_style, COLOR_GREEN);
// btn hala RED (inline wins)

// ✅ DOĞRU - Local style kullanın
style_t custom_button_style;
style_init(&custom_button_style);
style_set_bg_color(&custom_button_style, COLOR_RED);

widget_add_style(btn, &custom_button_style, SELECTOR_MAIN);
// Theme < Local < Inline hierarchy preserved
```

### 2. State Değiştirmeden Önce Transition Set Edilmeli

```c
// ❌ HATALI - Transition sonradan ekleniyor
widget_add_state(btn, STATE_PRESSED);  // Instant change
style_set_transition_time(&style, 300);  // Too late!

// ✅ DOĞRU - Transition önce
style_set_transition_time(&style, 300);
widget_add_style(btn, &style, SELECTOR_MAIN);
widget_add_state(btn, STATE_PRESSED);  // Now animates
```

### 3. Property Inheritance Beklentisi

```c
// ❌ YANLIŞ BEKLENT İ - bg_color inherit olmaz
parent->inline_style.bg_color = BLUE;
child->...  // bg_color != BLUE (uses DEFAULT)

// ✅ DOĞRU - Sadece text properties inherit olur
parent->inline_style.text_color = BLUE;
child->text_color == BLUE  // ✓ Inherited

// Eğer bg_color'u tüm child'lara uygulamak istiyorsanız:
// Option 1: Her child'a inline style
// Option 2: Shared style kullanın
style_t shared;
style_set_bg_color(&shared, BLUE);
widget_add_style(parent, &shared, SELECTOR_MAIN);
widget_add_style(child1, &shared, SELECTOR_MAIN);
widget_add_style(child2, &shared, SELECTOR_MAIN);
```

### 4. Selector State Bitmask Hatası

```c
// ❌ HATALI - Equality check instead of bitmask
if(state == STATE_PRESSED) { }  // Wrong for combined states

// Widget state = STATE_PRESSED | STATE_FOCUSED (0x0006)
// This check fails!

// ✅ DOĞRU - Bitmask test
if(state & STATE_PRESSED) { }  // ✓ Works for combined states
```

### 5. Style Object Lifecycle

```c
// ❌ TEHLİKELİ - Stack style dangling pointer
void setup_button(widget_t* btn)
{
    style_t local_style;  // STACK
    style_init(&local_style);
    style_set_bg_color(&local_style, COLOR_BLUE);

    widget_add_style(btn, &local_style, SELECTOR_MAIN);
    // local_style destroyed when function returns!
}

// ✅ DOĞRU - Heap veya static
static style_t global_style;  // Static lifetime

void setup_button(widget_t* btn)
{
    style_init(&global_style);
    style_set_bg_color(&global_style, COLOR_BLUE);
    widget_add_style(btn, &global_style, SELECTOR_MAIN);  // Safe
}

// Or heap:
style_t* heap_style = malloc(sizeof(style_t));
style_init(heap_style);
widget_add_style(btn, heap_style, SELECTOR_MAIN);
```

## 📚 Ek Kaynaklar

- **LVGL Resmi Dökümanları**: https://docs.lvgl.io
- **LVGL Style Sistemi**: https://docs.lvgl.io/master/overview/style.html
- **LVGL Theme Sistemi**: https://docs.lvgl.io/master/overview/theme.html
- **Bu Projenin Diğer Dökümanları**:
  - `LVGL_WIDGET_ARCHITECTURE.md` - Widget lifecycle
  - `LVGL_RENDER_PIPELINE_ARCHITECTURE.md` - Render pipeline
  - `../widget_system/README.md` - Widget system demo
  - `../render_pipeline/README.md` - Render pipeline demo

## ✅ Checklist

Demo'yu anladığınızı kontrol edin:

- [ ] Style property nedir ve nasıl saklanır?
- [ ] Selector nasıl çalışır? (Part + State)
- [ ] Cascading priority sırası nedir?
- [ ] Hangi property'ler inherit edilir?
- [ ] Theme nasıl çalışır?
- [ ] Transition ne zaman tetiklenir?
- [ ] Inline vs Local vs Theme style farkı?
- [ ] Selector matching bitmask logic?
- [ ] Style resolution algorithm?
- [ ] Hangi design pattern'lar kullanılıyor?

## 🎯 Pratik Egzersizler

### Egzersiz 1: Yeni Property
`STYLE_PROP_OPACITY` property'si ekleyin:
- Property tanımı
- Setter fonksiyon
- Getter fonksiyon
- Default value
- Demo scenario

### Egzersiz 2: Multi-State Style
Bir button için 4 state tanımlayın:
- DEFAULT (blue)
- PRESSED (green)
- FOCUSED (blue + orange border)
- DISABLED (gray)

Her state için ayrı style oluşturup test edin.

### Egzersiz 3: Custom Theme
"Ocean Theme" oluşturun:
- Mavi tonlar
- Yuvarlatılmış köşeler
- Beyaz text
- Shadow efektleri

Theme'i button, label ve container widget'larına uygulayın.

### Egzersiz 4: Cascading Test
Aynı property için 4 kaynaktan değer set edin:
- DEFAULT
- THEME
- LOCAL
- INLINE

Resolution sonucunu doğrulayın.

### Egzersiz 5: Deep Inheritance
3-level widget hierarchy:
```
Grandparent (text_color = RED, text_size = 20)
 └─ Parent (text_size = 16)
     └─ Child (no styles)
```

Child'ın resolved text_color ve text_size değerlerini bulun.

## 🎉 Sonuç

Bu demo, LVGL'nin style ve theme sisteminin temel mimarisini gösterir. Gerçek LVGL kodu çok daha optimize ve feature-rich olsa da, temel prensipler aynıdır.

**Önemli**: Bu bir öğretim aracıdır, production kodda LVGL'nin resmi API'lerini kullanın!

**Öğrendikleriniz**:
- ✅ Style property system (dynamic/static storage)
- ✅ Selector system (part + state bitmask)
- ✅ Cascading ve priority (INLINE > LOCAL > THEME > INHERITED > DEFAULT)
- ✅ Inheritance mekanizması (text properties only)
- ✅ Theme provider (automatic styling)
- ✅ Transition system (state change animation)
- ✅ Design patterns (Strategy, Flyweight, Composite, Observer, Template Method)

---

**Versiyon**: 1.0
**Tarih**: 2025-11-18
**Yazar**: Claude Code Demo
**Lisans**: MIT (Eğitim amaçlı)
