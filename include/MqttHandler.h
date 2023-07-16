#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <AsyncMqttClient.h>
#include "Config.h"

typedef std::function<void(char *, char *, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> OnMessageCallback;
typedef std::function<void(bool)> OnConnectCallback;

struct MqttHandler
{
  MqttConfig *config;

  MqttHandler(MqttConfig *);
  void connect();
  void disconnect();
  void subscribe(const char *topic, uint8_t qos = 0);
  void publishPayload(String topic, String payload, bool retain = false);
  void onMessage(OnMessageCallback);
  void onConnect(OnConnectCallback);
};

#endif