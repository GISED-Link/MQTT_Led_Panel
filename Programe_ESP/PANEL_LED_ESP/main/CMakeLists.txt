cmake_minimum_required(VERSION 3.5)
idf_component_register(SRCS "ethernet_module.c" "app_main.cpp"  "mqtt_module.c" "storage_module.c" 
                        "ethernet_module.c" "led_module.cpp" 
                    INCLUDE_DIRS ".")
project(${ProjectId})
