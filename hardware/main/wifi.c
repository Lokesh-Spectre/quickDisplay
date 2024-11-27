#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static EventGroupHandle_t wifi_event_group;

#define MAX_FAILURES 5
#define WIFI_FAILURE BIT0
#define WIFI_SUCCESS BIT1

static const char *WIFI_TAG = "WIFI LOG";
static int s_retry_num=0;
static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data){
    
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        ESP_LOGI(WIFI_TAG, "Connecting to AP....");
        esp_wifi_connect();
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED){
        ESP_LOGI(WIFI_TAG, "Connected to AP....");

    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if(s_retry_num < MAX_FAILURES){
            ESP_LOGI(WIFI_TAG, "\nSTA disconnected\nreconnecting....\n");
            esp_wifi_connect();
            s_retry_num++;
        }else{
            xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG,"STA IP: " IPSTR,IP2STR(&event->ip_info.ip));
        s_retry_num=0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);

    }
}
int connect_wifi(char* ssid, char* passwd){
   int status = WIFI_FAILURE;

   // initialize esp network interface driver
   ESP_ERROR_CHECK(esp_netif_init()); 

   // initialize default esp event loop
   ESP_ERROR_CHECK(esp_event_loop_create_default());

   esp_netif_create_default_wifi_sta();

   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

   wifi_event_group = xEventGroupCreate();

    // register and connect event handlers to functions
   esp_event_handler_instance_t wifi_handler_event_instance;
   ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    &wifi_event_handler,
    NULL,
    &wifi_handler_event_instance
   ));

   esp_event_handler_instance_t got_ip_handler_event_instance;
   ESP_ERROR_CHECK(esp_event_handler_instance_register(
    IP_EVENT,
    IP_EVENT_STA_GOT_IP,
    &ip_event_handler,
    NULL,
    &got_ip_handler_event_instance
   ));
   
   wifi_config_t wifi_config = {
    .sta={
        .ssid="ACTFIBERNET",
        .password="act12345",
        .pmf_cfg={
            .capable = true,
            .required = false
        },
    },
   };
   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
   ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
   ESP_ERROR_CHECK(esp_wifi_start());
   ESP_LOGI(WIFI_TAG,"wifi init finished.");
   EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
    WIFI_SUCCESS | WIFI_FAILURE,
    pdFALSE,
    pdFALSE,
    portMAX_DELAY);

    if(bits & WIFI_SUCCESS){
        ESP_LOGI(WIFI_TAG,"Connected to AP!!");
        status= WIFI_SUCCESS;
    }else if(bits & WIFI_FAILURE){
        ESP_LOGI(WIFI_TAG,"Failed to connect");
        status = WIFI_FAILURE;
    }else{
        ESP_LOGE(WIFI_TAG,"UNEXPECTED EVENT");
        status = WIFI_FAILURE;
    }
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP, got_ip_handler_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    vEventGroupDelete(wifi_event_group);

    return status;
}

// void app_main(void){
//     int status = WIFI_FAILURE;

//     esp_err_t ret = nvs_flash_init();
//     if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     status = connect_wifi();
// }