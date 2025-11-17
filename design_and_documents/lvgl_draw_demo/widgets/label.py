"""
Label Widget - LVGL lv_label benzeri
Basit metin gösterimi
"""

from widgets.widget_base import Widget
from core.draw_context import DrawLabelDescriptor
from core.color import Colors, Opacity
from core.area import Point


class Label(Widget):
    """
    Label widget - metin gösterimi
    LVGL lv_label_t benzeri
    """

    def __init__(self, x, y, text="Label", font_size=16):
        # Label için minimum boyut (text render edildikten sonra ayarlanabilir)
        super().__init__(x, y, 100, 30)

        self.text = text
        self.font_size = font_size
        self.text_color = Colors.BLACK
        self.align = 'left'  # 'left', 'center', 'right'

    def draw(self, draw_ctx):
        """Label çizimi"""
        label_dsc = DrawLabelDescriptor()
        label_dsc.color = self.text_color
        label_dsc.opa = Opacity.COVER if self.enabled else Opacity.OPA_50
        label_dsc.font_size = self.font_size
        label_dsc.align = self.align

        # Text pozisyonu
        if self.align == 'center':
            text_pos = Point(
                (self.area.x1 + self.area.x2) // 2,
                (self.area.y1 + self.area.y2) // 2
            )
        elif self.align == 'right':
            text_pos = Point(
                self.area.x2,
                (self.area.y1 + self.area.y2) // 2
            )
        else:  # left
            text_pos = Point(
                self.area.x1,
                (self.area.y1 + self.area.y2) // 2
            )

        draw_ctx.draw_label(draw_ctx, label_dsc, text_pos, self.text)

    def set_text(self, text):
        """Text değiştir"""
        self.text = text
        self.invalidate()


class Panel(Widget):
    """
    Panel widget - container
    LVGL lv_obj benzeri basit container
    """

    def __init__(self, x, y, width, height):
        super().__init__(x, y, width, height)

        self.bg_color = Colors.LIGHT_GRAY
        self.border_color = Colors.DARK_GRAY
        self.border_width = 2
        self.radius = 0

    def draw(self, draw_ctx):
        """Panel çizimi - basit background"""
        from core.draw_context import DrawRectDescriptor

        rect_dsc = DrawRectDescriptor()
        rect_dsc.bg_color = self.bg_color
        rect_dsc.bg_opa = Opacity.COVER
        rect_dsc.border_color = self.border_color
        rect_dsc.border_width = self.border_width
        rect_dsc.radius = self.radius

        draw_ctx.draw_rect(draw_ctx, rect_dsc, self.area)
