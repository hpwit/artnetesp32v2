#include "Arduino.h"
#include "artnetESP32V2.h"
#include "esp_log.h"

extern "C" {
#include "lwip/opt.h"
#include "lwip/inet.h"
#include "lwip/udp.h"
#include "lwip/igmp.h"
#include "lwip/ip_addr.h"
#include "lwip/mld6.h"
#include "lwip/prot/ethernet.h"
#include <esp_err.h>
#include <esp_wifi.h>
}

#include "lwip/priv/tcpip_priv.h"


#define BUFFER_SIZE 10
#define ART_DMX_START 18
#ifndef NB_FRAMES_DELTA
#define NB_FRAMES_DELTA 100
#endif

#define UDP_MUTEX_LOCK()    //xSemaphoreTake(_lock, portMAX_DELAY)
#define UDP_MUTEX_UNLOCK()  //xSemaphoreGive(_lock)
typedef struct {
    struct tcpip_api_call_data call;
    udp_pcb * pcb;
    const ip_addr_t *addr;
    uint16_t port;
    struct pbuf *pb;
    struct netif *netif;
    err_t err;
} udp_api_call_t;




static err_t _udp_connect_api(struct tcpip_api_call_data *api_call_msg){
    udp_api_call_t * msg = (udp_api_call_t *)api_call_msg;
    msg->err = udp_connect(msg->pcb, msg->addr, msg->port);
    return msg->err;
}

static err_t _udp_connect(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port){
    udp_api_call_t msg;
    msg.pcb = pcb;
    msg.addr = addr;
    msg.port = port;
    tcpip_api_call(_udp_connect_api, (struct tcpip_api_call_data*)&msg);
    return msg.err;
}

static err_t _udp_disconnect_api(struct tcpip_api_call_data *api_call_msg){
    udp_api_call_t * msg = (udp_api_call_t *)api_call_msg;
    msg->err = 0;
    udp_disconnect(msg->pcb);
    return msg->err;
}

static void  _udp_disconnect(struct udp_pcb *pcb){
    udp_api_call_t msg;
    msg.pcb = pcb;
    tcpip_api_call(_udp_disconnect_api, (struct tcpip_api_call_data*)&msg);
}

static err_t _udp_remove_api(struct tcpip_api_call_data *api_call_msg){
    udp_api_call_t * msg = (udp_api_call_t *)api_call_msg;
    msg->err = 0;
    udp_remove(msg->pcb);
    return msg->err;
}

static void  _udp_remove(struct udp_pcb *pcb){
    udp_api_call_t msg;
    msg.pcb = pcb;
    tcpip_api_call(_udp_remove_api, (struct tcpip_api_call_data*)&msg);
}

static err_t _udp_bind_api(struct tcpip_api_call_data *api_call_msg){
    udp_api_call_t * msg = (udp_api_call_t *)api_call_msg;
    msg->err = udp_bind(msg->pcb, msg->addr, msg->port);
    return msg->err;
}

static err_t _udp_bind(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port){
    udp_api_call_t msg;
    msg.pcb = pcb;
    msg.addr = addr;
    msg.port = port;
    tcpip_api_call(_udp_bind_api, (struct tcpip_api_call_data*)&msg);
    return msg.err;
}


typedef struct {
       // void *arg;
        //udp_pcb *pcb;
        pbuf *pb;
        int universe;
        //const ip_addr_t *addr;
        //uint16_t port;
        //struct netif * netif;
} lwip_event_packet_t;

static xQueueHandle _udp_queue;
static volatile TaskHandle_t _udp_task_handle = NULL;

