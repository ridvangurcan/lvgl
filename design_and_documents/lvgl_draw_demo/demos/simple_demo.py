#!/usr/bin/env python3
"""
Basit Demo - LVGL Draw Konseptlerini Minimal Örnekle Gösterir

Bu demo temel konseptleri gösterir:
- Draw context kullanımı
- Backend initialization
- Widget oluşturma
- Basit rendering
"""

import sys
import os

# Parent directory'yi path'e ekle
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import pygame
from core.display import DisplayDriver
from core.draw_context import DrawContext
from core.area import Area
from core.color import Colors
from backends.backend_sw import SoftwareBackend
from widgets.button import Button
from widgets.label import Label


def simple_demo():
    """Minimal demo - Temel konseptleri gösterir"""

    print("="*60)
    print("LVGL DRAW DEMO - Basit Örnek")
    print("="*60)

    # 1. Display oluştur (HAL)
    print("\n[1] Display driver oluşturuluyor...")
    display = DisplayDriver(640, 480, "LVGL Simple Demo")

    # 2. Draw context oluştur (STRATEGY PATTERN)
    print("[2] Draw context oluşturuluyor...")
    draw_ctx = DrawContext(display.draw_buf, display.buf_area)

    # 3. Backend initialize et (SW rendering)
    print("[3] Software backend initialize ediliyor...")
    SoftwareBackend.init_context(draw_ctx)
    print(f"    → Backend: {draw_ctx.backend_name}")

    # 4. Widget'lar oluştur (FACTORY PATTERN)
    print("\n[4] Widget'lar oluşturuluyor...")

    # Title label
    title = Label(20, 20, "LVGL Draw Konsept Demo", font_size=32)
    title.text_color = Colors.PRIMARY
    print(f"    → Label: {title}")

    # Demo button
    button = Button(220, 150, 200, 80, "Click Me!")
    button.shadow_enabled = True
    print(f"    → Button: {button}")

    # Event callback (OBSERVER PATTERN)
    click_count = [0]  # List kullanıyoruz (mutable)

    def on_button_click(widget, data):
        click_count[0] += 1
        print(f"\n[EVENT] Button clicked! Count: {click_count[0]}")

    button.on_event('click', on_button_click)

    # Info label
    info = Label(20, 300, "Mouse ile button'a tıklayın. ESC ile çıkış.", font_size=16)
    print(f"    → Info: {info}")

    # 5. Ana döngü
    print("\n[5] Rendering başlıyor...")
    print("    Kontroller: Mouse ile click, ESC ile çıkış\n")

    running = True
    clock = pygame.time.Clock()

    # İlk render
    display.clear(Colors.WHITE)
    title.draw(draw_ctx)
    button.draw(draw_ctx)
    info.draw(draw_ctx)
    display.flush()

    frame_count = 0

    while running:
        # Event handling
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False

            elif event.type == pygame.MOUSEMOTION:
                # Hover detection
                pos = event.pos
                was_hovered = button.has_state(16)  # HOVERED
                is_hovered = button.is_point_inside(pos[0], pos[1])

                if is_hovered != was_hovered:
                    if is_hovered:
                        button.on_mouse_enter()
                    else:
                        button.on_mouse_leave()

                    # Re-render (STATE PATTERN)
                    display.clear(Colors.WHITE)
                    title.draw(draw_ctx)
                    button.draw(draw_ctx)
                    info.draw(draw_ctx)
                    display.flush()

            elif event.type == pygame.MOUSEBUTTONDOWN:
                if button.is_point_inside(event.pos[0], event.pos[1]):
                    button.on_mouse_press()

                    # Re-render
                    display.clear(Colors.WHITE)
                    title.draw(draw_ctx)
                    button.draw(draw_ctx)
                    info.draw(draw_ctx)
                    display.flush()

            elif event.type == pygame.MOUSEBUTTONUP:
                if button.has_state(4):  # PRESSED
                    button.on_mouse_release()

                    # Re-render
                    display.clear(Colors.WHITE)
                    title.draw(draw_ctx)
                    button.draw(draw_ctx)
                    info.draw(draw_ctx)
                    display.flush()

        # FPS limit
        clock.tick(30)
        frame_count += 1

        # Stats (her 60 frame'de bir)
        if frame_count % 60 == 0:
            stats = display.get_stats()
            draw_stats = draw_ctx.get_stats()
            print(f"[STATS] Frame: {stats['frame_count']}, "
                  f"FPS: {stats['fps']:.1f}, "
                  f"Rects: {draw_stats['rect_count']}, "
                  f"Clicks: {click_count[0]}")

    # Cleanup
    pygame.quit()
    print("\n[DEMO] Kapatılıyor...")
    print(f"Toplam {frame_count} frame render edildi.")
    print(f"Button {click_count[0]} kez tıklandı.")
    print("="*60)


if __name__ == "__main__":
    try:
        simple_demo()
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
