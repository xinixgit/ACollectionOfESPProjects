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
  String topic;
  MessageTriggeredActionFn fn;
  uint8_t qos = 0;
  MessageTriggeredAction(String topic, MessageTriggeredActionFn fn, uint8_t qos = 0)
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

  CommunicationManager(MqttHandler *mqttHandler)
  {
    this->mqttHandler = mqttHandler;
  }

  virtual void init(std::vector<MessageTriggeredAction> messageTriggeredActions = {})
  {
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
      String topic = MQTT_TOPIC_SENSOR_TEMPERATURE) : CommunicationManager(mqttHandler)
  {
    this->topic = topic;
  }

  void publishTemperature(String payload)
  {
    mqttHandler->publishPayload(topic, payload);
  }
};

struct AirQualitySensorCommunicationManager : CommunicationManager
{
  String topic;
  AirQualitySensorCommunicationManager(
      MqttHandler *mqttHandler,
      String topic = MQTT_TOPIC_SENSOR_AQI,
      std::vector<MessageTriggeredAction> messageTriggeredActions = {}) : CommunicationManager(mqttHandler)
  {
    this->topic = topic;
    init(messageTriggeredActions);
  }

  void publishAirQuality(String payload)
  {
    mqttHandler->publishPayload(topic, payload);
  }
};

// ------------------------------ SoundPlayerCommunicationManager ------------------------------
struct SoundPlayerCommunicationManager : TempSensorCommunicationManager
{
  SoundPlayerCommunicationManager(
      MqttHandler *mqttHandler,
      String topic,
      MessageTriggeredActionFn volumeChangeRequestCallback,
      MessageTriggeredActionFn stateChangeRequestCallback,
      MessageTriggeredActionFn genreChangeRequestCallback) : TempSensorCommunicationManager(mqttHandler, topic)
  {
    std::vector<MessageTriggeredAction> actions{
        MessageTriggeredAction(spConfig.MqttTopicChangeState, stateChangeRequestCallback, 1),
        MessageTriggeredAction(spConfig.MqttTopicChangeVol, volumeChangeRequestCallback, 1),
        MessageTriggeredAction(spConfig.MqttTopicChangeGenre, genreChangeRequestCallback, 1)};

    init(actions);
  }
  void publishState(String payload)
  {
    mqttHandler->publishPayload(spConfig.MqttTopicStateChanged, payload);
  }
};

// ------------------------------ SprinklerCommunicationManager ------------------------------
struct SprinklerCommunicationManager : TempSensorCommunicationManager
{
  SprinklerConfig *config;

  SprinklerCommunicationManager(
      MqttHandler *mqttHandler,
      String topic,
      MessageTriggeredActionFn waterRequestCallback,
      MessageTriggeredActionFn fanRequestCallback) : TempSensorCommunicationManager(mqttHandler)
  {
    config = new SprinklerConfig();
    std::vector<MessageTriggeredAction> actions{
        MessageTriggeredAction(config->MqttTopicWater, waterRequestCallback, 1),
        MessageTriggeredAction(config->MqttTopicFan, fanRequestCallback, 1)};
    init(actions);
  }
  void publishState(String item, String state)
  {
    DynamicJsonDocument doc(64);
    doc["item"] = item.c_str();
    doc["state"] = state.c_str();
    String payload;
    serializeJson(doc, payload);

    mqttHandler->publishPayload(config->MqttTopicStateChanged, payload);
  }
};

#endif