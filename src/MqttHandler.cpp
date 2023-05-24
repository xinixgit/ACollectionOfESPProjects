#include <AsyncMqttClient.h>
#include "MqttHandler.h"

AsyncMqttClient mqttClient;
OnAudioPlayerVolumeChange audioPlayerVolumeChangeCallback;
OnAudioPlayerStateChange audioPlayerStateChangeCallback;

void onConnect(bool);
void onDisconnect(AsyncMqttClientDisconnectReason);
void onPublish(uint16_t);
void onMessage(char *, char *, AsyncMqttClientMessageProperties, size_t, size_t, size_t);

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

void MqttHandler::publishTemperatureSensorPayload(String payload)
{
  if (!mqttClient.connected())
  {
    Serial.println("MQTT client is disconnected, unable to send payload");
    return;
  }

  Serial.print("Publishing payload: ");
  Serial.println(payload);
  mqttClient.publish(MQTT_TOPIC_SENSOR_TEMPERATURE, 0, false, payload.c_str());
}

void MqttHandler::onAudioPlayerVolumeChange(OnAudioPlayerVolumeChange callback)
{
  audioPlayerVolumeChangeCallback = callback;
}

void MqttHandler::onAudioPlayerStateChange(OnAudioPlayerStateChange callback)
{
  audioPlayerStateChangeCallback = callback;
}

void onConnect(bool sessionPresent)
{
  Serial.println("MQTT client is connected");
  for (int i = 0; i < mqttTopicOfSubscription.size(); i++)
  {
    const char *topic = mqttTopicOfSubscription[i].c_str();
    mqttClient.subscribe(topic, 0);
    Serial.print("Subscribed to topic: ");
    Serial.println(topic);
  }
}

void onMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String fixed = ((String)payload).substring(0, len);
  Serial.printf("Payload received from topic %s: %s, %d, %d, %d", topic, fixed, len, index, total);
  Serial.println();

  if (strcmp(topic, MQTT_TOPIC_AUDIOPLAYER_STATE_CHANGE) == 0 && audioPlayerStateChangeCallback != NULL)
  {
    audioPlayerStateChangeCallback(fixed.c_str());
  }
  else if (strcmp(topic, MQTT_TOPIC_AUDIOPLAYER_VOL_CHANGE) == 0 && audioPlayerVolumeChangeCallback != NULL)
  {
    audioPlayerVolumeChangeCallback(fixed.c_str());
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