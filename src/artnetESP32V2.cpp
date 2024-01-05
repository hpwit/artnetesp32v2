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
#ifndef NB_FRAMES_DELTA
#define NB_FRAMES_DELTA 100
#endif

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


static xQueueHandle _udp_queue;
static xQueueHandle _show_queue[MAX_SUBARTNET];
static volatile TaskHandle_t _udp_task_handle = NULL;

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
            pbuf_free(e->pb);
            free((void *)(e));
        }
    }
    _udp_task_handle = NULL;
    vTaskDelete(NULL);
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

// static bool _udp_task_post(void *arg, udp_pcb *pcb, pbuf *pb, const ip_addr_t *addr, uint16_t port, struct netif *netif)
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

static void _udp_task_subrarnet_handle(void *pvParameters)
{
    subArtnet *subartnet = (subArtnet *)pvParameters;
    uint8_t *data = NULL;
     ESP_LOGV("ARTNETESP32","_udp_task_subrarnet_handle set on core %d",xPortGetCoreID() );
    for (;;)
    {
        if (xQueueReceive(_show_queue[subartnet->subArtnetNum], &data, portMAX_DELAY) == pdTRUE)
        {
             #if CORE_DEBUG_LEVEL>=2
           long t1=ESP.getCycleCount();
           #endif
            subartnet->nb_frames++;
            if (subartnet->frameCallback)
                //subartnet->frameCallback(data);
                          if( subartnet->subartnetglag)
                 subartnet->frameCallback((void *)subartnet);
            else
             subartnet->frameCallback((void *)data);
                //subartnet->_executeCallback();
            /*if(NB_MAX_BUFFER-uxQueueSpacesAvailable( _show_queue[subartnet->subArtnetNum])>0 )
                {
                    vTaskDelay(10);
                }
                */
             #if CORE_DEBUG_LEVEL>=2
             long t2=ESP.getCycleCount()-t1;
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

 void subArtnet::handleUniverse(lwip_event_packet_t *e)
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
        
        if(frame_disp)
        {

                data =  buffers[currentframenumber];
        currentframenumber = (currentframenumber + 1) % nbOfBuffers;
        offset = buffers[currentframenumber];
            frame_disp= false;
            
              
            if(_using_queues==true)
            {
                xQueueSend(_show_queue[subArtnetNum], &data, portMAX_DELAY);
            }
          else 
          {
            nb_frames++;
           if (frameCallback)
           {
            if(subartnetglag)
                frameCallback((void *)this);
            else
            frameCallback((void*)data);

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
            }
            
        }
        previousUniverse = startUniverse - 1;
        /*
        previousUniverse = current_uni;
        memcpy(offset, payload + ART_DMX_START, nbDataPerUniverse);
        tmp_len = (length - ART_DMX_START < nbDataPerUniverse) ? (length - ART_DMX_START) : nbDataPerUniverse;
        return;*/
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
                frame_disp=true;

            }
        
    }
    else
    {
        new_frame = false;
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
/*
void subArtnet::setFrameCallback(  void (*fptr)(uint8_t * data))
{
     frameCallback = fptr;
}

*/



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