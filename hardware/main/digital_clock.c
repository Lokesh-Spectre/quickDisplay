#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "string.h"
#include <time.h>
int clock_screen_buffer[8];

#define TIMEZONE_OFFSET 5*3600+30*60

const unsigned char clock_font[][8]={
	{12,18,18,18,18,18,18,12},
	{12,4,4,4,4,4,4,4},
	{28,2,2,12,16,16,16,14},
	{28,2,2,28,2,2,2,28},
	{16,16,16,20,20,30,4,4},
	{30,16,16,28,2,2,2,28},
	{14,16,16,28,18,18,18,12},
	{30,2,2,2,2,2,2,2},
	{12,18,18,12,18,18,18,12},
	{12,18,18,14,2,2,2,12},
	{0,14,14,0,0,14,14,0}, // colon
	{0,254,254,254,254,16,124,0}, // monitor
	{7,3,5,32,113,251,255,255}, // mountains with sun 
	{24,60,126,255,66,70,66,126}, // house
	{146,84,56,254,56,84,146,0}, // sun
    {0,0,0,0,0,0,0,0} // blank
};

struct clock_config_t{
    unsigned char hours;
    unsigned char minutes;
    unsigned char seconds;
    bool military_time;
    void (*onChange)(int* buffer);
    bool clock_render;
} clock_config;

bool tickTock = true;

void setRenderer(void (*renderer)(int* buffer)){
    clock_config.onChange = renderer;
}
void clock_setTime(long long int timestamp,long long int off_set){
    clock_config.hours = ((timestamp+off_set)/3600)%24;
    clock_config.minutes = ((timestamp+off_set)/60)%60;
    clock_config.seconds = (timestamp+off_set)%60;
}
void clock_add2screen(int digit){
    int n;
	if(digit<10) n=5;
	else if(digit==10) n=4;
    else if(digit==15) n=4;
	else n=8;
	for(int x=0;x<8;x++){
		clock_screen_buffer[x] = (clock_screen_buffer[x]<<n)+clock_font[digit][x]; 
	}
}
void clock_render(){
    if(!clock_config.clock_render){
        return;
    }
    int hr = clock_config.hours;
    int min = clock_config.minutes;
    bool am = hr<=12;
    int emoji=13;

    // memset(clock_screen_buffer,0,sizeof(clock_screen_buffer));
	if(!clock_config.military_time){
        if(!am) hr%=12;
        emoji = am ? 12: 11;
    }
    clock_add2screen(hr/10);
	clock_add2screen(hr%10);
    clock_add2screen(tickTock?10:15);
	clock_add2screen(min/10);
	clock_add2screen(min%10);
	clock_add2screen(emoji);
    clock_config.onChange(clock_screen_buffer);
    tickTock = !tickTock;
}
void clock_timer_handle(){
    long long int now;
    time(&now);
    clock_setTime(now,TIMEZONE_OFFSET);
    // printf("TIME NOW: %lld  hrs:%d min:%d sec:%d\n",now,clock_config.hours,clock_config.minutes,clock_config.seconds);
    clock_render();

}

void clock_setUp(void (*renderer)(int* buffer) ,bool military_time){
    // clock_setTime(timestamp);
    clock_config.military_time = military_time;
    clock_config.onChange = renderer;
    clock_config.clock_render = true;
    memset(clock_screen_buffer,0,sizeof(clock_screen_buffer));
    TimerHandle_t clock_timer;

    clock_timer = xTimerCreate(
        "Clock Timer",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        clock_timer_handle
    );
    xTimerStart(clock_timer,0);
    clock_render();
}

void clock_pause(){
    clock_config.clock_render = false;
}
void clock_resume(){
    clock_config.clock_render = true;
}
