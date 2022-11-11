/**
 * @file main.c
 * MQTT (over TCP)
 * @date 6.03.2022
 * Created on: 6 mai 2022
 * @author Thibault Sampiemon
 */
#include <Arduino.h>
#include <stdbool.h>
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_module.hpp"
#include "mqtt_module.h"
#include "storage_module.h"
/*Tasks parameters*/
#define DISPLAY_TASK_PRIO 1    //!< the priority of the task that configure the storage with the json file
#define NVS_RW_TASK_PRIO 3    //!< the priority of the task that configure the storage with the json file
#define MQTT_TASK_PRIO 2      //!< the priority of the task that manage the communication with the broker mqtt
/**
 * @fn void app_main(void)
 *
 * @brief main function
 *
 *  the main only start all the FreeRTOS tasks after the initialization of each modules (one task for each modules)
 *
 */
SemaphoreHandle_t xSemaphore;
extern "C" void app_main(void)
{
    // If change of micropros to a dualcore in the futur Allow other core to finish initialization
    vTaskDelay(pdMS_TO_TICKS(100));
    // config peripherals
    uart_init_config();
    storage_init();
    // ethernet_init();
    MQTT_init();
    config_panel();
    // Task creation
    xTaskCreatePinnedToCore(MQTT_Task, "MQTT_Task", 2048, NULL, MQTT_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(display_task, "display_task", 4096, NULL, DISPLAY_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(NVS_RW_task, "NVS_RW_task", 2048, NULL, NVS_RW_TASK_PRIO, NULL, tskNO_AFFINITY);
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