static void _udp_task(void *pvParameters){
    lwip_event_packet_t * e = NULL;
   // uint8_t u[2000];

    int currenbt_uni,nb_frames_lost,previous_uni,Nb_universe,nb_frames,startUniverse,lastUniverse;
    bool enterint_uni=false;
    uint32_t decal,decal2;
    long time1,time2;
    uint8_t *offset, *offset2;
    artnetESP32V2 * artnet=((artnetESP32V2 *)pvParameters);
    artnet->nbframeslost=0;
    artnet->nbframes=0;
 int nb_zero=0;
Nb_universe=artnet->nbNeededUniverses;
startUniverse=artnet->startuniverse;
lastUniverse=artnet->enduniverse;
     decal =  artnet->nbPixelsPerUniverse  + ART_DMX_START;
     decal2 =  artnet->nbNeededUniverses * decal;
     offset=artnet->buffers[0];
     bool new_frame=false;
     bool frame_disp=false;

    previous_uni=999;
    for (;;) {
        if(xQueueReceive(_udp_queue, &e, portMAX_DELAY) == pdTRUE){
            if(!e->pb){
                free((void*)(e));
                continue;
            }

            

             // Serial.printf("%d\n",e->universe);
//#define TOTO

#ifdef TOTO            
              //  Serial.printf("size %d\n",e->pb->len);
              currenbt_uni =e->universe;
             // Serial.printf("%d %d %d\n",currenbt_uni,((artnetESP32V2 *)pvParameters)->currentframenumber ,(((artnetESP32V2 *)pvParameters)->nbPixelsPerUniverse));
              // offset2 = ((artnetESP32V2 *)pvParameters)->artnetleds1 +  ((artnetESP32V2 *)pvParameters)->buffers[]+currenbt_uni*(((artnetESP32V2 *)pvParameters)->nbPixelsPerUniverse) * 3;
             // #ifndef FULL_LENGTTH_ARTNET
               offset2=offset+currenbt_uni*(artnet->nbPixelsPerUniverse);
                
            
              //Serial.printf("%d \n",currenbt_uni);
              if(currenbt_uni==Nb_universe-1 and enterint_uni==true)
              {
                //artnet->nbframes++;
                nb_frames++;
               // Serial.printf("%d\n",currenbt_uni);
                enterint_uni=false;
                //Serial.printf("ee\n");
                 memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,artnet->nbPixelsPerUniverse);
                 artnet->currentframe=offset;
                 (*((artnetESP32V2 *)pvParameters)->frameCallback)();
                artnet->currentframenumber= (artnet->currentframenumber+1)%2;
                offset=artnet->buffers[ artnet->currentframenumber];
               if((nb_frames)%100==0)
                {
                    //time2=millis();
                    //Serial.printf("frame %d lost %d %.2f in %.2fs or %.2f fps\n",nb_frames,nb_frames_lost,(float)(100*nb_frames_lost)/(nb_frames_lost+nb_frames),(float)(time2-time1)/1000,(float)(1000*1000/(time2-time1)));
                    //Serial.printf("frame %d lost %d %.2f \n",artnet->nbframes,artnet->nbframeslost,(float)(100*artnet->nbframeslost)/(artnet->nbframeslost+artnet->nbframes));
                    Serial.printf("frame %d lost %d %.2f nbzeros:%d\n",nb_frames,nb_frames_lost,(float)(100* nb_frames_lost)/(nb_frames_lost+nb_frames),nb_zero);
                   // time1=time2;
                }
                //enterint_uni==false;

              }
              else
              {
                if( (currenbt_uni>0) and (currenbt_uni!=previous_uni+1))
                {
                  if(previous_uni!=9999)
               // {
                  nb_frames_lost++;
                  enterint_uni=false;
                //}
                //else
                                
                  previous_uni=9999;
                

                } else
                {
                     memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,artnet->nbPixelsPerUniverse);
                  previous_uni=currenbt_uni;
                  if( currenbt_uni==0)
                  {
                    nb_zero++;
                     enterint_uni=true;
                    //Serial.printf("new frame 0 "); 
                     //previous_uni=0;
                  }
                  
                }
              }
              

#else
 ESP_LOGI("ARTNETESP32","%d",e->universe);
        currenbt_uni =e->universe;
       
        if(currenbt_uni <Nb_universe)
        {
            if( currenbt_uni == startUniverse)
            {
                
               
                if(new_frame==false)
                {
                    nb_frames_lost++;
                   //  Serial.printf(" bad previous universe nbzeros:%d:%d:%d\n",nb_zero,nb_frames,nb_frames_lost);
                }else
                {
                    if(frame_disp==false)
                    {
                        nb_frames_lost++;
                       // Serial.printf(" bad previous universe end frame nbzeros:%d:%d:%d\n",nb_zero,nb_frames,nb_frames_lost);
                    }
                }
                // nb_zero++;
                // Serial.printf("new frame nbzeors:%d :%d:%d delta:%d\n",nb_zero,nb_frames,nb_frames_lost,nb_frames+nb_frames_lost-nb_zero);
                new_frame= true;
                
               
            frame_disp=false;
                previous_uni =startUniverse;
                 offset2=offset;
                 memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,artnet->nbPixelsPerUniverse);
            }
            else
            {
            
                if(currenbt_uni==previous_uni+1)
                {
                   
                   
                    if(new_frame)
                    {
                        previous_uni=currenbt_uni;
                        offset2=offset+(currenbt_uni-startUniverse)*(artnet->nbPixelsPerUniverse);
                         memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,artnet->nbPixelsPerUniverse);
                        
                        if(currenbt_uni==lastUniverse-1)
                        {
                            nb_frames++;
                       // Serial.printf("Got frame %d:%d:%d\n",nb_zero,nb_frames,nb_frames_lost);
                        frame_disp=true;
                           // new_frame=false;
                            //Serial.printf("ee\n");
                        //  memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,artnet->nbPixelsPerUniverse);
                            artnet->currentframe=offset;
                            (*((artnetESP32V2 *)pvParameters)->frameCallback)();
                            artnet->currentframenumber= (artnet->currentframenumber+1)%2;
                            offset=artnet->buffers[ artnet->currentframenumber];
                        if((nb_frames)%NB_FRAMES_DELTA==0)
                            {
                                time2=millis();
                                //Serial.printf("frame %d lost %d %.2f in %.2fs or %.2f fps\n",nb_frames,nb_frames_lost,(float)(100*nb_frames_lost)/(nb_frames_lost+nb_frames),(float)(time2-time1)/1000,(float)(1000*1000/(time2-time1)));
                                //Serial.printf("frame %d lost %d %.2f \n",artnet->nbframes,artnet->nbframeslost,(float)(100*artnet->nbframeslost)/(artnet->nbframeslost+artnet->nbframes));
                                 ESP_LOGI("ARTNETESP32","frames fully received:%d frames lost:%d  percentage lost:%.2f  fps: %.2f",nb_frames,nb_frames_lost-1,(float)(100* (nb_frames_lost-1))/(nb_frames_lost+nb_frames-1),(float)(1000*NB_FRAMES_DELTA/(time2-time1)));
                             time1=time2;
                            }
                        }
                    }
        
                }
                else
                {
                    //new_frame=false;
                    //previous_uni=255;        
                    
                    
                    if(new_frame)
                        {
                           // Serial.printf("got worng %d %d \n",currenbt_uni,previous_uni);
                          // nb_frames_lost++;
                           new_frame=false;
                            
                        }

                }
                 
            }
        }
        else
        {
            Serial.printf("Universe %d out of bound",currenbt_uni);
        }
