"""
Drawing Primitives - Temel çizim algoritmaları
LVGL'deki lv_draw_sw_*.c dosyalarının karşılığı
"""

import pygame
import math
from core.color import Color, Opacity


class DrawPrimitives:
    """
    Temel çizim fonksiyonları
    Her backend bunları kendi yöntemiyle implement eder
    """

    @staticmethod
    def draw_line_bresenham(surface, x0, y0, x1, y1, color, width=1):
        """
        Bresenham çizgi algoritması
        LVGL'deki lv_draw_sw_line() benzeri

        Bresenham algoritması integer aritmetiği kullanarak
        float işlem yapmadan düz çizgi çizer.
        """
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy

        points = []

        while True:
            points.append((x0, y0))

            if x0 == x1 and y0 == y1:
                break

            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

        # Çizgiyi çiz
        if width == 1:
            for point in points:
                surface.set_at(point, color.to_rgb())
        else:
            pygame.draw.lines(surface, color.to_rgb(), False, points, width)

        return len(points)

    @staticmethod
    def draw_circle_midpoint(surface, center_x, center_y, radius, color, filled=False):
        """
        Midpoint Circle Algorithm (Bresenham benzeri)
        LVGL'deki lv_draw_sw_arc() benzeri

        Integer aritmetiği kullanarak çember çizer.
        """
        x = radius
        y = 0
        err = 0

        points = []

        while x >= y:
            # 8 simetrik noktayı ekle
            points.extend([
                (center_x + x, center_y + y),
                (center_x + y, center_y + x),
                (center_x - y, center_y + x),
                (center_x - x, center_y + y),
                (center_x - x, center_y - y),
                (center_x - y, center_y - x),
                (center_x + y, center_y - x),
                (center_x + x, center_y - y),
            ])

            y += 1
            err += 1 + 2 * y
            if 2 * (err - x) + 1 > 0:
                x -= 1
                err += 1 - 2 * x

        if filled:
            pygame.draw.circle(surface, color.to_rgb(), (center_x, center_y), radius)
        else:
            for point in points:
                if 0 <= point[0] < surface.get_width() and 0 <= point[1] < surface.get_height():
                    surface.set_at(point, color.to_rgb())

        return len(points)

    @staticmethod
    def draw_arc(surface, center_x, center_y, radius, start_angle, end_angle, color, width=1):
        """
        Yay çizimi
        LVGL'deki lv_draw_sw_arc() benzeri
        """
        # Arc için Pygame kullanıyoruz (pure implementation çok karmaşık)
        rect = pygame.Rect(center_x - radius, center_y - radius, radius * 2, radius * 2)

        # Açıları radyana çevir
        start_rad = math.radians(start_angle)
        end_rad = math.radians(end_angle)

        pygame.draw.arc(surface, color.to_rgb(), rect, start_rad, end_rad, width)

    @staticmethod
    def fill_rect_simple(surface, x, y, width, height, color):
        """
        Basit dikdörtgen dolgu
        LVGL'deki lv_draw_sw_blend() benzeri
        """
        rect = pygame.Rect(x, y, width, height)
        pygame.draw.rect(surface, color.to_rgb(), rect)

    @staticmethod
    def fill_rect_with_gradient(surface, x, y, width, height, color_start, color_end, vertical=True):
        """
        Gradient dolgu
        LVGL'deki lv_draw_sw_gradient() benzeri

        Gradient hesaplamaları:
        - Vertical: Yukarıdan aşağıya renk geçişi
        - Horizontal: Soldan sağa renk geçişi
        """
        if vertical:
            # Dikey gradient
            for i in range(height):
                ratio = i / height
                r = int(color_start.r + (color_end.r - color_start.r) * ratio)
                g = int(color_start.g + (color_end.g - color_start.g) * ratio)
                b = int(color_start.b + (color_end.b - color_start.b) * ratio)
                line_color = Color(r, g, b)
                pygame.draw.line(surface, line_color.to_rgb(),
                               (x, y + i), (x + width - 1, y + i))
        else:
            # Yatay gradient
            for i in range(width):
                ratio = i / width
                r = int(color_start.r + (color_end.r - color_start.r) * ratio)
                g = int(color_start.g + (color_end.g - color_start.g) * ratio)
                b = int(color_start.b + (color_end.b - color_start.b) * ratio)
                line_color = Color(r, g, b)
                pygame.draw.line(surface, line_color.to_rgb(),
                               (x + i, y), (x + i, y + height - 1))

    @staticmethod
    def draw_rounded_rect(surface, x, y, width, height, radius, color, filled=True):
        """
        Yuvarlatılmış köşeli dikdörtgen
        LVGL'deki radius desteği benzeri
        """
        rect = pygame.Rect(x, y, width, height)
        pygame.draw.rect(surface, color.to_rgb(), rect, border_radius=radius)

    @staticmethod
    def draw_shadow(surface, x, y, width, height, shadow_width, offset_x, offset_y, shadow_color, shadow_opa):
        """
        Gölge efekti
        LVGL'deki shadow rendering benzeri
        """
        # Gölge için semi-transparent surface oluştur
        shadow_surf = pygame.Surface((width + shadow_width * 2, height + shadow_width * 2), pygame.SRCALPHA)

        # Gölgeyi çiz (blur efekti için birden fazla layer)
        for i in range(shadow_width):
            alpha = int((shadow_width - i) / shadow_width * shadow_opa)
            color_with_alpha = (*shadow_color.to_rgb(), alpha)
            pygame.draw.rect(shadow_surf, color_with_alpha,
                           (i, i, width + shadow_width * 2 - i * 2, height + shadow_width * 2 - i * 2))

        # Ana surface'e blend et
        surface.blit(shadow_surf, (x + offset_x - shadow_width, y + offset_y - shadow_width))

    @staticmethod
    def blend_pixel(dest_surface, x, y, src_color, opacity):
        """
        Tek pixel blending
        LVGL blend formülü: result = (src * opa) + (dst * (255 - opa)) / 255
        """
        if 0 <= x < dest_surface.get_width() and 0 <= y < dest_surface.get_height():
            # Mevcut pixel rengini al
            dest_color_tuple = dest_surface.get_at((x, y))[:3]  # RGB
            dest_color = Color(*dest_color_tuple)

            # Blend hesapla
            blended = src_color.blend_with(dest_color, opacity)

            # Pixel'i yaz
            dest_surface.set_at((x, y), blended.to_rgb())

    @staticmethod
    def draw_text(surface, text, x, y, color, font_size=16, align='left'):
        """
        Metin çizimi
        LVGL'deki lv_draw_sw_letter() benzeri
        """
        font = pygame.font.Font(None, font_size)
        text_surf = font.render(text, True, color.to_rgb())

        # Align ayarla
        text_rect = text_surf.get_rect()
        if align == 'center':
            text_rect.center = (x, y)
        elif align == 'right':
            text_rect.right = x
            text_rect.centery = y
        else:  # left
            text_rect.left = x
            text_rect.centery = y

        surface.blit(text_surf, text_rect)
        return text_rect.width


class BlendMode:
    """
    Blend mode enumeration
    LVGL'deki lv_blend_mode_t benzeri
    """
    NORMAL = 0       # Alpha blending
    ADDITIVE = 1     # Renkleri topla
    SUBTRACTIVE = 2  # Renkleri çıkar
    MULTIPLY = 3     # Renkleri çarp
