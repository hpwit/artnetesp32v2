#ifndef ARTNETESP32_H
#define ARTNETESP32_H

#include "IPAddress.h"
#include "IPv6Address.h"
#include "Print.h"
#include "Stream.h"
#include <functional>
extern "C" {
#include "esp_netif.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
}


//class artnetESP32V2;

struct udp_pcb;
struct pbuf;
struct netif;
#define ART_DMX_START 18
 
class artnetESP32V2 //: public Print 
{
        protected:
            udp_pcb *_pcb;
    //xSemaphoreHandle _lock;
    bool _connected;
	esp_err_t _lastErr;
 
    bool _init();
    //void _recv(udp_pcb *upcb, pbuf *pb, const ip_addr_t *addr, uint16_t port, struct netif * netif);

    public:

       int num_universes;
    uint8_t *artnetleds1, *buffer2;
    uint8_t  *buffers[2];
      uint8_t currentframenumber;
      uint8_t *currentframe;
    int pixels_per_universe,nbPixels,nbPixelsPerUniverse,nbNeededUniverses,startuniverse;
        artnetESP32V2(uint16_t nbpixels, uint16_t nbpixelsperuniverses,int startunivers);
         ~artnetESP32V2();

    bool listen(const ip_addr_t *addr, uint16_t port);
    bool listen(const IPAddress addr, uint16_t port);
    bool listen(const IPv6Address addr, uint16_t port);
    bool listen(uint16_t port);
    bool connect(const ip_addr_t *addr, uint16_t port);
    bool connect(const IPAddress addr, uint16_t port);
    bool connect(const IPv6Address addr, uint16_t port);
    uint8_t * getframe()
    {
        return currentframe;
    }
    operator bool();
    void close();
    
   void (*frameCallback)();
  inline void setFrameCallback(  void (*fptr)())
  {
    frameCallback = fptr;
  }

//static void _s_recv(void *arg, udp_pcb *upcb, pbuf *p, const ip_addr_t *addr, uint16_t port, struct netif * netif);
};


#endif