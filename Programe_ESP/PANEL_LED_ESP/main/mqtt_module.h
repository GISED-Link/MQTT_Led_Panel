/**
 * @file mqtt_module.h
 * MQTT (over TCP)
 * @date 6.03.2022
 * Created on: 6 mai 2022
 * @author Thibault Sampiemon
 */
#ifndef MAIN_MQTT_MODULE_H_
#define MAIN_MQTT_MODULE_H_
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
//----------------------------------------------------------
// Defines
//----------------------------------------------------------
#define CONECTED_FLAG 0b00000001    //!< flag that inform that the esp is connected to the broker
#define DISCONECTED_FLAG 0b00000010 //!< flag that inform that the broker ended the connection properly
#define PUBLISHED_FLAG 0b00000100   //!< flag that inform that the broker received successfully a pub message
#define SUSCRIBED_FLAG 0b00001000   //!< flag that inform that the broker accept the subscription to a topic
#define UNSUSCRIBED_FLAG 0b00010000 //!< flag that inform that the broker unsubscribed the esp from a topic
#define DATA_FLAG 0b00100000        //!< flag that inform the esp received an update on a sbscribed topic
#define ERROR_FLAG 0b01000000       //!< flag that inform of a known error from mqtt
#define OTHER_FLAG 0b10000000       //!< flag that inform of any other message comming from the broker
#define RECEIVE_LED_FLAG 0b10000000 //!< flag that will activate when receiving a message from the broker
#ifdef __cplusplus
extern "C"
{
#endif
    void MQTT_Task(void *arg);
    void MQTT_init();
#ifdef __cplusplus
}
#endif
#endif /* MAIN_MQTT_MODULE_H_ */
