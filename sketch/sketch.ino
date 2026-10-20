#include <Arduino.h>
#include <SPI.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ---------------- PINS (NORVI XN, from reference sketch) ----------------
#define PORT_RST     38   // wakes up the I/O expanders
#define ETH_CS       1
#define DSP_CS       45
#define SD_chipSelect 42
#define SCLK         12
#define MISO         13
#define MOSI         11

// ---------------- LOVYANGFX DISPLAY SETUP ----------------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = 12;
      cfg.pin_mosi    = 11;
      cfg.pin_miso    = 13;
      cfg.pin_dc      = 46;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 45;
      cfg.pin_rst          = 47;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = true;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX tft;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PORT_RST, OUTPUT);
  digitalWrite(PORT_RST, HIGH);
  delay(100);

  SPI.begin(SCLK, MISO, MOSI);

  // Keep other SPI slaves deselected
  pinMode(ETH_CS, OUTPUT);        digitalWrite(ETH_CS, HIGH);
  pinMode(SD_chipSelect, OUTPUT); digitalWrite(SD_chipSelect, HIGH);
  pinMode(DSP_CS, OUTPUT);        digitalWrite(DSP_CS, HIGH);

  tft.init();
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(lgfx::middle_center);
  tft.setFont(&fonts::FreeSansBold18pt7b);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("ICONIC", tft.width() / 2, tft.height() / 2 - 25);

  tft.setTextColor(TFT_CYAN);
  tft.drawString("DEVICES", tft.width() / 2, tft.height() / 2 + 25);

  Serial.println("ICONIC DEVICES displayed.");
}

void loop() {
  delay(1000);
}
