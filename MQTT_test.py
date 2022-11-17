# -*- coding: utf-8 -*-
"""
Created on Wed Nov  9 13:42:20 2022

@author: louis
"""
import serial.tools.list_ports
import paho.mqtt.client as mqtt  # import the client1
import pyads
import threading
import time

broker_address = "192.168.90.192"  # addr locale
# def des nom des topics
topic_pannel_leds = "topic/pannel_led/leds_error"
user_name = "PyClient2"
user_pass = "CBPlcbB2046HPLPYWNFT"
storage_dic = {}
watermark_walue = ""
i = 0
# prévien lors d'une déconexion


def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("Unexpected MQTT disconnection. Will auto-reconnect\n")
# lors d'une reconnection ou d'une conection subscribe au bons topics


def on_connect(client, userdata, flags, rc):
    print("Subscribing to topic\n", topic_pannel_leds)
    client.subscribe(topic_pannel_leds, qos=1)


def thread_function(name):
    global i
    global watermark_walue
    while(1):
        value = input("entrez une commande valide pour l'afficher sur le panel \n")
        if(value == "quit"):
            client.disconnect()
            break
        else:
            value = value+'\0'
            client.publish(topic_pannel_leds, value, qos=2)  # publish
            print("publish var=", value)
            value = ""
        # time.sleep(0.1)
        # i = i+1
        # client.publish(topic_pannel_leds, "000"+str(i % 10)+" 113355"+" " +
        #                " TEST long drfdhgvjgdqskbfhdsbfjsbhkqdsbbhdsbk"+"\0", qos=2)


# ser = serial.Serial(port="COM8", baudrate=115200, timeout=0.5)
client = mqtt.Client("P1")  # create new instance
client.username_pw_set(user_name, user_pass)
client.connect(broker_address)  # connect to broker
client.publish("topic/leds", 0)  # publish
client.on_disconnect = on_disconnect  # attach function to callback si déconection
client.on_connect = on_connect  # attach function to callback si conection
print("connecting to broker\n")
client.connect(broker_address)  # connect au broker
x = threading.Thread(target=thread_function, args=(1,), daemon=True)  # generation tread
x.start()  # generation tread
client.loop_forever()  # start la loop du broker
x.join()  # generation tread
