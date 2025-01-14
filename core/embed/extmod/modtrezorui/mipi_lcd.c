#include STM32_HAL_H

#include "mipi_lcd.h"

static int dummy_dbg_printf(const char* fmt, ...) { return 0; }

static DbgPrintf_t dbg_printf = dummy_dbg_printf;

inline void disp_set_dbg_printf(DbgPrintf_t func) { dbg_printf = func; }

// Fps = LCD_PCLK / ((LCD_WIDTH + LCD_HBP + LCD_HFP + LCD_HSW) * (LCD_HEIGHT +
// LCD_VBP + LCD_VFP + LCD_VSW)) mipi_mbps = ((LCD_WIDTH + LCD_HBP + LCD_HFP +
// LCD_HSW) * (LCD_HEIGHT + LCD_VBP + LCD_VFP + LCD_VSW) * fps * 24) /

#define LCD_TXW350135B0

#if 0 
// this is for let every configuration use elif
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

#define LED_PWM_TIM_PERIOD (50)

// HSE/DIVM3*(DIVN3+(FRACN3/8192))/DIVR3/to_Khz
#define LTDC_FREQ                                          \
  (uint32_t)(HSE_VALUE / 5 *                               \
             (lcd_params.ltdc_pll.PLL3N +                  \
              (lcd_params.ltdc_pll.PLL3FRACN / 8192.0F)) / \
             lcd_params.ltdc_pll.PLL3R / 1000)

// HSE/IDF*2*NDIV/2/ODF/8/to_Khz = 62.5 Mhz or 625000 Khz
#define DSI_FREQ (uint32_t)(HSE_VALUE / 1 * 2 * 40 / 2 / 2 / 8 / 1000)

// LCD_PCLK / ((LCD_WIDTH + LCD_HBP + LCD_HFP + LCD_HSW) * (LCD_HEIGHT + LCD_VBP
// + LCD_VFP + LCD_VSW))
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

void DSI_IRQHandler(void) {
  HAL_DSI_IRQHandler(&hlcd_dsi);
  dbg_printf("DSI_IRQHandler called!");
}

float lcd_fps = 0.0;
// static void frame_callback(DSI_HandleTypeDef* hdsi)
// {
//     static uint32_t lcd_fps_tick = 0;
//     static uint32_t lcd_fps_tock = 0;
//     lcd_fps_tick = lcd_fps_tock;
//     lcd_fps_tock = HAL_GetTick();
//     lcd_fps = 1000 / (lcd_fps_tock - lcd_fps_tick);
// }

void lcd_pwm_init(void) {
  GPIO_InitTypeDef gpio_init_structure = {0};
  /* LCD_BL_CTRL GPIO configuration */
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_TIM1_CLK_ENABLE();
  // LCD_PWM/PA7 (backlight control)
  gpio_init_structure.Mode = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
  gpio_init_structure.Alternate = GPIO_AF1_TIM1;
  gpio_init_structure.Pin = LCD_BL_CTRL_PIN;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);

  // enable PWM timer
  TIM_HandleTypeDef TIM1_Handle;
  TIM1_Handle.Instance = TIM1;
  TIM1_Handle.Init.Period = LED_PWM_TIM_PERIOD - 1;
  // TIM1/APB2 source frequency equals to fCPU in our configuration,
  // we want 1 MHz
  TIM1_Handle.Init.Prescaler =
      SystemCoreClock / 1000000 / 4 - 1;  // APB is fCPU/2(AHB)/2(APB)
  TIM1_Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  TIM1_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TIM1_Handle.Init.RepetitionCounter = 0;
  HAL_TIM_PWM_Init(&TIM1_Handle);

  TIM_OC_InitTypeDef TIM_OC_InitStructure;
  TIM_OC_InitStructure.Pulse =
      (LED_PWM_TIM_PERIOD / 2 - 1);  // default 50% dutycycle
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