#endif
        //Serial.printf("%d\n",*((uint8_t *)(e->pb->payload+1)));
            //AsyncUDP::_s_recv(e->arg, e->pcb, e->pb, e->addr, e->port, e->netif);
             pbuf_free(e->pb);
            free((void*)(e));
        }
    }
    _udp_task_handle = NULL;
    vTaskDelete(NULL);
}




static void _udp_task_subrarnet(void *pvParameters){
    lwip_event_packet_t * e = NULL;
    ESP_LOGI("ARTNETESP32","Start listening task");
artnetESP32V2 * artnet=((artnetESP32V2 *)pvParameters);
 for(int subArnetIndex=0;subArnetIndex<artnet->numSubArtnet;subArnetIndex++)
   {
      subArtnet *sub_artnet= artnet->subArtnets[subArnetIndex];
      //Serial.printf("%ld\n", artnet->subArtnets[subArnetIndex]);
       ESP_LOGI("ARTNETESP32","subArtnet nr:%d start universe:%d Nb Universes: %d",sub_artnet->subArtnetNum,  (*sub_artnet).startUniverse,sub_artnet->nbNeededUniverses);
   }
 for (;;) {
        if(xQueueReceive(_udp_queue, &e, portMAX_DELAY) == pdTRUE){
            if(!e->pb){
                free((void*)(e));
                continue;
            }
    /*
    * the following part find the right subartnet
    */
   //Serial.printf("un iver %d\n",e->universe);
   for(int subArnetIndex=0;subArnetIndex<artnet->numSubArtnet;subArnetIndex++)
   {
      subArtnet *sub_artnet= (artnet->subArtnets)[subArnetIndex];
     //Serial.printf(" %d %d %d %d \n",subArnetIndex,(*sub_artnet).startUniverse,sub_artnet->startUniverse-1,e->universe);
      if( sub_artnet->startUniverse<=e->universe and (sub_artnet->endUniverse-1)>=e->universe )
      {
          //we've found the surbartnet sending info to him
         // xSemaphoreGive(subartnet->subArtnet_sem);
          sub_artnet->handleUniverse(e->universe,(uint8_t *)e->pb->payload,e->pb->len);
          continue;
      }
   }

            pbuf_free(e->pb);
            free((void*)(e));
        }
 }
     _udp_task_handle = NULL;
    vTaskDelete(NULL);
}




