#ifndef BoardConfig_h
#define BoardConfig_h

#include <Arduino.h>

struct BoardConfig {
  // General Section
  const char* hostname;

  // Wifi Section
  const char* wifi_name;
  const char* wifi_password;
  
  // OTA SECTION
  const char* password_ota;
  bool ota_auth;
  int ota_port;

  // MQTT section
  const char* mqtt_user;
  const char* mqtt_password;
  const char* mqtt_host;
  uint16_t mqtt_port;
  int max_retries;
  int update_interval;
  const char* PubTopic;

  // NTP Section
  const char* ntp_server_1;
  const char* ntp_server_2; 
  const char* ntp_server_3;
  long timezone;
  byte daysavetime;
};

#endif