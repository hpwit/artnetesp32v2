## artnetesp32v2 new version of the artnetesp32 library
Here is a new take on the artnet library the code is still in its infancy. The inspiration is the new aSyncUDP and the new udp_bind technique available in the new espressif core.

The code is quite not that clean yet but I can drive 35 unvierses at 40 fps with less than 1% loss. The tests doen with my testing tool seems to show 80 universes @30+ fps is not an issue either

This version philosphy is a bit different than the ArtnetESP32 library. 
There is a concept of subArtnets which are the recipients of the universes . The main class is a listener that will 'forward' the universe to subArtnet.
OK maybe it's not that clear let's get into it.

I suggest you read [Readme.me](https://github.com/hpwit/artnetESP32/blob/master/README.md) for consideration about using artnet

## Prerequisites
Here is what I use in my tests

* If you want to use artnet to display leds, this library requires the usage of [I2SClocklessLedDriver library](https://github.com/hpwit/I2SClocklessLedDriver) or [I2SClocklessVirtualLedDriver library](https://github.com/hpwit/I2SClocklessVirtualLedDriver) the latest library is in the case if you use the virtual pins for best results. I haven't fully tested it with FastLED
* Compiled with arduino with 2.0.6 core library
* IDE: Arduino IDE
* Board choosen DevModule or DEVKIT V1 (esp32) not (ESP32 Arduino) because it does not link the netif library
* I've made a small c program (sorry no python lol)  in `sendartnet` directory to test the code `udpsend.c`. This program sends artnet formatted universes at the speed that you desire.
  - to compile : `gcc udpsend.c -o udpsend`
  - usage *udpsend delay_between_universes_in_microseconds fps nb_of_universes ip_address* : `udpsend 100 40 35 192.168.0.11`
  - this will send 35 universes @ 40fps to the ip address 192.168.0.11 with 100us(0.1ms) between universes
  - do not forget to activate the debug level to 'Info'

## Other stuff
   * If you want to get some stat please activate the info debug level 'Tools>Core Debug Level>>Info'
   * do not hesitate to contact me if any question and please let me know about your builds with my libraires
   * The code is not super clean yet so please be indulgent
   * I have tried to test as much as I can with my panel (5904 leds, 35 universes)
      - [With Arena @40 fps](https://www.youtube.com/watch?v=CmE4naL7m_8)  <1% loss
      - [With Glediator @32 fps](https://youtu.be/BMQr672pBgQ) <1% loss
      - Madmapper 60fps <1% loss
   * Have some fun !!


## Declaration

 ```C
#define NUMBER_OF_LEDS 5000
#define NB_LEDS_PER_UNIVERSE 170
#define START_UNIVERSE 0
#include "artnetesp32v2.h"

 artnetESP32V2 artnet=artnetESP32V2();
 ```

Nothing really special :)

#### to start to listen to artnet packet `bool artnet.listen(int port)`
```C
#define NUMBER_OF_LEDS 5000
#define NB_LEDS_PER_UNIVERSE 170
#define START_UNIVERSE 0
#include "artnetesp32v2.h"

 artnetESP32V2 artnet=artnetESP32V2();

 ...
if(artnet.listen(6454)) {
   Serial.print("artnet Listening\n");
}
 ```

### let's start the fun
Now that we listen to artnet universes we need to store the information somewhere :)

![ss](/images/shcema.001.jpeg)

We need to define a reciever for theses artnet universes : a subArtnet (maybe not the best name  :) )

#### subArnet creation :`addSubArtnet(int star_universe,uint32_t size,uint32_t nb_data_per_universe,void (*fptr)(subArtnet *subartnet))`
* `star_universe` : the first universe to be recieved
* `size` : the size in bytes of what you want to get. For RGB leds this will be `NUM_LEDS * 3`
* `nb_data_per_universe` : the size of on universe in bytes (or channels) for most softwares this is 510 (170 * 3) but there is a brazilian software that uses all 512 bytes (channels) of the artnet universes
* `callback` function : this is the function that will be called once all the universes of one subartnet have been gathered. the callback function as the subartnet as input and should be declared like this 'void callback(subArtnet *subartnet)

#### main example with only one subartnet to gather all the universes 
This is what most people will use to display artnet animations

```C

#define NUM_LEDS_PER_STRIP 369
#define NUMSTRIPS 16
#define NB_CHANNEL_PER_LED 3 //Should be 4 if your sending tool is sending RGBW
#define UNIVERSE_SIZE_IN_CHANNEL  (170 * 3)  //here we define a universe of 170 pixels each pixel is composed of 3 channels
#define START_UNIVERSE 0
 artnetESP32V2 artnet=artnetESP32V2();

 void displayfunction(void *param){
      subArtnet *subartnet = (subArtnet *)param;
     driver.showPixels(NO_WAIT,subartnet->data);
} 

 ....

artnet.addSubArtnet(START_UNIVERSE ,NUM_LEDS_PER_STRIP * NUMSTRIPS * NB_CHANNEL_PER_LED,UNIVERSE_SIZE_IN_CHANNEL ,&displayfunction);

 ```

Please see the example `artnetdisplay.ino` which will be what most of you will do

#### What happens if two frames arrives to fast
  I have notice that even at 30 fps (1 frame every 33ms) and a refresh rate of my build of 90fps(11ms per frame) sometimes two frames arrive in less than that delay. In that case the `showPixels` will wait the the current frame to be displayed before showing the next one

#### You can declare several subartnet
What the use of this ? Well if you want to do some artnet=>DMX and if you're not sending everytime all the universes
if you  do this
 ```C
    // to transmit 600bytes , 2 universes of 510 bytes are needed
    //this subartnet will trigger when universes 0 and 1 will be recieved
    artnet.addSubArtnet(0 ,600,UNIVERSE_SIZE_IN_CHANNEL,&callbackfunction );  
    
    //to transmit 3500 bytes , 7 universes aof 510 bytes re needed
    //this subartnet will trigger when universes 2,3,4,5,6,7,8  will be recieved as we start with universe 2
     artnet.addSubArtnet(2,3500,UNIVERSE_SIZE_IN_CHANNEL,&callbackfunction );
 
    //to transmit 2500 bytes , 5 universes of 510 bytes are needed
    //this subartnet will trigger when universes 20,21,22,23,24 will be recieved as we start with universe 20
     artnet.addSubArtnet(20,2500,UNIVERSE_SIZE_IN_CHANNEL,&callbackfunction_2);

    //NB:  every universe not in those  subarnet will not be used
  ```

See the example `subArtnet.ino`

#### Artnet pool
You can define the name of the artnet node by using this function
```C
artnetESP32V2 artnet=artnetESP32V2();

...

artnet.setNodeName("name of the node");
```

### Optimization in case of only one subartnet
If you are using only one sub artnet you can add this to optimize the execution
```C
#define UNIQUE_SUBARTNET
#include "artnetESP32V2.h"
```



### What about ethernet ??
Several persons want to use ethernet, the library works with ethernet. A user has managed to get good results limiting to 10Mbps the esp32

### How can you help
 * Please do try this library try break it!!
 * Let me know your results !! I am always happy to hear back and see what have been done using my code
