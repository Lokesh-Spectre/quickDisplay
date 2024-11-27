#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include <esp_mac.h>
#include <time.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

static const char *MQTT_TAG = "mqtt";

char* MqttTopics[5];
void (*MqttHandles[5])(char* topic, char* payload) ={0}; 
int MqttHandles_index = 0;

void getMAC(char mac_str[18]){
    unsigned char mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(mac_str,"%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void setTimeFromStr(char* timestamp_str){
    int64_t timestamp;
    sscanf(timestamp_str,"%lld",&timestamp);
    const struct timeval tv = {
        .tv_sec = timestamp,
        .tv_usec = 0
    };
    settimeofday(&tv,NULL);
    printf("TIME UPDATE ARRIVED: %lld\n",timestamp);
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void MqttAddHandle(char* topic,void (*handle)(char*a,char*b)){
    MqttTopics[MqttHandles_index] = topic;
    MqttHandles[MqttHandles_index++] = handle;
}

static void subscribeToTopics(esp_mqtt_client_handle_t client){
    int n=MqttHandles_index;
    for(int i=0;i<n;i++){
        esp_mqtt_client_subscribe(client,MqttTopics[i],0);
    }
}
static void handle_message(esp_mqtt_event_handle_t event){
    // load topic
    char* topic = (char*)malloc(event->topic_len+1);
    memcpy(topic,event->topic,event->topic_len);
    topic[event->topic_len] = '\0';

    // load payload
    char* payload = (char*)malloc(event->data_len+1);
    memcpy(payload,event->data,event->data_len);
    payload[event->data_len] = '\0';

    long long int now;
    
    if(strcmp(topic,"/cosmicForge/time") == 0){
        time(&now);
        printf("BENCH MARK, startup time: %lld\n", now);
        setTimeFromStr(payload);
    }else{
        int n = MqttHandles_index;
        for(int i=0;i<n;i++){
            if(strcmp(MqttTopics[i],topic) ==0){
                MqttHandles[i](topic,payload);
            }
        }
    }
    free(topic);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        long long int now;
        time(&now);
        printf("MQTT CONNECTION: %lld",now);
        char mac[18];
        getMAC(mac);
        msg_id = esp_mqtt_client_subscribe(client, "/cosmicForge/time", 0);
        msg_id = esp_mqtt_client_publish(client, "/cosmicForge/Welcome", mac, 0, 1, 0);
        subscribeToTopics(client);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        handle_message(event);

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(MQTT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_mqtt_client_handle_t mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.0.107:1883",
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    return client;
}