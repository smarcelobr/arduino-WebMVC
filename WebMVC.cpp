#include "WebMVC.h"

#define DEBUGGING

// Request parameters
const char GET[] PROGMEM = "GET ";
const char POST[] PROGMEM = "POST ";

// HEADERS FOR RESPONSES
const char HD_START[] PROGMEM = "HTTP/1.1 ";
const char HD_CONTENT_TYPE[] PROGMEM = " \nContent-Type: ";
const char HD_END[] PROGMEM = "\nConnection: close\n\n";

/* - 1 = The request is too big, please send one with less bytes or check
 * 		 the maxSize constant to increase the value.
 */
#define ERROR_REQUEST_TOO_BIG 1
/* - 2 = Only GET and POST is supported, but something else has been received.*/
#define ERROR_INVALID_METHOD 2

WebDispatcher::WebDispatcher(EthernetServer &_server) :
		server(&_server) {
}

void WebDispatcher::setRoutes(const WebRoute routes_PGM[], uint8_t numRoutes) {
	this->routes_P = routes_PGM;
	this->numRoutes = numRoutes;
}

/**
 * Reads the next line from the client.
 * Sample POST:
 * POST / HTTP/1.1
 * Host: 192.168.178.22
 * User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:33.0) Gecko/20100101 Firefox/33.0
 * Accept: text/html,application/xhtml+xml,application/xml;q=0.9,* /*;q=0.8
 * Accept-Language: de
 * Accept-Encoding: gzip, deflate
 * DNT: 1
 * Content-Type: text/xml; charset=UTF-8
 * Content-Length: 4
 * Connection: keep-alive
 * Pragma: no-cache
 * Cache-Control: no-cache
 *
 * {\"json object key\":123}
 * end -----------------------
 *
 * Sample GET:
 * GET /?1=0 HTTP/1.1
 * Host: 192.168.178.22
 * User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:33.0) Gecko/20100101 Firefox/33.0
 * Accept: text/html,application/xhtml+xml,application/xml;q=0.9,* /*;q=0.8
 * Accept-Language: de
 * Accept-Encoding: gzip, deflate
 * DNT: 1
 * Connection: keep-alive
 * end -----------------------
 */
uint16_t WebDispatcher::getNextLine(EthernetClient &client, char dest[],
		uint16_t maxLen) const {
	char c;
	uint16_t len = 0;
	while (client.connected() && client.available()) {
		c = client.read();
		if (c == '\n') {
			// sai no fim da linha
			break;
		}
		// ignore all \r (CR) characters
		// descarta todos os caracteres da linha apos o maxLen
		if ((c != '\r') && (len < maxLen - 1)) {
			dest[len++] = c;
		} // end c != CR
	} // end while
	dest[len] = 0x00;
	return len;
}

/*
 * processRequest - preenche o request baseado nas informações proveniente da rede
 */
uint8_t WebDispatcher::processRequest(WebRequest &request) {
	char line[50];
	EthernetClient &client = request.client;

	request.method = METHOD_UNKNOWN;
	// pega o http method e a posicao inicial do resource identifier
	uint8_t resIdIndex;
	getNextLine(client, line, sizeof(line));
#ifdef DEBUGGING
	Serial.print(F("REQUEST:"));
	Serial.println(line);
#endif
	if (memcmp_P(line, GET, 4) == 0) {
		request.method = METHOD_GET;
		resIdIndex = 4;
	} else if (memcmp_P(line, POST, 5) == 0) {
		request.method = METHOD_POST;
		resIdIndex = 5;
	} else {
		request.response.httpStatus = RC_METHOD_NOT_ALLOWED;
		return 1; // todo (method not allowed) corrigir error code
	}

	// identifica o resource id e pega o webroute correspondente
#ifdef DEBUGGING
	Serial.println(F("ROTAS:"));
#endif
	bool achou = false;
	for (int i = 0; i < this->numRoutes; i++) {
		WebRoute route = { };
		memcpy_P(&route, &this->routes_P[i], sizeof(WebRoute));
		int len = strlen_P(route.resource_P);
		if (memcmp_P(&line[resIdIndex], route.resource_P, len) == 0) {
			request.route = route;
			achou = true;
		}
	}
	if (!achou) {
		request.response.httpStatus = RC_NOT_FOUND;
		return 1; // todo (resource not found) corrigir error code
	}

	// le os atributos do header que interessam e pula todos os cabecalhos (termina quando chega uma linha vazia)
	while (getNextLine(client, line, sizeof(line))) {
		//  o Content-Lenght interessa no momento?
	}

	// em tese, o 'client' estaria aqui posicionado nos dados no content (se houver)
	request.response.httpStatus = RC_OK; // 200 (OK) a nao ser que o controller diga algo diferente.
	return 0; // todo (sucesso) corrigir error code
}

/* processClient - Pega o request e faz o response */
void WebDispatcher::processClient(EthernetClient &client) {
#ifdef DEBUGGING
	Serial.println(F("\nCLIENT!"));
#endif
	WebRequest request;
	request.client = client;
	int err = processRequest(request);
	if (!err) {
#ifdef DEBUGGING
	Serial.println(F("CTRL()"));
#endif
		// com o request preenchido, chama o controller:
		request.route.controller->execute(*this, request);
	} else {
		request.response.contentType_P = CONTENT_TYPE_HTML;
		sendHeader(request);
	}
}

#ifdef DEBUGGING
  uint16_t d=0;
#endif
void WebDispatcher::process() {
	if (EthernetClient client = server->available()) {
#ifdef DEBUGGING
		Serial.print('c');
#endif
		if (client) {
			processClient(client);
			client.stop();
		} // end if client
	} // end if server available
#ifdef DEBUGGING
	else if (d++>40000) {d=0; Serial.print('w');}
#endif
}

void WebDispatcher::sendHeader(WebRequest &request) const {
#ifdef DEBUGGING
		Serial.println(F("-Header:\n"));
#endif
	sendData_P(request, HD_START);
	request.client.print(request.response.httpStatus);
	sendData_P(request, HD_CONTENT_TYPE);
	sendData_P(request, request.response.contentType_P);
	sendData_P(request, HD_END);
}

/**
 *
 * manda para a Ethernet uma string na memoria de programa.
 *
 */
void WebDispatcher::sendData_P(WebRequest &request, PGM_P msg_P) const {
	char myChar;
	while (myChar = pgm_read_byte(msg_P++)) {
		request.client.write(myChar);
#ifdef DEBUGGING
		Serial.write(myChar);
#endif
	}
}
