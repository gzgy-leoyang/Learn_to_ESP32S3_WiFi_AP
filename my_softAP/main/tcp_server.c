#include <string.h>
#include <sys/param.h>
#include "app_main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <stdio.h>

#include "driver/temp_sensor.h"
#include "esp_err.h"

#define PORT                        3333
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

#define WIFI_SSID                   "lexin.a"
#define WIFI_PASS                   "password.a"
#define EXAMPLE_ESP_WIFI_CHANNEL    CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN        CONFIG_ESP_MAX_STA_CONN

esp_err_t send_message( const char* _buf,const int _size );

static const char *TAG     = "TCP Service";
static int   temp_socket   = 0;
static int   listen_socket = 0;

       void      starting_tcp_service(void);
static void      wifi_event_handler(void*            arg, 
                                    esp_event_base_t event_base,
                                    int32_t          event_id, 
                                    void*            event_data);
static void      wifi_init_softap(void);
static void      tcp_server_task(void *pvParameters);
static esp_err_t create_listen_socket(void);

// //////////////////////////////////////////
/// @brief starting_tcp_service 启动tcp service任务
// //////////////////////////////////////////
void starting_tcp_service(void)
{
  //初始化NVS存储器，必须，wifi模块初始化过程需要
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // 初始化wifi模块为 SoftAP 模式
  wifi_init_softap();
  // 添加服务器任务
  xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
}


