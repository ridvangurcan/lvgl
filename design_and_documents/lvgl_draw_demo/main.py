#!/usr/bin/env python3
"""
LVGL Draw Subsystem Demo
=====================================================
Bu demo LVGL'nin /src/draw modülündeki mimari ve
tasarım desenlerini gösterir.

Özellikler:
- Strategy Pattern: Backend switching (SW ↔ GPU)
- Factory Pattern: Widget creation
- State Pattern: Widget states
- Observer Pattern: Event system
- Dirty region tracking
- Performance metrics
- Interactive UI

Kullanım:
- Space: Backend değiştir (SW ↔ GPU)
- Mouse: Button'larla etkileşim
- ESC: Çıkış
"""

import sys
import pygame
from core.display import DisplayDriver, InvalidAreaManager
from core.draw_context import DrawContext
from core.area import Area, Point
from core.color import Colors
from backends.backend_sw import SoftwareBackend
from backends.backend_gpu import GPUBackend
from widgets.button import Button
from widgets.label import Label, Panel
import time


class LVGLDemo:
    """
    Ana demo uygulaması
    LVGL'nin rendering pipeline'ını simüle eder
    """

    def __init__(self, width=800, height=600):
        self.width = width
        self.height = height

        # Display driver (HAL simulation)
        self.display = DisplayDriver(width, height, "LVGL Draw Subsystem Demo")

        # Dirty region manager
        self.inv_manager = InvalidAreaManager()

        # Draw context (STRATEGY PATTERN)
        self.draw_ctx = DrawContext(
            self.display.draw_buf,
            self.display.buf_area
        )

        # Backend selection (başlangıçta SW)
        self.current_backend = "SW"
        SoftwareBackend.init_context(self.draw_ctx)

        # Root container (widget tree)
        self.root = Panel(0, 0, width, height)
        self.root.bg_color = Colors.WHITE

        # Create UI
        self._create_ui()

        # Performance tracking
        self.frame_times = []
        self.max_frame_history = 60

        # Running flag
        self.running = True

    def _create_ui(self):
        """
        UI oluştur
        FACTORY PATTERN benzeri widget creation
        """
        # Title
        title = Label(20, 20, "LVGL Draw Subsystem Demo", font_size=28)
        title.text_color = Colors.PRIMARY
        self.root.add_child(title)

        # Backend info panel
        info_panel = Panel(20, 60, 360, 200)
        info_panel.bg_color = Color(240, 248, 255)  # Alice blue
        info_panel.radius = 8
        self.root.add_child(info_panel)

        self.backend_label = Label(40, 80, f"Backend: {self.current_backend}", font_size=20)
        self.backend_label.text_color = Colors.BLACK
        info_panel.add_child(self.backend_label)

        self.stats_label = Label(40, 110, "Frame: 0 | FPS: 0", font_size=16)
        info_panel.add_child(self.stats_label)

        self.draw_stats_label = Label(40, 140, "Draws: R:0 L:0 C:0", font_size=16)
        info_panel.add_child(self.draw_stats_label)

        self.perf_label = Label(40, 170, "Render Time: 0ms", font_size=16)
        info_panel.add_child(self.perf_label)

        # Instructions panel
        inst_panel = Panel(20, 280, 360, 140)
        inst_panel.bg_color = Color(255, 250, 240)  # Floral white
        inst_panel.radius = 8
        self.root.add_child(inst_panel)

        inst_title = Label(40, 295, "Kontroller:", font_size=18)
        inst_title.text_color = Colors.DARK_GRAY
        inst_panel.add_child(inst_title)

        inst1 = Label(40, 320, "SPACE: Backend Değiştir", font_size=14)
        inst_panel.add_child(inst1)

        inst2 = Label(40, 345, "Mouse: Button'larla Etkileş", font_size=14)
        inst_panel.add_child(inst2)

        inst3 = Label(40, 370, "ESC: Çıkış", font_size=14)
        inst_panel.add_child(inst3)

        # Demo buttons - STATE PATTERN demonstration
        button_panel = Panel(400, 60, 380, 360)
        button_panel.bg_color = Color(248, 248, 255)  # Ghost white
        button_panel.radius = 8
        self.root.add_child(button_panel)

        # Başlık
        btn_title = Label(420, 80, "Widget Demo (STATE PATTERN)", font_size=18)
        btn_title.text_color = Colors.DARK_GRAY
        button_panel.add_child(btn_title)

        # Demo buttons
        self.demo_buttons = []

        # Button 1: Primary
        btn1 = Button(420, 120, 160, 50, "Primary")
        btn1.colors[0] = Colors.PRIMARY
        btn1.on_event('click', lambda w, d: print(f"[EVENT] {w.text} clicked!"))
        button_panel.add_child(btn1)
        self.demo_buttons.append(btn1)

        # Button 2: Success
        btn2 = Button(600, 120, 160, 50, "Success")
        btn2.colors[0] = Colors.SUCCESS
        btn2.on_event('click', lambda w, d: print(f"[EVENT] {w.text} clicked!"))
        button_panel.add_child(btn2)
        self.demo_buttons.append(btn2)

        # Button 3: Warning
        btn3 = Button(420, 190, 160, 50, "Warning")
        btn3.colors[0] = Colors.WARNING
        btn3.on_event('click', lambda w, d: print(f"[EVENT] {w.text} clicked!"))
        button_panel.add_child(btn3)
        self.demo_buttons.append(btn3)

        # Button 4: Danger
        btn4 = Button(600, 190, 160, 50, "Danger")
        btn4.colors[0] = Colors.DANGER
        btn4.on_event('click', lambda w, d: print(f"[EVENT] {w.text} clicked!"))
        button_panel.add_child(btn4)
        self.demo_buttons.append(btn4)

        # Button 5: Disabled
        btn5 = Button(420, 260, 340, 50, "Disabled Button")
        btn5.colors[0] = Colors.GRAY
        btn5.set_state(2048, True)  # Disable
        btn5.enabled = False
        button_panel.add_child(btn5)
        self.demo_buttons.append(btn5)

        # Drawing techniques demo
        draw_panel = Panel(20, 440, 760, 140)
        draw_panel.bg_color = Color(255, 245, 238)  # Seashell
        draw_panel.radius = 8
        self.root.add_child(draw_panel)

        draw_title = Label(40, 455, "Çizim Teknikleri Demo", font_size=18)
        draw_title.text_color = Colors.DARK_GRAY
        draw_panel.add_child(draw_title)

        # Gradient demo
        self.gradient_btn = Button(40, 490, 140, 60, "Gradient")
        self.gradient_btn.shadow_enabled = True
        draw_panel.add_child(self.gradient_btn)

        # Shadow demo
        self.shadow_btn = Button(200, 490, 140, 60, "Shadow")
        self.shadow_btn.shadow_enabled = True
        self.shadow_btn.colors[0] = Colors.SECONDARY
        draw_panel.add_child(self.shadow_btn)

        # Rounded demo
        self.rounded_btn = Button(360, 490, 140, 60, "Rounded")
        self.rounded_btn.radius = 30  # Çok yuvarlak
        self.rounded_btn.colors[0] = Colors.INFO
        draw_panel.add_child(self.rounded_btn)

        # No shadow demo
        self.noshadow_btn = Button(520, 490, 140, 60, "No Shadow")
        self.noshadow_btn.shadow_enabled = False
        self.noshadow_btn.colors[0] = Colors.SUCCESS
        draw_panel.add_child(self.noshadow_btn)

    def switch_backend(self):
        """
        Backend değiştir (STRATEGY PATTERN demonstration)
        Runtime'da rendering backend'i değiştirme
        """
        if self.current_backend == "SW":
            self.current_backend = "GPU"
            GPUBackend.init_context(self.draw_ctx)
            print("\n[BACKEND SWITCH] Software → GPU Simulated")
        else:
            self.current_backend = "SW"
            SoftwareBackend.init_context(self.draw_ctx)
            print("\n[BACKEND SWITCH] GPU Simulated → Software")

        # UI güncelle
        self.backend_label.set_text(f"Backend: {self.draw_ctx.backend_name}")

        # Full refresh
        self.invalidate_all()

    def invalidate_all(self):
        """Tüm ekranı geçersiz kıl (full refresh)"""
        self.inv_manager.add_area(self.display.buf_area)

    def handle_mouse_motion(self, pos):
        """
        Mouse hareket olayı
        Hover state tracking
        """
        for btn in self.demo_buttons + [self.gradient_btn, self.shadow_btn,
                                         self.rounded_btn, self.noshadow_btn]:
            was_hovered = btn.has_state(16)  # HOVERED state
            is_hovered = btn.is_point_inside(pos[0], pos[1])

            if is_hovered and not was_hovered:
                btn.on_mouse_enter()
                self.inv_manager.add_area(btn.area)
            elif not is_hovered and was_hovered:
                btn.on_mouse_leave()
                self.inv_manager.add_area(btn.area)

    def handle_mouse_button_down(self, pos):
        """Mouse button press"""
        for btn in self.demo_buttons + [self.gradient_btn, self.shadow_btn,
                                         self.rounded_btn, self.noshadow_btn]:
            if btn.is_point_inside(pos[0], pos[1]) and btn.enabled:
                btn.on_mouse_press()
                self.inv_manager.add_area(btn.area)

    def handle_mouse_button_up(self, pos):
        """Mouse button release"""
        for btn in self.demo_buttons + [self.gradient_btn, self.shadow_btn,
                                         self.rounded_btn, self.noshadow_btn]:
            if btn.has_state(4):  # PRESSED state
                btn.on_mouse_release()
                self.inv_manager.add_area(btn.area)

    def render_frame(self):
        """
        Bir frame render et
        LVGL'nin lv_refr_area() benzeri
        """
        start_time = time.time()

        # Dirty areas var mı?
        if not self.inv_manager.has_areas():
            return  # Hiçbir şey çizilmeyecek

        # Her invalid area için
        for inv_area in self.inv_manager.get_areas():
            # Clipping area ayarla
            self.draw_ctx.set_clip_area(inv_area)

            # Widget tree'yi çiz
            self.root.draw_tree(self.draw_ctx)

            # Flush
            self.display.flush(inv_area)

        # Invalid areas temizle
        self.inv_manager.clear()

        # Performance tracking
        render_time = (time.time() - start_time) * 1000  # ms
        self.frame_times.append(render_time)
        if len(self.frame_times) > self.max_frame_history:
            self.frame_times.pop(0)

        return render_time

    def update_stats(self):
        """İstatistikleri güncelle"""
        stats = self.display.get_stats()
        draw_stats = self.draw_ctx.get_stats()

        self.stats_label.set_text(
            f"Frame: {stats['frame_count']} | FPS: {stats['fps']:.1f}"
        )

        self.draw_stats_label.set_text(
            f"Draws: R:{draw_stats['rect_count']} "
            f"L:{draw_stats['line_count']} "
            f"C:{draw_stats['circle_count']}"
        )

        avg_time = sum(self.frame_times) / len(self.frame_times) if self.frame_times else 0
        self.perf_label.set_text(f"Render Time: {avg_time:.2f}ms")

    def run(self):
        """Ana döngü - LVGL'nin lv_timer_handler() benzeri"""
        print("\n" + "="*60)
        print("LVGL DRAW SUBSYSTEM DEMO")
        print("="*60)
        print(f"Backend: {self.draw_ctx.backend_name}")
        print("Controls:")
        print("  SPACE  - Switch backend (SW ↔ GPU)")
        print("  Mouse  - Interact with buttons")
        print("  ESC    - Exit")
        print("="*60 + "\n")

        clock = pygame.time.Clock()
        frame_count = 0

        # İlk full refresh
        self.invalidate_all()

        while self.running:
            # Event handling
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        self.running = False
                    elif event.key == pygame.K_SPACE:
                        self.switch_backend()
                elif event.type == pygame.MOUSEMOTION:
                    self.handle_mouse_motion(event.pos)
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    self.handle_mouse_button_down(event.pos)
                elif event.type == pygame.MOUSEBUTTONUP:
                    self.handle_mouse_button_up(event.pos)

            # Render frame
            self.render_frame()

            # Stats güncelle (her 10 frame'de bir)
            frame_count += 1
            if frame_count % 10 == 0:
                self.update_stats()
                # Stats panel'i invalidate et
                for child in self.root.children[1].children:  # info_panel
                    self.inv_manager.add_area(child.area)

            # FPS limit (30 FPS - LVGL default)
            clock.tick(30)

        pygame.quit()
        print("\n[DEMO] Exiting...")


def main():
    """Ana fonksiyon"""
    try:
        demo = LVGLDemo(800, 600)
        demo.run()
    except Exception as e:
        print(f"[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())


# Import Color for panels
from core.color import Color
