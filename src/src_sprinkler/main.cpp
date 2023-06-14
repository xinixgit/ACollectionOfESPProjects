#include <WiFi.h>
#include <WifiClient.h>
#include "Config.h"
#include "MqttHandler.h"
#include "CommunicationManager.h"
#include "ESPCamHandler.h"
#include "WebStreamer.h"

#define TEN_MIN 600000

Config config;
MqttHandler *mqttHandler;
SprinklerCommunicationManager *communicationManager;
ESPCamHandler *camHandler;
audp::WebStreamer *webStreamer;

bool setupComplete = false;

// functions declaration
void blinkLED(void *parameter);
void startWebStream(void *parameter);
void connectToWifi();
void initCommunicationManager();

void setup()
{
  Serial.begin(9600);
  pinMode(ESP32CAM_LED, OUTPUT);
  pinMode(SPRINKLER_WATER, OUTPUT);

  xTaskCreatePinnedToCore(
      blinkLED,
      "Blink LED",
      1024,
      (void *)&setupComplete,
      10,
      NULL,
      0);

  connectToWifi();
  delay(500);

  camHandler = new ESPCamHandler();
  webStreamer = new audp::WebStreamer(camHandler);
  xTaskCreatePinnedToCore(
      startWebStream,
      "Start web stream",
      4096,
      NULL,
      4,
      NULL,
      0);
  delay(500);

  mqttHandler = new MqttHandler(&config.mqtt_config);
  initCommunicationManager();

  mqttHandler->connect();
  delay(500);

  setupComplete = true;
}

void loop()
{
}

void connectToWifi()
{
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to Wi-Fi network ");
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

void onWaterOnRequest(const char *payload)
{
  digitalWrite(SPRINKLER_WATER, HIGH);
  Serial.println("Sprinkler started watering.");
}

void onWaterOffRequest(const char *payload)
{
  digitalWrite(SPRINKLER_WATER, LOW);
  Serial.println("Sprinkler stopped watering.");
}

void onScheduledWaterRequest(const char *payload)
{
  int durationInSec = std::stoi(payload);
  onWaterOnRequest(payload);
  delay(durationInSec * 1000);
  onWaterOffRequest(payload);
}

void onPictureRequest(const char *payload)
{
  Serial.println("Picture request received.");
  camHandler->takePicAndSave();
}

void initCommunicationManager()
{
  communicationManager = new SprinklerCommunicationManager(mqttHandler);
  communicationManager->onScheduledWaterRequest(onScheduledWaterRequest);
  communicationManager->onWaterRequest(onWaterOnRequest, onWaterOffRequest);
  communicationManager->onPictureRequest(onPictureRequest);
}

void blinkLED(void *parameter)
{
  bool *setupComplete = (bool *)parameter;
  while (*setupComplete == false)
  {
    digitalWrite(ESP32CAM_LED, HIGH);
    delay(250);
    digitalWrite(ESP32CAM_LED, LOW);
    delay(250);
  }

  // turn on the LED so we know system is working
  digitalWrite(ESP32CAM_LED, LOW);
  vTaskDelete(NULL);
}

void startWebStream(void *parameter)
{
  webStreamer->begin();
  for (;;)
  {
    webStreamer->loop();
    delay(1000);
  }
}