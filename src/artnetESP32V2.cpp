#include "Arduino.h"
#include "artnetESP32V2.h"


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

#define BUFFER_SIZE 400
#define ART_DMX_START 18

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
        void *arg;
        udp_pcb *pcb;
        pbuf *pb;
        const ip_addr_t *addr;
        uint16_t port;
        struct netif * netif;
} lwip_event_packet_t;

static xQueueHandle _udp_queue;
static volatile TaskHandle_t _udp_task_handle = NULL;

static void _udp_task(void *pvParameters){
    lwip_event_packet_t * e = NULL;
    uint8_t u[2000];

    int currenbt_uni,nb_frames_lost,previous_uni,Nb_universe,nb_frames;
    bool enterint_uni=false;
    uint32_t decal,decal2;
    long time1,time2;
    uint8_t *offset, *offset2;
Nb_universe=((artnetESP32V2 *)pvParameters)->nbNeededUniverses;
     decal =  ((artnetESP32V2 *)pvParameters)->nbPixelsPerUniverse * 3 + ART_DMX_START;
     decal2 =  ((artnetESP32V2 *)pvParameters)->nbNeededUniverses * decal;
     offset=((artnetESP32V2 *)pvParameters)->buffers[0];
    for (;;) {
        if(xQueueReceive(_udp_queue, &e, portMAX_DELAY) == pdTRUE){
            if(!e->pb){
                free((void*)(e));
                continue;
            }

    

              //Serial.printf("%d\n",(e& 0xff00)>>8);
              
               
              //  Serial.printf("size %d\n",e->pb->len);
              currenbt_uni =*((uint8_t *)(e->pb->payload)+14);
             // Serial.printf("%d %d %d\n",currenbt_uni,((artnetESP32V2 *)pvParameters)->currentframenumber ,(((artnetESP32V2 *)pvParameters)->nbPixelsPerUniverse));
              // offset2 = ((artnetESP32V2 *)pvParameters)->artnetleds1 +  ((artnetESP32V2 *)pvParameters)->buffers[]+currenbt_uni*(((artnetESP32V2 *)pvParameters)->nbPixelsPerUniverse) * 3;
              #ifndef FULL_LENGTTH_ARTNET
               offset2=offset+currenbt_uni*(((artnetESP32V2 *)pvParameters)->nbPixelsPerUniverse) * 3;
                 memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,((artnetESP32V2 *)pvParameters)->nbPixelsPerUniverse * 3);
            #else
                offset2=offset+currenbt_uni*512;
                 memcpy(offset2,(uint8_t *)e->pb->payload+ART_DMX_START,e->pb->len-ART_DMX_START);
            #endif
              //Serial.printf("%d \n",currenbt_uni);
              if(currenbt_uni==Nb_universe-1 and enterint_uni==true)
              {
                nb_frames++;
               // Serial.printf("%d\n",currenbt_uni);
                enterint_uni=false;
                //Serial.printf("ee\n");
                 ((artnetESP32V2 *)pvParameters)->currentframe=offset;
                 (*((artnetESP32V2 *)pvParameters)->frameCallback)();
                ((artnetESP32V2 *)pvParameters)->currentframenumber=( ((artnetESP32V2 *)pvParameters)->currentframenumber+1)%2;
                offset=((artnetESP32V2 *)pvParameters)->buffers[ ((artnetESP32V2 *)pvParameters)->currentframenumber];
                #ifdef  ARTNET_STAT
               if((nb_frames)%100==0)
                {
                    //time2=millis();
                    //Serial.printf("frame %d lost %d %.2f in %.2fs or %.2f fps\n",nb_frames,nb_frames_lost,(float)(100*nb_frames_lost)/(nb_frames_lost+nb_frames),(float)(time2-time1)/1000,(float)(1000*1000/(time2-time1)));
                    Serial.printf("frame %d lost %d %.2f \n",nb_frames,nb_frames_lost,(float)(100*nb_frames_lost)/(nb_frames_lost+nb_frames));
                   // time1=time2;
                }
                #endif
                enterint_uni==false;

              }
              else
              {
                if( (currenbt_uni>0) and (currenbt_uni!=previous_uni+1))
                {
                  if(previous_uni!=9999)
                  nb_frames_lost++;
                //Serial.printf("frame %d lost %d\n",nb_frames,nb_frames_lost);
                  previous_uni=9999;
                   enterint_uni=false;

                } else
                {
                  previous_uni=currenbt_uni;
                  if( currenbt_uni==0)
                  {
                     enterint_uni=true;
                    //Serial.printf("new frame 0 "); 
                     previous_uni=0;
                  }
                  else
                  {
                    //Serial.printf("%d ",currenbt_uni);
                  }
                }
              }
              

        //Serial.printf("%d\n",*((uint8_t *)(e->pb->payload+1)));
            //AsyncUDP::_s_recv(e->arg, e->pcb, e->pb, e->addr, e->port, e->netif);
             pbuf_free(e->pb);
            free((void*)(e));
        }
    }
    _udp_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool _udp_task_start(artnetESP32V2 *p){
    if(!_udp_queue){
        _udp_queue = xQueueCreate(64, sizeof(lwip_event_packet_t *));
        if(!_udp_queue){
            return false;
        }
    }
    if(!_udp_task_handle){
        xTaskCreateUniversal(_udp_task, "async_udp", 4096, p, CONFIG_ARDUINO_UDP_TASK_PRIORITY, (TaskHandle_t*)&_udp_task_handle, 0);
        if(!_udp_task_handle){
            return false;
        }
    }
    return true;
}

