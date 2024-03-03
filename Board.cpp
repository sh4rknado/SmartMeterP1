#include "core_esp8266_features.h"
#include "Board.h"

Board::Board(Client &client) : MqttClient(client) {
  _debug = new RemoteDebug();  
  _fileManager = new FileManager(_debug);
  _firmware = new FirmwareManager(_debug, _fileManager);
  setClient(client);
}

// #################################################### < SETTER/GETTER REGION > ###########################################################

BoardConfig Board::GetConfiguration() { return _config; }

int Board::GetUpdateInterval() { return _config.update_interval; }

// #################################################### < SETUP REGION > ###########################################################

void Board::Setup() {
  // mount file system
  _fileManager->Mount(); 
  
  // read config
  ReadBoardConfiguration("/config.json", _fileManager, _config);
  
  // setup wifi configuration
  SetupWifi();
  
  // setup telnet debug
  Serial.println("setup remote debug...");
  _debug->begin(_config.hostname, RemoteDebug::ANY);

  // setup firmware OTA
  Serial.println("setup firmware...");
  _firmware->SetupFirmware(_config.hostname, _config.ota_port, _config.ota_auth, _config.password_ota);
  
  // setup MQTT
  Serial.println("Setup MQTT Client");
  
  setUsernamePassword(_config.mqtt_user, _config.mqtt_password);
  ReconnectMQTT();
}

