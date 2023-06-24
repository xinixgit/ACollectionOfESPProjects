#ifndef SENSORHANDLER_H
#define SENSORHANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_AHTX0.h>
#include <DFRobot_ENS160.h>
#include <DHT.h>
#include <list>

typedef std::function<void(String)> PublishFn;

enum TemperatureSensorType
{
  DHT11Sensor,
  BME280Sensor,
  AHT21
};

enum AirQualitySensorType
{
  NoSensor,
  ENS160
};

struct TemperatureSensorConfig
{
  PublishFn publishFn;
  TemperatureSensorType type;
  uint16_t DHTPin;
  uint16_t SCLPin;
  uint16_t SDAPin;

  TemperatureSensorConfig(
      PublishFn publishFn,
      TemperatureSensorType type = DHT11Sensor,
      uint16_t DHTPin = DHTPIN,
      uint16_t SCLPin = SCL, // default SCL on a ESP8266 is 5
      uint16_t SDAPin = SDA) // default SDA on a ESP8266 is 4
  {
    this->publishFn = publishFn;
    this->type = type;
    this->DHTPin = DHTPin;
    this->SCLPin = SCLPin;
    this->SDAPin = SDAPin;
  }
};

struct AirQualitySensorConfig
{
  PublishFn publishFn;
  AirQualitySensorType type;
  uint16_t SCLPin;
  uint16_t SDAPin;
  float nominalTemperature;
  float nominalHumidity;

  AirQualitySensorConfig()
  {
    this->type = NoSensor;
  }

  AirQualitySensorConfig(
      PublishFn publishFn,
      AirQualitySensorType type = ENS160,
      uint16_t SCLPin = SCL, // default SCL on a ESP8266 is 5
      uint16_t SDAPin = SDA, // default SDA on a ESP8266 is 4
      float nominalTemperature = 22.78,
      float nominalHumidity = 42)
  {
    this->publishFn = publishFn;
    this->type = type;
    this->SCLPin = SCLPin;
    this->SDAPin = SDAPin;
    this->nominalTemperature = nominalTemperature;
    this->nominalHumidity = nominalHumidity;
  }
};

struct Sensor
{
  PublishFn publishFn;
  Sensor(PublishFn publishFn)
  {
    this->publishFn = publishFn;
  }

  virtual String createPayload() { return ""; };
};

struct TemperatureSensor : Sensor
{
  TemperatureSensor(PublishFn publishFn) : Sensor(publishFn)
  {
  }

  virtual float readTemperature() { return 0; }

  virtual float readTemperatureF() { return readTemperature() * 1.8 + 32; };

  virtual float readHumidity() { return 0; };

  virtual float readPressure() { return 0; };

  virtual float readAltitude() { return 0; };

  String createPayload()
  {
    DynamicJsonDocument doc(64);
    doc["temperature_f"] = this->readTemperatureF();
    doc["humidity"] = this->readHumidity();
    doc["pressure"] = this->readPressure();
    doc["altitude"] = this->readAltitude();

    String payload;
    serializeJson(doc, payload);
    return payload;
  }
};

struct DHT11TemperatureSensor : TemperatureSensor
{
  DHT *dht;
  DHT11TemperatureSensor(DHT *dht, PublishFn publishFn) : TemperatureSensor(publishFn)
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

