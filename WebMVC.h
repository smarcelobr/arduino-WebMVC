#ifndef WEBMVC_H
#define WEBMVC_H

#include <Arduino.h>
#include <UIPEthernet.h>
#include <UIPServer.h>
#include <UIPClient.h>
#include <ethernet_comp.h>

// HTTP METHOD Constants
#define METHOD_UNKNOWN 0
#define METHOD_GET 1
#define METHOD_POST 2

// http response status codes
#define RC_ERR 500
#define RC_BAD_REQ  400
#define RC_NOT_FOUND 404
#define RC_METHOD_NOT_ALLOWED 405
#define RC_OK 200

// HTTP Supported Content Types
const char CONTENT_TYPE_HTML[] PROGMEM="text/html";
const char CONTENT_TYPE_JSON[] PROGMEM="application/json";

class WebController; // forward declaration

typedef struct  {
  PGM_P resource_P;
  WebController *controller;
  PGM_P view_P;
} WebRoute;

typedef struct {
  uint16_t httpStatus;
  PGM_P contentType_P; /* CONTENT_TYPE_HTML,CONTENT_TYPE_JSON*/
} WebResponse;

typedef struct {
  EthernetClient client;
  uint8_t method; /* #define: METHOD_GET,METHOD_POST  */
  WebRoute route;
//  uint16_t ContentLength;
  WebResponse response;
} WebRequest;

class WebDispatcher {
private:
  EthernetServer server;
  uint8_t numRoutes;
  WebRoute *routes_P;
  
  uint8_t processRequest(WebRequest &request);
  void processClient(EthernetClient &client);

public:
  /* pode ser criado na funcao setup() ou, se necessário, dentro de outra funcao chamada no loop 
  para economizar memoria para outras funcoes, porem, gastando tempo de processando. */
  WebDispatcher(EthernetServer &_server);
  void setRoutes(WebRoute routes_PGM[], uint8_t numRoutes);

  /* process() - Precisa ser chamado na funcao loop() do arduino */
  void process();
  /* metodos para auxiliar o controller enviar a resposta */
  uint16_t getNextLine(EthernetClient &client, char dest[], uint16_t maxLen) const;
  void sendData_P(WebRequest &request, PGM_P msg_P) const;
  void sendHeader(WebRequest &request) const;
};

class WebController {
public:
  virtual void execute(WebDispatcher &webDispatcher, WebRequest &request) = 0;
};

class RedirectToViewCtrl: public WebController {
private:
	PGM_P contentType_P;
public:
	RedirectToViewCtrl(PGM_P contentType_PGM):contentType_P(contentType_PGM) {};
	void execute(WebDispatcher &webDispatcher, WebRequest &request) {
		request.response.httpStatus=request.method==METHOD_GET?RC_OK:RC_METHOD_NOT_ALLOWED;
		request.response.contentType_P=contentType_P;
		webDispatcher.sendHeader(request);
		if (request.method==METHOD_GET) webDispatcher.sendData_P(request,request.route.view_P);
	}
};

#endif // WEBMVC_H
