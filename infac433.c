#include <signal.h>
#include <lgpio.h>
#include <unistd.h>

#include "decoder.h"

#define PIN_IN 24

uint8_t aborted = 0;

static void exit_handler(int signum) {
  aborted = 1;
}

static void setup_handlers() {
  struct sigaction sa = {
    .sa_handler = exit_handler,
  };

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char *argv[]) {
  setup_handlers();

  uint8_t gpio_handle = lgGpiochipOpen(0);
  uint8_t edge = 0;
  uint8_t val = 0;

  lgGpioClaimInput(gpio_handle, LG_SET_PULL_DOWN, PIN_IN);

  while (!aborted) {
    val = lgGpioRead(gpio_handle, PIN_IN);
    if (val != edge) {
      edge = val;
      infac_decoder_process_pulse(val);
    }
    usleep(100);
  }

  lgGpioFree(gpio_handle, PIN_IN);

  return 0;
}
