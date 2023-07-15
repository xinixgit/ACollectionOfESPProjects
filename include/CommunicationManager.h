#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <string>
#include <map>
#include "MqttHandler.h"
#include "Config.h"
#include <vector>

using namespace std;

typedef std::function<void(string)> MessageTriggeredActionFn;

struct MessageTriggeredAction
{
  string topic;
  MessageTriggeredActionFn fn;
  uint8_t qos = 0;
  MessageTriggeredAction(string topic, MessageTriggeredActionFn fn, uint8_t qos = 0)
  {
    this->topic = topic;
    this->fn = fn;
    this->qos = qos;
  }
};

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
        mqttHandler->subscribe(action.topic.c_str(), action.qos);
      }
    }
  }

  virtual void onMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
  {
    std::string payloadStr;
    if (len > 0)
    {
      payloadStr.append(payload);
      payloadStr = payloadStr.substr(0, len);
    }

    Serial.printf("Received message from MQTT topic %s with payload: %s\n", topic, payloadStr.c_str());
    if (messageTriggeredActions.size() > 0)
    {
      for (auto action : messageTriggeredActions)
      {
        if (strcmp(action.topic.c_str(), topic) == 0)
        {
          action.fn(payloadStr);
        }
      }
    }
  }
};

// ------------------------------ TemperatureSensorCommunicationManager ------------------------------
struct TempSensorCommunicationManager : CommunicationManager
{
  String topic;
  TempSensorCommunicationManager(
      MqttHandler *mqttHandler,
      String topic = MQTT_TOPIC_SENSOR_TEMPERATURE,
      std::vector<MessageTriggeredAction> messageTriggeredActions = {}) : CommunicationManager(mqttHandler, messageTriggeredActions) {}
  void publishTemperature(String payload)
  {
    mqttHandler->publishPayload(topic.c_str(), payload.c_str());
  }
};

struct AirQualitySensorCommunicationManager : CommunicationManager
{
  String topic;
  AirQualitySensorCommunicationManager(
      MqttHandler *mqttHandler,
      String topic = MQTT_TOPIC_SENSOR_AQI,
      std::vector<MessageTriggeredAction> messageTriggeredActions = {}) : CommunicationManager(mqttHandler, messageTriggeredActions) {}
  void publishAirQuality(String payload)
  {
    mqttHandler->publishPayload(topic.c_str(), payload.c_str());
  }
};

// ------------------------------ SoundPlayerCommunicationManager ------------------------------
struct SoundPlayerCommunicationManager : TempSensorCommunicationManager
{
  MessageTriggeredActionFn volumeChangeRequestCallback;
  MessageTriggeredActionFn stateChangeRequestCallback;
  MessageTriggeredActionFn genreChangeRequestCallback;

  SoundPlayerCommunicationManager(MqttHandler *mqttHandler, String topic) : TempSensorCommunicationManager(mqttHandler, topic)
  {
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            spConfig.MqttTopicChangeState,
            [this](string payload)
            { this->stateChangeRequestCallback(payload); },
            1));
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            spConfig.MqttTopicChangeVol,
            [this](string payload)
            { this->volumeChangeRequestCallback(payload); },
            1));
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            spConfig.MqttTopicChangeGenre,
            [this](string payload)
            { this->genreChangeRequestCallback(payload); },
            1));
  }
  void publishState(String payload)
  {
    mqttHandler->publishPayload(spConfig.MqttTopicStateChanged, payload.c_str());
  }
  void onVolumeChangeRequest(MessageTriggeredActionFn callback)
  {
    volumeChangeRequestCallback = callback;
  }
  void onStateChangeRequest(MessageTriggeredActionFn callback)
  {
    stateChangeRequestCallback = callback;
  }
  void onGenreChangeRequest(MessageTriggeredActionFn callback)
  {
    genreChangeRequestCallback = callback;
  }
};

// ------------------------------ SprinklerCommunicationManager ------------------------------
struct SprinklerCommunicationManager : TempSensorCommunicationManager
{
  MessageTriggeredActionFn waterRequestCallback;
  MessageTriggeredActionFn fanRequestCallback;
  SprinklerConfig *config;

  SprinklerCommunicationManager(MqttHandler *mqttHandler, String topic) : TempSensorCommunicationManager(mqttHandler, topic)
  {
    config = new SprinklerConfig();
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            config->MqttTopicWater,
            [this](string payload)
            { this->waterRequestCallback(payload); },
            1));
    messageTriggeredActions.push_back(
        MessageTriggeredAction(
            config->MqttTopicFan,
            [this](string payload)
            { this->fanRequestCallback(payload); },
            1));
  }
  void onWaterRequest(MessageTriggeredActionFn callback)
  {
    waterRequestCallback = callback;
  }
  void onFanRequest(MessageTriggeredActionFn callback)
  {
    fanRequestCallback = callback;
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