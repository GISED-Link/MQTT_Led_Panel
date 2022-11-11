/**
 * @file led_module.hpp
 * MQTT (over TCP)
 * @date 01.11.2022
 * Created on: 1 nov 2022
 * @author Louis COLIN
 */
#ifndef MAIN_LED_MODULE_H_
#define MAIN_LED_MODULE_H_
/*      INCLUDES        */
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "config.h"
#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <stdio.h>
#include <stdlib.h>
/*      DEFINES     */
/*-----------------  Redefine LED  ---------------------*/
#define R1 17
#define G1 26
#define BL1 27
#define R2 14
#define G2 12
#define BL2 13
#define CH_A 23
#define CH_B 19
#define CH_C 5
#define CH_D -1 // required for 32 rows panels
#define CH_E -1 // required for 64 rows panels
#define LAT 4
#define OE 15
#define CLK 16
// MATRIX LILBRARY CONFIG 
#define COLOR_DEPTH 24 // Choose the color depth used for storing pixels in the layers: 24 or 48
#define KMATRIX_WIDTH  160 // Pixel number per row 32, 64, 96, 128, 160 (32 per panel)
#define KMATRIX_HEIGHT 16  // Pixel number per column 
#define KREFRESH_DEPTH 24  // Refreshrate (fps): 24, 36, 48
#define KDMA_BUFFER_ROWS 4
#define DEFAULT_BRIGHTNESS 50 // Brightness max
#define DEFAULT_SCROLLING_OFFSET_FROM_TOP 1
#define DEFAULT_SCROLLING_OFFSET_FROM_LEFT 32
#define SCROLLING_SPEED 47
#define BLACK {0x00,0x00,0x00}
#define WHITE {0xff,0xff,0xff}
/*      PROTOTYPES      */
void display_logo();
void config_panel();
void MatrixText(char *);
void display_task(void *);
bool check_protocol(char *);
void put_all_led_off();
#endif /* MAIN_BUTTON_MODULE_H_ */