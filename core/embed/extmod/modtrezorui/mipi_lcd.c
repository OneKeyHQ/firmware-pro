#include STM32_HAL_H
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "jpeg_dma.h"
#include "mipi_lcd.h"
#include "sdram.h"
#include "systick.h"

typedef struct {
  uint32_t start_x;
  uint32_t start_y;
  uint32_t end_x;
  uint32_t end_y;
} lcd_window_t;

#define LAYER2_MEMORY_BASE \
  (FMC_SDRAM_LTDC_BUFFER_ADDRESS + MB(1) + KB(512))  // 1.5MB offset

#define TRANSPARENT_STATUSBAR_HEIGHT 44
#define TRANSPARENT_COLOR_KEY 0x0001

static bool g_layer2_initialized = false;

static volatile bool g_animation_in_progress = false;

#define LCD_TXW350135B0

#if 0
#elif defined(LCD_TXW350135B0)
#include "TXW350135B0.c"
#define LCD_init_sequence TXW350135B0_init_sequence
const DisplayParam_t lcd_params = {
    .hres = TXW350135B0_HRES,
    .vres = TXW350135B0_VRES,
    .hsync = TXW350135B0_HSYNC,
    .hfp = TXW350135B0_HFP,
    .hbp = TXW350135B0_HBP,
    .vsync = TXW350135B0_VSYNC,
    .vfp = TXW350135B0_VFP,
    .vbp = TXW350135B0_VBP,
    .pixel_format_ltdc = LTDC_PIXEL_FORMAT_RGB565,
    .pixel_format_dsi = DSI_RGB565,
    .bbp = 2,
    .fb_base = DISPLAY_MEMORY_BASE,
    .ltdc_pll = {
        .PLL3N = 132U,
        .PLL3R = 20U,
        .PLL3FRACN = 0U,
    }};
#elif defined(LCD_TXW700140K0)
#include "TXW700140K0.c"
#define LCD_init_sequence TXW700140K0_init_sequence
const DisplayParam_t lcd_params = {
    .hres = TXW700140K0_HRES,
    .vres = TXW700140K0_VRES,
    .hsync = TXW700140K0_HSYNC,
    .hfp = TXW700140K0_HFP,
    .hbp = TXW700140K0_HBP,
    .vsync = TXW700140K0_VSYNC,
    .vfp = TXW700140K0_VFP,
    .vbp = TXW700140K0_VBP,
    .pixel_format_ltdc = LTDC_PIXEL_FORMAT_RGB565,
    .pixel_format_dsi = DSI_RGB565,
    .bbp = 2,
    .fb_base = DISPLAY_MEMORY_BASE,
    .ltdc_pll = {
        .PLL3N = 43U,
        .PLL3R = 5U,
        .PLL3FRACN = 2048U,
    }};
#else
#error "display selection not defined!"
#endif

static lcd_window_t lcd_window = {
    .start_x = 0,
    .start_y = 0,
    .end_x = lcd_params.hres - 1,
    .end_y = lcd_params.vres - 1,
};

#define LED_PWM_TIM_PERIOD (50)

#define LTDC_FREQ                                          \
  (uint32_t)(HSE_VALUE / 5 *                               \
             (lcd_params.ltdc_pll.PLL3N +                  \
              (lcd_params.ltdc_pll.PLL3FRACN / 8192.0F)) / \
             lcd_params.ltdc_pll.PLL3R / 1000)

#define DSI_FREQ (uint32_t)(HSE_VALUE / 1 * 2 * 40 / 2 / 2 / 8 / 1000)

#define FPS_TARGET                                                     \
  (uint32_t)((float)LTDC_FREQ / ((lcd_params.hres + lcd_params.hbp +   \
                                  lcd_params.hfp + lcd_params.hsync) * \
                                 (lcd_params.vres + lcd_params.vbp +   \
                                  lcd_params.vfp + lcd_params.vsync)))

static int DISPLAY_BACKLIGHT = -1;
static int DISPLAY_ORIENTATION = -1;

static DSI_HandleTypeDef hlcd_dsi = {0};
static DMA2D_HandleTypeDef hlcd_dma2d = {0};
static LTDC_HandleTypeDef hlcd_ltdc = {0};

void DSI_IRQHandler(void) { HAL_DSI_IRQHandler(&hlcd_dsi); }

float lcd_fps = 0.0;

void lcd_pwm_init(void) {
  GPIO_InitTypeDef gpio_init_structure = {0};
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_TIM1_CLK_ENABLE();
  gpio_init_structure.Mode = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
  gpio_init_structure.Alternate = GPIO_AF1_TIM1;
  gpio_init_structure.Pin = LCD_BL_CTRL_PIN;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);

  TIM_HandleTypeDef TIM1_Handle;
  TIM1_Handle.Instance = TIM1;
  TIM1_Handle.Init.Period = LED_PWM_TIM_PERIOD - 1;
  TIM1_Handle.Init.Prescaler = SystemCoreClock / 1000000 / 4 - 1;
  TIM1_Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  TIM1_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TIM1_Handle.Init.RepetitionCounter = 0;
  HAL_TIM_PWM_Init(&TIM1_Handle);

  TIM_OC_InitTypeDef TIM_OC_InitStructure;
  TIM_OC_InitStructure.Pulse = (LED_PWM_TIM_PERIOD / 2 - 1);
  TIM_OC_InitStructure.OCMode = TIM_OCMODE_PWM2;
  TIM_OC_InitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
  TIM_OC_InitStructure.OCFastMode = TIM_OCFAST_DISABLE;
  TIM_OC_InitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  TIM_OC_InitStructure.OCIdleState = TIM_OCIDLESTATE_SET;
  TIM_OC_InitStructure.OCNIdleState = TIM_OCNIDLESTATE_SET;
  HAL_TIM_PWM_ConfigChannel(&TIM1_Handle, &TIM_OC_InitStructure, TIM_CHANNEL_1);

  HAL_TIM_PWM_Start(&TIM1_Handle, TIM_CHANNEL_1);
  HAL_TIMEx_PWMN_Start(&TIM1_Handle, TIM_CHANNEL_1);
}

