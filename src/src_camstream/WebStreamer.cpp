#include "WebStreamer.h"

#define MSEC_PER_FRAME 100 // 10 fps

WebServer::THandlerFunction handleFn(OV2640 *, WebServer *);

audp::WebStreamer::WebStreamer(ESPCamHandler *camHandler)
{
  this->camHandler = camHandler;
  this->server = new WebServer(80);
  this->server->on("/jpg", HTTP_GET, handleFn(this->camHandler->getCam(), this->server));
}

WebServer::THandlerFunction handleFn(OV2640 *cam, WebServer *server)
{
  return [cam, server]()
  {
    WiFiClient thisClient = server->client();
    if (!thisClient.connected())
    {
      return;
    }
    cam->run();

    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-disposition: inline; filename=capture.jpg\r\n";
    response += "Content-type: image/jpeg\r\n\r\n";
    server->sendContent(response);
    thisClient.write((char *)cam->getfb(), cam->getSize());
  };
}

void audp::WebStreamer::begin()
{
  this->server->begin();
}

void audp::WebStreamer::loop()
{
  server->handleClient();
}