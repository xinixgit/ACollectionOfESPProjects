#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "Audio.h"

typedef std::function<void(String)> PublishState;

// keeps the logic of manipulating the "audio" lib to play audio
struct AudioPlayer
{
  void init();
  void start();
  void resume();
  void pause();
  void stop();
  void loop();
  void onVolumeChangeRequested(std::string);
  void onStateChangeRequested(std::string);
  void onGenreChangeRequested(std::string);
  void setPublishStateFn(PublishState);
};

#endif