// Initialize DMA2D peripheral
static HAL_StatusTypeDef dma2d_init(DMA2D_HandleTypeDef* hdma2d) {
  if (hdma2d->Instance != DMA2D) return HAL_ERROR;  // Check instance

  // clock source
  {
    __HAL_RCC_DMA2D_CLK_ENABLE();
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();
  }

  return HAL_OK;
}
// LTDC initialization function
static HAL_StatusTypeDef ltdc_init(LTDC_HandleTypeDef* hltdc) {
  if (hltdc->Instance != LTDC) return HAL_ERROR;

  // Configure LTDC clock source
  {
    __HAL_RCC_LTDC_CLK_ENABLE();     // Enable LTDC clock
    __HAL_RCC_LTDC_FORCE_RESET();    // Reset LTDC peripheral
    __HAL_RCC_LTDC_RELEASE_RESET();  // Release LTDC peripheral reset

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLL3.PLL3M = 5U;  // PLL3 division factor M
    PeriphClkInitStruct.PLL3.PLL3N =
        lcd_params.ltdc_pll.PLL3N;        // PLL3 multiplication factor N
    PeriphClkInitStruct.PLL3.PLL3P = 2U;  // PLL3 division factor P
    PeriphClkInitStruct.PLL3.PLL3Q = 2U;  // PLL3 division factor Q
    PeriphClkInitStruct.PLL3.PLL3R =
        lcd_params.ltdc_pll.PLL3R;  // PLL3 division factor R
    PeriphClkInitStruct.PLL3.PLL3RGE =
        RCC_PLLCFGR_PLL3RGE_2;  // PLL3 input frequency range
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;  // PLL3 wide VCO
    PeriphClkInitStruct.PLL3.PLL3FRACN =
        lcd_params.ltdc_pll.PLL3FRACN;  // PLL3 fractional division
    HAL_StatusTypeDef result = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    if (result != HAL_OK) {
      return result;
    }
  }

  // Configure polarity parameters
  hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AH;  // Horizontal sync polarity
  // hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AL;

  hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AH;  // Vertical sync polarity
  // hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AL;

  // hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AH;
  hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AL;  // Data enable polarity

  hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;  // Pixel clock polarity

  // Configure timing parameters
  hltdc->Init.HorizontalSync = lcd_params.hsync - 1;  // Horizontal sync width
  hltdc->Init.AccumulatedHBP =
      lcd_params.hsync + lcd_params.hbp - 1;  // Horizontal sync + back porch
  hltdc->Init.AccumulatedActiveW =
      lcd_params.hsync + lcd_params.hres + lcd_params.hbp -
      1;  // Horizontal sync + back porch + active width
  hltdc->Init.TotalWidth = lcd_params.hsync + lcd_params.hres + lcd_params.hbp +
                           lcd_params.hfp - 1;      // Total width
  hltdc->Init.VerticalSync = lcd_params.vsync - 1;  // Vertical sync width
  hltdc->Init.AccumulatedVBP =
      lcd_params.vsync + lcd_params.vbp - 1;  // Vertical sync + back porch
  hltdc->Init.AccumulatedActiveH =
      lcd_params.vsync + lcd_params.vres + lcd_params.vbp -
      1;  // Vertical sync + back porch + active height
  hltdc->Init.TotalHeigh = lcd_params.vsync + lcd_params.vres + lcd_params.vbp +
                           lcd_params.vfp - 1;  // Total height

  // Keep original black background to avoid affecting normal display
  hltdc->Init.Backcolor.Blue = 0x00;
  hltdc->Init.Backcolor.Green = 0x00;
  hltdc->Init.Backcolor.Red = 0x00;

  return HAL_LTDC_Init(hltdc);
}

// Configure LTDC layer parameters
static HAL_StatusTypeDef ltdc_layer_config(LTDC_HandleTypeDef* hltdc,
                                           uint32_t layer_index,
                                           LTDC_LAYERCONFIG* config) {
  LTDC_LayerCfgTypeDef pLayerCfg;

  // Set window start and end coordinates
  pLayerCfg.WindowX0 = config->x0;  // Window top-left X coordinate
  pLayerCfg.WindowX1 = config->x1;  // Window bottom-right X coordinate
  pLayerCfg.WindowY0 = config->y0;  // Window top-left Y coordinate
  pLayerCfg.WindowY1 = config->y1;  // Window bottom-right Y coordinate
  // Set pixel format
  pLayerCfg.PixelFormat = config->pixel_format;  // Pixel format
  // Set frame buffer start address
  pLayerCfg.FBStartAdress = config->address;  // Frame buffer start address
  // Set image width and height
  pLayerCfg.ImageWidth = (config->x1 - config->x0);   // Image width
  pLayerCfg.ImageHeight = (config->y1 - config->y0);  // Image height
  // Set background color to black
  pLayerCfg.Backcolor.Blue = 0;   // Background blue component
  pLayerCfg.Backcolor.Green = 0;  // Background green component
  pLayerCfg.Backcolor.Red = 0;    // Background red component

  // Simplified blending configuration: all layers use standard opaque settings
  pLayerCfg.Alpha = 255;  // Fully opaque
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  // Configure layer
  HAL_StatusTypeDef result =
      HAL_LTDC_ConfigLayer(hltdc, &pLayerCfg, layer_index);
  return result;
}

