"""
Software Rendering Backend
LVGL'deki src/draw/sw/ implementasyonunun benzeri
Tamamen CPU tabanlı rendering
"""

import pygame
from core.draw_context import DrawContext, DrawRectDescriptor, DrawLabelDescriptor, DrawLineDescriptor, DrawArcDescriptor
from core.area import Area, Point
from core.color import Colors, Opacity
from draw.primitives import DrawPrimitives
import time


class SoftwareBackend:
    """
    Software rendering backend - CPU tabanlı
    LVGL'deki lv_draw_sw_ctx_t benzeri

    STRATEGY PATTERN implementation:
    Bu backend DrawContext'in function pointer'larını doldurur
    """

    @staticmethod
    def init_context(draw_ctx):
        """
        Draw context'i SW backend ile initialize et
        LVGL'deki lv_draw_sw_init_ctx() benzeri

        Bu fonksiyon STRATEGY PATTERN'in핵심idir:
        - DrawContext'in function pointer'larını set eder
        - Her pointer SW implementation'ına yönlendirilir
        """
        # Function pointer'ları SW implementasyonlarına yönlendir
        draw_ctx.draw_rect = SoftwareBackend.draw_rect
        draw_ctx.draw_line = SoftwareBackend.draw_line
        draw_ctx.draw_arc = SoftwareBackend.draw_arc
        draw_ctx.draw_circle = SoftwareBackend.draw_circle
        draw_ctx.draw_label = SoftwareBackend.draw_label
        draw_ctx.blend = SoftwareBackend.blend
        draw_ctx.wait_for_finish = SoftwareBackend.wait_for_finish

        draw_ctx.backend_name = "Software (CPU)"

        print(f"[SW Backend] Initialized: {draw_ctx.buf_area}")

    @staticmethod
    def draw_rect(draw_ctx, dsc, coords):
        """
        Dikdörtgen çizimi - LVGL lv_draw_sw_rect() benzeri

        Rendering sırası (LVGL benzeri):
        1. Shadow (gölge)
        2. Background (gradient veya solid)
        3. Border (kenarlık)
        """
        # Clipping kontrolü - Görünmüyorsa çizme
        if not draw_ctx.is_area_visible(coords):
            return

        # Clip area ile kesişimi hesapla
        success, clipped_area = coords.intersect(draw_ctx.clip_area)
        if not success:
            return

        x, y = coords.x1, coords.y1
        width = coords.get_width()
        height = coords.get_height()

        # 1. Shadow çiz (eğer varsa)
        if dsc.shadow_width > 0:
            DrawPrimitives.draw_shadow(
                draw_ctx.buf,
                x, y, width, height,
                dsc.shadow_width,
                dsc.shadow_ofs_x,
                dsc.shadow_ofs_y,
                dsc.shadow_color or Colors.BLACK,
                dsc.shadow_opa
            )

        # 2. Background çiz
        if dsc.bg_grad_color:
            # Gradient fill
            DrawPrimitives.fill_rect_with_gradient(
                draw_ctx.buf,
                x, y, width, height,
                dsc.bg_color,
                dsc.bg_grad_color,
                vertical=(dsc.bg_grad_dir == 0)
            )
        else:
            # Solid fill
            if dsc.radius > 0:
                # Yuvarlatılmış köşeler
                DrawPrimitives.draw_rounded_rect(
                    draw_ctx.buf,
                    x, y, width, height,
                    dsc.radius,
                    dsc.bg_color,
                    filled=True
                )
            else:
                # Normal dikdörtgen
                DrawPrimitives.fill_rect_simple(
                    draw_ctx.buf,
                    x, y, width, height,
                    dsc.bg_color
                )

        # 3. Border çiz (eğer varsa)
        if dsc.border_width > 0:
            border_rect = pygame.Rect(x, y, width, height)
            pygame.draw.rect(
                draw_ctx.buf,
                dsc.border_color.to_rgb(),
                border_rect,
                width=dsc.border_width,
                border_radius=dsc.radius
            )

        # İstatistik güncelle
        draw_ctx.stats['rect_count'] += 1
        draw_ctx.stats['pixels_drawn'] += coords.get_size()

    @staticmethod
    def draw_line(draw_ctx, dsc, point1, point2):
        """
        Çizgi çizimi - LVGL lv_draw_sw_line() benzeri
        Bresenham algoritması kullanır
        """
        pixel_count = DrawPrimitives.draw_line_bresenham(
            draw_ctx.buf,
            point1.x, point1.y,
            point2.x, point2.y,
            dsc.color,
            dsc.width
        )

        draw_ctx.stats['line_count'] += 1
        draw_ctx.stats['pixels_drawn'] += pixel_count

    @staticmethod
    def draw_circle(draw_ctx, center, radius, color, filled=False):
        """
        Çember çizimi - LVGL lv_draw_sw_arc() benzeri
        Midpoint Circle Algorithm kullanır
        """
        pixel_count = DrawPrimitives.draw_circle_midpoint(
            draw_ctx.buf,
            center.x, center.y,
            radius,
            color,
            filled
        )

        draw_ctx.stats['circle_count'] += 1
        draw_ctx.stats['pixels_drawn'] += pixel_count

    @staticmethod
    def draw_arc(draw_ctx, dsc, center, radius, start_angle, end_angle):
        """Yay çizimi"""
        DrawPrimitives.draw_arc(
            draw_ctx.buf,
            center.x, center.y,
            radius,
            start_angle, end_angle,
            dsc.color,
            dsc.width
        )

        draw_ctx.stats['pixels_drawn'] += radius * 2  # Yaklaşık

    @staticmethod
    def draw_label(draw_ctx, dsc, pos, text):
        """
        Metin çizimi - LVGL lv_draw_sw_letter() benzeri
        """
        width = DrawPrimitives.draw_text(
            draw_ctx.buf,
            text,
            pos.x, pos.y,
            dsc.color,
            dsc.font_size,
            dsc.align
        )

        draw_ctx.stats['pixels_drawn'] += width * dsc.font_size

    @staticmethod
    def blend(draw_ctx, src_area, src_color, opacity):
        """
        Blending işlemi - LVGL lv_draw_sw_blend() benzeri
        Alpha blending: result = (src * opa) + (dst * (255 - opa)) / 255
        """
        for y in range(src_area.y1, src_area.y2 + 1):
            for x in range(src_area.x1, src_area.x2 + 1):
                DrawPrimitives.blend_pixel(
                    draw_ctx.buf,
                    x, y,
                    src_color,
                    opacity
                )

        draw_ctx.stats['blend_count'] += 1

    @staticmethod
    def wait_for_finish(draw_ctx):
        """
        SW backend'de beklenecek bir şey yok (senkron)
        LVGL'deki lv_draw_sw_wait_for_finish() benzeri
        """
        pass  # Software rendering is synchronous
