#include <WebServer.h>
#include "ESPCamHandler.h"

namespace audp
{
  struct WebStreamer
  {
    // WiFiServer *rtspServer;
    // CStreamer *streamer;
    // WiFiClient rtspClient;

    WebServer *server;
    ESPCamHandler *camHandler;

    WebStreamer(ESPCamHandler *camHandler);
    void begin();
    void loop();
  };
}