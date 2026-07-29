// Copyright (C) 2024, Mark Qvist

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <Adafruit_GFX.h>

#define DISP_W 128
#define DISP_H 64

#if DISPLAY == OLED
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#define DISPLAY_BLACK SSD1306_BLACK
#define DISPLAY_WHITE SSD1306_WHITE

#elif DISPLAY == EINK_BW || DISPLAY == EINK_3C
void (*display_callback)();
void display_add_callback(void (*callback)()) { display_callback = callback; }
void busyCallback(const void* p) { display_callback(); }
#define DISPLAY_BLACK GxEPD_BLACK
#define DISPLAY_WHITE GxEPD_WHITE

#elif DISPLAY == ADAFRUIT_TFT
    // t-deck
    #include <Adafruit_ST7789.h>
    #define DISPLAY_WHITE ST77XX_WHITE
    #define DISPLAY_BLACK ST77XX_BLACK

#elif DISPLAY == TFT
    // t114
    #include "src/display/ST7789.h"
    #define DISPLAY_WHITE ST77XX_WHITE
    #define DISPLAY_BLACK ST77XX_BLACK
    #define COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

#elif DISPLAY == MONO_OLED
    // tbeam_s
    #include <Adafruit_SH110X.h>
    #define DISPLAY_WHITE SH110X_WHITE
    #define DISPLAY_BLACK SH110X_BLACK
#endif

#if DISPLAY == EINK_BW
// use GxEPD2 because adafruit EPD support for partial refresh is bad
#include <GxEPD2_BW.h>
#include <SPI.h>
#elif DISPLAY == EINK_3C
#include <GxEPD2_3C.h>
#include <SPI.h>
#endif

#include "Fonts/Org_01.h"

#if BOARD_MODEL == BOARD_RNODE_NG_20 || BOARD_MODEL == BOARD_LORA32_V2_0
  #if DISPLAY == OLED
  #define DISP_RST -1
  #define DISP_ADDR 0x3C
  #endif
#elif BOARD_MODEL == BOARD_TBEAM
  #if DISPLAY == OLED
  #define DISP_RST 13
  #define DISP_ADDR 0x3C
  #define DISP_CUSTOM_ADDR true
  #endif
#elif BOARD_MODEL == BOARD_HELTEC32_V2 || BOARD_MODEL == BOARD_LORA32_V1_0
  #if DISPLAY == OLED
  #define DISP_RST 16
  #define DISP_ADDR 0x3C
  #define SCL_OLED 15
  #define SDA_OLED 4
  #endif
#elif BOARD_MODEL == BOARD_HELTEC32_V3
  #define DISP_RST 21
  #define DISP_ADDR 0x3C
  #define SCL_OLED 18
  #define SDA_OLED 17
#elif BOARD_MODEL == BOARD_RNODE_NG_21
  #if DISPLAY == OLED
  #define DISP_RST -1
  #define DISP_ADDR 0x3C
  #endif
#elif BOARD_MODEL == BOARD_T3S3
  #if DISPLAY == EINK_BW
  #undef DISP_W
  #undef DISP_H
  #define DISP_W 122
  #define DISP_H 250
  #define DISP_ADDR -1
  SPIClass displaySPI(HSPI);
  #elif DISPLAY == OLED
  #define DISP_RST 21
  #define DISP_ADDR 0x3C
  #define SCL_OLED 17
  #define SDA_OLED 18
  #endif
#elif BOARD_MODEL == BOARD_RAK4631 || BOARD_MODEL == BOARD_OPENCOM_XL
  #if DISPLAY == OLED
  // RAK1921/SSD1306
  #define DISP_RST -1
  #define DISP_ADDR 0x3C
  #define SCL_OLED 14
  #define SDA_OLED 13
  #elif DISPLAY == EINK_BW
  // todo: change this to be defined in Boards.h in the future
  #define DISP_W 250
  #define DISP_H 122
  #define DISP_ADDR -1
  #elif DISPLAY == EINK_3C
  #define DISP_W 250
  #define DISP_H 122
  #define DISP_ADDR -1
  #endif
#elif BOARD_MODEL == BOARD_TECHO
  SPIClass displaySPI = SPIClass(NRF_SPIM0, pin_disp_miso, pin_disp_sck, pin_disp_mosi);
  #define DISP_W 128
  #define DISP_H 64
  #define DISP_ADDR -1
#elif BOARD_MODEL == BOARD_TBEAM_S_V1
  #define DISP_RST -1
  #define DISP_ADDR 0x3C
  #define SCL_OLED 18
  #define SDA_OLED 17
  #define DISP_CUSTOM_ADDR false
#elif BOARD_MODEL == BOARD_H_W_PAPER
  #define DISP_W 250
  #define DISP_H 122
  #define DISP_ADDR -1
#elif BOARD_MODEL == BOARD_XIAO_S3
  #define DISP_RST -1
  #define DISP_ADDR 0x3C
  #define SCL_OLED 6
  #define SDA_OLED 5
  #define DISP_CUSTOM_ADDR true
#else
  #define DISP_RST -1
  #define DISP_ADDR 0x3C
  #define DISP_CUSTOM_ADDR true
#endif

#define SMALL_FONT &Org_01

#include "Graphics.h"

#if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
  GxEPD2_BW<DISPLAY_MODEL, DISPLAY_MODEL::HEIGHT> display(DISPLAY_MODEL(pin_disp_cs, pin_disp_dc, pin_disp_reset, pin_disp_busy));
  float disp_target_fps = 0.5;
  uint32_t last_epd_refresh = 0;
  uint32_t last_epd_full_refresh = 0;
  uint16_t epd_partial_refreshes = 0;
  bool epd_force_full_refresh = true;
  uint32_t epd_last_frame_hash = 0;
  bool epd_frame_hash_valid = false;
  #define REFRESH_PERIOD 300000 // 5 minutes in ms
  #define EPD_MAX_PARTIAL_REFRESHES 30
#elif BOARD_MODEL == BOARD_RAK4631 || BOARD_MODEL == BOARD_OPENCOM_XL || BOARD_MODEL == BOARD_H_W_PAPER
  #if DISPLAY == EINK_BW
  GxEPD2_BW<DISPLAY_MODEL, DISPLAY_MODEL::HEIGHT> display(DISPLAY_MODEL(pin_disp_cs, pin_disp_dc, pin_disp_reset, pin_disp_busy));
  float disp_target_fps = 0.5;
  uint32_t last_epd_refresh = 0;
  uint32_t last_epd_full_refresh = 0;
  #define REFRESH_PERIOD 300000 // 5 minutes in ms
  #elif DISPLAY == EINK_3C
  GxEPD2_3C<DISPLAY_MODEL, DISPLAY_MODEL::HEIGHT> display(DISPLAY_MODEL(pin_disp_cs, pin_disp_dc, pin_disp_reset, pin_disp_busy));
  float disp_target_fps = 0.05; // refresh usually takes longer on 3C
  uint32_t last_epd_refresh = 0;
  uint32_t last_epd_full_refresh = 0;
  #define REFRESH_PERIOD 600000 // 10 minutes in ms
  #endif
