# LVGL Projeye Entegrasyon Rehberi

**Hedef Kitle**: LVGL'yi ilk kez projelerine entegre edecek yazılımcılar

**Versiyon**: LVGL v8/v9 için geçerli

---

## İçindekiler

1. [LVGL Nedir?](#1-lvgl-nedir)
2. [Gereksinimler ve Ön Hazırlık](#2-gereksinimler-ve-ön-hazırlık)
3. [LVGL Kaynak Kodunu Projeye Ekleme](#3-lvgl-kaynak-kodunu-projeye-ekleme)
4. [Konfigürasyon (lv_conf.h)](#4-konfigürasyon-lv_confh)
5. [HAL (Hardware Abstraction Layer) Implementasyonu](#5-hal-hardware-abstraction-layer-implementasyonu)
6. [İlk UI Oluşturma](#6-i̇lk-ui-oluşturma)
7. [Build System Entegrasyonu](#7-build-system-entegrasyonu)
8. [Debugging ve Troubleshooting](#8-debugging-ve-troubleshooting)
9. [Platform-Specific Örnekler](#9-platform-specific-örnekler)
10. [Best Practices](#10-best-practices)

---

## 1. LVGL Nedir?

**LVGL (Light and Versatile Graphics Library)** açık kaynaklı, hafif bir grafik kütüphanesidir.

### Temel Özellikler

- ✅ **Platform Bağımsız**: Herhangi bir MCU/MPU'da çalışır
- ✅ **Düşük Kaynak**: 64 KB RAM, 128 KB Flash'dan başlar
- ✅ **Modern UI**: Animasyon, anti-aliasing, opacity
- ✅ **Zengin Widget**: 50+ hazır UI bileşeni
- ✅ **Ücretsiz**: MIT lisansı

### Desteklenen Platformlar

- **MCU**: ARM Cortex-M (STM32, NXP, Nordic, etc.), RISC-V, AVR
- **MPU**: ARM Cortex-A (Raspberry Pi, iMX, etc.)
- **Desktop**: Windows, Linux, macOS (simülatör)
- **OS**: Bare-metal, FreeRTOS, Zephyr, Linux

### Ne Zaman LVGL Kullanılır?

| Kullan | Kullanma |
|--------|----------|
| ✅ Gömülü sistem GUI | ❌ High-end desktop app |
| ✅ Touch screen interface | ❌ 3D graphics |
| ✅ Dashboard, HMI | ❌ Video oyunu |
| ✅ IoT cihaz ekranı | ❌ Web browser |

---

## 2. Gereksinimler ve Ön Hazırlık

### 2.1 Donanım Gereksinimleri

**Minimum (Basit UI)**:
- MCU: 16 MHz+
- RAM: 64 KB (partial buffering ile)
- Flash: 128 KB
- Display: 240x240 piksel

**Önerilen (Modern UI)**:
- MCU: 100 MHz+
- RAM: 256 KB
- Flash: 512 KB
- Display: 320x240 - 800x480 piksel

**Optimal (Smooth Animation)**:
- MCU: 200 MHz+, FPU
- RAM: 512 KB+
- Flash: 1 MB+
- Display: Herhangi bir boyut
- GPU: DMA2D, PXP, etc. (opsiyonel)

### 2.2 Yazılım Gereksinimleri

#### Compiler
- GCC ARM: `arm-none-eabi-gcc` (STM32, NXP, etc.)
- GCC RISC-V: `riscv64-unknown-elf-gcc`
- Keil MDK (ARM)
- IAR Embedded Workbench

#### Build System
- Make
- CMake
- Platform-specific IDE (STM32CubeIDE, MCUXpresso, etc.)

#### C Standartı
- **Minimum**: C99
- **Önerilen**: C11 veya C++11

### 2.3 Ön Hazırlık Checklist

- [ ] Display driver çalışıyor mu? (test pattern görüntüle)
- [ ] Touch controller çalışıyor mu? (touch koordinatlarını oku)
- [ ] SPI/I2C/RGB interface doğru konfigure edilmiş mi?
- [ ] DMA kullanılacaksa enable mi?
- [ ] Yeterli RAM/Flash var mı?
- [ ] Toolchain kurulu ve çalışıyor mu?

---

## 3. LVGL Kaynak Kodunu Projeye Ekleme

### 3.1 LVGL İndirme

#### Yöntem 1: Git Clone (Önerilen)

```bash
cd your_project_directory
git clone --recurse-submodules https://github.com/lvgl/lvgl.git

# Specific version için
git clone -b release/v8.3 https://github.com/lvgl/lvgl.git
```

#### Yöntem 2: Zip İndirme

1. https://github.com/lvgl/lvgl/releases adresine git
2. Son stable release'i indir (örn: v8.3.11)
3. Projenize çıkart: `your_project/lvgl/`

#### Yöntem 3: Git Submodule (En İyi Pratik)

```bash
cd your_project_directory
git submodule add https://github.com/lvgl/lvgl.git libs/lvgl
git submodule update --init --recursive
```

**Avantajlar**:
- Version control
- Kolay güncelleme
- Team collaboration

### 3.2 Dizin Yapısı

```
your_project/
├── src/
│   ├── main.c
│   ├── lv_port_disp.c      # Display driver
│   ├── lv_port_indev.c     # Touch driver
│   └── ui/                  # UI kodu
├── include/
│   ├── lv_conf.h           # LVGL config (ÖNEMLI!)
│   ├── lv_port_disp.h
│   └── lv_port_indev.h
├── lvgl/                    # LVGL source
│   ├── src/
│   ├── examples/
│   └── demos/
└── build/
```

### 3.3 Hangi Dosyaları Dahil Etmeli?

**Minimum (Core LVGL)**:
```
lvgl/src/
├── core/           # Widget base, event, style
├── draw/           # Drawing engine
├── font/           # Fonts
├── hal/            # Hardware abstraction
├── misc/           # Utilities
└── widgets/        # UI widgets
```

**Opsiyonel**:
```
lvgl/
├── examples/       # Örnek kodlar (learning için)
├── demos/          # Demo uygulamalar
└── tests/          # Unit testler (skip OK)
```

---

## 4. Konfigürasyon (lv_conf.h)

### 4.1 lv_conf.h Oluşturma

```bash
# LVGL template'i kopyala
cp lvgl/lv_conf_template.h include/lv_conf.h
```

**ÖNEMLI**: `lv_conf_template.h` dosyasının başındaki `#if 0` satırını düzenleyin:

```c
// ❌ YANLIŞ (template hali)
#if 0
/*Set it to "1" to enable content*/

// ✅ DOĞRU
#if 1  // ENABLE!
/*Set it to "1" to enable content*/
```

### 4.2 Temel Konfigürasyonlar

#### Display Ayarları

```c
/*====================
   DISPLAY SETTINGS
 *====================*/

/* Horizontal and vertical resolution of the display */
#define LV_HOR_RES_MAX          320
#define LV_VER_RES_MAX          240

/* Color depth: 1 (1 byte per pixel), 8, 16, 32 */
#define LV_COLOR_DEPTH          16    // RGB565 (en yaygın)

/* Default display refresh period. Can be changed in the display driver */
#define LV_DISP_DEF_REFR_PERIOD 30    // 30ms = ~33 FPS

/* Swap the 2 bytes of RGB565 color (useful for some displays) */
#define LV_COLOR_16_SWAP        0     // Display'e göre ayarla
```

**Color Depth Seçimi**:
- `1`: Monochrome (1 bit)
- `8`: 256 color (RGB332)
- `16`: RGB565 (**en yaygın, önerilen**)
- `32`: ARGB8888 (yüksek kalite, fazla RAM)

#### Memory Ayarları

```c
/*====================
   MEMORY SETTINGS
 *====================*/

/* Size of the memory available for LVGL's internal memory manager (bytes) */
#define LV_MEM_CUSTOM           0     // 0: LVGL internal allocator
#define LV_MEM_SIZE             (32U * 1024U)  // 32 KB

/* Set the number of maximum cached style properties */
#define LV_STYLE_PROP_CACHE_SIZE 256

/* Number of cached fonts */
#define LV_FONT_FMT_TXT_CACHE_SIZE 256
```

**RAM Hesaplama**:
```
Minimum LVGL heap: 16-32 KB (basit UI)
Önerilen: 32-64 KB (orta karmaşıklık)
Rahat: 64-128 KB (karmaşık UI)
```

#### Font Ayarları

```c
/*=================
   FONT SETTINGS
 *=================*/

/* Montserrat fonts with various sizes and bpp (bits per pixel) */
#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    0
#define LV_FONT_MONTSERRAT_12    0
#define LV_FONT_MONTSERRAT_14    1  // Enable 14pt
#define LV_FONT_MONTSERRAT_16    1  // Enable 16pt
#define LV_FONT_MONTSERRAT_18    0
#define LV_FONT_MONTSERRAT_20    1  // Enable 20pt
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    0
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    0
#define LV_FONT_MONTSERRAT_30    0
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_34    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_38    0
#define LV_FONT_MONTSERRAT_40    0
#define LV_FONT_MONTSERRAT_42    0
#define LV_FONT_MONTSERRAT_44    0
#define LV_FONT_MONTSERRAT_46    0
#define LV_FONT_MONTSERRAT_48    0

/* Default font */
#define LV_FONT_DEFAULT         &lv_font_montserrat_14
```

**Font Size Rehberi**:
- 12-14pt: Küçük text, detaylar
- 16-20pt: Normal text (önerilen)
- 24-32pt: Başlıklar
- 40+pt: Büyük sayılar, saat display

**Not**: Her enabled font ~10-50 KB Flash kullanır!

#### GPU/DMA Ayarları

```c
/*=========================
   GPU AND DMA SETTINGS
 *=========================*/

/* STM32 DMA2D (Chrom-ART) support */
#define LV_USE_GPU_STM32_DMA2D  0    // STM32F4/F7/H7 için 1 yap

/* NXP PXP support (iMXRT) */
#define LV_USE_GPU_NXP_PXP      0

/* NXP VGLite support (iMXRT) */
#define LV_USE_GPU_NXP_VG_LITE  0

/* SDL GPU acceleration (for desktop simulator) */
#define LV_USE_GPU_SDL          0
```

#### Log Ayarları (Debugging)

```c
/*==================
   LOGGING
 *==================*/

/* Enable the log module */
#define LV_USE_LOG      1

#if LV_USE_LOG

/* How important log should be added:
 * LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
 * LV_LOG_LEVEL_INFO        Log important events
 * LV_LOG_LEVEL_WARN        Log if something unexpected happened
 * LV_LOG_LEVEL_ERROR       Only critical issues
 * LV_LOG_LEVEL_USER        Only user messages
 * LV_LOG_LEVEL_NONE        Do not log anything */
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN

/* 1: Print the log with 'printf'; 0: Custom log function */
#define LV_LOG_PRINTF   1

#if LV_LOG_PRINTF
  /* printf ile log */
#else
  /* Custom log function */
  void my_log_cb(const char * buf);
  #define LV_LOG_PRINTF   my_log_cb
#endif

#endif  /*LV_USE_LOG*/
```

#### Optimizasyon Ayarları

```c
/*=====================
   COMPILER SETTINGS
 *=====================*/

/* Define a custom attribute to `lv_tick_inc` function */
#define LV_ATTRIBUTE_TICK_INC

/* Define a custom attribute to `lv_timer_handler` function */
#define LV_ATTRIBUTE_TIMER_HANDLER

/* Performance monitor (FPS, render time) */
#define LV_USE_PERF_MONITOR     1    // Development için enable

/* Memory monitor (heap usage, fragmentation) */
#define LV_USE_MEM_MONITOR      1    // Development için enable
```

### 4.3 Platform-Specific Şablonlar

#### STM32F4 (128 KB RAM, 1 MB Flash)

```c
#define LV_HOR_RES_MAX          320
#define LV_VER_RES_MAX          240
#define LV_COLOR_DEPTH          16
#define LV_MEM_SIZE             (32U * 1024U)
#define LV_USE_GPU_STM32_DMA2D  1    // DMA2D enable
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
```

#### ESP32 (520 KB RAM)

```c
#define LV_HOR_RES_MAX          320
#define LV_VER_RES_MAX          240
#define LV_COLOR_DEPTH          16
#define LV_MEM_SIZE             (64U * 1024U)
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_20   1
```

#### STM32F103 (64 KB RAM - kısıtlı)

```c
#define LV_HOR_RES_MAX          240
#define LV_VER_RES_MAX          240
#define LV_COLOR_DEPTH          16
#define LV_MEM_SIZE             (24U * 1024U)   // Tight!
#define LV_FONT_MONTSERRAT_14   1    // Sadece 1 font
#define LV_FONT_MONTSERRAT_16   0    // Disable extras
```

---

## 5. HAL (Hardware Abstraction Layer) Implementasyonu

LVGL donanımdan bağımsız çalışır. Donanım-specific kod **porting layer**'da implement edilir.

### 5.1 Display Driver (lv_port_disp.c)

#### Şablon Kopyalama

```bash
cp lvgl/examples/porting/lv_port_disp_template.c src/lv_port_disp.c
cp lvgl/examples/porting/lv_port_disp_template.h include/lv_port_disp.h
```

#### Display Driver Implementasyonu

```c
// lv_port_disp.c

#include "lv_port_disp.h"
#include "lvgl.h"

/* Display resolution */
#define MY_DISP_HOR_RES    320
#define MY_DISP_VER_RES    240

/* Display buffer (partial buffering) */
#define BUF_HEIGHT         10    // 10 satırlık buffer

static lv_disp_draw_buf_t draw_buf_dsc;
static lv_color_t buf_1[MY_DISP_HOR_RES * BUF_HEIGHT];
static lv_color_t buf_2[MY_DISP_HOR_RES * BUF_HEIGHT];  // Double buffering

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();   // Your display initialization (SPI, GPIO, etc.)

    /*------------------------------------
     * Create a buffer for drawing
     *-----------------------------------*/
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, MY_DISP_HOR_RES * BUF_HEIGHT);

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/
    static lv_disp_drv_t disp_drv;          // Descriptor
    lv_disp_drv_init(&disp_drv);            // Initialize

    /* Set up the functions to access to your display */
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;         // YOUR flush function
    disp_drv.draw_buf = &draw_buf_dsc;

    /* Optional callbacks */
    // disp_drv.rounder_cb = disp_rounder;  // Round coordinates
    // disp_drv.set_px_cb = disp_set_px;    // Custom pixel set
    // disp_drv.monitor_cb = disp_monitor;  // Performance monitor

    /* Register the driver */
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Flush the content of the internal buffer to the display */
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*
     * İşlemler:
     * 1. Display window set et (area->x1, y1, x2, y2)
     * 2. color_p buffer'ını display'e transfer et (SPI/DMA)
     * 3. Transfer bitince lv_disp_flush_ready() çağır
     */

    int32_t x, y;
    int32_t width = area->x2 - area->x1 + 1;
    int32_t height = area->y2 - area->y1 + 1;

    /* Set display window */
    disp_set_window(area->x1, area->y1, area->x2, area->y2);

    /* Method 1: Polling transfer (basit ama yavaş) */
    #if 0
    uint32_t size = width * height;
    for(uint32_t i = 0; i < size; i++) {
        disp_send_pixel(color_p[i]);
    }
    lv_disp_flush_ready(disp_drv);  // Transfer complete
    #endif

    /* Method 2: DMA transfer (önerilen) */
    #if 1
    uint32_t size = width * height * sizeof(lv_color_t);
    disp_dma_transfer((uint8_t*)color_p, size);

    // NOT: lv_disp_flush_ready() DMA complete callback'te çağrılacak!
    #endif
}

/* DMA Transfer Complete Callback (example) */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* DMA transfer complete */
    lv_disp_flush_ready(&disp_drv);  // Tell LVGL we're done
}
```

#### Display-Specific Fonksiyonlar

Bu fonksiyonları display controller'ınıza göre implement edin:

```c
/* Display initialization (example for ILI9341) */
void disp_init(void)
{
    /* GPIO init (CS, DC, RST) */
    GPIO_Init();

    /* SPI init */
    SPI_Init();

    /* Reset display */
    LCD_Reset();

    /* Send initialization commands */
    LCD_WriteCommand(0x01);  // Software Reset
    HAL_Delay(120);

    LCD_WriteCommand(0x11);  // Sleep Out
    HAL_Delay(120);

    LCD_WriteCommand(0x3A);  // Pixel Format
    LCD_WriteData(0x55);     // 16-bit color

    LCD_WriteCommand(0x29);  // Display ON
}

/* Set display window */
void disp_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_WriteCommand(0x2A);  // Column Address Set
    LCD_WriteData(x1 >> 8);
    LCD_WriteData(x1 & 0xFF);
    LCD_WriteData(x2 >> 8);
    LCD_WriteData(x2 & 0xFF);

    LCD_WriteCommand(0x2B);  // Page Address Set
    LCD_WriteData(y1 >> 8);
    LCD_WriteData(y1 & 0xFF);
    LCD_WriteData(y2 >> 8);
    LCD_WriteData(y2 & 0xFF);

    LCD_WriteCommand(0x2C);  // Memory Write
}

/* DMA transfer (example for STM32) */
void disp_dma_transfer(uint8_t* data, uint32_t size)
{
    HAL_SPI_Transmit_DMA(&hspi1, data, size);
}
```

### 5.2 Touch/Input Driver (lv_port_indev.c)

#### Şablon Kopyalama

```bash
cp lvgl/examples/porting/lv_port_indev_template.c src/lv_port_indev.c
cp lvgl/examples/porting/lv_port_indev_template.h include/lv_port_indev.h
```

#### Touch Driver Implementasyonu

```c
// lv_port_indev.c

#include "lv_port_indev.h"
#include "lvgl.h"

/* Touchpad input device */
static lv_indev_drv_t indev_drv;

void lv_port_indev_init(void)
{
    /*------------------
     * Touchpad
     * -----------------*/

    /* Initialize your touchpad controller (I2C, SPI) */
    touchpad_init();

    /* Register a touchpad input device */
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Read touchpad data */
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;

    /*
     * İşlemler:
     * 1. Touch controller'dan veri oku (I2C/SPI)
     * 2. data->point.x, data->point.y set et
     * 3. data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL
     */

    bool touched = touchpad_is_touched();

    if(touched) {
        touchpad_get_xy(&last_x, &last_y);
        data->state = LV_INDEV_STATE_PR;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }

    /* Set the coordinates (last valid coordinates if released) */
    data->point.x = last_x;
    data->point.y = last_y;
}
```

#### Touch Controller Örnekleri

**FT6236 (I2C Touch Controller)**:

```c
#define FT6236_I2C_ADDR 0x38

bool touchpad_is_touched(void)
{
    uint8_t data;
    HAL_I2C_Mem_Read(&hi2c1, FT6236_I2C_ADDR, 0x02, 1, &data, 1, 100);
    return (data & 0x0F) > 0;  // Touch count
}

void touchpad_get_xy(int16_t *x, int16_t *y)
{
    uint8_t data[4];
    HAL_I2C_Mem_Read(&hi2c1, FT6236_I2C_ADDR, 0x03, 1, data, 4, 100);

    *x = ((data[0] & 0x0F) << 8) | data[1];
    *y = ((data[2] & 0x0F) << 8) | data[3];
}
```

**XPT2046 (SPI Resistive Touch)**:

```c
#define CMD_X_READ  0x90
#define CMD_Y_READ  0xD0

void touchpad_get_xy(int16_t *x, int16_t *y)
{
    uint8_t tx[3], rx[3];

    /* Read X */
    tx[0] = CMD_X_READ;
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, 100);
    *x = ((rx[1] << 8) | rx[2]) >> 3;

    /* Read Y */
    tx[0] = CMD_Y_READ;
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, 100);
    *y = ((rx[1] << 8) | rx[2]) >> 3;

    /* Scale to display resolution (12-bit ADC → pixel) */
    *x = (*x * MY_DISP_HOR_RES) / 4096;
    *y = (*y * MY_DISP_VER_RES) / 4096;
}
```

### 5.3 Tick Interface (Zaman Kaynağı)

LVGL animasyon ve zamanlama için **millisecond tick** gerektirir.

#### Yöntem 1: SysTick (Bare-Metal)

```c
// main.c

volatile uint32_t lvgl_tick = 0;

/* SysTick interrupt (her 1ms) */
void SysTick_Handler(void)
{
    HAL_IncTick();      // HAL tick
    lvgl_tick++;        // LVGL tick
}

/* LVGL tick callback */
uint32_t my_tick_get_cb(void)
{
    return lvgl_tick;
}

/* main() içinde */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    lv_init();
    lv_tick_set_cb(my_tick_get_cb);  // Register tick callback

    /* ... */
}
```

#### Yöntem 2: HAL_GetTick() (STM32)

```c
/* LVGL tick callback */
uint32_t my_tick_get_cb(void)
{
    return HAL_GetTick();  // HAL zaten 1ms tick tutuyor
}

/* main() içinde */
lv_tick_set_cb(my_tick_get_cb);
```

#### Yöntem 3: FreeRTOS

```c
/* LVGL tick callback */
uint32_t my_tick_get_cb(void)
{
    return xTaskGetTickCount();  // FreeRTOS tick (genelde 1ms)
}

/* LVGL task */
void lvgl_task(void *pvParameters)
{
    while(1) {
        lv_timer_handler();  // LVGL timer processing
        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms sleep
    }
}

/* main() içinde */
lv_init();
lv_tick_set_cb(my_tick_get_cb);

xTaskCreate(lvgl_task, "LVGL", 4096, NULL, 1, NULL);
vTaskStartScheduler();
```

---

## 6. İlk UI Oluşturma

### 6.1 Main Loop Setup

```c
// main.c

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

int main(void)
{
    /* Hardware init */
    HAL_Init();
    SystemClock_Config();

    /* LVGL init */
    lv_init();
    lv_tick_set_cb(my_tick_get_cb);

    /* HAL init */
    lv_port_disp_init();
    lv_port_indev_init();

    /* Create UI */
    ui_init();

    /* Main loop */
    while(1) {
        lv_timer_handler();  // LVGL task handler
        HAL_Delay(5);        // 5ms delay (~200 Hz)
    }
}
```

**ÖNEMLI**: `lv_timer_handler()` düzenli olarak çağrılmalı (5-10ms intervals).

### 6.2 Basit "Hello World" UI

```c
// ui.c

#include "lvgl.h"

void ui_init(void)
{
    /* Create a label */
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello LVGL!");
    lv_obj_center(label);

    /* Create a button */
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);

    /* Add label to button */
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me");
    lv_obj_center(btn_label);

    /* Add event callback */
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
}

static void btn_event_cb(lv_event_t * e)
{
    LV_LOG_USER("Button clicked!");
}
```

### 6.3 Daha Karmaşık Örnek

```c
void ui_init(void)
{
    /* Set background color */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x003a57), LV_PART_MAIN);

    /* Create a container */
    lv_obj_t * cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, 280, 180);
    lv_obj_center(cont);

    /* Title label */
    lv_obj_t * title = lv_label_create(cont);
    lv_label_set_text(title, "LVGL Demo");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    /* Slider */
    lv_obj_t * slider = lv_slider_create(cont);
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, -10);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Value label */
    lv_obj_t * value_label = lv_label_create(cont);
    lv_label_set_text(value_label, "Value: 50");
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, 30);

    /* Store label reference for update */
    lv_obj_set_user_data(slider, value_label);
}

static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_user_data(slider);

    int32_t value = lv_slider_get_value(slider);

    char buf[16];
    lv_snprintf(buf, sizeof(buf), "Value: %d", value);
    lv_label_set_text(label, buf);
}
```

---

## 7. Build System Entegrasyonu

### 7.1 Makefile

```makefile
# Makefile example

# Compiler
CC = arm-none-eabi-gcc

# LVGL source files
LVGL_DIR = lvgl
LVGL_SRCS = $(shell find $(LVGL_DIR)/src -name '*.c')

# Project source files
SRCS = src/main.c \
       src/lv_port_disp.c \
       src/lv_port_indev.c \
       src/ui.c \
       $(LVGL_SRCS)

# Include paths
INCLUDES = -Iinclude \
           -I$(LVGL_DIR) \
           -I$(LVGL_DIR)/src

# Compiler flags
CFLAGS = -mcpu=cortex-m4 -mthumb -O2 -g \
         -Wall -fdata-sections -ffunction-sections \
         $(INCLUDES)

# Linker flags
LDFLAGS = -Wl,--gc-sections

# Output
TARGET = firmware.elf

# Build
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
```

### 7.2 CMake

```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.13)
project(lvgl_project C)

# LVGL library
add_subdirectory(lvgl)

# Project sources
add_executable(firmware
    src/main.c
    src/lv_port_disp.c
    src/lv_port_indev.c
    src/ui.c
)

# Include directories
target_include_directories(firmware PRIVATE
    include
    lvgl
)

# Link LVGL
target_link_libraries(firmware PRIVATE lvgl)

# Compiler options
target_compile_options(firmware PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -O2
    -Wall
)
```

**LVGL CMakeLists.txt** (lvgl dizininde oluştur):

```cmake
# lvgl/CMakeLists.txt

file(GLOB_RECURSE SOURCES
    src/*.c
)

add_library(lvgl STATIC ${SOURCES})

target_include_directories(lvgl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

### 7.3 STM32CubeIDE

1. **LVGL Klasörünü Ekle**:
   - Right-click project → Properties
   - C/C++ Build → Settings → Tool Settings
   - MCU GCC Compiler → Include paths → Add: `${workspace_loc:/${ProjName}/lvgl}`

2. **Source Folders Ekle**:
   - Right-click project → Properties
   - C/C++ General → Paths and Symbols
   - Source Location → Add Folder: `lvgl/src`

3. **Exclude Unnecessary Files** (opsiyonel):
   - Right-click `lvgl/examples` → Resource Configurations → Exclude from Build

4. **Increase Heap/Stack**:
   - Open `.ld` linker script
   - Increase `_Min_Heap_Size` to 0x8000 (32 KB)
   - Increase `_Min_Stack_Size` to 0x2000 (8 KB)

---

## 8. Debugging ve Troubleshooting

### 8.1 Yaygın Hatalar

#### Hata 1: Compile Error - "lv_conf.h not found"

**Nedeni**: Include path eksik veya lv_conf.h yanlış yerde.

**Çözüm**:
```bash
# lv_conf.h'yi include/ dizinine koy
# Compiler include path ekle: -Iinclude
```

#### Hata 2: Compile Error - "lv_conf.h is disabled"

**Nedeni**: `#if 0` değiştirilmemiş.

**Çözüm**:
```c
// lv_conf.h ilk satır
#if 1  // 0'dan 1'e değiştir
```

#### Hata 3: Ekranda Hiçbir Şey Gözükmüyor

**Olası nedenler**:
1. Display driver flush callback çağrılmıyor
2. `lv_disp_flush_ready()` çağrılmamış
3. `lv_timer_handler()` çağrılmıyor
4. Display initialization hatalı

**Debug**:
```c
/* lv_port_disp.c içine ekle */
static void disp_flush(...)
{
    printf("Flush called: x1=%d, y1=%d, x2=%d, y2=%d\n",
           area->x1, area->y1, area->x2, area->y2);

    /* ... transfer ... */

    lv_disp_flush_ready(disp_drv);  // Mutlaka çağır!
}
```

#### Hata 4: Touch Çalışmıyor

**Debug**:
```c
static void touchpad_read(...)
{
    bool touched = touchpad_is_touched();
    if(touched) {
        int16_t x, y;
        touchpad_get_xy(&x, &y);
        printf("Touch: x=%d, y=%d\n", x, y);
    }
    /* ... */
}
```

#### Hata 5: Crash / Hard Fault

**Olası nedenler**:
1. Stack overflow (artır!)
2. Heap overflow (LV_MEM_SIZE artır)
3. Unaligned memory access
4. NULL pointer dereference

**Debug**:
- Enable `LV_USE_LOG`
- Enable `LV_USE_MEM_MONITOR`
- Check stack/heap size

```c
/* Memory monitoring */
lv_mem_monitor_t mon;
lv_mem_monitor(&mon);
printf("Heap: %d / %d bytes, frag: %.1f%%\n",
       mon.used_cnt, mon.total_size, mon.frag_pct);
```

### 8.2 Performance Issues

#### Düşük FPS

**Kontrol listesi**:
- [ ] Buffer boyutu yeterli mi? (10-30 satır)
- [ ] DMA kullanılıyor mu?
- [ ] GPU enabled mı? (STM32 DMA2D, etc.)
- [ ] CPU clock yeterli mi?
- [ ] Flush callback blocking mu?

**Ölçüm**:
```c
/* lv_conf.h */
#define LV_USE_PERF_MONITOR 1

/* main.c */
lv_obj_t * label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "");
lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 5);

while(1) {
    uint32_t start = HAL_GetTick();
    lv_timer_handler();
    uint32_t end = HAL_GetTick();

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "Frame: %lu ms", end - start);
    lv_label_set_text(label, buf);

    HAL_Delay(5);
}
```

#### Flicker / Tearing

**Neden**: Single buffer + yavaş flush.

**Çözüm**: Double buffering enable et:
```c
static lv_color_t buf_1[...];
static lv_color_t buf_2[...];  // Add second buffer

lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, ...);
```

---

## 9. Platform-Specific Örnekler

### 9.1 STM32 + ILI9341 (SPI)

**Hardware**:
- MCU: STM32F407VG (168 MHz, 192 KB RAM)
- Display: ILI9341 320x240 RGB565 (SPI)
- Touch: XPT2046 (SPI)

**Konfigürasyon**:
```c
// lv_conf.h
#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240
#define LV_COLOR_DEPTH 16
#define LV_MEM_SIZE (64U * 1024U)
#define LV_USE_GPU_STM32_DMA2D 1
```

**Display Driver** (özet):
```c
void disp_init(void)
{
    /* SPI1: 42 MHz, 8-bit */
    hspi1.Instance = SPI1;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    HAL_SPI_Init(&hspi1);

    /* ILI9341 init sequence */
    // ... (manufacturer datasheet)
}

static void disp_flush(...)
{
    disp_set_window(area->x1, area->y1, area->x2, area->y2);
    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)color_p, size);
    // lv_disp_flush_ready() called in DMA callback
}
```

**Performance**: ~30 FPS (complex UI)

### 9.2 ESP32 + ST7789 (SPI)

**Hardware**:
- MCU: ESP32 (240 MHz, 520 KB RAM)
- Display: ST7789 240x240 RGB565 (SPI)
- Touch: Capacitive (I2C)

**ESP-IDF Component**:

Existing component kullan: **lvgl_esp32_drivers**

```bash
cd components
git clone https://github.com/lvgl/lvgl_esp32_drivers.git
```

**sdkconfig**:
```
CONFIG_LV_HOR_RES_MAX=240
CONFIG_LV_VER_RES_MAX=240
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_DISP_ST7789=y
CONFIG_LV_DISP_SPI_MOSI=23
CONFIG_LV_DISP_SPI_CLK=18
```

**main.c**:
```c
#include "lvgl.h"
#include "lvgl_helpers.h"

void app_main(void)
{
    lv_init();

    lvgl_driver_init();  // ESP32 helper

    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = disp_driver_flush;  // From lvgl_helpers
    lv_disp_drv_register(&disp_drv);

    /* UI */
    ui_init();

    /* LVGL task */
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
    }
}
```

### 9.3 Raspberry Pi Pico + SSD1306 (I2C OLED)

**Hardware**:
- MCU: RP2040 (Pico)
- Display: SSD1306 128x64 Monochrome OLED (I2C)

**lv_conf.h**:
```c
#define LV_HOR_RES_MAX 128
#define LV_VER_RES_MAX 64
#define LV_COLOR_DEPTH 1  // Monochrome
#define LV_MEM_SIZE (16U * 1024U)
```

**Display Driver**:
```c
#define SSD1306_I2C_ADDR 0x3C

void ssd1306_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};  // 0x00 = command
    i2c_write_blocking(i2c0, SSD1306_I2C_ADDR, buf, 2, false);
}

void disp_init(void)
{
    ssd1306_cmd(0xAE);  // Display OFF
    ssd1306_cmd(0xD5);  // Set clock
    ssd1306_cmd(0x80);
    // ... (initialization sequence)
    ssd1306_cmd(0xAF);  // Display ON
}

static void disp_flush(...)
{
    uint8_t row_start = area->y1 / 8;
    uint8_t row_end = area->y2 / 8;

    for(uint8_t row = row_start; row <= row_end; row++) {
        ssd1306_cmd(0xB0 + row);  // Set page
        ssd1306_cmd(0x00 + (area->x1 & 0x0F));  // Set column low
        ssd1306_cmd(0x10 + (area->x1 >> 4));     // Set column high

        /* Convert color_p to byte array and send */
        // ...
    }

    lv_disp_flush_ready(disp_drv);
}
```

---

## 10. Best Practices

### 10.1 Genel Tavsiyeler

✅ **DO**:
- Version control kullan (git submodule)
- Double buffering + DMA kullan (performance)
- Partial buffering kullan (RAM tasarrufu)
- GPU acceleration enable et (varsa)
- Performance monitoring enable et (development)
- Logging enable et (debugging)
- RTOS kullan (complex projeler için)

❌ **DON'T**:
- LVGL kaynak kodunu modify etme (fork olarak tut)
- lv_timer_handler()'ı interrupt içinde çağırma
- Blocking operations display flush'ta yapma
- Her frame full redraw yapma (partial invalidation kullan)
- Çok fazla font/image enable etme (Flash tasarrufu)

### 10.2 Memory Optimization

**Stack Size**:
```
Minimum: 4 KB
Önerilen: 8 KB
Complex UI: 16 KB
```

**Heap Size** (LVGL_MEM_SIZE):
```
Simple UI: 16-32 KB
Medium UI: 32-64 KB
Complex UI: 64-128 KB
```

**Buffer Size**:
```
Minimum: 5-10 satır (tight RAM)
Optimal: 10-30 satır (best balance)
Maximum: Full screen (high RAM, best FPS)
```

### 10.3 Performance Optimization

1. **Enable GPU**: STM32 DMA2D, NXP PXP, etc.
2. **DMA Transfers**: Non-blocking SPI/I2C
3. **Partial Buffering**: 10-30 satır
4. **Double Buffering**: CPU/DMA parallelism
5. **Dirty Area**: Sadece değişen alanları redraw
6. **SIMD**: Cortex-M4+ için enable (LV_USE_SIMD)
7. **Cache Optimization**: Aligned structs

### 10.4 Code Organization

```
project/
├── src/
│   ├── main.c              # Entry point
│   ├── hal/                # Hardware drivers
│   │   ├── lv_port_disp.c
│   │   └── lv_port_indev.c
│   └── ui/                 # UI code
│       ├── ui_screens.c    # Screen definitions
│       ├── ui_widgets.c    # Custom widgets
│       └── ui_events.c     # Event handlers
├── include/
│   ├── lv_conf.h           # LVGL config
│   └── ui/
│       └── ui.h
└── lvgl/                   # LVGL library (submodule)
```

### 10.5 Version Control

**.gitignore**:
```
# LVGL build artifacts
lvgl/build/
*.o
*.elf
*.bin
*.hex

# IDE files
.vscode/
.settings/
Debug/
Release/
```

**Git submodule update**:
```bash
# Update LVGL to latest
cd lvgl
git checkout release/v8.3
git pull

# Update in parent repo
cd ..
git add lvgl
git commit -m "Update LVGL to v8.3.11"
```

---

## 11. İleri Seviye Konular

### 11.1 Custom Fonts

1. **Font convert**: https://lvgl.io/tools/fontconverter
2. Font file (.c) projeye ekle
3. lv_conf.h'de declare et:

```c
// lv_conf.h
LV_FONT_DECLARE(my_font_20)

/* main.c'de kullan */
lv_obj_set_style_text_font(label, &my_font_20, 0);
```

### 11.2 Custom Themes

```c
static void apply_dark_theme(lv_theme_t * th, lv_obj_t * obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), 0);
}

lv_theme_t * my_theme = lv_theme_basic_init(lv_disp_get_default());
my_theme->apply_cb = apply_dark_theme;
lv_disp_set_theme(NULL, my_theme);
```

### 11.3 Custom Widgets

Widget oluşturma template'i için:
- `lvgl/docs/widgets.md` - Widget development guide
- `design_and_documents/demos/widget_system/` - Bu projedeki demo

### 11.4 SquareLine Studio Entegrasyonu

**SquareLine Studio**: LVGL için drag-drop UI designer.

1. SquareLine Studio'da UI tasarla
2. Export code (C files)
3. Projeye ekle
4. `ui_init()` çağır

---

## 12. Troubleshooting Checklist

Sorun yaşıyorsanız bu checklist'i takip edin:

### Compile Errors
- [ ] lv_conf.h exist ve `#if 1`?
- [ ] Include paths doğru?
- [ ] LVGL src/ folder added to build?
- [ ] C99 veya üzeri?

### Display Issues
- [ ] Display driver test edildi mi? (test pattern)
- [ ] `disp_flush()` implement edildi mi?
- [ ] `lv_disp_flush_ready()` çağrılıyor mu?
- [ ] `lv_timer_handler()` main loop'ta mı?
- [ ] Buffer boyutu yeterli mi?

### Touch Issues
- [ ] Touch controller test edildi mi? (raw coordinates)
- [ ] `touchpad_read()` implement edildi mi?
- [ ] Coordinates doğru scale ediliyor mu?
- [ ] Calibration gerekli mi?

### Performance Issues
- [ ] FPS ne kadar? (target: 20+ FPS)
- [ ] DMA kullanılıyor mu?
- [ ] Double buffering var mı?
- [ ] CPU clock yeterli mi?
- [ ] Dirty area tracking aktif mi?

### Memory Issues
- [ ] Heap size yeterli mi? (LV_MEM_SIZE)
- [ ] Stack overflow var mı?
- [ ] Fragmentation yüksek mi? (<10% olmalı)
- [ ] Gereksiz font/image var mı?

---

## 13. Kaynaklar ve Daha Fazlası

### Resmi Kaynaklar
- **Website**: https://lvgl.io
- **Documentation**: https://docs.lvgl.io
- **GitHub**: https://github.com/lvgl/lvgl
- **Forum**: https://forum.lvgl.io
- **Blog**: https://blog.lvgl.io

### Bu Projedeki Kaynaklar
- `design_and_documents/docs/LVGL_WIDGET_ARCHITECTURE.md` - Widget yapısı
- `design_and_documents/docs/LVGL_RENDER_PIPELINE_ARCHITECTURE.md` - Render pipeline
- `design_and_documents/docs/LVGL_PERFORMANCE_OPTIMIZATION.md` - Performans optimizasyonu
- `design_and_documents/demos/` - Standalone demo'lar

### Araçlar
- **SquareLine Studio**: https://squareline.io - UI designer
- **Font Converter**: https://lvgl.io/tools/fontconverter
- **Image Converter**: https://lvgl.io/tools/imageconverter

### Community
- **Discord**: https://chat.lvgl.io
- **YouTube**: LVGL official channel
- **Twitter**: @lvgl_official

---

## 14. Örnek Proje Yapısı (Özet)

```
your_project/
├── CMakeLists.txt (veya Makefile)
├── .gitignore
├── .gitmodules (submodule için)
│
├── src/
│   ├── main.c                   # Entry point
│   ├── hal/
│   │   ├── lv_port_disp.c       # Display driver
│   │   └── lv_port_indev.c      # Touch driver
│   └── ui/
│       ├── ui_init.c            # UI initialization
│       └── ui_screens.c         # Screen definitions
│
├── include/
│   ├── lv_conf.h                # LVGL config (CRITICAL!)
│   └── ui/
│       └── ui.h
│
├── lvgl/                        # Git submodule
│   ├── src/
│   ├── examples/
│   └── lv_conf_template.h
│
└── build/                       # Build output
```

### Minimal main.c Template

```c
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui/ui.h"

/* Tick callback */
uint32_t my_tick_get_cb(void) {
    return HAL_GetTick();  // Platform-specific
}

int main(void)
{
    /* 1. Hardware init */
    HAL_Init();
    SystemClock_Config();

    /* 2. LVGL init */
    lv_init();
    lv_tick_set_cb(my_tick_get_cb);

    /* 3. HAL init */
    lv_port_disp_init();
    lv_port_indev_init();

    /* 4. UI init */
    ui_init();

    /* 5. Main loop */
    while(1) {
        lv_timer_handler();
        HAL_Delay(5);  // 5ms = ~200 Hz
    }
}
```

---

## 15. Hızlı Start Komutu

```bash
# 1. Clone project
git clone https://github.com/your/project.git
cd project

# 2. Add LVGL submodule
git submodule add https://github.com/lvgl/lvgl.git
git submodule update --init --recursive

# 3. Copy config template
cp lvgl/lv_conf_template.h include/lv_conf.h

# 4. Edit lv_conf.h
# - #if 0 → #if 1
# - LV_HOR_RES_MAX, LV_VER_RES_MAX ayarla
# - LV_COLOR_DEPTH ayarla (16 önerilen)
# - LV_MEM_SIZE ayarla (32 KB başlangıç)

# 5. Copy porting templates
cp lvgl/examples/porting/lv_port_disp_template.c src/lv_port_disp.c
cp lvgl/examples/porting/lv_port_indev_template.c src/lv_port_indev.c

# 6. Implement HAL functions
# - disp_flush()
# - touchpad_read()
# - tick callback

# 7. Create UI
# - ui_init()

# 8. Build and run!
make
```

---

**Başarılar! 🚀**

Bu döküman LVGL'yi projenize entegre etmek için ihtiyacınız olan her şeyi içerir. Sorunlarla karşılaşırsanız:
1. Bu dökümanın **Troubleshooting** bölümüne bakın
2. LVGL forum'a sorun: https://forum.lvgl.io
3. GitHub issues kontrol edin: https://github.com/lvgl/lvgl/issues

---

**Versiyon**: 1.0
**Tarih**: 2025-11-18
**Yazar**: LVGL Integration Guide
**Lisans**: MIT
