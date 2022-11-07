/**
 * @file led_mdule.c
 * MQTT (over TCP)
 * @date 01.11.2022
 * Created on: 1 nov 2022
 * @author Louis COLIN
 */
/*      INCLUDES     */
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include "logo.h"
#include "led_module.hpp"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mqtt_module.h"

/*      GLOBAL VAR     */
static const char *TAG = "DISPLAY_EXAMPLE";
MatrixPanel_I2S_DMA *dma_display = NULL;
#include <MatrixHardware_ESP32_V0.h>                // This file contains multiple ESP32 hardware configurations, edit the file to define GPIOPINOUT (or add #define GPIOPINOUT with a hardcoded number before this #include)
#include <SmartMatrix.h>

#define COLOR_DEPTH 24                  // Choose the color depth used for storing pixels in the layers: 24 or 48 (24 is good for most sketches - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24)
const uint8_t kMatrixWidth = 160;                                    // Anzahl Pixel der Gesammtlänge der Anzeige: 32, 64, 96, 128, 160
const uint8_t kMatrixHeight = 16;                                    // Anzahl Pixel der Gesammthöhe der Anzeige: 16
const uint8_t kRefreshDepth = 24;                                    // Refreshrate (fps): 24, 36, 48
const uint8_t kDmaBufferRows = 4;                                    // Grösse des DMA-Buffers: 4
const uint8_t kPanelType = SMARTMATRIX_HUB75_16ROW_MOD8SCAN;         // LED-RGB-Matrix-typ: HUB75 16 Zeilen, 8 Pixel Scanrate
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);           // Matrix-Optionen, siehe: http://docs.pixelmatix.com/SmartMatrix
const uint8_t knumberLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);    // Background-Layer für Fehlermeldungscode mit Name "numberLayer"
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);  // Scrolling-Layer für Fehlermeldungstext mit Name "scrollingLayer"
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);      // Indexed-Layer für Statusangaben mit Name "indexedLayer"

/*  TEST     */
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer1, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer2, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer3, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer4, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer5, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);

SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(numberLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, knumberLayerOptions);             // Übertrage Einstellungen auf Layer "numberLaer"
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);        // Übertrage Einstellungen auf Layer "scrollingLayer"
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);              // Übertrage Einstellungen auf Layer "indexedLayer"

const int defaultBrightness = 100 * (255 / 100);            // Helligkeit der Gesammtanzeige: 100%
const int defaultScrollOffset = 1;                          // Versatz des Fehlermeldung-Textes zur Oberkannte der Anzeige
const rgb24 defaultBackgroundColor = { 0x00, 0x00, 0x00 };  // Hintergrundfarbe: Ausgeschalten

String Fehlermeldung = "";                                       // Eintreffende Daten werden in diesem String abgespeichert
String StringIn, AddString, StringRead;                         // Flankenerkennung eines neu eingetroffenen Strings
char FehlermeldungNummer[5] = "", FehlermeldungText[100] = "";  // Ausgabe-Array für Fehlermeldungsnummer und Fehlermeldungstext (Max. Zeichen-Anzahl = 100)
unsigned long Farbe;                                            // Farbe des Hintergrunds von Fehlermeldungsnummer
unsigned int FehlermeldungTextLength;                           // Länge des Fehlermeldungtextes
boolean alreadyConnected = 0, ReceivedString = 0, SDState = 0;  // Flags für Datenverarbeitung

unsigned int ScrollSpeed = 47;                          // Scroll-Geschwindigkeit der Fehlermeldung
boolean Scrolling = 0;                                  // Zustand des Fehlermeldungtextes (0 = Stehend, 1 = Scrolling)
unsigned int ScrollingTimer = 0, ScrollingDelay = 150;  // Wartepause, vor wiederholtem Scrollen des Textes
/**
 * @fn void display_logo()
 *
 * @brief Display logo Helptec
 *
 */
