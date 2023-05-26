#include <ArduinoJson.h>
#include "Audio.h"
#include "AudioPlayer.h"
#include "AudioSource.h"
#include <esp_random.h>
#include <list>

#define I2S_DOUT 26
#define I2S_BCLK 14
#define I2S_LRC 27

int volume = 10; // start with 10
std::list<String> playlist;

Audio audio;
AudioSource *audioSource;
PublishState publishStateFn;

void playRandomSong();

void publishState(bool isRunning, int volume, const char *title = NULL)
{
  if (publishStateFn != NULL)
  {
    DynamicJsonDocument doc(64);
    doc["is_on"] = isRunning;
    doc["volume"] = volume;
    doc["title"] = title;

    String payload;
    serializeJson(doc, payload);
    publishStateFn(payload);
  }
}

AudioPlayer::AudioPlayer()
{
  audioSource = new DLNAAudioSource();
  // Connect MAX98357 I2S Amplifier Module
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // Set thevolume (0-21)
  audio.setVolume(volume);
}

// only meant for external callers, internal func should use `playRandomSong`
// since playlist should already be populated
void AudioPlayer::play()
{
  playlist.clear();
  playlist = audioSource->populatePlaylist();
  playRandomSong();
  publishState(audioSource->isRunning, volume);
}

void AudioPlayer::loop()
{
  audio.loop();
  if (!audio.isRunning())
  {
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
  }
}

void AudioPlayer::onVolumeChangeRequested(const char *payload)
{
  int vol = std::stoi(payload);
  volume = vol;
  audio.setVolume(vol);
  Serial.printf("Audio volume changed to %d\n", vol);
}

void AudioPlayer::onStateChangeRequested(const char *payload)
{
  if (strcmp(payload, "off") == 0)
  {
    if (audio.isRunning())
    {
      // stream cannot be stopped, so we simply set volume to 0 to
      // mute the music; the next song will not be played after this
      audio.setVolume(0);
      audioSource->stop(&audio);
      Serial.println("Audio stopped.");
    }
    return;
  }

  if (strcmp(payload, "on") == 0)
  {
    // here we check if audioSource is running instead of audio is running
    // since audioSource may simply be muted previously
    if (!audioSource->isRunning)
    {
      audio.setVolume(volume);
      if (!audio.isRunning())
      {
        playRandomSong();
      }
      Serial.println("Audio started.");
    }
    return;
  }

  Serial.printf("Unable to identify payload: %s\n", payload);
}

void AudioPlayer::setPublishStateFn(PublishState fn)
{
  publishStateFn = fn;
}

void playRandomSong()
{
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
  }
}
void audio_eof_stream(const char *lastHost)
{ // end of webstream
  Serial.print("eof_stream     ");
  Serial.println(lastHost);

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
