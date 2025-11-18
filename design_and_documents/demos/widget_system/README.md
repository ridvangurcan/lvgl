# LVGL Widget System Architecture Demo

## 📋 Genel Bakış

Bu demo, **LVGL'nin widget sistem mimarisini** tamamen bağımsız, saf C kodu ile gösterir. LVGL kütüphanesine bağımlılığı yoktur ve standalone olarak derlenip çalıştırılabilir.

### Amaç

LVGL'nin karmaşık widget sistem mekanizmasını adım adım öğretmek için tasarlanmıştır. Gerçek grafik çizimi yapmaz, bunun yerine **printf ile her adımı detaylı açıklar**.

## 🎯 Kapsanan Konular

### 1. **Widget Base Class (lv_obj)**
- Base widget yapısı (coords, state, flags)
- Widget ağacı yönetimi (parent-child)
- Widget oluşturma ve silme lifecycle
- Memory management

### 2. **Widget Class System (Inheritance)**
- C dilinde OOP emülasyonu
- Class descriptor yapısı
- Base class pointer ile inheritance
- Virtual method pattern (function pointers)
- Constructor/Destructor chains

### 3. **Widget Lifecycle**
- Two-phase initialization (create + construct)
- Constructor chain (base → derived)
- Destructor chain (derived → base)
- Recursive tree cleanup

### 4. **Event System**
- Observer pattern implementation
- Event codes (clicked, value_changed, etc.)
- Dynamic event callback arrays
- Event filtering per callback
- User data passing

### 5. **Event Bubbling**
- Chain of Responsibility pattern
- Parent event propagation
- Stop bubbling mechanism
- Target vs current_target distinction

### 6. **State and Flag Management**
- Bit-packed state enum
- Widget flag system
- State change tracking
- Flag-based behavior control

### 7. **Custom Widgets**
- Button widget implementation
- Label widget implementation
- Widget-specific data extension
- Custom event handlers

## 🏗️ Mimari Bileşenler

```
┌─────────────────────────────────────────────────┐
│  Widget Class System                            │
│  ┌───────────────────────────────┐              │
│  │  widget_class_t               │              │
│  │  - name                       │              │
│  │  - base_class ──────────┐     │              │
│  │  - constructor          │     │              │
│  │  - destructor           │     │              │
│  │  - event_handler        │     │              │
│  │  - instance_size        │     │              │
│  └───────────────────────────────┘              │
│                 △                                │
│                 │ inheritance                    │
│         ┌───────┴────────┐                      │
│         │                │                       │
│  ┌──────────────┐ ┌──────────────┐             │
│  │ button_class │ │ label_class  │             │
│  └──────────────┘ └──────────────┘             │
└─────────────────────────────────────────────────┘
                    │
                    │ instantiation
                    ↓
┌─────────────────────────────────────────────────┐
│  Widget Instance Tree                           │
│  ┌───────────────────────────────┐              │
│  │  widget_t (container)         │              │
│  │  ├─ widget_t (button1)        │              │
│  │  │   └─ user_data: button_t   │              │
│  │  ├─ widget_t (button2)        │              │
│  │  │   └─ user_data: button_t   │              │
│  │  └─ widget_t (label1)         │              │
│  │      └─ user_data: label_t    │              │
│  └───────────────────────────────┘              │
└─────────────────────────────────────────────────┘
                    │
                    │ events
                    ↓
┌─────────────────────────────────────────────────┐
│  Event System                                   │
│  ┌───────────────────────────────┐              │
│  │  event_t                      │              │
│  │  - code (CLICKED, etc.)       │              │
│  │  - target (original widget)   │              │
│  │  - current_target (bubbling)  │              │
│  │  - user_data                  │              │
│  │  - stop_bubbling              │              │
│  └───────────────────────────────┘              │
│                 │                                │
│                 ↓                                │
│  ┌───────────────────────────────┐              │
│  │  Event Callbacks              │              │
│  │  - callback function          │              │
│  │  - filter (event code)        │              │
│  │  - user_data                  │              │
│  └───────────────────────────────┘              │
└─────────────────────────────────────────────────┘
```

## 🔧 Derleme ve Çalıştırma

