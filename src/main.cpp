#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "Config.h"
#include "AudioPlayer.h"
#include "MqttHandler.h"
#include "SensorHandler.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define TEN_MIN 600000
#define USE_BME_SENSOR true

Config config;
AudioPlayer *audioPlayer;
MqttHandler *mqttHandler;
SensorHandler *sensorHandler;

void connectToWifi();
void connectToMqtt();
void startAudioPlayer(void *);
void startSensor(void *);

void setup()
{
  Serial.begin(9600);

  connectToWifi();
  delay(500);

  mqttHandler = new MqttHandler(&config.mqtt_config);
  audioPlayer = new AudioPlayer();

  connectToMqtt();
  delay(500);

  xTaskCreatePinnedToCore(
      startAudioPlayer,
      "Play audio",
      4096,
      NULL,
      10,
      NULL,
      0);
  delay(500);

  xTaskCreatePinnedToCore(
      startSensor,
      "Start publishing all sensor metrics",
      4096,
      NULL,
      10,
      NULL,
      1);
  delay(500);
}

void loop()
{
}

void connectToWifi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // WiFi Connected, print IP to serial monitor
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

void connectToMqtt()
{
  mqttHandler->onAudioPlayerVolumeChangeRequest([](const char *payload)
                                                { audioPlayer->onVolumeChangeRequested(payload); });
  mqttHandler->onAudioPlayerStateChangeRequest([](const char *payload)
                                               { audioPlayer->onStateChangeRequested(payload); });
  mqttHandler->connect();
}

void startAudioPlayer(void *parameter)
{
  audioPlayer->setPublishStateFn([](String payload)
                                 { mqttHandler->publishAudioPlayerState(payload); });
  audioPlayer->play();
  for (;;)
  {
    audioPlayer->loop();
  }
}

void startSensor(void *parameter)
{
  Sensor *temperatureSensor;

  if (USE_BME_SENSOR)
  {
    TwoWire *I2CBME = new TwoWire(0);
    bool status = I2CBME->begin(BME_I2C_SDA, BME_I2C_SCL, 400000);
    if (!status)
    {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      return;
    }

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

  const TickType_t xDelay = TEN_MIN / portTICK_PERIOD_MS;
  for (;;)
  {
    sensorHandler->publishAll();
    vTaskDelay(xDelay);
  }
}
