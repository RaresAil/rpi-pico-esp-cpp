#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include <string>

#include "extras/Display.cpp"

#define DEVICE_TYPE         "Thermostat"
#define TRIGGER_INERVAL_MS  300000

#define RELAY_GPIO_PIN      14
#define DHT_22_GPIO_PIN     13
#define PULL_TIME           55
#define DTH_MAX_TIMINGS     1100

static mutex_t m_read_temp;
static mutex_t m_heating;

enum class COMMAND {
  SET,
  GET
};

std::string COMMANDS(const COMMAND& command) {
  switch (command) {
    case COMMAND::SET:
      return "SET";
    case COMMAND::GET:
      return "GET";
    default:
      return "";
  }
}

class Thermostat {
  private:
    const unsigned char sun_icon[32] = {
      0x01, 0x80, 0x01, 0x80, 0x20, 0x04, 0x10, 0x08, 0x03, 0xc0, 0x06, 0x60, 0x0c, 0x30, 0xc8, 0x13, 
      0xc8, 0x13, 0x0c, 0x30, 0x06, 0x60, 0x03, 0xc0, 0x10, 0x08, 0x20, 0x04, 0x01, 0x80, 0x01, 0x80
    };

    struct repeating_timer timer;
    bool is_first_init = false;
    bool prev_heating = false;

    double target_temperature = 0;
    bool unit_is_celsius = true;
    bool winter_mode = false;
    double temperature = 0;
    int humidity = 0;

    std::string c_s_temp = "";
    std::string c_heat = "";
    bool c_winter = false;
    void trigger_display_update(const bool &heating) {
      if (!is_first_init) {
        return;
      }

      const std::string s_temp_f = std::to_string(this->temperature);
      const std::string s_temp = 
        getParam(0, '.', '\n', s_temp_f) + 
        std::string(".") + 
        getParam(1, '.', '\n', s_temp_f).substr(0, 1) + 
        std::string(" C");

      const std::string heat = heating  ? "HEAT" : "";

      if (
        this->c_s_temp == s_temp &&
        this->c_heat == heat &&
        this->c_winter == this->winter_mode
      ) {
        return;
      }

      this->c_s_temp = s_temp;
      this->c_heat = heat;
      this->c_winter = this->winter_mode;

      this->display.update_display(
        s_temp,
        new uint8_t[2] { 29, 8 },
        heat,
        new uint8_t[2] { 0, 25 },
        !this->winter_mode,
        this->sun_icon,
        new uint8_t[2] { 111, 15 }
      );
    }

    Display display;

    uint32_t expect_pulse(bool state) {
      uint32_t count = 0;

      while (gpio_get(DHT_22_GPIO_PIN) == state) {
        if (count++ >= DTH_MAX_TIMINGS) {
          return UINT32_MAX;
        }
      }

      return count;
    }

