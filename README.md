# artnetesp32v2 new version of the artnet library
Here is a new take on the artnet library the code is still in its infancy

The code is quite not that clean yet but I can drive 35 unvierses at 30+ fps with a 1% loss.


## prerequisites
This library requires the usage of [https://github.com/hpwit/I2SClocklessLedDriver] or [https://github.com/hpwit/I2SClocklessVirtualLedDriver] the latest library is in the case if you use the virtual pins.

## Declaration

 ```C
#define NUMBER_OF_LEDS 5000
#define NB_LEDS_PER_UNIVERSE 170
#define START_UNIVERSE 0
#include "artnetesp32v2.h"

 artnetESP32V2 artnet=artnetESP32V2();
 ```

Nothing really special :)

## to start to listen to artnet packet ` void beginByLed(uint16_t nbpixels, uint16_t nbpixelsperuniverses,int startunivers);`
 Start the artnet by declaring the number of leds needed and the size per universe  
```C
#define NUMBER_OF_LEDS 5000
#define NB_LEDS_PER_UNIVERSE 170
#define START_UNIVERSE 0
#include "artnetesp32v2.h"

 artnetESP32V2 artnet=artnetESP32V2();

 ...
artnet.beginByLed(NUMBER_OF_LEDS * 3,NB_CHANNEL_PER_UNIVERSE,START_UNIVERSE);
 ```

## to start to listen to artnet packet   `void beginByChannelbeginByChannel(uint16_t nbchannels, uint16_t nbchannelsperuniverses,int startunivers);`
some artnet software do not send X leds per universe but the full 512 channels. Hence a led is split between two universes. to deal with that:
 ```C
#define NUMBER_OF_LEDS 5000
#define NB_CHANNEL_PER_UNIVERSE 512
#define START_UNIVERSE 0

#include "artnetesp32v2.h"

 artnetESP32V2 artnet=artnetESP32V2() 

 ....

 artnet.beginByChannel(NUMBER_OF_LEDS * 3,NB_CHANNEL_PER_UNIVERSE,START_UNIVERSE);
 ```
## to start to listen to artnet packet `listen(int port_number)`

 ```C
 artnetESP32V2 artnet=artnetESP32V2()
 
 ...

artnet.beginByLed(NUMBER_OF_LEDS,NB_LEDS_PER_UNIVERSE,START_UNIVERSE);
 if(artnet.listen(6454)) {
        Serial.print("Arnet Listening ....\n");
 }
  ```

## The callback function `void setFrameCallback(void (*fptr)())`

## to display your leds we do not wait ...
In the callback function you can display the leds. the `NO_WAIT` option part of the Clockless driver (virtual pins or not), will allow you to display the led without being a blocking process
 ```C
I2SClocklessLedDriver driver;
  void displayfunction(){

     driver.showPixels(NO_WAIT,artnet.getframe());
}
 ```
 ### What happens if two frames arrives to fast
  I have notice that even at 30 fps (1 frame every 33ms) and a refresh rate of my build of 90fps(11ms per frame) sometimes two frames arrive in less than that delay. In that case the `showPixels` will wait the the current frame to be displayed before showing the next one


## you have one example
for the moment one example with the ClocklessLedDriver.h to use the virtual pin driver just the declaration of your led driver changes all the rest stay the same

### ethernet
Several persons want to use ethernet, the library works with ethernet. A user has managed to get good results limiting to 10Mbps the esp32

## How can you help
Please do try this library try break it and let mn know I am always intersted when my code is used
