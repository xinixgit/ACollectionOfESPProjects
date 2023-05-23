#include "Config.h"

struct MqttHandler
{
  MqttConfig *config;

  MqttHandler(MqttConfig *);
  void connect();
  void publishPayload(String payload);
};