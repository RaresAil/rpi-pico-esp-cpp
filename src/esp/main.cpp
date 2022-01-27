static mutex_t m_esp;

#include "at-commands.cpp"
#include "commands.cpp"

int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

bool initialize_esp() {
  try {
    clearATBuffer(4000);

    if (sendATCommandOK("RST", 2000)) {
      sendATCommand("", 2000, "ready", true);

      std::string sdkVersion = getSDKVersion();
      if (sdkVersion.empty()) {
        printf("[ESP8266]: Failed to get version\n");
        return false;
      }

      printf("[ESP8266]: SDK Version: %s\n", sdkVersion.c_str());

      uart_puts(UART_ID, "ATE0\r\n");
      if (!sendATCommandOK("", 1000)) {
        printf("[ESP8266]: Failed to set ATE0\n");
        return false;
      }

      return true;
    } else {
      printf("[ESP8266]: Failed to connect\n");
      return false;
    }
  } catch (...) {
    printf("[ESP8266]: ESP Init Exception\n");
    return false;
  }
}

bool initialize_uart() {
  try {
    mutex_init(&m_esp);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_init(UART_ID, BAUD_RATE);

    gpio_pull_up(UART_RX_PIN);

    uart_set_translate_crlf(UART_ID, false);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

    return true;
  } catch (...) {
    printf("[ESP8266]: UART Exception\n");
    return false;
  }
}
