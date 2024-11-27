#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

static const char* NTP_TAG = "NTP_SYNC";

static void obtain_time(void);
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(NTP_TAG, "Time synchronization event!");
}

void SyncTimeWithInternet(){
    char strftime_buf[64];

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    ESP_LOGI(NTP_TAG, "Time is not set, getting time over NTP.");
    obtain_time();
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(NTP_TAG, "The current date/time in New York is: %s", strftime_buf);
    time(&now);
}

void obtain_time(void){
    // ESP_ERROR_CHECK( nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK( esp_event_loop_create_default());
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    esp_netif_sntp_start();
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(NTP_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(NTP_TAG,"TIME SYNCED CORRECTLY: %lld",now);
    esp_netif_sntp_deinit();
}
void ntp_setup(){
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG();
    config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
    config.sync_cb = time_sync_notification_cb;
    esp_netif_sntp_init(&config);
}