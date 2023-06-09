#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <DHT.h>
#include <list>

typedef std::function<void(String)> PublishFn;

enum TemperatureSensorType
{
  DHT11Sensor,
  BME280Sensor
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

  virtual float readTemperatureF() { return 0; };

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

  float readTemperatureF()
  {
    return this->dht->readTemperature(true);
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

  float readTemperatureF()
  {
    return this->bme->readTemperature() * 1.8 + 32;
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

TemperatureSensor *initTemperatureSensor(TemperatureSensorType sensorType, PublishFn publishFn)
{
  TemperatureSensor *temperatureSensor = nullptr;
  switch (sensorType)
  {
  case DHT11Sensor:
  {
    DHT *dht = new DHT(DHTPIN, DHTTYPE);
    dht->begin();
    temperatureSensor = new DHT11TemperatureSensor(dht, publishFn);
    break;
  }
  case BME280Sensor:
  {
    TwoWire *I2CBME;
#ifdef ESP8266
    I2CBME = new TwoWire();
    I2CBME->begin(BME_I2C_SDA, BME_I2C_SCL);
#elif defined(ESP32)
    I2CBME = new TwoWire(0);
    bool status = I2CBME->begin(BME_I2C_SDA, BME_I2C_SCL);
    if (!status)
    {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      break;
    }
#else
#error "Unsupported platform"
#endif

    Adafruit_BME280 *bme = new Adafruit_BME280();
    bme->begin(0x76, I2CBME);
    temperatureSensor = new BME280TemperatureSensor(bme, publishFn);
    break;
  }

  default:
    break;
  }
  return temperatureSensor;
}

struct SensorHandler
{
  std::list<Sensor *> sensors;
  SensorHandler(std::list<Sensor *> sensors)
  {
    this->sensors = sensors;
  };

  SensorHandler(PublishFn publishFn, TemperatureSensorType temperatureSensorType = DHT11Sensor)
  {
    auto temperatureSensor = initTemperatureSensor(temperatureSensorType, publishFn);
    if (temperatureSensor != nullptr)
    {
      this->sensors.push_back(temperatureSensor);
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