#elif BOARD_MODEL == BOARD_TECHO
GxEPD2_BW<DISPLAY_MODEL, DISPLAY_MODEL::HEIGHT> display(DISPLAY_MODEL(pin_disp_cs, pin_disp_dc, pin_disp_reset, pin_disp_busy));
float disp_target_fps = 0.2;
uint32_t last_epd_refresh = 0;
uint32_t last_epd_full_refresh = 0;
#define REFRESH_PERIOD 300000 // 5 minutes in ms
#else
  #if DISPLAY == OLED
    Adafruit_SSD1306 display(DISP_W, DISP_H, &Wire, DISP_RST);
  #elif BOARD_MODEL == BOARD_TDECK
    // The display and SX1262 share the same SPI bus on T-Deck and T-Deck Plus.
    extern SPIClass interface_spi[1];
    Adafruit_ST7789 display(&interface_spi[0], DISPLAY_CS, DISPLAY_DC, -1);
  #elif BOARD_MODEL == BOARD_TBEAM_S_V1
    Adafruit_SH1106G display = Adafruit_SH1106G(DISP_W, DISP_H, &Wire, -1);
  #elif BOARD_MODEL == BOARD_HELTEC_T114
    ST7789Spi display(&SPI1, DISPLAY_RST, DISPLAY_DC, DISPLAY_CS);
  #endif
  float disp_target_fps = 7;
  #define SCREENSAVER_TIME 500 // ms
  uint32_t last_screensaver = 0;
  #define SCREENSAVER_INTERVAL 600000 // 10 minutes in ms
  bool screensaver_enabled = false;
#endif

#define DISP_MODE_UNKNOWN   0x00
#define DISP_MODE_LANDSCAPE 0x01
#define DISP_MODE_PORTRAIT  0x02
#define DISP_PIN_SIZE   6
#define DISPLAY_BLANKING_TIMEOUT 15*1000
uint8_t disp_mode = DISP_MODE_UNKNOWN;
uint8_t disp_ext_fb = false;
unsigned char fb[512];
uint32_t last_disp_update = 0;
uint32_t last_unblank_event = 0;
uint32_t display_blanking_timeout = DISPLAY_BLANKING_TIMEOUT;
uint8_t display_unblank_intensity = display_intensity;
bool display_blanked = false;
bool display_tx[INTERFACE_COUNT] = {false};
bool recondition_display = false;
int disp_update_interval = 1000/disp_target_fps;
int epd_update_interval = 1000/disp_target_fps;
uint32_t last_page_flip = 0;
uint32_t last_interface_page_flip = 0;
int page_interval = 4000;
bool device_signatures_ok();
bool device_firmware_ok();

bool stat_area_initialised = false;
bool radio_online = false;

#define START_PAGE 0
const uint8_t pages = 3;
uint8_t disp_page = START_PAGE;
uint8_t interface_page = START_PAGE;

uint8_t online_interface_list[INTERFACE_COUNT] = {0};

uint8_t online_interfaces = 0;

#define WATERFALL_SIZE 46

int waterfall[INTERFACE_COUNT][WATERFALL_SIZE] = {0};
int waterfall_head[INTERFACE_COUNT] = {0};

int p_ad_x = 0;
int p_ad_y = 0;
int p_as_x = 0;
int p_as_y = 0;

GFXcanvas1 stat_area(64, 64);
GFXcanvas1 disp_area(64, 64);

void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t colour);

void update_area_positions() {
  #if BOARD_MODEL == BOARD_TDECK
    // Centre the two 64x64 UI panels at 2x scale on the 320x240 display.
    if (disp_mode == DISP_MODE_PORTRAIT) {
      p_ad_x = 56;
      p_ad_y = 32;
      p_as_x = 56;
      p_as_y = 160;
    } else if (disp_mode == DISP_MODE_LANDSCAPE) {
      p_ad_x = 32;
      p_ad_y = 56;
      p_as_x = 160;
      p_as_y = 56;
    }
  #elif BOARD_MODEL == BOARD_HELTEC_T114
    if (disp_mode == DISP_MODE_PORTRAIT) {
      p_ad_x = 16;
      p_ad_y = 64;
      p_as_x = 16;
      p_as_y = p_ad_y+126;
    } else if (disp_mode == DISP_MODE_LANDSCAPE) {
      p_ad_x = 0;
      p_ad_y = 96;
      p_as_x = 126;
      p_as_y = p_ad_y;
    }
  #elif BOARD_MODEL == BOARD_TECHO
    if (disp_mode == DISP_MODE_PORTRAIT) {
      p_ad_x = 61;
      p_ad_y = 36;
      p_as_x = 64;
      p_as_y = 64+36;
    } else if (disp_mode == DISP_MODE_LANDSCAPE) {
      p_ad_x = 0;
      p_ad_y = 0;
      p_as_x = 64;
      p_as_y = 0;
    }
  #else
    if (disp_mode == DISP_MODE_PORTRAIT) {
      p_ad_x = 0 * DISPLAY_SCALE;
      p_ad_y = 0 * DISPLAY_SCALE;
      p_as_x = 0 * DISPLAY_SCALE;
      p_as_y = 64 * DISPLAY_SCALE;
    } else if (disp_mode == DISP_MODE_LANDSCAPE) {
      p_ad_x = 0 * DISPLAY_SCALE;
      p_ad_y = 0 * DISPLAY_SCALE;
      p_as_x = 64 * DISPLAY_SCALE;
      p_as_y = 0 * DISPLAY_SCALE;
    }
  #endif
}

uint8_t display_contrast = 0x00;
#if BOARD_MODEL == BOARD_TBEAM_S_V1
  void set_contrast(Adafruit_SH1106G *display, uint8_t value) {
  }
#elif BOARD_MODEL == BOARD_HELTEC_T114
  void set_contrast(ST7789Spi *display, uint8_t value) { }
#elif BOARD_MODEL == BOARD_TECHO
  void set_contrast(void *display, uint8_t value) {
    if (value == 0) { analogWrite(pin_backlight, 0); }
    else            { analogWrite(pin_backlight, value); }
  }
#elif BOARD_MODEL == BOARD_TDECK
  void set_contrast(Adafruit_ST7789 *display, uint8_t value) {
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (value > 15) value = 15;
    if (value == 0) {
        digitalWrite(DISPLAY_BL_PIN, 0);
        delay(3);
        level = 0;
        return;
    }
    if (level == 0) {
        digitalWrite(DISPLAY_BL_PIN, 1);
        level = steps;
        delayMicroseconds(30);
    }
    int from = steps - level;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(DISPLAY_BL_PIN, 0);
        digitalWrite(DISPLAY_BL_PIN, 1);
    }
    level = value;
  }
#elif (BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW) || BOARD_MODEL == BOARD_OPENCOM_XL || BOARD_MODEL == BOARD_RAK4631 || BOARD_MODEL == BOARD_H_W_PAPER
  // no backlight on these displays
  void set_contrast (void* display, uint8_t contrast) {};
#else
  void set_contrast(Adafruit_SSD1306 *display, uint8_t contrast) {
    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(contrast);
  }
#endif

