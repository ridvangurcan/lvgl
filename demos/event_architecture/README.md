# Event-Driven Architecture - Demo Kodları

Bu klasör, LVGL'de kullanılan 4 temel event stratejisinin standalone demo kodlarını içerir.

## 📂 Demo Dosyaları

### 1. Event Bubbling (Kabarcıklanma)
**Dosya:** `01_event_bubbling_demo.c`

**Konsept:** Olaylar child'dan parent'a doğru yükselir.

**Gösterilen Teknikler:**
- Parent chain traversal
- Delegated event handling
- stop_bubbling kontrolü
- Multi-level propagation

**Compile & Run:**
```bash
gcc 01_event_bubbling_demo.c -o bubbling_demo
./bubbling_demo
```

**Avantajlar:**
- Az handler registration
- Kolay child yönetimi
- Memory efficient

---

### 2. Event Filtering (Filtreleme)
**Dosya:** `02_event_filtering_demo.c`

**Konsept:** Handler'lar belirli event tiplerini dinler.

**Gösterilen Teknikler:**
- Event type matching
- EVENT_ALL wildcard
- Selective handler execution
- Filter optimization

**Compile & Run:**
```bash
gcc 02_event_filtering_demo.c -o filtering_demo
./filtering_demo
```

**Avantajlar:**
- Sadece ilgili event'ler işlenir
- Clean code (ayrı handler, ayrı concern)
- CPU efficient

---

### 3. Event Preprocessing (Ön-İşleme)
**Dosya:** `03_event_preprocessing_demo.c`

**Konsept:** Handler'lar default handler'dan ÖNCE çalışır.

**Gösterilen Teknikler:**
- 3-phase execution (preprocess → default → regular)
- Validation before action
- Interception (prevent default)
- Audit/logging

**Compile & Run:**
```bash
gcc 03_event_preprocessing_demo.c -o preprocessing_demo
./preprocessing_demo
```

**Avantajlar:**
- Validation önce
- Default behavior önlenebilir
- Audit her şeyden önce

---

### 4. Nested Event Handling (İç İçe Olaylar)
**Dosya:** `04_nested_events_demo.c`

**Konsept:** Event handler içinde başka event gönderilebilir.

**Gösterilen Teknikler:**
- Event stack (linked list)
- Deletion marking
- Safe cascading events
- Deep nesting (3+ levels)

**Compile & Run:**
```bash
gcc 04_nested_events_demo.c -o nested_demo
./nested_demo
```

**Avantajlar:**
- Event zincirleri güvenli
- Deletion crash etmez
- Karmaşık workflow'lar

---

### 5. Tüm Stratejiler (Karşılaştırma)
**Dosya:** `05_all_strategies_comparison.c`

**Konsept:** 4 stratejiyi birleştirerek gösterir.

**Gösterilen Teknikler:**
- Combined strategy usage
- Performance comparison
- Memory comparison
- Best practices

**Compile & Run:**
```bash
gcc 05_all_strategies_comparison.c -o comparison_demo
./comparison_demo
```

**Sonuç:**
- 4 strateji birlikte çalışır
- Her birinin yeri var
- Güçlü, esnek, güvenli!

---

## 🎯 Hızlı Başlangıç

Tüm demoları derle ve çalıştır:

```bash
# Event Bubbling
gcc 01_event_bubbling_demo.c -o bubbling && ./bubbling

# Event Filtering
gcc 02_event_filtering_demo.c -o filtering && ./filtering

# Event Preprocessing
gcc 03_event_preprocessing_demo.c -o preprocessing && ./preprocessing

# Nested Events
gcc 04_nested_events_demo.c -o nested && ./nested

# All Strategies
gcc 05_all_strategies_comparison.c -o comparison && ./comparison
```

---

## 📊 Strateji Karşılaştırması

| Strateji | Amaç | Kullanım | Önem |
|----------|------|----------|------|
| **Bubbling** | Propagation up | Delegated handling | 🟢 Yüksek |
| **Filtering** | Selective handling | Specific events | 🟢 Yüksek |
| **Preprocessing** | Before default | Validation | 🟡 Orta |
| **Nested Events** | Event in event | Cascading | 🟢 Yüksek |

---

## 💡 Kullanım Önerileri

### Bubbling
**Ne zaman kullan:**
- Çok sayıda benzer widget (100 button)
- Delegated handling istersen
- Memory optimization gerekiyorsa

**Örnek:**
```c
// 100 button, 1 handler
container->add_handler(click_handler);
for (int i = 0; i < 100; i++) {
    button[i]->enable_bubbling = true;
}
```

### Filtering
**Ne zaman kullan:**
- Belirli event'lere tepki vereceksen
- Gereksiz handler çağrısını önlemek istersen
- Clean code istersen

**Örnek:**
```c
// Sadece click'i yakala
add_handler(widget, handler, EVENT_CLICK);
// Hover, value_changed vb. ignore edilir
```

### Preprocessing
**Ne zaman kullan:**
- Validation gerekiyorsa
- Default behavior'u önlemek istersen
- Audit/logging istersen

**Örnek:**
```c
// Validate BEFORE value changes
add_handler(slider, validate, EVENT_VALUE_CHANGED, PREPROCESS);
// Default handler value'yu değiştirir
// Eğer validate fail ederse, stop_processing()
```

### Nested Events
**Ne zaman kullan:**
- Event handler içinde başka event göndereceksen
- Cascading update'ler varsa
- Complex workflow'lar varsa

**Örnek:**
```c
void click_handler(event_t* e) {
    // Nested event güvenli
    event_send(other_widget, EVENT_VALUE_CHANGED);
}
```

---

## ⚙️ Gereksinimler

- **Compiler:** GCC 4.9+ veya Clang 3.5+
- **C Standard:** C99 veya üzeri
- **Platform:** Linux, macOS, Windows (MinGW/Cygwin)

---

## 📖 Öğrenme Sırası

Önerilen demo çalıştırma sırası:

1. **Filtering** - En basit, anlaşılır
2. **Bubbling** - Propagation konsepti
3. **Preprocessing** - Execution order
4. **Nested Events** - En karmaşık
5. **All Strategies** - Hepsinin kombinasyonu

---

## 🎓 Alıştırmalar

### Alıştırma 1: Bubbling + Filtering
`01_event_bubbling_demo.c` dosyasını değiştir:
- Parent'a EVENT_CLICK handler ekle
- Child'lara EVENT_HOVER handler ekle
- Her ikisini de test et

### Alıştırma 2: Preprocess + Stop
`03_event_preprocessing_demo.c` dosyasını değiştir:
- Preprocess handler'da değer kontrolü yap
- Değer 100'den büyükse stop_processing()
- Regular handler'ın çağrılmadığını doğrula

### Alıştırma 3: Deep Nesting
`04_nested_events_demo.c` dosyasını değiştir:
- 5 seviye deep nesting yap
- Event stack depth'i logla
- Her seviyede farklı widget kullan

---

**Hazırlayan:** Claude (Anthropic)
**Tarih:** 2025-11-16
**Versiyon:** 1.0