static HAL_StatusTypeDef dma2d_init(DMA2D_HandleTypeDef* hdma2d) {
  if (hdma2d->Instance != DMA2D) return HAL_ERROR;

  // clock source
  {
    __HAL_RCC_DMA2D_CLK_ENABLE();
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();
  }

  return HAL_OK;
}

static HAL_StatusTypeDef ltdc_init(LTDC_HandleTypeDef* hltdc) {
  if (hltdc->Instance != LTDC) return HAL_ERROR;

  // clock source
  {
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_LTDC_FORCE_RESET();
    __HAL_RCC_LTDC_RELEASE_RESET();

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLL3.PLL3M = 5U;
    PeriphClkInitStruct.PLL3.PLL3N = lcd_params.ltdc_pll.PLL3N;
    PeriphClkInitStruct.PLL3.PLL3P = 2U;
    PeriphClkInitStruct.PLL3.PLL3Q = 2U;
    PeriphClkInitStruct.PLL3.PLL3R = lcd_params.ltdc_pll.PLL3R;
    PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLLCFGR_PLL3RGE_2;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN = lcd_params.ltdc_pll.PLL3FRACN;
    HAL_StatusTypeDef result = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    if (result != HAL_OK) {
      return result;
    }
  }

  hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AH;
  // hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AL;

  hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AH;
  // hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AL;

  // hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AH;
  hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AL;

  hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  hltdc->Init.HorizontalSync = lcd_params.hsync - 1;
  hltdc->Init.AccumulatedHBP = lcd_params.hsync + lcd_params.hbp - 1;
  hltdc->Init.AccumulatedActiveW =
      lcd_params.hsync + lcd_params.hres + lcd_params.hbp - 1;
  hltdc->Init.TotalWidth =
      lcd_params.hsync + lcd_params.hres + lcd_params.hbp + lcd_params.hfp - 1;
  hltdc->Init.VerticalSync = lcd_params.vsync - 1;
  hltdc->Init.AccumulatedVBP = lcd_params.vsync + lcd_params.vbp - 1;
  hltdc->Init.AccumulatedActiveH =
      lcd_params.vsync + lcd_params.vres + lcd_params.vbp - 1;
  hltdc->Init.TotalHeigh =
      lcd_params.vsync + lcd_params.vres + lcd_params.vbp + lcd_params.vfp - 1;

  hltdc->Init.Backcolor.Blue = 0x00;
  hltdc->Init.Backcolor.Green = 0x00;
  hltdc->Init.Backcolor.Red = 0x00;

  return HAL_LTDC_Init(hltdc);
}

static HAL_StatusTypeDef ltdc_layer_config(LTDC_HandleTypeDef* hltdc,
                                           uint32_t layer_index,
                                           LTDC_LAYERCONFIG* config) {
  LTDC_LayerCfgTypeDef pLayerCfg;

  pLayerCfg.WindowX0 = config->x0;
  pLayerCfg.WindowX1 = config->x1;
  pLayerCfg.WindowY0 = config->y0;
  pLayerCfg.WindowY1 = config->y1;
  pLayerCfg.PixelFormat = config->pixel_format;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = config->address;
  pLayerCfg.ImageWidth = (config->x1 - config->x0);
  pLayerCfg.ImageHeight = (config->y1 - config->y0);
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  return HAL_LTDC_ConfigLayer(hltdc, &pLayerCfg, layer_index);
}

