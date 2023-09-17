void infac_mqtt_connect(const char* addr, const char* port);
void infac_mqtt_disconnect();
void infac_mqtt_publish_packet(const char* device_id, infac_packet *packet);
