#ifndef ARTNETESP32_H
#define ARTNETESP32_H
#include "WiFi.h"
#include "IPAddress.h"
#include "IPv6Address.h"
#include "Print.h"
#include "Stream.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <functional>
   #include "string.h"
extern "C"
{
#include "esp_netif.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
}

#include "lwip/priv/tcpip_priv.h"

// CONFIG_UDP_RECVMBOX_SIZE=6
// CONFIG_LWIP_MAX_UDP_PCBS
// class artnetESP32V2;

struct udp_pcb;
struct pbuf;
struct netif;

typedef struct
{
    pbuf *pb;
    int universe;
} lwip_event_packet_t;
//struct lwip_event_packet_t;
#define ART_DMX_START 18
#define BUFFER_SIZE 512
#define MAX_SUBARTNET 20
#define NB_MAX_BUFFER 10
#ifndef  _USING_QUEUES
#define _USING_QUEUES 1
#endif
#ifndef SUBARTNET_CORE
#define SUBARTNET_CORE 0
#endif
#ifndef CALLBACK_CORE
#define CALLBACK_CORE 1
#endif
#define FRAME_END 1
#define FRAME_START 2
#ifndef _SYNC_FRAME_
#define _SYNC_FRAME_ FRAME_END
#endif

#ifndef NB_FRAMES_DELTA
#define NB_FRAMES_DELTA 100
#endif

static xQueueHandle _show_queue[MAX_SUBARTNET];
static xQueueHandle _udp_queue;
static volatile TaskHandle_t _udp_task_handle = NULL;
 
 
 static void _udp_task_subrarnet(void *pvParameters);
 static void _udp_task_subrarnet_handle(void *pvParameters);
 //static bool _udp_task_start(artnetESP32V2 *p);
class subArtnet
{
public:
  int startUniverse, endUniverse, nbDataPerUniverse, nbNeededUniverses, nb_frames_lost, nb_frames, previous_lost,nb_frame_double;
  uint8_t *buffers[10];
  uint8_t currentframenumber;
  uint8_t nbOfBuffers = 2;
  uint8_t nbBufferled;
  uint8_t *data;
  int previousUniverse;
  size_t len;
  size_t tmp_len;
  bool subartnetglag;
  //#ifdef ARTNET_CALLBACK_SUBARTNET
    void (*frameCallback)(void *params);
  //#else
  //void (*frameCallback)(uint8_t *data);
  //#endif
  bool new_frame = false;
  bool frame_disp = false;
  int subArtnetNum;
  uint32_t _nb_data;
  long time1, time2;
  uint8_t *offset, *offset2;
  uint8_t _callback_core= CALLBACK_CORE;

 
  volatile xSemaphoreHandle subArtnet_sem = NULL;
  subArtnet(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe);
  void createBuffers(uint8_t *leds);
  void _initialize(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe, uint8_t *leds);
  ~subArtnet();

void handleUniverse(lwip_event_packet_t *e)
{
    if(e->universe == startUniverse)
    {
        
        offset = buffers[currentframenumber];
                  #if  CORE_DEBUG_LEVEL>=2
        if (new_frame == false)
        {
            nb_frames_lost++;
        }
        #endif 
        new_frame = true;
        #if _SYNC_FRAME_ == FRAME_START
        if(frame_disp)
        {

                data =  buffers[currentframenumber];
        currentframenumber = (currentframenumber + 1) % nbOfBuffers;
        offset = buffers[currentframenumber];
            frame_disp= false;
            
             #ifdef  _USING_QUEUES

                xQueueSend(_show_queue[subArtnetNum], &data, portMAX_DELAY);
            
          #else 
          
            nb_frames++;
           if (frameCallback)
           {
            //if(subartnetglag)
                frameCallback((void *)this);
           // else
            //frameCallback((void*)data);

           }
               #if CORE_DEBUG_LEVEL>=2
                    if ((nb_frames) % NB_FRAMES_DELTA == 0)
                    {
                        time2 =millis();
                        ESP_LOGI("ARTNETESP32", "SUBARTNET:%d frames fully received:%d frames lost:%d  delta:%d percentage lost:%.2f  fps: %.2f ", subArtnetNum, nb_frames, nb_frames_lost - 1, nb_frames_lost - previous_lost, (float)(100 * (nb_frames_lost - 1)) / (nb_frames_lost + nb_frames - 1), (float)(1000 * NB_FRAMES_DELTA / ((time2 - time1) / 1)));
                        time1 = time2;
                        previous_lost = nb_frames_lost;
                    }
                #endif
            
            #endif
            
        }
        #endif
        previousUniverse = startUniverse - 1;
        
    }
    if(!new_frame)
    {
        return;
    }
    if (e->universe  == previousUniverse + 1)
    {
       
            previousUniverse++;
        memcpy(offset, e->pb->payload , e->pb->len);
        offset += e->pb->len;
            if (e->universe  == endUniverse - 1)
            {
                // nb_frames++;
                //memset(data+_nb_data+3,10,3);
                

                #if _SYNC_FRAME_ == FRAME_END
    
                data =  buffers[currentframenumber];
        currentframenumber = (currentframenumber + 1) % nbOfBuffers;
        offset = buffers[currentframenumber];
           // frame_disp= false;
            
             #ifdef  _USING_QUEUES

                xQueueSend(_show_queue[subArtnetNum], &data, portMAX_DELAY);
            
          #else 
          
            nb_frames++;
           if (frameCallback)
           {
            //if(subartnetglag)
                frameCallback((void *)this);
           // else
            //frameCallback((void*)data);

           }
               #if CORE_DEBUG_LEVEL>=2
                    if ((nb_frames) % NB_FRAMES_DELTA == 0)
                    {
                        time2 =millis();
                        ESP_LOGI("ARTNETESP32", "SUBARTNET:%d frames fully received:%d frames lost:%d  delta:%d percentage lost:%.2f  fps: %.2f ", subArtnetNum, nb_frames, nb_frames_lost - 1, nb_frames_lost - previous_lost, (float)(100 * (nb_frames_lost - 1)) / (nb_frames_lost + nb_frames - 1), (float)(1000 * NB_FRAMES_DELTA / ((time2 - time1) / 1)));
                        time1 = time2;
                        previous_lost = nb_frames_lost;
                    }
                #endif
            
            #endif
          
        
        #else
        frame_disp=true;
        #endif

            }
        
    }
    else
    {
        new_frame = false;
    }

}

