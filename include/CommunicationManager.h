#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <string>
#include <map>
#include "MqttHandler.h"
#include "Config.h"
#include <vector>

typedef std::function<void(const char *)> MessageTriggeredActionFn;

struct MessageTriggeredAction
{
  const char *topic;
  MessageTriggeredActionFn fn;
  uint8_t qos = 0;
  MessageTriggeredAction(const char *topic, MessageTriggeredActionFn fn, uint8_t qos = 0)
  {
    this->topic = topic;
    this->fn = fn;
    this->qos = qos;
  }
};

MessageTriggeredActionFn onOffAction(MessageTriggeredActionFn onAction, MessageTriggeredActionFn offAction)
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
  std::vector<MessageTriggeredAction> messageTriggeredActions;

  CommunicationManager(MqttHandler *mqttHandler, std::vector<MessageTriggeredAction> messageTriggeredActions = {})
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
      for (auto action : messageTriggeredActions)
      {
        mqttHandler->subscribe(action.topic, action.qos);
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
      for (auto action : messageTriggeredActions)
      {
        if (strcmp(action.topic, topic) == 0)
        {
          action.fn(pl);
        }
      }
    }
  }
};

// ------------------------------ TemperatureSensorCommunicationManager ------------------------------
struct TemperatureSensorCommunicationManager : CommunicationManager
{
  TemperatureSensorCommunicationManager(MqttHandler *mqttHandler, std::vector<MessageTriggeredAction> messageTriggeredActions = {}) : CommunicationManager(mqttHandler, messageTriggeredActions)
  {
  }

  void publishTemperature(String payload, const char *topic = MQTT_TOPIC_SENSOR_TEMPERATURE)
  {
    mqttHandler->publishPayload(topic, payload.c_str());
  }
  void publishAQI(String payload, const char *topic = MQTT_TOPIC_SENSOR_AQI)
  {
    mqttHandler->publishPayload(topic, payload.c_str());
  }
};

// ------------------------------ SoundPlayerCommunicationManager ------------------------------
struct SoundPlayerCommunicationManager : TemperatureSensorCommunicationManager
{
  MessageTriggeredActionFn volumeChangeRequestCallback;
  MessageTriggeredActionFn stateChangeRequestCallback;

  SoundPlayerCommunicationManager(MqttHandler *mqttHandler) : TemperatureSensorCommunicationManager(mqttHandler)
  {
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            MQTT_TOPIC_AUDIOPLAYER_CHANGE_STATE,
            [this](const char *payload)
            { this->stateChangeRequestCallback(payload); },
            1));
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            MQTT_TOPIC_AUDIOPLAYER_CHANGE_VOL,
            [this](const char *payload)
            { this->volumeChangeRequestCallback(payload); },
            1));
  }
  void publishState(String payload)
  {
    mqttHandler->publishPayload(MQTT_TOPIC_AUDIOPLAYER_STATE_CHANGED, payload.c_str());
  }
  void onVolumeChangeRequest(MessageTriggeredActionFn callback)
  {
    volumeChangeRequestCallback = callback;
  }
  void onStateChangeRequest(MessageTriggeredActionFn callback)
  {
    stateChangeRequestCallback = callback;
  }
};

// ------------------------------ SprinklerCommunicationManager ------------------------------
struct SprinklerCommunicationManager : TemperatureSensorCommunicationManager
{
  MessageTriggeredActionFn waterRequestCallback;
  SprinkerConfig *config;

  SprinklerCommunicationManager(MqttHandler *mqttHandler) : TemperatureSensorCommunicationManager(mqttHandler)
  {
    config = new SprinkerConfig();
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            config->MqttTopicWater,
            [this](const char *payload)
            { this->waterRequestCallback(payload); },
            1));
  }
  void onWaterRequest(MessageTriggeredActionFn callback)
  {
    waterRequestCallback = callback;
  }
  void publishState(const char *item, const char *state)
  {
    DynamicJsonDocument doc(64);
    doc["item"] = item;
    doc["state"] = state;
    String payload;
    serializeJson(doc, payload);

    mqttHandler->publishPayload(config->MqttTopicStateChanged, payload.c_str());
  }
};

#endif