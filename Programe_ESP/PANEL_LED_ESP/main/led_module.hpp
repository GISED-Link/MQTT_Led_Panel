
/**
 * @file LED_module.h
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
#define CH_D -1 // required for 32 rows panels need to route the pins
#define CH_E -1 // required for 64 rows panels need to route the pins
#define LAT 4
#define OE 15
#define CLK 16
// MATRIX LILBRARY CONFIG
#define PANEL_RES_X 32 // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 16 // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 4  // Total number of panels chained one to another
/*      PROTOTYPES      */
void display_logo();
void config_panel();
void MatrixText();
void Display_Task(void *arg);
void test();
#endif /* MAIN_BUTTON_MODULE_H_ */