  uint8_t *getData();
  
void setFrameCallback(  void (*fptr)(void * params))
  {

     frameCallback = fptr;
  }


   
   bool _using_queues;


};

// typedef subArtnet_h *subArtnet;

class artnetESP32V2 //: public Print
{
protected:
  udp_pcb *_pcb;
  // xSemaphoreHandle _lock;
  bool _connected;
  esp_err_t _lastErr;

  bool _init();
  // void _recv(udp_pcb *upcb, pbuf *pb, const ip_addr_t *addr, uint16_t port, struct netif * netif);

public:
  int num_universes, numSubArtnet = 0;
  uint8_t *artnetleds1, *buffer2;
  uint8_t *buffers[10];
  uint8_t currentframenumber;
  uint8_t nbOfBuffers = 2;
  uint8_t _udp_task_core = SUBARTNET_CORE;

  uint8_t *currentframe;
  uint32_t pixels_per_universe, nbPixels, nbPixelsPerUniverse, nbNeededUniverses, startuniverse, enduniverse, nbframes, nbframeslost;
  artnetESP32V2();
  ~artnetESP32V2();

  bool listen(const ip_addr_t *addr, uint16_t port);
  bool listen(const IPAddress addr, uint16_t port);
  bool listen(const IPv6Address addr, uint16_t port);
  bool listen(uint16_t port);
  bool connect(const ip_addr_t *addr, uint16_t port);
  bool connect(const IPAddress addr, uint16_t port);
  bool connect(const IPv6Address addr, uint16_t port);
  uint8_t *getframe()
  {
    return currentframe;
  }
  operator bool();
  void close();

  void (*frameCallback)();
  inline void setFrameCallback(void (*fptr)())
  {
    frameCallback = fptr;
  }

