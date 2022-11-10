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
#include <MatrixHardware_ESP32_V0.h> // This file contains multiple ESP32 hardware configurations, edit the file to define GPIOPINOUT 
#include <SmartMatrix.h>
#include "config.h"                          

/*      GLOBAL VAR     */
static const char *TAG = "DISPLAY_EXAMPLE";
extern EventGroupHandle_t xEvent_data_COM;
extern QueueHandle_t xQueue_data_LED_COM;
/*    Panel Matrix creation    */
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, KMATRIX_WIDTH, KMATRIX_HEIGHT, KREFRESH_DEPTH, KDMA_BUFFER_ROWS, SMARTMATRIX_HUB75_16ROW_MOD8SCAN, SMARTMATRIX_OPTIONS_NONE);
/*  Allocate memory for these three layers   */
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(numberLayer, KMATRIX_WIDTH, KMATRIX_HEIGHT, COLOR_DEPTH, SM_BACKGROUND_OPTIONS_NONE);             
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, KMATRIX_WIDTH/5*4, KMATRIX_HEIGHT, COLOR_DEPTH, SM_SCROLLING_OPTIONS_NONE);        
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, KMATRIX_WIDTH, KMATRIX_HEIGHT, COLOR_DEPTH, SM_INDEXED_OPTIONS_NONE);              
boolean Scrolling = 0;                                  // Zustand des Fehlermeldungtextes (0 = Stehend, 1 = Scrolling)
unsigned int ScrollingTimer = 0, ScrollingDelay = 150;  // Wartepause, vor wiederholtem Scrollen des Textes
int i=0;
/**
 * @fn bool check_protocol()
 *
 * @brief Display logo Helptec
 *
 * @param str_error Message to control
 */
bool check_protocol(String str_error)
{
    bool valid_message=1;
    if((str_error.substring(12)).length() > 0)  
    {
      int i=0;
      char temp_str[COLOR_SIZE+ERROR_NUMBER_SIZE+2];
      str_error.toCharArray(temp_str, (COLOR_SIZE+ERROR_NUMBER_SIZE+2));
      for(i=0;i<ERROR_NUMBER_SIZE-1;i++)
      {
        if((temp_str[i]<0x30) || (temp_str[i]>0x39)) valid_message=0;
      } 
      if(temp_str[ERROR_NUMBER_SIZE-1]!=0x20) valid_message=0;
      for(i=ERROR_NUMBER_SIZE;i<(ERROR_NUMBER_SIZE+COLOR_SIZE);i++)
      {
        if((temp_str[i]<0x30) || (temp_str[i]>0x39)) valid_message=0;
      }
      if(temp_str[ERROR_NUMBER_SIZE+COLOR_SIZE]!=0x20) valid_message=0;
    }
    else
    {
      valid_message=0;
    }
    return valid_message;
}
/**
 * @fn void extract_values()
 *
 * @brief get the values from the message MQTT
 *
 * @param str_error Message to control
 */
// void extract_values(String str_error,uint8_t* color, char* error, char* text)
// {                                                                      
//   str_error.toCharArray(*error, ERROR_NUMBER_SIZE);  
//   str_error = str_error.substring(ERROR_NUMBER_SIZE);    
//   for (int i = 0; i < 6; i++)                         
//   {
//     if (str_error[i] < 58) *color[0+(i/2)] |=(str_error[i] - 48) << (4 * (1 - i%2)); 
//     else *color[0+(i/2)] |=(str_error[i] - 55) << (4 * (1 - i%2)); 
//   }
//   str_error = str_error.substring(7);                                                          
//   str_error.toCharArray(*text, str_error.length() + 1);
// }
/**
 * 
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
    vTaskDelay(pdMS_TO_TICKS(100));
}
/**
 * @fn void config_panel()
 *
 * @brief configuration of the pins use for the panel
 *
 */