// Initialize DSI host
static HAL_StatusTypeDef dsi_host_init(DSI_HandleTypeDef* hdsi) {
  if (hdsi->Instance != DSI) return HAL_ERROR;

  // Configure DSI clock source
  {
    /** Enable DSI host and wrapper clocks */
    __HAL_RCC_DSI_CLK_ENABLE();     // Enable DSI clock
    __HAL_RCC_DSI_FORCE_RESET();    // DSI reset
    __HAL_RCC_DSI_RELEASE_RESET();  // Release DSI reset

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection =
        RCC_PERIPHCLK_DSI;  // Select DSI peripheral clock
    PeriphClkInitStruct.DsiClockSelection =
        RCC_DSICLKSOURCE_PHY;  // Select DSI clock source as PHY
    // PeriphClkInitStruct.DsiClockSelection = RCC_DSICLKSOURCE_PLL2; // PLL2Q =
    // 9 in sdram.c
    HAL_StatusTypeDef result = HAL_RCCEx_PeriphCLKConfig(
        &PeriphClkInitStruct);  // Configure peripheral clock
    if (result != HAL_OK) {
      return result;
    }
  }

  // Configure DSI clock division
  {
    hdsi->Init.AutomaticClockLaneControl =
        DSI_AUTO_CLK_LANE_CTRL_DISABLE;  // Disable automatic clock lane control
    hdsi->Init.TXEscapeCkdiv =
        4;  // lp txclkesc 15.625 Mhz, low power escape clock division
    hdsi->Init.NumberOfLanes = DSI_TWO_DATA_LANES;  // Use 2 data lanes

    // Configure DSI PLL, lane byte rate 62.5MHz (maximum supported)
    DSI_PLLInitTypeDef PLLInit = {0};
    PLLInit.PLLNDIV = 40;               // PLL multiplication factor
    PLLInit.PLLIDF = DSI_PLL_IN_DIV1;   // PLL input division
    PLLInit.PLLODF = DSI_PLL_OUT_DIV2;  // PLL output division
    if (HAL_DSI_Init(hdsi, &PLLInit) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  // Configure DSI timeout parameters
  {
    DSI_HOST_TimeoutTypeDef HostTimeouts = {0};
    HostTimeouts.TimeoutCkdiv = 1;  // Timeout clock division
    HostTimeouts.HighSpeedTransmissionTimeout =
        0;  // High speed transmission timeout
    HostTimeouts.LowPowerReceptionTimeout = 0;  // Low power reception timeout
    HostTimeouts.HighSpeedReadTimeout = 0;      // High speed read timeout
    HostTimeouts.LowPowerReadTimeout = 0;       // Low power read timeout
    HostTimeouts.HighSpeedWriteTimeout = 0;     // High speed write timeout
    HostTimeouts.HighSpeedWritePrespMode =
        DSI_HS_PM_DISABLE;  // High speed write pre-response mode
    HostTimeouts.LowPowerWriteTimeout = 0;  // Low power write timeout
    HostTimeouts.BTATimeout = 0;            // BTA timeout
    if (HAL_DSI_ConfigHostTimeouts(hdsi, &HostTimeouts) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  // Configure DSI PHY timing parameters
  {
    DSI_PHY_TimerTypeDef PhyTimings = {0};
    PhyTimings.ClockLaneHS2LPTime = 27;  // Clock lane HS to LP transition time
    PhyTimings.ClockLaneLP2HSTime = 32;  // Clock lane LP to HS transition time
    PhyTimings.DataLaneHS2LPTime = 15;   // Data lane HS to LP transition time
    PhyTimings.DataLaneLP2HSTime = 24;   // Data lane LP to HS transition time
    PhyTimings.DataLaneMaxReadTime = 0;  // Data lane maximum read time
    PhyTimings.StopWaitTime = 0;         // Stop wait time
    if (HAL_DSI_ConfigPhyTimer(hdsi, &PhyTimings) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  // Configure DSI flow control, low power reception filter and error monitoring
  if (HAL_DSI_ConfigFlowControl(hdsi, DSI_FLOW_CONTROL_BTA) != HAL_OK) {
    return HAL_ERROR;
  }
  if (HAL_DSI_SetLowPowerRXFilter(hdsi, 10000) != HAL_OK) {
    return HAL_ERROR;
  }
  if (HAL_DSI_ConfigErrorMonitor(hdsi, HAL_DSI_ERROR_GEN) != HAL_OK) {
    return HAL_ERROR;
  }

  // Configure DSI video mode parameters
  {
    DSI_VidCfgTypeDef VidCfg = {0};
    VidCfg.VirtualChannelID = 0;                        // Virtual channel ID
    VidCfg.ColorCoding = lcd_params.pixel_format_dsi;   // Color coding
    VidCfg.LooselyPacked = DSI_LOOSELY_PACKED_DISABLE;  // Not loosely packed
    VidCfg.Mode = DSI_VID_MODE_BURST;                   // Burst mode
    VidCfg.PacketSize = lcd_params.hres;                // Pixels per line
    VidCfg.NumberOfChunks = 0;  // No chunking needed in burst mode
    VidCfg.NullPacketSize = 0;  // No null packets needed in burst mode
    VidCfg.HSPolarity = DSI_HSYNC_ACTIVE_HIGH;  // Horizontal sync polarity
    VidCfg.VSPolarity = DSI_VSYNC_ACTIVE_HIGH;  // Vertical sync polarity
    VidCfg.DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH;  // Data enable polarity
    VidCfg.HorizontalSyncActive =
        (lcd_params.hsync * DSI_FREQ) / LTDC_FREQ;  // Horizontal sync width
    VidCfg.HorizontalBackPorch =
        (lcd_params.hbp * DSI_FREQ) / LTDC_FREQ;  // Horizontal back porch
    VidCfg.HorizontalLine = ((lcd_params.hres + lcd_params.hsync +
                              lcd_params.hbp + lcd_params.hfp) *
                             DSI_FREQ) /
                            LTDC_FREQ;             // Total line period
    VidCfg.VerticalSyncActive = lcd_params.vsync;  // Vertical sync width
    VidCfg.VerticalBackPorch = lcd_params.vbp;     // Vertical back porch
    VidCfg.VerticalFrontPorch = lcd_params.vfp;    // Vertical front porch
    VidCfg.VerticalActive = lcd_params.vres;       // Active lines

    // Low power mode related configuration
    VidCfg.LPCommandEnable =
        DSI_LP_COMMAND_ENABLE;           // Enable low power commands
    VidCfg.LPLargestPacketSize = 128;    // Low power largest packet size
    VidCfg.LPVACTLargestPacketSize = 0;  // Low power VACT largest packet size
    VidCfg.LPHorizontalFrontPorchEnable =
        DSI_LP_HFP_ENABLE;  // Enable low power HFP
    VidCfg.LPHorizontalBackPorchEnable =
        DSI_LP_HBP_ENABLE;  // Enable low power HBP
    VidCfg.LPVerticalActiveEnable =
        DSI_LP_VACT_ENABLE;  // Enable low power VACT
    VidCfg.LPVerticalFrontPorchEnable =
        DSI_LP_VFP_ENABLE;  // Enable low power VFP
    VidCfg.LPVerticalBackPorchEnable =
        DSI_LP_VBP_ENABLE;  // Enable low power VBP
    VidCfg.LPVerticalSyncActiveEnable =
        DSI_LP_VSYNC_ENABLE;  // Enable low power VSYNC
    VidCfg.FrameBTAAcknowledgeEnable =
        DSI_FBTAA_DISABLE;  // Disable BTA frame acknowledgment

    if (HAL_DSI_ConfigVideoMode(hdsi, &VidCfg) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

#define CONVERTRGB5652ARGB8888(Color)                                   \
  ((((((((Color) >> (11U)) & 0x1FU) * 527U) + 23U) >> (6U)) << (16U)) | \
   (((((((Color) >> (5U)) & 0x3FU) * 259U) + 33U) >> (6U)) << (8U)) |   \
   (((((Color)&0x1FU) * 527U) + 23U) >> (6U)) | (0xFF000000U))

// Write pixel value to specified coordinates (x_pos, y_pos)
void fb_write_pixel(uint32_t x_pos, uint32_t y_pos, uint32_t color) {
  if (x_pos < lcd_window.start_x || x_pos > lcd_window.end_x ||
      y_pos < lcd_window.start_y || y_pos > lcd_window.end_y) {
    return;
  }
  if (lcd_params.pixel_format_ltdc == LTDC_PIXEL_FORMAT_ARGB8888) {
    *(uint32_t*)(lcd_params.fb_base +
                 (lcd_params.bbp * (lcd_params.hres * y_pos + x_pos))) = color;
  } else {
    /* LTDC pixel format is RGB565 */
    *(uint16_t*)(lcd_params.fb_base +
                 (lcd_params.bbp * (lcd_params.hres * y_pos + x_pos))) = color;
  }
}

// Fill buffer using DMA2D
static void fb_fill_buffer(uint32_t* dest, uint32_t x_size, uint32_t y_size,
                           uint32_t offset, uint32_t color) {
  uint32_t output_color_mode, input_color = color;

  switch (lcd_params.pixel_format_ltdc) {
    case LTDC_PIXEL_FORMAT_RGB565:
      output_color_mode = DMA2D_OUTPUT_RGB565;      /* RGB565 output mode */
      input_color = CONVERTRGB5652ARGB8888(color);  // Convert to ARGB8888
      break;
    case LTDC_PIXEL_FORMAT_RGB888:
    default:
      output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 output mode */
      break;
  }

  /* Configure DMA2D to register-to-memory mode, color mode as output_color_mode
   */
  hlcd_dma2d.Init.Mode = DMA2D_R2M;
  hlcd_dma2d.Init.ColorMode = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = offset;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D initialization */
  if (HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK) {
    if (HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK) {
      if (HAL_DMA2D_Start(&hlcd_dma2d, input_color, (uint32_t)dest, x_size,
                          y_size) == HAL_OK) {
        /* Poll for DMA transfer completion */
        (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 25);
      }
    }
  }
}

// Fill rectangular area
void fb_fill_rect(uint32_t x_pos, uint32_t y_pos, uint32_t width,
                  uint32_t height, uint32_t color) {
  uint32_t start_x, start_y, end_x, end_y;
  if (x_pos + width < lcd_window.start_x || x_pos > lcd_window.end_x) {
    return;
  }
  if (y_pos + height < lcd_window.start_y || y_pos > lcd_window.end_y) {
    return;
  }
  start_x = x_pos < lcd_window.start_x ? lcd_window.start_x : x_pos;
  start_y = y_pos < lcd_window.start_y ? lcd_window.start_y : y_pos;
  end_x = x_pos + width - 1;
  end_y = y_pos + height - 1;
  end_x = end_x > lcd_window.end_x ? lcd_window.end_x : end_x;
  end_y = end_y > lcd_window.end_y ? lcd_window.end_y : end_y;
  /* Get the rectangle start address */
  uint32_t address = lcd_params.fb_base +
                     ((lcd_params.bbp) * (lcd_params.hres * start_y + start_x));

  /* Fill the rectangle */
  fb_fill_buffer((uint32_t*)address, end_x - start_x + 1, end_y - start_y + 1,
                 (lcd_params.hres - (end_x - start_x + 1)), color);
}

void fb_draw_hline(uint32_t x_pos, uint32_t y_pos, uint32_t len,
                   uint32_t color) {
  if (y_pos > lcd_window.end_y || y_pos < lcd_window.start_y) {
    return;
  }
  uint32_t address = lcd_params.fb_base +
                     ((lcd_params.bbp) * (lcd_params.hres * y_pos + x_pos));
  fb_fill_buffer((uint32_t*)address, len, 1, 0, color);
}

void fb_draw_vline(uint32_t x_pos, uint32_t y_pos, uint32_t len,
                   uint32_t color) {
  uint32_t address = lcd_params.fb_base +
                     ((lcd_params.bbp) * (lcd_params.hres * y_pos + x_pos));
  fb_fill_buffer((uint32_t*)address, 1, len, lcd_params.hres - 1, color);
}

// Copy buffer using DMA2D
void dma2d_copy_buffer(uint32_t* pSrc, uint32_t* pDst, uint16_t x, uint16_t y,
                       uint16_t xsize, uint16_t ysize) {
  uint32_t destination =
      (uint32_t)pDst + (y * lcd_params.hres + x) * (lcd_params.bbp);
  uint32_t source = (uint32_t)pSrc;

  /*##-1- Configure DMA2D mode, color mode and output offset #############*/
  hlcd_dma2d.Init.Mode = DMA2D_M2M;
  hlcd_dma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hlcd_dma2d.Init.OutputOffset = lcd_params.hres - xsize;
  hlcd_dma2d.Init.AlphaInverted =
      DMA2D_REGULAR_ALPHA; /* Do not invert output Alpha */
  hlcd_dma2d.Init.RedBlueSwap =
      DMA2D_RB_REGULAR; /* Do not swap output red-blue */

  /*##-2- DMA2D callback configuration ######################################*/
  hlcd_dma2d.XferCpltCallback = NULL;

  /*##-3- Foreground layer configuration
   * ###########################################*/
  hlcd_dma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hlcd_dma2d.LayerCfg[1].InputAlpha = 0xFF;
  hlcd_dma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
  hlcd_dma2d.LayerCfg[1].InputOffset = 0;
  hlcd_dma2d.LayerCfg[1].RedBlueSwap =
      DMA2D_RB_REGULAR; /* Do not swap foreground red-blue */
  hlcd_dma2d.LayerCfg[1].AlphaInverted =
      DMA2D_REGULAR_ALPHA; /* Do not invert foreground Alpha */

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D initialization */
  if (HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK) {
    if (HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK) {
      // while ( lcd_ltdc_busy() ) {}
      if (HAL_DMA2D_Start(&hlcd_dma2d, source, destination, xsize, ysize) ==
          HAL_OK) {
        /* Poll for DMA transfer completion */
        HAL_DMA2D_PollForTransfer(&hlcd_dma2d, HAL_MAX_DELAY);
      }
    }
  }
}

void dma2d_copy_ycbcr_to_rgb(uint32_t* pSrc, uint32_t* pDst,
                             uint16_t copy_width, uint16_t copy_height,
                             uint32_t ChromaSampling,
                             uint16_t output_line_width,
                             uint16_t input_line_width) {
  if (input_line_width == 0) {
    input_line_width = copy_width;
  }
  if (copy_width > input_line_width) {
    copy_width = input_line_width;
  }

  uint32_t cssMode = DMA2D_CSS_420;
  uint32_t alignmentOffset = 0;

  if (ChromaSampling == JPEG_420_SUBSAMPLING) {
    cssMode = DMA2D_CSS_420;
    alignmentOffset = input_line_width % 16;
    if (alignmentOffset != 0) {
      alignmentOffset = 16 - alignmentOffset;
    }
  } else if (ChromaSampling == JPEG_444_SUBSAMPLING) {
    cssMode = DMA2D_NO_CSS;
    alignmentOffset = input_line_width % 8;
    if (alignmentOffset != 0) {
      alignmentOffset = 8 - alignmentOffset;
    }
  } else if (ChromaSampling == JPEG_422_SUBSAMPLING) {
    cssMode = DMA2D_CSS_422;
    alignmentOffset = input_line_width % 16;
    if (alignmentOffset != 0) {
      alignmentOffset = 16 - alignmentOffset;
    }
  }

  // Skip any pixels that we intentionally crop from the right side so that
  // Layer2 never overruns the 480 px area.
  uint32_t croppedOffset = 0;
  if (input_line_width > copy_width) {
    croppedOffset = input_line_width - copy_width;
  }

  // Calculate output offset for proper line alignment.
  uint32_t outputLineOffset = 0;
  if (output_line_width > copy_width) {
    outputLineOffset = output_line_width - copy_width;
  }

  /*##-1- Configure DMA2D mode, color mode and output offset #############*/
  hlcd_dma2d.Init.Mode = DMA2D_M2M_PFC;
  hlcd_dma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hlcd_dma2d.Init.OutputOffset = outputLineOffset;
  hlcd_dma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
  hlcd_dma2d.Init.RedBlueSwap = DMA2D_RB_REGULAR;

  /*##-2- DMA2D callback configuration ######################################*/
  hlcd_dma2d.XferCpltCallback = NULL;

  /*##-3- Foreground layer configuration
   * ###########################################*/
  hlcd_dma2d.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  hlcd_dma2d.LayerCfg[1].InputAlpha = 0xFF;
  hlcd_dma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR;
  hlcd_dma2d.LayerCfg[1].ChromaSubSampling = cssMode;
  hlcd_dma2d.LayerCfg[1].InputOffset = alignmentOffset + croppedOffset;
  hlcd_dma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
  hlcd_dma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;

  hlcd_dma2d.Instance = DMA2D;

  HAL_StatusTypeDef dma2d_result = HAL_DMA2D_Init(&hlcd_dma2d);
  if (dma2d_result != HAL_OK) {
    return;
  }

  dma2d_result = HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1);
  if (dma2d_result != HAL_OK) {
    return;
  }

  dma2d_result = HAL_DMA2D_Start(&hlcd_dma2d, (uint32_t)pSrc, (uint32_t)pDst,
                                 copy_width, copy_height);
  if (dma2d_result != HAL_OK) {
    return;
  }

  dma2d_result = HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 1000);
  if (dma2d_result != HAL_OK) {
    return;
  }
}

// Send DSI DCS write command
int DSI_DCS_write(uint16_t cmd, uint8_t* data, uint16_t data_len) {
  // If data length is <= 1, use short packet write command
  if (data_len <= 1) {
    return HAL_DSI_ShortWrite(&hlcd_dsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, cmd,
                              (uint32_t)data[0]);
  } else {
    // Otherwise use long packet write command
    return HAL_DSI_LongWrite(&hlcd_dsi, 0, DSI_DCS_LONG_PKT_WRITE, data_len,
                             cmd, data);
  }
}

// Set display backlight brightness (0~255) and return current backlight value
int display_backlight(int val) {
  // Only update when brightness value changes and is within valid range
  if (DISPLAY_BACKLIGHT != val && val >= 0 && val <= 255) {
    DISPLAY_BACKLIGHT = val;
    // Set PWM duty cycle to adjust backlight
    TIM1->CCR1 = (LED_PWM_TIM_PERIOD - 1) * val / 255;
  }
  return DISPLAY_BACKLIGHT;
}

// Set backlight brightness and automatically reset/restore LCD when turning
// on/off
int display_backlight_with_lcd_reset(int val) {
  // When turning off backlight and current is not 0, turn off backlight first
  // then suspend refresh
  if (val == 0 && DISPLAY_BACKLIGHT != 0) {
    display_backlight(0);
    lcd_refresh_suspend();
  } else if (val > 0 && DISPLAY_BACKLIGHT == 0) {
    lcd_refresh_resume();
    HAL_Delay(5);  // Wait for LCD recovery
  }
  return display_backlight(val);
}

int display_orientation(int degrees) {
  if (degrees != DISPLAY_ORIENTATION) {
    if (degrees == 0 || degrees == 90 || degrees == 180 || degrees == 270) {
      DISPLAY_ORIENTATION = degrees;
    }
  }
  return DISPLAY_ORIENTATION;
}

// LCD initialization function
void lcd_init(void) {
  // GPIO initialization
  {
    GPIO_InitTypeDef gpio_init_structure = {0};

    // RESET pin initialization
    __HAL_RCC_GPIOG_CLK_ENABLE();
    gpio_init_structure.Pin = LCD_RESET_PIN;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_RESET_GPIO_PORT, &gpio_init_structure);

    // TE pin initialization
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    gpio_init_structure.Pin = LCD_TE_PIN;
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_TE_GPIO_PORT, &gpio_init_structure);
    // HAL_GPIO_WritePin(LCD_TE_GPIO_PORT, LCD_TE_PIN, GPIO_PIN_SET); // TODO:
    // is this needed?
  }

  // DMA2D initialization
  {
    hlcd_dma2d.Instance = DMA2D;
    dma2d_init(&hlcd_dma2d);
  }

  {
    hlcd_ltdc.Instance = LTDC;
    ltdc_init(&hlcd_ltdc);

    LTDC_LAYERCONFIG config;
    config.x0 = 0;
    config.x1 = lcd_params.hres;
    config.y0 = 0;
    config.y1 = lcd_params.vres;
    config.pixel_format = lcd_params.pixel_format_ltdc;
    config.address = DISPLAY_MEMORY_BASE;
    ltdc_layer_config(&hlcd_ltdc, 0, &config);
    memset((void*)DISPLAY_MEMORY_BASE, 0,
           lcd_params.hres * lcd_params.vres * 2);
  }

  {
    hlcd_dsi.Instance = DSI;
    dsi_host_init(&hlcd_dsi);
  }

  // LCD initialization process
  {
    // Reset LCD
    HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(120);
    HAL_DSI_Start(&hlcd_dsi);  // Ensure LTDC is initialized before starting DSI

    int result = LCD_init_sequence(DSI_DCS_write, HAL_Delay);
    if (result != 0) {
      while (1)
        ;
    }
  }

  lcd_add_second_layer();
}

// Refresh display (not implemented)
void display_refresh(void) {}

// Set display window (not implemented)
void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {}

void display_reset_state() {}

void display_clear_save(void) {}

const char* display_save(const char* prefix) { return NULL; }

// Convert grayscale value to RGB565 format (inverted)
static uint16_t grayscale_to_rgb565(uint8_t gray) {
  uint16_t r = (gray * 31 + 127) / 255;
  uint16_t g = (gray * 63 + 127) / 255;
  uint16_t b = (gray * 31 + 127) / 255;

  return 0xffff - ((r << 11) | (g << 5) | b);
}

// Draw pixel block according to grayscale data
void display_fp(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                const uint8_t* data) {
  for (uint32_t i = 0; i < w * h; i++) {
    fb_write_pixel(x + i % w, y + i / w, grayscale_to_rgb565(data[i]));
  }
}

// Check if LTDC is busy (low level means busy)
int lcd_ltdc_busy(void) {
  hlcd_ltdc.Instance = LTDC;
  // low is busy
  return hlcd_ltdc.Instance->CDSR & 0x01 ? 0 : 1;
}

static void ltdc_reload_on_vertical_blank(void) {
  hlcd_ltdc.Instance = LTDC;

  __HAL_LTDC_VERTICAL_BLANKING_RELOAD_CONFIG(&hlcd_ltdc);

  const uint32_t timeout_ms = 2;
  uint32_t start = HAL_GetTick();
  while ((hlcd_ltdc.Instance->SRCR & LTDC_SRCR_VBR) != 0U) {
    if ((HAL_GetTick() - start) > timeout_ms) {
      __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hlcd_ltdc);
      break;
    }
  }
}

static inline void ltdc_reload_immediate(void) {
  hlcd_ltdc.Instance = LTDC;
  __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hlcd_ltdc);
}

// Disable LTDC and DSI (wait for transfer completion)
void lcd_ltdc_dsi_disable(void) {
  hlcd_ltdc.Instance = LTDC;
  hlcd_dsi.Instance = DSI;
  while (lcd_ltdc_busy()) {
  }
  HAL_DSI_Stop(&hlcd_dsi);
  __HAL_LTDC_DISABLE(&hlcd_ltdc);
}

// Enable LTDC and DSI
void lcd_ltdc_dsi_enable(void) {
  hlcd_ltdc.Instance = LTDC;
  hlcd_dsi.Instance = DSI;
  __HAL_LTDC_ENABLE(&hlcd_ltdc);
  HAL_DSI_Start(&hlcd_dsi);
}

// Suspend LCD refresh (disable LTDC/DSI first, then reset LCD, wait for
// complete blanking)
void lcd_refresh_suspend(void) {
  // Wait for transfer completion
  lcd_ltdc_dsi_disable();

  // Reset LCD
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_RESET);
  // Wait for complete blanking, 120ms needed in Sleep Out mode
  HAL_Delay(125);
}

void lcd_refresh_resume(void) {  // Resume LCD refresh
  // lcd reset // Reset LCD
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN,
                    GPIO_PIN_RESET);  // Pull reset pin low
  HAL_Delay(5);                       // Delay 5 milliseconds
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN,
                    GPIO_PIN_SET);  // Pull reset pin high
  HAL_Delay(50);                    // Delay 50 milliseconds
  lcd_ltdc_dsi_enable();
  int result = LCD_init_sequence(DSI_DCS_write, HAL_Delay);
  if (result != 0) {
    while (1)
      ;
  }
}

