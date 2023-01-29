
#define NUM_LEDS_PER_STRIP 369
#define NUMSTRIPS 16
#define NB_CHANNEL_PER_LED 3 //Should be 4 if your sending tool is sending RGBW
#define COLOR_GRB
#define UNIVERSE_SIZE_IN_CHANNEL  (170 * 3)  //here we define a universe of 170 pixels each pixel is composed of 3 channels
#define START_UNIVERSE 0
//#define USE_PIXELSLIB
#include "I2SClocklessLedDriver.h"
#include "Arduino.h"
#include "WiFi.h"
#include "artnetESP32V2.h"
//here we have 3 colors per pixel


const char * ssid = "****";
const char * password = "***";

int pins[16]={2,4,5,12,13,14,15,17,16,19,21,22,23,25,26};

artnetESP32V2 artnet=artnetESP32V2();
I2SClocklessLedDriver driver;
  void displayfunction(subArtnet *subartnet){

     driver.showPixels(NO_WAIT,subartnet->data);
}

void setup() {
  // put your setup code here, to run once:
Serial.begin(2000000);


    /* if you do not use the driver for other thing than artnet you do not
     * need to put a led buffer. If you need one you can use it. In this example It's not needed
     * as the subArtnet contains the buffer to be displayed.
     */
   driver.initled(NULL,pins,NUMSTRIPS,NUM_LEDS_PER_STRIP);  
    driver.setBrightness(20);
       
 WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while(1) {
            delay(1000);
        }
    }

    /* We add the subartnet reciever. Here we only need one
     * addSubartnet (Start universe,size of data,number of channel per universe,callback function)
     * NUM_LEDS_PER_STRIP * NUMSTRIPS is the total number of leds 
     * NUM_LEDS_PER_STRIP * NUMSTRIPS * NB_CHANNEL_PER_LED is the total number of channels needed here 3 channel per led (R,G,B)
     */ 
    artnet.addSubArtnet(START_UNIVERSE ,NUM_LEDS_PER_STRIP * NUMSTRIPS * NB_CHANNEL_PER_LED,UNIVERSE_SIZE_IN_CHANNEL ,&displayfunction);
    
     
    if(artnet.listen(6454)) {
        Serial.print("artnet Listening on IP: ");
        Serial.println(WiFi.localIP());
       
    }
}

void loop()
{
  vTaskDelete(NULL);
}
