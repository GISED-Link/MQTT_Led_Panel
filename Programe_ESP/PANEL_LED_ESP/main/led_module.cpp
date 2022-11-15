/**
 * @file led_module.c
 * MQTT (over TCP)
 * @date 01.11.2022
 * Created on: 1 nov 2022
 * @author Louis COLIN
 */
/*      INCLUDES     */
#include <Arduino.h> // Must call that lib first
#include "led_module.hpp"
#include "config.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "logo.h"
#include "mqtt_module.h"
#include <MatrixHardware_ESP32_V0.h> // This file contains multiple ESP32 hardware configurations, edit the file to define GPIOPINOUT
#include <SmartMatrix.h>
#include <stdio.h>
#include <stdlib.h>
/*      GLOBAL VAR     */
static const char *TAG = "DISPLAY_EXAMPLE";
extern QueueHandle_t xQueue_data_LED_COM;
/*    Panel Matrix creation    */
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, KMATRIX_WIDTH, KMATRIX_HEIGHT, KREFRESH_DEPTH, KDMA_BUFFER_ROWS,
                             SMARTMATRIX_HUB75_16ROW_MOD8SCAN, SMARTMATRIX_OPTIONS_NONE);
/*  Allocate memory for these three layers   */
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(numberLayer, KMATRIX_WIDTH, KMATRIX_HEIGHT, COLOR_DEPTH,
                                      SM_BACKGROUND_OPTIONS_NONE);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, KMATRIX_WIDTH / 5 * 4, KMATRIX_HEIGHT, COLOR_DEPTH,
                                     SM_SCROLLING_OPTIONS_NONE);
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, KMATRIX_WIDTH, KMATRIX_HEIGHT, COLOR_DEPTH, SM_INDEXED_OPTIONS_NONE);
int K = 0;
lines next_line;
/**
 * @fn bool check_protocol()
 *
 * @brief Check protocol
 *
 * @param str_error Message to control
 */
bool check_protocol(char *str_error)
{
    bool valid_message = 1;
    if(strlen(str_error) > 12)
    {
        int i = 0;
        for(i = 0; i < ERROR_NUMBER_SIZE - 1; i++)
        {
            if((str_error[i] < 0x30) || (str_error[i] > 0x39)) valid_message = 0;
        }
        if(str_error[ERROR_NUMBER_SIZE - 1] != 0x20) valid_message = 0;
        for(i = ERROR_NUMBER_SIZE; i < (ERROR_NUMBER_SIZE + COLOR_SIZE); i++)
        {
            if((str_error[i] < 0x30) || (str_error[i] > 0x39)) valid_message = 0;
        }
        if(str_error[ERROR_NUMBER_SIZE + COLOR_SIZE] != 0x20) valid_message = 0;
    }
    else
    {
        valid_message = 0;
    }
    return valid_message;
}
/**
 * @fn put_all_led_off()
 *
 */