static bool _udp_task_start(artnetESP32V2 *p){
    if(!_udp_queue){
        _udp_queue = xQueueCreate(128, sizeof(lwip_event_packet_t *));
        if(!_udp_queue){
            return false;
        }
    }
    if(!_udp_task_handle){
        //xTaskCreateUniversal(_udp_task, "async_udp", 4096, p, CONFIG_ARDUINO_UDP_TASK_PRIORITY, (TaskHandle_t*)&_udp_task_handle, 0);
        xTaskCreateUniversal(_udp_task_subrarnet, "async_udp", 4096, p, CONFIG_ARDUINO_UDP_TASK_PRIORITY, (TaskHandle_t*)&_udp_task_handle, 0);
        
        if(!_udp_task_handle){
            return false;
        }
    }
    return true;
}

//static bool _udp_task_post(void *arg, udp_pcb *pcb, pbuf *pb, const ip_addr_t *addr, uint16_t port, struct netif *netif)
static bool _udp_task_post( pbuf *pb)
{
    if(!_udp_task_handle || !_udp_queue){
        return false;
    }
    lwip_event_packet_t * e = (lwip_event_packet_t *)malloc(sizeof(lwip_event_packet_t));
    if(!e){
        return false;
    }
   // e->arg = arg;
    //e->pcb = pcb;
    e->pb = pb;
    e->universe=*((uint8_t *)(e->pb->payload)+14);
    //e->addr = addr;
    //e->port = port;
    //e->netif = netif;
     //Serial.printf("un ivsddser %d\n",e->universe);
    if (xQueueSend(_udp_queue, &e, portMAX_DELAY) != pdPASS) {
        free((void*)(e));
        return false;
    }
    return true;
}

static void _udp_recv(void *arg, udp_pcb *pcb, pbuf *pb, const ip_addr_t *addr, uint16_t port)
{
    while(pb != NULL) {
        pbuf * this_pb = pb;
        pb = pb->next;
        this_pb->next = NULL;
        
        artnetESP32V2 * artnet=((artnetESP32V2 *)arg);
        /*
        if(*((uint8_t *)(this_pb->payload)+14)>=artnet->nbNeededUniverses)
        {
            //log_e("universe %d out of bound",((uint8_t *)(this_pb->payload)+14);
        ESP_LOGE("ARTNETESP32", "universe %d out of bound",*((uint8_t *)(this_pb->payload)+14));
             pbuf_free(this_pb);
             return;
        }*/
      // Serial.printf(":%d\n", *((uint8_t *)(this_pb->payload+1) ));
       // if(!_udp_task_post(arg, pcb, this_pb, addr, port, ip_current_input_netif())){
if(!_udp_task_post(this_pb)){
            pbuf_free(this_pb);
        }
    }
}



