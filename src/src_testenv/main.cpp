#include "Config.h"
#include "SensorHandler.h"
#include "WifiScanner.h"

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  scanWifi();
  Serial.println("Scan done");
  Serial.println("");
  delay(10000);
}