  bool addSubArtnet(subArtnet *subart)
  {
    #ifdef UNIQUE_SUBARTNET
    Serial.printf("delta %d \n",NB_FRAMES_DELTA );
    #else
    Serial.printf("jkjcvbcvbcvbcvkjk\n");
  #endif
    if (numSubArtnet < MAX_SUBARTNET)
    {
      subArtnets[numSubArtnet] = subart;
      subart->subArtnetNum = numSubArtnet;
      numSubArtnet++;
      subart->_callback_core= CALLBACK_CORE;
      #ifdef _USING_QUEUES
      subart->_using_queues = true;
      #else
      subart->_using_queues = false;
      #endif
      return true;
    }
    else
    {
      return false;
    }
  }
  /*// #ifndef ARTNET_CALLBACK_SUBARTNET
  subArtnet *addSubArtnet(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe, void (*fptr)(uint8_t *data))
  {
    return addSubArtnet(star_universe, nb_data, nb_data_per_universe, fptr, NULL);
  }
  subArtnet *addSubArtnet(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe, void (*fptr)(uint8_t *data), uint8_t *leds)
  {
    _udp_task_core = SUBARTNET_CORE;
    if (numSubArtnet < MAX_SUBARTNET)
    {
      subArtnets[numSubArtnet] = (subArtnet *)calloc(sizeof(subArtnet), 1);
      subArtnets[numSubArtnet]->_initialize(star_universe, nb_data, nb_data_per_universe, leds);
      subArtnets[numSubArtnet]->subArtnetNum = numSubArtnet;
      subArtnets[numSubArtnet]->frameCallback = fptr;
     subArtnets[numSubArtnet]->_callback_core= CALLBACK_CORE;
      #if _USING_QUEUES == 1
       subArtnets[numSubArtnet]->_using_queues = true;
      #else
       subArtnets[numSubArtnet]->_using_queues = false;
      #endif
subArtnets[numSubArtnet]->subartnetglag = false;

       numSubArtnet++;
      return subArtnets[numSubArtnet - 1];
    }
    else
    {
      return NULL;
    }
  }
#else*/
subArtnet *addSubArtnet(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe, void (*fptr)(void *params))
  {
    return addSubArtnet(star_universe, nb_data, nb_data_per_universe, fptr, NULL);
  }
  subArtnet *addSubArtnet(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe, void (*fptr)(void *params), uint8_t *leds)
  {
        
    _udp_task_core = SUBARTNET_CORE;
    if (numSubArtnet < MAX_SUBARTNET)
    {
      subArtnets[numSubArtnet] = (subArtnet *)calloc(sizeof(subArtnet), 1);
      subArtnets[numSubArtnet]->_initialize(star_universe, nb_data, nb_data_per_universe, leds);
      subArtnets[numSubArtnet]->subArtnetNum = numSubArtnet;
      subArtnets[numSubArtnet]->frameCallback = fptr;
     subArtnets[numSubArtnet]->_callback_core= CALLBACK_CORE;
      #if _USING_QUEUES == 1
       subArtnets[numSubArtnet]->_using_queues = true;
      #else
       subArtnets[numSubArtnet]->_using_queues = false;
      #endif
      subArtnets[numSubArtnet]->subartnetglag = true;
       numSubArtnet++;
      return subArtnets[numSubArtnet - 1];
    }
    else
    {
      return NULL;
    }
  }
//#endif
 
  void setNodeName(String s);

  subArtnet *subArtnets[MAX_SUBARTNET];
  // static void _s_recv(void *arg, udp_pcb *upcb, pbuf *p, const ip_addr_t *addr, uint16_t port, struct netif * netif);
};


 static void _udp_task_subrarnet(void *pvParameters)
{
    lwip_event_packet_t *e = NULL;
    ESP_LOGI("ARTNETESP32", "Start listening task");
    artnetESP32V2 *artnet = ((artnetESP32V2 *)pvParameters);
    for (int subArnetIndex = 0; subArnetIndex < artnet->numSubArtnet; subArnetIndex++)
    {
        subArtnet *sub_artnet = artnet->subArtnets[subArnetIndex];
        ESP_LOGI("ARTNETESP32", "subArtnet nr:%d start universe:%d Nb Universes: %d", sub_artnet->subArtnetNum, (*sub_artnet).startUniverse, sub_artnet->nbNeededUniverses);
    }
    for (;;)
    {
        if (xQueueReceive(_udp_queue, &e, portMAX_DELAY) == pdTRUE)
        {
            if (!e->pb)
            {
                free((void *)(e));
                continue;
            }
            e->pb->payload+=ART_DMX_START;
            #ifndef UNIQUE_SUBARTNET
            for (int subArnetIndex = 0; subArnetIndex < artnet->numSubArtnet; subArnetIndex++)
            {
                subArtnet *sub_artnet = (artnet->subArtnets)[subArnetIndex];
                //ESP_LOGV("ARTNETESP32", " %d %d %d %d \n", subArnetIndex, (*sub_artnet).startUniverse, sub_artnet->startUniverse - 1, e->universe);
                if (sub_artnet->startUniverse <= e->universe and (sub_artnet->endUniverse - 1) >= e->universe)
                {
                    e->pb->len=(e->pb->len - ART_DMX_START < sub_artnet->nbDataPerUniverse) ? (e->pb->len - ART_DMX_START) : sub_artnet->nbDataPerUniverse;
                   
                    sub_artnet->handleUniverse(e);
                    continue;
                }
            }
            #else
            subArtnet *sub_artnet = (artnet->subArtnets)[0];
            e->pb->len=(e->pb->len - ART_DMX_START < sub_artnet->nbDataPerUniverse) ? (e->pb->len - ART_DMX_START) : sub_artnet->nbDataPerUniverse;
                   
                    sub_artnet->handleUniverse(e);
            #endif
            pbuf_free(e->pb);
            free((void *)(e));
        }
    }
    _udp_task_handle = NULL;
    vTaskDelete(NULL);
}

