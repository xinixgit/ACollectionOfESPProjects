#include <ArduinoJson.h>
#include "Audio.h"
#include "AudioPlayer.h"
#include "AudioSource.h"
#include <esp_random.h>
#include <list>
#include "Config.h"

int volume = 10; // default volume

Audio audio;
AudioMenu audioMenu;
AudioSource *audioSource;
PublishState publishStateFn;

void playRandomSong();

void publishState(bool isRunning, int volume, const char *title = NULL, AudioMenu *audioMenu = NULL)
{
  if (publishStateFn != NULL)
  {
    DynamicJsonDocument doc(512);
    doc["is_on"] = isRunning;
    doc["volume"] = volume;
    doc["title"] = title;

    if (audioMenu != NULL)
    {
      JsonObject audioMenuObj = doc.createNestedObject("audio_menu");
      audioMenuObj["selected_genre"] = audioMenu->selectedGenre;

      JsonArray genres = audioMenuObj.createNestedArray("genres");
      for (auto genre : audioMenu->getGenres())
      {
        genres.add(genre);
      }
    }

    String payload;
    serializeJson(doc, payload);
    publishStateFn(payload);
    delay(500);
  }
}

void AudioPlayer::init()
{
  audioSource = new SDAudioSource();
  // Connect MAX98357 I2S Amplifier Module
  audio.setPinout(spConfig.I2S_BCLK, spConfig.I2S_LRC, spConfig.I2S_DOUT);
  // Set thevolume (0-21)
  audio.setVolume(volume);
}

// only meant for external callers, internal func should use `playRandomSong`
// since playlist should already be populated
void AudioPlayer::start()
{
  audioSource->populateAudioMenu(audioMenu);
  publishState(audioSource->isRunning, volume, NULL, &audioMenu);
}

void AudioPlayer::loop()
{
  audio.loop();
  if (!audio.isRunning())
  {
    delay(1000);
  }
}

void AudioPlayer::onVolumeChangeRequested(std::string payload)
{
  int vol = std::stoi(payload);
  volume = vol;
  audio.setVolume(vol);
  Serial.printf("Audio volume changed to %d\n", vol);
}

void AudioPlayer::onStateChangeRequested(std::string payload)
{
  if (strcmp(payload.c_str(), "off") == 0)
  {
    pause();
    return;
  }

  if (strcmp(payload.c_str(), "on") == 0)
  {
    resume();
    return;
  }

  Serial.printf("Unable to identify payload: %s\n", payload);
}

void AudioPlayer::onGenreChangeRequested(std::string payload)
{
  audio.stopSong();
  audioMenu.selectedGenre = String(payload.c_str());
  Serial.printf("Playlist refreshed with %d songs in genre %s.\n", audioMenu.getAudiosInSelectedGenre().size(), audioMenu.selectedGenre.c_str());
  playRandomSong();
}

void AudioPlayer::setPublishStateFn(PublishState fn)
{
  publishStateFn = fn;
}

void AudioPlayer::pause()
{
  if (audio.isRunning())
  {
    // stream cannot be stopped, so we simply set volume to 0 to
    // mute the music; the next song will not be played after this
    audio.setVolume(0);
    audio.pauseResume();
    audioSource->pause();
    Serial.println("Audio paused.");
  }
}

void AudioPlayer::resume()
{
  // here we check if audioSource is running instead of audio is running
  // since audioSource may simply be muted previously
  if (!audioSource->isRunning)
  {
    audio.setVolume(volume);
    if (!audio.isRunning())
    {
      audioSource->resume();
      playRandomSong();
    }
    Serial.println("Audio started.");
  }
}

void playRandomSong()
{
  auto playlist = audioMenu.getAudiosInSelectedGenre();
  if (playlist.size() == 0)
  {
    Serial.println("No song in the playlist.");
    return;
  }

  int idx = esp_random() % playlist.size();
  auto l_front = playlist.begin();
  std::advance(l_front, idx);

  String path = "" + *l_front;
  audioSource->play(path, &audio);
}

void audio_info(const char *info)
{
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info)
{ // id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);

  std::string data;
  data.append(info);
  if (data.rfind("Title") == 0)
  {
    std::string title = data.substr(7);
    publishState(audioSource->isRunning, volume, title.c_str());
    delay(500);
  }
}
void audio_eof_stream(const char *lastHost)
{ // end of stream
  Serial.print("eof_stream     ");
  Serial.println(lastHost);

  if (audioSource->isRunning)
  {
    playRandomSong();
  }
}
void audio_eof_mp3(const char *info)
{ // end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
  if (audioSource->isRunning)
  {
    playRandomSong();
  }
}
void audio_showstation(const char *info)
{
  Serial.print("station     ");
  Serial.println(info);
}
void audio_showstreaminfo(const char *info)
{
  Serial.print("streaminfo  ");
  Serial.println(info);
}
void audio_showstreamtitle(const char *info)
{
  Serial.print("streamtitle ");
  Serial.println(info);
}
void audio_bitrate(const char *info)
{
  Serial.print("bitrate     ");
  Serial.println(info);
}
void audio_commercial(const char *info)
{ // duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info)
{ // homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}
void audio_lasthost(const char *info)
{ // stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info)
{
  Serial.print("eof_speech  ");
  Serial.println(info);
}
