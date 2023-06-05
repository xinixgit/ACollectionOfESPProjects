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
    return;
  }

  Serial.println("SD ready.");
}

std::list<String> SDAudioSource::populatePlaylist()
{
  std::list<String> playlist;
  File root = SD.open("/");
  fetchPlaylist(root, &playlist);
  return playlist;
}

void fetchPlaylist(File dir, std::list<String> *playlist)
{
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      // no more file to read
      break;
    }

    String entryName = String(entry.name());
    if (entryName.startsWith("."))
    {
      // Meta data file used by Apple
      continue;
    }

    if (entry.isDirectory())
    {
      fetchPlaylist(entry, playlist);
      continue;
    }

    String currPath = String(dir.path()) + entryName;
    playlist->push_back(currPath);
    entry.close();
  }
}

void SDAudioSource::play(String path, Audio *audio)
{
  this->isRunning = true;
  Serial.printf("Now playing %s from SD.", path.c_str());
  audio->connecttoFS(SD, path.c_str());
}

void SDAudioSource::stop(Audio *audio)
{
  if (audio->isRunning())
  {
    this->isRunning = false;
    audio->pauseResume();
  }
}

void SDAudioSource::resume(Audio *audio)
{
  if (!audio->isRunning())
  {
    this->isRunning = true;
    audio->pauseResume();
  }
}
