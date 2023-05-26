#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include "Config.h"

typedef std::function<void(const char *)> OnAudioPlayerVolumeChangeRequest;
typedef std::function<void(const char *)> OnAudioPlayerStateChangeRequest;

struct MqttHandler
{
  MqttConfig *config;

  MqttHandler(MqttConfig *);
  void connect();
  void publishTemperature(String payload);
  void publishAudioPlayerState(String payload);
  void onAudioPlayerVolumeChangeRequest(OnAudioPlayerVolumeChangeRequest callback);
  void onAudioPlayerStateChangeRequest(OnAudioPlayerStateChangeRequest callback);
};

#endif