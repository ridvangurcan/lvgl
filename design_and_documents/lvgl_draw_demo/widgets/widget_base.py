"""
Base Widget Class - LVGL lv_obj_t benzeri
STATE PATTERN ve OBSERVER PATTERN implementasyonu
"""

from core.area import Area
from core.color import Colors


class WidgetState:
    """
    Widget state enumeration - LVGL lv_state_t benzeri

    STATE PATTERN:
    Widget'lar farklı state'lerde farklı görünüme sahip olabilir
    """
    DEFAULT = 0
    CHECKED = 1
    FOCUSED = 2
    PRESSED = 4
    DISABLED = 8
    HOVERED = 16


class Widget:
    """
    Base widget class - LVGL lv_obj_t benzeri

    OBSERVER PATTERN:
    - Event callback sistemi
    - Parent-child hierarchy
    """

    def __init__(self, x, y, width, height):
        # Pozisyon ve boyut
        self.area = Area.from_pos_size(x, y, width, height)

        # Parent-child hierarchy
        self.parent = None
        self.children = []

        # State (STATE PATTERN)
        self.state = WidgetState.DEFAULT

        # Görünürlük
        self.visible = True
        self.enabled = True

        # Style (basitleştirilmiş)
        self.bg_color = Colors.WHITE
        self.border_color = Colors.BLACK
        self.border_width = 1

        # Event callbacks (OBSERVER PATTERN)
        self.event_callbacks = {}

    def add_child(self, child_widget):
        """Child widget ekle"""
        child_widget.parent = self
        self.children.append(child_widget)

    def remove_child(self, child_widget):
        """Child widget çıkar"""
        if child_widget in self.children:
            self.children.remove(child_widget)
            child_widget.parent = None

    def invalidate(self):
        """
        Widget'ı geçersiz kıl (yeniden çizilmesi gerekiyor)
        LVGL'deki lv_obj_invalidate() benzeri

        Dirty region tracking için kullanılır
        """
        # Bu normalde display'e bildirim gönderir
        pass

    def set_state(self, state, value=True):
        """
        State değiştir - STATE PATTERN

        Args:
            state: WidgetState flag
            value: True to set, False to clear
        """
        if value:
            self.state |= state
        else:
            self.state &= ~state

        self.invalidate()  # State değişince yeniden çiz

    def has_state(self, state):
        """State kontrolü"""
        return (self.state & state) != 0

    def is_visible(self):
        """Widget görünür mü?"""
        return self.visible and (self.parent is None or self.parent.is_visible())

    def draw(self, draw_ctx):
        """
        Widget'ı çiz
        Alt sınıflar bu fonksiyonu override etmeli
        """
        raise NotImplementedError("Subclasses must implement draw()")

    def draw_tree(self, draw_ctx):
        """
        Widget ve tüm child'larını çiz (tree traversal)
        LVGL'deki lv_obj_draw() benzeri
        """
        if not self.is_visible():
            return

        # Clipping kontrolü
        if not draw_ctx.is_area_visible(self.area):
            return

        # Widget'ı çiz
        self.draw(draw_ctx)

        # Child'ları çiz
        for child in self.children:
            child.draw_tree(draw_ctx)

    def on_event(self, event_type, callback):
        """
        Event callback register et
        OBSERVER PATTERN

        Args:
            event_type: Event tipi (string)
            callback: Çağrılacak fonksiyon
        """
        if event_type not in self.event_callbacks:
            self.event_callbacks[event_type] = []
        self.event_callbacks[event_type].append(callback)

    def trigger_event(self, event_type, data=None):
        """Event tetikle"""
        if event_type in self.event_callbacks:
            for callback in self.event_callbacks[event_type]:
                callback(self, data)

    def is_point_inside(self, x, y):
        """Nokta widget içinde mi? (hit test)"""
        return self.area.is_point_inside(x, y)

    def __repr__(self):
        return f"{self.__class__.__name__}@{self.area}"
