#include "AudioSource.h"
#include "SoapESP32.h"

#define DLNA_PREFIX "http://192.168.0.181:8200/"

WiFiClient client;
WiFiUDP udp;
SoapESP32 soap(&client, &udp);

void discoverDlnaServer();
void fetchPlaylist(String objectId, std::list<String> *playlist);

std::list<String> DLNAAudioSource::populatePlaylist()
{
  std::list<String> playlist;
  discoverDlnaServer();
  fetchPlaylist("0", &playlist);
  return playlist;
}

void DLNAAudioSource::play(String path, Audio *audio)
{
  this->isRunning = true;
  String uri = DLNA_PREFIX + path;
  audio->connecttohost(uri.c_str());
}

void DLNAAudioSource::stop(Audio *audio)
{
  this->isRunning = false;
}

void DLNAAudioSource::resume(Audio *audio)
{
  this->isRunning = true;
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

void fetchPlaylist(String objectId, std::list<String> *playlist)
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
      playlist->push_back(object.uri);
    }
  }

  for (int j = 0; j < dirIds.size(); j++)
  {
    fetchPlaylist(dirIds[j], playlist);
  }
}