### Gereksinimler
- GCC veya herhangi bir C derleyici
- Standart C kütüphanesi (stdio, stdlib, string, stdbool)

### Derleme

```bash
cd design_and_documents/demos/widget_system
gcc lvgl_widget_system_demo.c -o demo
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
║         LVGL Widget System Architecture Demo                  ║
║                                                                ║
║  Bu demo LVGL'nin widget sistem mimarisini gösterir:          ║
║  1. Widget Base Class (lv_obj)                                ║
║  2. Widget Inheritance (Class System)                         ║
║  3. Widget Lifecycle (Constructor/Destructor Chains)          ║
║  4. Event System (Observer Pattern)                           ║
║  5. Event Bubbling (Chain of Responsibility)                  ║
╚════════════════════════════════════════════════════════════════╝

========================================
SCENARIO 1: Widget Tree Creation
========================================

>>> Creating container widget
[BASE CONSTRUCTOR] Widget #0 created
    Class: widget
    Coords: [0,0 - 0,0]
    State: DEFAULT
    Flags: CLICKABLE

>>> Creating button1
[BASE CONSTRUCTOR] Widget #1 created
    Class: widget
    Coords: [0,0 - 0,0]
    State: DEFAULT
    Flags: CLICKABLE
[BUTTON CONSTRUCTOR] Button created: "Click Me"
    Added to parent widget #0

>>> Setting button1 position to (10, 10, 100, 40)
    Widget #1 moved to [10,10 - 109,49]

...
```

## 🎓 Öğrenme Yolu

### 1. İlk Okuma
Kodu yukarıdan aşağıya okuyun. Her bölüm açıklamalı:
- Basic types (coords_t, color_t, event_t)
- Widget state and flags
- Event system structures
- Widget base structure
- Widget class system
- Constructor/destructor chains
- Event processing and bubbling
- Custom widgets (Button, Label)
- Demo scenarios

### 2. Kodu Çalıştırma
Demo'yu çalıştırın ve çıktıyı inceleyin. Her adımın ne yaptığını görün.

### 3. Değişiklikler Yapma
Kodu değiştirerek deneyin:

**Yeni event type ekleyin:**
```c
typedef enum {
    EVENT_CLICKED,
    EVENT_VALUE_CHANGED,
    EVENT_FOCUSED,        // YENİ
    EVENT_DEFOCUSED,      // YENİ
    EVENT_DELETE_REQUEST
} event_code_t;
```

**Yeni widget oluşturun:**
```c
typedef struct {
    widget_t widget;  // Base class (FIRST MEMBER)
    int32_t value;
    int32_t min_value;
    int32_t max_value;
} slider_t;

void slider_constructor(widget_t* widget) {
    slider_t* slider = (slider_t*)widget;
    slider->value = 0;
    slider->min_value = 0;
    slider->max_value = 100;
    printf("[SLIDER CONSTRUCTOR] Created (0-100)\n");
}

void slider_event_handler(widget_t* widget, event_t* e) {
    slider_t* slider = (slider_t*)widget;
    if(e->code == EVENT_VALUE_CHANGED) {
        printf("[SLIDER EVENT] Value: %d\n", slider->value);
    }
}

const widget_class_t slider_class = {
    .name = "slider",
    .base_class = &base_widget_class,
    .constructor = slider_constructor,
    .destructor = NULL,
    .event_handler = slider_event_handler,
    .instance_size = sizeof(slider_t),
};
```

**Event callback ekleyin:**
```c
void on_button_clicked(event_t* e) {
    printf("[APP] Button clicked callback!\n");
    // Custom logic
}

widget_add_event_cb(button, on_button_clicked, EVENT_CLICKED, NULL);
```

### 4. LVGL Karşılaştırması
LVGL kaynak kodunu açın ve bu demo ile karşılaştırın:

