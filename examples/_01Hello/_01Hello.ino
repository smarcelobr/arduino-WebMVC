#include <Arduino.h>
#include <SPI.h>
#include <UIPEthernet.h>
// The connection_data struct needs to be defined in an external file.
#include <UIPServer.h>
#include <UIPClient.h>
#include <WebMVC.h>
/**
 * Hardware:
 * 1 Arduino Nano (or compatible)
 * 1 ENC28J60 LAN Ethernet Network Board Module
 * 
 * Pins Connection:
 * Arduino        Ethernet Module
 *  pin              pin
 *  D13 (SCK)  ----  SCK (Atenção: não confunda com CLK!)
 *  D12 (MISO) ----  SO
 *  D11 (MOSI) ----  ST (SI)
 *  D10 (SS)   ----  CS
 *  5V         ----  5V
 *  GND        ----  GND
 */

const char RESOURCE_HOME_PAGE[] PROGMEM = "/ ";

const char VIEW_HOME_PAGE[] PROGMEM = 
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
 "<meta charset=\"utf-8\"/>\n"
 "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"/>\n"
 "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/>\n"
 "<title>Hello World</title>\n"
"</head>\n"
"<body>\n"
"<div class=\"content\">\n"
  "<h1>Hello World!</h1>\n"
  "<p>Welcome to the web world.</p>\n"
"</div>\n"
"</body>\n"
"</html>\n";

RedirectToViewCtrl redirectToHTMLCtrl(CONTENT_TYPE_HTML);

#define NUM_ROUTES 1
const WebRoute routes[ NUM_ROUTES ] PROGMEM = {
  {RESOURCE_HOME_PAGE,&redirectToHTMLCtrl,VIEW_HOME_PAGE}
};

// start the server on port 80
EthernetServer server = EthernetServer(80);

void setup() {
  uint8_t mac[6];

  Serial.begin(9600);
  
  mac[0]=0x22; // Warning: the last two bits of first mac octet must be '10'. See https://en.wikipedia.org/wiki/MAC_address
  mac[1]=0x00;mac[2]=0x00;mac[3]=0xAD;mac[4]=0x00;mac[5]=0x01;
  
  IPAddress myIP(192,168,1,22);
  
  Ethernet.begin(mac, myIP);

  Serial.print(F("Local IP: "));
  Serial.println(Ethernet.localIP());
  Serial.print(F("     MAC: "));
  for (int i=0;i<6;i++) {
    if (i>0) Serial.print(F(":"));
    Serial.print(mac[i]);
  }
  Serial.println();

  server.begin();
}

void loop() {
  processWebRequests();
}

void processWebRequests() {
  WebDispatcher webDispatcher(server);
  webDispatcher.setRoutes(routes,NUM_ROUTES);
  webDispatcher.process();
} 


