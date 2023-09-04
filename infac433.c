#include <lgpio.h>
#include <unistd.h>

#include "decoder.h"

#define PIN_IN 24

int main(int argc, char *argv[]) {
  uint8_t h = lgGpiochipOpen(0);
  uint8_t edge = 0;
  uint8_t val = 0;

  lgGpioClaimInput(h, LG_SET_PULL_DOWN, PIN_IN);

  while (1) {
    val = lgGpioRead(h, PIN_IN);
    if (val != edge) {
      edge = val;
      infac_decoder_process_pulse(val);
    }
    usleep(100);
  }
}
