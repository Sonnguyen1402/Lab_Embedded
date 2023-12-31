#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

// AP config
#define CONFIG_AP_SSID "Embedded System"
#define CONFIG_AP_PASSWORD "12345678"
#define CONFIG_AP_CHANNEL 1
#define CONFIG_AP_MAX_STA_CON 6

// STA config
#define CONFIG_STA_SSID "LAB 6"
#define CONFIG_STA_PASSWORD "12345678"

static const char *TAG = "ESP32 LAB6";

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

static void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;
    if (initialized)
    {
        return;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    initialized = true;
}

static bool wifi_apsta(int timeout_ms)
{
    // AP config
    wifi_config_t ap_config = {0};
    strcpy((char *)ap_config.ap.ssid, CONFIG_AP_SSID);
    strcpy((char *)ap_config.ap.password, CONFIG_AP_PASSWORD);
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    ap_config.ap.ssid_len = strlen(CONFIG_AP_SSID);
    ap_config.ap.channel = CONFIG_AP_CHANNEL;
    ap_config.ap.max_connection = CONFIG_AP_MAX_STA_CON;

    if (strlen(CONFIG_AP_PASSWORD) == 0)
    {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    // STA config
    wifi_config_t sta_config = {0};
    strcpy((char *)sta_config.sta.ssid, CONFIG_STA_SSID);
    strcpy((char *)sta_config.sta.password, CONFIG_STA_PASSWORD);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WIFI_MODE_AP started. SSID:%s password:%s channel:%d",
             CONFIG_AP_SSID, CONFIG_AP_PASSWORD, CONFIG_AP_CHANNEL);

    ESP_ERROR_CHECK(esp_wifi_connect());
    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                   pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
    if (bits)
    {
        ESP_LOGI(TAG, "WIFI_MODE_STA connected. SSID:%s password:%s",
                 CONFIG_STA_SSID, CONFIG_STA_PASSWORD);
    }
    else
    {
        ESP_LOGI(TAG, "WIFI_MODE_STA can't connected. SSID:%s password:%s",
                 CONFIG_STA_SSID, CONFIG_STA_PASSWORD);
    }
    return (bits & CONNECTED_BIT) != 0;
}

void app_main()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    initialise_wifi();
    ESP_LOGW(TAG, "Start APSTA Mode");
    wifi_apsta(5 * 1000);
}
