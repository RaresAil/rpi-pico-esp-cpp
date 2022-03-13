#include "hardware/i2c.h"

#include "harbys-ssd1306/ssd1306.cpp"
#include "harbys-ssd1306/textRenderer/TextRenderer.cpp"

// I2C Display
#define I2C_ID            i2c1
#define I2C_BAUD_RATE     1000000
#define I2C_SDA_PIN       2
#define I2C_SCL_PIN       3

static mutex_t m_display;

using SSD1306 = pico_ssd1306::SSD1306;

class Display {
  private:
    SSD1306 display;
    std::string network = "";

  public:
    Display() {
      mutex_init(&m_display);

      i2c_init(I2C_ID, I2C_BAUD_RATE);

      gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
      gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

      gpio_pull_up(I2C_SDA_PIN);
      gpio_pull_up(I2C_SCL_PIN);
    }

    void update_newtwork(const std::string& network) {
      this->network = network;
    }

    void setup() {
      mutex_enter_blocking(&m_display);

      try {
        this->display = SSD1306(I2C_ID, 0x3C, pico_ssd1306::Size::W128xH32);
        this->display.setOrientation(1);

        drawText(&this->display, font_8x8, "Initializing", 15, 7);
        this->display.sendBuffer();
      } catch (...) {
        printf("[Display]:[ERROR]: Failed to setup.\n");
      }

      mutex_exit(&m_display);
    }

    void update_display(
      const std::string &center_text, const uint8_t c_pos[2],
      const std::string &left_b_text, const uint8_t lb_pos[2],
      const bool &show_rb = false,
      const unsigned char rb_icon[32] = {}, const uint8_t rb_pos[2] = {}
    ) {
      mutex_enter_blocking(&m_display);

      try {
        printf("[Display]: Updating.\n");
        this->display.clear();

        drawText(&this->display, font_8x8, this->network.c_str(), 0, 0);

        drawText(&this->display, font_12x16, center_text.c_str(), c_pos[0], c_pos[1]);
        drawText(&this->display, font_8x8, left_b_text.c_str(), lb_pos[0], lb_pos[1]);

        if (show_rb) {
          this->display.addBitmapImage(rb_pos[0], rb_pos[1], 16, 16, const_cast<unsigned char*>(rb_icon));
        }

        this->display.sendBuffer();
      } catch (...) {
        printf("[Display]:[ERROR]: Failed to update.\n");
      }

      mutex_exit(&m_display);
    }
};
