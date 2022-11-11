/**
 * @file config.h
 * MQTT (over TCP)
 * @date 10.08.2022
 * Created on: 4 juillet 2022
 * @author Thibault Sampiemon
 */
#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_
/*-----------------------------------------------------------------------
 * config file options
 * --------------------------------------------------------------------*/
#define USE_CONFIGURATIONFILE 1                    //!< use the config file instead of the default config
#define USER "pannel1"                             //!< username for the connection to the broker
#define PASS "itisnotagoodpasswordbutwhocarehaha1" //!< password for the connection to the broker
/*-----------------------------------------------------------------------
 * PCB options
 * --------------------------------------------------------------------*/
#define CONFIG_WITHOUT_POTENTIOMETER 1
//----------------------------------------------------------
// Includes
//----------------------------------------------------------
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
// ETHERNET
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include <netdb.h>
//----------------------------------------------------------
// Defines
//----------------------------------------------------------
// UART CONFIG
#define CONFIG_TEST_TXD (1)
#define CONFIG_TEST_RXD (3)
#define CONFIG_TEST_RTS (UART_PIN_NO_CHANGE)
#define CONFIG_TEST_CTS (UART_PIN_NO_CHANGE)
#define CONFIG_UART_PORT_NUM (0)
#define CONFIG_UART_BAUD_RATE (115200)
#define CONFIG_TASK_STACK_SIZE (2048)
#define BUF_SIZE 1024
// Frame format  [HEADER][PAYLOAD] (Header) = [R/W SIZE SIZE]
#define WRITE_COMMAND 0x00
#define READ_COMMAND 0x01
// Json objects
#define JSON_STRING 0
#define JSON_INT 1
// Max size for Json str item
#define MAX_STR_SIZE 50
#define MAX_IP_SIZE 16
// Json TAG
#define JSON_ARGS "pannel_config_args1"
#define JSON_USERNAME "MQTT_username"
#define JSON_PASSWORD "password"
#define JSON_BROKER_IP "broker_ip"
#define JSON_CLIENT_IP "client_ip"
#define JSON_GW_IP "gateway_ip"
#define JSON_MASK "mask"
#define JSON_CLIENT_ID "client_id"
#define JSON_PORT "broker_port"
#define JSON_TOPIC_BUTTON "topic0"
#define JSON_TOPIC_LED "topic1"
#define JSON_TOPIC_POTENTIOMETER "topic2"
// NVS TAG
#define STORAGE_PARTITION "nvs"
#define SIZE_ITEM "MQTTsize"
#define MQTT_CONFIG_STR_ITEM "MQTTstr"
// ETHERNET
#define PIN_PHY_POWER GPIO_NUM_12
#define ETHERNET_MASK_ADDR "255.255.255.00"
#define ETHERNET_GW_ADDR "169.254.36.255"
// PANEL LED
#define MAX_STR_SIZE_MSG 100
#define COLOR_SIZE 6
#define ERROR_NUMBER_SIZE 5
#ifdef __cplusplus
extern "C"
{
#endif
    //----------------------------------------------------------
    // Structure
    //----------------------------------------------------------
    typedef struct MQTT_config_t
    {
        char MQTT_username[MAX_STR_SIZE];
        char password[MAX_STR_SIZE];
        char ip[MAX_STR_SIZE];
        char id[MAX_STR_SIZE];
        uint16_t port;
        char *topic_bp;
        char *topic_del;
        char *topic_pot;
    } MQTT_config_t;
    //----------------------------------------------------------
    // Prototypes
    //----------------------------------------------------------
    void set_static_ip(esp_netif_t *);
    void ethernet_init();
    void NVS_RW_task(void *arg);
    void storage_init(void);
    void uart_init_config(void);
    void read_MQTT_config(MQTT_config_t *);
    void read_IP_value(char *, char *, char *);
#ifdef __cplusplus
}
#endif
#endif /* MAIN_CONFIG_H_ */