| Demo Fonksiyon | LVGL Fonksiyon | Dosya |
|----------------|----------------|-------|
| `widget_create()` | `lv_obj_create()` | `src/core/lv_obj.c` |
| `widget_construct_chain()` | `lv_obj_class_create_obj()` | `src/core/lv_obj_class.c` |
| `widget_destruct_chain()` | `lv_obj_destructor()` | `src/core/lv_obj_class.c` |
| `widget_add_event_cb()` | `lv_obj_add_event_cb()` | `src/core/lv_event.c` |
| `widget_send_event()` | `lv_event_send()` | `src/core/lv_event.c` |
| `widget_process_event()` | `lv_obj_event_base()` | `src/core/lv_obj.c` |
| `widget_t` | `lv_obj_t` | `src/core/lv_obj.h` |
| `widget_class_t` | `lv_obj_class_t` | `src/core/lv_obj_class.h` |

## 🧩 Design Pattern'lar

Demo'da kullanılan design pattern'lar:

### 1. Prototype Pattern (Class System)
```c
typedef struct widget_class_t {
    const char* name;
    const struct widget_class_t* base_class;  // Inheritance
    void (*constructor)(widget_t* obj);       // Virtual method
    void (*destructor)(widget_t* obj);        // Virtual method
    void (*event_handler)(widget_t* obj, event_t* e);
    size_t instance_size;
} widget_class_t;

// Widget creation uses class as prototype
widget_t* widget_create(widget_t* parent, const widget_class_t* class_p) {
    widget_t* widget = malloc(class_p->instance_size);
    widget->class_p = class_p;
    // ...
}
```

### 2. Template Method Pattern (Constructor Chain)
```c
void widget_construct_chain(widget_t* widget) {
    const widget_class_t* original_class = widget->class_p;

    // STEP 1: BASE CLASS CONSTRUCTOR (recursive)
    if(widget->class_p->base_class) {
        widget->class_p = widget->class_p->base_class;
        widget_construct_chain(widget);  // TEMPLATE: recurse
    }

    widget->class_p = original_class;

    // STEP 2: OWN CONSTRUCTOR (hook)
    if(widget->class_p->constructor) {
        widget->class_p->constructor(widget);
    }
}
```

**LVGL'de aynı pattern**: `src/core/lv_obj_class.c:lv_obj_class_create_obj()`

### 3. Observer Pattern (Event System)
```c
// Subject: widget_t with event_callbacks array
// Observers: event_dsc_t entries

void widget_add_event_cb(widget_t* widget, event_cb_t cb,
                         event_code_t filter, void* user_data) {
    // Register observer
    widget->event_callbacks = realloc(...);
    widget->event_callbacks[idx] = (event_dsc_t){cb, filter, user_data};
}

void widget_process_event(widget_t* widget, event_t* e) {
    // Notify all observers
    for(uint32_t i = 0; i < widget->event_cb_count; i++) {
        if(dsc->filter == EVENT_ALL || dsc->filter == e->code) {
            dsc->cb(e);  // Call observer
        }
    }
}
```

**LVGL'de aynı pattern**: `src/core/lv_event.c`

### 4. Chain of Responsibility (Event Bubbling)
```c
void widget_send_event(widget_t* widget, event_t* e) {
    e->target = widget;
    e->current_target = widget;

    // STEP 1: Process at current level
    widget_process_event(widget, e);

    // STEP 2: Bubble to parent (chain continues)
    if(!e->stop_bubbling && widget->parent &&
       widget_has_flag(widget, WIDGET_FLAG_EVENT_BUBBLE)) {
        e->current_target = widget->parent;
        widget_process_event(widget->parent, e);  // Next in chain
    }
}
```

**LVGL'de aynı pattern**: `src/core/lv_event.c:lv_event_send()`

### 5. Composite Pattern (Widget Tree)
```c
typedef struct widget_t {
    widget_t* parent;      // Composite reference
    widget_t** children;   // Component array
    uint32_t child_count;
} widget_t;

// Recursive operations on tree
void widget_delete_recursive(widget_t* widget) {
    // Delete all children (leaf or composite)
    for(uint32_t i = 0; i < widget->child_count; i++) {
        widget_delete_recursive(widget->children[i]);
    }
    // Delete self
    widget_delete(widget);
}
```

**LVGL'de aynı pattern**: `src/core/lv_obj.c` - widget tree management

### 6. Strategy Pattern (Event Handlers)
```c
// Strategy interface
typedef void (*event_cb_t)(event_t* e);

// Concrete strategies
void on_button_clicked(event_t* e) { /* strategy 1 */ }
void on_value_changed(event_t* e) { /* strategy 2 */ }

// Context uses strategy
widget_add_event_cb(widget, on_button_clicked, EVENT_CLICKED, NULL);
```

