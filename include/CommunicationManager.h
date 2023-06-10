#include <AsyncMqttClient.h>
#include "MqttHandler.h"
#include "Config.h"

typedef std::function<void(const char *)> OnAudioPlayerVolumeChangeRequest;
typedef std::function<void(const char *)> OnAudioPlayerStateChangeRequest;

struct CommunicationManager
{
  MqttHandler *mqttHandler;

  CommunicationManager(MqttHandler *mqttHandler)
  {
    this->mqttHandler = mqttHandler;
    this->mqttHandler->onConnect([this](bool sessionPresent)
                                 { this->onConnect(sessionPresent); });
    this->mqttHandler->onMessage([this](char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
                                 { this->onMessage(topic, payload, properties, len, index, total); });
  }

  virtual void onConnect(bool sessionPresent)
  {
    Serial.println("Connected to MQTT.");
  }

  virtual void onMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
  {
    Serial.printf("Received message from MQTT topic %s with payload: %s\n", topic, payload);
  }
};

// ------------------------------ TemperatureSensorCommunicationManager ------------------------------
struct TemperatureSensorCommunicationManager : CommunicationManager
{
  TemperatureSensorCommunicationManager(MqttHandler *mqttHandler) : CommunicationManager(mqttHandler)
  {
  }

  void publishTemperature(String payload)
  {
    mqttHandler->publishPayload(MQTT_TOPIC_SENSOR_TEMPERATURE, payload.c_str());
  }
};

// ------------------------------ SoundPlayerCommunicationManager ------------------------------
struct SoundPlayerCommunicationManager : TemperatureSensorCommunicationManager
{
  OnAudioPlayerVolumeChangeRequest audioPlayerVolumeChangeRequestCallback;
  OnAudioPlayerStateChangeRequest audioPlayerStateChangeRequestCallback;

  SoundPlayerCommunicationManager(MqttHandler *mqttHandler) : TemperatureSensorCommunicationManager(mqttHandler)
  {
  }
  void publishTemperature(String payload)
  {
    TemperatureSensorCommunicationManager::publishTemperature(payload);
  }
  void publishAudioPlayerState(String payload)
  {
    mqttHandler->publishPayload(MQTT_TOPIC_AUDIOPLAYER_STATE_CHANGED, payload.c_str());
  }
  void onAudioPlayerVolumeChangeRequest(OnAudioPlayerVolumeChangeRequest callback)
  {
    audioPlayerVolumeChangeRequestCallback = callback;
  }
  void onAudioPlayerStateChangeRequest(OnAudioPlayerStateChangeRequest callback)
  {
    audioPlayerStateChangeRequestCallback = callback;
  }
  void onConnect(bool sessionPresent)
  {
    Serial.println("MQTT client is connected");
    for (String s : MqttTopicOfSubscription)
    {
      const char *topic = s.c_str();
      mqttHandler->subscribe(topic);
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
};