static void _udp_task_subrarnet_handle(void *pvParameters)
{
  subArtnet * subartnet=(subArtnet *)pvParameters;
  for(;;)
  {
    xSemaphoreTake(subartnet->subArtnet_sem, portMAX_DELAY);
   // subartnet->frameCallback(subartnet->subArtnetNum, subartnet->currentframe,subartnet->len[subartnet->currentframenumber]);
    subartnet->frameCallback(subartnet);
  // subartnet->frameCallback(subartnet->currentframe);
  }
}



  subArtnet::subArtnet(int star_universe,uint32_t nb_data,uint32_t nb_data_per_universe)
  {
    _initialize(star_universe,nb_data,nb_data_per_universe);
  }

  void subArtnet::_initialize(int star_universe,uint32_t nb_data,uint32_t nb_data_per_universe)
  {
    nb_frames=0;
    nb_frames_lost=0;
    previous_lost=0;
    previousUniverse=0;
     bool new_frame=false;
    
nbDataPerUniverse=nb_data_per_universe;
    startUniverse=star_universe;
     nbNeededUniverses = nb_data/ nbDataPerUniverse;
    if (nbNeededUniverses * nbDataPerUniverse < nb_data )
    {

        nbNeededUniverses++;
    }
    len=nb_data;
      //nbNeededUniverses = nb_universes;
        endUniverse=startUniverse+nbNeededUniverses;
    
    ESP_LOGI("ARTNETESP32","Initialize subArtnet Start Universe: %d  end Universe: %d, universes %d",startUniverse,endUniverse-1,  nbNeededUniverses);
   buffers[0] = (uint8_t *)calloc((nbDataPerUniverse ) * nbNeededUniverses  + 8 + BUFFER_SIZE,1);
   //buffers[0] = (uint8_t *)calloc((510 + ART_DMX_START) * 73  + 8 + BUFFER_SIZE,1);
    if ( buffers[0] == NULL)
    {
        Serial.printf("impossible to create the buffer 1\n");
        return;
    }
    memset(buffers[0],0,(nbDataPerUniverse ) * nbNeededUniverses  + 8 + BUFFER_SIZE);
    buffers[1] = (uint8_t *)calloc((nbDataPerUniverse  ) * nbNeededUniverses  + 8 + BUFFER_SIZE,1);
   // buffers[1] = (uint8_t *)calloc((510 + ART_DMX_START) * 73  + 8 + BUFFER_SIZE,1);
    if (buffers[1] == NULL)
    {
        Serial.printf("impossible to create the buffer 2\n");
        return;
    }
    memset(buffers[1],0,(nbDataPerUniverse ) * nbNeededUniverses  + 8 + BUFFER_SIZE);
   // Serial.printf("Starting SubArtnet nbNee sdedUniverses:%d\n", nbNeededUniverses);
    currentframenumber=0;
    offset=buffers[0];
    new_frame=false;
    subArtnet_sem=xSemaphoreCreateBinary();
     xTaskCreateUniversal(_udp_task_subrarnet_handle, "async_udp", 4096, this, 3, NULL, 0);

  }