static volatile uint32_t g_current_display_addr =
    FMC_SDRAM_LTDC_BUFFER_ADDRESS;  // Current display frame buffer address

void lcd_set_src_addr(uint32_t addr) {    // Set LTDC source address
  static uint32_t animation_counter = 0;  // Animation counter
  static uint32_t last_addr = 0;          // Last set address

  if (addr == last_addr) {
    return;
  }

  last_addr = addr;  // Update the last address

  hlcd_ltdc.Instance = LTDC;    // Set LTDC instance
  LTDC_LAYERCONFIG config;      // Define layer configuration struct
  config.x0 = 0;                // Layer start X coordinate is 0
  config.x1 = lcd_params.hres;  // Layer end X coordinate is screen width
  config.y0 = 0;                // Layer start Y coordinate is 0
  config.y1 = lcd_params.vres;  // Layer end Y coordinate is screen height
  config.pixel_format = lcd_params.pixel_format_ltdc;
  config.address = addr;
  ltdc_layer_config(&hlcd_ltdc, 0, &config);

  __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 0);  // Enable Layer0

  // Use VSync reload to reduce flicker
  __HAL_LTDC_RELOAD_CONFIG(
      &hlcd_ltdc);  // Trigger LTDC reload (VSync synchronized)

  if (!g_animation_in_progress) {
    animation_counter++;
    if (animation_counter % 2 == 0) {
      lcd_ensure_second_layer();
    }
  }
  // Update current display address
}

