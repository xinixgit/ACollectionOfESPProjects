#include "AudioSource.h"
#include "SD.h"
#include "FS.h"

#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

void fetchPlaylist(File, std::list<String> *);

SDAudioSource::SDAudioSource()
{
  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Initialize SPI bus for microSD Card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
  if (!SD.begin(SD_CS))
  {
    Serial.println("Error accessing microSD card!");
    while (true)
      ;
  }
}

std::list<String> SDAudioSource::populatePlaylist()
{
  std::list<String> playlist;
  File root = SD.open("/");
  fetchPlaylist(root, &playlist);
  return playlist;
}

void SDAudioSource::play(String path, Audio *audio)
{
  audio->connecttoFS(SD, path.c_str());
}

void SDAudioSource::stop(Audio *audio)
{
  this->isRunning = false;
  audio->stopSong();
}

void SDAudioSource::resume(Audio *audio)
{
  this->isRunning = true;
}

void fetchPlaylist(File dir, std::list<String> *playlist)
{
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      break;
    }

    Serial.printf("Found file: %s\n", entry.name());
    if (entry.isDirectory())
    {
      fetchPlaylist(entry, playlist);
    }

    entry.close();
  }
}