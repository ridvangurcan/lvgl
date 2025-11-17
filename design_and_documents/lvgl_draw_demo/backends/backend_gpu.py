"""
Simulated GPU Backend
GPU hızlandırma simülasyonu - LVGL'deki DMA2D/PXP/VGLite benzeri

Bu backend GPU'yu simüle eder:
- Bazı işlemler için "hardware acceleration" (aslında optimized code)
- Threshold-based CPU fallback
- Async rendering simulation
"""

import pygame
import time
from backends.backend_sw import SoftwareBackend
from core.area import Area, Point
from draw.primitives import DrawPrimitives


class GPUBackend:
    """
    Simulated GPU Backend

    HYBRID RENDERING:
    - Büyük işlemler GPU'da (simulated hardware)
    - Küçük işlemler CPU'da (fallback)
    - LVGL'deki PXP/DMA2D pattern'ini takip eder

    COMPOSITION PATTERN:
    - SW backend'i base olarak kullanır
    - Sadece bazı fonksiyonları override eder
    """

    # GPU threshold - bu değerin altındaki işlemler CPU'da yapılır
    # LVGL'de tipik değer: 5000 pixel
    GPU_THRESHOLD_PIXELS = 5000

    # GPU operasyon simülasyonu için delay (microseconds)
    GPU_OPERATION_DELAY = 0.0001  # 100 us

    @staticmethod
    def init_context(draw_ctx):
        """
        GPU backend initialization
        LVGL'deki lv_draw_pxp_init_ctx() / lv_draw_stm32_dma2d_init_ctx() benzeri

        COMPOSITION PATTERN:
        1. Önce SW backend ile initialize et
        2. GPU-accelerated fonksiyonları override et
        3. SW fonksiyonlarını fallback olarak sakla
        """
        # 1. SW backend ile başla (base implementation)
        SoftwareBackend.init_context(draw_ctx)

        # 2. SW fonksiyonlarını backup olarak sakla
        draw_ctx._sw_draw_rect = draw_ctx.draw_rect
        draw_ctx._sw_blend = draw_ctx.blend

        # 3. GPU fonksiyonlarını override et
        draw_ctx.draw_rect = GPUBackend.draw_rect
        draw_ctx.blend = GPUBackend.blend
        draw_ctx.wait_for_finish = GPUBackend.wait_for_finish

        # GPU state
        draw_ctx._gpu_busy = False
        draw_ctx._gpu_operations = 0

        draw_ctx.backend_name = "Simulated GPU"

        print(f"[GPU Backend] Initialized with threshold: {GPUBackend.GPU_THRESHOLD_PIXELS} pixels")

    @staticmethod
    def draw_rect(draw_ctx, dsc, coords):
        """
        GPU-accelerated rectangle drawing

        THRESHOLD-BASED DECISION:
        - area_size < threshold → CPU rendering (SW fallback)
        - area_size >= threshold → GPU rendering (simulated hardware)
        """
        area_size = coords.get_size()

        if area_size < GPUBackend.GPU_THRESHOLD_PIXELS:
            # Küçük alan - CPU fallback
            # print(f"[GPU] Small rect ({area_size}px) → CPU fallback")
            draw_ctx._sw_draw_rect(draw_ctx, dsc, coords)
            return

        # Büyük alan - GPU acceleration
        # print(f"[GPU] Large rect ({area_size}px) → GPU acceleration")

        # GPU busy simülasyonu
        draw_ctx._gpu_busy = True

        # Simulated GPU operation delay
        time.sleep(GPUBackend.GPU_OPERATION_DELAY)

        # GPU ile hızlı fill (Pygame'in optimize edilmiş fonksiyonları)
        x, y = coords.x1, coords.y1
        width = coords.get_width()
        height = coords.get_height()

        # DMA2D/PXP benzeri hızlı fill
        if dsc.bg_grad_color:
            # Gradient - GPU'da hızlı
            GPUBackend._gpu_fill_gradient(
                draw_ctx.buf,
                x, y, width, height,
                dsc.bg_color,
                dsc.bg_grad_color,
                vertical=(dsc.bg_grad_dir == 0)
            )
        else:
            # Solid fill - GPU'da çok hızlı
            GPUBackend._gpu_fill_solid(
                draw_ctx.buf,
                x, y, width, height,
                dsc.bg_color
            )

        # Border için CPU fallback (border genelde küçük)
        if dsc.border_width > 0:
            border_rect = pygame.Rect(x, y, width, height)
            pygame.draw.rect(
                draw_ctx.buf,
                dsc.border_color.to_rgb(),
                border_rect,
                width=dsc.border_width,
                border_radius=dsc.radius
            )

        draw_ctx._gpu_busy = False
        draw_ctx._gpu_operations += 1

        # İstatistik
        draw_ctx.stats['rect_count'] += 1
        draw_ctx.stats['pixels_drawn'] += area_size

    @staticmethod
    def blend(draw_ctx, src_area, src_color, opacity):
        """
        GPU-accelerated blending
        DMA2D/PXP blend operation benzeri
        """
        area_size = src_area.get_size()

        if area_size < GPUBackend.GPU_THRESHOLD_PIXELS:
            # CPU fallback
            draw_ctx._sw_blend(draw_ctx, src_area, src_color, opacity)
            return

        # GPU blending (simulated)
        draw_ctx._gpu_busy = True
        time.sleep(GPUBackend.GPU_OPERATION_DELAY * 2)  # Blend biraz daha yavaş

        # Pygame'in alpha blend'i kullan (hardware accelerated olabilir)
        temp_surface = pygame.Surface((src_area.get_width(), src_area.get_height()), pygame.SRCALPHA)
        temp_surface.fill((*src_color.to_rgb(), opacity))

        draw_ctx.buf.blit(temp_surface, (src_area.x1, src_area.y1))

        draw_ctx._gpu_busy = False
        draw_ctx._gpu_operations += 1
        draw_ctx.stats['blend_count'] += 1

    @staticmethod
    def wait_for_finish(draw_ctx):
        """
        GPU işlemlerinin bitmesini bekle
        LVGL'deki wait_for_finish() benzeri

        Real hardware'da DMA/GPU tamamlanana kadar bekler
        """
        while draw_ctx._gpu_busy:
            time.sleep(0.00001)  # 10 us polling

    @staticmethod
    def _gpu_fill_solid(surface, x, y, width, height, color):
        """
        GPU solid fill simulation
        DMA2D Register-to-Memory mode benzeri
        """
        rect = pygame.Rect(x, y, width, height)
        pygame.draw.rect(surface, color.to_rgb(), rect)

    @staticmethod
    def _gpu_fill_gradient(surface, x, y, width, height, color_start, color_end, vertical):
        """
        GPU gradient fill simulation
        PXP/VGLite gradient benzeri
        """
        # GPU gradient'i optimize edilmiş
        DrawPrimitives.fill_rect_with_gradient(
            surface, x, y, width, height,
            color_start, color_end, vertical
        )

    @staticmethod
    def get_gpu_stats(draw_ctx):
        """GPU-specific istatistikler"""
        return {
            'gpu_operations': draw_ctx._gpu_operations,
            'gpu_busy': draw_ctx._gpu_busy
        }