uint32_t lcd_get_src_addr(void) { return g_current_display_addr; }

void lcd_add_second_layer(void) {
  if (g_layer2_initialized) {
    return;
  }

  LTDC_LAYERCONFIG config;
  config.x0 = 0;                // Layer start X coordinate is 0
  config.x1 = lcd_params.hres;  // Layer end X coordinate is screen width
  config.y0 = 0;                // Display from 0, covering full screen
  config.y1 = lcd_params.vres;  // Layer end Y coordinate is screen height
  config.pixel_format = lcd_params.pixel_format_ltdc;  // Set pixel format
  config.address = LAYER2_MEMORY_BASE;

  if (ltdc_layer_config(&hlcd_ltdc, 1, &config) != HAL_OK) {
    return;
  }

  lcd_cover_background_init();  // Initialize CoverBackground content

  hlcd_ltdc.Instance = LTDC;  // Set LTDC instance

  __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 0);  // Enable Layer0

  HAL_LTDC_SetAlpha(&hlcd_ltdc, 255, 1);
  __HAL_LTDC_LAYER_DISABLE(&hlcd_ltdc, 1);

  g_layer2_initialized = true;  // Mark Layer2 as initialized

  // Initialize animation system
  lcd_animation_init();  // Initialize animation system

  // Use VSync reload to avoid interfering with normal display
  __HAL_LTDC_RELOAD_CONFIG(
      &hlcd_ltdc);  // Trigger LTDC reload (VSync synchronized)
}

