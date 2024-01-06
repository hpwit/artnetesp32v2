#include "Arduino.h"
#include "artnetESP32V2.h"
#include "esp_log.h"

extern "C"
{
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

 #include "artpoll.h"
#define BUFFER_SIZE 10
#define ART_DMX_START 18
#define NB_MAX_BUFFER 10
#define MAX_SUBARTNET 20


#define SUBARTNET_CORE 0
#define CALLBACK_CORE 1
//#define _USING_QUEUES
#define UDP_MUTEX_LOCK()   // xSemaphoreTake(_lock, portMAX_DELAY)
#define UDP_MUTEX_UNLOCK() // xSemaphoreGive(_lock)
typedef struct
{
    struct tcpip_api_call_data call;
    udp_pcb *pcb;
    const ip_addr_t *addr;
    uint16_t port;
    struct pbuf *pb;
    struct netif *netif;
    err_t err;
} udp_api_call_t;

static err_t _udp_connect_api(struct tcpip_api_call_data *api_call_msg)
{
    udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
    msg->err = udp_connect(msg->pcb, msg->addr, msg->port);
    return msg->err;
}

static err_t _udp_connect(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port)
{
    udp_api_call_t msg;
    msg.pcb = pcb;
    msg.addr = addr;
    msg.port = port;
    tcpip_api_call(_udp_connect_api, (struct tcpip_api_call_data *)&msg);
    return msg.err;
}

static err_t _udp_disconnect_api(struct tcpip_api_call_data *api_call_msg)
{
    udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
    msg->err = 0;
    udp_disconnect(msg->pcb);
    return msg->err;
}

static void _udp_disconnect(struct udp_pcb *pcb)
{
    udp_api_call_t msg;
    msg.pcb = pcb;
    tcpip_api_call(_udp_disconnect_api, (struct tcpip_api_call_data *)&msg);
}

static err_t _udp_remove_api(struct tcpip_api_call_data *api_call_msg)
{
    udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
    msg->err = 0;
    udp_remove(msg->pcb);
    return msg->err;
}

static void _udp_remove(struct udp_pcb *pcb)
{
    udp_api_call_t msg;
    msg.pcb = pcb;
    tcpip_api_call(_udp_remove_api, (struct tcpip_api_call_data *)&msg);
}

static err_t _udp_bind_api(struct tcpip_api_call_data *api_call_msg)
{
    udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
    msg->err = udp_bind(msg->pcb, msg->addr, msg->port);
    return msg->err;
}

static err_t _udp_bind(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port)
{
    udp_api_call_t msg;
    msg.pcb = pcb;
    msg.addr = addr;
    msg.port = port;
    tcpip_api_call(_udp_bind_api, (struct tcpip_api_call_data *)&msg);
    return msg.err;
}

static bool _udp_task_post(pbuf *pb)
{
    if (!_udp_task_handle || !_udp_queue)
    {
        return false;
    }
    lwip_event_packet_t *e = (lwip_event_packet_t *)malloc(sizeof(lwip_event_packet_t));
    if (!e)
    {
        return false;
    }
    e->pb = pb;
    e->universe = *((uint8_t *)(e->pb->payload) + 14);
    
     //ESP_LOGV("TAG", "opcode: %s",(char *)(e->pb->payload));

    if (xQueueSend(_udp_queue, &e, portMAX_DELAY) != pdPASS)
    {
        free((void *)(e));
        return false;
    }
    return true;
}

static void _udp_recv(void *arg, udp_pcb *pcb, pbuf *pb, const ip_addr_t *addr, uint16_t port)
{
   
    while (pb != NULL)
    {
        pbuf *this_pb = pb;
        pb = pb->next;
        this_pb->next = NULL;
       // Serial.printf("%x\n",*((uint16_t *)(this_pb->payload) + 14));
    if(*((uint16_t *)(this_pb->payload) + 4)==0x2100)
    {
           // ESP_LOGV("ATAG","%s",ip4addr_ntoa(&addr->u_addr.ip4));
            //ESP_LOGV("ATAG","%d",this_pb->ref);

       // IPAddress my_ip = WiFi.localIP();
        //ESP_LOGV("tag","%d %d %d %d",my_ip[0],my_ip[1],my_ip[2],my_ip[3]);
    //ESP_LOGV("TAG", "opcode: %x ",*((uint16_t *)(this_pb->payload) + 4));//,*((uint8_t *)(this_pb->pb->payload) + 15),*((uint8_t *)(e->pb->payload) + 16),*((uint8_t *)(e->pb->payload) + 17),*((uint8_t *)(e->pb->payload) + 18));
    poll_reply(pcb,addr);
     pbuf_free(this_pb);
    //return;
    }
    else
    {
        
       // artnetESP32V2 *artnet = ((artnetESP32V2 *)arg);
        if(*((uint16_t *)(this_pb->payload) + 4)==0x5000)
        {
        if (!_udp_task_post(this_pb))
        {
            pbuf_free(this_pb);
        }
        }
        else
        {
             pbuf_free(this_pb);
        }
    }
    }
}



subArtnet::subArtnet(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe)
{
    _initialize(star_universe, nb_data, nb_data_per_universe, NULL);
}

void subArtnet::createBuffers(uint8_t *leds)
{
    for (int buffnum = 0; buffnum < NB_MAX_BUFFER; buffnum++)
    {
        buffers[buffnum] = (uint8_t *)calloc((nbDataPerUniverse)*nbNeededUniverses + 8 + BUFFER_SIZE, 1);
        if (buffers[buffnum] == NULL)
        {
            ESP_LOGI("ARTNETESP32", "impossible to create the buffer %d", buffnum);
            if (leds != NULL)
            {
                buffers[buffnum] = leds;
                ESP_LOGI("ARTNETESP32", "using leds array as  buffer %d", buffnum);
              
                nbOfBuffers = buffnum + 1;
                return;
            }
            else
            {
                nbOfBuffers = buffnum;
                ESP_LOGI("ARTNETESP32", "nb total of buffers:%d", nbOfBuffers);

                return;
            }
        }
        else
        {
            ESP_LOGI("ARTNETESP32", "Creation of  buffer %d", buffnum);
           
            nbOfBuffers = buffnum + 1;
        }
        memset(buffers[buffnum], 0, (nbDataPerUniverse)*nbNeededUniverses + 8 + BUFFER_SIZE);
    }
    ESP_LOGI("ARTNETESP32", "nb total of buffers:%d", nbOfBuffers);
}

void subArtnet::_initialize(int star_universe, uint32_t nb_data, uint32_t nb_data_per_universe, uint8_t *leds)
{
    nb_frames = 0;
    nb_frame_double = 0;
    nb_frames_lost = 0;
    previous_lost = 0;
    previousUniverse = 99;
     new_frame = false;
     _nb_data=nb_data;

    nbDataPerUniverse = nb_data_per_universe;
    startUniverse = star_universe;
    nbNeededUniverses = nb_data / nbDataPerUniverse;
    if (nbNeededUniverses * nbDataPerUniverse < nb_data)
    {
        nbNeededUniverses++;
    }
    len = nb_data;
    endUniverse = startUniverse + nbNeededUniverses;

    ESP_LOGI("ARTNETESP32", "Initialize subArtnet Start Universe: %d  end Universe: %d, universes %d", startUniverse, endUniverse - 1, nbNeededUniverses);
    createBuffers(leds);
    currentframenumber = 0;
    offset = buffers[0];
   // new_frame = false;
}

subArtnet::~subArtnet()
{
    if (buffers[0] != NULL)
        free(buffers[0]);
    if (buffers[1] != NULL)
        free(buffers[1]);
}



uint8_t *subArtnet::getData()
{
    return data;
}



artnetESP32V2::artnetESP32V2()
{
    _pcb = NULL;
    _connected = false;
    _lastErr = ERR_OK;
}

bool artnetESP32V2::_init()
{
    if (_pcb)
    {
        return true;
    }
    _pcb = udp_new();
    if (!_pcb)
    {
        return false;
    }
    //_lock = xSemaphoreCreateMutex();
    udp_recv(_pcb, &_udp_recv, (void *)this);
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
    // vSemaphoreDelete(_lock);
}

void artnetESP32V2::close()
{
    UDP_MUTEX_LOCK();
    if (_pcb != NULL)
    {
        if (_connected)
        {
            _udp_disconnect(_pcb);
        }
        _connected = false;
        // todo: unjoin multicast group
    }
    UDP_MUTEX_UNLOCK();
}

bool artnetESP32V2::connect(const ip_addr_t *addr, uint16_t port)
{
    if (!_udp_task_start(this))
    {
        log_e("failed to start task");
        return false;
    }
    if (!_init())
    {
        return false;
    }
    close();
    UDP_MUTEX_LOCK();
    _lastErr = _udp_connect(_pcb, addr, port);
    if (_lastErr != ERR_OK)
    {
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
    memcpy((uint8_t *)(daddr.u_addr.ip6.addr), (const uint8_t *)addr, 16);
    return connect(&daddr, port);
}

bool artnetESP32V2::listen(const ip_addr_t *addr, uint16_t port)
{
    if (!_udp_task_start(this))
    {
        log_e("failed to start task");
        return false;
    }
    char mess[60];

        for (int subartnet = 0; subartnet < numSubArtnet; subartnet++)
        {
            if( subArtnets[subartnet]->_using_queues ==true)
            {
                memset(mess, 0, 60);
                sprintf(mess, "handle_subartnet_%d", subartnet);
                subArtnets[subartnet]->subArtnet_sem = xSemaphoreCreateBinary();
                xTaskCreateUniversal(_udp_task_subrarnet_handle, mess, 4096, subArtnets[subartnet], 3, NULL, CALLBACK_CORE);
            }
        }
   
    if (!_init())
    {
        return false;
    }
    close();
    if (addr)
    {
        IP_SET_TYPE_VAL(_pcb->local_ip, addr->type);
        IP_SET_TYPE_VAL(_pcb->remote_ip, addr->type);
    }
    UDP_MUTEX_LOCK();
    if (_udp_bind(_pcb, addr, port) != ERR_OK)
    {
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
    memcpy((uint8_t *)(laddr.u_addr.ip6.addr), (const uint8_t *)addr, 16);
    return listen(&laddr, port);
}
artnetESP32V2::operator bool()
{
    return true;
}

  void artnetESP32V2::setNodeName(String s)
  {
    short_name=s;
  }