  BME280TemperatureSensor(Adafruit_BME280 *bme, PublishFn publishFn) : TemperatureSensor(publishFn)
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
  AHT21Sensor(Adafruit_AHTX0 *aht, PublishFn publishFn) : TemperatureSensor(publishFn)
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

TemperatureSensor *initTemperatureSensor(TemperatureSensorConfig sensorConfig)
{
  switch (sensorConfig.type)
  {
  case DHT11Sensor:
  {
    DHT *dht = new DHT(sensorConfig.DHTPin, DHT11);
    dht->begin();
    return new DHT11TemperatureSensor(dht, sensorConfig.publishFn);
  }
  case BME280Sensor:
  {
    TwoWire *I2CBME;
#ifdef ESP8266
    I2CBME = new TwoWire();
    I2CBME->begin(sensorConfig.SDAPin, sensorConfig.SCLPin);
#elif defined(ESP32)
    I2CBME = new TwoWire(0);
    bool status = I2CBME->begin(sensorConfig.SDAPin, sensorConfig.SCLPin);
    if (!status)
    {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      return nullptr;
    }
#else
#error "Unsupported platform"
#endif

    Adafruit_BME280 *bme = new Adafruit_BME280();
    bme->begin(0x76, I2CBME);
    return new BME280TemperatureSensor(bme, sensorConfig.publishFn);
  }
  case AHT21:
  {
    TwoWire *wire;
#ifdef ESP8266
    wire = new TwoWire();
    wire->begin(sensorConfig.SDAPin, sensorConfig.SCLPin);
#elif defined(ESP32)
    wire = new TwoWire(0);
    bool status = wire->begin(sensorConfig.SDAPin, sensorConfig.SCLPin);
    if (!status)
    {
      Serial.println("Could not start wiring between SDA & SCL, check wiring!");
      return nullptr;
    }
#endif

    Adafruit_AHTX0 *aht = new Adafruit_AHTX0();
    if (!aht->begin(wire))
    {
      Serial.println("Could not start AHT sensor, check wiring!");
      return nullptr;
    }

    return new AHT21Sensor(aht, sensorConfig.publishFn);
  }

  default:
    Serial.printf("Cannot find the temp sensor type specified: %d", sensorConfig.type);
    return nullptr;
  }
}

struct AirQualitySensor : Sensor
{
  AirQualitySensor(PublishFn publishFn) : Sensor(publishFn)
  {
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

  String createPayload()
  {
    DynamicJsonDocument doc(64);
    doc["aqi"] = this->getAQI();
    doc["tvoc"] = this->getTVOC();
    doc["co2"] = this->getECO2();

    String payload;
    serializeJson(doc, payload);
    return payload;
  }
};

struct ENS160Sensor : AirQualitySensor
{
  DFRobot_ENS160 *sensor;

  ENS160Sensor(DFRobot_ENS160 *sensor, PublishFn publishFn) : AirQualitySensor(publishFn)
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
    return this->sensor->getECO2();
  };
};

AirQualitySensor *initAirQualitySensor(AirQualitySensorConfig config)
{
  switch (config.type)
  {
  case NoSensor:
  {
    return nullptr;
  }
  case ENS160:
  {
    TwoWire *wire;
#ifdef ESP8266
    wire = new TwoWire();
    wire->begin(config.SDAPin, config.SCLPin);
#elif defined(ESP32)
    wire = new TwoWire(0);
    bool status = wire->begin(config.SDAPin, config.SCLPin);
    if (!status)
    {
      Serial.println("Could not find a valid ENS160 sensor, check wiring!");
      return nullptr;
    }
#endif

    DFRobot_ENS160_I2C *ens160 = new DFRobot_ENS160_I2C(wire, 0x53);
    if (NO_ERR != ens160->begin())
    {
      Serial.println("Unable to start ENS160 sensor");
      return nullptr;
    }
    ens160->setPWRMode(ENS160_STANDARD_MODE);
    ens160->setTempAndHum(config.nominalTemperature, config.nominalHumidity);
    Serial.printf("ENS160 operational status is: %d\n", ens160->getENS160Status());

    return new ENS160Sensor(ens160, config.publishFn);
  }
  default:
  {
    return nullptr;
  }
  }
}

struct SensorHandler
{
  std::list<Sensor *> sensors;
  SensorHandler(std::list<Sensor *> sensors)
  {
    this->sensors = sensors;
  };

  SensorHandler(
      TemperatureSensorConfig temperatureSensorConfig,
      AirQualitySensorConfig airQualitySensorConfig = AirQualitySensorConfig())
  {
    auto temperatureSensor = initTemperatureSensor(temperatureSensorConfig);
    if (temperatureSensor != nullptr)
    {
      this->sensors.push_back(temperatureSensor);
    }

    auto airQualitySensor = initAirQualitySensor(airQualitySensorConfig);
    if (airQualitySensor != nullptr)
    {
      this->sensors.push_back(airQualitySensor);
    }
  }

  void publishAll()
  {
    for (auto sensor : sensors)
    {
      sensor->publishFn(sensor->createPayload());
    }
  }
};

#endif