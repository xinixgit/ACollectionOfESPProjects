#include "Audio.h"
#include "AudioPlayer.h"

#define I2S_DOUT  26
#define I2S_BCLK  14
#define I2S_LRC   27

Audio audio;

AudioPlayer::AudioPlayer() {
  // Connect MAX98357 I2S Amplifier Module
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // Set thevolume (0-21)
  audio.setVolume(9);
}

void AudioPlayer::play() {
  audio.connecttohost("http://192.168.0.181:32469/object/43503c5504258d9d833b/file.mp3");
}

void AudioPlayer::loop() {
  audio.loop();
}

void audio_info(const char *info) {
  Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info) { //id3 metadata
  Serial.print("id3data     "); Serial.println(info);
}
void audio_eof_stream(const char *lastHost) { //end of webstream
  Serial.print("eof_stream     ");
  Serial.println(lastHost);
  audio.connecttohost(lastHost);
}
void audio_showstation(const char *info) {
  Serial.print("station     "); Serial.println(info);
}
void audio_showstreaminfo(const char *info) {
  Serial.print("streaminfo  "); Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle "); Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     "); Serial.println(info);
}
void audio_commercial(const char *info) { //duration in sec
  Serial.print("commercial  "); Serial.println(info);
}
void audio_icyurl(const char *info) { //homepage
  Serial.print("icyurl      "); Serial.println(info);
}
void audio_lasthost(const char *info) { //stream URL played
  Serial.print("lasthost    "); Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  "); Serial.println(info);
}