subArtnet:: ~subArtnet()
{
    if(buffers[0]!=NULL)
        free(buffers[0]);
     if(buffers[1]!=NULL)
        free(buffers[1]);

}
void subArtnet::handleUniverse(int currenbt_uni,uint8_t *payload,size_t length)
 {
     // Serial.printf("uni:%d %d\n",currenbt_uni,subArtnetNum);
            if( currenbt_uni == startUniverse)
            {
                
               tmp_len=0;
                if(new_frame==false)
                {
                    nb_frames_lost++;
                   //  Serial.printf(" bad previous universe nbzeros:%d:%d:%d\n",nb_zero,nb_frames,nb_frames_lost);
                }
                /*else
                {
                    if(frame_disp==false)
                    {
                        nb_frames_lost++;
                       // Serial.printf(" bad previous universe end frame nbzeros:%d:%d:%d\n",nb_zero,nb_frames,nb_frames_lost);
                    }
                }
                // nb_zero++;
                // Serial.printf("new frame nbzeors:%d :%d:%d delta:%d\n",nb_zero,nb_frames,nb_frames_lost,nb_frames+nb_frames_lost-nb_zero);
                new_frame= true;
                */
                
               
            new_frame=true;
                previousUniverse =startUniverse;
                // offset2=offset;
                memcpy(offset,payload+ART_DMX_START,nbDataPerUniverse);
                 tmp_len=(length-ART_DMX_START<nbDataPerUniverse)?(length-ART_DMX_START):nbDataPerUniverse;
                 if(startUniverse==endUniverse-1)
                 {
                  nb_frames++;
                  // new_frame=true;
                  data=offset;
                  // len=tmp_len;
                         if(frameCallback)
                          {
                           xSemaphoreGive(subArtnet_sem);
                              //frameCallback(subArtnetNum, offset);
                              //frameCallback(this);
                          }
                           currentframenumber= (currentframenumber+1)%2;
                            offset=buffers[ currentframenumber];
                        if((nb_frames)%NB_FRAMES_DELTA==0)
                            {
                                time2=millis();
                                //Serial.printf("frame %d lost %d %.2f in %.2fs or %.2f fps\n",nb_frames,nb_frames_lost,(float)(100*nb_frames_lost)/(nb_frames_lost+nb_frames),(float)(time2-time1)/1000,(float)(1000*1000/(time2-time1)));
                                //Serial.printf("frame %d lost %d %.2f \n",artnet->nbframes,artnet->nbframeslost,(float)(100*artnet->nbframeslost)/(artnet->nbframeslost+artnet->nbframes));
                                 ESP_LOGI("ARTNETESP32","SUBARTNET:%d frames fully received:%d frames lost:%d  delta:%d percentage lost:%.2f  fps: %.2f",subArtnetNum, nb_frames,nb_frames_lost-1, nb_frames_lost-previous_lost,(float)(100* (nb_frames_lost-1))/(nb_frames_lost+nb_frames-1),(float)(1000*NB_FRAMES_DELTA/(time2-time1)));
                             time1=time2;
                             previous_lost=nb_frames_lost;
                            }
                 }
            }
            else
            {
            
                if(currenbt_uni==previousUniverse+1)
                {
                   
                    //Serial.printf("uni %d\n",currenbt_uni);
                    if(new_frame)
                    {
                        previousUniverse=currenbt_uni;
                        //offset2=offset+(currenbt_uni-startUniverse)*(nbDataPerUniverse);
                         //memcpy(offset2,payload+ART_DMX_START,nbDataPerUniverse);
                         memcpy(offset+tmp_len,payload+ART_DMX_START,nbDataPerUniverse);
                         //Serial.printf("%d %d\n",currenbt_uni,(length-ART_DMX_START<nbDataPerUniverse)?(length-ART_DMX_START):nbDataPerUniverse);
                        tmp_len+=(length-ART_DMX_START<nbDataPerUniverse)?(length-ART_DMX_START):nbDataPerUniverse;
                        if(currenbt_uni==endUniverse-1)
                        {
                            nb_frames++;
                       // Serial.printf("Got frame %d:%d:%d\n",nb_zero,nb_frames,nb_frames_lost);
                       // new_frame=true;
                           // new_frame=false;
                            //Serial.printf("ee\n");
                        //  memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,artnet->nbPixelsPerUniverse);
                           data=offset;
                          // len=tmp_len;
                          if(frameCallback)
                          {
                            xSemaphoreGive(subArtnet_sem);
                              //frameCallback(subArtnetNum, offset);
                              // frameCallback(this);
                          }
                           currentframenumber= (currentframenumber+1)%2;
                            offset=buffers[ currentframenumber];
                        if((nb_frames)%NB_FRAMES_DELTA==0)
                            {
                                time2=millis();
                                //Serial.printf("frame %d lost %d %.2f in %.2fs or %.2f fps\n",nb_frames,nb_frames_lost,(float)(100*nb_frames_lost)/(nb_frames_lost+nb_frames),(float)(time2-time1)/1000,(float)(1000*1000/(time2-time1)));
                                //Serial.printf("frame %d lost %d %.2f \n",artnet->nbframes,artnet->nbframeslost,(float)(100*artnet->nbframeslost)/(artnet->nbframeslost+artnet->nbframes));
                                 ESP_LOGI("ARTNETESP32","SUBARTNET:%d frames fully received:%d frames lost:%d  delta:%d percentage lost:%.2f  fps: %.2f",subArtnetNum, nb_frames,nb_frames_lost-1, nb_frames_lost-previous_lost,(float)(100* (nb_frames_lost-1))/(nb_frames_lost+nb_frames-1),(float)(1000*NB_FRAMES_DELTA/(time2-time1)));
                             time1=time2;
                             previous_lost=nb_frames_lost;
                            }
                        }
                    }
        
                }
                else
                {
                    if(currenbt_uni<=previousUniverse)
                            nb_frames_lost++;
                    new_frame=false;
                    //previous_uni=255;        
                    
                    /*
                    if(new_frame)
                        {
                           // Serial.printf("got worng %d %d \n",currenbt_uni,previous_uni);
                          // nb_frames_lost++;
                           new_frame=false;
                            
                        }
                        */

                }
              }
 }

   uint8_t * subArtnet::getData()
    {
        return data;
    }
    /*
    void subArtnet::setFrameCallback(  void (*fptr)(uint8_t * data))
    {
         frameCallback = fptr;
    }

    */

    void subArtnet::setFrameCallback(  void (*fptr)(subArtnet   * subartnet))
    {
         frameCallback = fptr;
    }
    
   /*
     void subArtnet::setFrameCallback(  void (*fptr)(int num,uint8_t * data,size_t len))
    {
      frameCallback = fptr;
    } */
