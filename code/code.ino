#include <ESP8266WiFi.h>
#include "pms5003.h"
#include "sht30.h"
#include "vz-89te.h"

#include "mqtt.h"

// BEGIN: Edit these variables to suit your setup

// WiFi details
#define SSID "9CC9EB4F3DFD"
#define PSK "SN3979757"

// MQTT broker details
#define BROKER_IP IPAddress(10, 0, 0, 12)
#define BROKER_PORT 1883

#define MQTT_ID "iot-iaq-sensor-3"
#define MQTT_USER "mqtt"
#define MQTT_PSK "5mjrq-m0oUy-UbtDu-3RQzf-yIRTv"

// Refresh interval (milliseconds)
#define REFRESH_INTERVAL 30000

// END: Edit these variables to suit your setup
// BEGIN: Only edit variables here if you know what you're doing

PMS5003 pms5003(13, 12, 9600);

VZ_89TE vz_89te(0x70, &Wire);
SHT30 sht30(0x44, &Wire);

// END: Only edit variables here if you know what you're doing

WiFiClient wifiClient;
MQTT mqtt(
    BROKER_IP,
    BROKER_PORT,
    wifiClient);

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSK);
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }

  pms5003.begin();

  Wire.begin();
  vz_89te.begin();

  sht30.begin();
  sht30.setHeater(false);
  
  delay(5000);
}

void printSensorValues()
{
  Serial.println("--- BEGIN SENSOR INFORMATION ---");
  Serial.println("SHT30: ");
  Serial.println("Temperature: " + String(sht30.temperature()) + "C");
  Serial.println("Humidity: " + String(sht30.humidity()) + "%");
  Serial.println();
  Serial.println("VZ-89TE: ");
  Serial.println("CO2: " + String(vz_89te.co2()) + "ppm");
  Serial.println("VOC: " + String(vz_89te.voc()) + "ppb");
  Serial.println();
  Serial.println("PMS5003: ");
  Serial.println("SP-PM1.0: " + String(pms5003.sp_pm10()));
  Serial.println("SP-PM2.5: " + String(pms5003.sp_pm25()));
  Serial.println("SP-PM10: " + String(pms5003.sp_pm100()));
  Serial.println("AE-PM1.0: " + String(pms5003.ae_pm10()));
  Serial.println("AE-PM2.5: " + String(pms5003.ae_pm25()));
  Serial.println("AE-PM10: " + String(pms5003.ae_pm100()));
  Serial.println("0.3 particles: " + String(pms5003.p03()));
  Serial.println("0.5 particles: " + String(pms5003.p05()));
  Serial.println("1.0 particles: " + String(pms5003.p10()));
  Serial.println("2.5 particles: " + String(pms5003.p25()));
  Serial.println("5.0 particles: " + String(pms5003.p50()));
  Serial.println("10.0 particles: " + String(pms5003.p100()));
  Serial.println();
  Serial.println("--- END SENSOR INFORMATION ---");
}

void loop()
{

  mqtt.reconnect(MQTT_ID, MQTT_USER, MQTT_PSK);
  mqtt.loop();

  String deviceID = String(MQTT_ID);

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