#ifndef _TXW700140K0_H_
#define _TXW700140K0_H_

#include <mipi_display.h>

// Lane Num=2 lane
// Frame rate：60Hz
// Pixel Clk：43.25MHz

#define TXW700140K0_LANE 2         // MIPI-DSI Lane count
#define TXW700140K0_PCLK 43250000  // Pixel Clk (Hz)
#define TXW700140K0_HRES 600       // Horizontal Resolution
#define TXW700140K0_HSYNC 20       // Horizontal synchronization
#define TXW700140K0_HBP 20         // Horizontal back porch
#define TXW700140K0_HFP 40         // Horizontal front porch
#define TXW700140K0_VRES 1024      // Vertical Resolution
#define TXW700140K0_VSYNC 5        // Vertical synchronization
#define TXW700140K0_VBP 8          // Vertical back porch
#define TXW700140K0_VFP 24         // Vertical front porch

int TXW700140K0_init_sequence(DSI_Writer_t dsi_writter,
                              Delay_ms_uint32 delay_ms);

#endif
