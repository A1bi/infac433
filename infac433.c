#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <lgpio.h>
#include <unistd.h>
#include <string.h>

#include "decoder.h"
#include "utils.h"

#define DEFAULT_INPUT_PIN 24

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

void infac_print_packet(infac_packet *packet) {
  infac_log_info("channel: %d | temperature: %.1f C | humidity: %d %% | battery low: %d",
                 packet->channel, packet->temperature, packet->humidity, packet->battery_low);
}

int main(int argc, char *argv[]) {
  uint8_t input_pin = DEFAULT_INPUT_PIN;
  uint8_t filtered_channels[3];
  uint8_t filtered_channels_count = 0;

  int8_t option;
  while ((option = getopt(argc, argv, "p:c:d")) != -1) {
    switch (option) {
      case 'p':
        input_pin = atoi(optarg);
        break;

      case 'c': {
        char *channel = strtok(optarg, ",");
        while (channel != NULL) {
          if (filtered_channels_count >= sizeof(filtered_channels)) break;

          uint8_t channel_i = atoi(channel);
          if (channel_i < 1 || channel_i > 3) {
            fprintf(stderr, "Channels must be in the range of 1 to 3.\n");
            return 1;
          }
          filtered_channels[filtered_channels_count++] = channel_i;
          channel = strtok(NULL, ",");
        }
        break;
      }

      case 'd':
        infac_set_log_level(INFAC_LOG_DEBUG);
        break;

      case '?':
        return 1;
    }
  }

  setup_handlers();

  uint8_t gpio_handle = lgGpiochipOpen(0);
  uint8_t edge = 0;
  uint8_t val = 0;

  lgGpioClaimInput(gpio_handle, LG_SET_PULL_DOWN, input_pin);

  while (!aborted) {
    val = lgGpioRead(gpio_handle, input_pin);
    if (val != edge) {
      edge = val;

      infac_packet *packet = infac_decoder_process_pulse(val);
      if (packet) {
        bool drop_packet = filtered_channels_count > 0;
        for (int i = 0; i < filtered_channels_count; ++i) {
          if (filtered_channels[i] == packet->channel) {
            drop_packet = false;
            break;
          }
        }
        if (drop_packet) {
          infac_log_debug("dropping packet because of specified channel filter");
        } else {
          infac_print_packet(packet);
        }
        free(packet);
      }
    }
    usleep(100);
  }

  lgGpioFree(gpio_handle, input_pin);

  return 0;
}