// CoverBackground layer state - hardware layer implementation
static struct {
  bool visible;       // Visibility state
  uint8_t opacity;    // 0-255 opacity level
  int32_t y_offset;   // Y-axis offset, -60 for hidden position, 0 for visible
                      // position
  bool is_animating;  // Animation state
} cover_bg_state = {
    false, 0, -800,
    false};  // Initial state: hidden, transparent, positioned above screen

// Initialize CoverBackground content
void lcd_cover_background_init(void) {  // Initialize CoverBackground content
  uint16_t* layer2_buffer = (uint16_t*)LAYER2_MEMORY_BASE;
  uint32_t buffer_size = lcd_params.hres * lcd_params.vres;

  for (uint32_t i = 0; i < buffer_size; i++) {
    layer2_buffer[i] = 0x0000;
  }

  // Wait for LTDC idle to avoid conflicts with ongoing display operations
  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  // Wait for DMA2D idle to ensure previous operations are complete
  while (HAL_DMA2D_GetState(&hlcd_dma2d) == HAL_DMA2D_STATE_BUSY) {
    HAL_Delay(1);
  }

  // Safely copy current Layer1 display content using DMA2D
  if (g_current_display_addr != 0) {  // If current display address is valid
    // Use DMA2D to copy entire Layer1 content to Layer2 for background
    // consistency
    dma2d_copy_buffer((uint32_t*)g_current_display_addr,
                      (uint32_t*)LAYER2_MEMORY_BASE, 0, 0, lcd_params.hres,
                      lcd_params.vres);  // DMA2D copy
  } else {
    // If no Layer1 content, use default Layer1 frame buffer
    dma2d_copy_buffer((uint32_t*)FMC_SDRAM_LTDC_BUFFER_ADDRESS,
                      (uint32_t*)LAYER2_MEMORY_BASE, 0, 0, lcd_params.hres,
                      lcd_params.vres);  // DMA2D copy
  }

  // Wait for DMA2D copy completion to ensure Layer2 content is ready
  while (HAL_DMA2D_GetState(&hlcd_dma2d) != HAL_DMA2D_STATE_READY) {
    HAL_Delay(1);
  }

  // Verify Layer2 content after initialization
  uint32_t total_non_black = 0;
  for (uint32_t i = 0; i < buffer_size; i++) {
    if (layer2_buffer[i] != 0x0000) {
      total_non_black++;
    }
  }

  // Pre-configure Layer2 but keep disabled to avoid initialization flicker
  hlcd_ltdc.Instance = LTDC;
  HAL_LTDC_SetAlpha(&hlcd_ltdc, 0, 1);      // Set to completely transparent
  __HAL_LTDC_LAYER_DISABLE(&hlcd_ltdc, 1);  // Disable Layer2

  // Layer2 content is now ready, will display only when show is called
}

// Optimized seamless display to avoid flicker
__attribute__((used)) void lcd_cover_background_show(void) {
  if (!g_layer2_initialized) {
    return;
  }

  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  hlcd_ltdc.Instance = LTDC;

  // Ensure Layer1 (Layer2 overlay) is initially completely disabled and
  // transparent
  __HAL_LTDC_LAYER_DISABLE(&hlcd_ltdc, 1);
  HAL_LTDC_SetAlpha(&hlcd_ltdc, 0, 1);  // Layer1 initially transparent

  // Force reload to ensure disabled state takes effect
  __HAL_LTDC_RELOAD_CONFIG(&hlcd_ltdc);

  // Wait for reset completion to ensure hardware state is stable
  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  // Clean SDRAM cache to ensure LTDC reads the latest data
  SCB_CleanDCache_by_Addr((uint32_t*)LAYER2_MEMORY_BASE,
                          lcd_params.hres * lcd_params.vres * lcd_params.bbp);

  // Memory barrier to ensure cache operations complete
  __DSB();
  __ISB();

  // Pre-configure Layer2 state
  cover_bg_state.visible = true;
  cover_bg_state.y_offset = 0;
  cover_bg_state.opacity = 255;

  LTDC_LAYERCONFIG config;
  config.x0 = 0;
  config.x1 = lcd_params.hres;
  config.y0 = 0;
  config.y1 = lcd_params.vres;
  config.pixel_format = lcd_params.pixel_format_ltdc;
  config.address = LAYER2_MEMORY_BASE;

  HAL_LTDC_SetAlpha(&hlcd_ltdc, 0, 1);
  ltdc_layer_config(&hlcd_ltdc, 1, &config);
  __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 1);

  __HAL_LTDC_RELOAD_CONFIG(&hlcd_ltdc);

  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  volatile uint32_t timeout = 10000;
  while (timeout-- > 0) {
    if (hlcd_ltdc.Instance->CDSR & 0x01) {  // Check VSync status
      break;
    }
  }

  // During VSync only modify Alpha value from 0 to 255 for atomic switching
  HAL_LTDC_SetAlpha(&hlcd_ltdc, 255, 1);
  __HAL_LTDC_RELOAD_CONFIG(&hlcd_ltdc);
}