## 🔍 Detaylı Kod Analizi

### Widget Memory Layout (First Member Trick)

```c
// Base class
typedef struct widget_t {
    const widget_class_t* class_p;
    // ... other base fields
} widget_t;

// Derived class
typedef struct {
    widget_t widget;  // FIRST MEMBER - kritik!
    char label[64];   // Extended data
    bool toggled;
} button_t;

// Safe casting (pointer to first member = pointer to struct)
widget_t* w = widget_create(NULL, &button_class);
button_t* btn = (button_t*)w;  // SAFE: widget is first member
```

**LVGL'de aynı mantık**: Tüm widget'lar ilk member olarak `lv_obj_t` içerir.

```c
// src/widgets/btn/lv_btn.h
typedef struct {
    lv_obj_t obj;  // FIRST MEMBER
} lv_btn_t;
```

### Constructor Chain Flow

```
button_create()
  └─► widget_create(&button_class)
       ├─► malloc(sizeof(button_t))  // instance_size
       ├─► widget_construct_chain()
       │    ├─► base_widget_constructor()  // base first
       │    └─► button_constructor()       // derived second
       └─► return widget_t*

Call stack:
1. widget_construct_chain(button)
2.   widget->class_p = base_widget_class (temporarily)
3.   widget_construct_chain(button)  // RECURSIVE
4.     base_widget_constructor()
5.   widget->class_p = button_class (restore)
6.   button_constructor()
```

**LVGL'de aynı flow**: `src/core/lv_obj_class.c:33-59`

### Destructor Chain Flow (Reverse Order)

```
widget_delete(button)
  └─► widget_destruct_chain()
       ├─► button_destructor()       // derived first
       └─► base_widget_destructor()  // base second

Call stack:
1. widget_destruct_chain(button)
2.   button_destructor()
3.   widget->class_p = base_widget_class
4.   widget_destruct_chain(button)  // RECURSIVE
5.     base_widget_destructor()
```

**Neden ters sıra?** Derived class, base class'ın kaynaklarını kullanıyor olabilir. Önce derived cleanup, sonra base cleanup.

**LVGL'de aynı flow**: `src/core/lv_obj_class.c:61-77`

### Event Filtering and Dispatch

```c
typedef struct {
    event_cb_t cb;
    event_code_t filter;  // EVENT_ALL or specific code
    void* user_data;
} event_dsc_t;

void widget_process_event(widget_t* widget, event_t* e) {
    // 1. Class event handler (always called)
    if(widget->class_p->event_handler) {
        widget->class_p->event_handler(widget, e);
    }

    // 2. User callbacks (filtered)
    for(uint32_t i = 0; i < widget->event_cb_count; i++) {
        event_dsc_t* dsc = &widget->event_callbacks[i];

        // Filter check
        if(dsc->filter == EVENT_ALL || dsc->filter == e->code) {
            e->user_data = dsc->user_data;
            dsc->cb(e);  // Invoke callback
        }
    }
}
```

**LVGL'de aynı mantık**: `src/core/lv_event.c:64-107`

### Event Bubbling Mechanism

```c
void widget_send_event(widget_t* widget, event_t* e) {
    e->target = widget;          // Original target (never changes)
    e->current_target = widget;  // Current handler (changes during bubble)
    e->stop_bubbling = false;

    // Process at original target
    widget_process_event(widget, e);

    // Bubble up the tree
    if(!e->stop_bubbling && widget->parent &&
       widget_has_flag(widget, WIDGET_FLAG_EVENT_BUBBLE)) {

        e->current_target = widget->parent;  // Update current
        widget_process_event(widget->parent, e);
        // Note: Real LVGL continues bubbling further up
    }
}

// Callback can stop bubbling
void my_event_handler(event_t* e) {
    printf("Handling event at widget #%u\n", e->current_target->id);
    e->stop_bubbling = true;  // Stop propagation
}
```

**LVGL'de aynı mantık**: `src/core/lv_event.c:lv_event_send()` - tam recursive bubbling

## 💡 İleri Seviye Konular

