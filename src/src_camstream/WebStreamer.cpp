#include <WebServer.h>
#include "WebStreamer.h"

#define MSEC_PER_FRAME 100 // 10 fps

WebServer::THandlerFunction handleFn(OV2640 *, WebServer *);

audp::WebStreamer::WebStreamer(ESPCamHandler *camHandler)
{
  this->camHandler = camHandler;
  this->server = new WebServer(80);
  this->server->on("/", HTTP_GET, handleFn(this->camHandler->getCam(), this->server));
}

WebServer::THandlerFunction handleFn(OV2640 *cam, WebServer *server)
{
  return [cam, server]()
  {
    WiFiClient thisClient = server->client();
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    server->sendContent(response);

    while (1)
    {
      cam->run();
      if (!thisClient.connected())
      {
        break;
      }
      response = "--frame\r\n";
      response += "Content-Type: image/jpeg\r\n\r\n";
      server->sendContent(response);

      thisClient.write((char *)cam->getfb(), cam->getSize());
      server->sendContent("\r\n");
      delay(100);
    }
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