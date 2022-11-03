/**
 * @file mqtt_module.c
 * MQTT (over TCP)
 * @date 6.03.2022
 * Created on: 6 mai 2022
 * @author Thibault Sampiemon
 */
#include "mqtt_module.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define DEBUG 1
// #define PIN_PHY_POWER GPIO_NUM_12 //!< pin to power on the phy..mac controller of the olimex board
/*flag interpretation table for the flag coming from mqtt event loop of esp*/
#define CONECTED_FLAG                                                                                                  \
    0b00000001 //!< flag that inform that the esp is connected to the broker (receive connect response mqtt message)
#define DISCONECTED_FLAG                                                                                               \
    0b00000010 //!< flag that inform that the broker ended the connection properly (receive disconnect mqtt message)
#define PUBLISHED_FLAG                                                                                                 \
    0b00000100 //!< flag that inform that the broker received successfully the publish from esp(receive publish ack mqtt
               //!< message)
#define SUSCRIBED_FLAG                                                                                                 \
    0b00001000 //!< flag that inform that the broker received and accept the subscription to a topic from esp(receive
               //!< subscription ack mqtt message)
#define UNSUSCRIBED_FLAG                                                                                               \
    0b00010000 //!< flag that inform that the broker unsubscribed the esp from a specific topic (receive unsubscibe mqtt
               //!< message)
#define DATA_FLAG                                                                                                      \
    0b00100000 //!< flag that inform that the esp ricieved an update on a sbscribed topic from the broker (receive
               //!< publish message mqtt message from the brocker)
#define ERROR_FLAG 0b01000000 //!< flag that inform of a known error from mqtt
#define OTHER_FLAG 0b10000000 //!< flag that inform of any other message comming from the broker
/*---------------------------------------------------------
 * default config of the communication
 -----------------------------------------------------------*/
#if USE_CONFIGURATIONFILE != 1
#define topic_potentiometer "topic/potentiometer"
#define topic_button "topic/buttons"
#define topic_led "topic/leds"
#define USER "pannel1"
#define PASS "itisnotagoodpasswordbutwhocarehaha1"
#endif
static EventGroupHandle_t xMQTTRecieveEventBits;
EventGroupHandle_t xEvent_data_COM;
QueueHandle_t xQueue_data_LED_COM;    //!< FreeRTOS queue to ask the led task to activate a specific led or a group of
                                      //!< leds (from an command that come from the mqtt brocker)
QueueHandle_t xQueue_data_Button_COM; //!< FreeRTOS queue to ask the communication task to send the value of the
                                      //!< buttons to the broker
QueueHandle_t xQueue_data_Potentimeter_COM; //!< FreeRTOS queue to ask the communication task to send the value of
                                            //!< the potentiometer to the broker
// static char topic_button[MAX_STR_SIZE];
// static char topic_led[MAX_STR_SIZE];
// static char topic_potentiometer[MAX_STR_SIZE];
#define topic_potentiometer "topic/potentiometer"
#define topic_button "topic/buttons"
#define topic_led "topic/leds"
static const char *TAG = "MQTT_EXAMPLE";
/**
 *  @fn static void log_error_if_nonzero(const char *message, int error_code)
 *
 *  @brief function log
 *
 *  function used to send log trough the serial interface
 *
 *  @param message message to send trough the serial interface (generaly common to all massages of the module)
 *  @param error_code error code (mainly from mqtt event )
 */
static void log_error_if_nonzero(const char *message, int error_code)
{
    if(error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
esp_mqtt_client_handle_t client = NULL;
/**
 * @fn static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
 *
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
#if DEBUG
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
#endif
    esp_mqtt_event_handle_t event = event_data;
    switch((esp_mqtt_event_id_t) event_id)
    {
        case MQTT_EVENT_CONNECTED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits,   /* The event group being updated. */
                               CONECTED_FLAG);          /* The bits being set. */
            xEventGroupClearBits(xMQTTRecieveEventBits, /* The event group being updated. */
                                 DISCONECTED_FLAG);     /* The bits being reset. */
            break;
        case MQTT_EVENT_DISCONNECTED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits,   /* The event group being updated. */
                               DISCONECTED_FLAG);       /* The bits being set. */
            xEventGroupClearBits(xMQTTRecieveEventBits, /* The event group being updated. */
                                 CONECTED_FLAG);        /* The bits being reset. */
            break;
        case MQTT_EVENT_SUBSCRIBED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits,   /* The event group being updated. */
                               SUSCRIBED_FLAG);         /* The bits being set. */
            xEventGroupClearBits(xMQTTRecieveEventBits, /* The event group being updated. */
                                 UNSUSCRIBED_FLAG);     /* The bits being reset. */
            break;
        case MQTT_EVENT_UNSUBSCRIBED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits,   /* The event group being updated. */
                               UNSUSCRIBED_FLAG);       /* The bits being set. */
            xEventGroupClearBits(xMQTTRecieveEventBits, /* The event group being updated. */
                                 SUSCRIBED_FLAG);       /* The bits being reset. */
            break;
        case MQTT_EVENT_PUBLISHED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits, /* The event group being updated. */
                               PUBLISHED_FLAG);       /* The bits being set. */
            break;
        case MQTT_EVENT_DATA :
            {
#if DEBUG
                ESP_LOGI(TAG, "MQTT_EVENT_DATA");
                printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                printf("DATA=%.*s\r\n", event->data_len, event->data);
#endif
                int temp = 0;
                for(int u = 0; u < (event->data_len); u++)
                {
                    temp = temp * 10 + (event->data[u] - 48);
                }
#if DEBUG
                printf("DATA_Byte=%d\n", temp);
#endif
                uint8_t a = (uint8_t) temp;
                xQueueSend(xQueue_data_LED_COM, &a, (TickType_t) 0);
                xEventGroupSetBits(xMQTTRecieveEventBits, /* The event group being updated. */
                                   DATA_FLAG);            /* The bits being set. */
                xEventGroupSetBits(xEvent_data_COM, RECEIVE_LED_FLAG);
            }
            break;
        case MQTT_EVENT_ERROR :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
