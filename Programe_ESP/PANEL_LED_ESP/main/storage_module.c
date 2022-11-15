/**
 * @file storage_module.c
 * Storage essential data for config via python script
 * @date 28.09.2022
 * Created on: 28 september 2022
 * @author Louis COLIN
 */
#include "config.h"
// #include "storage_module.h"
static const char *TAG = "STORAGE EXAMPLE";
/**
 * @fn void NVS_RW_task(void *arg)
 *
 * @brief Freertos task that configure MQTT parameters
 *
 *  this task allow the MQTT configuration from a software through uart
 *
 * @param arg FreeRTOS standard argument of a task
 */
void NVS_RW_task(void *arg)
{
    uint8_t *data = NULL;
    uint16_t remaining_size = 0;
    uint8_t header[3] = {0};
    uint16_t size = 0;
    nvs_handle_t my_handle;
    while(1)
    {
        int len = uart_read_bytes(CONFIG_UART_PORT_NUM, header, (3), 20 / portTICK_PERIOD_MS);
        if(len == 3)
        {
            switch(header[0])
            {
                case WRITE_COMMAND :
                    size = (header[1] << 8) + (header[2]);
                    data = (uint8_t *) malloc(size);
                    remaining_size = size;
                    while(remaining_size != 0)
                    {
                        len = uart_read_bytes(CONFIG_UART_PORT_NUM, &(data[size - remaining_size]), remaining_size,
                                              5 / portTICK_PERIOD_MS);
                        remaining_size = remaining_size - len;
                    }
                    nvs_open(STORAGE_PARTITION, NVS_READWRITE, &my_handle);
                    nvs_set_u16(my_handle, SIZE_ITEM, size);
                    nvs_set_str(my_handle, MQTT_CONFIG_STR_ITEM, (const char *) data);
                    nvs_commit(my_handle);
                    nvs_close(my_handle);
                    free(data);
                    assert(0); // force hardfault
                    break;
                case READ_COMMAND :
                    size = (header[1] << 8) + (header[2]);
                    if(size == 0)
                    {
                        nvs_open(STORAGE_PARTITION, NVS_READWRITE, &my_handle);
                        esp_err_t err = nvs_get_u16(my_handle, SIZE_ITEM, &size);
                        char temp_size[2] = {((size >> 8) & 0xFF), (size & 0xFF)};
                        if(err == ESP_OK)
                        {
                            data = (uint8_t *) malloc(size);
                            nvs_get_str(my_handle, MQTT_CONFIG_STR_ITEM, (char *) data, (size_t *) &size);
                            nvs_close(my_handle);
                            uart_write_bytes(CONFIG_UART_PORT_NUM, (const char *) temp_size, 2);
                            uart_write_bytes(CONFIG_UART_PORT_NUM, (const char *) data, size - 1);
                            free(data);
                        }
                        else
                        {
                            nvs_close(my_handle);
                        }
                        break;
                    }
                default : break;
            }
        }
    }
}
/**
 * @fn void storage_init()
 *
 * @brief initialize NVS
 *
 *  initialize NVS for the json file for the MQTT configuration parameters
 *
 */
void storage_init(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}
/**
 * @fn void uart_init_config()
 *
 * @brief initialize uart
 *
 *  initialize uart parameters to load a json string to config the MQTT protocol
 *
 */
void uart_init_config(void)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = CONFIG_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(
        uart_set_pin(CONFIG_UART_PORT_NUM, CONFIG_TEST_TXD, CONFIG_TEST_RXD, CONFIG_TEST_RTS, CONFIG_TEST_CTS));
}
/**
 * @fn void read_MQTT_config()
 *
 * @brief read json file objects to give the value of the researched item
 *
 *
 * @param port, data pointer for the port value
 * @param MQTT_username, data pointer for the username value
 * @param password, data pointer for the the password value
 * @param ip, data pointer for the ip adress value
 * @param topic_bp, data pointer for the button topic value
 * @param topic_del, data pointer for the led topic value
 *
 */
