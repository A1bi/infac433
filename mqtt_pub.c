#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include "mqtt.h"

#include "utils.h"

#define MQTT_DEFAULT_PORT "1883"
#define MQTT_TOPIC_FORMAT "infac433/%s/%d/%s"

struct mqtt_client client;
uint8_t sendbuf[2048];
uint8_t recvbuf[1024];
int sockfd = -1;
pthread_t refresh_thread;

static void infac_mqtt_publish_callback(void** unused, struct mqtt_response_publish *published) {}

static void *infac_mqtt_client_refresher(void* _client) {
  while (1) {
    enum MQTTErrors error = mqtt_sync(&client);
    if (error != MQTT_OK) {
      infac_log_error("failed to sync with MQTT broker: %s", mqtt_error_str(error));
    }
    usleep(100000U);
  }
  return NULL;
}

static int infac_mqtt_open_socket(const char* addr, const char* port) {
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM
  };

  int sockfd = -1;
  int result;
  struct addrinfo *servinfo;

  result = getaddrinfo(addr, port, &hints, &servinfo);
  if (result != 0) {
    infac_log_error("failed to open socket (getaddrinfo): %s", gai_strerror(result));
    return -1;
  }

  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (sockfd == -1) {
    infac_log_error("failed to open socket");
    return -1;
  }

  result = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
  if (result == -1) {
    infac_log_error("failed to connect to host");
    close(sockfd);
    sockfd = -1;
  }

  freeaddrinfo(servinfo);

  // make non-blocking
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

  return sockfd;
}

void infac_mqtt_connect(const char* addr, const char* port) {
  if (sockfd != -1) return;

  if (!port) port = MQTT_DEFAULT_PORT;

  infac_log_debug("connecting to MQTT broker host %s:%s", addr, port);

  if ((sockfd = infac_mqtt_open_socket(addr, port)) == -1) return;

  mqtt_init(&client, sockfd, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), infac_mqtt_publish_callback);
  mqtt_connect(&client, NULL, NULL, NULL, 0, NULL, NULL, MQTT_CONNECT_CLEAN_SESSION, 400);

  if (client.error != MQTT_OK) {
    infac_log_error("failed to connect to MQTT broker: %s", mqtt_error_str(client.error));
    return;
  }

  if (pthread_create(&refresh_thread, NULL, infac_mqtt_client_refresher, &client)) {
    infac_log_error("failed to start MQTT thread");
    return;
  }

  infac_log_debug("connected to MQTT broker");
}

void infac_mqtt_disconnect() {
  if (sockfd == -1) return;

  pthread_cancel(refresh_thread);
  mqtt_disconnect(&client);
  close(sockfd);
  sockfd = -1;

  infac_log_debug("disconnected from MQTT broker");
}

void infac_mqtt_publish(const char* device_id, uint8_t channel, const char* datapoint, const char* data) {
  char topic[50];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_FORMAT, device_id, channel, datapoint);

  mqtt_publish(&client, topic, data, strlen(data) + 1, MQTT_PUBLISH_QOS_0);

  if (client.error != MQTT_OK) {
    infac_log_error("failed to publish to MQTT topic: %s", mqtt_error_str(client.error));
    return;
  }

  infac_log_debug("published MQTT message to topic %s", topic);
}
