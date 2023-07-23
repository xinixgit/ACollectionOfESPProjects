#include "Config.h"
#include "SensorHandler.h"
#include "I2CAddressScanner.h"

TemperatureSensor *tempSensor;
AirQualitySensor *aqSensor;

void setup()
{
  Serial.begin(9600);

  TemperatureSensorConfig tempConfig = TemperatureSensorConfig(Type_ENS210);
  tempSensor = initTemperatureSensor(tempConfig);

  AirQualitySensorConfig aqConfig = AirQualitySensorConfig(ENS160);
  aqSensor = initAirQualitySensor(aqConfig);
}

void loop()
{
  String payload = tempSensor->createPayload();
  Serial.println(payload);
  payload = aqSensor->createPayload();
  Serial.println(payload);
  delay(30000);
}