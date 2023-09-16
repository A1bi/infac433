void infac_mqtt_connect(const char* addr, const char* port);
void infac_mqtt_disconnect();
void infac_mqtt_publish(const char* device_id, uint8_t channel, const char* datapoint, const char* data);
