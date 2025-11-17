# API Design Demos (API Tasarımı Demoları)

Bu dizin, LVGL'de kullanılan profesyonel API tasarım stratejilerini gösteren standalone C demo programlarını içerir.

## İçerik

Her demo, LVGL'de kullanılan bir API tasarım stratejisini öğretir:

| Demo | Strateji | Açıklama |
|------|----------|----------|
| `01_opaque_pointers_demo.c` | **Opaque Pointers** | İmplementasyon detaylarını gizleme |
| `02_getter_setter_pattern_demo.c` | **Getter/Setter Pattern** | Kontrollü özellik erişimi |
| `03_internal_public_api_demo.c` | **Internal vs Public API** | API ayrımı ve stability |
| `04_const_correctness_demo.c` | **Const Correctness** | Tip güvenliği ve optimizasyon |

## Özellikler

✓ **Standalone**: LVGL bağımlılığı yok, sadece standart C
✓ **Eğitsel**: Her demo bir konsepti detaylı açıklar
✓ **Karşılaştırmalı**: İyi vs kötü örnekler gösterir
✓ **Türkçe**: Tüm açıklamalar Türkçe
✓ **LVGL Örnekleri**: Her demo LVGL'den gerçek örnekler içerir

## Derleme ve Çalıştırma

Her demo bağımsız olarak derlenebilir:

```bash
# Demo 1: Opaque Pointers
gcc 01_opaque_pointers_demo.c -o opaque_demo
./opaque_demo

# Demo 2: Getter/Setter Pattern
gcc 02_getter_setter_pattern_demo.c -o getter_setter_demo
./getter_setter_demo

# Demo 3: Internal vs Public API
gcc 03_internal_public_api_demo.c -o api_separation_demo
./api_separation_demo

# Demo 4: Const Correctness
gcc 04_const_correctness_demo.c -o const_demo -Wall
./const_demo
```

### Tüm demoları derle

```bash
# Linux/macOS
for f in *.c; do gcc "$f" -o "${f%.c}" -Wall; done

# Veya tek tek
gcc 01_opaque_pointers_demo.c -o demo1
gcc 02_getter_setter_pattern_demo.c -o demo2
gcc 03_internal_public_api_demo.c -o demo3
gcc 04_const_correctness_demo.c -o demo4 -Wall
```

## Demo Detayları

### 1. Opaque Pointers (Opak İşaretçiler)

**Dosya**: `01_opaque_pointers_demo.c`

