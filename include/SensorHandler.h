#ifndef SENSORHANDLER_H
#define SENSORHANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_AHTX0.h>
#include <ScioSense_ENS160.h>
#include <DHT.h>
#include "MQ135.h"
#include <list>
#include "Config.h"
#include "CommunicationManager.h"

enum SensorCategory
{
  Unknown,
  Temperature,
  AirQuality
};

enum TemperatureSensorType
{
  NoopTempSensor,
  DHT11Sensor,
  BME280Sensor,
  AHT21
};

enum AirQualitySensorType
{
  NoSensor,
  ENS160,
  Type_MQ135
};

struct TemperatureSensorConfig
{
  TemperatureSensorType type;
  uint16_t DHTPin;
  uint16_t SCLPin;
  uint16_t SDAPin;

  TemperatureSensorConfig() : TemperatureSensorConfig(NoopTempSensor){};

  TemperatureSensorConfig(
      TemperatureSensorType type,
      uint16_t DHTPin = DHTPIN,
      uint16_t SCLPin = SCL, // default SCL on a ESP8266 is 5
      uint16_t SDAPin = SDA) // default SDA on a ESP8266 is 4
  {
    this->type = type;
    this->DHTPin = DHTPin;
    this->SCLPin = SCLPin;
    this->SDAPin = SDAPin;
  }
};

struct AirQualitySensorConfig
{
  AirQualitySensorType type;
  String location;
  uint16_t SCLPin;
  uint16_t SDAPin;

  AirQualitySensorConfig() : AirQualitySensorConfig(NoSensor){};

  AirQualitySensorConfig(
      AirQualitySensorType type,
      String location = LOCATION_TEST,
      uint16_t SCLPin = SCL, // default SCL on a ESP8266 is 5
      uint16_t SDAPin = SDA) // default SDA on a ESP8266 is 4)
  {
    this->type = type;
    this->location = location;
    this->SCLPin = SCLPin;
    this->SDAPin = SDAPin;
  }
};

struct Sensor
{
  virtual String createPayload() { return ""; };

  virtual SensorCategory getCategory() { return Unknown; }
};

struct TemperatureSensor : Sensor
{

  TemperatureSensor() {}

  virtual float readTemperature() { return 0; }

  virtual float readTemperatureF() { return readTemperature() * 1.8 + 32; };

  virtual float readHumidity() { return 0; };

  virtual float readPressure() { return 0; };

  virtual float readAltitude() { return 0; };

  String createPayload()
  {
    DynamicJsonDocument doc(128);
    doc["temperature_f"] = this->readTemperatureF();
    doc["humidity"] = this->readHumidity();
    doc["pressure"] = this->readPressure();
    doc["altitude"] = this->readAltitude();

    String payload;
    serializeJson(doc, payload);
    return payload;
  }

  SensorCategory getCategory() { return Temperature; }
};

struct DHT11TemperatureSensor : TemperatureSensor
{
  DHT *dht;
  DHT11TemperatureSensor(DHT *dht)
  {
    this->dht = dht;
  }

  float readTemperature()
  {
    return this->dht->readTemperature(false);
  }

  float readHumidity()
  {
    return this->dht->readHumidity();
  }
};

struct BME280TemperatureSensor : TemperatureSensor
{
  Adafruit_BME280 *bme;

  BME280TemperatureSensor(Adafruit_BME280 *bme)
  {
    this->bme = bme;
  }

  float readTemperature()
  {
    return this->bme->readTemperature();
  }

  float readHumidity()
  {
    return this->bme->readHumidity();
  }

  float readPressure()
  {
    return this->bme->readPressure() / 100.0F;
  }

  float readAltitude()
  {
    return this->bme->readAltitude(1013.25); // 1013.25 is sea level pressure
  }
};

struct AHT21Sensor : TemperatureSensor
{
  Adafruit_AHTX0 *aht;
  AHT21Sensor(Adafruit_AHTX0 *aht)
  {
    this->aht = aht;
  }

  float readTemperature()
  {
    sensors_event_t temperature;
    this->aht->getEvent(nullptr, &temperature);
    return temperature.temperature;
  }

  float readHumidity()
  {
    sensors_event_t humidity;
    this->aht->getEvent(&humidity, nullptr);
    return humidity.relative_humidity;
  }
};

struct AirQualitySensor : Sensor
{
  String location;

  AirQualitySensor(String location)
  {
    this->location = location;
  }

  /**
   * Get the air quality index
   * Return value: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy
   */
  virtual uint8_t getAQI() { return 0; };

  /**
   * Get TVOC concentration
   * Return value range: 0–65000, unit: ppb
   */
  virtual uint16_t getTVOC() { return 0; };

