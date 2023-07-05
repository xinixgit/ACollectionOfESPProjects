#include "AudioSource.h"
#include "SD.h"
#include "FS.h"
#include "Config.h"

void fetchAudios(File, std::map<String, list<String>> &audios, String genre = "");

SDAudioSource::SDAudioSource()
{
  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(spConfig.SD_CS, OUTPUT);
  digitalWrite(spConfig.SD_CS, HIGH);

  // Initialize SPI bus for microSD Card
  SPI.begin(spConfig.SPI_SCK, spConfig.SPI_MISO, spConfig.SPI_MOSI, spConfig.SD_CS);
  if (!SD.begin(spConfig.SD_CS))
  {
    Serial.println("Error accessing microSD card!");
    return;
  }

  Serial.println("SD ready.");
}

void SDAudioSource::populateAudioMenu(AudioMenu &menu)
{
  menu.audioMap.clear();
  File root = SD.open("/");
  fetchAudios(root, menu.audioMap);
  menu.selectedGenre = spConfig.defaultAudioGenre;
}

void fetchAudios(File dir, std::map<String, list<String>> &audios, String genre)
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
      String newGenre = genre;
      if (genre == "")
      {
        newGenre = entryName;
        audios[newGenre] = list<String>();
      }

      fetchAudios(entry, audios, newGenre);
      continue;
    }

    String currPath = String(dir.path()) + "/" + entryName;
    audios[genre].push_back(currPath);
    entry.close();
  }
}

void SDAudioSource::play(String path, Audio *audio)
{
  this->isRunning = true;
  Serial.printf("Now playing %s from SD.", path.c_str());
  audio->connecttoFS(SD, path.c_str());
}

void SDAudioSource::pause()
{
  this->isRunning = false;
}

void SDAudioSource::resume()
{
  this->isRunning = true;
}
