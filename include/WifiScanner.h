#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

String translateEncryptionType(uint8_t);

void scanWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int n = WiFi.scanNetworks();
  Serial.printf("found %d networks\n", n);

  for (int i = 0; i < n; ++i)
  {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(") ");
    Serial.print(" [");
    Serial.print(WiFi.channel(i));
    Serial.print("] ");
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    Serial.println(encryptionTypeDescription);
    delay(10);
  }
}

String translateEncryptionType(uint8_t encryptionType)
{
  switch (encryptionType)
  {
  case (0):
    return "Open";
  case (1):
    return "WEP";
  case (2):
    return "WPA_PSK";
  case (3):
    return "WPA2_PSK";
  case (4):
    return "WPA_WPA2_PSK";
  case (5):
    return "WPA2_ENTERPRISE";
  default:
    return "UNKOWN";
  }
}