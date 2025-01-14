
#define USE_LINUX_MIPI_HDR

#include <TXW700140K0.h>

int TXW700140K0_init_sequence(DSI_Writer_t dsi_writter,
                              Delay_ms_uint32 delay_ms) {
  // NOTE: only in page 0, DCS is accepted, otherwise all in-house regs

  DSI_CMD(dsi_writter, 0xE0, 0x00);  // PAGE0
  DSI_CMD(dsi_writter, 0xE1, 0x93);  // SET_PASSWD
  DSI_CMD(dsi_writter, 0xE2, 0x65);  // SET_PASSWD
  DSI_CMD(dsi_writter, 0xE3, 0xF8);  // SET_PASSWD
  DSI_CMD(dsi_writter, 0x80, 0x01);  // SETDSISETUP (2 LANE)

  DSI_CMD(dsi_writter, 0xE0, 0x01);  // PAGE1

  DSI_CMD(dsi_writter, 0x00, 0x00);  // VCOM_SET
  DSI_CMD(dsi_writter, 0x01, 0x66);  // VCOM_SET

  DSI_CMD(dsi_writter, 0x03, 0x00);  // VCOM_R_SE
  DSI_CMD(dsi_writter, 0x04, 0x43);  // VCOM_R_SE

  DSI_CMD(dsi_writter, 0x0C, 0x74);  // PWRIC_SET

  DSI_CMD(dsi_writter, 0x17, 0x00);  // GAMMA_SET
  DSI_CMD(dsi_writter, 0x18, 0xB0);  // GAMMA_SET
  DSI_CMD(dsi_writter, 0x19, 0x01);  // GAMMA_SET
  DSI_CMD(dsi_writter, 0x1A, 0x00);  // GAMMA_SET
  DSI_CMD(dsi_writter, 0x1B, 0xB0);  // GAMMA_SET
  DSI_CMD(dsi_writter, 0x1C, 0x01);  // GAMMA_SET
  DSI_CMD(dsi_writter, 0x24, 0xFE);  // DCDC_CTRL

  DSI_CMD(dsi_writter, 0x35, 0x28);  // SETSTBA

  DSI_CMD(dsi_writter, 0x37, 0x29);  // SETPANEL

  DSI_CMD(dsi_writter, 0x38, 0x05);  // SETRGBCYC
  DSI_CMD(dsi_writter, 0x39, 0x00);  // SETRGBCYC
  DSI_CMD(dsi_writter, 0x3A, 0x01);  // SETRGBCYC
  DSI_CMD(dsi_writter, 0x3C, 0x90);  // SETRGBCYC
  DSI_CMD(dsi_writter, 0x3D, 0xFF);  // SETRGBCYC
  DSI_CMD(dsi_writter, 0x3E, 0xFF);  // SETRGBCYC
  DSI_CMD(dsi_writter, 0x3F, 0xFF);  // SETRGBCYC

  DSI_CMD(dsi_writter, 0x40, 0x02);  // SET_TCON
  DSI_CMD(dsi_writter, 0x41, 0x80);  // SET_TCON
  DSI_CMD(dsi_writter, 0x42, 0x9D);  // SET_TCON
  DSI_CMD(dsi_writter, 0x43, 0x18);  // SET_TCON
  DSI_CMD(dsi_writter, 0x44, 0x0B);  // SET_TCON
  DSI_CMD(dsi_writter, 0x45, 0x28);  // SET_TCON

  DSI_CMD(dsi_writter, 0x55, 0x02);  // DCDC_SEL
  DSI_CMD(dsi_writter, 0x57, 0x89);  // DCDC_SEL
  DSI_CMD(dsi_writter, 0x59, 0x0A);  // DCDC_SEL
  DSI_CMD(dsi_writter, 0x5A, 0x28);  // DCDC_SEL
  DSI_CMD(dsi_writter, 0x5B, 0x15);  // DCDC_SEL

  //----------SET_GAMMA 2.2 2.5? -------//
  DSI_CMD(dsi_writter, 0x5D, 0x7F);  // 0x7F
  DSI_CMD(dsi_writter, 0x5E, 0x6A);  // 0x69
  DSI_CMD(dsi_writter, 0x5F, 0x60);  // 0x5E
  DSI_CMD(dsi_writter, 0x60, 0x5A);  // 0x58
  DSI_CMD(dsi_writter, 0x61, 0x5D);  // 0x59
  DSI_CMD(dsi_writter, 0x62, 0x52);  // 0x4E
  DSI_CMD(dsi_writter, 0x63, 0x59);  // 0x53
  DSI_CMD(dsi_writter, 0x64, 0x45);  // 0x3E
  DSI_CMD(dsi_writter, 0x65, 0x5E);  // 0x57
  DSI_CMD(dsi_writter, 0x66, 0x5B);  // 0x53
  DSI_CMD(dsi_writter, 0x67, 0x58);  // 0x4E
  DSI_CMD(dsi_writter, 0x68, 0x6F);  // 0x63
  DSI_CMD(dsi_writter, 0x69, 0x57);  // 0x48
  DSI_CMD(dsi_writter, 0x6A, 0x56);  // 0x45
  DSI_CMD(dsi_writter, 0x6B, 0x44);  // 0x33
  DSI_CMD(dsi_writter, 0x6C, 0x3B);  // 0x2A
  DSI_CMD(dsi_writter, 0x6D, 0x2B);  // 0x1B
  DSI_CMD(dsi_writter, 0x6E, 0x15);  // 0x0A
  DSI_CMD(dsi_writter, 0x6F, 0x02);  // 0x02
  DSI_CMD(dsi_writter, 0x70, 0x67);  // 0x67
  DSI_CMD(dsi_writter, 0x71, 0x6A);  // 0x69
  DSI_CMD(dsi_writter, 0x72, 0x60);  // 0x5E
  DSI_CMD(dsi_writter, 0x73, 0x5A);  // 0x58
  DSI_CMD(dsi_writter, 0x74, 0x5D);  // 0x59
  DSI_CMD(dsi_writter, 0x75, 0x52);  // 0x4E
  DSI_CMD(dsi_writter, 0x76, 0x59);  // 0x53
  DSI_CMD(dsi_writter, 0x77, 0x45);  // 0x3E
  DSI_CMD(dsi_writter, 0x78, 0x5E);  // 0x57
  DSI_CMD(dsi_writter, 0x79, 0x5B);  // 0x53
  DSI_CMD(dsi_writter, 0x7A, 0x58);  // 0x4E
  DSI_CMD(dsi_writter, 0x7B, 0x6F);  // 0x63
  DSI_CMD(dsi_writter, 0x7C, 0x57);  // 0x48
  DSI_CMD(dsi_writter, 0x7D, 0x56);  // 0x45
  DSI_CMD(dsi_writter, 0x7E, 0x44);  // 0x33
  DSI_CMD(dsi_writter, 0x7F, 0x3B);  // 0x2A
  DSI_CMD(dsi_writter, 0x80, 0x2B);  // 0x1B
  DSI_CMD(dsi_writter, 0x81, 0x15);  // 0x0A
  DSI_CMD(dsi_writter, 0x82, 0x02);  // 0x02

  DSI_CMD(dsi_writter, 0xE0, 0x02);  // PAGE2

  /* SET_GIP_L */
  DSI_CMD(dsi_writter, 0x00, 0x5F);
  DSI_CMD(dsi_writter, 0x01, 0x5F);
  DSI_CMD(dsi_writter, 0x02, 0x51);
  DSI_CMD(dsi_writter, 0x03, 0x50);
  DSI_CMD(dsi_writter, 0x04, 0x5E);
  DSI_CMD(dsi_writter, 0x05, 0x5F);
  DSI_CMD(dsi_writter, 0x06, 0x45);
  DSI_CMD(dsi_writter, 0x07, 0x47);
  DSI_CMD(dsi_writter, 0x08, 0x49);
  DSI_CMD(dsi_writter, 0x09, 0x4B);
  DSI_CMD(dsi_writter, 0x0A, 0x41);
  DSI_CMD(dsi_writter, 0x0B, 0x5F);
  DSI_CMD(dsi_writter, 0x0C, 0x5F);
  DSI_CMD(dsi_writter, 0x0D, 0x5F);
  DSI_CMD(dsi_writter, 0x0E, 0x5F);
  DSI_CMD(dsi_writter, 0x0F, 0x5F);
  DSI_CMD(dsi_writter, 0x10, 0x5F);
  DSI_CMD(dsi_writter, 0x11, 0x5F);
  DSI_CMD(dsi_writter, 0x12, 0x5F);
  DSI_CMD(dsi_writter, 0x13, 0x5F);
  DSI_CMD(dsi_writter, 0x14, 0x5F);
  DSI_CMD(dsi_writter, 0x15, 0x5F);

  /* SET_GIP_R */
  DSI_CMD(dsi_writter, 0x16, 0x5F);
  DSI_CMD(dsi_writter, 0x17, 0x5F);
  DSI_CMD(dsi_writter, 0x18, 0x51);
  DSI_CMD(dsi_writter, 0x19, 0x50);
  DSI_CMD(dsi_writter, 0x1A, 0x5E);
  DSI_CMD(dsi_writter, 0x1B, 0x5F);
  DSI_CMD(dsi_writter, 0x1C, 0x44);
  DSI_CMD(dsi_writter, 0x1D, 0x46);
  DSI_CMD(dsi_writter, 0x1E, 0x48);
  DSI_CMD(dsi_writter, 0x1F, 0x4A);
  DSI_CMD(dsi_writter, 0x20, 0x40);
  DSI_CMD(dsi_writter, 0x21, 0x5F);
  DSI_CMD(dsi_writter, 0x22, 0x5F);
  DSI_CMD(dsi_writter, 0x23, 0x5F);
  DSI_CMD(dsi_writter, 0x24, 0x5F);
  DSI_CMD(dsi_writter, 0x25, 0x5F);
  DSI_CMD(dsi_writter, 0x26, 0x5F);
  DSI_CMD(dsi_writter, 0x27, 0x5F);
  DSI_CMD(dsi_writter, 0x28, 0x5F);
  DSI_CMD(dsi_writter, 0x29, 0x5F);
  DSI_CMD(dsi_writter, 0x2A, 0x5F);
  DSI_CMD(dsi_writter, 0x2B, 0x5F);

  /* SETGIP1 */
  DSI_CMD(dsi_writter, 0x58, 0x40);
  DSI_CMD(dsi_writter, 0x5B, 0x10);
  DSI_CMD(dsi_writter, 0x5C, 0x07);
  DSI_CMD(dsi_writter, 0x5D, 0x40);
  DSI_CMD(dsi_writter, 0x5E, 0x01);
  DSI_CMD(dsi_writter, 0x5F, 0x02);
  DSI_CMD(dsi_writter, 0x60, 0x33);
  DSI_CMD(dsi_writter, 0x61, 0x01);
  DSI_CMD(dsi_writter, 0x62, 0x02);
  DSI_CMD(dsi_writter, 0x63, 0x68);
  DSI_CMD(dsi_writter, 0x64, 0x68);
  DSI_CMD(dsi_writter, 0x65, 0x54);
  DSI_CMD(dsi_writter, 0x66, 0x16);
  DSI_CMD(dsi_writter, 0x67, 0x73);
  DSI_CMD(dsi_writter, 0x68, 0x09);
  DSI_CMD(dsi_writter, 0x69, 0x68);
  DSI_CMD(dsi_writter, 0x6A, 0x68);
  DSI_CMD(dsi_writter, 0x6B, 0x08);

  /* SETGIP2 */
  DSI_CMD(dsi_writter, 0x6C, 0x00);
  DSI_CMD(dsi_writter, 0x6D, 0x00);
  DSI_CMD(dsi_writter, 0x6E, 0x00);
  DSI_CMD(dsi_writter, 0x6F, 0x88);

  DSI_CMD(dsi_writter, 0xE0, 0x04);  // PAGE4
  DSI_CMD(dsi_writter, 0x00, 0x0E);  // POWER_OPT
  DSI_CMD(dsi_writter, 0x02, 0xB3);  // POWER_OPT
  DSI_CMD(dsi_writter, 0x09, 0x60);  // SETRGBCYC2
  DSI_CMD(dsi_writter, 0x0E, 0x48);  // SETSTBA2

  DSI_CMD(dsi_writter, 0xE0, 0x00);  // PAGE0
  delay_ms(140);

  DSI_CMD(dsi_writter, MIPI_DCS_EXIT_SLEEP_MODE);
  delay_ms(140);

  DSI_CMD(dsi_writter, MIPI_DCS_SET_DISPLAY_ON);
  delay_ms(20);

  DSI_CMD(dsi_writter, MIPI_DCS_SET_TEAR_OFF);
  DSI_CMD(dsi_writter, MIPI_DCS_SET_TEAR_ON, 0x00);

  return 0;
}
