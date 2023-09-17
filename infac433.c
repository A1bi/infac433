#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <lgpio.h>
#include <unistd.h>
#include <string.h>

#include "decoder.h"
#include "mqtt_pub.h"
#include "utils.h"

#define DEFAULT_INPUT_PIN 24
#define DEFAULT_SLEEP_SECONDS_AFTER_RECEIVED 60
#define MAX_CHANNELS 3

uint8_t aborted = 0;
uint16_t sleep_seconds_after_received = DEFAULT_SLEEP_SECONDS_AFTER_RECEIVED;
const char *mqtt_device_id = 0;
const char *mqtt_broker_host = 0;
const char *mqtt_broker_port = 0;

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
  bool channel_filtered[MAX_CHANNELS];
  bool packet_received_on_channel[MAX_CHANNELS];

  for (int i = 0; i < MAX_CHANNELS; ++i) {
    channel_filtered[i] = true;
    packet_received_on_channel[i] = false;
  }

  int8_t option;
  while ((option = getopt(argc, argv, "p:c:i:s:d")) != -1) {
    switch (option) {
      case 'p':
        input_pin = atoi(optarg);
        break;

      case 'c': {
        for (int i = 0; i < MAX_CHANNELS; ++i) {
          channel_filtered[i] = false;
        }

        char *channel = strtok(optarg, ",");
        while (channel != NULL) {
          uint8_t channel_i = atoi(channel);
          if (channel_i < 1 || channel_i > MAX_CHANNELS) {
            infac_log_error("Channels must be in the range of 1 to %d.", MAX_CHANNELS);
            return 1;
          }
          channel_filtered[channel_i - 1] = true;
          channel = strtok(NULL, ",");
        }
        break;
      }

      case 'i':
        mqtt_device_id = optarg;
        break;

      case 's':
        sleep_seconds_after_received = atoi(optarg);
        break;

      case 'd':
        infac_set_log_level(INFAC_LOG_DEBUG);
        break;

      case '?':
        return 1;
    }
  }

  if (optind < argc) {
    mqtt_broker_host = argv[optind];
    if (++optind < argc) mqtt_broker_port = argv[optind];
  } else {
    infac_log_error("MQTT broker host must be specified.");
    return 1;
  }

  if (!mqtt_device_id) {
    infac_log_error("Device identifier must be specified using the -i option.");
    return 1;
  }

  setup_handlers();

  infac_mqtt_connect(mqtt_broker_host, mqtt_broker_port);

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
        if (channel_filtered[packet->channel - 1]) {
          infac_print_packet(packet);
          infac_mqtt_publish_packet(mqtt_device_id, packet);
          packet_received_on_channel[packet->channel - 1] = true;

          bool all_packets_received = true;
          for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (!packet_received_on_channel[i] && channel_filtered[i]) {
              all_packets_received = false;
              break;
            }
          }

          if (all_packets_received) {
            infac_log_debug("packets received for all filtered channels. pausing decoder for %d seconds...", sleep_seconds_after_received);
            sleep(sleep_seconds_after_received);
            infac_log_debug("resuming decoder");
          }

        } else {
          infac_log_debug("dropping packet because of specified channel filter");
        }
        free(packet);
      }
    }
    usleep(100);
  }

  lgGpioFree(gpio_handle, input_pin);

  infac_mqtt_disconnect();

  return 0;
}