void display_logo()
{
    int x, y;
    rgb24 color;
    for(y = 0; y < 16; y++)
    {
        for(x = 0; x < 160; x++)
        {
            // logo is convert to BGR HEX
            color.red=(uint8_t) LOGO_PIXELS[3 * y * 160 + 3 * x + 2];
            color.green=(uint8_t) LOGO_PIXELS[3 * y * 160 + 3 * x + 1];
            color.blue=(uint8_t) LOGO_PIXELS[3 * y * 160 + 3 * x];
            numberLayer.drawPixel(x,y,color);
        }
    }
    numberLayer.swapBuffers(true);
}
/**
 * @fn void config_panel()
 *
 * @brief configuration of the pins use for the panel
 *
 */
void config_panel()
{
    matrix.setRotation(rotation180);                       // Matrix-Ausrichtung festlegen
    matrix.setBrightness(defaultBrightness);               // Matrix-Helligkeit übertragen
    scrollingLayer.setOffsetFromTop(defaultScrollOffset);  // Text-Abstand gegenüber Oberkannte festlegen
    numberLayer.enableColorCorrection(true);               // Farbkorrektur aktivieren
    matrix.addLayer(&numberLayer);                         // numberLayer befindet sich auf der hintersten Anzeige-Ebene
    matrix.addLayer(&indexedLayer);                        // indexedLayer befindet sich auf der mittleren Anzeige-Ebene
    matrix.addLayer(&scrollingLayer);                      // scrollingLayer befindet sich auf der vordesten Anzeige-Ebene
    matrix.begin();                                        // Matrix-Ausgabe starten
    // Ethernet Connection
    indexedLayer.setFont(font8x13);                         // Schriftsatz einstellen: 8x13 Pixel
    indexedLayer.setIndexedColor(1, { 0xff, 0xff, 0xff });  // Farbe der Schrift festlegen: Weiss
    indexedLayer.fillScreen(0);                             // Anzeige komplet löschen (Schwarz ausfüllen)
    delay(2000);
    MatrixText();
}

