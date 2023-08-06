#ifndef WEBSTREAMER_H
#define WEBSTREAMER_H

#include "ESPCamHandler.h"
#include <ESPAsyncWebServer.h>

namespace audp
{
  struct WebStreamer
  {
    void setCamHandler(ESPCamHandler *camHandler);
    void begin(AsyncWebServer &);
    void loop();
  };
}

#endif