#if BOARD_MODEL == BOARD_TDECK
void tdeck_display_init() {
  // T-Deck and T-Deck Plus use the same 240x320 ST7789 panel. The values
  // below mirror LilyGO's current INIT_SEQUENCE_2 configuration.
  display.init(240, 320, SPI_MODE0);
  display.setSPISpeed(40000000);

  display.sendCommand(ST77XX_SLPOUT);
  delay(120);
  display.sendCommand(ST77XX_NORON);

  const uint8_t porch[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
  const uint8_t gate[] = {0x75};
  const uint8_t vcom[] = {0x1A};
  const uint8_t lcm[] = {0x2C};
  const uint8_t vdv_vrh_enable[] = {0x01};
  const uint8_t vrh[] = {0x13};
  const uint8_t vdv[] = {0x20};
  const uint8_t frame_rate[] = {0x0F};
  const uint8_t power[] = {0xA4, 0xA1};
  const uint8_t positive_gamma[] = {
    0xD0, 0x0D, 0x14, 0x0D, 0x0D, 0x09, 0x38,
    0x44, 0x4E, 0x3A, 0x17, 0x18, 0x2F, 0x30
  };
  const uint8_t negative_gamma[] = {
    0xD0, 0x09, 0x0F, 0x08, 0x07, 0x14, 0x37,
    0x44, 0x4D, 0x38, 0x15, 0x16, 0x2C, 0x3E
  };

  display.sendCommand(0xB2, porch, sizeof(porch));
  display.sendCommand(0xB7, gate, sizeof(gate));
  display.sendCommand(0xBB, vcom, sizeof(vcom));
  display.sendCommand(0xC0, lcm, sizeof(lcm));
  display.sendCommand(0xC2, vdv_vrh_enable, sizeof(vdv_vrh_enable));
  display.sendCommand(0xC3, vrh, sizeof(vrh));
  display.sendCommand(0xC4, vdv, sizeof(vdv));
  display.sendCommand(0xC6, frame_rate, sizeof(frame_rate));
  display.sendCommand(0xD0, power, sizeof(power));
  display.sendCommand(0xE0, positive_gamma, sizeof(positive_gamma));
  display.sendCommand(0xE1, negative_gamma, sizeof(negative_gamma));
  display.sendCommand(ST77XX_INVON);
  display.sendCommand(ST77XX_DISPON);
  delay(120);
}
#endif

bool display_init() {
  #if HAS_DISPLAY
    #if BOARD_MODEL == BOARD_RNODE_NG_20 || BOARD_MODEL == BOARD_LORA32_V2_0
      int pin_display_en = 16;
      digitalWrite(pin_display_en, LOW);
      delay(50);
      digitalWrite(pin_display_en, HIGH);
    #elif BOARD_MODEL == BOARD_T3S3
      #if DISPLAY == EINK_BW
      // Keep the display on HSPI. The LoRa modem uses the global SPI bus
      // with a different pin mapping on T3S3.
      displaySPI.begin(pin_disp_sck, pin_disp_miso, pin_disp_mosi, pin_disp_cs);
      display.init(0, true, 10, false, displaySPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
      display.setFullWindow();
      display.epd2.setBusyCallback(busyCallback);
      #else
      Wire.begin(SDA_OLED, SCL_OLED);
      #endif
    #elif BOARD_MODEL == BOARD_HELTEC32_V2
      Wire.begin(SDA_OLED, SCL_OLED);
    #elif BOARD_MODEL == BOARD_HELTEC32_V3
      // enable vext / pin 36
      pinMode(Vext, OUTPUT);
      digitalWrite(Vext, LOW);
      delay(50);
      int pin_display_en = 21;
      pinMode(pin_display_en, OUTPUT);
      digitalWrite(pin_display_en, LOW);
      delay(50);
      digitalWrite(pin_display_en, HIGH);
      delay(50);
      Wire.begin(SDA_OLED, SCL_OLED);
    #elif BOARD_MODEL == BOARD_LORA32_V1_0
      int pin_display_en = 16;
      digitalWrite(pin_display_en, LOW);
      delay(50);
      digitalWrite(pin_display_en, HIGH);
      Wire.begin(SDA_OLED, SCL_OLED);
    #elif BOARD_MODEL == BOARD_RAK4631 || BOARD_MODEL == BOARD_OPENCOM_XL
      #if DISPLAY == OLED
      #elif DISPLAY == EINK_BW || DISPLAY == EINK_3C
      pinMode(pin_disp_en, INPUT_PULLUP);
      digitalWrite(pin_disp_en, HIGH);
      display.init(0, true, 10, false, SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
      display.setPartialWindow(0, 0, DISP_W, DISP_H);

      display.epd2.setBusyCallback(busyCallback);
      #endif
    #elif BOARD_MODEL == BOARD_H_W_PAPER
      pinMode(pin_disp_en, OUTPUT);
      digitalWrite(pin_disp_en, LOW);
      display.init(0, true, 10, false, SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
      display.setPartialWindow(0, 0, DISP_W, DISP_H);

      display.epd2.setBusyCallback(busyCallback);
    #elif BOARD_MODEL == BOARD_HELTEC_T114
      pinMode(PIN_T114_TFT_EN, OUTPUT);
      digitalWrite(PIN_T114_TFT_EN, LOW);
    #elif BOARD_MODEL == BOARD_TECHO
      display.init(0, true, 10, false, displaySPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
      display.setPartialWindow(0, 0, DISP_W, DISP_H);
      display.epd2.setBusyCallback(busyCallback);
      #if HAS_BACKLIGHT
        pinMode(pin_backlight, OUTPUT);
        analogWrite(pin_backlight, 0);
      #endif
    #elif BOARD_MODEL == BOARD_TBEAM_S_V1
      Wire.begin(SDA_OLED, SCL_OLED);
    #elif BOARD_MODEL == BOARD_XIAO_S3
      Wire.begin(SDA_OLED, SCL_OLED);
    #endif

    #if HAS_EEPROM
      uint8_t display_rotation = EEPROM.read(eeprom_addr(ADDR_CONF_DROT));
    #elif MCU_VARIANT == MCU_NRF52
      uint8_t display_rotation = eeprom_read(eeprom_addr(ADDR_CONF_DROT));
    #endif
    if (display_rotation < 0 or display_rotation > 3) display_rotation = 0xFF;

    #if DISP_CUSTOM_ADDR == true
      #if HAS_EEPROM
        uint8_t display_address = EEPROM.read(eeprom_addr(ADDR_CONF_DADR));
      #elif MCU_VARIANT == MCU_NRF52
        uint8_t display_address = eeprom_read(eeprom_addr(ADDR_CONF_DADR));
      #endif
      if (display_address == 0xFF) display_address = DISP_ADDR;
    #else
      uint8_t display_address = DISP_ADDR;
    #endif

    #if HAS_EEPROM
      if (EEPROM.read(eeprom_addr(ADDR_CONF_BSET)) == CONF_OK_BYTE) {
        uint8_t db_timeout = EEPROM.read(eeprom_addr(ADDR_CONF_DBLK));
        if (db_timeout == 0x00) {
          display_blanking_enabled = false;
        } else {
          display_blanking_enabled = true;
          display_blanking_timeout = db_timeout*1000;
        }
      }
    #elif MCU_VARIANT == MCU_NRF52
      if (eeprom_read(eeprom_addr(ADDR_CONF_BSET)) == CONF_OK_BYTE) {
        uint8_t db_timeout = eeprom_read(eeprom_addr(ADDR_CONF_DBLK));
        if (db_timeout == 0x00) {
          display_blanking_enabled = false;
        } else {
          display_blanking_enabled = true;
          display_blanking_timeout = db_timeout*1000;
        }
      }
    #endif
    
    #if DISPLAY == EINK_BW || DISPLAY == EINK_3C
    if(false) {
    #elif BOARD_MODEL == BOARD_TDECK
    tdeck_display_init();
    if (false) {
    #elif BOARD_MODEL == BOARD_HELTEC_T114
    display.init();
    // set white as default pixel colour for Heltec T114
    display.setRGB(COLOR565(0xFF, 0xFF, 0xFF));
    if (false) {
    #elif BOARD_MODEL == BOARD_TBEAM_S_V1
    if (!display.begin(display_address, true)) {
    #else
    if (!display.begin(SSD1306_SWITCHCAPVCC, display_address)) {
    #endif
      return false;
    } else {
      #if DISPLAY == OLED
        set_contrast(&display, display_contrast);
      #endif
      #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
        if (display_rotation == 0xFF) display_rotation = 2;
        // GxEPD2 exposes the panel as 122x250 at rotations 0/2.
        if (display_rotation == 0 || display_rotation == 2) {
          disp_mode = DISP_MODE_PORTRAIT;
        } else {
          disp_mode = DISP_MODE_LANDSCAPE;
        }
        display.setRotation(display_rotation);
      #elif BOARD_MODEL == BOARD_TDECK
        if (display_rotation == 0xFF) display_rotation = 3;
        if (display_rotation == 0 || display_rotation == 2) {
          disp_mode = DISP_MODE_PORTRAIT;
        } else {
          disp_mode = DISP_MODE_LANDSCAPE;
        }
        display.setRotation(display_rotation);
      #else
        if (display_rotation != 0xFF) {
          if (display_rotation == 0 || display_rotation == 2) {
            disp_mode = DISP_MODE_LANDSCAPE;
          } else {
            disp_mode = DISP_MODE_PORTRAIT;
          }
          display.setRotation(display_rotation);
        } else {
          #if BOARD_MODEL == BOARD_RNODE_NG_20
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(3);
          #elif BOARD_MODEL == BOARD_RNODE_NG_21
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(3);
          #elif BOARD_MODEL == BOARD_LORA32_V1_0
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(3);
          #elif BOARD_MODEL == BOARD_LORA32_V2_0
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(3);
          #elif BOARD_MODEL == BOARD_LORA32_V2_1
            disp_mode = DISP_MODE_LANDSCAPE;
            display.setRotation(0);
          #elif BOARD_MODEL == BOARD_TBEAM
            disp_mode = DISP_MODE_LANDSCAPE;
            display.setRotation(0);
          #elif BOARD_MODEL == BOARD_TBEAM_S_V1
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(1);
          #elif BOARD_MODEL == BOARD_HELTEC32_V2
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(1);
          #elif BOARD_MODEL == BOARD_RAK4631 || BOARD_MODEL == BOARD_OPENCOM_XL
            #if DISPLAY == OLED
            #elif DISPLAY == EINK_BW || DISPLAY == EINK_3C
            disp_mode = DISP_MODE_PORTRAIT;
            #endif
          #elif BOARD_MODEL == BOARD_TECHO
            disp_mode = DISP_MODE_LANDSCAPE;
            display.setRotation(3);
          #elif BOARD_MODEL == BOARD_HELTEC32_V3
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(1);
          #elif BOARD_MODEL == BOARD_RAK4631 || BOARD_MODEL == BOARD_OPENCOM_XL
            disp_mode = DISP_MODE_LANDSCAPE;
            display.setRotation(0);
          #else
            disp_mode = DISP_MODE_PORTRAIT;
            display.setRotation(3);
          #endif
        }
      #endif

      update_area_positions();
      for (int i = 0; i < INTERFACE_COUNT; i++) {
          for (int j = 0; j < WATERFALL_SIZE; j++) { waterfall[i][j] = 0; }
      }

      last_page_flip = millis();

      stat_area.cp437(true);
      disp_area.cp437(true);

      #if BOARD_MODEL != BOARD_HELTEC_T114
      display.cp437(true);
      #endif

      #if HAS_EEPROM
        display_intensity = EEPROM.read(eeprom_addr(ADDR_CONF_DINT));
      #elif MCU_VARIANT == MCU_NRF52
        display_intensity = eeprom_read(eeprom_addr(ADDR_CONF_DINT));
      #endif
      display_unblank_intensity = display_intensity;

      #if BOARD_MODEL == BOARD_TECHO
        #if HAS_BACKLIGHT
          if (display_intensity == 0) { analogWrite(pin_backlight, 0); }
          else                        { analogWrite(pin_backlight, display_intensity); }
        #endif
      #endif

      #if BOARD_MODEL == BOARD_TDECK
        display.fillScreen(DISPLAY_BLACK);
      #endif

      #if BOARD_MODEL == BOARD_HELTEC_T114
        // Enable backlight led (display is always black without this)
        fillRect(p_ad_x, p_ad_y, 128, 128, DISPLAY_BLACK);
        fillRect(p_as_x, p_as_y, 128, 128, DISPLAY_BLACK);
        pinMode(PIN_T114_TFT_BLGT, OUTPUT);
        digitalWrite(PIN_T114_TFT_BLGT, LOW);
      #endif

      return true;
    }
  #else
    return false;
  #endif
}

// Draws a line on the screen
void drawLine(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t colour) {
  #if BOARD_MODEL == BOARD_HELTEC_T114
  if(colour == DISPLAY_WHITE){
    display.setColor(WHITE);
  } else if(colour == DISPLAY_BLACK) {
    display.setColor(BLACK);
  }
  display.drawLine(x, y, width, height);
  #else
  display.drawLine(x, y, width, height, colour);
  #endif
}

// Draws a filled rectangle on the screen
void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t colour) {
  #if BOARD_MODEL == BOARD_HELTEC_T114
  if(colour == DISPLAY_WHITE){
    display.setColor(WHITE);
  } else if(colour == DISPLAY_BLACK) {
    display.setColor(BLACK);
  }
  display.fillRect(x, y, width, height);
  #else
  display.fillRect(x, y, width, height, colour);
  #endif
}

// Draws a bitmap to the display and auto scales it based on the boards configured DISPLAY_SCALE
void drawBitmap(int16_t startX, int16_t startY, const uint8_t* bitmap, int16_t bitmapWidth, int16_t bitmapHeight, uint16_t foregroundColour, uint16_t backgroundColour) {
  #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
    for (int16_t row = 0; row < bitmapHeight; row++) {
      const int16_t y0 = startY + (row * DISPLAY_SCALE_NUMERATOR) / DISPLAY_SCALE_DENOMINATOR;
      const int16_t y1 = startY + ((row + 1) * DISPLAY_SCALE_NUMERATOR) / DISPLAY_SCALE_DENOMINATOR;
      for (int16_t col = 0; col < bitmapWidth; col++) {
        const int16_t index = row * ((bitmapWidth + 7) / 8) + (col / 8);
        const uint8_t bitmask = 1 << (7 - (col % 8));
        const int16_t x0 = startX + (col * DISPLAY_SCALE_NUMERATOR) / DISPLAY_SCALE_DENOMINATOR;
        const int16_t x1 = startX + ((col + 1) * DISPLAY_SCALE_NUMERATOR) / DISPLAY_SCALE_DENOMINATOR;
        const uint16_t colour = bitmap[index] & bitmask ? foregroundColour : backgroundColour;
        fillRect(x0, y0, x1 - x0, y1 - y0, colour);
      }
    }
  #elif BOARD_MODEL == BOARD_TDECK
    // Stream each scaled row in a single SPI transaction. Calling fillRect()
    // once per source pixel is too slow for a display sharing SPI with LoRa.
    const int16_t scaled_width = bitmapWidth * DISPLAY_SCALE;
    uint16_t row_pixels[DISP_W * DISPLAY_SCALE];

    display.startWrite();
    display.setAddrWindow(
      startX,
      startY,
      scaled_width,
      bitmapHeight * DISPLAY_SCALE
    );

    for (int16_t row = 0; row < bitmapHeight; row++) {
      for (int16_t col = 0; col < bitmapWidth; col++) {
        const int16_t index = row * ((bitmapWidth + 7) / 8) + (col / 8);
        const uint8_t bitmask = 1 << (7 - (col % 8));
        const uint16_t colour =
          bitmap[index] & bitmask ? foregroundColour : backgroundColour;

        for (uint8_t x_scale = 0; x_scale < DISPLAY_SCALE; x_scale++) {
          row_pixels[col * DISPLAY_SCALE + x_scale] = colour;
        }
      }

      for (uint8_t y_scale = 0; y_scale < DISPLAY_SCALE; y_scale++) {
        display.writePixels(row_pixels, scaled_width);
      }
    }
    display.endWrite();
  #elif !DISPLAY_SCALE_OVERRIDE
    display.drawBitmap(startX, startY, bitmap, bitmapWidth, bitmapHeight, foregroundColour, backgroundColour);
  #else
    for(int16_t row = 0; row < bitmapHeight; row++){
        for(int16_t col = 0; col < bitmapWidth; col++){

            // determine index and bitmask
            int16_t index = row * ((bitmapWidth + 7) / 8) + (col / 8);
            uint8_t bitmask = 1 << (7 - (col % 8));

            // check if the current pixel is set in the bitmap
            if(bitmap[index] & bitmask){
                // draw a scaled rectangle for the foreground pixel
                fillRect(ceil(startX + col * DISPLAY_SCALE), ceil(startY + row * DISPLAY_SCALE), ceil(DISPLAY_SCALE), ceil(DISPLAY_SCALE), foregroundColour);
            } else {
                // draw a scaled rectangle for the background pixel
                fillRect(ceil(startX + col * DISPLAY_SCALE), ceil(startY + row * DISPLAY_SCALE), ceil(DISPLAY_SCALE), ceil(DISPLAY_SCALE), backgroundColour);
            }

        }
    }
  #endif
}

void draw_cable_icon(int px, int py) {
  if (cable_state == CABLE_STATE_DISCONNECTED) {
      stat_area.drawBitmap(px, py, bm_cable+0*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  } else if (cable_state == CABLE_STATE_CONNECTED) {
      stat_area.drawBitmap(px, py, bm_cable+1*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  }
}

void draw_bt_icon(int px, int py) {
  if (bt_allow_pairing) {
      stat_area.drawBitmap(px, py, bm_bt+2*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  } else if (bt_state == BT_STATE_OFF) {
      stat_area.drawBitmap(px, py, bm_bt+0*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  } else if (bt_state == BT_STATE_ON) {
      stat_area.drawBitmap(px, py, bm_bt+1*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  } else if (bt_state == BT_STATE_CONNECTED) {
      stat_area.drawBitmap(px, py, bm_bt+3*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  } else {
      stat_area.drawBitmap(px, py, bm_bt+0*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  }
}

void draw_lora_icon(RadioInterface* radio, int px, int py) {
  // todo: make display show other interfaces
  if (radio_online) {
    if (online_interface_list[interface_page] != radio->getIndex()) {
      stat_area.drawBitmap(px - 1, py - 1, bm_dot_sqr, 18, 19, DISPLAY_WHITE, DISPLAY_BLACK);

      // redraw stat area on next refresh
      stat_area_initialised = false;
    }
    if (radio->getRadioOnline()) {
      stat_area.drawBitmap(px, py, bm_rf+1*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
    } else {
      stat_area.drawBitmap(px, py, bm_rf+0*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
    }
    } else {
      stat_area.drawBitmap(px, py, bm_rf+0*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
    }
}

void draw_mw_icon(int px, int py) {
  if (INTERFACE_COUNT >= 2) {
      if (interface_obj[1]->getRadioOnline()) {
          stat_area.drawBitmap(px, py, bm_rf+3*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
      } else {
          stat_area.drawBitmap(px, py, bm_rf+2*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
      }
  } else {
      stat_area.drawBitmap(px, py, bm_rf+2*32, 16, 16, DISPLAY_WHITE, DISPLAY_BLACK);
  }
}

uint8_t charge_tick = 0;
void draw_battery_bars(int px, int py) {
  if (pmu_ready) {
    if (battery_ready) {
      if (battery_installed) {
        float battery_value = battery_percent;

        // Disable charging state display for now, since
        // boards without dedicated PMU are completely
        // unreliable for determining actual charging state.
        bool disable_charge_status = false;
        if (battery_indeterminate && battery_state == BATTERY_STATE_CHARGING) {
          disable_charge_status = true;
        }
        
        if (battery_state == BATTERY_STATE_CHARGING && !disable_charge_status) {
          battery_value = charge_tick;
          charge_tick += 3;
          if (charge_tick > 100) charge_tick = 0;
        }

        if (battery_indeterminate && battery_state == BATTERY_STATE_CHARGING && !disable_charge_status) {
          stat_area.fillRect(px-2, py-2, 18, 7, DISPLAY_BLACK);
          stat_area.drawBitmap(px-2, py-2, bm_plug, 17, 7, DISPLAY_WHITE, DISPLAY_BLACK);
        } else {
          if (battery_state == BATTERY_STATE_CHARGED) {
              stat_area.fillRect(px-2, py-2, 18, 7, DISPLAY_BLACK);
              stat_area.drawBitmap(px-2, py-2, bm_plug, 17, 7, DISPLAY_WHITE, DISPLAY_BLACK);
          } else {
                stat_area.fillRect(px, py, 14, 3, DISPLAY_BLACK);
                stat_area.fillRect(px-2, py-2, 18, 7, DISPLAY_BLACK);
                stat_area.drawRect(px-2, py-2, 17, 7, DISPLAY_WHITE);
                stat_area.drawLine(px+15, py, px+15, py+3, DISPLAY_WHITE);
                if (battery_value > 7) stat_area.drawLine(px, py, px, py+2, DISPLAY_WHITE);
                if (battery_value > 20) stat_area.drawLine(px+1*2, py, px+1*2, py+2, DISPLAY_WHITE);
                if (battery_value > 33) stat_area.drawLine(px+2*2, py, px+2*2, py+2, DISPLAY_WHITE);
                if (battery_value > 46) stat_area.drawLine(px+3*2, py, px+3*2, py+2, DISPLAY_WHITE);
                if (battery_value > 59) stat_area.drawLine(px+4*2, py, px+4*2, py+2, DISPLAY_WHITE);
                if (battery_value > 72) stat_area.drawLine(px+5*2, py, px+5*2, py+2, DISPLAY_WHITE);
                if (battery_value > 85) stat_area.drawLine(px+6*2, py, px+6*2, py+2, DISPLAY_WHITE);
          }
        }
      } else {
        stat_area.fillRect(px-2, py-2, 18, 7, DISPLAY_BLACK);
        stat_area.drawBitmap(px-2, py-2, bm_plug, 17, 7, DISPLAY_WHITE, DISPLAY_BLACK);
      }
    }
  } else {
    stat_area.fillRect(px-2, py-2, 18, 7, DISPLAY_BLACK);
    stat_area.drawBitmap(px-2, py-2, bm_plug, 17, 7, DISPLAY_WHITE, DISPLAY_BLACK);
  }
}

#define Q_SNR_STEP 2.0
#define Q_SNR_MIN_BASE -9.0
#define Q_SNR_MAX 6.0

void draw_quality_bars(int px, int py) {
    signed char t_snr = (signed int)last_snr_raw;
    int snr_int = (int)t_snr;
    float snr_min = Q_SNR_MIN_BASE-(int)interface_obj[interface_page]->getSpreadingFactor()*Q_SNR_STEP;
    float snr_span = (Q_SNR_MAX-snr_min);
    float snr = ((int)snr_int) * 0.25;
    float quality = ((snr-snr_min)/(snr_span))*100;
    if (quality > 100.0) quality = 100.0;
    if (quality < 0.0) quality = 0.0;

    // Serial.printf("Last SNR: %.2f\n, quality: %.2f\n", snr, quality);
    // Serial.printf("Last SNR: %.2f\n, quality: %.2f\n", snr, quality);
    if (quality > 0)  stat_area.drawLine(px+0*2, py+7, px+0*2, py+6, DISPLAY_WHITE);
    if (quality > 15) stat_area.drawLine(px+1*2, py+7, px+1*2, py+5, DISPLAY_WHITE);
    if (quality > 30) stat_area.drawLine(px+2*2, py+7, px+2*2, py+4, DISPLAY_WHITE);
    if (quality > 45) stat_area.drawLine(px+3*2, py+7, px+3*2, py+3, DISPLAY_WHITE);
    if (quality > 60) stat_area.drawLine(px+4*2, py+7, px+4*2, py+2, DISPLAY_WHITE);
    if (quality > 75) stat_area.drawLine(px+5*2, py+7, px+5*2, py+1, DISPLAY_WHITE);
    if (quality > 90) stat_area.drawLine(px+6*2, py+7, px+6*2, py+0, DISPLAY_WHITE);
  //}
}

#if MODEM == SX1280
  #define S_RSSI_MIN -105.0
  #define S_RSSI_MAX -65.0
#else
  #define S_RSSI_MIN -135.0
  #define S_RSSI_MAX -75.0
#endif
#define S_RSSI_SPAN (S_RSSI_MAX-S_RSSI_MIN)
void draw_signal_bars(int px, int py) {
  stat_area.fillRect(px, py, 13, 7, DISPLAY_BLACK);

  if (radio_online) {
    int rssi_val = last_rssi;
    if (rssi_val < S_RSSI_MIN) rssi_val = S_RSSI_MIN;
    if (rssi_val > S_RSSI_MAX) rssi_val = S_RSSI_MAX;
    int signal = ((rssi_val - S_RSSI_MIN)*(1.0/S_RSSI_SPAN))*100.0;

    if (signal > 100.0) signal = 100.0;
    if (signal < 0.0) signal = 0.0;

    // Serial.printf("Last SNR: %.2f\n, quality: %.2f\n", snr, quality);

#define WF_TX_SIZE 5
#define WF_TX_WIDTH 5
    // Serial.printf("Last SNR: %.2f\n, quality: %.2f\n", snr, quality);
    if (signal > 85) stat_area.drawLine(px+0*2, py+7, px+0*2, py+0, DISPLAY_WHITE);
    if (signal > 72) stat_area.drawLine(px+1*2, py+7, px+1*2, py+1, DISPLAY_WHITE);
    if (signal > 59) stat_area.drawLine(px+2*2, py+7, px+2*2, py+2, DISPLAY_WHITE);
    if (signal > 46) stat_area.drawLine(px+3*2, py+7, px+3*2, py+3, DISPLAY_WHITE);
    if (signal > 33) stat_area.drawLine(px+4*2, py+7, px+4*2, py+4, DISPLAY_WHITE);
    if (signal > 20) stat_area.drawLine(px+5*2, py+7, px+5*2, py+5, DISPLAY_WHITE);
    if (signal > 7)  stat_area.drawLine(px+6*2, py+7, px+6*2, py+6, DISPLAY_WHITE);
  }
}

#define WF_TX_SIZE 5
#define WF_RSSI_MAX -60
#define WF_RSSI_MIN -135
#define WF_RSSI_SPAN (WF_RSSI_MAX - WF_RSSI_MIN)
#define WF_PIXEL_WIDTH 10
void draw_waterfall(int px, int py) {
  int rssi_val = interface_obj[interface_page]->currentRssi();
  if (rssi_val < WF_RSSI_MIN) rssi_val = WF_RSSI_MIN;
  if (rssi_val > WF_RSSI_MAX) rssi_val = WF_RSSI_MAX;
  int rssi_normalised = ((rssi_val - WF_RSSI_MIN)*(1.0/WF_RSSI_SPAN))*WF_PIXEL_WIDTH;
  if (display_tx[interface_page]) {
    for (uint8_t i = 0; i < WF_TX_SIZE; i++) {
      waterfall[interface_page][waterfall_head[interface_page]++] = -1;
      if (waterfall_head[interface_page] >= WATERFALL_SIZE) waterfall_head[interface_page] = 0;
    }
    display_tx[interface_page] = false;
  } else {
    waterfall[interface_page][waterfall_head[interface_page]++] = rssi_normalised;
    if (waterfall_head[interface_page] >= WATERFALL_SIZE) waterfall_head[interface_page] = 0;
  }

  stat_area.fillRect(px,py,WF_PIXEL_WIDTH, WATERFALL_SIZE, DISPLAY_BLACK);
  for (int i = 0; i < WATERFALL_SIZE; i++){
    int wi = (waterfall_head[interface_page]+i)%WATERFALL_SIZE;
    int ws = waterfall[interface_page][wi];
    if (ws > 0) {
      stat_area.drawLine(px, py+i, px+ws-1, py+i, DISPLAY_WHITE);
    } else if (ws == -1) {
      uint8_t o = i%2;
      for (uint8_t ti = 0; ti < WF_PIXEL_WIDTH/2; ti++) {
        stat_area.drawPixel(px+ti*2+o, py+i, DISPLAY_WHITE);
      }
    }
  }
}

void draw_stat_area() {
  if (device_init_done) {
    if (!stat_area_initialised) {
        stat_area.drawBitmap(0, 0, bm_frame, 64, 64, DISPLAY_WHITE, DISPLAY_BLACK);
      stat_area_initialised = true;
    }

    if (millis()-last_interface_page_flip >= page_interval) {
      int online_interfaces_check = 0;

      // todo, is there a more efficient way of doing this?
      for (int i = 0; i < INTERFACE_COUNT; i++) {
          if (interface_obj[i]->getRadioOnline()) {
              online_interfaces_check++;
          }
      }

      if (online_interfaces != online_interfaces_check) {
          online_interfaces = online_interfaces_check;
      }

      // cap at two for now, as only two boxes to symbolise interfaces
      // available on display
      if (online_interfaces > 2) {
          online_interfaces = 2;
      }

      uint8_t index = 0;

      for (int i = 0; i < INTERFACE_COUNT; i++) {
          if (interface_obj[i]->getRadioOnline()) {
              online_interface_list[index] = i;
              index++;
          }
      }

      if (online_interfaces > 0) {
          interface_page = (++interface_page%online_interfaces);
      }
      last_interface_page_flip = millis();
    }

    draw_cable_icon(3, 8);
    draw_bt_icon(3, 30);
    draw_lora_icon(interface_obj[0], 45, 8);

    // todo, expand support to show more than two interfaces on screen
    if (INTERFACE_COUNT > 1) {
        draw_lora_icon(interface_obj[1], 45, 30);
    }
    draw_battery_bars(4, 58);
    radio_online = false;
    for (int i = 0; i < INTERFACE_COUNT; i++) {
        if (interface_obj[i]->getRadioOnline()) {
            radio_online = true;
            break;
        }
    }
    draw_quality_bars(28, 56);
    draw_signal_bars(44, 56);
    if (radio_online) {
      draw_waterfall(27, 4);
    }
  }
}

void update_stat_area() {
  if (eeprom_ok && !firmware_update_mode && !console_active) {
    draw_stat_area();
    if (disp_mode == DISP_MODE_PORTRAIT) {
      drawBitmap(p_as_x, p_as_y, stat_area.getBuffer(), stat_area.width(), stat_area.height(), DISPLAY_WHITE, DISPLAY_BLACK);
    } else if (disp_mode == DISP_MODE_LANDSCAPE) {
      drawBitmap(p_as_x+2, p_as_y, stat_area.getBuffer(), stat_area.width(), stat_area.height(), DISPLAY_WHITE, DISPLAY_BLACK);
      #if BOARD_MODEL != BOARD_TDECK
        if (device_init_done && !disp_ext_fb) drawLine(p_as_x, 0, p_as_x, DISP_W/2, DISPLAY_WHITE);
      #endif
    }

  } else {
    if (firmware_update_mode) {
      drawBitmap(p_as_x, p_as_y, bm_updating, stat_area.width(), stat_area.height(), DISPLAY_BLACK, DISPLAY_WHITE);
    } else if (console_active && device_init_done) {
      drawBitmap(p_as_x, p_as_y, bm_console, stat_area.width(), stat_area.height(), DISPLAY_BLACK, DISPLAY_WHITE);
      if (disp_mode == DISP_MODE_LANDSCAPE) {
        #if BOARD_MODEL != BOARD_TDECK
          drawLine(p_as_x, 0, p_as_x, DISP_W/2, DISPLAY_WHITE);
        #endif
      }
    }
  }
}

extern char bt_devname[11];
extern char bt_dh[16];

void draw_disp_area() {
  if (!device_init_done || firmware_update_mode) {
    uint8_t p_by = 37;
    if (disp_mode == DISP_MODE_LANDSCAPE || firmware_update_mode) {
      p_by = 18;
      disp_area.fillRect(0, 0, disp_area.width(), disp_area.height(), DISPLAY_BLACK);
    }
    if (!device_init_done) disp_area.drawBitmap(0, p_by, bm_boot, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
    if (firmware_update_mode) disp_area.drawBitmap(0, p_by, bm_fw_update, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
  } else {
    if (!disp_ext_fb or bt_ssp_pin != 0) {
      if (radio_online && display_diagnostics) {

        disp_area.fillRect(0,8,disp_area.width(),37, DISPLAY_BLACK); disp_area.fillRect(0,37,disp_area.width(),27, DISPLAY_WHITE); 

        disp_area.setFont(SMALL_FONT); disp_area.setTextWrap(false); disp_area.setTextColor(DISPLAY_WHITE); disp_area.setTextSize(1);

        selected_radio = interface_obj[online_interface_list[interface_page]];

        disp_area.setCursor(2, 13);
        disp_area.print("On");
        disp_area.setCursor(14, 13);
        disp_area.print("@");
        disp_area.setCursor(21, 13);
        disp_area.printf("%.1fKbps", (float)selected_radio->getBitrate()/1000.0);

        //disp_area.setCursor(31, 23-1);
        disp_area.setCursor(2, 23-1);
        disp_area.print("Airtime:");
        
        disp_area.setCursor(11, 33-1);
        if (selected_radio->getAirtime() < 0.099) {
          //disp_area.printf("%.1f%%", total_channel_util*100.0);
          disp_area.printf("%.1f%%", selected_radio->getAirtime()*100.0);
        } else {
          //disp_area.printf("%.0f%%", total_channel_util*100.0);
          disp_area.printf("%.0f%%", selected_radio->getAirtime()*100.0);
        }
        disp_area.drawBitmap(2, 26-1, bm_hg_low, 5, 9, DISPLAY_WHITE, DISPLAY_BLACK);

        disp_area.setCursor(32+11, 33-1);
        if (selected_radio->getLongtermAirtime() < 0.099) {
          //disp_area.printf("%.1f%%", longterm_channel_util*100.0);
          disp_area.printf("%.1f%%", selected_radio->getLongtermAirtime()*100.0);
        } else {
          //disp_area.printf("%.0f%%", longterm_channel_util*100.0);
          disp_area.printf("%.0f%%", selected_radio->getLongtermAirtime()*100.0);
        }
        disp_area.drawBitmap(32+2, 26-1, bm_hg_high, 5, 9, DISPLAY_WHITE, DISPLAY_BLACK);


        disp_area.setTextColor(DISPLAY_BLACK);
        disp_area.setCursor(2, 46);
        disp_area.print("Channel");
        disp_area.setCursor(38, 46);
        disp_area.print("Load:");
        
        disp_area.setCursor(11, 57);
        if (selected_radio->getTotalChannelUtil() < 0.099) {
          //disp_area.printf("%.1f%%", airtime*100.0);
          disp_area.printf("%.1f%%", selected_radio->getTotalChannelUtil()*100.0);
        } else {
          //disp_area.printf("%.0f%%", airtime*100.0);
          disp_area.printf("%.0f%%", selected_radio->getTotalChannelUtil()*100.0);
        }
        disp_area.drawBitmap(2, 50, bm_hg_low, 5, 9, DISPLAY_BLACK, DISPLAY_WHITE);

        disp_area.setCursor(32+11, 57);
        if (selected_radio->getLongtermChannelUtil() < 0.099) {
          //disp_area.printf("%.1f%%", longterm_airtime*100.0);
          disp_area.printf("%.1f%%", selected_radio->getLongtermChannelUtil()*100.0);
        } else {
          //disp_area.printf("%.0f%%", longterm_airtime*100.0);
          disp_area.printf("%.0f%%", selected_radio->getLongtermChannelUtil()*100.0);
        }
        disp_area.drawBitmap(32+2, 50, bm_hg_high, 5, 9, DISPLAY_BLACK, DISPLAY_WHITE);

        disp_area.setTextColor(DISPLAY_BLACK); disp_area.setTextSize(1);

        // TODO, for some reason there is a weird artifact at the top of the screen if this line isn't here. Need to investigate.
        disp_area.fillRect(0,0,disp_area.width(),8,DISPLAY_WHITE);

        // display device ID on top bar
        disp_area.setCursor(4, 5); disp_area.print(bt_devname);

      } else {
        if (device_signatures_ok()) {
          disp_area.drawBitmap(0, 0, bm_def_lc, disp_area.width(), 37, DISPLAY_WHITE, DISPLAY_BLACK);      
        } else {
          disp_area.drawBitmap(0, 0, bm_def, disp_area.width(), 37, DISPLAY_WHITE, DISPLAY_BLACK);      
        }

        // display device ID beneath header
        disp_area.setFont(SMALL_FONT); disp_area.setTextWrap(false); disp_area.setCursor(13, 32); disp_area.setTextColor(DISPLAY_WHITE); disp_area.setTextSize(2);
        disp_area.printf("%02X%02X", bt_dh[14], bt_dh[15]);
      }

      if (!hw_ready || !device_firmware_ok()) {
        if (!device_firmware_ok()) {
          disp_area.drawBitmap(0, 37, bm_fw_corrupt, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
        } else {
          if (!modems_installed) {
            disp_area.drawBitmap(0, 37, bm_no_radio, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
          } else {
            disp_area.drawBitmap(0, 37, bm_conf_missing, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
          }
        }
      } else if (bt_allow_pairing and bt_ssp_pin != 0) {
        char *pin_str = (char*)malloc(DISP_PIN_SIZE+1);
        sprintf(pin_str, "%06d", bt_ssp_pin);

        disp_area.drawBitmap(0, 37, bm_pairing, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
        for (int i = 0; i < DISP_PIN_SIZE; i++) {
          uint8_t numeric = pin_str[i]-48;
          uint8_t offset = numeric*5;
          disp_area.drawBitmap(7+9*i, 37+16, bm_n_uh+offset, 8, 5, DISPLAY_WHITE, DISPLAY_BLACK);
        }
        free(pin_str);
      } else {
        if (millis()-last_page_flip >= page_interval) {
          disp_page = (++disp_page%pages);
          last_page_flip = millis();
          if (not community_fw and disp_page == 0) disp_page = 1;
        }

        if (radio_online) {
          if (!display_diagnostics) {
            disp_area.drawBitmap(0, 37, bm_online, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
          }
        } else {
          if (disp_page == 0) {
            if (true || device_signatures_ok()) {
              disp_area.drawBitmap(0, 37, bm_checks, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
            } else {
              disp_area.drawBitmap(0, 37, bm_nfr, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
            }
          } else if (disp_page == 1) {
            if (!console_active) {
              disp_area.drawBitmap(0, 37, bm_hwok, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
            } else {
              disp_area.drawBitmap(0, 37, bm_console_active, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
            }
          } else if (disp_page == 2) {
            disp_area.drawBitmap(0, 37, bm_version, disp_area.width(), 27, DISPLAY_WHITE, DISPLAY_BLACK);
            char *v_str = (char*)malloc(3+1);
            sprintf(v_str, "%01d%02d", MAJ_VERS, MIN_VERS);
            for (int i = 0; i < 3; i++) {
              uint8_t numeric = v_str[i]-48; uint8_t bm_offset = numeric*5;
              uint8_t dxp = 20;
              if (i == 1) dxp += 9*1+4;
              if (i == 2) dxp += 9*2+4;
              disp_area.drawBitmap(dxp, 37+16, bm_n_uh+bm_offset, 8, 5, DISPLAY_WHITE, DISPLAY_BLACK);
            }
            free(v_str);
            disp_area.drawLine(27, 37+19, 28, 37+19, DISPLAY_BLACK);
            disp_area.drawLine(27, 37+20, 28, 37+20, DISPLAY_BLACK);
          }
        }
      }
    } else {
      disp_area.drawBitmap(0, 0, fb, disp_area.width(), disp_area.height(), DISPLAY_WHITE, DISPLAY_BLACK);
    }
  }
}

void update_disp_area() {
  draw_disp_area();

  drawBitmap(p_ad_x, p_ad_y, disp_area.getBuffer(), disp_area.width(), disp_area.height(), DISPLAY_WHITE, DISPLAY_BLACK);
  if (disp_mode == DISP_MODE_LANDSCAPE) {
    if (device_init_done && !firmware_update_mode && !disp_ext_fb) {
      #if BOARD_MODEL != BOARD_TDECK
        drawLine(0, 0, 0, 63, DISPLAY_WHITE);
      #endif
    }
  }
}

void display_recondition() {
  #if DISPLAY == OLED
    for (uint8_t iy = 0; iy < disp_area.height(); iy++) {
      unsigned char rand_seg [] = {random(0xFF),random(0xFF),random(0xFF),random(0xFF),random(0xFF),random(0xFF),random(0xFF),random(0xFF)};
      stat_area.drawBitmap(0, iy, rand_seg, 64, 1, DISPLAY_WHITE, DISPLAY_BLACK);
      disp_area.drawBitmap(0, iy, rand_seg, 64, 1, DISPLAY_WHITE, DISPLAY_BLACK);
    }

    drawBitmap(p_ad_x, p_ad_y, disp_area.getBuffer(), disp_area.width(), disp_area.height(), DISPLAY_WHITE, DISPLAY_BLACK);
    if (disp_mode == DISP_MODE_PORTRAIT) {
      drawBitmap(p_as_x, p_as_y, stat_area.getBuffer(), stat_area.width(), stat_area.height(), DISPLAY_WHITE, DISPLAY_BLACK);
    } else if (disp_mode == DISP_MODE_LANDSCAPE) {
      drawBitmap(p_as_x, p_as_y, stat_area.getBuffer(), stat_area.width(), stat_area.height(), DISPLAY_WHITE, DISPLAY_BLACK);
    }
  #endif
}

bool epd_blanked = false;
#if DISPLAY == EINK_3C || DISPLAY == EINK_BW
  #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
  uint32_t epd_frame_hash() {
    uint32_t hash = 2166136261UL;
    const uint8_t *buffers[] = {stat_area.getBuffer(), disp_area.getBuffer()};

    for (uint8_t buffer_index = 0; buffer_index < 2; buffer_index++) {
      for (uint16_t i = 0; i < 64 * 64 / 8; i++) {
        hash ^= buffers[buffer_index][i];
        hash *= 16777619UL;
      }
    }

    const uint8_t state[] = {
      (uint8_t)eeprom_ok,
      (uint8_t)firmware_update_mode,
      (uint8_t)console_active,
      (uint8_t)device_init_done,
      (uint8_t)disp_mode,
      (uint8_t)disp_ext_fb,
      (uint8_t)display.getRotation()
    };
    for (uint8_t i = 0; i < sizeof(state); i++) {
      hash ^= state[i];
      hash *= 16777619UL;
    }

    return hash;
  }
  #endif

  void epd_blank(bool full_update = true) {
    display.setFullWindow();
    display.fillScreen(DISPLAY_WHITE);
    #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
    display.display(!full_update);
    #else
    display.display(full_update);
    #endif
  }

  void epd_black(bool full_update = true) {
    display.setFullWindow();
    display.fillScreen(DISPLAY_BLACK);
    #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
    display.display(!full_update);
    #else
    display.display(full_update);
    #endif
  }
#endif

void update_display(bool blank = false) {
  #if BOARD_MODEL == BOARD_TDECK
    // T-Deck's TFT and SX1262 share one SPI controller. The ESP32-S3 SPI
    // implementation does not mask the modem IRQ during display transactions,
    // so keep the ISR from accessing the bus until the frame update is done.
    noInterrupts();
  #endif

  display_updating = true;
  if (blank == true) {
    last_disp_update = millis()-disp_update_interval-1;
  } else {
    if (display_blanking_enabled && millis()-last_unblank_event >= display_blanking_timeout) {
      blank = true;
      display_blanked = true;
      if (display_intensity != 0) {
        display_unblank_intensity = display_intensity;
      }
      display_intensity = 0;
    } else {
      display_blanked = false;
      if (display_unblank_intensity != 0x00) {
        display_intensity = display_unblank_intensity;
        display_unblank_intensity = 0x00;
      }
    }
  }

  if (blank) {
    if (millis()-last_disp_update >= disp_update_interval) {
      if (display_contrast != display_intensity) {
        display_contrast = display_intensity;
        set_contrast(&display, display_contrast);
      }

      #if DISPLAY == EINK_3C || DISPLAY == EINK_BW
        if (!epd_blanked) {
          epd_blank();
          epd_blanked = true;
          #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
          last_epd_full_refresh = millis();
          epd_partial_refreshes = 0;
          epd_force_full_refresh = false;
          epd_frame_hash_valid = false;
          #endif
        }
      #endif

      #if BOARD_MODEL == BOARD_HELTEC_T114
        display.clear();
        display.display();
      #elif BOARD_MODEL != BOARD_TDECK && DISPLAY != EINK_3C && DISPLAY != EINK_BW
        display.clearDisplay();
        display.display();
      #else
        // TODO: Clear screen
      #endif

      last_disp_update = millis();
    }

  } else {
    if (millis()-last_disp_update >= disp_update_interval) {
      uint32_t current = millis();
      if (display_contrast != display_intensity) {
        display_contrast = display_intensity;
        set_contrast(&display, display_contrast);
      }

      #if BOARD_MODEL == BOARD_HELTEC_T114
        display.clear();
      #elif BOARD_MODEL != BOARD_TDECK && DISPLAY != EINK_3C && DISPLAY != EINK_BW
        display.clearDisplay();
      #endif

      if (recondition_display) {
        disp_target_fps = 30;
        disp_update_interval = 1000/disp_target_fps;
        display_recondition();
      } else {
        #if DISPLAY == EINK_BW || DISPLAY == EINK_3C
          display.setFullWindow();
          display.fillScreen(DISPLAY_WHITE);
        #endif

        update_stat_area();
        update_disp_area();
      }
      
      #if DISPLAY == EINK_BW || DISPLAY == EINK_3C
        #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
        const uint32_t frame_hash = epd_frame_hash();
        const bool frame_changed =
          !epd_frame_hash_valid ||
          frame_hash != epd_last_frame_hash;

        if (frame_changed && current-last_epd_refresh >= epd_update_interval) {
          const bool full_refresh_due =
            epd_force_full_refresh ||
            current-last_epd_full_refresh >= REFRESH_PERIOD ||
            epd_partial_refreshes >= EPD_MAX_PARTIAL_REFRESHES;

          display.display(!full_refresh_due);
          if (full_refresh_due) {
            last_epd_full_refresh = millis();
            epd_partial_refreshes = 0;
            epd_force_full_refresh = false;
          } else {
            epd_partial_refreshes++;
          }
          last_epd_refresh = millis();
          epd_last_frame_hash = frame_hash;
          epd_frame_hash_valid = true;
          epd_blanked = false;
        }
        #else
        if (current-last_epd_refresh >= epd_update_interval) {
          if (current-last_epd_full_refresh >= REFRESH_PERIOD) { display.display(false); last_epd_full_refresh = millis(); }
          else { display.display(true); }
          last_epd_refresh = millis();
          epd_blanked = false;
        }
        #endif
      #elif BOARD_MODEL != BOARD_TDECK
        display.display();
      #endif

      last_disp_update = millis();
    }
  }
  display_updating = false;

  #if BOARD_MODEL == BOARD_TDECK
    interrupts();
  #endif
}

void display_unblank() {
  last_unblank_event = millis();
}

void display_request_refresh() {
  last_disp_update = millis()-disp_update_interval-1;
  #if DISPLAY == EINK_BW || DISPLAY == EINK_3C
    last_epd_refresh = millis()-epd_update_interval-1;
    #if BOARD_MODEL == BOARD_T3S3 && DISPLAY == EINK_BW
      epd_frame_hash_valid = false;
    #endif
  #endif
}

void ext_fb_enable() {
  disp_ext_fb = true;
}

void ext_fb_disable() {
  disp_ext_fb = false;
}
