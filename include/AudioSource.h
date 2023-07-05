#include <Arduino.h>
#include "AudioPlayer.h"
#include "Audio.h"
#include <list>
#include <map>

using namespace std;

struct AudioMenu
{
  String selectedGenre = "";
  std::map<String, list<String>> audioMap;

  list<String> getGenres()
  {
    list<String> genres;
    for (auto it = audioMap.begin(); it != audioMap.end(); it++)
    {
      genres.push_back(it->first);
    }
    return genres;
  }

  list<String> &getAudiosInSelectedGenre()
  {
    return audioMap[selectedGenre];
  }
};

// define the difference sources of audio data, and keep a state
// of how the data is read
struct AudioSource
{
  bool isRunning = false;

  virtual void play(String, Audio *){};
  virtual void pause(){};
  virtual void resume(){};
  virtual void populateAudioMenu(AudioMenu &){};
};

struct DLNAAudioSource : AudioSource
{
  void play(String, Audio *);
  void pause();
  void resume();
  void populateAudioMenu(AudioMenu &);
};

struct SDAudioSource : AudioSource
{
  SDAudioSource();
  void play(String, Audio *);
  void pause();
  void resume();
  void populateAudioMenu(AudioMenu &);
};