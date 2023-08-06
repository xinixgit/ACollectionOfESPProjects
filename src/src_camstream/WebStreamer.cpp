#include "WebStreamer.h"

ESPCamHandler *_camHandler;

void audp::WebStreamer::setCamHandler(ESPCamHandler *camHandler)
{
  _camHandler = camHandler;
}

void onPictureRequest(AsyncWebServerRequest *request)
{
  OV2640 *cam = _camHandler->getCam();
  cam->run();
  request->send_P(200, "image/jpeg", cam->getfb(), cam->getSize());
}

void audp::WebStreamer::begin(AsyncWebServer &_server)
{
  _server.on("/jpg", HTTP_GET, onPictureRequest);
}
