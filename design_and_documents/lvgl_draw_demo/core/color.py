"""
Color utilities - LVGL benzeri renk yönetimi
"""

class Color:
    """RGB888 renk sınıfı - LVGL lv_color_t benzeri"""

    def __init__(self, r=0, g=0, b=0, a=255):
        self.r = max(0, min(255, r))
        self.g = max(0, min(255, g))
        self.b = max(0, min(255, b))
        self.a = max(0, min(255, a))  # Opacity

    def to_tuple(self):
        """SDL için (r, g, b, a) tuple döner"""
        return (self.r, self.g, self.b, self.a)

    def to_rgb(self):
        """RGB tuple (alpha olmadan)"""
        return (self.r, self.g, self.b)

    def blend_with(self, other, opa):
        """
        Alpha blending: result = (src * opa) + (dst * (255 - opa)) / 255
        LVGL'deki lv_color_mix() benzeri
        """
        inv_opa = 255 - opa
        r = (self.r * opa + other.r * inv_opa) // 255
        g = (self.g * opa + other.g * inv_opa) // 255
        b = (self.b * opa + other.b * inv_opa) // 255
        return Color(r, g, b, 255)

    @staticmethod
    def from_hex(hex_str):
        """Hex string'den renk oluştur (#RRGGBB veya RRGGBB)"""
        hex_str = hex_str.lstrip('#')
        return Color(
            int(hex_str[0:2], 16),
            int(hex_str[2:4], 16),
            int(hex_str[4:6], 16)
        )

    def __repr__(self):
        return f"Color(r={self.r}, g={self.g}, b={self.b}, a={self.a})"


# LVGL benzeri renk tanımları
class Colors:
    """Önceden tanımlı renkler - LVGL lv_palette_t benzeri"""
    WHITE = Color(255, 255, 255)
    BLACK = Color(0, 0, 0)
    RED = Color(255, 0, 0)
    GREEN = Color(0, 255, 0)
    BLUE = Color(0, 0, 255)
    YELLOW = Color(255, 255, 0)
    CYAN = Color(0, 255, 255)
    MAGENTA = Color(255, 0, 255)
    GRAY = Color(128, 128, 128)
    LIGHT_GRAY = Color(192, 192, 192)
    DARK_GRAY = Color(64, 64, 64)

    # Material Design benzeri
    PRIMARY = Color(33, 150, 243)      # Blue
    SECONDARY = Color(255, 152, 0)     # Orange
    SUCCESS = Color(76, 175, 80)       # Green
    WARNING = Color(255, 193, 7)       # Amber
    DANGER = Color(244, 67, 54)        # Red
    INFO = Color(3, 169, 244)          # Light Blue


# Opacity değerleri (LVGL benzeri)
class Opacity:
    """Opacity constants - LVGL lv_opa_t benzeri"""
    TRANSP = 0
    OPA_10 = 25
    OPA_20 = 51
    OPA_30 = 76
    OPA_40 = 102
    OPA_50 = 127
    OPA_60 = 153
    OPA_70 = 178
    OPA_80 = 204
    OPA_90 = 229
    COVER = 255
