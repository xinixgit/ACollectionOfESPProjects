#include <AsyncMqttClient.h>
#include "MqttHandler.h"
#include <vector>

AsyncMqttClient mqttClient;

void onDisconnect(AsyncMqttClientDisconnectReason);
void onPublish(uint16_t);

MqttHandler::MqttHandler(MqttConfig *config)
{
  this->config = config;

  mqttClient.setCleanSession(this->config->cleanSession);
  mqttClient.setServer(this->config->host.c_str(), this->config->port);
  mqttClient.setCredentials(this->config->username.c_str(), this->config->password.c_str());
  mqttClient.onDisconnect(onDisconnect);
  mqttClient.onPublish(onPublish);
}

void MqttHandler::connect()
{
  Serial.printf("Connecting to MQTT with id %s...\n", mqttClient.getClientId());
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

void MqttHandler::publishPayload(String topic, String payload, bool retain)
{
  const char *topicCStr = topic.c_str();
  const char *payloadCStr = payload.c_str();

  if (!mqttClient.connected())
  {
    Serial.printf("MQTT client is disconnected, unable to send payload to topic %s\n", topicCStr);
    return;
  }

  mqttClient.publish(topicCStr, 0, retain, payloadCStr);
  Serial.printf("Payload published to topic %s: %s\n", topicCStr, payloadCStr);
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