void MatrixText()  // Ausgabe-Funktion der Matrix-Anzeige
{
  memset(FehlermeldungNummer, 0, sizeof(FehlermeldungNummer));  // FehlermeldungNummer-Array löschen
  memset(FehlermeldungText, 0, sizeof(FehlermeldungText));      // FehlermeldungText-Array löschen
  uint8_t temp[3]={0};
  Farbe = 0x000000;                                             // Fehlermeldung-Farbe löschen
  FehlermeldungTextLength = 0;                                  // Fehlermeldungs Textläne löschen
  numberLayer.fillScreen(defaultBackgroundColor);               // Matrix-hintergrundfarbe festlegen
  numberLayer.swapBuffers();                                    // Aktualisiere numberLayer
  indexedLayer.fillScreen(0);                                   // Lösche indexedLayer (Schwarz ausfüllen)
  indexedLayer.swapBuffers();                                   // Aktualisiere indexedLayer
  scrollingLayer.update("");                                    // Lösche FehlermeldungsText                                         // Lösche Statusausgabe

  Fehlermeldung = StringRead;     // Übertrage Fehlermeldungsinhalt in String
  //Serial.println(Fehlermeldung);  // Ausgabe von Fehlermeldung auf Serieller-Schnittstelle
  if ((Fehlermeldung.substring(12)).length() > 0)  // Sofern Fehlermeldungs-Gesammtlänge grösser als 12 Zeichen ist (XXXX XXXXXX X)
  {
    Fehlermeldung.toCharArray(FehlermeldungNummer, 5);  // Konvertiere die ersten 5 Zeichen des Strings zu einem Char-Array (FehlermeldungNummer)
    Fehlermeldung = Fehlermeldung.substring(5);         // Entferne die ersten 5 Zeichen des Strings (XXXX_)
    for (int i = 0; i < 6; i++)                         // Auslesen des HEX-Farbcodes (6 Zeichen)
    {
      if (Fehlermeldung[i] < 58) temp[0+(i/2)] |=(Fehlermeldung[i] - 48) << (4 * (1 - i%2)); //Farbe |= (Fehlermeldung[i] - 48) << (4 * (5 - i));  // ASCII Zeichenbereich 0...9
      else temp[0+(i/2)] |=(Fehlermeldung[i] - 55) << (4 * (1 - i%2)); //Farbe |= (Fehlermeldung[i] - 55) << (4 * (5 - i));                        // ASCIIZeichenbereich  A...F
    }
    Fehlermeldung = Fehlermeldung.substring(7);                                 // Entferne die folgenden 7 Zeichen des Strings (XXXXXX_)
    FehlermeldungTextLength = Fehlermeldung.length();                           // Ermittle die zeichenanzahl des eingentlichen Fehlertextes
    Fehlermeldung.toCharArray(FehlermeldungText, FehlermeldungTextLength + 1);  // Konvertiere verbleibender String zu Char-Array (FehlermeldungText)

    scrollingLayer.setColor({ 0xFF, 0xFF, 0xFF });         // Schriftfarbe definieren: Weiss
    scrollingLayer.setSpeed(ScrollSpeed);                  // Scroll-Geschwindigkeit festlegen
    scrollingLayer.setFont(font8x13);                      // Schriftsatz einstellen: 8x13 Pixel
    scrollingLayer.setOffsetFromTop(defaultScrollOffset);  // Versatz des Fehlermeldung-Textes zur Oberkannte der Anzeige
    scrollingLayer.setStartOffsetFromLeft(1);              // Versatz des Fehlermeldung-Textes nach Rechts

    scrollingLayer.start(FehlermeldungText, -1);  // Scrolling-Modus auf kontinuierlich wiederholen wechseln
    scrollingLayer.setMode(stopped);              // Scrolling-Text anhalten/fixieren
    scrollingLayer.update(FehlermeldungText);     // Übergabe des Fehlermeldungtextes auf Ausgabe-Funktion

    if (FehlermeldungTextLength > 16) Scrolling = 1;  // Wenn der Fehlermeldungtext länger ist als 16 Zeichen, Scrollen aktivieren
    else Scrolling = 0;                               // Sonst den Text linksbündig anzeigen
    numberLayer.fillRectangle(0, 0, 31, 15, { temp[0],temp[1],temp[2]/*(Farbe >> 16) & 0xFF), (Farbe >> 8) & 0xFF, Farbe & 0xFF */});  // Farbiger Hintergrund hinter Fehlermeldung-Nummer zeichnen (32x16 Pixel)
    numberLayer.swapBuffers();                                                                             // Ausgabe des Hintergrundes
    numberLayer.setFont(font8x13);                                                                         // Schriftsatz einstellen: 8x13 Pixel
    numberLayer.drawString(0, 1, { 0xff, 0xff, 0xff }, FehlermeldungNummer);                               // Ausgabe der Fehlermeldung-Nummer, Schriftfarbe: Weiss
    numberLayer.swapBuffers();                                                                             // Matrix aktualisieren
  } else                                                                                                   // Wenn keine Fehlermeldung mehr besteht
  {
    numberLayer.fillScreen(defaultBackgroundColor);         // Matrix-Hintergrundfarbe festlegen
    numberLayer.swapBuffers();                              // Matrix aktualisieren
    indexedLayer.fillScreen(0);                             // Matrix komplett löschen (Schwarz ausfüllen)
    indexedLayer.swapBuffers();                             // Matrix erneut aktualisieren
    scrollingLayer.setMode(stopped);                        // Scrolling-Text anhalten/fixieren
    scrollingLayer.update("");                              // Lösche FehlermeldungsText
    indexedLayer.setFont(font8x13);                         // Schriftsatz einstellen: 8x13 Pixel
    indexedLayer.setIndexedColor(1, { 0xff, 0xff, 0xff });  // Schriftfarbe festlegen: Weiss
    indexedLayer.fillScreen(0);                             // Matrix komplett löschen (Schwarz ausfüllen)
    display_logo();
  }
}
/**
 * @fn void Display_Task(void *arg)
 *
 * @brief Freertos task that allow to transmit data to the led panel
 *
 * @param arg FreeRTOS standard argument of a task
 */
void Display_Task(void *arg)
{
    while(1)
    {
        // display_logo();
    }
}