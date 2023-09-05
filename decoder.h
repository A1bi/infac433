#include <stdint.h>
#include <stdbool.h>

typedef struct infac_packet {
  double temperature;
  uint8_t humidity;
  bool battery_low;
  uint8_t channel;
} infac_packet;

infac_packet *infac_decoder_process_pulse(uint8_t pulse);
