/**
 * @file storage_module.h
 * Storage essential data for config via python script
 * @date 28.09.2022
 * Created on: 18 september 2022
 * @author Louis COLIN
 */
#ifndef MAIN_MQTT_MODULE_H_
#define MAIN_MQTT_MODULE_H_
// // ----------------------------------------------------------
// // Includes
// // ----------------------------------------------------------
// #include "config.h"
// #include "driver/gpio.h"
// #include "driver/uart.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "sdkconfig.h"
// #include <stdio.h>
// //----------------------------------------------------------
// // Defines
// //----------------------------------------------------------
// // UART CONFIG
// #define CONFIG_TEST_TXD (1)
// #define CONFIG_TEST_RXD (3)
// #define CONFIG_TEST_RTS (UART_PIN_NO_CHANGE)
// #define CONFIG_TEST_CTS (UART_PIN_NO_CHANGE)
// #define CONFIG_UART_PORT_NUM (0)
// #define CONFIG_UART_BAUD_RATE (115200)
// #define CONFIG_TASK_STACK_SIZE (2048)
// // Frame format  [HEADER][PAYLOAD][STOP_CMD] (Header) = [CMD SIZE SIZE]
// #define WRITE_COMMAND 0x00
// #define READ_COMMAND 0x01
// #define STOP_COMMAND 0xF0
// #define BYTE_FOR_SIZE 0x02
// // Inutile ?
// #define BUF_SIZE (1024)
// #ifdef __cplusplus
// extern "C"
// {
// #endif
//     //----------------------------------------------------------
//     // Prototypes
//     //----------------------------------------------------------
//     void NVS_RW_task(void *arg);
//     void storage_init(void);
//     void uart_init_config(void);
//     void read_MQTT_config(MQTT_config_t *);
//     void read_IP_value(char *, char *, char *);
// #ifdef __cplusplus
// }
// #endif
#endif /* MAIN_STORAGE_MODULE_H_ */