
#define NUM_LEDS_PER_STRIP 369
#define NUMSTRIPS 16
#define USE_FASTLED //to enable the link between Pixel type and CRGB type if you need it and to increase compatibility
#define COLOR_GRB
//#define USE_PIXELSLIB
#include "I2SClocklessLedDriver.h"
#include "Arduino.h"
#include "WiFi.h"
#include "artnetESP32V2.h"
//here we have 3 colors per pixel
uint8_t leds[NUMSTRIPS*NUM_LEDS_PER_STRIP*3];

const char * ssid = "****";
const char * password = "***";

int pins[16]={2,4,5,12,13,14,15,17,16,19,21,22,23,25,26};

artnetESP32V2 artnet=artnetESP32V2();
I2SClocklessLedDriver driver;
  void displayfunction(){

     driver.showPixels(NO_WAIT,artnet.getframe());
}

void setup() {
  // put your setup code here, to run once:
Serial.begin(2000000);
udp.beginByLed(NUM_LEDS_PER_STRIP*NUMSTRIPS,170,0); //here we declare NUM_LEDS_PER_STRIP*NUMSTRIPS led with artnet universe size of 170 leds

//the following line is the same
//udp.beginByChannel(NUM_LEDS_PER_STRIP*NUMSTRIPS*3,510,0); here we declare NUM_LEDS_PER_STRIP*NUMSTRIPS*3 channels (because RBG is coded 3 channels) with artnet universe size of 510 channels

//if your artnet program uses all the artnet size
//udp.beginByChannel(NUM_LEDS_PER_STRIP*NUMSTRIPS*3,512,0)

     // leds=alleds.getPixels();
   driver.initled(leds,pins,NUMSTRIPS,NUM_LEDS_PER_STRIP);
    driver.setBrightness(20);
       
 WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while(1) {
            delay(1000);
        }
    }
      artnet.setFrameCallback(&displayfunction);
     
    if(artnet.listen(6454)) {
        Serial.print("artnet Listening on IP: ");
        Serial.println(WiFi.localIP());
       
    }
}

void loop()
{
  vTaskDelete(NULL);
}
