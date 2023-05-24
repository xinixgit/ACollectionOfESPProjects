#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "AudioPlayer.h"
#include "MqttHandler.h"

#define ONE_MIN 60000

// Temperature Sensor
#define DHTPIN 2 // GPIO 2
#define DHTTYPE DHT11

Config config;
AudioPlayer *audioPlayer;
MqttHandler *mqttHandler;
DHT dht(DHTPIN, DHTTYPE);

void connectToWifi();
void connectToMqtt();
String createMqttPayload(float, float);
void setUpAudioPlayer(void *);
void publishTemperatureMetrics(void *);
void onAudioPlayerVolumeChange(const char *);
void onAudioPlayerStateChange(const char *);

void setup()
{
  Serial.begin(9600);

  connectToWifi();

  xTaskCreatePinnedToCore(
      setUpAudioPlayer,
      "Play audio",
      4096,
      NULL,
      10,
      NULL,
      0);

  delay(500);

  connectToMqtt();

  xTaskCreatePinnedToCore(
      publishTemperatureMetrics,
      "Publish temperature metrics",
      4096,
      NULL,
      10,
      NULL,
      1);

  delay(500);
}

void loop()
{
  delay(1000);
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
  mqttHandler = new MqttHandler(&config.mqtt_config);
  mqttHandler->onAudioPlayerVolumeChange(onAudioPlayerVolumeChange);
  mqttHandler->onAudioPlayerStateChange(onAudioPlayerStateChange);
  mqttHandler->connect();
}

void publishTemperatureMetrics(void *parameter)
{
  dht.begin();
  const TickType_t xDelay = ONE_MIN / portTICK_PERIOD_MS;
  for (;;)
  {
    float temperatureF = dht.readTemperature(true);
    float humidity = dht.readHumidity();
    String payload = createMqttPayload(temperatureF, humidity);
    mqttHandler->publishTemperatureSensorPayload(payload);
    vTaskDelay(xDelay);
  }
}

void setUpAudioPlayer(void *parameter)
{
  audioPlayer = new AudioPlayer();
  audioPlayer->play();
  for (;;)
  {
    audioPlayer->loop();
  }
}

String createMqttPayload(float temperatureF, float humidity)
{
  DynamicJsonDocument doc(64);
  doc["temperature_f"] = temperatureF;
  doc["humidity"] = humidity;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

void onAudioPlayerVolumeChange(const char *payload)
{
  audioPlayer->onVolumeChangeRequested(payload);
}

void onAudioPlayerStateChange(const char *payload)
{
  audioPlayer->onStateChangeRequested(payload);
}
