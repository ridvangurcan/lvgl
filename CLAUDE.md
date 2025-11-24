# CLAUDE.md - LVGL Developer Guide for AI Assistants

**Version:** 9.4.0
**Last Updated:** 2025-11-24
**Repository:** https://github.com/lvgl/lvgl

---

## Table of Contents

1. [Repository Overview](#repository-overview)
2. [Codebase Structure](#codebase-structure)
3. [Key Architectural Patterns](#key-architectural-patterns)
4. [File Naming Conventions](#file-naming-conventions)
5. [Development Workflow](#development-workflow)
6. [Build System](#build-system)
7. [Configuration System](#configuration-system)
8. [Testing Infrastructure](#testing-infrastructure)
9. [Code Style and Quality](#code-style-and-quality)
10. [Contributing Guidelines](#contributing-guidelines)
11. [Important Considerations for AI Assistants](#important-considerations-for-ai-assistants)

---

## Repository Overview

**LVGL (Light and Versatile Graphics Library)** is a free and open-source UI library written in C (C++ compatible) that enables creation of graphical user interfaces for MCUs and MPUs on any platform.

### Key Characteristics

- **Language:** Pure C with C++ compatibility
- **License:** MIT
- **Dependencies:** None (fully portable)
- **Requirements:** ~32KB RAM, ~128KB Flash minimum
- **Platforms:** MCUs, MPUs, Desktop (Windows/Linux/macOS), Mobile
- **OS Support:** Bare metal, FreeRTOS, Zephyr, RT-Thread, NuttX, QNX, MQX, POSIX
- **Package Managers:** Arduino, PlatformIO, ESP-IDF, Zephyr, CMSIS-Pack

### Project Goals

1. **Portability:** Compile for any platform without external dependencies
2. **Efficiency:** Minimal resource usage for embedded systems
3. **Flexibility:** Modular design with conditional compilation
4. **Rich Features:** 30+ widgets, hardware acceleration, 3D support

---

## Codebase Structure

### Root Directory Layout

```
/home/user/lvgl/
├── src/                    # Main source code (19 major modules)
├── examples/               # 100+ code examples
├── demos/                  # Demo applications
├── tests/                  # Test suite (unit, performance, visual)
├── docs/                   # Documentation source (Doxygen)
├── scripts/                # Build and utility scripts
├── env_support/            # Platform-specific support (CMake, esp, etc.)
├── configs/                # Configuration examples
├── libs/                   # External library integrations
├── .github/workflows/      # CI/CD workflows (26+ workflows)
├── zephyr/                 # Zephyr OS integration
├── xmls/                   # XML schema files
├── CMakeLists.txt          # CMake build configuration
├── Kconfig                 # Kconfig configuration (51KB)
├── lv_conf_template.h      # Configuration template (copy as lv_conf.h)
├── lvgl.h                  # Main public header (includes all modules)
└── lvgl_private.h          # Private internal header
```

### Source Code Organization (`/src`)

The source code is divided into **19 major modules**:

#### 1. **core/** - Object System & Event Handling
- `lv_obj.*` - Base object implementation
- `lv_obj_class.*` - OOP-style class system
- `lv_obj_tree.*` - Object hierarchy management
- `lv_obj_pos.*`, `lv_obj_scroll.*`, `lv_obj_style.*`, `lv_obj_draw.*`
- `lv_obj_event.*` - Event system
- `lv_refr.*` - Rendering/refresh logic
- `lv_observer.*` - Data binding (observer pattern)
- `lv_group.*` - Focus/group management

#### 2. **draw/** - Rendering System
- **Core:** `lv_draw.*`, `lv_draw_buf.*`, `lv_image_decoder.*`
- **Primitives:** `lv_draw_rect.*`, `lv_draw_arc.*`, `lv_draw_line.*`, `lv_draw_label.*`, `lv_draw_image.*`
- **Backends:** `sw/` (software), `sdl/`, `vg_lite/`, `opengles/`, `dma2d/`, `nema_gfx/`, `nxp/`, `renesas/`, `espressif/`, `eve/`
- **Advanced:** `lv_draw_3d.*`, `lv_draw_vector.*`, `lv_draw_mask.*`

#### 3. **widgets/** - 39 UI Widgets
Each widget in its own subdirectory with pattern:
- `lv_[widget].h` - Public API
- `lv_[widget].c` - Implementation
- `lv_[widget]_private.h` - Internal structures

**Common widgets:** `button/`, `label/`, `image/`, `slider/`, `bar/`, `checkbox/`, `switch/`, `dropdown/`, `textarea/`, `chart/`, `table/`, `calendar/`, `keyboard/`, `list/`, `menu/`, `tabview/`, `win/`, etc.

#### 4. **misc/** - Utilities & Data Structures
- **Data structures:** `lv_ll.*` (linked list), `lv_rb.*` (red-black tree), `lv_array.*`, `lv_circle_buf.*`
- **Core utilities:** `lv_timer.*`, `lv_event.*`, `lv_anim.*` (animation engine)
- **Graphics:** `lv_color.*`, `lv_area.*`, `lv_style.*` (~100 style properties), `lv_matrix.*`
- **Text:** `lv_text.*`, `lv_bidi.*` (bidirectional text)
- **System:** `lv_log.*`, `lv_fs.*` (filesystem), `lv_math.*`
- **Cache:** `cache/` - Caching system (LRU, etc.)

#### 5. **font/** - Font System
- `lv_font.*` - Font abstraction
- Built-in fonts: `lv_font_montserrat_*.c` (8-48pt), `lv_font_unscii_*.c`
- CJK support: `lv_font_source_han_sans_sc_*_cjk.c`
- Font loaders: `fmt_txt/`, `binfont_loader/`, `imgfont/`, `font_manager/`

#### 6. **layouts/** - Layout Engines
- `flex/` - Flexbox-like layout
- `grid/` - CSS Grid-like layout

#### 7. **display/** - Display Management
- `lv_display.*` - Display device abstraction

#### 8. **indev/** - Input Device Handling
- `lv_indev.*` - Input device abstraction
- `lv_indev_gesture.*` - Gesture recognition
- `lv_gridnav.*` - Grid navigation

#### 9. **themes/** - UI Themes
- `default/`, `simple/`, `mono/` - Pre-built themes

#### 10. **libs/** - External Library Integrations (27 modules)
**Image formats:** `bmp/`, `gif/`, `libpng/`, `libjpeg_turbo/`, `libwebp/`, `lodepng/`, `tjpgd/`
**Fonts:** `freetype/`, `tiny_ttf/`
**Vector graphics:** `svg/`, `thorvg/`, `rlottie/`, `barcode/`, `qrcode/`
**3D:** `gltf/`
**Media:** `ffmpeg/`, `gstreamer/`
**Compression:** `rle/`, `lz4/`, `bin_decoder/`
**Filesystem:** `fsdrv/`, `frogfs/`
**Hardware:** `FT800-FT813/`, `nema_gfx/`, `vg_lite_driver/`

#### 11. **drivers/** - Platform-Specific Drivers
**Display/Draw:** `display/`, `draw/`
**Platform:** `sdl/`, `x11/`, `wayland/`, `windows/`, `opengles/`, `evdev/`, `libinput/`, `qnx/`, `nuttx/`, `uefi/`

#### 12. **osal/** - OS Abstraction Layer
- `lv_os.*` - OS abstraction API
- Implementations: `lv_os_none.*`, `lv_pthread.*`, `lv_freertos.*`, `lv_cmsis_rtos2.*`, `lv_rtthread.*`, `lv_windows.*`, `lv_mqx.*`

#### 13. **stdlib/** - Standard Library Wrappers
- `lv_mem.*`, `lv_string.*`, `lv_sprintf.*`
- Implementations: `builtin/` (LVGL's own), `clib/`, `micropython/`, `rtthread/`, `uefi/`

#### 14. **tick/** - Timing System
- `lv_tick.*` - Time tracking for animations and timers

#### 15. **xml/** - XML UI System (LVGL Pro)
- `lv_xml.*` - Runtime XML UI loading
- `lv_xml_parser.*`, `lv_xml_component.*`, `lv_xml_style.*`, `lv_xml_translation.*`
- Supports LVGL Pro Editor format

#### 16. **debugging/** - Development Tools
- `sysmon/` - System monitor widget
- `monkey/` - Monkey testing (random input)
- `test/` - Test framework

#### 17. **others/** - Additional Features
- `file_explorer/` - File browser widget
- `fragment/` - Fragment system (UI state management)
- `translation/` - Multi-language support

---

## Key Architectural Patterns

### 1. Object-Oriented Design in C

LVGL implements OOP using C structs and function pointers:

```c
// Class definition
lv_obj_class_t lv_button_class = {
    .constructor_cb = lv_button_constructor,
    .base_class = &lv_obj_class,        // Inheritance
    .name = "lv_button",
    .instance_size = sizeof(lv_button_t),
};

// Object creation
lv_obj_t * lv_button_create(lv_obj_t * parent) {
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}
```

**Key Concepts:**
- All widgets inherit from `lv_obj_t` base class
- Virtual methods via function pointers in `lv_obj_class_t`
- Constructor callbacks for initialization
- Type hierarchy maintained at runtime

### 2. Event System

Event-driven architecture with bubbling, trickling, and capture modes:

```c
lv_obj_add_event_cb(obj, event_handler, LV_EVENT_CLICKED, user_data);
```

- Events propagate through object hierarchy
- Multiple callbacks per object
- Event filtering and stopping propagation

### 3. Style System

CSS-like styling with ~100 properties:

```c
lv_style_t style;
lv_style_init(&style);
lv_style_set_bg_color(&style, lv_color_hex(0xff8800));
lv_obj_add_style(obj, &style, LV_PART_MAIN | LV_STATE_DEFAULT);
```

- Styles applied to object parts and states
- Style inheritance from parent to child
- Transitions and animations supported

### 4. Observer Pattern (Data Binding)

Two-way data binding for automatic UI updates:

```c
lv_subject_t subject;
lv_subject_init_int(&subject, 42);
lv_slider_bind_value(slider, &subject);
lv_label_bind_text(label, &subject, "Value: %d");
```

### 5. Task-Based Rendering

Efficient rendering with hardware acceleration support:
- Draw tasks queued for rendering
- Layer-based rendering system
- Multiple backend support (software, GPU, DMA)
- Partial rendering support

### 6. Conditional Compilation

Features selectively enabled via configuration:

```c
#if LV_USE_BUTTON
    // Button implementation
#endif
```

This allows compiling out unused features to save memory.

---

## File Naming Conventions

### Headers (.h)
- **Public API:** `lv_[module].h` (e.g., `lv_button.h`)
- **Private API:** `lv_[module]_private.h` (e.g., `lv_button_private.h`)
- **Generated:** `lv_[module]_gen.h` (e.g., `lv_obj_style_gen.h`)

### Implementation (.c)
- **Main Implementation:** `lv_[module].c`
- Pattern: All LVGL files prefixed with `lv_`

### Configuration
- **User config:** `lv_conf.h` (copied from `lv_conf_template.h`)
- **Template:** `lv_conf_template.h` (51KB reference)
- **Internal:** `lv_conf_internal.h` (auto-generated)
- **Kconfig:** `lv_conf_kconfig.h` (generated from Kconfig)

### Standard File Structure

Every module follows this pattern:

```c
/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
#if LV_USE_[MODULE]  // Feature guard

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**********************
 * STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_[MODULE]*/
```

---

## Development Workflow

### Setting Up Development Environment

1. **Clone the repository:**
   ```bash
   git clone https://github.com/lvgl/lvgl.git
   cd lvgl
   ```

2. **Install prerequisites:**
   ```bash
   scripts/install-prerequisites.sh
   ```

3. **Configure LVGL:**
   ```bash
   cp lv_conf_template.h lv_conf.h
   # Edit lv_conf.h: Change #if 0 to #if 1 on line 15
   ```

### Making Changes

1. **Read before writing:**
   - Always read existing code before modifying
   - Understand the module's architecture
   - Check for related files (`*_private.h`, tests, examples)

2. **Follow the patterns:**
   - Use existing widgets/modules as templates
   - Maintain consistent naming conventions
   - Add feature guards (`#if LV_USE_[FEATURE]`)

3. **Update documentation:**
   - Add Doxygen comments for public APIs
   - Update examples if APIs change
   - Document breaking changes

4. **Add tests:**
   - Unit tests in `tests/src/test_cases/`
   - Screenshot tests for visual validation
   - Performance tests if relevant

5. **Update configuration:**
   - If adding new feature, update `lv_conf_template.h`
   - Run `scripts/lv_conf_internal_gen.py`
   - Update `Kconfig` for embedded systems

### Pre-commit Hooks

LVGL uses pre-commit hooks (`.pre-commit-config.yaml`):

1. **Code formatting:** Uses AStyle v3.4.12
   ```bash
   cd scripts
   ./install_astyle.sh
   ```

2. **Template checking:** Validates file template conformance

3. **Spell checking:** Via typos tool

**Run formatting manually:**
```bash
python scripts/code-format.py
```

### Pull Request Process

1. **Create feature branch:**
   ```bash
   git checkout -b feature/my-feature
   ```

2. **Make changes and commit:**
   ```bash
   git add .
   git commit -m "feat(module): Add new feature"
   ```

3. **Run tests:**
   ```bash
   ./tests/main.py test
   ```

4. **Format code:**
   ```bash
   python scripts/code-format.py
   ```

5. **Push and create PR:**
   - Mark as Draft initially
   - Fill out PR template
   - Link related issues with `Fixes #xxxx`
   - Mark as Ready when complete

6. **Required PR checklist:**
   - [ ] Documentation updated
   - [ ] Examples added if relevant
   - [ ] Tests added if applicable
   - [ ] `lv_conf_template.h` and `Kconfig` updated if new options added
   - [ ] Code formatted with `scripts/code-format.py`
   - [ ] No trailing whitespace

---

## Build System

LVGL supports multiple build systems:

### 1. CMake (Primary)

```bash
mkdir build && cd build
cmake ..
make
```

**Key files:**
- `CMakeLists.txt` - Root configuration
- `CMakePresets.json` - CMake presets
- `env_support/cmake/` - Platform-specific setup
  - `esp.cmake` - ESP-IDF support
  - `micropython.cmake` - MicroPython
  - `os_desktop.cmake` - Desktop platforms

### 2. Kconfig (Embedded)

For systems using Kconfig (Zephyr, ESP-IDF):
- `Kconfig` - Configuration tree (51KB)
- Generated as `lv_conf_kconfig.h`

### 3. Make/Makefile

```bash
make -f lvgl.mk
```

- `lvgl.mk` - Makefile integration
- `component.mk` - ESP-IDF component makefile

### 4. Package Managers

- **Arduino:** `library.properties`
- **PlatformIO:** `library.json`
- **ESP-IDF:** `idf_component.yml`
- **Zephyr:** Native integration

### Platform-Specific Setup

**ESP-IDF (ESP32):**
```bash
idf.py add-dependency "lvgl/lvgl^9.4.0"
```

**Arduino:**
```cpp
#include <lvgl.h>
```

**PlatformIO:**
```ini
lib_deps = lvgl/lvgl@^9.4.0
```

---

## Configuration System

LVGL uses a multi-layer configuration system:

### Configuration Layers

1. **User Configuration** (`lv_conf.h`)
   - Copied from `lv_conf_template.h`
   - Change `#if 0` to `#if 1` on line 15
   - Customize as needed

2. **Kconfig Configuration** (`Kconfig`)
   - For embedded systems using Kconfig
   - Generated as `lv_conf_kconfig.h`

3. **Internal Configuration** (`lv_conf_internal.h`)
   - Auto-generated file combining all sources
   - Generated by `scripts/lv_conf_internal_gen.py`

### Key Configuration Categories

**Color & Display:**
```c
#define LV_COLOR_DEPTH 16              // 1, 8, 16, 24, 32
#define LV_DPI_DEF 130                 // Dots per inch
```

**Memory:**
```c
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (64 * 1024U)       // Memory pool size
```

**Operating System:**
```c
#define LV_USE_OS LV_OS_NONE           // LV_OS_FREERTOS, LV_OS_PTHREAD, etc.
```

**Feature Toggles:**
```c
#define LV_USE_BUTTON 1
#define LV_USE_LABEL 1
#define LV_USE_SLIDER 1
// ... ~100 LV_USE_* options
```

**Rendering:**
```c
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN 4
```

### Regenerating Configuration

After modifying `lv_conf_template.h` or `Kconfig`:

```bash
python scripts/lv_conf_internal_gen.py
```

---

## Testing Infrastructure

### Test Types

1. **Unit Tests** - `tests/src/test_cases/`
2. **Performance Tests** - `tests/src/test_cases_perf/`
3. **Visual Tests** - Screenshot comparison
4. **Emulated Benchmarks** - ARM emulation via QEMU

### Running Tests Locally

**Prerequisites:**
```bash
scripts/install-prerequisites.sh
```

**Run all tests:**
```bash
./tests/main.py test
```

**Run with coverage:**
```bash
./tests/main.py --clean --report build test
```

**Update reference images:**
```bash
./tests/main.py --update-image test
```

### Docker Testing

**Build test environment:**
```bash
docker build . -f tests/Dockerfile -t lvgl_test_env
```

**Run tests:**
```bash
docker run --rm -it -v $(pwd):/work lvgl_test_env "./tests/main.py"
```

### Performance Testing

**Requirements:** Docker + Linux

```bash
./tests/perf.py test
```

**Emulated benchmarks:**
```bash
./tests/benchmark_emu.py run
```

### Creating New Tests

1. **Create test file:** `tests/src/test_cases/test_<name>.c`
2. **Use template:** Copy from `_test_template.c`
3. **Add assertions:**
   - Standard Unity asserts
   - `TEST_ASSERT_EQUAL_SCREENSHOT("image.png")`
   - `TEST_ASSERT_EQUAL_COLOR(color1, color2)`

4. **Screenshot testing:**
   - First run auto-generates reference image
   - Subsequent runs compare against reference
   - Failures create `*_err.png` diff image

### CI/CD Integration

GitHub Actions automatically runs tests on:
- Push to `master` or `release/v*` branches
- Pull requests
- Multiple platforms and configurations

**Key workflows:** `.github/workflows/`
- `main.yml` - Main test suite
- `ccpp.yml` - C/C++ compilation
- `perf_tests.yml` - Performance tests
- `check_style.yml` - Code style validation

---

## Code Style and Quality

### Code Formatting

**Tool:** AStyle v3.4.12

**Install:**
```bash
cd scripts
./install_astyle.sh
```

**Format code:**
```bash
python scripts/code-format.py
```

**Configuration:** `scripts/code-format.cfg`

### Style Guidelines

1. **Indentation:** 4 spaces (not tabs)
2. **Braces:** On same line for functions, new line for control structures
3. **Naming:**
   - Functions: `lv_module_action()` (e.g., `lv_obj_create()`)
   - Types: `lv_module_t` (e.g., `lv_obj_t`)
   - Enums: `LV_MODULE_VALUE` (e.g., `LV_EVENT_CLICKED`)
   - Macros: `LV_MODULE_MACRO` (e.g., `LV_USE_BUTTON`)

4. **Documentation:** Doxygen format
   ```c
   /**
    * Brief description
    * @param obj pointer to an object
    * @return description of return value
    */
   ```

5. **Comments:**
   - Use `/* */` for multi-line comments
   - Use `//` for single-line comments
   - Explain "why", not "what"

### Static Analysis

**Tools used in CI:**
- `cppcheck` - Static analysis
- `infer` - Facebook's static analyzer
- Pre-commit hooks

**Run locally:**
```bash
scripts/cppcheck_run.sh
scripts/infer_run.sh
```

### Common Conventions

1. **NULL checking:**
   ```c
   LV_ASSERT_NULL(obj);
   if(obj == NULL) return;
   ```

2. **Feature guards:**
   ```c
   #if LV_USE_FEATURE
       // Feature code
   #endif
   ```

3. **Error handling:**
   ```c
   if(error_condition) {
       LV_LOG_ERROR("Error message");
       return LV_RESULT_INVALID;
   }
   ```

4. **Memory management:**
   ```c
   void * ptr = lv_malloc(size);  // Not malloc()
   lv_free(ptr);                   // Not free()
   ```

---

## Contributing Guidelines

### Contribution Types

1. **Bug fixes** - Always welcome
2. **New features** - Discuss in issue first
3. **Performance improvements**
4. **Documentation improvements**
5. **Examples and demos**
6. **Tests**

### Code of Conduct

See `docs/CODE_OF_CONDUCT.md` - Be respectful and professional.

### Legal

- **License:** MIT - Very permissive
- **Copyright:** Retained by contributors
- **CLA:** Not required
- **Third-party code:** Must be compatible with MIT license

### Review Process

1. Maintainer reviews PR
2. CI tests must pass
3. Changes requested if needed
4. Re-request review after updates
5. Merge when approved

### Getting Help

- **Documentation:** https://docs.lvgl.io/
- **Forum:** https://forum.lvgl.io/
- **Discord:** Community chat
- **GitHub Issues:** Bug reports and feature requests

---

## Important Considerations for AI Assistants

### Critical Guidelines

#### 1. Always Read Before Writing

**NEVER** propose changes to code you haven't read. If asked to modify a file:
1. Read the file first with Read tool
2. Understand the context and patterns
3. Check for related files (`*_private.h`, tests)
4. Then make changes

#### 2. Understand the Module System

LVGL is highly modular. When working with any module:
- Check if it's conditionally compiled (`#if LV_USE_*`)
- Look for private headers (`*_private.h`)
- Understand the inheritance hierarchy for widgets
- Check for related examples and tests

#### 3. Maintain Compatibility

- Don't break existing APIs without discussion
- Use deprecation warnings for API changes
- Maintain backward compatibility when possible
- Document breaking changes clearly

#### 4. Memory and Performance

This is embedded systems code:
- Be mindful of memory usage
- Avoid dynamic allocations in hot paths
- Use LVGL's memory functions (`lv_malloc`, not `malloc`)
- Consider stack usage
- Think about MCU constraints (limited RAM/Flash)

#### 5. Feature Guards

Always wrap new features in configuration guards:

```c
#if LV_USE_NEW_FEATURE
    // Implementation
#endif
```

And update `lv_conf_template.h` and `Kconfig`.

#### 6. Platform Independence

- Don't assume specific platforms (no `#ifdef __linux__`)
- Use LVGL's abstraction layers:
  - `lv_malloc/lv_free` instead of `malloc/free`
  - `lv_snprintf` instead of `snprintf`
  - OS abstraction for threading
- Test on multiple platforms if possible

#### 7. Testing is Mandatory

For any significant change:
1. Add unit tests in `tests/src/test_cases/`
2. Add screenshot tests for visual changes
3. Run full test suite before PR
4. Consider performance implications

#### 8. Documentation

Update all three types:
1. **Doxygen comments** in headers (for API docs)
2. **Examples** in `examples/` if relevant
3. **Markdown docs** if needed

#### 9. Code Style

- Run `scripts/code-format.py` before committing
- Follow existing patterns in the module
- Use LVGL naming conventions strictly
- Add proper Doxygen documentation

#### 10. Common Pitfalls to Avoid

**DON'T:**
- Use standard C library directly (use LVGL wrappers)
- Assume 32-bit integers or pointers
- Ignore feature guards
- Mix tabs and spaces (use 4 spaces)
- Add features without configuration option
- Break widget inheritance patterns
- Forget to update `lv_conf_template.h`

**DO:**
- Follow existing widget patterns for new widgets
- Use proper memory functions
- Add comprehensive tests
- Document all public APIs
- Consider embedded constraints
- Test with different `lv_conf.h` configurations

#### 11. Widget Development Pattern

When creating a new widget, follow this structure:

```
/src/widgets/my_widget/
├── lv_my_widget.h          # Public API
├── lv_my_widget.c          # Implementation
└── lv_my_widget_private.h  # Private structures

/examples/widgets/my_widget/
└── lv_example_my_widget_1.c

/tests/src/test_cases/widgets/
└── test_my_widget.c
```

#### 12. Understanding Object Hierarchy

All widgets inherit from `lv_obj_t`:
```
lv_obj_t (base)
  ├── lv_button_t
  │     └── lv_imagebutton_t
  ├── lv_label_t
  ├── lv_image_t
  └── ... (all other widgets)
```

When modifying widgets, understand:
- What base class provides
- What the widget adds
- Virtual methods that can be overridden
- Style parts and states

#### 13. Configuration-Dependent Code

Test with multiple configurations:
- Different color depths (1, 8, 16, 24, 32)
- Different OS settings (bare metal, RTOS)
- Different stdlib options (builtin, clib, custom)
- Features enabled/disabled

#### 14. Working with External Libraries

When integrating external libraries (in `src/libs/`):
- Keep them as separate subdirectories
- Don't modify third-party code directly
- Add wrapper headers if needed
- Update `COPYRIGHTS.md`
- Verify license compatibility (MIT-compatible)

#### 15. Performance Considerations

- Use task-based drawing system efficiently
- Leverage hardware acceleration when available
- Minimize redraws (use dirty areas)
- Avoid blocking operations
- Profile with `lv_profiler` when optimizing

### Quick Reference Commands

```bash
# Setup
cp lv_conf_template.h lv_conf.h
scripts/install-prerequisites.sh

# Development
python scripts/code-format.py        # Format code
./tests/main.py test                 # Run tests
python scripts/lv_conf_internal_gen.py  # Regenerate config

# Testing
./tests/main.py --update-image test  # Update reference images
./tests/perf.py test                 # Performance tests
docker build -f tests/Dockerfile .   # Docker test env

# Static Analysis
scripts/cppcheck_run.sh
scripts/infer_run.sh
```

### File Modification Checklist

When modifying LVGL code:

- [ ] Read existing code first
- [ ] Understand the module's architecture
- [ ] Follow naming conventions
- [ ] Add feature guards if needed
- [ ] Use LVGL's memory/string functions
- [ ] Add Doxygen documentation
- [ ] Create/update tests
- [ ] Add examples if relevant
- [ ] Update `lv_conf_template.h` if needed
- [ ] Update `Kconfig` if needed
- [ ] Run `scripts/code-format.py`
- [ ] Run `./tests/main.py test`
- [ ] Check for memory leaks
- [ ] Test with different configurations
- [ ] Update documentation

---

## Additional Resources

### Official Documentation
- **Main docs:** https://docs.lvgl.io/
- **API reference:** https://docs.lvgl.io/master/api/index.html
- **Examples:** https://docs.lvgl.io/master/examples.html
- **Coding style:** https://docs.lvgl.io/master/CODING_STYLE.html

### Community
- **Forum:** https://forum.lvgl.io/
- **GitHub:** https://github.com/lvgl/lvgl
- **Website:** https://lvgl.io/

### Tools
- **LVGL Pro Editor:** https://pro.lvgl.io/
- **Online Viewer:** https://viewer.lvgl.io/
- **Demos:** https://lvgl.io/demos

### Commercial Services
- Graphics design
- UI implementation
- Consulting and support
- Board certification

**Contact:** https://lvgl.io/#contact

---

## Version History

- **v9.4.0** (Current) - Latest stable release
- **v9.x** - Major redesign with improved modularity
- **v8.x** - Previous stable branch (LTS)

---

**This document was auto-generated for AI assistant use. For human-readable documentation, visit https://docs.lvgl.io/**