static void _udp_task_subrarnet_handle(void *pvParameters)
{
    subArtnet *subartnet = (subArtnet *)pvParameters;
    uint8_t *data = NULL;
     ESP_LOGV("ARTNETESP32","_udp_task_subrarnet_handle set on core %d",xPortGetCoreID() );
                  #if CORE_DEBUG_LEVEL>=2
           long t1,t2;
           #endif
    for (;;)
    {
        if (xQueueReceive(_show_queue[subartnet->subArtnetNum], &data, portMAX_DELAY) == pdTRUE)
        {
             #if CORE_DEBUG_LEVEL>=2
            t1=ESP.getCycleCount();
           #endif
            subartnet->nb_frames++;
            if (subartnet->frameCallback)
                //subartnet->frameCallback(data);
                         // if( subartnet->subartnetglag)
                 subartnet->frameCallback((void *)subartnet);
            //else
             //subartnet->frameCallback((void *)data);
                //subartnet->_executeCallback();
            /*if(NB_MAX_BUFFER-uxQueueSpacesAvailable( _show_queue[subartnet->subArtnetNum])>0 )
                {
                    vTaskDelay(10);
                }
                */
             #if CORE_DEBUG_LEVEL>=2
             t2=ESP.getCycleCount()-t1;
            if(NB_MAX_BUFFER-uxQueueSpacesAvailable( _show_queue[subartnet->subArtnetNum])>0 )
                {
                     ESP_LOGD("ARTNETESP32","encore %d Frame:%d %f" ,NB_MAX_BUFFER-uxQueueSpacesAvailable( _show_queue[subartnet->subArtnetNum]),subartnet->nb_frames,(float)t2/240000);
                    subartnet->nb_frame_double++;
                }
                   
                if ((subartnet->nb_frames) % NB_FRAMES_DELTA == 0)
                {
                    subartnet->time2 =millis();
                    ESP_LOGI("ARTNETESP32", "SUBARTNET:%d frames fully received:%d frames lost:%d  delta:%d percentage lost:%.2f  fps: %.2f nb frame 'too fast': %d", subartnet->subArtnetNum, subartnet->nb_frames, subartnet->nb_frames_lost - 1, subartnet->nb_frames_lost - subartnet->previous_lost, (float)(100 * (subartnet->nb_frames_lost - 1)) / (subartnet->nb_frames_lost + subartnet->nb_frames - 1), (float)(1000 * NB_FRAMES_DELTA / ((subartnet->time2 - subartnet->time1) / 1)),subartnet->nb_frame_double);
                    subartnet->time1 = subartnet->time2;
                    subartnet->previous_lost = subartnet->nb_frames_lost;
                } 
                #endif  
        }

    }
}
static bool _udp_task_start(artnetESP32V2 *p)
{
    if (!_udp_queue)
    {
        _udp_queue = xQueueCreate(128, sizeof(lwip_event_packet_t *));
        if (!_udp_queue)
        {
            return false;
        }
    }
    for (int subArnetIndex = 0; subArnetIndex < p->numSubArtnet; subArnetIndex++)
    {
        if (!_show_queue[subArnetIndex])
        {
            _show_queue[subArnetIndex] = xQueueCreate(NB_MAX_BUFFER, sizeof(uint8_t *));
            if (!_show_queue[subArnetIndex])
            {
                ESP_LOGD("ARTNETESP32", "SHOW QUEUE %d QUEUE NOT CREATED", subArnetIndex);
                return false;
            }
            ESP_LOGI("ARTNETESP32", "QUEUES CREATED");
        }
    }
    if (!_udp_task_handle)
    {
        xTaskCreateUniversal(_udp_task_subrarnet, "_udp_task_subrarnet", 4096, p, CONFIG_ARDUINO_UDP_TASK_PRIORITY, (TaskHandle_t *)&_udp_task_handle, SUBARTNET_CORE);

        if (!_udp_task_handle)
        {
            ESP_LOGI("ARTNETESP32", "no task handle");
            return false;
        }
    }
    return true;
}

#endif