void read_MQTT_config(MQTT_config_t *ConfigStruct)
{
    nvs_handle_t my_handle;
    uint8_t *data = NULL;
    uint16_t size = 0;
    nvs_open(STORAGE_PARTITION, NVS_READWRITE, &my_handle);
    esp_err_t err = nvs_get_u16(my_handle, SIZE_ITEM, &size);
    if(err == ESP_OK)
    {
        ESP_LOGI(TAG, "OPEN\n");
        data = (uint8_t *) malloc(size);
        nvs_get_str(my_handle, MQTT_CONFIG_STR_ITEM, (char *) data, (size_t *) &size);
        nvs_close(my_handle);
        cJSON *config_json = cJSON_Parse((const char *) data);
        if(config_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            if(error_ptr != NULL)
            {
                ESP_LOGI(TAG, "Error before: %s\n", error_ptr);
            }
        }
        else
        {
            cJSON *cfng = cJSON_GetObjectItem(config_json, JSON_ARGS);
            ESP_LOGI(TAG, "NON NUL\n");
            if(cfng == NULL)
            {
                ESP_LOGI(TAG, "NUL\n");
            }
            if(strlen(cJSON_GetObjectItem(cfng, JSON_USERNAME)->valuestring) < MAX_STR_SIZE)
            {
                strncpy(ConfigStruct->MQTT_username, cJSON_GetObjectItem(cfng, JSON_USERNAME)->valuestring,
                        strlen(cJSON_GetObjectItem(cfng, JSON_USERNAME)->valuestring));
            }
            if(strlen(cJSON_GetObjectItem(cfng, JSON_PASSWORD)->valuestring) < MAX_STR_SIZE)
            {
                strncpy(ConfigStruct->password, cJSON_GetObjectItem(cfng, JSON_PASSWORD)->valuestring,
                        strlen(cJSON_GetObjectItem(cfng, JSON_PASSWORD)->valuestring));
            }
            if(strlen(cJSON_GetObjectItem(cfng, JSON_BROKER_IP)->valuestring) < MAX_STR_SIZE)
            {
                strncpy(ConfigStruct->ip, cJSON_GetObjectItem(cfng, JSON_BROKER_IP)->valuestring,
                        strlen(cJSON_GetObjectItem(cfng, JSON_BROKER_IP)->valuestring));
            }
            if(strlen(cJSON_GetObjectItem(cfng, JSON_CLIENT_ID)->valuestring) < MAX_STR_SIZE)
            {
                strncpy(ConfigStruct->id, cJSON_GetObjectItem(cfng, JSON_CLIENT_ID)->valuestring,
                        strlen(cJSON_GetObjectItem(cfng, JSON_CLIENT_ID)->valuestring));
            }
            ConfigStruct->port = cJSON_GetObjectItem(cfng, JSON_PORT)->valueint;
            if(strlen(cJSON_GetObjectItem(cfng, JSON_TOPIC_LED)->valuestring) < MAX_STR_SIZE)
            {
                strncpy(ConfigStruct->topic_del, cJSON_GetObjectItem(cfng, JSON_TOPIC_LED)->valuestring,
                        strlen(cJSON_GetObjectItem(cfng, JSON_TOPIC_LED)->valuestring));
            }
        }
        cJSON_Delete(config_json);
    }
    else
    {
        nvs_close(my_handle);
    }
}
/**
 * @fn void read_IP_value()
 *
 * @brief read json file to get to ip
 *
 */
void read_IP_value(char *static_ip, char *mask, char *gateway_ip)
{
    nvs_handle_t my_handle;
    uint8_t *data = NULL;
    uint16_t size = 0;
    nvs_open(STORAGE_PARTITION, NVS_READWRITE, &my_handle);
    esp_err_t err = nvs_get_u16(my_handle, SIZE_ITEM, &size);
    if(err == ESP_OK)
    {
        data = (uint8_t *) malloc(size);
        nvs_get_str(my_handle, MQTT_CONFIG_STR_ITEM, (char *) data, (size_t *) &size);
        nvs_close(my_handle);
        cJSON *config_json = cJSON_Parse((const char *) data);
        if(config_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            if(error_ptr != NULL)
            {
                ESP_LOGI(TAG, "Error before: %s\n", error_ptr);
            }
        }
        else
        {
            cJSON *cfng = cJSON_GetObjectItem(config_json, JSON_ARGS);
            if(strlen(cJSON_GetObjectItem(cfng, JSON_CLIENT_IP)->valuestring) < MAX_IP_SIZE)
            {
                strncpy(static_ip, cJSON_GetObjectItem(cfng, JSON_CLIENT_IP)->valuestring,
                        strlen(cJSON_GetObjectItem(cfng, JSON_CLIENT_IP)->valuestring));
            }
            // if(strlen(cJSON_GetObjectItem(cfng, JSON_GW_IP)->valuestring) < MAX_IP_SIZE)
            // {
            //     strncpy(gateway_ip, cJSON_GetObjectItem(cfng, JSON_GW_IP)->valuestring,
            //             strlen(cJSON_GetObjectItem(cfng, JSON_GW_IP)->valuestring));
            // }
            // if(strlen(cJSON_GetObjectItem(cfng, JSON_MASK)->valuestring) < MAX_IP_SIZE)
            // {
            //     strncpy(mask, cJSON_GetObjectItem(cfng, JSON_MASK)->valuestring,
            //             strlen(cJSON_GetObjectItem(cfng, JSON_MASK)->valuestring));
            // }
        }
        cJSON_Delete(config_json);
    }
    else
    {
        nvs_close(my_handle);
    }
}