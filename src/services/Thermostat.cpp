#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include <string>

#define DEVICE_TYPE         "Thermostat"
#define TRIGGER_INERVAL_MS  60000

#define RELAY_GPIO_PIN      14
#define LM35_GPIO_PIN       26

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
    struct repeating_timer timer;

    float target_temperature = 0;
    bool unit_is_celsius = true;
    bool winter_mode = false;
    float temperature = 0;

    bool read_temperature() {
      try {
        // Select the external LM35 sensor
        adc_select_input(0);

        uint16_t result = adc_read();
        printf("[Thermostat]: Temp Raw: %f\n", result);

        const float voltage = (result / 65536.0) * 5000;
        printf("[Thermostat]: Temp Voltage: %f\n", voltage);

        this->temperature = std::ceil(voltage * 10.0) / 10.0;
        printf("[Thermostat]: Temp C: %f.\n", this->temperature);

        return true;
      } catch (...) {
        printf("[Thermostat]:[ERROR]: Failed to read temperature.\n");
      }

      return false;
    }

    static bool check(struct repeating_timer *rt) {
      try {
        Thermostat *instance = (Thermostat *)rt->user_data;
        gpio_put(RELAY_GPIO_PIN, instance->is_heating());
      } catch (...) {
        printf("[Thermostat]:[ERROR]: While checking the heating mode\n");
      }

      return true;
    }

  public:
    Thermostat() {
      adc_set_temp_sensor_enabled(false);
      adc_gpio_init(LM35_GPIO_PIN);

      gpio_init(RELAY_GPIO_PIN);
      gpio_set_dir(RELAY_GPIO_PIN, GPIO_OUT);
      gpio_put(RELAY_GPIO_PIN, 0);

      // Executed on core 0
      add_repeating_timer_ms(1000, Thermostat::check, this, &this->timer);
    }

    void change_target_temperature(float t) {
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
    float get_target_temperature() {
      return this->target_temperature;
    }

    float get_temperature() {
      return this->temperature;
    }

    bool get_unit_is_celsius() {
      return this->unit_is_celsius;
    }

    bool get_winter_mode() {
      return this->winter_mode;
    }

    bool is_heating() {
      if (
        winter_mode &&
        temperature <= target_temperature + 1
      ) {
        return true;
      }

      return false;
    }

    bool trigger_data_update() {
      int retries = 10;
      do {
        if (this->read_temperature()) {
          break;
        }

        sleep_ms(1000);
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
      float target_temperature = command.value(
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
      {"heating", service.is_heating()},
      {"cmd_id", cmd_id}
    };

    respond(response);
  } catch (...) {
    printf("[Thermostat]:[ERROR]: while handling command\n");
  }
}