    bool read_temperature() {
      mutex_enter_blocking(&m_read_temp);
      try {
        int dht_data[5] = { 0, 0, 0, 0, 0 };

        gpio_set_dir(DHT_22_GPIO_PIN, GPIO_IN);
        sleep_ms(1);

        gpio_set_dir(DHT_22_GPIO_PIN, GPIO_OUT);
        gpio_put(DHT_22_GPIO_PIN, 0);

        sleep_us(1100);
        uint32_t cycles[80];

        gpio_set_dir(DHT_22_GPIO_PIN, GPIO_IN);
        sleep_us(PULL_TIME);

        if (this->expect_pulse(0) == UINT32_MAX) {
          printf("[Thermostat]:[TIMEOUT]: Failed on low pulse.\n");
          mutex_exit(&m_read_temp);
          return false;
        }

        if (this->expect_pulse(1) == UINT32_MAX) {
          printf("[Thermostat]:[TIMEOUT]: Failed on high pulse.\n");
          mutex_exit(&m_read_temp);
          return false;
        }

        for (int i = 0; i < 80; i += 2) {
          cycles[i] = this->expect_pulse(0);
          cycles[i + 1] = this->expect_pulse(1);
        }

        for (int i = 0; i < 40; ++i) {
          uint32_t lowCycles = cycles[2 * i];
          uint32_t highCycles = cycles[2 * i + 1];

          if ((lowCycles == UINT32_MAX) || (highCycles == UINT32_MAX)) {
            printf("[Thermostat]:[TIMEOUT]: Failed on check pulse.\n");
            mutex_exit(&m_read_temp);
            return false;
          }

          dht_data[i / 8] <<= 1;
          if (highCycles > lowCycles) {
            dht_data[i / 8] |= 1;
          }
        }

        if (dht_data[4] == ((dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF)) {
          this->temperature = ((uint32_t)(dht_data[2] & 0x7F)) << 8 | dht_data[3];
          this->temperature *= 0.1;
          if (dht_data[2] & 0x80) {
            this->temperature *= -1;
          }

          this->humidity = ((uint32_t)dht_data[0]) << 8 | dht_data[1];
          this->humidity *= 0.1;

          this->temperature = std::ceil(this->temperature * 10.0) / 10.0;

          printf("[Thermostat]: Success temperature: (%f) H: (%d).\n", this->temperature, this->humidity);
          mutex_exit(&m_read_temp);
          return true;
        } else {
          printf("[Thermostat]: Failed to check checksum.\n");
        }
      } catch (...) {
        printf("[Thermostat]:[ERROR]: Failed to read temperature.\n");
      }

      mutex_exit(&m_read_temp);
      return false;
    }

    static bool check(struct repeating_timer *rt) {
      try {
        Thermostat *instance = (Thermostat *)rt->user_data;
        const bool heating = instance->is_heating();
        gpio_put(RELAY_GPIO_PIN, heating);
        instance->trigger_display_update(heating);
      } catch (...) {
        printf("[Thermostat]:[ERROR]: While checking the heating mode\n");
      }

      return true;
    }
  public:
    Thermostat() {
      mutex_init(&m_read_temp);
      mutex_init(&m_heating);

      gpio_init(RELAY_GPIO_PIN);
      gpio_set_dir(RELAY_GPIO_PIN, GPIO_OUT);
      gpio_put(RELAY_GPIO_PIN, 0);

      gpio_init(DHT_22_GPIO_PIN);
      gpio_set_dir(DHT_22_GPIO_PIN, GPIO_IN);

      // Executed on core 0
      add_repeating_timer_ms(1000, Thermostat::check, this, &this->timer);
    }

    void update_newtwork(const std::string& network) {
      this->display.update_newtwork(network);
    }

    void setup_service() {
      this->display.setup();
    }

    void ready() {
      printf("[Thermostat]: Ready.\n");
      is_first_init = true;
    }

    void change_target_temperature(double t) {
      this->target_temperature = t;
    }

    /*
     * @param winter false disables winter mode
     */
    void set_winter_mode(bool winter) {
      this->winter_mode = winter;
    }

    /*
     * @param unit true if celsius, false if fahrenheit
     */
    void set_unit_celsius(bool celsius) {
      this->unit_is_celsius = celsius;
    }

    // Getters
    double get_target_temperature() {
      return this->target_temperature;
    }

    double get_temperature() {
      return this->temperature;
    }

    int get_humidity() {
      return this->humidity;
    }

    bool get_unit_is_celsius() {
      return this->unit_is_celsius;
    }

    bool get_winter_mode() {
      return this->winter_mode;
    }

    bool is_heating() {
      mutex_enter_blocking(&m_heating);
      if (!this->winter_mode) {
        this->prev_heating = false;
      } else {
        if (!this->prev_heating) {
          if (this->temperature < this->target_temperature) {
            this->prev_heating = true;
          }
        } else {
          if (this->temperature >= (this->target_temperature + 1)) {
            this->prev_heating = false;
          }
        }
      }

      mutex_exit(&m_heating);
      return this->prev_heating;
    }

    bool trigger_data_update() {
      int retries = 10;
      do {
        if (this->read_temperature()) {
          break;
        }

        sleep_ms(2000);
      } while (retries--);

      if (retries <= 0) {
        return false;
      }

      return true;
    }
};

Thermostat service = Thermostat();

// Executed on core 1
void handle_command(
  const json& command, 
  void (*respond)(const json&)
) {
  try {
    if (command.is_null() || command.value("cmd", "") == "") {
      return;
    }

    printf("[Thermostat]: Handling command\n");
    std::string cmd = command.at("cmd").get<std::string>();

    if (cmd == COMMANDS(COMMAND::SET)) {
      double target_temperature = command.value(
        "target_temperature", 
        service.get_target_temperature()
      );
      bool unit = command.value(
        "unit",
        service.get_unit_is_celsius()
      );
      bool winter = command.value(
        "winter",
        service.get_winter_mode()
      );
    
      service.change_target_temperature(target_temperature);
      service.set_unit_celsius(unit);
      service.set_winter_mode(winter);
    } else if (cmd != COMMANDS(COMMAND::GET)) {
      return;
    }
    
    const std::string cmd_id = command.value("cmd_id", "");

    const json response = {
      {"target_temperature", service.get_target_temperature()},
      {"temperature", service.get_temperature()},
      {"unit", service.get_unit_is_celsius()},
      {"winter", service.get_winter_mode()},
      {"humidity", service.get_humidity()},
      {"heating", service.is_heating()},
      {"cmd_id", cmd_id}
    };

    respond(response);
  } catch (...) {
    printf("[Thermostat]:[ERROR]: while handling command\n");
  }
}