struct AudioPlayer
{
  AudioPlayer();

  void play();
  void loop();
  void onVolumeChangeRequested(const char *);
  void onStateChangeRequested(const char *);
};