**Öğretilenler**:
- Fully Opaque pattern (struct tanımı .c dosyasında)
- Semi-Opaque pattern (struct tanımı header'da ama internal)
- Enkapsülasyon avantajları
- ABI stability
- Memory safety
- Factory pattern kullanımı

**LVGL Örnekleri**:
- `_lv_event_dsc_t` - Fully opaque
- `_lv_obj_t` - Semi-opaque
- `_lv_group_t` - Semi-opaque

**Çıktı Örneği**:
```
========================================
AVANTAJ 1: Enkapsülasyon
========================================
[OPAQUE] Button created: 'Submit'
Label (via API): Send
✓ Kullanıcı struct detaylarına erişemez (güvenli)
```

---

### 2. Getter/Setter Pattern (Alıcı/Ayarlayıcı Deseni)

**Dosya**: `02_getter_setter_pattern_demo.c`

**Öğretilenler**:
- `set_*` pattern - Değer ayarlama
- `get_*` pattern - Değer okuma
- `add_*` / `clear_*` pattern - Flag işlemleri
- `has_*` / `is_*` pattern - Boolean sorgular
- Validation (doğrulama)
- Side effects yönetimi
- Lazy computation
- Caching

**LVGL Örnekleri**:
- `lv_obj_set_width()` / `lv_obj_get_width()`
- `lv_obj_add_flag()` / `lv_obj_clear_flag()`
- `lv_obj_has_flag()` / `lv_obj_is_visible()`
- `lv_obj_add_state()` / `lv_obj_has_state()`

**Çıktı Örneği**:
```
========================================
AVANTAJ 2: Validation (Doğrulama)
========================================
⚠ Warning: Negative width rejected (-50)
⚠ Warning: Width too large, clamping to 10000
✓ Width set to 200
```

---

### 3. Internal vs Public API Separation

**Dosya**: `03_internal_public_api_demo.c`

**Öğretilenler**:
- Prefix-based naming (`lv_*` vs `_lv_*`)
- Public API stability
- Internal API flexibility
- API surface reduction
- Refactoring freedom
- Semantic versioning
- Clear contract definition

**LVGL Örnekleri**:
- Public: `lv_init()`, `lv_obj_create()`, `lv_obj_del()`
- Internal: `_lv_refr_init()`, `_lv_obj_destruct()`, `_lv_inv_area()`

**Çıktı Örneği**:
```
========================================
✓ DOĞRU KULLANIM: Public API
========================================
[PUBLIC] mylib_init() - User-facing initialization
[INTERNAL] _mylib_init() called
[PUBLIC] mylib_object_create("Button")
[INTERNAL] _mylib_malloc(72 bytes)
```

---

### 4. Const Correctness (Const Doğruluğu)

**Dosya**: `04_const_correctness_demo.c`

**Öğretilenler**:
- Const parameters (read-only parametreler)
- Const return types (immutable return değerleri)
- Const struct members (değişmez alanlar)
- Const levels (farklı const seviyeleri)
- Compiler optimizations
- Embedded benefits (ROM vs RAM)
- API intent documentation
- Compile-time error prevention

**LVGL Örnekleri**:
- `lv_obj_get_width(const lv_obj_t* obj)`
- `const lv_obj_class_t* lv_obj_get_class(const lv_obj_t* obj)`
- `const lv_font_t font` (ROM'da)

**Çıktı Örneği**:
```
========================================
EMBEDDED BENEFITS (Gömülü Sistem Faydaları)
========================================
Non-const data (RAM):
  uint8_t buffer[1024];        → 1024 bytes RAM

Const data (ROM/Flash):
  const uint8_t table[256];    → 256 bytes ROM
  - RAM tasarrufu (0 byte RAM!)
```

## Öğrenme Sırası

Önerilen sıra:

1. **Opaque Pointers** → Enkapsülasyon temellerini öğren
2. **Getter/Setter** → Kontrollü erişim stratejilerini öğren
3. **Internal/Public** → API tasarımını öğren
4. **Const Correctness** → Tip güvenliğini öğren

## Karşılaştırma Tablosu

| Strateji | Amaç | LVGL'de Kullanım | Fayda |
|----------|------|------------------|-------|
| **Opaque Pointers** | Detayları gizle | `_lv_event_dsc_t`, `_lv_obj_t` | Enkapsülasyon, ABI stability |
| **Getter/Setter** | Kontrollü erişim | `lv_obj_set_*`, `lv_obj_get_*` | Validation, side effects |
| **Internal/Public** | API ayrımı | `lv_*` vs `_lv_*` | API clarity, refactoring |
| **Const Correctness** | Tip güvenliği | `const lv_obj_t*`, `const lv_font_t` | Safety, optimization |

## Birlikte Kullanım

Bu stratejiler LVGL'de birlikte kullanılır:

```c
// Opaque pointer
typedef struct _lv_obj_t lv_obj_t;

// Public API + Const correctness
const lv_obj_class_t* lv_obj_get_class(const lv_obj_t* obj);  // Getter

// Internal API (kullanıcı görmez)
void _lv_obj_destruct(lv_obj_t* obj);

// Setter (validation + side effects)
void lv_obj_set_width(lv_obj_t* obj, lv_coord_t w);
```

## Ek Kaynaklar

- **Ana Döküman**: `../../LVGL_API_DESIGN.md` - Detaylı Türkçe açıklama
- **LVGL Kaynak Kodu**: `/src/core` - Gerçek implementasyonlar
- **Diğer Demolar**:
  - `../memory_optimization/` - Bellek optimizasyonu stratejileri
  - `../event_architecture/` - Event-driven architecture

## Katkıda Bulunma

Bu demolar eğitim amaçlıdır. İyileştirme önerileri için issue açabilirsiniz.

## Notlar

- Demolar **LVGL bağımlılığı olmadan** çalışır
- Sadece **standart C** kullanır (C99+)
- **-Wall** flag'i ile derleyerek tüm uyarıları görebilirsiniz
- Const violations'ı test etmek için demo kodlarındaki yorumları açın

## Özet

Bu demolar, LVGL'nin nasıl profesyonel bir C API tasarımı sunduğunu gösterir:

✓ **Enkapsülasyon** → Opaque pointers ile gizlilik
✓ **Kontrol** → Getter/setter ile validation
✓ **Berraklık** → Internal/public ayrımı
✓ **Güvenlik** → Const correctness

Sonuç: **Sürdürülebilir, güvenli ve kullanımı kolay** bir API.

---

**Versiyon**: 1.0
**Tarih**: 2024
**Dil**: Türkçe
**LVGL Versiyonu**: v8.x