static bool _udp_task_post(void *arg, udp_pcb *pcb, pbuf *pb, const ip_addr_t *addr, uint16_t port, struct netif *netif)
{
    if(!_udp_task_handle || !_udp_queue){
        return false;
    }
    lwip_event_packet_t * e = (lwip_event_packet_t *)malloc(sizeof(lwip_event_packet_t));
    if(!e){
        return false;
    }
    e->arg = arg;
    e->pcb = pcb;
    e->pb = pb;
    e->addr = addr;
    e->port = port;
    e->netif = netif;
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
      // Serial.printf(":%d\n", *((uint8_t *)(this_pb->payload+1) ));
        if(!_udp_task_post(arg, pcb, this_pb, addr, port, ip_current_input_netif())){
            pbuf_free(this_pb);
        }
    }
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





artnetESP32V2::artnetESP32V2(uint16_t nbpixels, uint16_t nbpixelsperuniverses,int startunivers)
{
    _pcb = NULL;
    _connected = false;
	_lastErr = ERR_OK;
     currentframenumber=0;
   // _handler = NULL;
     startuniverse=startunivers;
       nbPixels = nbpixels;
    nbPixelsPerUniverse = nbpixelsperuniverses;
    nbNeededUniverses = nbPixels / nbPixelsPerUniverse;
    if (nbNeededUniverses * nbPixelsPerUniverse - nbPixels < 0)
    {

        nbNeededUniverses++;
    }
    //artnetleds1= (uint8_t *)malloc(nbpixels*buffernumber*3);
    buffers[0] = (uint8_t *)malloc((nbpixelsperuniverses * 3 + ART_DMX_START) * nbNeededUniverses  + 8 + BUFFER_SIZE);
    if ( buffers[0] == NULL)
    {
        Serial.printf("impossible to create the buffer\n");
        return;
    }
    buffers[1] = (uint8_t *)malloc((nbpixelsperuniverses * 3 + ART_DMX_START) * nbNeededUniverses  + 8 + BUFFER_SIZE);
    if (buffers[1] == NULL)
    {
        Serial.printf("impossible to create the buffer\n");
        return;
    }
    Serial.printf("Starting Artnet nbNee sdedUniverses:%d\n", nbNeededUniverses);
    currentframenumber=0;
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