// Hide CoverBackground - completely disable Layer2
__attribute__((used)) void lcd_cover_background_hide(void) {
  if (!g_layer2_initialized) {
    return;
  }

  // Wait for LTDC idle
  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  cover_bg_state.visible = false;
  cover_bg_state.opacity = 0;
  cover_bg_state.y_offset = -800;

  // First move layer to hidden position, then disable Layer2
  lcd_cover_background_move_to_y(-800);

  hlcd_ltdc.Instance = LTDC;
  __HAL_LTDC_LAYER_DISABLE(&hlcd_ltdc, 1);  // Completely disable Layer2

  __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hlcd_ltdc);
}

// Set CoverBackground visibility state - does not change hardware, only updates
// state
__attribute__((used)) void lcd_cover_background_set_visible(bool visible) {
  if (!g_layer2_initialized) {
    return;
  }

  cover_bg_state.visible = visible;
}

// Set CoverBackground image data
__attribute__((used)) void lcd_cover_background_set_image(
    const void* image_data, uint32_t image_size) {
  if (!g_layer2_initialized) {
    return;
  }

  uint16_t* layer2_buffer = (uint16_t*)LAYER2_MEMORY_BASE;
  uint32_t max_pixels = lcd_params.hres * lcd_params.vres;
  uint32_t max_bytes = max_pixels * 2;

  uint32_t copy_size = (image_size > max_bytes) ? max_bytes : image_size;

  memcpy(layer2_buffer, image_data, copy_size);
}

// Load JPEG image to CoverBackground hardware layer
__attribute__((used)) void lcd_cover_background_load_jpeg(
    const char* jpeg_path) {
  if (!g_layer2_initialized) {
    return;
  }

  jpeg_save_state();

  // Use dedicated JPEG output buffer
  uint32_t jpeg_output_address = FMC_SDRAM_JPEG_OUTPUT_DATA_BUFFER_ADDRESS;

  jpeg_decode_file_operation(JPEG_FILE_FATFS);  // Explicitly use FATFS mode
  jpeg_decode_init(jpeg_output_address);        // Use dedicated buffer

  int decode_result = jpeg_decode_start(jpeg_path);
  if (decode_result != 0) {
    jpeg_restore_state();
    return;
  }

  // Get decoded image information
  uint32_t width, height, subsampling;
  jpeg_decode_info(&width, &height, &subsampling);

  // Prepare Layer2 buffer and clear to black before copying the decoded image
  uint16_t* layer2_buffer = (uint16_t*)LAYER2_MEMORY_BASE;
  uint32_t total_pixels = lcd_params.hres * lcd_params.vres;
  for (uint32_t i = 0; i < total_pixels; i++) {
    layer2_buffer[i] = 0x0000;
  }

  // When the decoded image height is smaller than the screen height,
  // add black padding above and below to keep the image vertically centered.
  uint32_t top_padding = 0;
  if (height < lcd_params.vres) {
    top_padding = (lcd_params.vres - height) / 2;
  }

  // Adjust width to standard 480 pixels
  // If width < 480: convert actual width, right side already filled with black
  // (layer2 cleared) If width > 480: crop to 480 pixels
  uint32_t adjusted_width = width;
  if (width > lcd_params.hres) {
    // Crop to screen width (480)
    adjusted_width = lcd_params.hres;
  }
  // If width < 480, use actual width - black padding on right is already there

  uint32_t dest_address =
      (uint32_t)(layer2_buffer + top_padding * lcd_params.hres);
  dma2d_copy_ycbcr_to_rgb((uint32_t*)jpeg_output_address,
                          (uint32_t*)dest_address, adjusted_width, height,
                          subsampling, lcd_params.hres,
                          width);  // Pass screen width for proper line stride,
                                   // original stride for cropping

  jpeg_restore_state();
}

// Correct hardware movement of CoverBackground - dynamic window to avoid black
// screen blocking
void lcd_cover_background_move_to_y(int16_t y_position) {
  if (!g_layer2_initialized) {
    return;
  }

  // Wait until LTDC reaches vertical blank before reconfiguring the layer to
  // avoid partially-applied settings that show up as stripes.
  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  // Update state
  cover_bg_state.y_offset = y_position;

  if (y_position <= -((int16_t)lcd_params.vres)) {
    LTDC_LAYERCONFIG config;
    config.x0 = 0;
    config.x1 = lcd_params.hres;
    config.y0 = -((int32_t)lcd_params.vres);
    config.y1 = lcd_params.vres;
    config.pixel_format = lcd_params.pixel_format_ltdc;
    config.address = LAYER2_MEMORY_BASE;

    ltdc_layer_config(&hlcd_ltdc, 1, &config);

    __HAL_LTDC_LAYER_DISABLE(&hlcd_ltdc, 1);
    ltdc_reload_on_vertical_blank();
    return;
  }

  __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 1);

  uint32_t window_x0 = 0;
  uint32_t window_y0, window_y1;
  uint32_t window_x1 = lcd_params.hres;
  uint32_t layer_address = LAYER2_MEMORY_BASE;

  if (y_position < 0) {
    window_y0 = 0;
    window_y1 = lcd_params.vres + y_position;

    uint32_t bytes_per_line = lcd_params.hres * lcd_params.bbp;
    uint32_t skip_lines = (uint32_t)(-y_position);
    layer_address = LAYER2_MEMORY_BASE + (skip_lines * bytes_per_line);
  } else {
    // Moving down: window shifts downward
    window_y0 = y_position;
    window_y1 = lcd_params.vres + y_position;
    // Address remains unchanged
    layer_address = LAYER2_MEMORY_BASE;
  }
  // Ensure window doesn't exceed screen bounds
  if (window_y1 > lcd_params.vres) {
    window_y1 = lcd_params.vres;
  }
  if (g_animation_in_progress) {
    // During animation ensure Layer1 state is stable so blending parameters
    // don't drift and create flicker artifacts.
    static uint32_t layer1_stabilize_counter = 0;
    layer1_stabilize_counter++;

    if (layer1_stabilize_counter % 4 == 0) {
      HAL_LTDC_SetAlpha(&hlcd_ltdc, 255, 1);
      __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 1);
    }
  }

  // Configure Layer2 window once per update, regardless of animation state.
  LTDC_LAYERCONFIG config;
  config.x0 = window_x0;
  config.x1 = window_x1;
  config.y0 = window_y0;
  config.y1 = window_y1;
  config.pixel_format = lcd_params.pixel_format_ltdc;
  config.address = layer_address;

  ltdc_layer_config(&hlcd_ltdc, 1, &config);

  if (g_animation_in_progress) {
    ltdc_reload_immediate();
  } else {
    ltdc_reload_on_vertical_blank();
  }
}