### Custom Widget with Extended State

```c
typedef enum {
    SLIDER_STATE_IDLE = 0x00,
    SLIDER_STATE_DRAGGING = 0x01,
    SLIDER_STATE_ANIMATING = 0x02,
} slider_custom_state_t;

typedef struct {
    widget_t widget;
    int32_t value;
    slider_custom_state_t custom_state;  // Widget-specific state
} slider_t;

void slider_event_handler(widget_t* widget, event_t* e) {
    slider_t* slider = (slider_t*)widget;

    if(e->code == EVENT_PRESSED) {
        slider->custom_state = SLIDER_STATE_DRAGGING;
    }
    else if(e->code == EVENT_RELEASED) {
        slider->custom_state = SLIDER_STATE_IDLE;
    }
}
```

### Virtual Method Override

```c
// Base class defines virtual method
void base_widget_render(widget_t* widget) {
    printf("[BASE RENDER] Widget #%u\n", widget->id);
}

// Derived class overrides
void button_render(widget_t* widget) {
    // Call base implementation first (optional)
    base_widget_render(widget);

    // Extended rendering
    button_t* btn = (button_t*)widget;
    printf("[BUTTON RENDER] Label: '%s'\n", btn->label);
}

// Class descriptor
const widget_class_t button_class = {
    .render = button_render,  // Override
};
```

### Multi-level Inheritance

```c
// Base: widget_t
const widget_class_t base_widget_class = { /*...*/ };

// Level 1: container_t extends widget_t
typedef struct {
    widget_t widget;
    layout_type_t layout;  // flex, grid, etc.
} container_t;

const widget_class_t container_class = {
    .base_class = &base_widget_class,
    .constructor = container_constructor,
};

// Level 2: panel_t extends container_t
typedef struct {
    container_t container;  // FIRST MEMBER
    bool has_border;
} panel_t;

const widget_class_t panel_class = {
    .base_class = &container_class,  // Points to container
    .constructor = panel_constructor,
};

// Constructor chain: base → container → panel
```

### Event Data Passing

```c
typedef struct {
    int32_t old_value;
    int32_t new_value;
} value_change_data_t;

void slider_set_value(widget_t* widget, int32_t value) {
    slider_t* slider = (slider_t*)widget;

    value_change_data_t data = {
        .old_value = slider->value,
        .new_value = value
    };

    slider->value = value;

    event_t e = {
        .code = EVENT_VALUE_CHANGED,
        .user_data = &data  // Pass custom data
    };
    widget_send_event(widget, &e);
}

void on_value_changed(event_t* e) {
    value_change_data_t* data = (value_change_data_t*)e->user_data;
    printf("Value changed: %d → %d\n", data->old_value, data->new_value);
}
```

## 🐛 Yaygın Hatalar ve Çözümler

### 1. First Member Rule İhlali
```c
// ❌ HATALI
typedef struct {
    char label[64];    // Widget base class ilk değil!
    widget_t widget;
    bool toggled;
} button_t;

widget_t* w = widget_create(NULL, &button_class);
button_t* btn = (button_t*)w;  // CRASH! Pointer mismatch

// ✅ DOĞRU
typedef struct {
    widget_t widget;  // İLK MEMBER
    char label[64];
    bool toggled;
} button_t;
```

### 2. Constructor Chain'i Manuel Çağırma
```c
// ❌ HATALI - Constructor chain otomatik çalışır
widget_t* widget = malloc(sizeof(button_t));
base_widget_constructor(widget);  // Manuel çağırma
button_constructor(widget);

// ✅ DOĞRU - widget_create zaten chain'i çalıştırır
widget_t* widget = widget_create(NULL, &button_class);
```

### 3. Parent Silmeden Child Silme
```c
// ❌ HATALI - Dangling pointer
widget_delete(child);
// Parent hala child'a pointer tutuyor!

// ✅ DOĞRU - widget_delete otomatik olarak parent'tan çıkarır
widget_delete(child);  // Removes from parent->children too
```

