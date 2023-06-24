#ifndef WEBSTREAMER_H
#define WEBSTREAMER_H

#include <WebServer.h>
#include "ESPCamHandler.h"

namespace audp
{
  struct WebStreamer
  {
    WebServer *server;
    ESPCamHandler *camHandler;

    WebStreamer(ESPCamHandler *camHandler);
    void begin();
    void loop();
  };
}

#endif