void Board::SetupNTP() {
  configTime(3600 * _config.timezone, _config.daysavetime * 3600,  _config.ntp_server_1,  _config.ntp_server_2,  _config.ntp_server_3);
  struct tm tmstruct;
  delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  _debug->printf("\nNow is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
}

// #################################################### < Handle REGION > ###########################################################

void Board::Loop() { 
  _firmware->CheckFirmwareUpdate();  //handle OTA and update MDNS discovery
  _debug->handle();                  //handle debug
  poll();                   //keep the connection alive
}

// #################################################### < CONNECTIONS > ###########################################################

void Board::Publish(const char* subtopic, const char* value) {
  char* topic = new char[strlen(_config.PubTopic) + strlen(subtopic)];
  strcpy(topic, _config.PubTopic);
  strcat(topic, subtopic);

  Serial.printf("Sending message to topic: %s with the value : %s\n", topic, value);
  Serial.printf("size of payload: %d\n", strlen(value));
  beginMessage(topic, strlen(value), false, 0, false);
  print(value);
  delay(200); // make sure that finished to write before end;
  endMessage();
  
  delete[] topic;
}

void Board::SetupWifi() {
  WiFi.mode(WIFI_STA);  
  ConnectToWifi();
}

void Board::ConnectToWifi() {
  WiFi.begin(_config.wifi_name, _config.wifi_password);
  
  Serial.print("Connecting to WiFi");

	while (WiFi.status() != WL_CONNECTED) {
		Serial.printf(".");
    delay(500);
	}

	Serial.print(" Connected !\n");
	Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Netmask: %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
}

void Board::ReconnectMQTT() {
    int cpt = 0;
    while(!connected()) {
      connect(_config.mqtt_host, _config.mqtt_port);
      Serial.printf("connection failed with result : %d, Try again in 5s.\n", connectError());
      delay(500); // wait 1s. 
      cpt++;
  }
  
  Serial.printf("client is connected !\n", connectError());
}

// #################################################### < CONFIGURATION > ###########################################################

void Board::ReadBoardConfiguration(const char* configurationPath, FileManager* fileManager, BoardConfig& config) {
  JsonDocument doc; // Allocate a temporary JsonDocument
  
  fileManager->DeleteFile(configurationPath);

  if(!fileManager->Exists(configurationPath)) {
    ApplyDefaultConfiguration(configurationPath, fileManager, config);
  }
  else if(fileManager->ReadJson(configurationPath, doc)) {
    
    Serial.println("Configuration found, read configuration...");

    //general section
    config.hostname = doc["hostname"] | "esp8266";
    
    //wifi section
    config.wifi_name = doc["wifi_name"] | "wifi";
    config.wifi_password = doc["wifi_password"] | "password";

    // OTA Section
    config.password_ota = doc["password_ota"] | "smartmeterp1";
    config.ota_port = doc["ota_port"] | 8266;
    config.ota_auth = doc["ota_auth"] | true;
    
    // // MQTT Section
    config.mqtt_user = doc["mqtt_user"] | "user";
    config.mqtt_password = doc["mqtt_password"] | "password";
    config.mqtt_host = doc["mqtt_host"] | "ip";
    config.mqtt_port = doc["mqtt_port"] | 1883;
    config.max_retries = doc["max_retries"] | 5;
    config.update_interval = doc["update_interval"] | 1000;
    config.PubTopic = doc["PubTopic"] | "sensors/power/p1meter/";

    // NTP Section
    config.ntp_server_1 = doc["ntp_server_1"] | "1.be.pool.ntp.org";
    config.ntp_server_2 = doc["ntp_server_2"] | "2.be.pool.ntp.org";
    config.ntp_server_3 = doc["ntp_server_3"] | "3.be.pool.ntp.org";
    config.timezone = doc["timezone"] | 0;
    config.daysavetime = doc["daysavetime"] | 1;

    // debug json serialize
    // serializeJson(doc, Serial);
  }
}

void Board::ApplyDefaultConfiguration(const char* configurationPath, FileManager* fileManager, BoardConfig& config) {
    
    Serial.println("Configuration not found, default configuration will be used");

    //general section
    config.hostname = "esp8266";

    //wifi section
    config.wifi_name = "user";
    config.wifi_password = "password";
    
    // OTA Section
    config.password_ota = "smartmeterp1";
    config.ota_port = 8266;
    config.ota_auth = true;
    
    // MQTT Section
    config.mqtt_user = "user";
    config.mqtt_password = "password";
    config.mqtt_host = "ip";
    config.mqtt_port = 1883;
    config.max_retries = 5;
    config.update_interval = 1000; // 1 seconde
    config.PubTopic = "sensors/power/p1meter/";

    // NTP Section
    config.ntp_server_1 = "1.be.pool.ntp.org";
    config.ntp_server_2 = "2.be.pool.ntp.org";
    config.ntp_server_3 = "3.be.pool.ntp.org";
    config.timezone = 0;
    config.daysavetime = 1;

    SaveBoardConfiguration(configurationPath, fileManager, config);
}

void Board::SaveBoardConfiguration(const char* configurationPath, FileManager* fileManager, const BoardConfig& config) {
  JsonDocument doc;

  // General Section
  doc["hostname"] = config.hostname;
  
  // Wifi Section
  doc["wifi_name"] = config.wifi_name;
  doc["wifi_password"] = config.wifi_password;

  // OTA SECTION
  doc["password_ota"] = config.password_ota;
  doc["ota_auth"] = config.ota_auth;
  doc["ota_port"] = config.ota_port;

  // MQTT section
  doc["mqtt_user"] = config.mqtt_user;
  doc["mqtt_password"] = config.mqtt_password;
  doc["mqtt_host"] = config.mqtt_host;
  doc["mqtt_port"] = config.mqtt_port;
  doc["max_retries"] = config.max_retries;
  doc["update_interval"] = config.update_interval;
  doc["PubTopic"] = config.PubTopic;

  // NTP Section
  doc["ntp_server_1"] = config.ntp_server_1;
  doc["ntp_server_2"] = config.ntp_server_2;
  doc["ntp_server_3"] = config.ntp_server_3;
  doc["timezone"] = config.timezone;
  doc["daysavetime"] = config.daysavetime;

  Serial.println("Write default configuration...");
  fileManager->WriteJson(configurationPath, doc);
  Serial.println("save configuration completed");
}


