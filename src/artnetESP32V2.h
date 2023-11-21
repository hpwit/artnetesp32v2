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
#define ART_DMX_START 18
#define BUFFER_SIZE 512
#define MAX_SUBARTNET 20
#define NB_MAX_BUFFER 4
#ifndef  _USING_QUEUES
#define _USING_QUEUES 1
#endif
#ifndef SUBARTNET_CORE
#define SUBARTNET_CORE 0
#endif
#ifndef CALLBACK_CORE
#define CALLBACK_CORE 1
#endif

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
  void (*frameCallback)(uint8_t *data);
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
 void handleUniverse(int currenbt_uni, uint8_t *payload, size_t len);
  uint8_t *getData();
  void setFrameCallback(void (*fptr)(uint8_t *data));
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
       numSubArtnet++;
      return subArtnets[numSubArtnet - 1];
    }
    else
    {
      return NULL;
    }
  }

  void setNodeName(String s);

  subArtnet *subArtnets[MAX_SUBARTNET];
  // static void _s_recv(void *arg, udp_pcb *upcb, pbuf *p, const ip_addr_t *addr, uint16_t port, struct netif * netif);
};


 

#endif