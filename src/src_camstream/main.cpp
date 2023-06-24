#include <WiFi.h>
#include <WifiClient.h>
#include "Config.h"
#include "MqttHandler.h"
#include "SensorHandler.h"
#include "CommunicationManager.h"
#include "ESPCamHandler.h"
#include "WebStreamer.h"

#define TEN_MIN 600000

Config config;
CamStreamConfig camConfig;
MqttHandler *mqttHandler;
TemperatureSensorCommunicationManager *communicationManager;
SensorHandler *sensorHandler;
ESPCamHandler *camHandler;
audp::WebStreamer *webStreamer;

bool setupComplete = false;

// functions declaration
void blinkLED(void *parameter);
void startWebStream(void *parameter);
void connectToWifi();
void initCommunicationManager();
void initSensorHandler();

void setup()
{
  Serial.begin(9600);
  pinMode(camConfig.LEDPin, OUTPUT);

  xTaskCreate(
      blinkLED,
      "Blink LED",
      1024,
      (void *)&setupComplete,
      10,
      NULL);

  connectToWifi();
  delay(500);

  camHandler = new ESPCamHandler();
  webStreamer = new audp::WebStreamer(camHandler);

  xTaskCreate(
      startWebStream,
      "Start web stream",
      4096,
      NULL,
      4,
      NULL);
  delay(500);

  mqttHandler = new MqttHandler(&config.mqtt_config);
  communicationManager = new TemperatureSensorCommunicationManager(mqttHandler);
  initSensorHandler();

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
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Mac address: %s\n", WiFi.macAddress().c_str());
  Serial.println("");
}

void onPictureRequest(const char *payload)
{
  Serial.println("Picture request received.");
  camHandler->takePicAndSave();
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

void startWebStream(void *parameter)
{
  webStreamer->begin();
  for (;;)
  {
    webStreamer->loop();
    delay(1000);
  }
}

void initSensorHandler()
{
  TemperatureSensorConfig tempSensorConfig = TemperatureSensorConfig([](String payload)
                                                                     { communicationManager->publishTemperature(payload); });
  tempSensorConfig.type = BME280Sensor;
  tempSensorConfig.SCLPin = camConfig.SCLPin;
  tempSensorConfig.SDAPin = tempSensorConfig.SDAPin;
  TemperatureSensor *tempSensor = initTemperatureSensor(tempSensorConfig);
  sensorHandler = new SensorHandler(std::list<Sensor *>{tempSensor});
}