// Check if animation is in progress
__attribute__((used)) bool lcd_cover_background_is_animating(void) {
  return g_animation_in_progress;
}

// Global animation state structure
typedef struct {
  bool active;
  int16_t start_y;
  int16_t target_y;
  uint32_t start_time;
  uint32_t duration_ms;
  uint32_t last_update_time;
  uint32_t frame_count;
} animation_state_t;

static animation_state_t g_animation_state = {0};

// Systick callback function - update animation in system tick
static void animation_systick_callback(uint32_t tick) {
  // Limit animation update frequency to match 60fps (about 16ms)
  // Add basic protection check
  if (g_layer2_initialized && g_animation_state.active) {
    static uint32_t last_update_tick = 0;

    // Update every 16ms to match 60fps display
    if (tick - last_update_tick >= 16) {
      lcd_cover_background_update_animation();
      last_update_tick = tick;
    }
  }
}

// Initialize animation system
__attribute__((used)) void lcd_animation_init(void) {
  systick_enable_dispatch(SYSTICK_DISPATCH_ANIMATION_UPDATE,
                          animation_systick_callback);
}

// Start animation
__attribute__((used)) void lcd_cover_background_start_animation(
    int16_t target_y, uint16_t duration_ms) {
  if (!g_layer2_initialized) {
    return;
  }

  int16_t start_y = cover_bg_state.y_offset;

  if (start_y == target_y) {
    return;
  }
  g_animation_state.active = true;
  g_animation_state.start_y = start_y;
  g_animation_state.target_y = target_y;
  g_animation_state.start_time = HAL_GetTick();
  g_animation_state.duration_ms = duration_ms;
  g_animation_state.last_update_time = g_animation_state.start_time;
  g_animation_state.frame_count = 0;

  // Set global animation flag
  g_animation_in_progress = true;

  // Pre-enable Layer1 before animation to ensure smooth animation
  __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 1);

  hlcd_ltdc.Instance = LTDC;
  HAL_LTDC_SetAlpha(&hlcd_ltdc, 255, 1);  // Layer1 completely opaque
  cover_bg_state.opacity = 255;

  // Force refresh once to ensure Layer1 settings take effect
  __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hlcd_ltdc);

  // Simplified animation configuration, no Color Keying setup
}

__attribute__((used)) bool lcd_cover_background_update_animation(void) {
  if (!g_animation_state.active) {
    return false;
  }

  uint32_t current_time = HAL_GetTick();
  uint32_t elapsed_time = current_time - g_animation_state.start_time;

  // Check if animation is complete
  if (elapsed_time >= g_animation_state.duration_ms) {
    // Animation complete, move to exact position
    lcd_cover_background_move_to_y(g_animation_state.target_y);

    // accurately
    ltdc_reload_on_vertical_blank();

    // Clear animation state
    g_animation_state.active = false;
    g_animation_in_progress = false;

    // Wait for LTDC idle before final configuration
    while (lcd_ltdc_busy()) {
      HAL_Delay(1);
    }

    ltdc_reload_on_vertical_blank();

    return false;
  }

  // Calculate animation progress
  float progress = (float)elapsed_time / g_animation_state.duration_ms;

  // Use cubic ease-in-out easing function
  float eased_progress;
  if (progress < 0.5f) {
    eased_progress = 4.0f * progress * progress * progress;
  } else {
    float temp = -2.0f * progress + 2.0f;
    eased_progress = 1.0f - (temp * temp * temp) / 2.0f;
  }

  // Calculate current position
  int16_t distance = g_animation_state.target_y - g_animation_state.start_y;
  int16_t current_y =
      g_animation_state.start_y + (int16_t)(distance * eased_progress);

  lcd_cover_background_move_to_y(current_y);

  g_animation_state.frame_count++;

  return true;
}

// Check if any animation is in progress
__attribute__((used)) bool lcd_cover_background_has_active_animation(void) {
  return g_animation_state.active;
}

// Stop current animation
__attribute__((used)) void lcd_cover_background_stop_animation(void) {
  if (g_animation_state.active) {
    g_animation_state.active = false;
    g_animation_in_progress = false;
  }
}

__attribute__((used)) void lcd_cover_background_animate_to_y(
    int16_t target_y, uint16_t duration_ms) {
  if (!g_layer2_initialized) {
    return;
  }

  int16_t start_y = cover_bg_state.y_offset;

  if (start_y == target_y) {
    return;
  }

  // Set animation flag
  g_animation_in_progress = true;

  // Wait for LTDC idle to ensure previous operations complete
  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  // Ensure Layer1 is correctly configured, set only once
  hlcd_ltdc.Instance = LTDC;
  __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 1);
  HAL_LTDC_SetAlpha(&hlcd_ltdc, 255, 1);
  cover_bg_state.opacity = 255;

  // Use VBlank reload to ensure synchronization
  ltdc_reload_on_vertical_blank();

  uint32_t start_time = HAL_GetTick();
  uint32_t frame_count = 0;
  int16_t distance = target_y - start_y;

  while (true) {
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - start_time;

    // Check if animation is complete
    if (elapsed_time >= duration_ms) {
      // Animation complete, move to exact position
      lcd_cover_background_move_to_y(target_y);
      break;
    }

    // Calculate animation progress
    float progress = (float)elapsed_time / duration_ms;

    // Use cubic ease-in-out easing function
    float eased_progress;
    if (progress < 0.5f) {
      eased_progress = 4.0f * progress * progress * progress;
    } else {
      float temp = -2.0f * progress + 2.0f;
      eased_progress = 1.0f - (temp * temp * temp) / 2.0f;
    }

    // Calculate current position
    int16_t current_y = start_y + (int16_t)(distance * eased_progress);

    // Update position
    lcd_cover_background_move_to_y(current_y);

    // Wait for LTDC idle after each frame to ensure configuration takes effect
    while (lcd_ltdc_busy()) {
      HAL_Delay(1);
    }

    frame_count++;

    // 16ms delay for ~60fps
    HAL_Delay(16);
  }

  while (lcd_ltdc_busy()) {
    HAL_Delay(1);
  }

  ltdc_reload_on_vertical_blank();

  g_animation_in_progress = false;
}

// Get current opacity
__attribute__((used)) uint8_t lcd_cover_background_get_opacity(void) {
  return cover_bg_state.opacity;
}

// Check if visible
__attribute__((used)) bool lcd_cover_background_is_visible(void) {
  return cover_bg_state.visible && cover_bg_state.opacity > 0;
}

// Function to ensure second layer remains active
__attribute__((used)) void lcd_ensure_second_layer(void) {
  static bool layer_enabled = false;

  if (!g_layer2_initialized) {
    return;
  }

  // Enable second layer only once (unless it was disabled)
  if (!layer_enabled) {
    __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, 1);
    layer_enabled = true;
  }
}

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  lcd_window.start_x = x0;
  lcd_window.start_y = y0;
  lcd_window.end_x = x0 + w - 1;
  lcd_window.end_y = y0 + h - 1;
}
