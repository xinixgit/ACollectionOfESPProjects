#include "SensorHandler.h"

TemperatureSensor *initTemperatureSensor(TemperatureSensorConfig sensorConfig)
{
  switch (sensorConfig.type)
  {
  case NoopTempSensor:
  {
    return nullptr;
  }
  case DHT11Sensor:
  {
    DHT *dht = new DHT(sensorConfig.DHTPin, DHT11);
    dht->begin();
    return new DHT11TemperatureSensor(dht);
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
    return new BME280TemperatureSensor(bme);
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

    return new AHT21Sensor(aht);
  }

  default:
    Serial.printf("Cannot find the temp sensor type specified: %d", sensorConfig.type);
    return nullptr;
  }
}

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
    ScioSense_ENS160 *ens160 = new ScioSense_ENS160(ENS160_I2CADDR_1);
    ens160->setI2C(config.SDAPin, config.SCLPin);

    if (!ens160->begin())
    {
      Serial.println("Unable to start ENS160.");
      return nullptr;
    }

    if (!ens160->available())
    {
      Serial.println("ENS160 sensor is not available.");
      return nullptr;
    }

    if (!ens160->setMode(ENS160_OPMODE_STD))
    {
      Serial.println("Unable to set ENS160 to standard mode.");
      return nullptr;
    }

    return new ENS160Sensor(ens160, config.location);
  }
  case Type_MQ135:
  {
    MQ135 *sensor = new MQ135(A0);
    return new MQ135Sensor(sensor, config.location);
  }
  default:
  {
    return nullptr;
  }
  }
}