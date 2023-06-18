#include <AsyncMqttClient.h>
#include "MqttHandler.h"
#include <vector>

AsyncMqttClient mqttClient;

void onDisconnect(AsyncMqttClientDisconnectReason);
void onPublish(uint16_t);
void publishPayload(const char *, const char *);

MqttHandler::MqttHandler(MqttConfig *config)
{
  this->config = config;

  mqttClient.setCleanSession(this->config->cleanSession);
  mqttClient.setServer(this->config->host, this->config->port);
  mqttClient.setCredentials(this->config->username.c_str(), this->config->password.c_str());
  mqttClient.onDisconnect(onDisconnect);
  mqttClient.onPublish(onPublish);
}

void MqttHandler::connect()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void MqttHandler::disconnect()
{
  Serial.println("Disconnecting from MQTT...");
  mqttClient.disconnect();
}

void MqttHandler::subscribe(const char *topic, uint8_t qos)
{
  mqttClient.subscribe(topic, qos);
  Serial.printf("Subscribed to topic %s with qos %d\n", topic, qos);
}

void MqttHandler::onConnect(OnConnectCallback onConnectCallback)
{
  mqttClient.onConnect(onConnectCallback);
}

void MqttHandler::onMessage(OnMessageCallback onMessageCallback)
{
  mqttClient.onMessage(onMessageCallback);
}

void MqttHandler::publishPayload(const char *topic, const char *payload, bool retain)
{
  if (!mqttClient.connected())
  {
    Serial.printf("MQTT client is disconnected, unable to send payload to topic %s\n", topic);
    return;
  }

  mqttClient.publish(topic, 0, retain, payload);
  Serial.printf("Payload published to topic %s: %s\n", topic, payload);
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
