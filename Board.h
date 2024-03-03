#ifndef Board_h
#define Board_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <RemoteDebug.h>
#include "FirmwareManager.h"
#include "FileManager.h"
#include "BoardConfig.h"
#include <ArduinoMqttClient.h>

class Board : public MqttClient {
  private:
    RemoteDebug* _debug;
    FirmwareManager* _firmware;
    FileManager* _fileManager;
    BoardConfig _config;

  public:
    Board(Client& client);
    BoardConfig GetConfiguration();
    int GetUpdateInterval();
    void Setup();
    void SetupNTP();
    void SetupWifi();
    void Loop();
    void ConnectToWifi();
    void SaveBoardConfiguration(const char* configurationPath, FileManager* fileManager, const BoardConfig& config);
    void ApplyDefaultConfiguration(const char* configurationPath, FileManager* fileManager, BoardConfig& config);
    void ReadBoardConfiguration(const char* configurationPath, FileManager* fileManager, BoardConfig& config);
    void Publish(const char* topic, const char* value);
    void ReconnectMQTT();

};

#endif


