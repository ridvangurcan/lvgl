"""
Button Widget - LVGL lv_btn benzeri
STATE PATTERN kullanımı örneği
"""

from widgets.widget_base import Widget, WidgetState
from core.draw_context import DrawRectDescriptor, DrawLabelDescriptor
from core.color import Colors, Opacity
from core.area import Point


class Button(Widget):
    """
    Button widget - LVGL lv_btn_t benzeri

    STATE PATTERN demonstration:
    - DEFAULT: Normal görünüm
    - HOVERED: Mouse üzerinde
    - PRESSED: Tıklanmış
    - DISABLED: Devre dışı
    """

    def __init__(self, x, y, width, height, text="Button"):
        super().__init__(x, y, width, height)

        self.text = text

        # State-specific colors (STATE PATTERN)
        self.colors = {
            WidgetState.DEFAULT: Colors.PRIMARY,
            WidgetState.HOVERED: Colors.INFO,
            WidgetState.PRESSED: Color(20, 100, 180),  # Darker blue
            WidgetState.DISABLED: Colors.GRAY
        }

        # Style
        self.radius = 8  # Yuvarlatılmış köşeler
        self.shadow_enabled = True

    def get_current_color(self):
        """
        Mevcut state'e göre renk döner
        STATE PATTERN implementation
        """
        if self.has_state(WidgetState.DISABLED):
            return self.colors[WidgetState.DISABLED]
        elif self.has_state(WidgetState.PRESSED):
            return self.colors[WidgetState.PRESSED]
        elif self.has_state(WidgetState.HOVERED):
            return self.colors[WidgetState.HOVERED]
        else:
            return self.colors[WidgetState.DEFAULT]

    def draw(self, draw_ctx):
        """
        Button çizimi
        LVGL lv_btn_draw() pattern'ini takip eder:
        1. Background (state'e göre renk)
        2. Shadow (eğer varsa)
        3. Text
        """
        # Draw descriptor oluştur
        rect_dsc = DrawRectDescriptor()
        rect_dsc.bg_color = self.get_current_color()
        rect_dsc.bg_opa = Opacity.COVER if self.enabled else Opacity.OPA_50

        # Gradient efekti (pressed değilse)
        if not self.has_state(WidgetState.PRESSED):
            # Yukarıdan aşağıya hafif gradient
            base_color = rect_dsc.bg_color
            from core.color import Color
            darker = Color(
                max(0, base_color.r - 30),
                max(0, base_color.g - 30),
                max(0, base_color.b - 30)
            )
            rect_dsc.bg_grad_color = darker
            rect_dsc.bg_grad_dir = 0  # Vertical

        # Border
        rect_dsc.border_color = Colors.BLACK
        rect_dsc.border_width = 2 if self.has_state(WidgetState.FOCUSED) else 0
        rect_dsc.border_opa = Opacity.OPA_30

        # Radius (yuvarlatılmış köşeler)
        rect_dsc.radius = self.radius

        # Shadow (pressed olmadığında)
        if self.shadow_enabled and not self.has_state(WidgetState.PRESSED):
            rect_dsc.shadow_width = 4
            rect_dsc.shadow_ofs_x = 2
            rect_dsc.shadow_ofs_y = 4
            rect_dsc.shadow_color = Colors.BLACK
            rect_dsc.shadow_opa = Opacity.OPA_30

        # Pressed efekti:約間 offset
        offset_y = 2 if self.has_state(WidgetState.PRESSED) else 0
        draw_area = self.area.copy()
        draw_area.y1 += offset_y
        draw_area.y2 += offset_y

        # Rectangle çiz
        draw_ctx.draw_rect(draw_ctx, rect_dsc, draw_area)

        # Text çiz
        label_dsc = DrawLabelDescriptor()
        label_dsc.color = Colors.WHITE
        label_dsc.opa = Opacity.COVER
        label_dsc.font_size = 20
        label_dsc.align = 'center'

        # Text pozisyonu (center)
        text_pos = Point(
            (draw_area.x1 + draw_area.x2) // 2,
            (draw_area.y1 + draw_area.y2) // 2
        )

        draw_ctx.draw_label(draw_ctx, label_dsc, text_pos, self.text)

    def on_mouse_enter(self):
        """Mouse button üzerine geldiğinde"""
        self.set_state(WidgetState.HOVERED, True)
        self.trigger_event('hover', True)

    def on_mouse_leave(self):
        """Mouse button'dan ayrıldığında"""
        self.set_state(WidgetState.HOVERED, False)
        self.trigger_event('hover', False)

    def on_mouse_press(self):
        """Mouse tıklaması"""
        self.set_state(WidgetState.PRESSED, True)
        self.trigger_event('press', True)

    def on_mouse_release(self):
        """Mouse bırakma"""
        was_pressed = self.has_state(WidgetState.PRESSED)
        self.set_state(WidgetState.PRESSED, False)

        # Click event (sadece button üzerindeyken)
        if was_pressed and self.has_state(WidgetState.HOVERED):
            self.trigger_event('click', None)


# Import Color for gradient
from core.color import Color
