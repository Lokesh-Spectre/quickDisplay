#include "driver/gpio.h"
#include "esp_log.h"
#include "font.c"
#include "freertos/FreeRTOS.h"
#include <string.h>


#define DEFAULT_fast_boot true
#define DEFAULT_displayCount 1

const char* MAX_TAG = "MAX7219";
TaskHandle_t scrollTextTask_handle;

typedef struct Pins{
    int cs;
    int clk;
    int data;
};

typedef struct MAX7219_config_t{
    struct Pins pins;
    int displayCount;
};

typedef struct scroll_params_t{
    char* text;
    int speed;
    int left;
};
struct MAX7219_config_t MAX7219_config;
struct scroll_params_t scroll_params;
void write_word(short int word){
    bool clk=false; // clk inactive state
    for(int i=15;i>=0;i--){
        gpio_set_level(MAX7219_config.pins.data,(word>>i) & 1);
        gpio_set_level(MAX7219_config.pins.clk,!clk);
        gpio_set_level(MAX7219_config.pins.clk,clk);
    }
}

void send_cmd(int command){
    gpio_set_level(MAX7219_config.pins.cs,false); // pull down CS to start transmission
    write_word(command);
    gpio_set_level(MAX7219_config.pins.cs,true);
}

void setBrightness(int level){
    if(level >15){
        level =15;
    }
    if(level <0){
        level =0;
    }
    level += 0x0A00;
    gpio_set_level(MAX7219_config.pins.cs,false);
    for(int i=0;i<MAX7219_config.displayCount;i++){
        write_word(level);
    }
    gpio_set_level(MAX7219_config.pins.cs,true);
}

void clearDisplay(){
    for(int i=1;i<9;i++){
        send_cmd(0x0100 + (i<<8));
    }
    for(int i=0;i<MAX7219_config.displayCount;i++){
        send_cmd(0x0000);
    }
}

void setUpDisplay(int cs_pin,int clk_pin,int data_pin,int displayCount,bool fast_boot){
    MAX7219_config.pins.cs = cs_pin;
    MAX7219_config.pins.clk = clk_pin;
    MAX7219_config.pins.data = data_pin;
    MAX7219_config.displayCount = displayCount;

    // scroll_params = (struct scroll_params_t*) malloc(sizeof(struct scroll_params_t));
    if(fast_boot){
        gpio_reset_pin(cs_pin);
        gpio_reset_pin(clk_pin);
        gpio_reset_pin(data_pin);
    }
    gpio_set_direction(cs_pin,GPIO_MODE_OUTPUT);
    gpio_set_direction(clk_pin,GPIO_MODE_OUTPUT);
    gpio_set_direction(data_pin,GPIO_MODE_OUTPUT);

    int commands[]={
        0x0A00, // set brightness to minimum
        0x0C01, // normal operation mode/out of shutdown mode
        0x0F00, // normal operation mode/out of testing mode
        0x0900, // set Decode mode to no decode for D0-D7
        0x0B07, // set scan limit to 7 to unlock all 8 rows in a single display
    };

    int n=sizeof(commands)/sizeof(commands[0]);
    for(int i=0;i<n;i++){
        send_cmd(commands[i]);
    }
    ESP_LOGI(MAX_TAG,"Display initialized and ready to use......");
}

void updateDisplay(int* buffer){
    int n = MAX7219_config.displayCount * 8;
    short int address;
    short int data;
    for (int r =0;r<8;r++){
        gpio_set_level(MAX7219_config.pins.cs,false);
		for (int c=n-1;c>=0;c--){
            address = (r+1)<<8;
            data = (buffer[r] >> c*8) & 0xFF;
			write_word(address+data);
		}
        gpio_set_level(MAX7219_config.pins.cs,true);
	}
}

// text feature ++++++++++
void addChar2S(char ch, int buff[8]){
    int x = (int) ch;
    for(int i=0;i<8;i++){
        buff[i] = (buff[i]<< 8) + font[x][i]; 
    }
}

void displayStaticText(char* text){
    int n=strlen(text);
    int buff[8];
    for (int i=0;i<n;i++){
        addChar2S(text[i],buff);
    }
    updateDisplay(buff);
}

void shiftDisplay(int screen[8],int buff[8],int left){
    // +ve for right
    // -ve for left
    for(int i=0;i<8;i++){
        screen[i] <<=1;
        screen[i] += (buff[i]>>7) & 1;
        buff[i]<<=1;
    }
}
void displayScrollText(struct scroll_params_t* params){
    // speed is the frequency of display shift
    char* text = params->text; 
    int speed = params->speed;
    int left = params->left;

    int timeDelay= (1000/speed)/portTICK_PERIOD_MS;
    int n = strlen(text);
    int screen[8] = {0};
    int tmpBuff[8];
    int x=MAX7219_config.displayCount;

    while (true){
        for(int i=0;i<n;i++){
            addChar2S(text[i],tmpBuff);
            for(int j=0;j<8;j++){
                vTaskDelay(pdMS_TO_TICKS(1000/speed));
                shiftDisplay(screen,tmpBuff,left);
                updateDisplay(screen);
            }
        }
        
        for(int i=0;i<x*8;i++){
            vTaskDelay(pdMS_TO_TICKS(1000/speed));
            shiftDisplay(screen,tmpBuff,left);
            updateDisplay(screen);
        }
    }    
    
}

void startScrollText(char* text,int speed,int left){
    scrollTextTask_handle = NULL;
    scroll_params.text = text;
    scroll_params.speed = speed;
    scroll_params.left = left;
    xTaskCreate(displayScrollText,"SCROLL TEXT",4096,(void*)&scroll_params,5,&scrollTextTask_handle);
}
void stopScrollText(){
    vTaskDelete(scrollTextTask_handle);
}
// text feature ------------
