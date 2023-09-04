#include <stdio.h>
#include <sys/time.h>

#include "decoder.h"
#include "crc4.h"

enum InfacPacketReceiveState {
  INFAC_RECEIVE_IDLE,
  INFAC_RECEIVE_PREAMBLE,
  INFAC_RECEIVE_SYNC,
  INFAC_RECEIVE_DATA
};

enum InfacPacketReceiveState current_state = INFAC_RECEIVE_IDLE;
uint64_t last_timestamp = 0;

struct timeval tv;
uint8_t state_bits = 0;
uint8_t packet[5];

static uint8_t infac_decoder_crc_check_packet() {
  uint8_t crc_received = packet[1] >> 4;
  // channel bits are moved to the CRC position since only four bytes are included
  packet[1] = (packet[1] & 0x0f) | ((packet[4] & 0x0f) << 4);
  uint8_t crc_calculated = crc4(packet, 4, 0x13, 0);
  crc_calculated ^= packet[4] >> 4;

  return crc_received == crc_calculated;
}

static void infac_decoder_process_packet() {
  if (!infac_decoder_crc_check_packet()) {
    printf("CRC check failed\n");
    return;
  }

  uint8_t humidity = (packet[3] & 0xf) * 10 + (packet[4] >> 4);
  uint16_t temperature_raw = ((uint16_t)packet[2] << 4) | (packet[3] >> 4);
  double temperature_f = temperature_raw / 10.0 - 90;
  double temperature_c = (temperature_f - 32) * 5 / 9;

  printf("temperature: %.1f C\n", temperature_c);
  printf("humidity: %d %%\n", humidity);
  printf("battery low: %d\n", (packet[1] & 0x4) > 0);
  printf("channel: %d\n", packet[4] & 0x3);
}

void infac_decoder_process_pulse(uint8_t pulse) {
  gettimeofday(&tv,NULL);
  uint64_t timestamp = tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
  uint64_t diff = timestamp - last_timestamp;

  if (diff < 150) return;

  last_timestamp = timestamp;
  uint8_t past_pulse = !pulse;

  switch (current_state) {
    case INFAC_RECEIVE_IDLE:
      if (pulse == 1) {
        current_state = INFAC_RECEIVE_PREAMBLE;
        state_bits = 0;
      }
      break;
    case INFAC_RECEIVE_PREAMBLE:
      if (diff > 800) {
        if (++state_bits >= 8) {
          current_state = INFAC_RECEIVE_SYNC;
        }
      } else {
        current_state = INFAC_RECEIVE_IDLE;
      }
      break;
    case INFAC_RECEIVE_SYNC:
      if (past_pulse == 0 && diff >= 7000) {
        current_state = INFAC_RECEIVE_DATA;
        state_bits = 0;
      } else if ((past_pulse == 1 && diff < 400) || (past_pulse == 0 && diff < 7000)) {
        current_state = INFAC_RECEIVE_IDLE;
      }
      break;
    case INFAC_RECEIVE_DATA:
      if (pulse == 1) {
        uint8_t current_byte = state_bits / 8;
        uint8_t current_bit = 7 - state_bits % 8;
        packet[current_byte] &= ~(1 << current_bit);
        if (diff > 3250) {
          packet[current_byte] |= (1 << current_bit);
        }
        if (++state_bits >= 40) {
          infac_decoder_process_packet();
          current_state = INFAC_RECEIVE_IDLE;
        }
      }
      break;
  }
}