/*

void artnetESP32V2::beginByChannel(uint32_t nbchannels, uint32_t nbchannelsperuniverses,int startunivers)
 {
  _pcb = NULL;
    _connected = false;
	_lastErr = ERR_OK;
     currentframenumber=0;
   // _handler = NULL;
     startuniverse=startunivers;
       nbPixels = nbchannels;
    nbPixelsPerUniverse = nbchannelsperuniverses;
    nbNeededUniverses = nbchannels / nbPixelsPerUniverse;
     ESP_LOGI("ARTNETESP32","universes %d",nbNeededUniverses);
    if (nbNeededUniverses * nbPixelsPerUniverse <nbchannels )
    {

        nbNeededUniverses++;
    }
    enduniverse=startunivers+nbNeededUniverses;
      
     ESP_LOGI("ARTNETESP32","universes %d %d \n",nbchannels,  nbNeededUniverses);
    //artnetleds1= (uint8_t *)malloc(nbpixels*buffernumber*3);
    buffers[0] = (uint8_t *)malloc((nbchannelsperuniverses  + ART_DMX_START) * nbNeededUniverses   + BUFFER_SIZE);
    if ( buffers[0] == NULL)
    {
        Serial.printf("impossible to create the buffer 1\n");
        return;
    }
    buffers[1] = (uint8_t *)malloc((nbchannelsperuniverses  + ART_DMX_START) * nbNeededUniverses   + BUFFER_SIZE);
    if (buffers[1] == NULL)
    {
        Serial.printf("impossible to create the buffer 2\n");
        return;
    }
    Serial.printf("Starting Artnet nbNee sdedUniverses:%d\n", nbNeededUniverses);
    currentframenumber=0;
 }

void artnetESP32V2::beginByLed(uint16_t nbpixels, uint32_t nbpixelsperuniverses,int startunivers)
{
      _pcb = NULL;
    _connected = false;
	_lastErr = ERR_OK;
     currentframenumber=0;
   // _handler = NULL;
     startuniverse=startunivers;
       nbPixels = nbpixels;
    nbPixelsPerUniverse = nbpixelsperuniverses*3;
    nbNeededUniverses = (nbPixels *3) / nbPixelsPerUniverse;
    if (nbNeededUniverses * nbPixelsPerUniverse < nbPixels*3 )
    {

        nbNeededUniverses++;
    }
        enduniverse=startunivers+nbNeededUniverses;
    //artnetleds1= (uint8_t *)malloc(nbpixels*buffernumber*3);
    ESP_LOGI("ARTNETESP32","universes %d \n",nbNeededUniverses);
    buffers[0] = (uint8_t *)malloc((nbpixelsperuniverses * 3 + ART_DMX_START) * nbNeededUniverses  + 8 + BUFFER_SIZE);
    if ( buffers[0] == NULL)
    {
        Serial.printf("impossible to create the buffer 1\n");
        return;
    }
    buffers[1] = (uint8_t *)malloc((nbpixelsperuniverses * 3 + ART_DMX_START) * nbNeededUniverses  + 8 + BUFFER_SIZE);
    if (buffers[1] == NULL)
    {
        Serial.printf("impossible to create the buffer 2\n");
        return;
    }
    Serial.printf("Starting Artnet nbNee sdedUniverses:%d\n", nbNeededUniverses);
    currentframenumber=0;
}*/

