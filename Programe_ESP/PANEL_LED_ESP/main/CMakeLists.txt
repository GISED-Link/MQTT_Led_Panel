idf_component_register(SRCS "ethernet_module.c" "app_main.cpp"  "mqtt_module.c" "storage_module.c" 
                        "ethernet_module.c" "led_module.cpp" 
                    INCLUDE_DIRS ".")
#spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT) "button_module.c" "adc_module.c"
