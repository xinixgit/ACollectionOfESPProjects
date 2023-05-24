#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include "Config.h"

typedef std::function<void(const char *)> OnAudioPlayerVolumeChange;
typedef std::function<void(const char *)> OnAudioPlayerStateChange;

struct MqttHandler
{
  MqttConfig *config;

  MqttHandler(MqttConfig *);
  void connect();
  void publishTemperatureSensorPayload(String payload);
  void onAudioPlayerVolumeChange(OnAudioPlayerVolumeChange callback);
  void onAudioPlayerStateChange(OnAudioPlayerStateChange callback);
};

#endif