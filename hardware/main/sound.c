#include <stdio.h>
#include "driver/gpio.h"
#include "driver/dac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
TaskHandle_t soundTask;

struct sound_config_t{
    bool alarm;
    // bool silent;
    int* tone;
    int tone_len;
    int speed;
};
struct sound_config_t sound_config;
void soundSetup(){
    int tone[] = {0,255,0};
    int n=sizeof(tone)/sizeof(int);
    dac_output_enable(DAC_CHAN_0);
    sound_config.alarm = false;
    // sound_config.silent = false;
    sound_config.tone =(int*)malloc(sizeof(int)*3);
    memcpy(sound_config.tone,tone,n);
    sound_config.tone_len = n;
    sound_config.speed =500;
}
void playSound(){
    // dac_output_enable(DAC_CHANNEL_2);
    int* tone =sound_config.tone;// ,150,200,150,200,150,255,0};
    int n=sound_config.tone_len;
    int speed= sound_config.speed;
    int start,end;
    while (true){
        for(int ti =1;ti<n;ti++){
            start = tone[ti-1];
            end = tone[ti];
            for(int s = start; s < end; start<end? s++ : s--){
                vTaskDelay(pdMS_TO_TICKS(1000/speed));
                dac_output_voltage(DAC_CHAN_0,s);
            }
        }
    }
}

void startSound(){
    if(sound_config.alarm){
        vTaskDelete(soundTask);
    }
    soundTask = NULL;
    xTaskCreate(playSound,"Alarm sound",4096,NULL,5,&soundTask);
    configASSERT(soundTask);
    sound_config.alarm = true;
}

void stopSound(void){
    if(sound_config.alarm){
        vTaskDelete(soundTask);
        sound_config.alarm = false;
    }
    dac_output_voltage(DAC_CHAN_0,0);
}