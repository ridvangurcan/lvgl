"""
Draw Context - LVGL lv_draw_ctx_t benzeri
STRATEGY PATTERN implementasyonu
"""

from core.area import Area


class DrawContext:
    """
    Draw Context - Rendering backend'leri için soyutlama katmanı

    STRATEGY PATTERN:
    - Bu sınıf interface tanımlar
    - Farklı backend'ler (SW, GPU) bu fonksiyonları implement eder
    - Runtime'da backend değiştirilebilir

    LVGL'deki lv_draw_ctx_t yapısının karşılığı
    """

    def __init__(self, buffer, buf_area):
        """
        Args:
            buffer: Çizim için kullanılacak buffer (SDL Surface)
            buf_area: Buffer'ın koordinatları ve boyutu
        """
        self.buf = buffer
        self.buf_area = buf_area
        self.clip_area = buf_area.copy()  # Başlangıçta tüm buffer

        # === STRATEGY PATTERN: Function pointers ===
        # Bu fonksiyonlar backend tarafından override edilecek
        self.draw_rect = None
        self.draw_line = None
        self.draw_arc = None
        self.draw_circle = None
        self.draw_label = None
        self.blend = None
        self.wait_for_finish = None

        # Backend bilgisi
        self.backend_name = "None"

        # İstatistikler
        self.stats = {
            'rect_count': 0,
            'line_count': 0,
            'circle_count': 0,
            'blend_count': 0,
            'pixels_drawn': 0
        }

    def set_clip_area(self, area):
        """
        Clipping alanı belirle
        Sadece bu alan içindeki pikseller çizilecek
        """
        success, clipped = self.buf_area.intersect(area)
        if success:
            self.clip_area = clipped
        else:
            self.clip_area = Area()  # Boş alan

    def reset_clip_area(self):
        """Clipping'i kaldır"""
        self.clip_area = self.buf_area.copy()

    def is_area_visible(self, area):
        """Alan görünür mü? (Clipping kontrolü)"""
        success, _ = area.intersect(self.clip_area)
        return success

    def get_stats(self):
        """İstatistikleri döner"""
        return self.stats.copy()

    def reset_stats(self):
        """İstatistikleri sıfırla"""
        for key in self.stats:
            self.stats[key] = 0


class DrawRectDescriptor:
    """
    Dikdörtgen çizim parametreleri
    LVGL'deki lv_draw_rect_dsc_t benzeri
    """

    def __init__(self):
        # Renk ve opacity
        self.bg_color = None
        self.bg_opa = 255

        # Border
        self.border_color = None
        self.border_width = 0
        self.border_opa = 255

        # Köşe yuvarlatma
        self.radius = 0

        # Gradient
        self.bg_grad_color = None
        self.bg_grad_dir = 0  # 0: vertical, 1: horizontal

        # Shadow (gölge)
        self.shadow_width = 0
        self.shadow_ofs_x = 0
        self.shadow_ofs_y = 0
        self.shadow_color = None
        self.shadow_opa = 128


class DrawLabelDescriptor:
    """
    Metin çizim parametreleri
    LVGL'deki lv_draw_label_dsc_t benzeri
    """

    def __init__(self):
        self.color = None
        self.opa = 255
        self.font_size = 16
        self.align = 'center'  # 'left', 'center', 'right'


class DrawLineDescriptor:
    """
    Çizgi çizim parametreleri
    LVGL'deki lv_draw_line_dsc_t benzeri
    """

    def __init__(self):
        self.color = None
        self.width = 1
        self.opa = 255


class DrawArcDescriptor:
    """
    Yay/Daire çizim parametreleri
    LVGL'deki lv_draw_arc_dsc_t benzeri
    """

    def __init__(self):
        self.color = None
        self.width = 1
        self.opa = 255
        self.rounded = False  # Yuvarlatılmış uçlar
