/**
 * @file storage_module.c
 * Storage essential data for config via python script
 * @date 28.09.2022
 * Created on: 18 september 2022
 * @author Louis COLIN
 */
#include "config.h"
// #include "storage_module.h"
static const char *TAG = "ETHERNET EXAMPLE";
/**
 * @fn void set_static_ip()
 *
 * @brief set static ip for the pannel
 *
 */
void set_static_ip(esp_netif_t *eth_netif)
{
    char static_ip[MAX_IP_SIZE] = "";
    char gateway_ip[MAX_IP_SIZE] = "";
    char mask[MAX_IP_SIZE] = "";
    read_IP_value(static_ip, mask, gateway_ip);
    if(esp_netif_dhcpc_stop(eth_netif) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(static_ip);
    ip.netmask.addr = ipaddr_addr(ETHERNET_MASK_ADDR);
    ip.gw.addr = ipaddr_addr(ETHERNET_GW_ADDR);
    if(esp_netif_set_ip_info(eth_netif, &ip) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set ip info");
        return;
    }
}
/**
 * @fn void ethernet_init()
 *
 * @brief init the ethernet link
 *
 */
void ethernet_init()
{
    gpio_pad_select_gpio(PIN_PHY_POWER);
    gpio_set_direction(PIN_PHY_POWER, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_PHY_POWER, 1);
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Create new default instance of esp-netif for Ethernet
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    set_static_ip(eth_netif);
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    // phy_config.phy_addr = CONFIG_EXAMPLE_ETH_PHY_ADDR;
    // phy_config.reset_gpio_num = CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;
    // mac_config.smi_mdc_gpio_num = CONFIG_EXAMPLE_ETH_MDC_GPIO;
    // mac_config.smi_mdio_gpio_num = CONFIG_EXAMPLE_ETH_MDIO_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}