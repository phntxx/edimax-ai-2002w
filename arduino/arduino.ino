#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>

#include <EEPROM.h>

#include "pms5003.h"
#include "sht30.h"
#include "vz-89te.h"
#include "mqtt.h"

#define MQTT_ID "MQTT_ID"
#define MQTT_BROKER_HOST "MQTT_BROKER_HOST"
#define MQTT_BROKER_PORT 1883
#define MQTT_USER "MQTT_USER"
#define MQTT_PSK "MQTT_PSK"

#define REFRESH_INTERVAL 30000

// BEGIN: Only edit variables here if you know what you're doing

PMS5003 pms5003(13, 12, 9600);

VZ_89TE vz_89te(0x70, &Wire);
SHT30 sht30(0x44, &Wire);

// END: Only edit variables here if you know what you're doing

WiFiManager wifiManager;

WiFiClient wifiClient;
MQTT mqtt(
  MQTT_BROKER_HOST,
  MQTT_BROKER_PORT,
  wifiClient
);

void setup()
{

  // BEGIN: Set up serial connection
  Serial.begin(9600);
  Serial.println("Starting up...");

  // END: Set up serial connection
  // BEGIN: Set up WiFi and MQTT
  wifiManager.setConnectTimeout(10);

  bool result = wifiManager.autoConnect(MQTT_ID);

  if (!result) {
    Serial.println("Connection Failed! Rebooting...");
    ESP.restart();
  }
  
  // END: Set up WiFi and MQTT
  // BEGIN: Set up ArduinoOTA

  ArduinoOTA.setHostname(MQTT_ID);

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "type";
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error code: [%u]", error);
  });
  
  // END: Set up ArduinoOTA
  
  // Start up ArduinoOTA
  ArduinoOTA.begin();

  // Start up PMS5003 particulate matter sensor
  pms5003.begin();

  // Start up VZ_89TE CO2/VOC sensor
  Wire.begin();
  vz_89te.begin();

  // Start up SHT30 temperature/humidity sensor
  sht30.begin();
  sht30.setHeater(false);

  // Wait for all systems to properly power up
  delay(5000);
}

void printSensorValues()
{
  Serial.println("--- BEGIN SENSOR INFORMATION ---");

  Serial.println("SHT30: ");
  Serial.printf("Temperature: %fC\n", sht30.temperature());
  Serial.printf("Humidity: %f%%\n", sht30.humidity());
  Serial.println();

  Serial.println("VZ-89TE: ");
  Serial.printf("CO2: %f ppm\n", vz_89te.co2());
  Serial.printf("VOC: %f ppb\n", vz_89te.voc());
  Serial.println();

  Serial.println("PMS5003: ");
  Serial.printf("SP-PM1.0: %i\n", pms5003.sp_pm10());
  Serial.printf("SP-PM2.5: %i\n", pms5003.sp_pm25());
  Serial.printf("SP-PM10: %i\n", pms5003.sp_pm100());
  Serial.printf("AE-PM1.0: %i\n", pms5003.ae_pm10());
  Serial.printf("AE-PM2.5: %i\n", pms5003.ae_pm25());
  Serial.printf("AE-PM10: %i\n", pms5003.ae_pm100());
  Serial.printf("0.3 particles: %i\n", pms5003.p03());
  Serial.printf("0.5 particles: %i\n", pms5003.p05());
  Serial.printf("1.0 particles: %i\n", pms5003.p10());
  Serial.printf("2.5 particles: %i\n", pms5003.p25());
  Serial.printf("5.0 particles: %i\n", pms5003.p50());
  Serial.printf("10.0 particles: %i\n", pms5003.p100());
  Serial.println();

  Serial.println("--- END SENSOR INFORMATION ---");
}

void loop()
{
  String deviceID = String(MQTT_ID);

  if (!mqtt.connected()) {
    mqtt.reconnect(MQTT_ID, MQTT_USER, MQTT_PSK);
  }

  ArduinoOTA.handle();
  mqtt.loop();

  if (sht30.measure())
  {
    mqtt.publish(deviceID + "/sht30/temperature", sht30.temperature());
    mqtt.publish(deviceID + "/sht30/humidity", sht30.humidity());
  }

  if (vz_89te.measure())
  {
    mqtt.publish(deviceID + "/vz-89te/co2", vz_89te.co2());
    mqtt.publish(deviceID + "/vz-89te/voc", vz_89te.voc());
  }

  pms5003.wakeUp();
  if (pms5003.measure())
  {
    mqtt.publish(deviceID + "/pms5003/sp-pm10", pms5003.sp_pm10());
    mqtt.publish(deviceID + "/pms5003/sp-pm25", pms5003.sp_pm25());
    mqtt.publish(deviceID + "/pms5003/sp-pm100", pms5003.sp_pm100());
    mqtt.publish(deviceID + "/pms5003/ae-pm10", pms5003.ae_pm10());
    mqtt.publish(deviceID + "/pms5003/ae-pm25", pms5003.ae_pm25());
    mqtt.publish(deviceID + "/pms5003/ae-pm100", pms5003.ae_pm100());
    mqtt.publish(deviceID + "/pms5003/p03", pms5003.p03());
    mqtt.publish(deviceID + "/pms5003/p05", pms5003.p05());
    mqtt.publish(deviceID + "/pms5003/p10", pms5003.p10());
    mqtt.publish(deviceID + "/pms5003/p25", pms5003.p25());
    mqtt.publish(deviceID + "/pms5003/p50", pms5003.p50());
    mqtt.publish(deviceID + "/pms5003/p100", pms5003.p100());
  }
  pms5003.sleep();

  printSensorValues();
  delay(REFRESH_INTERVAL);
}