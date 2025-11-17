"""
Area (bölge) yönetimi - LVGL lv_area_t benzeri
"""

class Area:
    """
    Dikdörtgen alan tanımı
    LVGL'deki lv_area_t yapısının Python karşılığı
    """

    def __init__(self, x1=0, y1=0, x2=0, y2=0):
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2

    @classmethod
    def from_pos_size(cls, x, y, width, height):
        """Pozisyon ve boyuttan alan oluştur"""
        return cls(x, y, x + width - 1, y + height - 1)

    def get_width(self):
        """Alan genişliği"""
        return self.x2 - self.x1 + 1

    def get_height(self):
        """Alan yüksekliği"""
        return self.y2 - self.y1 + 1

    def get_size(self):
        """Alan boyutu (piksel sayısı)"""
        return self.get_width() * self.get_height()

    def is_point_inside(self, x, y):
        """Nokta alan içinde mi?"""
        return self.x1 <= x <= self.x2 and self.y1 <= y <= self.y2

    def intersect(self, other):
        """
        İki alanın kesişimini hesapla
        LVGL'deki _lv_area_intersect() benzeri
        Returns: (success, intersection_area)
        """
        res_area = Area()
        res_area.x1 = max(self.x1, other.x1)
        res_area.y1 = max(self.y1, other.y1)
        res_area.x2 = min(self.x2, other.x2)
        res_area.y2 = min(self.y2, other.y2)

        # Geçerli kesişim var mı?
        if res_area.x1 <= res_area.x2 and res_area.y1 <= res_area.y2:
            return True, res_area
        return False, None

    def join(self, other):
        """
        İki alanı birleştir (union)
        LVGL'deki _lv_area_join() benzeri
        """
        self.x1 = min(self.x1, other.x1)
        self.y1 = min(self.y1, other.y1)
        self.x2 = max(self.x2, other.x2)
        self.y2 = max(self.y2, other.y2)

    def is_in(self, other):
        """Bu alan diğer alanın içinde mi?"""
        return (self.x1 >= other.x1 and self.x2 <= other.x2 and
                self.y1 >= other.y1 and self.y2 <= other.y2)

    def copy(self):
        """Alan kopyası oluştur"""
        return Area(self.x1, self.y1, self.x2, self.y2)

    def to_rect(self):
        """SDL Rect formatına çevir (x, y, width, height)"""
        return (self.x1, self.y1, self.get_width(), self.get_height())

    def __repr__(self):
        return f"Area({self.x1}, {self.y1}, {self.x2}, {self.y2}) [{self.get_width()}x{self.get_height()}]"

    def __eq__(self, other):
        if not isinstance(other, Area):
            return False
        return (self.x1 == other.x1 and self.y1 == other.y1 and
                self.x2 == other.x2 and self.y2 == other.y2)


class Point:
    """2D nokta - LVGL lv_point_t benzeri"""

    def __init__(self, x=0, y=0):
        self.x = x
        self.y = y

    def __repr__(self):
        return f"Point({self.x}, {self.y})"

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y