// //////////////////////////////////////////
/// @brief tcp_server_task TCP Service 任务
/// 监听客户端链接请求，并接受客户端发来的数据
/// @param: pvParameters
// //////////////////////////////////////////
static void tcp_server_task(void *pvParameters)
{
  char addr_str[128];
  int keepAlive    = 1;
  int keepIdle     = KEEPALIVE_IDLE;
  int keepInterval = KEEPALIVE_INTERVAL;
  int keepCount    = KEEPALIVE_COUNT;

  // 新建并启动监听socket，等待Client发起链接
  create_listen_socket();
  while (1) {
    // 任务核心：
    // 1.没有client时，不断监听 listen_socket，直到接收到client请求;
    // 2.当收到client链接后转入对client数据的处理;
    if ( temp_socket == 0 ){
      ESP_LOGI(TAG, "Listening...");
      struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
      socklen_t addr_len = sizeof(source_addr);
      // 在未链接client之前，**阻塞**直到有client链接本Server
      temp_socket = accept(listen_socket, (struct sockaddr *)&source_addr, &addr_len);
      if (temp_socket < 0) {
          ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
          break;
      }
      ESP_LOGI(TAG, "< Welcome ESP32-S3 SoftAP > %i",temp_socket);

      // 链接之后配置
      setsockopt(temp_socket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
      setsockopt(temp_socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
      setsockopt(temp_socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
      setsockopt(temp_socket, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
      // 打印Client地址
      if (source_addr.ss_family == PF_INET) {
          inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, 
                      addr_str, 
                      sizeof(addr_str) - 1);
          ESP_LOGI(TAG, " Client Addr...%s:%i",
                   addr_str,
                   ((struct sockaddr_in*)&source_addr)->sin_port);
      }
    } else if (temp_socket > 0) {
      // 一旦有client链接后，就会持续监听client发来的数据
      char rx_buffer[128] = {0};
      // 本次读取数据：len>0,接受len字节，=0表示客户端已经下线，<0则为错误
      // 再次执行recv，**阻塞**本任务直到收到数据
      int len = recv(temp_socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
      if (len < 0) {
        ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        temp_socket = 0;
      } else if (len == 0) {
        // 当前客户端已经离线，返回temp_socket=0状态，回到监听端口状态
        ESP_LOGW(TAG, "Client has disconnected");
        temp_socket = 0;
      } else {
        // 接受client发来的内容
        rx_buffer[len] = 0; 
        ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
        int written = send(temp_socket, rx_buffer, len, 0);
        if (written < 0) {
          ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        }
      }
    } else {
      // temp_socket < 0
    }
  }

  if ( temp_socket > 0 ){
    shutdown(temp_socket, 0);
    close(temp_socket);
  }
}

// //////////////////////////////////////////
/// @brief create_listen_socket 建立监听socket
///
/// @return: 
// //////////////////////////////////////////
static esp_err_t create_listen_socket(void)
{
  // 1. 新建用于监听的socket
  listen_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (listen_socket < 0) {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    vTaskDelete(NULL);
    return -1;
  }
  ESP_LOGI(TAG, "Listen Socket created");

  int opt = 1;
  setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  setsockopt(listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

  struct sockaddr_storage dest_addr;
  struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
  //INADDR_ANY=0.0.0.0，表示本机的所有网卡的IP
  dest_addr_ip4->sin_addr.s_addr    = htonl(INADDR_ANY);
  dest_addr_ip4->sin_family         = AF_INET;
  dest_addr_ip4->sin_port           = htons(PORT);

  // 2. 绑定监听端口
  int err = bind(listen_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0) {
      ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
      goto CLEAN_UP;
  }
  ESP_LOGI(TAG, "Listen Socket bind, port %d", PORT);

  // 启动监听过程
  err = listen(listen_socket, 1);
  if (err != 0) {
      ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
      goto CLEAN_UP;
  }
  ESP_LOGI(TAG, "Listening loop...");
  return ESP_OK;
CLEAN_UP:
    close(listen_socket);
    vTaskDelete(NULL);
    return -1;
}

// //////////////////////////////////////////
/// @brief wifi_event_handler 处理wifi链接过程的事件
///
/// @param: arg
/// @param: event_base
/// @param: event_id
/// @param: event_data
// //////////////////////////////////////////
static void wifi_event_handler(void* arg, 
                               esp_event_base_t event_base,
                               int32_t event_id, 
                               void* event_data)
{
  switch( event_id ){
    case WIFI_EVENT_AP_START:
      ESP_LOGI(TAG, "SoftAP Start");
      break;
    case WIFI_EVENT_AP_STOP:
      ESP_LOGI(TAG, "SoftAP Stop");
      break;
    case WIFI_EVENT_AP_STACONNECTED:{
      ESP_LOGI(TAG, "Connected");
      wifi_event_ap_staconnected_t* event=(wifi_event_ap_staconnected_t*)event_data;
      ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
               MAC2STR(event->mac), event->aid);
    }
      break;
    case WIFI_EVENT_AP_STADISCONNECTED:{
      ESP_LOGI(TAG, "Disconnected");
      wifi_event_ap_stadisconnected_t* event=(wifi_event_ap_stadisconnected_t*)event_data;
      ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
               MAC2STR(event->mac), event->aid);

    }
      break;
  }
}

// //////////////////////////////////////////
/// @brief wifi_init_softap 建立wifi AP
/// @note 
/// 当启动wifi后，可能通过netif调用启动了lwip的DHCP服务器，
/// 该服务器,esp_netif_dhcps_start() 启动了DHCPS，并指定默认192.168.4.1 
/// 作为AP的地址，为其他链接到本AP的 DHCP客户端分配地址,
/// 目前还没有找到在哪里调用了esp_netif_dhcps_start函数的，有时间继续找
// //////////////////////////////////////////
static void wifi_init_softap(void)
{
  //netif是ESP32官方在tcp/ip协议栈上封装的一层接口,目前只为lwip实现了netif层
  //初始化TCP/IP协议栈
  ESP_ERROR_CHECK(esp_netif_init());
  // 创建默认事件循环
  // 默认事件循环是一个特殊的，用于系统事件的循环，这个循环的句柄对用户来说是
  // 隐藏的，事件的建立，删除，注册和传递都是通过API实现
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // esp_netif_t *esp_netif_create_default_wifi_ap(void),
  // 返回esp-netif实例指针
  // 创建一个具备默认AP配置的esp_netif对象，
  // 且绑定到WIFI，并注册默认的wifi处理句柄
  esp_netif_create_default_wifi_ap();

  // 产生一个wifi堆栈的默认配置
  // 通过宏定义获取的默认配置，所有参数均可以确保正确的默认值，之后在修改个别
  // 参数也比较方便
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  // 初始化wifi,包括：wifi驱动的资源，wifi控制结构体，收发缓冲区，NVS结构参数，
  // 并启动WIFI任务
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // 向默认事件循环添加注册一个回调函数用于处理“所有与WIFI相关的事件”
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, //the base ID of the event 
                                                      ESP_EVENT_ANY_ID,//the ID of the event
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));
  //#define ESP_EVENT_ANY_ID -1 //register handler for any event id

  // 配置wifi的参数结构
  wifi_config_t wifi_config = {
      .ap = {
          .ssid           = WIFI_SSID,
          .ssid_len       = strlen(WIFI_SSID),
          .channel        = EXAMPLE_ESP_WIFI_CHANNEL,
          .password       = WIFI_PASS,
          .max_connection = EXAMPLE_MAX_STA_CONN,
          .authmode       = WIFI_AUTH_WPA_WPA2_PSK
      },
  };

  //设置wifi使用AP模式
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  // 写入wifi配置参数
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  // 启动WIFI
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "SoftAP SSID:%s password:%s channel:%d",
           WIFI_SSID, WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

// //////////////////////////////////////////
/// @brief send_message  通过socket发送数据
///
/// @param: _buf 数据缓冲区指针
/// @param: _size 数据长度
///
/// @return: 
// //////////////////////////////////////////
esp_err_t send_message( const char* _buf,const int _size )
{
  if ( temp_socket <= 0 ){
    return -1;
  }

  int ret = send(temp_socket, _buf, _size, 0);
  if (ret < 0) {
    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    return -1;
  }
  return ESP_OK;
}
// end of code //
