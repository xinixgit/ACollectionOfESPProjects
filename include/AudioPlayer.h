#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "Audio.h"

typedef std::function<void(String)> PublishState;

// keeps the logic of manipulating the "audio" lib to play audio
struct AudioPlayer
{
  AudioPlayer();
  void play();
  void loop();
  void onVolumeChangeRequested(const char *);
  void onStateChangeRequested(const char *);
  void setPublishStateFn(PublishState);
};

#endif