artnetESP32V2::artnetESP32V2()
{
    _pcb = NULL;
    _connected = false;
	_lastErr = ERR_OK;
}

bool artnetESP32V2::_init(){
    if(_pcb){
        return true;
    }
    _pcb = udp_new();
    if(!_pcb){
        return false;
    }
    //_lock = xSemaphoreCreateMutex();
    udp_recv(_pcb, &_udp_recv, (void *) this);
    return true;
}







artnetESP32V2::~artnetESP32V2()
{
    close();
    UDP_MUTEX_LOCK();
    udp_recv(_pcb, NULL, NULL);
    _udp_remove(_pcb);
    _pcb = NULL;
    UDP_MUTEX_UNLOCK();
    //vSemaphoreDelete(_lock);
}


void artnetESP32V2::close()
{
    UDP_MUTEX_LOCK();
    if(_pcb != NULL) {
        if(_connected) {
            _udp_disconnect(_pcb);
        }
        _connected = false;
        //todo: unjoin multicast group
    }
    UDP_MUTEX_UNLOCK();
}


bool artnetESP32V2::connect(const ip_addr_t *addr, uint16_t port)
{
    if(!_udp_task_start(this)){
        log_e("failed to start task");
        return false;
    }
    if(!_init()) {
        return false;
    }
    close();
    UDP_MUTEX_LOCK();
    _lastErr = _udp_connect(_pcb, addr, port);
    if(_lastErr != ERR_OK) {
        UDP_MUTEX_UNLOCK();
        return false;
    }
    _connected = true;
    UDP_MUTEX_UNLOCK();
    return true;
}


bool artnetESP32V2::connect(const IPAddress addr, uint16_t port)
{
    ip_addr_t daddr;
    daddr.type = IPADDR_TYPE_V4;
    daddr.u_addr.ip4.addr = addr;
    return connect(&daddr, port);
}



bool artnetESP32V2::connect(const IPv6Address addr, uint16_t port)
{
    ip_addr_t daddr;
    daddr.type = IPADDR_TYPE_V6;
    memcpy((uint8_t*)(daddr.u_addr.ip6.addr), (const uint8_t*)addr, 16);
    return connect(&daddr, port);
}


bool artnetESP32V2::listen(const ip_addr_t *addr, uint16_t port)
{
    if(!_udp_task_start(this)){
        log_e("failed to start task");
        return false;
    }
    if(!_init()) {
        return false;
    }
    close();
    if(addr){
        IP_SET_TYPE_VAL(_pcb->local_ip,  addr->type);
        IP_SET_TYPE_VAL(_pcb->remote_ip, addr->type);
    }
    UDP_MUTEX_LOCK();
    if(_udp_bind(_pcb, addr, port) != ERR_OK) {
        UDP_MUTEX_UNLOCK();
        return false;
    }
    _connected = true;
    UDP_MUTEX_UNLOCK();
    return true;
}

bool artnetESP32V2::listen(uint16_t port)
{
    return listen(IP_ANY_TYPE, port);
}

bool artnetESP32V2::listen(const IPAddress addr, uint16_t port)
{
    ip_addr_t laddr;
    laddr.type = IPADDR_TYPE_V4;
    laddr.u_addr.ip4.addr = addr;
    return listen(&laddr, port);
}
bool artnetESP32V2::listen(const IPv6Address addr, uint16_t port)
{
    ip_addr_t laddr;
    laddr.type = IPADDR_TYPE_V6;
    memcpy((uint8_t*)(laddr.u_addr.ip6.addr), (const uint8_t*)addr, 16);
    return listen(&laddr, port);
}
artnetESP32V2::operator bool()
{
    return true;
}