static HAL_StatusTypeDef dsi_host_init(DSI_HandleTypeDef* hdsi) {
  if (hdsi->Instance != DSI) return HAL_ERROR;

  // clock source
  {
    /** Enable DSI Host and wrapper clocks */
    __HAL_RCC_DSI_CLK_ENABLE();
    __HAL_RCC_DSI_FORCE_RESET();
    __HAL_RCC_DSI_RELEASE_RESET();

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DSI;
    PeriphClkInitStruct.DsiClockSelection = RCC_DSICLKSOURCE_PHY;
    // PeriphClkInitStruct.DsiClockSelection = RCC_DSICLKSOURCE_PLL2; // PLL2Q =
    // 9 in sdram.c
    HAL_StatusTypeDef result = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    if (result != HAL_OK) {
      return result;
    }
  }

  // interrupt
  {
      // HAL_NVIC_SetPriority(DSI_IRQn, 0, 0);
      // HAL_NVIC_EnableIRQ(DSI_IRQn);
  }

  // clock div
  {
    hdsi->Init.AutomaticClockLaneControl = DSI_AUTO_CLK_LANE_CTRL_DISABLE;
    hdsi->Init.TXEscapeCkdiv = 4;  // lp txclkesc 15.625 Mhz
    hdsi->Init.NumberOfLanes = DSI_TWO_DATA_LANES;

    // dsi speed (lane byte) 62.5 Mhz, which is the maximum can support
    DSI_PLLInitTypeDef PLLInit = {0};
    PLLInit.PLLNDIV = 40;
    PLLInit.PLLIDF = DSI_PLL_IN_DIV1;
    PLLInit.PLLODF = DSI_PLL_OUT_DIV2;
    if (HAL_DSI_Init(hdsi, &PLLInit) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  // timeout
  {
    DSI_HOST_TimeoutTypeDef HostTimeouts = {0};
    HostTimeouts.TimeoutCkdiv = 1;
    HostTimeouts.HighSpeedTransmissionTimeout = 0;
    HostTimeouts.LowPowerReceptionTimeout = 0;
    HostTimeouts.HighSpeedReadTimeout = 0;
    HostTimeouts.LowPowerReadTimeout = 0;
    HostTimeouts.HighSpeedWriteTimeout = 0;
    HostTimeouts.HighSpeedWritePrespMode = DSI_HS_PM_DISABLE;
    HostTimeouts.LowPowerWriteTimeout = 0;
    HostTimeouts.BTATimeout = 0;
    if (HAL_DSI_ConfigHostTimeouts(hdsi, &HostTimeouts) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  // phy timing
  {
    DSI_PHY_TimerTypeDef PhyTimings = {0};
    PhyTimings.ClockLaneHS2LPTime = 27;
    PhyTimings.ClockLaneLP2HSTime = 32;
    PhyTimings.DataLaneHS2LPTime = 15;
    PhyTimings.DataLaneLP2HSTime = 24;
    PhyTimings.DataLaneMaxReadTime = 0;
    PhyTimings.StopWaitTime = 0;
    if (HAL_DSI_ConfigPhyTimer(hdsi, &PhyTimings) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  if (HAL_DSI_ConfigFlowControl(hdsi, DSI_FLOW_CONTROL_BTA) != HAL_OK) {
    return HAL_ERROR;
  }
  if (HAL_DSI_SetLowPowerRXFilter(hdsi, 10000) != HAL_OK) {
    return HAL_ERROR;
  }
  if (HAL_DSI_ConfigErrorMonitor(hdsi, HAL_DSI_ERROR_GEN) != HAL_OK) {
    return HAL_ERROR;
  }

  // video mode
  {
    DSI_VidCfgTypeDef VidCfg = {0};
    VidCfg.VirtualChannelID = 0;
    VidCfg.ColorCoding = lcd_params.pixel_format_dsi;
    VidCfg.LooselyPacked = DSI_LOOSELY_PACKED_DISABLE;
    VidCfg.Mode = DSI_VID_MODE_BURST;
    VidCfg.PacketSize = lcd_params.hres;
    VidCfg.NumberOfChunks = 0;  // no need in video burst mode
    VidCfg.NullPacketSize = 0;  // no need in video burst mode
    VidCfg.HSPolarity = DSI_HSYNC_ACTIVE_HIGH;
    VidCfg.VSPolarity = DSI_VSYNC_ACTIVE_HIGH;
    VidCfg.DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH;
    VidCfg.HorizontalSyncActive = (lcd_params.hsync * DSI_FREQ) / LTDC_FREQ;
    VidCfg.HorizontalBackPorch = (lcd_params.hbp * DSI_FREQ) / LTDC_FREQ;
    VidCfg.HorizontalLine = ((lcd_params.hres + lcd_params.hsync +
                              lcd_params.hbp + lcd_params.hfp) *
                             DSI_FREQ) /
                            LTDC_FREQ;
    VidCfg.VerticalSyncActive = lcd_params.vsync;
    VidCfg.VerticalBackPorch = lcd_params.vbp;
    VidCfg.VerticalFrontPorch = lcd_params.vfp;
    VidCfg.VerticalActive = lcd_params.vres;

    VidCfg.LPCommandEnable = DSI_LP_COMMAND_ENABLE;
    VidCfg.LPLargestPacketSize = 128;
    VidCfg.LPVACTLargestPacketSize = 0;
    VidCfg.LPHorizontalFrontPorchEnable = DSI_LP_HFP_ENABLE;
    VidCfg.LPHorizontalBackPorchEnable = DSI_LP_HBP_ENABLE;
    VidCfg.LPVerticalActiveEnable = DSI_LP_VACT_ENABLE;
    VidCfg.LPVerticalFrontPorchEnable = DSI_LP_VFP_ENABLE;
    VidCfg.LPVerticalBackPorchEnable = DSI_LP_VBP_ENABLE;
    VidCfg.LPVerticalSyncActiveEnable = DSI_LP_VSYNC_ENABLE;
    VidCfg.FrameBTAAcknowledgeEnable = DSI_FBTAA_DISABLE;

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

void fb_read_pixel(uint32_t x_pos, uint32_t y_pos, uint32_t* color) {
  if (lcd_params.pixel_format_ltdc == LTDC_PIXEL_FORMAT_ARGB8888) {
    /* Read data value from SDRAM memory */
    *color = *(uint32_t*)(lcd_params.fb_base +
                          (lcd_params.bbp * (lcd_params.hres * y_pos + x_pos)));
  } else {
    /*LTDC_PIXEL_FORMAT_RGB565 */
    *color = *(uint16_t*)(lcd_params.fb_base +
                          (lcd_params.bbp * (lcd_params.hres * y_pos + x_pos)));
  }
}

void fb_write_pixel(uint32_t x_pos, uint32_t y_pos, uint32_t color) {
  if (lcd_params.pixel_format_ltdc == LTDC_PIXEL_FORMAT_ARGB8888) {
    *(uint32_t*)(lcd_params.fb_base +
                 (lcd_params.bbp * (lcd_params.hres * y_pos + x_pos))) = color;
  } else {
    /*LTDC_PIXEL_FORMAT_RGB565 */
    *(uint16_t*)(lcd_params.fb_base +
                 (lcd_params.bbp * (lcd_params.hres * y_pos + x_pos))) = color;
  }
}

static void fb_fill_buffer(uint32_t* dest, uint32_t x_size, uint32_t y_size,
                           uint32_t offset, uint32_t color) {
  uint32_t output_color_mode, input_color = color;

  switch (lcd_params.pixel_format_ltdc) {
    case LTDC_PIXEL_FORMAT_RGB565:
      output_color_mode = DMA2D_OUTPUT_RGB565; /* RGB565 */
      input_color = CONVERTRGB5652ARGB8888(color);
      break;
    case LTDC_PIXEL_FORMAT_RGB888:
    default:
      output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 */
      break;
  }

  /* Register to memory mode with ARGB8888 as color Mode */
  hlcd_dma2d.Init.Mode = DMA2D_R2M;
  hlcd_dma2d.Init.ColorMode = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = offset;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if (HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK) {
    if (HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK) {
      if (HAL_DMA2D_Start(&hlcd_dma2d, input_color, (uint32_t)dest, x_size,
                          y_size) == HAL_OK) {
        /* Polling For DMA transfer */
        (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 25);
      }
    }
  }
}

void fb_fill_rect(uint32_t x_pos, uint32_t y_pos, uint32_t width,
                  uint32_t height, uint32_t color) {
  /* Get the rectangle start address */
  uint32_t address = lcd_params.fb_base +
                     ((lcd_params.bbp) * (lcd_params.hres * y_pos + x_pos));

  /* Fill the rectangle */
  fb_fill_buffer((uint32_t*)address, width, height, (lcd_params.hres - width),
                 color);
}

void fb_draw_hline(uint32_t x_pos, uint32_t y_pos, uint32_t len,
                   uint32_t color) {
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

void dma2d_copy_buffer(uint32_t* pSrc, uint32_t* pDst, uint16_t x, uint16_t y,
                       uint16_t xsize, uint16_t ysize) {
  uint32_t destination =
      (uint32_t)pDst + (y * lcd_params.hres + x) * (lcd_params.bbp);
  uint32_t source = (uint32_t)pSrc;

  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/
  hlcd_dma2d.Init.Mode = DMA2D_M2M;
  hlcd_dma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hlcd_dma2d.Init.OutputOffset = lcd_params.hres - xsize;
  hlcd_dma2d.Init.AlphaInverted =
      DMA2D_REGULAR_ALPHA; /* No Output Alpha Inversion*/
  hlcd_dma2d.Init.RedBlueSwap =
      DMA2D_RB_REGULAR; /* No Output Red & Blue swap */

  /*##-2- DMA2D Callbacks Configuration ######################################*/
  hlcd_dma2d.XferCpltCallback = NULL;

  /*##-3- Foreground Configuration ###########################################*/
  hlcd_dma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hlcd_dma2d.LayerCfg[1].InputAlpha = 0xFF;
  hlcd_dma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
  hlcd_dma2d.LayerCfg[1].InputOffset = 0;
  hlcd_dma2d.LayerCfg[1].RedBlueSwap =
      DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
  hlcd_dma2d.LayerCfg[1].AlphaInverted =
      DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if (HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK) {
    if (HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK) {
      // while ( lcd_ltdc_busy() ) {}
      if (HAL_DMA2D_Start(&hlcd_dma2d, source, destination, xsize, ysize) ==
          HAL_OK) {
        /* Polling For DMA transfer */
        HAL_DMA2D_PollForTransfer(&hlcd_dma2d, HAL_MAX_DELAY);
      }
    }
  }
}

void dma2d_copy_ycbcr_to_rgb(uint32_t* pSrc, uint32_t* pDst, uint16_t xsize,
                             uint16_t ysize, uint32_t ChromaSampling) {
  uint32_t cssMode = DMA2D_CSS_420, inputLineOffset = 0;

  if (ChromaSampling == JPEG_420_SUBSAMPLING) {
    cssMode = DMA2D_CSS_420;

    inputLineOffset = xsize % 16;
    if (inputLineOffset != 0) {
      inputLineOffset = 16 - inputLineOffset;
    }
  } else if (ChromaSampling == JPEG_444_SUBSAMPLING) {
    cssMode = DMA2D_NO_CSS;

    inputLineOffset = xsize % 8;
    if (inputLineOffset != 0) {
      inputLineOffset = 8 - inputLineOffset;
    }
  } else if (ChromaSampling == JPEG_422_SUBSAMPLING) {
    cssMode = DMA2D_CSS_422;

    inputLineOffset = xsize % 16;
    if (inputLineOffset != 0) {
      inputLineOffset = 16 - inputLineOffset;
    }
  }

  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/
  hlcd_dma2d.Init.Mode = DMA2D_M2M_PFC;
  hlcd_dma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hlcd_dma2d.Init.OutputOffset = 0;
  hlcd_dma2d.Init.AlphaInverted =
      DMA2D_REGULAR_ALPHA; /* No Output Alpha Inversion*/
  hlcd_dma2d.Init.RedBlueSwap =
      DMA2D_RB_REGULAR; /* No Output Red & Blue swap */

  /*##-2- DMA2D Callbacks Configuration ######################################*/
  hlcd_dma2d.XferCpltCallback = NULL;

  /*##-3- Foreground Configuration ###########################################*/
  hlcd_dma2d.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  hlcd_dma2d.LayerCfg[1].InputAlpha = 0xFF;
  hlcd_dma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR;
  hlcd_dma2d.LayerCfg[1].ChromaSubSampling = cssMode;
  hlcd_dma2d.LayerCfg[1].InputOffset = inputLineOffset;
  hlcd_dma2d.LayerCfg[1].RedBlueSwap =
      DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
  hlcd_dma2d.LayerCfg[1].AlphaInverted =
      DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */

  hlcd_dma2d.Instance = DMA2D;

  /*##-4- DMA2D Initialization     ###########################################*/
  HAL_DMA2D_Init(&hlcd_dma2d);
  HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1);

  HAL_DMA2D_Start(&hlcd_dma2d, (uint32_t)pSrc, (uint32_t)pDst, xsize, ysize);
  HAL_DMA2D_PollForTransfer(
      &hlcd_dma2d, 25); /* wait for the previous DMA2D transfer to ends */
}

int DSI_DCS_read(uint16_t cmd, uint8_t* data, uint16_t data_len) {
  return HAL_DSI_Read(&hlcd_dsi, 0, data, data_len, DSI_DCS_SHORT_PKT_READ, cmd,
                      (uint8_t[]){0, 0});
}

int DSI_DCS_write(uint16_t cmd, uint8_t* data, uint16_t data_len) {
  if (data_len <= 1) {
    return HAL_DSI_ShortWrite(&hlcd_dsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, cmd,
                              (uint32_t)data[0]);
  } else {
    return HAL_DSI_LongWrite(&hlcd_dsi, 0, DSI_DCS_LONG_PKT_WRITE, data_len,
                             cmd, data);
  }
}

int display_backlight(int val) {
  if (DISPLAY_BACKLIGHT != val && val >= 0 && val <= 255) {
    DISPLAY_BACKLIGHT = val;
    TIM1->CCR1 = (LED_PWM_TIM_PERIOD - 1) * val / 255;
  }
  return DISPLAY_BACKLIGHT;
}

int display_backlight_with_lcd_reset(int val) {
  if (val == 0 && DISPLAY_BACKLIGHT != 0) {
    display_backlight(0);
    lcd_refresh_suspend();
  } else if (val > 0 && DISPLAY_BACKLIGHT == 0) {
    lcd_refresh_resume();
    HAL_Delay(5);
  }
  return display_backlight(val);
}

int display_orientation(int degrees) {
  if (degrees != DISPLAY_ORIENTATION) {
    if (degrees == 0 || degrees == 90 || degrees == 180 || degrees == 270) {
      DISPLAY_ORIENTATION = degrees;
      // noop
    }
  }
  return DISPLAY_ORIENTATION;
}

void lcd_init(void) {
  // gpio
  {
    GPIO_InitTypeDef gpio_init_structure = {0};

    // RESET PIN
    __HAL_RCC_GPIOG_CLK_ENABLE();
    gpio_init_structure.Pin = LCD_RESET_PIN;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_RESET_GPIO_PORT, &gpio_init_structure);

    // TE PIN
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    gpio_init_structure.Pin = LCD_TE_PIN;
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_TE_GPIO_PORT, &gpio_init_structure);
    // HAL_GPIO_WritePin(LCD_TE_GPIO_PORT, LCD_TE_PIN, GPIO_PIN_SET); // TODO:
    // Needed?
  }

  // dma2d
  {
    hlcd_dma2d.Instance = DMA2D;
    dma2d_init(&hlcd_dma2d);
  }

  // ltdc
  {
    hlcd_ltdc.Instance = LTDC;
    if (ltdc_init(&hlcd_ltdc) != HAL_OK) dbg_printf("ltdc_init failed !\r\n");

    LTDC_LAYERCONFIG config;
    config.x0 = 0;
    config.x1 = lcd_params.hres;
    config.y0 = 0;
    config.y1 = lcd_params.vres;
    config.pixel_format = lcd_params.pixel_format_ltdc;
    config.address = DISPLAY_MEMORY_BASE;
    if (ltdc_layer_config(&hlcd_ltdc, 0, &config) != HAL_OK)
      dbg_printf("ltdc_layer_config failed !\r\n");
  }

  // dsi host
  {
    hlcd_dsi.Instance = DSI;
    if (dsi_host_init(&hlcd_dsi) != HAL_OK)
      dbg_printf("dsi_host_init failed !\r\n");
  }

  // lcd init
  {
    HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(120);

    HAL_DSI_Start(&hlcd_dsi);  // make sure ltdc inited before this call

    // Send Init Seq.
    int result = LCD_init_sequence(DSI_DCS_write, HAL_Delay);
    if (result != 0) {
      dbg_printf("LCD_init_sequence failed with 0x%02x!\r\n", result);
      while (1)
        ;
    }
  }

  dbg_printf("LTDC_FREQ=%d\r\n", LTDC_FREQ);
  dbg_printf("DSI_FREQ=%d\r\n", DSI_FREQ);
  dbg_printf("FPS_TARGET=%d\r\n", FPS_TARGET);
}

void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {}

int display_get_orientation(void) { return DISPLAY_ORIENTATION; }
void display_reset_state() {}
void display_clear_save(void) {}
const char* display_save(const char* prefix) { return NULL; }
void display_refresh(void) {}

static uint16_t grayscale_to_rgb565(uint8_t gray) {
  uint16_t r = (gray * 31 + 127) / 255;
  uint16_t g = (gray * 63 + 127) / 255;
  uint16_t b = (gray * 31 + 127) / 255;

  return 0xffff - ((r << 11) | (g << 5) | b);
}

void display_fp(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                const uint8_t* data) {
  for (uint32_t i = 0; i < w * h; i++) {
    fb_write_pixel(x + i % w, y + i / w, grayscale_to_rgb565(data[i]));
  }
}

int lcd_ltdc_busy(void) {
  hlcd_ltdc.Instance = LTDC;
  // low is busy
  return hlcd_ltdc.Instance->CDSR & 0x01 ? 0 : 1;
}

void lcd_ltdc_dsi_disable(void) {
  hlcd_ltdc.Instance = LTDC;
  hlcd_dsi.Instance = DSI;
  while (lcd_ltdc_busy()) {
  }
  HAL_DSI_Stop(&hlcd_dsi);
  __HAL_LTDC_DISABLE(&hlcd_ltdc);
}

void lcd_ltdc_dsi_enable(void) {
  hlcd_ltdc.Instance = LTDC;
  hlcd_dsi.Instance = DSI;
  __HAL_LTDC_ENABLE(&hlcd_ltdc);
  HAL_DSI_Start(&hlcd_dsi);
}

void lcd_refresh_suspend(void) {
  // wait transfer
  lcd_ltdc_dsi_disable();

  // lcd reset
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_RESET);
  // wait for full blanking done
  // needs 120ms "When Reset applied during Sleep Out Mode."
  HAL_Delay(125);
}

void lcd_refresh_resume(void) {
  // lcd reset
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_RESET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_SET);
  HAL_Delay(50);
  lcd_ltdc_dsi_enable();
  // lcd wakeup / re-init
  int result = LCD_init_sequence(DSI_DCS_write, HAL_Delay);
  if (result != 0) {
    dbg_printf("LCD_init_sequence failed with 0x%02x!\r\n", result);
    while (1)
      ;
  }
}
