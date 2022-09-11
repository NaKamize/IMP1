/**
 * @file main.cpp
 * @author Jozef Makiš (xmakis00@stud.fit.vutbr.cz)
 * @date 2021-12-18
 * 
 */
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <FS.h>
#include <SPIFFS_ImageReader.h>

#define TFT_CS 5
#define TFT_RST 2
#define TFT_DC 4

// udaje pre pripojenie k wifi
const char *ssid = "DANOOOOO";
const char *password = "87654321";
char *inputText = "input";
// stranka serveru
const char page[] = "<!DOCTYPE HTML><html><head> <title>ESP Input Form</title></head><body><form action=\"/get\"> Text: <input type=\"text\" name=\"input\"><input type=\"submit\" value=\"Submit\"></form> <form method=\"post\" enctype=\"multipart/form-data\" action=\"/upload\"> Image: <input type=\"file\" name=\"image\"><input type=\"submit\" value=\"Submit image\"></form></body></html>";

// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
AsyncWebServer server(80);
SPIFFS_ImageReader reader;

/**
 * @brief Funckia vykresluje text na diplej.
 * 
 * @param text retazec na vykreslenie 
 * @param color farba retazca
 */
void drawtext(char *text, uint16_t color)
{
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void setup(void)
{
  Serial.begin(115200);
  Serial.print(F("Starting initialization ... "));
  // Inicializacia SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println(F("SPIFFS initialised."));
  // inicializacia displeja 1.44 palcoveho
  tft.initR(INITR_144GREENTAB);

  // pripojenie k wifi
  WiFi.begin(ssid, password);
  // cakanie k dokonceniu pripojenia
  while (WiFi.status() != WL_CONNECTED)
  {
  }
  // adresa serveru
  Serial.print("WiFi adress: ");
  Serial.println(WiFi.localIP());
  // odoslanie stranky k uzivatelovi
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", page); });
  // spracovanie requestu s obsahom spravy, ktora sa vykresli na displej
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String rcvMsg;

              if (request->hasParam(inputText))
              {
                rcvMsg = request->getParam(inputText)->value();
                tft.fillScreen(ST77XX_BLACK);
                delay(1000);
                char *cstr = new char[rcvMsg.length() + 1];
                strcpy(cstr, rcvMsg.c_str());
                drawtext(cstr, ST77XX_WHITE);
                Serial.println(rcvMsg);
              }
              request->send(200, "text/html", "Text bol odoslany ! <a href=\"/\">Home</a>");
            });
  // spracovanie requestu, ktory ulozi prijaty BMP obrazok do SPIFFS a nasledne ho vykresli
  server.on(
      "/upload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
          // vytvorenie suboru v SPIFFS
          request->_tempFile = SPIFFS.open("/" + filename, "w");
        }
        if (len)
        {
          // zapis dat do SPIFFS
          request->_tempFile.write(data, len);
        }
        if (final)
        {

          // zatvorenie suboru, potom co sa uspesne podarilo obrazok ulozit do SPIFFS
          request->_tempFile.close();
          request->send(200, "text/plain", "File Uploaded !");
        }
        // vykreslenie obrazku
        filename = "/" + filename;
        tft.fillScreen(ST77XX_BLACK);
        char *cstr = new char[filename.length() + 1];
        strcpy(cstr, filename.c_str());
        reader.drawBMP(cstr, tft, 0, 0);
      });

  server.begin();

  tft.fillScreen(ST77XX_BLACK);

  Serial.println("Initialization is finised.");
  delay(1000);
}

void loop()
{
}