#endif
            if(event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",
                                     event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
                xEventGroupSetBits(xMQTTRecieveEventBits, /* The event group being updated. */
                                   ERROR_FLAG);           /* The bits being set. */
            }
            break;
        default :
#if DEBUG
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits, /* The event group being updated. */
                               OTHER_FLAG);           /* The bits being set. */
            break;
    }
}
/**
 * @fn static void mqtt_app_start(void)
 *
 * @brief mqtt comunucation start
 *
 *  initializes the communication between the panel and the plc
 *
 */
static void mqtt_app_start(void)
{
    // MQTT_config_t Jsoncfng = {.MQTT_username = {""},
    //                           .password = {""},
    //                           .ip = {""},
    //                           .id = {""},
    //                           .port = 0,
    //                           .topic_bp = topic_button,
    //                           .topic_del = topic_led,
    //                           .topic_pot = topic_potentiometer};
    // read_MQTT_config(&Jsoncfng);
    // esp_mqtt_client_config_t mqtt_cfg = {
    //     //.uri = CONFIG_BROKER_URL,
    //     .username = Jsoncfng.MQTT_username, .password = Jsoncfng.password,        .port = Jsoncfng.port,
    //     .client_id = Jsoncfng.id,           .transport = MQTT_TRANSPORT_OVER_TCP, .host = Jsoncfng.ip};
    // WIFI CONFIG
    esp_mqtt_client_config_t mqtt_cfg = {
        //.uri = CONFIG_BROKER_URL,
        .username = "pannelLED1",  .password = "Test_Panel_LED1",        .port = 1883,
        .client_id = "Panel_LED1", .transport = MQTT_TRANSPORT_OVER_TCP, .host = "192.168.90.192"};
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
/**
 * @fn void MQTT_Task(void *arg)
 *
 * @brief Freertos task that handle mqtt communication
 *
 *  this task manage communication between the pannel and the plc
 *
 * @param arg FreeRTOS standard argument of a task
 */
void MQTT_Task(void *arg)
{
    int i = 0;
    char str[5] = "     ";
    EventBits_t flag_internal = 0;
    EventBits_t com_flags = 0;
    EventBits_t com_flags2 = 0;
    EventBits_t com_flags3 = 0;
    BaseType_t que_return_recieve1 = 0;
    BaseType_t que_return_recieve2 = 0;
    /*---------------------------------------------------------------------------------------------------------------------------------
     * In case of deconnexio or communication error Try reconnecting to the mqtt brocker (reintitilyse communication
     *from scratch)
     *-----------------------------------------------------------------------------------------------------------------------------------*/
    com_flags2 = 0;
    com_flags2 = xEventGroupWaitBits(xMQTTRecieveEventBits, CONECTED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(4000));
    while((com_flags2 & CONECTED_FLAG) != CONECTED_FLAG)
    {
        // MQTT_init();
        com_flags2 = xEventGroupWaitBits(xMQTTRecieveEventBits, CONECTED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(4000));
    }
    /*---------------------------------------------------------------------------------------------------------------------------------
     * Suscribe to the led topic in the init
     *-----------------------------------------------------------------------------------------------------------------------------------*/
    do
    {
        int msg_id = esp_mqtt_client_subscribe(client, topic_led, 2);
#if DEBUG
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
#endif
        com_flags2 = xEventGroupWaitBits(xMQTTRecieveEventBits, SUSCRIBED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(200));
    } while((com_flags2 & SUSCRIBED_FLAG) != SUSCRIBED_FLAG);
    //---------------------------------------------------------------------------------------------------------------------------------
    while(1)
    { // task loop
        com_flags = 0;
        com_flags =
            xEventGroupWaitBits(xMQTTRecieveEventBits, CONECTED_FLAG | UNSUSCRIBED_FLAG | DISCONECTED_FLAG | ERROR_FLAG,
                                pdFALSE, pdFALSE, pdMS_TO_TICKS(200));
        /*---------------------------------------------------------------------------------------------------------------------------------
         * In case of deconnexio or communication error Try reconnecting to the mqtt brocker (reintitilyse communication
         *from scratch)
         *-----------------------------------------------------------------------------------------------------------------------------------*/
        if((com_flags & DISCONECTED_FLAG) == DISCONECTED_FLAG || (com_flags & ERROR_FLAG) == ERROR_FLAG)
        {
            xEventGroupClearBits(xMQTTRecieveEventBits, /* The event group being updated. */
                                 ERROR_FLAG);           /* The bits being reset. */
            com_flags2 = 0;
            do
            {
                // MQTT_init();
                com_flags2 =
                    xEventGroupWaitBits(xMQTTRecieveEventBits, CONECTED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(4000));
            } while((com_flags2 & CONECTED_FLAG) != CONECTED_FLAG);
        }
        /*---------------------------------------------------------------------------------------------------------------------------------
         * Resuscribe to the led topic in the init
         *-----------------------------------------------------------------------------------------------------------------------------------*/
        if((com_flags & UNSUSCRIBED_FLAG) == UNSUSCRIBED_FLAG)
        {
            com_flags2 = 0;
            do
            {
                int msg_id = esp_mqtt_client_subscribe(client, topic_led, 2);
#if DEBUG
                ESP_LOGI(TAG, "sent resubscribe successful, msg_id=%d", msg_id);
#endif
                com_flags2 =
                    xEventGroupWaitBits(xMQTTRecieveEventBits, SUSCRIBED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(200));
            } while((com_flags2 & SUSCRIBED_FLAG) != SUSCRIBED_FLAG);
        }
        /*---------------------------------------------------------------------------------------------------------------------------------
         * Publish to the button topic or potentiometer topic if connected to broker
         *-----------------------------------------------------------------------------------------------------------------------------------*/
        if((com_flags & CONECTED_FLAG) == CONECTED_FLAG)
        { // if connected
            i = 0;
            flag_internal = xEventGroupWaitBits(xEvent_data_COM, SEND_POTENTIOMETRE_FLAG | SEND_BUTTON_FLAG, pdTRUE,
                                                pdFALSE, pdMS_TO_TICKS(100));
            if((flag_internal & SEND_BUTTON_FLAG) == SEND_BUTTON_FLAG)
            { // if qu ricieve button
                /*-----------------------------------------------------------------------------------------------
                 * Publish to the button topic
                 *----------------------------------------------------------------------------------------------*/
                i = 0;
                que_return_recieve1 = xQueueReceive(xQueue_data_Button_COM, &i, pdMS_TO_TICKS(20));
                if(que_return_recieve1 == pdTRUE)
                { // if queue ricieve button
                    do
                    {
                        com_flags3 = 0;
                        sprintf(str, "%d", i);
                        esp_mqtt_client_publish(client, topic_button, str, 0, 1, 0);
#if DEBUG
                        ESP_LOGI(TAG, "publisch data");
#endif
                        com_flags3 = xEventGroupWaitBits(xMQTTRecieveEventBits, PUBLISHED_FLAG, pdTRUE, pdFALSE,
                                                         pdMS_TO_TICKS(200));
                    } while((com_flags3 & PUBLISHED_FLAG) != PUBLISHED_FLAG);
                } // end if queue ricieve button
            }     // end if flag  button
            else if((flag_internal & SEND_POTENTIOMETRE_FLAG) == SEND_POTENTIOMETRE_FLAG)
            { // if flag  pottentiommeter
                /*-----------------------------------------------------------------------------------------------
                 * Publish to the potentiometer topic
                 *----------------------------------------------------------------------------------------------*/
                i = 0;
                que_return_recieve2 = xQueueReceive(xQueue_data_Potentimeter_COM, &i, pdMS_TO_TICKS(20));
                if(que_return_recieve2 == pdTRUE)
                { // if queue ricieve potentiometer
                    do
                    {
                        com_flags3 = 0;
                        sprintf(str, "%d", i);
                        esp_mqtt_client_publish(client, topic_potentiometer, str, 0, 1, 0);
#if DEBUG
                        ESP_LOGI(TAG, "publisch data");
#endif
                        com_flags3 = xEventGroupWaitBits(xMQTTRecieveEventBits, PUBLISHED_FLAG, pdTRUE, pdFALSE,
                                                         pdMS_TO_TICKS(200));
                    } while((com_flags3 & PUBLISHED_FLAG) != PUBLISHED_FLAG);
                } // end if queue ricieve potentiometer
            }     // end if flag  pottentiommeter
        }         // end if connected
    }             // endtask loop
} // end TASK
/**
 * @fn void MQTT_init()
 *
 * @brief initialize mqtt
 *
 *  this function initialize all the freertos communication artifacts, driver and material needed for the communication
 *
 */
void MQTT_init()
{
#if DEBUG
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
#endif
    xMQTTRecieveEventBits = xEventGroupCreate();
    xEvent_data_COM = xEventGroupCreate();
    xQueue_data_LED_COM = xQueueCreate(20, sizeof(uint8_t));
    xQueue_data_Button_COM = xQueueCreate(20, sizeof(uint8_t));
    xQueue_data_Potentimeter_COM = xQueueCreate(20, sizeof(uint8_t));
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
}