  /**
   * Get CO2 equivalent concentration calculated according to the detected data of VOCs and hydrogen (eCO2 – Equivalent CO2)
   * Return value range: 400–65000, unit: ppm
   * Five levels: Excellent(400 - 600), Good(600 - 800), Moderate(800 - 1000),
   *               Poor(1000 - 1500), Unhealthy(> 1500)
   */
  virtual uint16_t getECO2() { return 0; };

  /**
   * Experts measure air quality using parts per million, or PPM. This indicates how many milligrams
   * of pollutants there are per square meter of air. However, this measurement isn’t necessarily
   * meaningful to the average person who wants to ensure the air they breathe is safe.
   *
   * To that end, the US Environmental Protection Agency (EPA) developed the Air Quality Index, or
   * AQI, to measure air pollution in a way average citizens can understand.
   *
   */
  virtual uint16_t getAirQuality() { return 0; };

  String getLocation() { return location; }

  SensorCategory getCategory() { return AirQuality; }

  String createPayload()
  {
    DynamicJsonDocument doc(128);
    doc["location"] = this->getLocation();
    doc["aqi"] = this->getAQI();
    doc["tvoc"] = this->getTVOC();
    doc["co2"] = this->getECO2();
    doc["aq"] = this->getAirQuality();

    String payload;
    serializeJson(doc, payload);
    return payload;
  }
};

struct ENS160Sensor : AirQualitySensor
{
  ScioSense_ENS160 *sensor;

  ENS160Sensor(ScioSense_ENS160 *sensor, String location) : AirQualitySensor(location)
  {
    this->sensor = sensor;
  }

  uint8_t getAQI()
  {
    return this->sensor->getAQI();
  };

  /**
   * Get TVOC concentration
   * Return value range: 0–65000, unit: ppb
   */
  uint16_t getTVOC()
  {
    return this->sensor->getTVOC();
  };

  /**
   * Get CO2 equivalent concentration calculated according to the detected data of VOCs and hydrogen (eCO2 – Equivalent CO2)
   * Return value range: 400–65000, unit: ppm
   * Five levels: Excellent(400 - 600), Good(600 - 800), Moderate(800 - 1000),
   *               Poor(1000 - 1500), Unhealthy(> 1500)
   */
  uint16_t getECO2()
  {
    return this->sensor->geteCO2();
  };
};

struct MQ135Sensor : AirQualitySensor
{
  MQ135 *sensor;

  MQ135Sensor(MQ135 *sensor, String location) : AirQualitySensor(location)
  {
    this->sensor = sensor;
  }

  uint16_t getAirQuality()
  {
    return analogRead(A0);
  };
};

TemperatureSensor *initTemperatureSensor(TemperatureSensorConfig);
AirQualitySensor *initAirQualitySensor(AirQualitySensorConfig);

struct SensorHandler
{
  std::list<Sensor *> sensors;
  TempSensorCommunicationManager *tempCm;
  AirQualitySensorCommunicationManager *aqCm;

  SensorHandler(std::list<Sensor *> sensors)
  {
    this->sensors = sensors;
  };

  SensorHandler(
      AirQualitySensorConfig aqSensorConfig,
      AirQualitySensorCommunicationManager *aqCm) : SensorHandler(TemperatureSensorConfig(), nullptr, aqSensorConfig, aqCm) {}

  SensorHandler(
      TemperatureSensorConfig tempSensorConfig,
      TempSensorCommunicationManager *tempCm) : SensorHandler(tempSensorConfig, tempCm, AirQualitySensorConfig(), nullptr) {}

  SensorHandler(
      TemperatureSensorConfig tempSensorConfig,
      TempSensorCommunicationManager *tempCm,
      AirQualitySensorConfig aqSensorConfig,
      AirQualitySensorCommunicationManager *aqCm)
  {
    this->tempCm = tempCm;
    this->aqCm = aqCm;

    auto tempSensor = initTemperatureSensor(tempSensorConfig);
    if (tempSensor != nullptr)
    {
      this->sensors.push_back(tempSensor);
    }

    auto aqSensor = initAirQualitySensor(aqSensorConfig);
    if (aqSensor != nullptr)
    {
      this->sensors.push_back(aqSensor);
    }
  }

  void publishAll()
  {
    for (auto sensor : sensors)
    {
      if (sensor->getCategory() == Temperature && tempCm != nullptr)
      {
        String json = sensor->createPayload();
        tempCm->publishTemperature(json);
      }
      else if (sensor->getCategory() == AirQuality && aqCm != nullptr)
      {
        String json = sensor->createPayload();
        aqCm->publishAirQuality(json);
      }
    }
  }
};

#endif