#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "AudioPlayer.h"

Config config;
AudioPlayer* audioPlayer;

void audioPlayTaskCode(void*);

void setup() {
  Serial.begin(9600);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  // WiFi Connected, print IP to serial monitor
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  audioPlayer = new AudioPlayer();

  xTaskCreatePinnedToCore(
    audioPlayTaskCode,
    "Play audio",
    4096,
    NULL,
    10,
    NULL,
    0
  );
}

void audioPlayTaskCode(void* parameter) {
  audioPlayer->play();
  for(;;) {  
    audioPlayer->loop();
  }
}

void loop() {
  
}
