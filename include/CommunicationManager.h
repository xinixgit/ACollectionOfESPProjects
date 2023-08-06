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
  MqttHandler *_mqttHandler;

  void init(MqttHandler *mqttHandler, std::vector<MessageTriggeredAction> messageTriggeredActions = {})
  {
    _mqttHandler = mqttHandler;
    _mqttHandler->onConnect([this, messageTriggeredActions](bool sessionPresent)
                            { this->onConnect(sessionPresent, messageTriggeredActions); });
    _mqttHandler->onMessage([this, messageTriggeredActions](char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
                            { this->onMessage(topic, payload, properties, len, index, total, messageTriggeredActions); });
  }

  virtual void onConnect(bool sessionPresent, std::vector<MessageTriggeredAction> messageTriggeredActions)
  {
    Serial.println("Connected to MQTT.");
    if (messageTriggeredActions.size() > 0)
    {
      for (auto action : messageTriggeredActions)
      {
        _mqttHandler->subscribe(action.topic.c_str(), action.qos);
      }
    }
  }

  virtual void onMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total, std::vector<MessageTriggeredAction> messageTriggeredActions)
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
  String _topic;

  void init(MqttHandler *mqttHandler, String topic = MQTT_TOPIC_SENSOR_TEMPERATURE, std::vector<MessageTriggeredAction> actions = {})
  {
    _topic = topic;
    CommunicationManager::init(mqttHandler, actions);
  }

  void publishTemperature(String payload)
  {
    _mqttHandler->publishPayload(_topic, payload);
  }
};

struct AirQualitySensorCommunicationManager : CommunicationManager
{
  String _topic;

  void init(
      MqttHandler *mqttHandler,
      String topic = MQTT_TOPIC_SENSOR_AQI,
      std::vector<MessageTriggeredAction> messageTriggeredActions = {})
  {
    _topic = topic;
    CommunicationManager::init(mqttHandler, messageTriggeredActions);
  }

  void publishAirQuality(String payload)
  {
    _mqttHandler->publishPayload(_topic, payload);
  }
};

// ------------------------------ SoundPlayerCommunicationManager ------------------------------
struct SoundPlayerCommunicationManager : TempSensorCommunicationManager
{
  void init(
      MqttHandler *mqttHandler,
      String topic,
      MessageTriggeredActionFn volumeChangeRequestCallback,
      MessageTriggeredActionFn stateChangeRequestCallback,
      MessageTriggeredActionFn genreChangeRequestCallback)
  {
    std::vector<MessageTriggeredAction> actions{
        MessageTriggeredAction(spConfig.MqttTopicChangeState, stateChangeRequestCallback, 1),
        MessageTriggeredAction(spConfig.MqttTopicChangeVol, volumeChangeRequestCallback, 1),
        MessageTriggeredAction(spConfig.MqttTopicChangeGenre, genreChangeRequestCallback, 1)};

    TempSensorCommunicationManager::init(mqttHandler, topic, actions);
  }

  void publishState(String payload)
  {
    _mqttHandler->publishPayload(spConfig.MqttTopicStateChanged, payload);
  }
};

// ------------------------------ SprinklerCommunicationManager ------------------------------
struct SprinklerCommunicationManager : TempSensorCommunicationManager
{
  SprinklerConfig config;

  void init(MqttHandler *mqttHandler,
            String topic,
            MessageTriggeredActionFn waterRequestCallback,
            MessageTriggeredActionFn fanRequestCallback)
  {
    std::vector<MessageTriggeredAction> actions{
        MessageTriggeredAction(config.MqttTopicWater, waterRequestCallback, 1),
        MessageTriggeredAction(config.MqttTopicFan, fanRequestCallback, 1)};

    TempSensorCommunicationManager::init(mqttHandler, topic, actions);
  }
  void publishState(String item, String state)
  {
    DynamicJsonDocument doc(64);
    doc["item"] = item.c_str();
    doc["state"] = state.c_str();
    String payload;
    serializeJson(doc, payload);

    _mqttHandler->publishPayload(config.MqttTopicStateChanged, payload);
  }
};

struct CamStreamCommunicationManager : TempSensorCommunicationManager
{
  void init(MqttHandler *mqttHandler,
            CamStreamConfig &config,
            MessageTriggeredActionFn restartRequestCallack,
            MessageTriggeredActionFn pictureRequestCallack)
  {
    std::vector<MessageTriggeredAction> actions{
        MessageTriggeredAction(config.MqttTopicRestart, restartRequestCallack),
        MessageTriggeredAction(config.MqttTopicPicture, pictureRequestCallack),
    };

    TempSensorCommunicationManager::init(
        mqttHandler,
        config.MqttTopicSensorTemperature,
        actions);
  }
};

#endif