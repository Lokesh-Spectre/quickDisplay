#include <stdio.h>
#include "esp_log.h"
#include "MAX7219.c"
#include "digital_clock.c"
#include "wifi.c"
#include "mqtt.c"
#include "esp_wifi.h"
#include "sound.c"
#include "mqtt_client.h"

// pins
#define DIN 12
#define CS 14
#define CLK 27

#define NO_OF_MODULES 4
#define TIMEZONE_OFFSET 5*3600+30*60

// wifi config
#define SSID "ACTFIBERNET"
#define PASSWD "act12345"

const char* TAG = "APP";
esp_mqtt_client_handle_t mqtt_client;

struct app_config_t{
    char* mode;
};
struct app_config_t app_config;

/*
/config:
    {
        light:<int> 0 - 16
    }
*/

void mqtt_config_handle(char* topic, char* payload){
    cJSON* json = cJSON_Parse(payload);
    int brightness = cJSON_GetObjectItem(json,"light")->valueint;
    char* mode  = cJSON_GetObjectItem(json,"mode")->valuestring;
    if(strcmp(mode,app_config.mode)!=0){
        if(strcmp(mode,"clock") == 0){
            stopScrollText();
            clock_resume();
        }else if(strcmp(app_config.mode,"clock")==0){
            clock_pause();
            clearDisplay();
        }
        app_config.mode = mode;
    }
    
    setBrightness(brightness);
}
void mqtt_text_handle(char* topic,char* payload){
    cJSON* json = cJSON_Parse(payload);

    char* text = cJSON_GetObjectItem(json,"text")->valuestring;
    int speed = cJSON_GetObjectItem(json,"speed")->valueint;
    int left = cJSON_GetObjectItem(json,"left")->valueint;
    if(strcmp("scroll_text",app_config.mode) == 0){
        stopScrollText();
    }else if (strcmp("clock",app_config.mode) == 0){
        clock_pause();
        clearDisplay();
    }
    app_config.mode = "scroll_text";
    startScrollText(text,speed,left);
    // vTaskDelay(pdMS_TO_TICKS(5000));
    // stopScrollText();
}
/*
{
    sound:0|1
}
*/
void mqtt_alarm_handle(char* topic, char* payload){
    cJSON* json = cJSON_Parse(payload);
    int sound = cJSON_GetObjectItem(json,"sound")->valueint;
    if(sound == 1){
        startSound();
    }else{
        stopSound();
    }
}
void mqtt_setup(){
    // register message handlers for mqtt
    MqttAddHandle("/cosmicForge/config",mqtt_config_handle);
    MqttAddHandle("/cosmicForge/text",mqtt_text_handle);
    MqttAddHandle("/cosmicForge/alarm",mqtt_alarm_handle);
}
void app(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    int status = connect_wifi(SSID,PASSWD); // connect to wifi
    long long int now;
    time(&now);
    printf("WIFI CONNECTION: %lld",now);

    mqtt_client = mqtt_app_start();
    mqtt_setup();
    if(status != WIFI_SUCCESS){
        ESP_LOGI(TAG,"CANNOT CONNECT TO WIFI, entering CLOCK MODE WITH STORED TIME");
    }
    setUpDisplay(CS,CLK,DIN,NO_OF_MODULES,true);
    clearDisplay();
    clock_setUp(updateDisplay,false);
    soundSetup();
    app_config.mode = "clock";

    // xTaskCreate(sound_test, "SOUND TEST", 4096,NULL,1, NULL);

}
void test_main(void){
    setUpDisplay(CS,CLK,DIN,NO_OF_MODULES,true);
    clearDisplay();
    struct scroll_params_t params = {
        .text = "HOLA BOIS, THIS IS ANOTHER MESSAGE",
        .speed = 50,
        .left = 0
    };
    displayScrollText(&params);
}
void app_main(void)
{
    bool TEST_MODE = false;
    if(TEST_MODE){
        test_main();
    }else{
        app();
    }
}
/*
MQTT API
/Welcome - post a message when the micro controller is connected
/Time - used to sync time with the world, packets are emmited when a new controller is joined or if the backend just started or every 10 mins
/config - used to config the brightness.
/text -  used to set text to display.
    {
        text:"Something in here to be done",
        speed:10,
        left:1
    }
/alarm - used to start/stop alarm sound.
    {
        sound:true
    }
*/