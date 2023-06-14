#include <AsyncMqttClient.h>
#include <string.h>
#include <map>
#include "MqttHandler.h"
#include "Config.h"

typedef std::function<void(const char *)> MessageTriggeredAction;

MessageTriggeredAction onOffAction(MessageTriggeredAction onAction, MessageTriggeredAction offAction)
{
  return [onAction, offAction](const char *payload)
  {
    if (strcmp(payload, "on") == 0)
    {
      onAction(payload);
    }
    else if (strcmp(payload, "off") == 0)
    {
      offAction(payload);
    }
  };
}

struct CommunicationManager
{
  MqttHandler *mqttHandler;
  std::map<const char *, MessageTriggeredAction> messageTriggeredActions;

  CommunicationManager(MqttHandler *mqttHandler, std::map<const char *, MessageTriggeredAction> messageTriggeredActions = {})
  {
    this->mqttHandler = mqttHandler;
    this->messageTriggeredActions = messageTriggeredActions;
    this->mqttHandler->onConnect([this](bool sessionPresent)
                                 { this->onConnect(sessionPresent); });
    this->mqttHandler->onMessage([this](char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
                                 { this->onMessage(topic, payload, properties, len, index, total); });
  }

  virtual void onConnect(bool sessionPresent)
  {
    Serial.println("Connected to MQTT.");
    if (messageTriggeredActions.size() > 0)
    {
      for (auto const &x : messageTriggeredActions)
      {
        mqttHandler->subscribe(x.first);
      }
    }
  }

  virtual void onMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
  {
    const char *pl = "";
    if (len > 0)
    {
      std::string str;
      str.append(payload);
      pl = str.substr(0, len).c_str();
    }

    Serial.printf("Received message from MQTT topic %s with payload: %s\n", topic, pl);
    if (messageTriggeredActions.size() > 0)
    {
      for (auto const &x : messageTriggeredActions)
      {
        if (strcmp(x.first, topic) == 0)
        {
          x.second(pl);
        }
      }
    }
  }
};

// ------------------------------ TemperatureSensorCommunicationManager ------------------------------
struct TemperatureSensorCommunicationManager : CommunicationManager
{
  TemperatureSensorCommunicationManager(MqttHandler *mqttHandler, std::map<const char *, MessageTriggeredAction> messageTriggeredActions = {}) : CommunicationManager(mqttHandler, messageTriggeredActions)
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
  MessageTriggeredAction volumeChangeRequestCallback;
  MessageTriggeredAction stateChangeRequestCallback;

  SoundPlayerCommunicationManager(MqttHandler *mqttHandler) : TemperatureSensorCommunicationManager(
                                                                  mqttHandler,
                                                                  std::map<const char *, MessageTriggeredAction>{
                                                                      {MQTT_TOPIC_AUDIOPLAYER_CHANGE_STATE, [this](const char *payload)
                                                                       { this->stateChangeRequestCallback(payload); }},
                                                                      {MQTT_TOPIC_AUDIOPLAYER_CHANGE_VOL, [this](const char *payload)
                                                                       { this->volumeChangeRequestCallback(payload); }}}) {}
  void publishState(String payload)
  {
    mqttHandler->publishPayload(MQTT_TOPIC_AUDIOPLAYER_STATE_CHANGED, payload.c_str());
  }
  void onVolumeChangeRequest(MessageTriggeredAction callback)
  {
    volumeChangeRequestCallback = callback;
  }
  void onStateChangeRequest(MessageTriggeredAction callback)
  {
    stateChangeRequestCallback = callback;
  }
};

// ------------------------------ SprinklerCommunicationManager ------------------------------
struct SprinklerCommunicationManager : TemperatureSensorCommunicationManager
{
  MessageTriggeredAction waterRequestCallback;
  MessageTriggeredAction scheduledWaterRequestCallback;
  MessageTriggeredAction pictureRequestCallback;

  SprinklerCommunicationManager(MqttHandler *mqttHandler) : TemperatureSensorCommunicationManager(
                                                                mqttHandler,
                                                                std::map<const char *, MessageTriggeredAction>{
                                                                    {MQTT_TOPIC_SPRINKLER_SCHEDULED_WATER, [this](const char *payload)
                                                                     { this->scheduledWaterRequestCallback(payload); }},
                                                                    {MQTT_TOPIC_SPRINKLER_WATER, [this](const char *payload)
                                                                     { this->waterRequestCallback(payload); }},
                                                                    {MQTT_TOPIC_SPRINKLER_PICTURE, [this](const char *payload)
                                                                     { this->pictureRequestCallback(payload); }}}) {}
  void onScheduledWaterRequest(MessageTriggeredAction callback)
  {
    scheduledWaterRequestCallback = callback;
  }
  void onWaterRequest(MessageTriggeredAction onWaterTurnedOn, MessageTriggeredAction onWaterTurnedOff)
  {
    waterRequestCallback = onOffAction(onWaterTurnedOn, onWaterTurnedOff);
  }
  void onPictureRequest(MessageTriggeredAction callback)
  {
    pictureRequestCallback = callback;
  }
};