### 4. Event Callback'te Widget Silme
```c
// ❌ TEHLİKELİ
void on_close_clicked(event_t* e) {
    widget_delete(e->current_target);  // Callback içinde widget silme!
    // Callback return edince crash olabilir
}

// ✅ DOĞRU - Flag ile işaretle, sonra sil
void on_close_clicked(event_t* e) {
    widget_add_flag(e->current_target, WIDGET_FLAG_DELETE_REQUEST);
}

// Main loop'ta
if(widget_has_flag(widget, WIDGET_FLAG_DELETE_REQUEST)) {
    widget_delete(widget);
}
```

### 5. Event Bubbling Sonsuz Döngü
```c
// ❌ HATALI - Parent event handler çocuğa event gönderiyor
void parent_event_handler(widget_t* widget, event_t* e) {
    if(e->code == EVENT_CLICKED) {
        widget_send_event(widget->children[0], e);  // Tekrar bubble up!
    }
}

// ✅ DOĞRU - stop_bubbling kullan
void parent_event_handler(widget_t* widget, event_t* e) {
    if(e->code == EVENT_CLICKED) {
        e->stop_bubbling = true;  // Döngüyü kır
        // Handle event
    }
}
```

## 📚 Ek Kaynaklar

- **LVGL Resmi Dökümanları**: https://docs.lvgl.io
- **LVGL Widget Geliştirme**: https://docs.lvgl.io/master/widgets/index.html
- **LVGL Event Sistemi**: https://docs.lvgl.io/master/overview/event.html
- **Bu Projenin Diğer Dökümanları**:
  - `LVGL_WIDGET_ARCHITECTURE.md` - Detaylı widget lifecycle
  - `LVGL_RENDER_PIPELINE_ARCHITECTURE.md` - Render pipeline
  - `LVGL_CORE_DESIGN_PATTERNS.md` - Design patterns

## ✅ Checklist

Demo'yu anladığınızı kontrol edin:

- [ ] Widget base class (lv_obj) yapısı nasıl?
- [ ] First member trick neden önemli?
- [ ] Constructor chain nasıl çalışıyor? (base → derived)
- [ ] Destructor chain neden ters sırada? (derived → base)
- [ ] Event system nasıl çalışıyor? (Observer pattern)
- [ ] Event bubbling mekanizması nasıl? (Chain of Responsibility)
- [ ] Event filtering nasıl yapılıyor?
- [ ] Custom widget nasıl oluşturulur?
- [ ] Widget tree yönetimi nasıl? (Composite pattern)
- [ ] Hangi design pattern'lar kullanılıyor?

## 🎯 Pratik Egzersizler

### Egzersiz 1: Checkbox Widget
Checkbox widget'ı implement edin:
- `bool checked` state
- `EVENT_VALUE_CHANGED` event
- Constructor ve event handler yazın

### Egzersiz 2: Event Listener
Tüm event'leri logla:
```c
void event_logger(event_t* e) {
    printf("[EVENT LOG] Code=%d, Target=#%u, Current=#%u\n",
           e->code, e->target->id, e->current_target->id);
}

widget_add_event_cb(container, event_logger, EVENT_ALL, NULL);
```

### Egzersiz 3: Composite Widget
İçinde 2 button ve 1 label olan bir "dialog" widget yapın.

### Egzersiz 4: State Machine
Button widget'a state machine ekleyin:
- IDLE → PRESSED → RELEASED → IDLE
- Her state transition'da event gönderin

## 🎉 Sonuç

Bu demo, LVGL'nin widget sistem mimarisini basitleştirilmiş ama işlevsel bir şekilde gösterir. Gerçek LVGL kodu çok daha karmaşık olsa da, temel prensipler aynıdır.

**Önemli**: Bu bir öğretim aracıdır, production kodda LVGL'nin resmi API'lerini kullanın!

**Öğrendikleriniz**:
- ✅ C dilinde OOP emülasyonu (class system, inheritance, virtual methods)
- ✅ Widget lifecycle management (constructor/destructor chains)
- ✅ Event-driven architecture (Observer pattern)
- ✅ Event bubbling (Chain of Responsibility)
- ✅ Widget tree management (Composite pattern)
- ✅ Memory-efficient design (bit packing, first member trick)

---

**Versiyon**: 1.0
**Tarih**: 2025-11-18
**Yazar**: Claude Code Demo
**Lisans**: MIT (Eğitim amaçlı)
