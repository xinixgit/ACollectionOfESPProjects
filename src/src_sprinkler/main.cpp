#include <WiFi.h>
#include <WifiClient.h>
#include "Config.h"
#include "MqttHandler.h"
#include "CommunicationManager.h"
#include "ESPCameraHandler.h"

#define TEN_MIN 600000

Config config;
MqttHandler *mqttHandler;
SprinklerCommunicationManager *communicationManager;
ESPCameraHandler *cam;

// functions declaration
void connectToWifi();
void initCommunicationManager();

void setup()
{
  Serial.begin(9600);

  cam = new ESPCameraHandler();
  delay(1000);

  mqttHandler = new MqttHandler(&config.mqtt_config);
  initCommunicationManager();

  connectToWifi();
  delay(500);

  mqttHandler->connect();
  delay(500);
}

void loop()
{
  delay(1000);
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
  // digitalWrite(SPRINKLER_WATER, HIGH);
  Serial.println("Sprinkler started watering.");
}

void onWaterOffRequest(const char *payload)
{
  // digitalWrite(SPRINKLER_WATER, LOW);
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
  cam->takePic();
}

void onStartVideoRequest(const char *payload)
{
  Serial.println("Start video request received.");
}

void onStopVideoRequest(const char *payload)
{
  Serial.println("Stop video request received.");
}

void initCommunicationManager()
{
  communicationManager = new SprinklerCommunicationManager(mqttHandler);
  communicationManager->onScheduledWaterRequest(onScheduledWaterRequest);
  communicationManager->onWaterRequest(onWaterOnRequest, onWaterOffRequest);
  communicationManager->onPictureRequest(onPictureRequest);
  communicationManager->onVideoRequest(onStartVideoRequest, onStopVideoRequest);
}
