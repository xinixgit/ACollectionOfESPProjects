#include <ArduinoJson.h>
#include "SensorHandler.h"

String TemperatureSensor::createPayload()
{
  float temperatureF = this->dht->readTemperature(true);
  float humidity = this->dht->readHumidity();
  DynamicJsonDocument doc(64);
  doc["temperature_f"] = temperatureF;
  doc["humidity"] = humidity;

  String payload;
  serializeJson(doc, payload);
  return payload;
}