void put_all_led_off()
{
    numberLayer.fillScreen(BLACK);
    numberLayer.swapBuffers(true);
    scrollingLayer.setMode(stopped);
    scrollingLayer.update("");
}
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
            color.red = (uint8_t) LOGO_PIXELS[3 * y * 160 + 3 * x + 2];
            color.green = (uint8_t) LOGO_PIXELS[3 * y * 160 + 3 * x + 1];
            color.blue = (uint8_t) LOGO_PIXELS[3 * y * 160 + 3 * x];
            numberLayer.drawPixel(x, y, color);
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
void MatrixText(char *error)
{
    ESP_LOGI(TAG, "TEST K : %d", K);
    char errornumber[ERROR_NUMBER_SIZE] = "";
    uint8_t temp[3] = {0};
    for(int i = 0; i < ERROR_NUMBER_SIZE - 1; i++)
    {
        errornumber[i] = error[i];
    }
    put_all_led_off();
    for(int j = 0; j < 6; j++)
    {
        if(error[j + ERROR_NUMBER_SIZE] <= '9')
        {
            temp[0 + (j / 2)] |= (error[j + ERROR_NUMBER_SIZE] - '0') << (4 * (1 - j % 2));
        }
        else if(error[j] >= '0')
        {
            temp[0 + (j / 2)] |= (error[j + ERROR_NUMBER_SIZE] - '7') << (4 * (1 - j % 2));
        }
        else
        {
            ESP_LOGI(TAG, "MatrixText Invalid char");
        }
    }
    if(strlen(&error[ERROR_NUMBER_SIZE + COLOR_SIZE + 1]) > 16)
    {
        scrollingLayer.setMode(wrapForward);
        scrollingLayer.start(&error[ERROR_NUMBER_SIZE + COLOR_SIZE + 1], -1);
    }
    else
    {
        scrollingLayer.setMode(stopped);
        scrollingLayer.start(&error[ERROR_NUMBER_SIZE + COLOR_SIZE + 1], -1);
    }
    numberLayer.fillRectangle(0, 0, 31, 15, {temp[0], temp[1], temp[2]});
    numberLayer.swapBuffers(true);
    numberLayer.setFont(font8x13);
    numberLayer.drawString(0, 1, WHITE, errornumber);
    numberLayer.swapBuffers(true);
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
    EventBits_t msg_received = 0;
    char error[MAX_STR_SIZE_MSG + COLOR_SIZE + ERROR_NUMBER_SIZE + 1];
    UBaseType_t uxHighWaterMark;
    while(true)
    {
        msg_received = xQueueReceive(xQueue_data_LED_COM, &error, pdMS_TO_TICKS(1000));
        if(msg_received)
        {
            K++;
            uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(TAG, "MatrixText : value watermark %d", uxHighWaterMark);
            if(check_protocol(error))
            {
                MatrixText(error);
            }
            else
            {
                put_all_led_off();
                display_logo();
            }
        }
    }
}
/**
 * @fn void change_line()
 *
 * @brief function to change de line with the buffer
 *
 */
void change_line()
{
    gpio_set_level(LAT_PIN, LOW);
    switch(next_line)
    {
        case first_row :
            gpio_set_level(R1_PIN, LOW);
            gpio_set_level(G1_PIN, LOW);
            gpio_set_level(B1_PIN, LOW);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = second_row;
            break;
        case second_row :
            gpio_set_level(R1_PIN, HIGH);
            gpio_set_level(G1_PIN, LOW);
            gpio_set_level(B1_PIN, LOW);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = third_row;
            break;
        case third_row :
            gpio_set_level(R1_PIN, LOW);
            gpio_set_level(G1_PIN, HIGH);
            gpio_set_level(B1_PIN, LOW);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = fourth_row;
            break;
        case fourth_row :
            gpio_set_level(R1_PIN, HIGH);
            gpio_set_level(G1_PIN, HIGH);
            gpio_set_level(B1_PIN, LOW);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = fifth_row;
            break;
        case fifth_row :
            gpio_set_level(R1_PIN, LOW);
            gpio_set_level(G1_PIN, LOW);
            gpio_set_level(B1_PIN, HIGH);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = sixth_row;
            break;
        case sixth_row :
            gpio_set_level(R1_PIN, HIGH);
            gpio_set_level(G1_PIN, LOW);
            gpio_set_level(B1_PIN, HIGH);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = seventh_row;
            break;
        case seventh_row :
            gpio_set_level(R1_PIN, LOW);
            gpio_set_level(G1_PIN, HIGH);
            gpio_set_level(B1_PIN, HIGH);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = eighth_row;
            break;
        case eighth_row :
            gpio_set_level(R1_PIN, HIGH);
            gpio_set_level(G1_PIN, HIGH);
            gpio_set_level(B1_PIN, HIGH);
            gpio_set_level(R2_PIN, LOW);
            gpio_set_level(G2_PIN, LOW);
            next_line = first_row;
            break;
        default : break;
    }
    gpio_set_level(LAT_PIN, HIGH);
}