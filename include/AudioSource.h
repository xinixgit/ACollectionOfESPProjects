#include <Arduino.h>
#include "AudioPlayer.h"
#include "Audio.h"
#include <list>

// define the difference sources of audio data, and keep a state
// of how the data is read
struct AudioSource
{
  bool isRunning = false;
  virtual std::list<String> populatePlaylist();
  virtual void play(String, Audio *);
  virtual void stop(Audio *);
  virtual void resume(Audio *);
};

struct DLNAAudioSource : AudioSource
{
  std::list<String> populatePlaylist();
  void play(String, Audio *);
  void stop(Audio *);
  void resume(Audio *);
};