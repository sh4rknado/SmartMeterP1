#include "dsmr.h"
#include "Telegram.h"
#include "Board.h"

// Data to parse
const char raw[] =
  "/KFM5KAIFA-METER\r\n"
  "\r\n"
  "1-3:0.2.8(40)\r\n"
  "0-0:1.0.0(150117185916W)\r\n"
  "0-0:96.1.1(0000000000000000000000000000000000)\r\n"
  "1-0:1.8.1(000671.578*kWh)\r\n"
  "1-0:1.8.2(000842.472*kWh)\r\n"
  "1-0:2.8.1(000000.000*kWh)\r\n"
  "1-0:2.8.2(000000.000*kWh)\r\n"
  "0-0:96.14.0(0001)\r\n"
  "1-0:1.7.0(00.333*kW)\r\n"
  "1-0:2.7.0(00.000*kW)\r\n"
  "0-0:17.0.0(999.9*kW)\r\n"
  "0-0:96.3.10(1)\r\n"
  "0-0:96.7.21(00008)\r\n"
  "0-0:96.7.9(00007)\r\n"
  "1-0:99.97.0(1)(0-0:96.7.19)(000101000001W)(2147483647*s)\r\n"
  "1-0:32.32.0(00000)\r\n"
  "1-0:32.36.0(00000)\r\n"
  "0-0:96.13.1()\r\n"
  "0-0:96.13.0()\r\n"
  "1-0:31.7.0(001*A)\r\n"
  "1-0:21.7.0(00.332*kW)\r\n"
  "1-0:22.7.0(00.000*kW)\r\n"
  "0-1:24.1.0(003)\r\n"
  "0-1:96.1.0(0000000000000000000000000000000000)\r\n"
  "0-1:24.2.1(150117180000W)(00473.789*m3)\r\n"
  "0-1:24.4.0(1)\r\n"
  "!6F4A\r\n";

Board* board;
WiFiClient client;
long last_update = 0;

void setup() {  
  Serial.begin(115200);
  while (!Serial && millis() < 5000);
  delay(300);
  
  //board setup
  board = new Board(client);
  board->Setup();
}

void loop () {
  long cycleTime = millis();
  board->Loop();

  if(!board->connected()) {
    board->ReconnectMQTT();
  }

  String json = ReadTelegram();

  if(sizeof(json) > 0 && (cycleTime - last_update) > board->GetUpdateInterval()) {
    board->Publish("energy_sensor", (const char*)json.c_str());
    last_update = millis();
  }

}

String ReadTelegram() {
  Telegram data;
  String json;
  ParseResult<void> res = P1Parser::parse(&data, raw, lengthof(raw), true);

  if (res.err) {
    Serial.println(res.fullError(raw, raw + lengthof(raw)));
  } 
  else {
    JsonDocument doc;

    //power delivery
    doc["power_delivered"] = data.power_delivered.val();
    doc["power_returned"] = data.power_returned.val();
      
    // low and hight
    doc["power_delivered_l1"] = data.power_delivered_l1.val();
    doc["power_delivered_l2"] = data.power_delivered_l2.val();
    doc["power_returned_l1"] = data.power_returned_l1.val();
    doc["power_returned_l2"] = data.power_returned_l2.val();

    // tarif
    doc["energy_delivered_tariff1"] = data.energy_delivered_tariff1.val();
    doc["energy_delivered_tariff2"] = data.energy_delivered_tariff2.val();
    doc["energy_returned_tariff1"] = data.energy_returned_tariff1.val();
    doc["energy_returned_tariff2"] = data.energy_returned_tariff2.val();
    doc["electricity_tariff"] = (const char*)data.electricity_tariff.c_str();
      
    // power instant
    doc["voltage_l1"] = data.voltage_l1.val();
    doc["voltage_l2"] = data.voltage_l2.val();
    doc["voltage_l3"] = data.voltage_l3.val();

    // power current
    doc["current_l1"] = data.current_l1;
    doc["current_l2"] = data.current_l2;
    doc["current_l2"] = data.current_l3;

    // serialze data in json
    serializeJson(doc, json);
  }

  return json;
}

