/**
 * @file mqtt_module.h
 * MQTT (over TCP)
 * @date 6.03.2022
 * Created on: 6 mai 2022
 * @author Thibault Sampiemon
 */
#ifndef MAIN_MQTT_MODULE_H_
#define MAIN_MQTT_MODULE_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#define RECEIVE_LED_FLAG 0b10000000 //!< flag that will activate when receiving a message from the broker
#define SEND_BUTTON_FLAG                                                                                               \
    0b00000001 //!< flag to ask to send the value of the buttons (received by a queue in the mqtt task) to the broker
#define SEND_POTENTIOMETRE_FLAG                                                                                        \
    0b00000010 //!< flag to ask to send the value of the potentiometer (received by a queue in the mqtt task) to the
               //!< broker
    void MQTT_Task(void *arg);
    void MQTT_init();
#ifdef __cplusplus
}
#endif
#endif /* MAIN_MQTT_MODULE_H_ */
