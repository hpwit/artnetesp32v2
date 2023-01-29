#define UNIVERSE_SIZE_IN_CHANNEL  (170 * 3)  //here we define a universe of 170 pixels each pixel is composed of 3 channels hence 510 bytes

#include "Arduino.h"
#include "WiFi.h"
#include "artnetESP32V2.h"



const char * ssid = "DomainePutterie1";
const char * password = "Jeremyyves1";


artnetESP32V2 artnet=artnetESP32V2();

  void callbackfunction(subArtnet * subartnet){
    Serial.printf("call back of subartnet  nd:%d size:%d\n",subartnet->subArtnetNum,subartnet->len);
    //let's do something wih the data

    }

     void callbackfunction_2(subArtnet * subartnet){
    Serial.printf("call back of other subartnet Artnumber:%d\n",subartnet->subArtnetNum);
    //let's do something wih the data

    }

void setup() {
  // put your setup code here, to run once:
Serial.begin(2000000);


 WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while(1) {
            delay(1000);
        }
    }

    /* We add the subartnet recievers. 
     * addSubartnet (Start universe,size in channel,number of channel per universe,calklback function)
     */ 
    // to transmit 600bytes , 2 universes of 510 bytes are needed
    //this subartnet will trigger when universes 0 and 1 will be recieved
    artnet.addSubArtnet(0 ,600,UNIVERSE_SIZE_IN_CHANNEL,&callbackfunction );  
    
    //to transmit 3500 bytes , 7 universes aof 510 bytes re needed
    //this subartnet will trigger when universes 2,3,4,5,6,7,8  will be recieved as we start with universe 2
     artnet.addSubArtnet(2,3500,UNIVERSE_SIZE_IN_CHANNEL,&callbackfunction );
 
    //to transmit 2500 bytes , 5 universes of 510 bytes are needed
    //this subartnet will trigger when universes 20,21,22,23,24 will be recieved as we start with universe 20
     artnet.addSubArtnet(20,2500,UNIVERSE_SIZE_IN_CHANNEL,&callbackfunction_2);

    //NB: that every universe not in those will not be used
    
     
    if(artnet.listen(6454)) {
        Serial.print("artnet Listening on IP: ");
        Serial.println(WiFi.localIP());
       
    }
}

void loop()
{
  vTaskDelete(NULL);
}