void config_panel()
{
    matrix.setRotation(rotation180);                       
    matrix.setBrightness(DEFAULT_BRIGHTNESS);               
    scrollingLayer.setOffsetFromTop(DEFAULT_SCROLLING_OFFSET_FROM_TOP);
    scrollingLayer.setColor(WHITE);         
    scrollingLayer.setSpeed(SCROLLING_SPEED);                  
    scrollingLayer.setFont(font8x13);
    numberLayer.enableColorCorrection(true);  
    matrix.addLayer(&numberLayer);                         
    matrix.addLayer(&scrollingLayer);                                            
    matrix.begin();                                                                                                                                                                
    display_logo();
}
void MatrixText(String Fehlermeldung)  
{ 
  //ESP_LOGI(TAG,"MatrixText");
  // char FehlermeldungNummer[ERROR_NUMBER_SIZE] = ""; 
  // char FehlermeldungText[MAX_STR_SIZE_MSG] = "";
  // uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  // ESP_LOGI(TAG,"MatrixText : After def value watermark %d",uxHighWaterMark);
  // uint8_t temp[3]={0};
  // unsigned int FehlermeldungTextLength = 0;
  // i++;
  // ESP_LOGI(TAG,"TEST i : %d",i);    
  // // numberLayer.fillScreen(BLACK);
  // // numberLayer.swapBuffers(true);                                                              
  // // scrollingLayer.update("");
  // Fehlermeldung.toCharArray(FehlermeldungNummer, ERROR_NUMBER_SIZE);  
  // Fehlermeldung = Fehlermeldung.substring(ERROR_NUMBER_SIZE);
  // // char fault_color_str[6];
  // // for(int j = 0; j<6; j++)
  // // {
  // //   fault_color_str[i] = Fehlermeldung[i];
  // // }
  // // printf("MatrixText fault_color_str = %s\n", fault_color_str);
  // // uint32_t fault_color = (uint32_t)strtol(fault_color_str, NULL, 16);
  // // temp[0]=(uint8_t)fault_color>>16;
  // // temp[1]=(uint8_t)fault_color>>8;
  // // temp[2]=(uint8_t)fault_color;
  // for (int i = 0; i < 6; i++)
  // {
  //   if (Fehlermeldung[i] <= '9')
  //   {
  //     temp[0+(i/2)] |=(Fehlermeldung[i] - '0') << (4 * (1 - i%2));
  //   }
  //   else if (Fehlermeldung[i] >= '0')
  //   {
  //     temp[0+(i/2)] |=(Fehlermeldung[i] - '7') << (4 * (1 - i%2));
  //   }
  //   else
  //   {
  //     ESP_LOGI(TAG,"MatrixText Invalid char");
  //   }
  // }
  // // Fehlermeldung = Fehlermeldung.substring(7);                                 
  // // FehlermeldungTextLength = Fehlermeldung.length();                           
  // // Fehlermeldung.toCharArray(FehlermeldungText, FehlermeldungTextLength + 1);
  // // if(FehlermeldungTextLength > 16) 
  // // {
  // //   scrollingLayer.setMode(wrapForward); 
  // //   scrollingLayer.start(FehlermeldungText, -1);
  // // }
  // // else
  // // {
  // //   scrollingLayer.setMode(stopped);              
  // //   scrollingLayer.start(FehlermeldungText,-1);
  // // }                                           
  // numberLayer.fillRectangle(0, 0, 31, 15, {temp[0],temp[1],temp[2]}); 
  // numberLayer.swapBuffers(true);                                                                            
  // numberLayer.setFont(font8x13);                                                                         
  // numberLayer.drawString(0, 1, WHITE, FehlermeldungNummer); 
  // numberLayer.swapBuffers(true);      
  // uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  // ESP_LOGI(TAG,"MatrixText : end value watermark %d",uxHighWaterMark);                                                                                                                                                                   
}
/**
 * @fn void display_Task(void *arg)
 *
 * @brief Freertos task that allow to transmit data to the led panel
 *
 * @param arg FreeRTOS standard argument of a task
 */
void display_task(void *arg)
{
    EventBits_t flag_sub=0;
    // char error_number[ERROR_NUMBER_SIZE] = ""; 
    // char text[MAX_STR_SIZE_MSG] = "";
    // uint8_t color[3];
    //char error[MAX_STR_SIZE_MSG + COLOR_SIZE + ERROR_NUMBER_SIZE+1];
    char error;
    UBaseType_t uxHighWaterMark;
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG,"MatrixText : before start value watermark %d",uxHighWaterMark);
    while(true)
    {
      uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
      ESP_LOGI(TAG,"MatrixText : value watermark %d",uxHighWaterMark);
      flag_sub = xQueueReceive(xQueue_data_LED_COM, &error, pdMS_TO_TICKS(1000));
      if(flag_sub)
      {
        // String str_error(error);
        if(1)// if(check_protocol(str_error))
        {
          // extract_values(str_error,&color,&error_number,&text);
          // ESP_LOGI(TAG," COLOR : %d %d %d \n",color[0],color[1],color[2]);
          // printf("ERROR : %s \n",error_number);
          // printf("TEXT : %s \n",text);
          //MatrixText("");
        }
        else
        {
          // numberLayer.fillScreen(BLACK);                         
          // numberLayer.swapBuffers(true);                                                        
          // scrollingLayer.setMode(stopped);                  
          // scrollingLayer.update("");                                                
          // display_logo();
        }
      }
    }
}