#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "AudioPlayer.h"

Config config;

void connectToWifi();
void startAudioPlayTask(void *);

void setup()
{
  Serial.begin(9600);

  connectToWifi();

  xTaskCreatePinnedToCore(
      startAudioPlayTask,
      "Play audio",
      4096,
      NULL,
      10,
      NULL,
      0);
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

void startAudioPlayTask(void *parameter)
{
  AudioPlayer audioPlayer;
  audioPlayer.play();
  for (;;)
  {
    audioPlayer.loop();
  }
}

void loop()
{
}
