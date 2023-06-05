#include <Arduino.h>
#include <DHT.h>
#include <list>

typedef std::function<void(String)> PublishFn;

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
  DHT *dht;
  TemperatureSensor(DHT *dht, PublishFn publishFn) : Sensor(publishFn)
  {
    this->dht = dht;
  }

  String createPayload();
};

struct SensorHandler
{
  std::list<Sensor *> sensors;
  SensorHandler(std::list<Sensor *> sensors)
  {
    this->sensors = sensors;
  };

  void publishAll()
  {
    for (auto sensor : sensors)
    {
      sensor->publishFn(sensor->createPayload());
    }
  }
};
