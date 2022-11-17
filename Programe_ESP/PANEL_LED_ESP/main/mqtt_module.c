/**
 * @file mqtt_module.c
 * MQTT (over TCP)
 * @date 6.03.2022
 * Created on: 6 mai 2022
 * @author Thibault Sampiemon
 */
#include "mqtt_module.h"
#include "config.h"
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
#define DEBUG 1
#define TOPIC_LED_ERROR "topic/leds_error"
static char topic_led_error[MAX_STR_SIZE];
static const char *TAG = "MQTT_EXAMPLE";
static EventGroupHandle_t xMQTTRecieveEventBits;
QueueHandle_t xQueue_data_LED_COM;
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
            xEventGroupSetBits(xMQTTRecieveEventBits, CONECTED_FLAG);
            xEventGroupClearBits(xMQTTRecieveEventBits, DISCONECTED_FLAG);
            break;
        case MQTT_EVENT_DISCONNECTED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits, DISCONECTED_FLAG);
            xEventGroupClearBits(xMQTTRecieveEventBits, CONECTED_FLAG);
            xEventGroupSetBits(xMQTTRecieveEventBits, UNSUSCRIBED_FLAG);
            xEventGroupClearBits(xMQTTRecieveEventBits, SUSCRIBED_FLAG);
            break;
        case MQTT_EVENT_SUBSCRIBED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits, SUSCRIBED_FLAG);
            xEventGroupClearBits(xMQTTRecieveEventBits, UNSUSCRIBED_FLAG);
            break;
        case MQTT_EVENT_UNSUBSCRIBED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits, UNSUSCRIBED_FLAG);
            xEventGroupClearBits(xMQTTRecieveEventBits, SUSCRIBED_FLAG);
            break;
        case MQTT_EVENT_PUBLISHED :
#if DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits, PUBLISHED_FLAG);
            break;
        case MQTT_EVENT_DATA :
            {
#if DEBUG
                ESP_LOGI(TAG, "MQTT_EVENT_DATA");
                printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                printf("DATA=%.*s\r\n", event->data_len, event->data);
#endif
                if(strlen(event->data) <= (MAX_STR_SIZE_MSG + COLOR_SIZE + ERROR_NUMBER_SIZE + 1))
                {
                    char message[MAX_STR_SIZE_MSG + COLOR_SIZE + ERROR_NUMBER_SIZE + 1] = "";
                    strncpy(message, event->data, strlen(event->data));
                    xQueueSend(xQueue_data_LED_COM, &message, (TickType_t) 0);
                }
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
                xEventGroupSetBits(xMQTTRecieveEventBits, ERROR_FLAG);
            }
            break;
        default :
#if DEBUG
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
#endif
            xEventGroupSetBits(xMQTTRecieveEventBits, OTHER_FLAG);
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
    MQTT_config_t Jsoncfng = {
        .MQTT_username = {""}, .password = {""}, .ip = {""}, .id = {""}, .port = 0, .topic_del = topic_led_error};
    read_MQTT_config(&Jsoncfng);
    esp_mqtt_client_config_t mqtt_cfg = {
        //.uri = CONFIG_BROKER_URL,
        .username = Jsoncfng.MQTT_username, .password = Jsoncfng.password,        .port = Jsoncfng.port,
        .client_id = Jsoncfng.id,           .transport = MQTT_TRANSPORT_OVER_TCP, .host = Jsoncfng.ip};
    // WIFI CONFIG
    // esp_mqtt_client_config_t mqtt_cfg = {
    //     //.uri = CONFIG_BROKER_URL,
    //     .username = "pannelLED1",  .password = "Test_Panel_LED1",        .port = 1883,
    //     .client_id = "Panel_LED1", .transport = MQTT_TRANSPORT_OVER_TCP, .host = "192.168.90.192"};
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
    EventBits_t mqtt_flag = 0;
    while(1)
    {
        mqtt_flag = xEventGroupWaitBits(xMQTTRecieveEventBits, CONECTED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(4000));
        while((mqtt_flag & CONECTED_FLAG) != CONECTED_FLAG)
        {
            mqtt_flag =
                xEventGroupWaitBits(xMQTTRecieveEventBits, CONECTED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(4000));
        }
        while((mqtt_flag & SUSCRIBED_FLAG) != SUSCRIBED_FLAG)
        {
            // int msg_id = esp_mqtt_client_subscribe(client, TOPIC_LED_ERROR, 2);
            int msg_id = esp_mqtt_client_subscribe(client, topic_led_error, 2);
#if DEBUG
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
#endif
            mqtt_flag =
                xEventGroupWaitBits(xMQTTRecieveEventBits, SUSCRIBED_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(4000));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
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
    xQueue_data_LED_COM = xQueueCreate(5, sizeof(char[MAX_STR_SIZE_MSG + COLOR_SIZE + ERROR_NUMBER_SIZE + 1]));
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
