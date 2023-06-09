#include <AsyncMqttClient.h>
#include "MqttHandler.h"
#include <vector>

AsyncMqttClient mqttClient;
OnAudioPlayerVolumeChangeRequest audioPlayerVolumeChangeRequestCallback;
OnAudioPlayerStateChangeRequest audioPlayerStateChangeRequestCallback;

void onConnect(bool);
void onDisconnect(AsyncMqttClientDisconnectReason);
void onPublish(uint16_t);
void onMessage(char *, char *, AsyncMqttClientMessageProperties, size_t, size_t, size_t);
void publishPayload(const char *, const char *);

MqttHandler::MqttHandler(MqttConfig *config)
{
  this->config = config;

  mqttClient.setServer(this->config->host, this->config->port);
  mqttClient.setCredentials(this->config->username.c_str(), this->config->password.c_str());
  mqttClient.onConnect(onConnect);
  mqttClient.onDisconnect(onDisconnect);
  mqttClient.onPublish(onPublish);
  mqttClient.onMessage(onMessage);
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

void MqttHandler::publishTemperature(String payload)
{
  publishPayload(MQTT_TOPIC_SENSOR_TEMPERATURE, payload.c_str());
}

void MqttHandler::publishAudioPlayerState(String payload)
{
  publishPayload(MQTT_TOPIC_AUDIOPLAYER_STATE_CHANGED, payload.c_str());
}

void MqttHandler::onAudioPlayerVolumeChangeRequest(OnAudioPlayerVolumeChangeRequest callback)
{
  audioPlayerVolumeChangeRequestCallback = callback;
}

void MqttHandler::onAudioPlayerStateChangeRequest(OnAudioPlayerStateChangeRequest callback)
{
  audioPlayerStateChangeRequestCallback = callback;
}

void publishPayload(const char *topic, const char *payload)
{
  if (!mqttClient.connected())
  {
    Serial.printf("MQTT client is disconnected, unable to send payload to topic %s\n", topic);
    return;
  }

  mqttClient.publish(topic, 0, false, payload);
  Serial.printf("Payload published to topic %s: %s\n", topic, payload);
}

void onConnect(bool sessionPresent)
{
  Serial.println("MQTT client is connected");
  for (String s : MqttTopicOfSubscription)
  {
    const char *topic = s.c_str();
    mqttClient.subscribe(topic, 0);
    Serial.print("Subscribed to topic: ");
    Serial.println(topic);
  }
}

void onMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String fixed = ((String)payload).substring(0, len);

  if (strcmp(topic, MQTT_TOPIC_AUDIOPLAYER_CHANGE_STATE) == 0 && audioPlayerStateChangeRequestCallback != NULL)
  {
    audioPlayerStateChangeRequestCallback(fixed.c_str());
  }
  else if (strcmp(topic, MQTT_TOPIC_AUDIOPLAYER_CHANGE_VOL) == 0 && audioPlayerVolumeChangeRequestCallback != NULL)
  {
    audioPlayerVolumeChangeRequestCallback(fixed.c_str());
  }
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