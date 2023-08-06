#include <WiFi.h>
#include <WifiClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "SD_MMC.h"
#include "Config.h"
#include "MqttHandler.h"
#include "SensorHandler.h"
#include "CommunicationManager.h"
#include "ESPCamHandler.h"
#include "WebStreamer.h"

#define TEN_MIN_IN_MS 600000

Config config;
CamStreamConfig camConfig;
MqttHandler mqttHandler;
CamStreamCommunicationManager commMgr;
SensorHandler sensorHandler;
ESPCamHandler camHandler;
audp::WebStreamer webStreamer;
AsyncWebServer server(80);

bool setupComplete = false;

// functions declaration
void blinkLED(void *);
void startWebServer();
void connectToWifi();
void initSensorHandler();
void initSD();
void initCommMgr();

void setup()
{
  Serial.begin(9600);

  pinMode(camConfig.LEDPin, OUTPUT);
  pinMode(camConfig.SCLPin, INPUT);
  pinMode(camConfig.SDAPin, INPUT);

  xTaskCreate(
      blinkLED,
      "Blink LED",
      1024,
      (void *)&setupComplete,
      10,
      NULL);

  connectToWifi();
  delay(500);

  mqttHandler.init(config.mqtt_config);
  delay(500);

  mqttHandler.connect();
  delay(500);

  initCommMgr();
  delay(500);

  initSensorHandler();

  initSD();

  camHandler.init();
  delay(500);

  startWebServer();
  delay(500);

  setupComplete = true;
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED || !mqttHandler.isConnected())
  {
    ESP.restart();
  }

  sensorHandler.publishAll();
  delay(TEN_MIN_IN_MS);
}

void connectToWifi()
{
  Serial.print("Connecting to Wi-Fi network ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("Connected to Wi-Fi.");
  Serial.println(WiFi.localIP());
}

void onRestartRequest()
{
  ESP.restart();
}

void onPictureRequest()
{
  camHandler.takePicAndSave();
}

void blinkLED(void *parameter)
{
  bool *setupComplete = (bool *)parameter;
  while (*setupComplete == false)
  {
    digitalWrite(camConfig.LEDPin, LOW); // LOW turns on the LED
    delay(250);
    digitalWrite(camConfig.LEDPin, HIGH);
    delay(250);
  }

  // turn on the LED so we know system is working
  digitalWrite(camConfig.LEDPin, LOW);
  vTaskDelete(NULL);
}

void startWebServer()
{
  AsyncElegantOTA.begin(&server);

  webStreamer.setCamHandler(&camHandler);
  webStreamer.begin(server);

  // server.serveStatic("/", SD_MMC, "/");
  server.begin();
}

void initSD()
{
  Serial.println("Starting SD Card");
  if (!SD_MMC.begin())
  {
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD Card attached");
    return;
  }
}

void initCommMgr()
{
  commMgr.init(
      &mqttHandler,
      camConfig,
      [](std::string payload)
      { onRestartRequest(); },
      [](std::string payload)
      { onPictureRequest(); });
}

void initSensorHandler()
{
  TemperatureSensorConfig tempSensorConfig = TemperatureSensorConfig(AHT21);
  tempSensorConfig.SCLPin = camConfig.SCLPin;
  tempSensorConfig.SDAPin = camConfig.SDAPin;
  sensorHandler.init(tempSensorConfig, &commMgr);
}