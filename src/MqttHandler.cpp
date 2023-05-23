#include <AsyncMqttClient.h>
#include "MqttHandler.h"

AsyncMqttClient mqttClient;

void onConnect(bool);
void onDisconnect(AsyncMqttClientDisconnectReason);
void onPublish(uint16_t);

MqttHandler::MqttHandler(MqttConfig *config)
{
  this->config = config;

  mqttClient.setServer(this->config->host, this->config->port);
  mqttClient.setCredentials(this->config->username.c_str(), this->config->password.c_str());
  mqttClient.onConnect(onConnect);
  mqttClient.onDisconnect(onDisconnect);
  mqttClient.onPublish(onPublish);
}

void MqttHandler::connect()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void MqttHandler::publishPayload(String payload)
{
  if (!mqttClient.connected())
  {
    Serial.println("MQTT client is disconnected, unable to send payload");
    return;
  }

  Serial.print("Publishing payload: ");
  Serial.println(payload);
  mqttClient.publish(this->config->topic.c_str(), 0, false, payload.c_str());
}

void onConnect(bool sessionPresent)
{
  Serial.print("MQTT client is connected");
}

void onDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.print("MQTT client is disconnected with reason: ");
  int r = (int8_t)reason;
  Serial.println(r);
}

void onPublish(uint16_t packetId)
{
  Serial.print("Payload published with packet id: ");
  Serial.println(packetId);
}