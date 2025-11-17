"""
Display HAL - LVGL lv_disp_drv_t benzeri
Hardware Abstraction Layer simülasyonu
"""

import pygame
from core.area import Area
from core.color import Colors
import time


class DisplayDriver:
    """
    Display driver - LVGL lv_disp_drv_t benzeri

    OBSERVER PATTERN:
    - Callback'ler aracılığıyla event-driven mimari
    - flush_cb, monitor_cb vb. callback'ler
    """

    def __init__(self, hor_res, ver_res, title="LVGL Draw Demo"):
        self.hor_res = hor_res
        self.ver_res = ver_res

        # Pygame/SDL initialization
        pygame.init()
        self.screen = pygame.display.set_mode((hor_res, ver_res))
        pygame.display.set_caption(title)

        # Draw buffer (LVGL'deki draw_buf benzeri)
        self.draw_buf = pygame.Surface((hor_res, ver_res))
        self.draw_buf.fill(Colors.WHITE.to_rgb())

        # Buffer area
        self.buf_area = Area.from_pos_size(0, 0, hor_res, ver_res)

        # === CALLBACK FUNCTIONS ===
        self.flush_cb = None            # Zorunlu: buffer'ı ekrana yaz
        self.monitor_cb = None          # İsteğe bağlı: performans takibi
        self.render_start_cb = None     # İsteğe bağlı: render başlangıcı

        # Display flags
        self.direct_mode = False
        self.full_refresh = False
        self.antialiasing = True

        # İstatistikler
        self.frame_count = 0
        self.total_pixels_flushed = 0
        self.last_render_time = 0

        # FPS tracking
        self.fps_clock = pygame.time.Clock()
        self.target_fps = 30  # LVGL default: 33ms = ~30 FPS

    def flush(self, area=None):
        """
        Buffer'ı ekrana flush et
        LVGL'deki flush_cb benzeri

        Args:
            area: Flush edilecek alan (None ise tüm ekran)
        """
        if area is None:
            area = self.buf_area

        start_time = time.time()

        # Callback varsa çağır
        if self.render_start_cb:
            self.render_start_cb(self)

        # Buffer'dan screen'e blit
        if area == self.buf_area:
            # Full screen blit
            self.screen.blit(self.draw_buf, (0, 0))
        else:
            # Partial area blit
            rect = area.to_rect()
            self.screen.blit(self.draw_buf, rect, rect)

        pygame.display.flip()

        # Süre hesapla
        flush_time_ms = (time.time() - start_time) * 1000
        pixel_count = area.get_size()

        # İstatistikler güncelle
        self.frame_count += 1
        self.total_pixels_flushed += pixel_count
        self.last_render_time = flush_time_ms

        # Monitor callback
        if self.monitor_cb:
            self.monitor_cb(self, flush_time_ms, pixel_count)

        # Custom flush callback
        if self.flush_cb:
            self.flush_cb(self, area)

        # FPS limit
        self.fps_clock.tick(self.target_fps)

    def get_fps(self):
        """Gerçek FPS değerini döner"""
        return self.fps_clock.get_fps()

    def get_stats(self):
        """Display istatistikleri"""
        return {
            'frame_count': self.frame_count,
            'fps': self.get_fps(),
            'total_pixels': self.total_pixels_flushed,
            'last_render_time_ms': self.last_render_time
        }

    def clear(self, color=None):
        """Ekranı temizle"""
        if color is None:
            color = Colors.WHITE
        self.draw_buf.fill(color.to_rgb())


class InvalidAreaManager:
    """
    Dirty region tracking - LVGL'deki inv_areas[] benzeri

    Sadece değişen alanları takip ederek performans optimizasyonu
    """

    MAX_AREAS = 32  # LVGL'de LV_INV_BUF_SIZE default: 32

    def __init__(self):
        self.inv_areas = []  # Geçersiz (yeniden çizilecek) alanlar

    def add_area(self, area):
        """
        Geçersiz alan ekle
        Gerekirse komşu alanları birleştir (join optimization)
        """
        # Önce birleştirmeyi dene
        joined = False
        for i, existing in enumerate(self.inv_areas):
            if self._should_join(area, existing):
                existing.join(area)
                joined = True
                break

        if not joined:
            self.inv_areas.append(area.copy())

        # Max alan sınırı aşıldıysa full refresh'e geç
        if len(self.inv_areas) >= self.MAX_AREAS:
            self._merge_to_full_screen()

    def _should_join(self, area1, area2):
        """
        İki alan birleştirilmeli mi?
        LVGL'deki join algoritması benzeri

        Join area size < (area1 + area2) * 1.1 ise birleştir
        """
        # Test: birleşik alan hesapla
        test_area = area1.copy()
        test_area.join(area2)

        join_size = test_area.get_size()
        total_size = area1.get_size() + area2.get_size()

        # %10'dan fazla boşluk olmasın
        return join_size < total_size * 1.1

    def _merge_to_full_screen(self):
        """Tüm alanları tek bir full screen alanına birleştir"""
        if not self.inv_areas:
            return

        # Tüm alanları kapsayan tek bir alan oluştur
        full_area = self.inv_areas[0].copy()
        for area in self.inv_areas[1:]:
            full_area.join(area)

        self.inv_areas = [full_area]

    def get_areas(self):
        """Geçersiz alanları döner"""
        return self.inv_areas.copy()

    def clear(self):
        """Tüm geçersiz alanları temizle"""
        self.inv_areas.clear()

    def has_areas(self):
        """Geçersiz alan var mı?"""
        return len(self.inv_areas) > 0
