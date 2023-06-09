#include <ESP8266WiFi.h>
#include <WifiClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Config.h"
#include "MqttHandler.h"
#include "SensorHandler.h"

#define TEN_MIN 600000

#define USE_BME_SENSOR false

Config config;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
MqttHandler *mqttHandler;
SensorHandler *sensorHandler;

// functions declaration
void connectToWifi();
void initSensors();
void onWifiConnect(const WiFiEventStationModeGotIP &);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &);

void setup()
{
  Serial.begin(9600);

  mqttHandler = new MqttHandler(&config.mqtt_config);
  initSensors();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
}

void loop()
{
  connectToWifi();
  delay(500);

  mqttHandler->connect();
  delay(500);

  sensorHandler->publishAll();
  delay(500);

  mqttHandler->disconnect();

  WiFi.mode(WIFI_OFF);

  WiFi.forceSleepBegin();

  delay(TEN_MIN);

  WiFi.forceSleepWake();
  delay(500);
}

void connectToWifi()
{
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to Wi-Fi network ");
  WiFi.begin(config.wifi_ssid, config.wifi_password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
}

void initSensors()
{
  Sensor *temperatureSensor;

  if (USE_BME_SENSOR)
  {
    TwoWire *I2CBME = new TwoWire();
    I2CBME->begin(BME_I2C_SDA, BME_I2C_SCL);
    // if (!status)
    // {
    //   Serial.println("Could not find a valid BME280 sensor, check wiring!");
    //   return;
    // }

    Adafruit_BME280 *bme = new Adafruit_BME280();
    bme->begin(0x76, I2CBME);
    temperatureSensor = new BME280TemperatureSensor(bme, [](String payload)
                                                    { mqttHandler->publishTemperature(payload); });
  }
  else
  {
    // default use DHT11 sensor
    DHT *dht = new DHT(DHTPIN, DHTTYPE);
    dht->begin();
    temperatureSensor = new DHT11TemperatureSensor(dht, [](String payload)
                                                   { mqttHandler->publishTemperature(payload); });
  }

  std::list<Sensor *> sensors = {temperatureSensor};
  sensorHandler = new SensorHandler(sensors);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Connected to Wi-Fi.");
  Serial.println(WiFi.localIP());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  Serial.println("Disconnected from Wi-Fi.");
}
