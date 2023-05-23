#include "Audio.h"
#include "AudioPlayer.h"
#include "SoapESP32.h"
#include <list>

#define I2S_DOUT 26
#define I2S_BCLK 14
#define I2S_LRC 27

#define DLNA_PREFIX "http://192.168.0.181:8200/"

Audio audio;
std::list<String> playlist;
WiFiClient client;
WiFiUDP udp;
SoapESP32 soap(&client, &udp);

void discoverDlnaServer();
void fetchPlaylist(String);
void playRandomSong();

AudioPlayer::AudioPlayer()
{
  // Connect MAX98357 I2S Amplifier Module
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // Set thevolume (0-21)
  audio.setVolume(9);

  discoverDlnaServer();
  fetchPlaylist("0");
  playlist.sort();
  playlist.unique();
}

void AudioPlayer::play()
{
  playRandomSong();
}

void AudioPlayer::loop()
{
  audio.loop();
}

void playRandomSong()
{
  int idx = rand() % playlist.size();
  auto l_front = playlist.begin();
  std::advance(l_front, idx);

  String uri = DLNA_PREFIX + *l_front;
  audio.connecttohost(uri.c_str());
}

void discoverDlnaServer()
{
  uint8_t numServers = 0;
  while (numServers == 0)
  {
    soap.seekServer();
    numServers = soap.getServerCount();
    Serial.print("Found dlna server count: ");
    Serial.println(numServers);
  }

  soapServer_t srv;
  soap.getServerInfo(0, &srv);
  Serial.printf("Server[%d]: IP address: %s port: %d name: %s -> controlURL: %s\n",
                0, srv.ip.toString().c_str(), srv.port, srv.friendlyName.c_str(), srv.controlURL.c_str());
}

void fetchPlaylist(String objectId)
{
  std::vector<String> dirIds{};
  soapObjectVect_t browseResult;

  soap.browseServer(0, objectId.c_str(), &browseResult);

  for (int i = 0; i < browseResult.size(); i++)
  {
    soapObject_t object = browseResult[i];
    if (object.isDirectory)
    {
      dirIds.push_back(object.id);
    }
    else
    {
      playlist.push_back(object.uri);
    }
  }

  for (int j = 0; j < dirIds.size(); j++)
  {
    fetchPlaylist(dirIds[j]);
  }
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
}
void audio_eof_stream(const char *lastHost)
{ // end of webstream
  Serial.print("eof_stream     ");
  Serial.println(lastHost);
  playRandomSong();
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
