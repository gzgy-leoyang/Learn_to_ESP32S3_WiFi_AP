
## 概述
ESP32-S3是一颗面向 wireless 的Soc，如果不尝试一下wifi的滋味岂不是浪费。  
下面这个demo就利用 ESP32 WIFI 建立一个热点AP，以手机/PC等其他设备链接该AP后建立一个TCP/IP链接，双方采用服务器/客户端模式协同，由ESP32S3向外部设备暴露系统内部数据，比如MCU内部温度。


## 结构
本demo的软件中将建立两个模块：  
- app模块 定时读取MCU内部温度，将温度数据写入一个公共数据仓库中，并通过tcp模块的send接口将数据推送到Client。
- tcp模块 tcp任务负责建立tcp服务器并监听3333端口，当发现客户端请求链接时，新建一个tcp socket与 client建立数据通道，同时，tcp模块还提供数据发送接口，由其他模块调用

## app_main 任务
用户开发从app_main开始，将会由main函数以app_main建立任务执行用户代码，执行完app_main后返回将会导致上一层任务被删除，但不会影响用户任务。
> [官方说明](https://docs.espressif.com/projects/esp-idf/zh_CN/release-v4.4/esp32s3/api-guides/startup.html)
> 在所有其他组件都初始化后，主任务会被创建，FreeRTOS 调度器开始运行。做完一些初始化任务后（需要启动调度器），主任务在固件中运行应用程序提供的函数 app_main。运行 app_main 的主任务有一个固定的 RTOS 优先级（比最小值高）和一个 可配置的堆栈大小。主任务的内核亲和性也是可以配置的，请参考 CONFIG_ESP_MAIN_TASK_AFFINITY。与普通的 FreeRTOS 任务（或嵌入式 C 的 main 函数）不同，app_main 任务可以返回。如果``app_main`` 函数返回，那么主任务将会被删除。系统将继续运行其他的 RTOS 任务。因此可以将 app_main 实现为一个创建其他应用任务然后返回的函数，或主应用任务本身。

如下，通过调用各自模块的启动函数，完成模块内部初始化的准备工作，建立对应的任务并启动运行，之后任务之间就可通过多种“任务间通信”进行同步，比如信号，队列，消息队列等等。
```c
void app_main(void)
{
  starting_app_service();
  starting_tcp_service();
  // 返回,将销毁main的对应任务,但不会影响刚刚建立的“子任务”
}
```

## TCP Service 模块
本模块的核心以 tcp_server_task() 构成tcp服务端任务。启动任务前首先需要初始化wifi模块，并将该wifi调整为 SoftAP 模式，其实ESP32-S3的WIFI可以同时支持 SoftAp和Station模式，只是目前demo不涉及Sta相关功能。
```c
void starting_tcp_service(void)
{
  ...
  wifi_init_softap();
  xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
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
```
AP启动后的WIFI网卡的IP地址默认是192.16.4.1，这个地址应该是由 netif 层在启动 DHCPS（DHCP服务器）时设置的，esp_netif_dhcps_start()函数的调用过程目前还没有理清。其实DHCP整个的协商过程本身也比较深，相对来说已经比较复杂了，需要彻底的搞明白是需要专门花一定的时间，咱们先暂时这么用着，先从总体都理顺了再回头逐一细化关键部分。

</b>

当WIF设置完成后，WIFI网卡，IP地址，DHCP服务器等都已经具备，一旦外部设备以Sta模式链接后就可以通过DHCP获取一个IP作为Client端的地址。这部分的过程应该是在 esp_netif/lwip 层已经完成。在此的用户程序只需要建立一个 Socket 用于监听指定端口，当Client端向该监听端口发出请求时，为该Client新建一个临时Socket用于数据流即可。  
此处建立监听Socket的过程与Linux上过程基本是一样的，接口函数也都是一样的，对Linux熟悉的人来说还是比较容易理解的，我估计Windows下也应该是一致的接口，只是我对Win下的开发不太熟悉了(仅会一点点的C#也是10年前，目前.net的版本是多少我都不知道）。  

```c
// //////////////////////////////////////////
/// @brief create_listen_socket 建立监听socket
///
/// @return: 
// //////////////////////////////////////////
static esp_err_t create_listen_socket(void)
{
  // 1. 新建用于监听的socket
  listen_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_IP);
  ...
  // 2. 绑定监听端口
  int err = bind(listen_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  ...
  // 启动监听过程
  err = listen(listen_socket, 1);
  ...
  return ESP_OK;
}

static void tcp_server_task(void *pvParameters)
{
  create_listen_socket();
  while (1) {
    if ( temp_socket == 0 ){
      ...
      // 在未链接client之前，**阻塞**直到有client链接本Server
      temp_socket = accept(listen_socket, (struct sockaddr *)&source_addr, &addr_len);
      ...
      }
    } else if (temp_socket > 0) {
      ...
      // 本次读取数据：len>0,接受len字节，=0表示客户端已经下线，<0则为错误
      // 再次执行recv，**阻塞**本任务直到收到数据
      int len = recv(temp_socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
      ...
    } else {
    }
  }
  ...
}
```
需要注意的是，accept() 和 recv() 都是阻塞方式的，也就是说，在本任务下如果还有其他操作都会由于这个阻塞特性而不能正常运行。极端一点，将界面的刷新过程加入这个函数，必然导致UI在绝大部分时候是“卡住”的，只有发生通信时可以刷新一次。  

</b>

将数据发送接口封装在tcp模块中，当其他任务模块需要发送数据时可以通过该接口实现，主要目的如下:  
- 将关键的资源，对象和数据等封装起来比较安全一点，比如将数据流的Socket文件描述符封装在本模块中，不需要暴露到外部；    
- 将数据流的操作，包括错误处理，重发等机制在本模块内部实现，外部模块所需要做的就是将“数据指针+长度”扔过来就完了，至于tcp模块什么时候发送，如何发送，错误如何处理等全部不用考虑   

```c 
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
    ESP_LOGI(TAG, " Can't transmit message 'cause did't connect any client");
    return -1;
  }

  int ret = send(temp_socket, _buf, _size, 0);
  if (ret < 0) {
    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    return -1;
  }
  return ESP_OK;
}
```
这个demo主要尝试一下基本功能，演示一个大体的思路，所以这个部分的错误检查也就相对简单一点，实际的工程中应该要非常仔细的处理，我常常想，软件设计就像硬件设计一样，80%的回路和元件都是“不必须”的，就是为了预防10%，甚至为了处理1%概率的异常准备的。 

## App Service 模块
应用任务模块相对就简单很多，仅仅涉及到温度传感器的配置和读取操作。需要说明的是：
ESP32的内部温度传感器只是为测量芯片核心温度准备的，不能作为环境温度测量使用，应该会高于环境温度一点点，而且该偏差应该还会由于软件负载情况存在动态变化。  
一般来说监视芯片温度是用于高温条件下预防CPU可能存在的异常工作，我想半导体器件对高温还是比较敏感的吧，比如电子迁移，势垒等（不确定具体原理），所以很多芯片都有对应的处理机制，举两个例子：  
- 使用的S12(16bit MCU)就包含对应的高温中断异常，这是一个不可屏蔽异常。  
- 使用的i.MX6 (Cortex-A9) 嵌入式Linux环境中，当温度高于95度时CPU会主动减低频率，包括GPU也会同步降低频率，当温度继续升高到115度，则Linux会主动关机，预防在不确定的环境下做什么“出格”的事。  


```c
void starting_app_service(void)
{
  ...
  temprature_init();
  xTaskCreate(app_task, "app_service", 4096, (void*)AF_INET, 5, NULL);
}

static void app_task(void *pvParameters)
{
  ...
  while (1) {
    if ( (err =temprature_read(&temp)) == ESP_OK ){
      ...
        ...
        data_store.core_temp = temp;
        memset(buf,0,20);
        sprintf( buf,"Core Temp..%.02f\n",data_store.core_temp );
        ss = strlen(buf);
        if (send_message(buf,ss) < 0 ){
          ESP_LOGI(TAG, "Failed Send Message via WIFI");
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelete(NULL);
}
```

ESP32-S3 的调试过程如下：  
```sh
I (567) TCP Service: SoftAP SSID:lexin.a password:password.a channel:1
"启动任务，建立监听"
I (567) TCP Service: SoftAP Start
I (577) TCP Service: Listen Socket created
I (577) TCP Service: IP=0 1293
I (587) TCP Service: Listen Socket bind, port 3333
I (587) TCP Service: Listening loop...
I (597) TCP Service: Listening...
I (8397) App Service: Core Temp...15.88
"没有客户端时，发送失败"
I (8397) TCP Service:  Can't transmit message 'cause did't connect any client
I (8397) App Service: Failed Send Message via WIFI
I (24407) App Service: Core Temp...16.75
I (24407) TCP Service:  Can't transmit message 'cause did't connect any client
I (24407) App Service: Failed Send Message via WIFI
"客户端设备链接到AP，获取地址192.168.4.2"
I (33207) wifi:new:<1,1>, old:<1,1>, ap:<1,1>, sta:<255,255>, prof:1
I (33217) wifi:station: 14:4f:8a:99:35:81 join, AID=1, bgn, 40U
I (33237) TCP Service: Connected
I (33237) TCP Service: station 14:4f:8a:99:35:81 join, AID=1
I (33287) esp_netif_lwip: DHCP server assigned IP to a station, IP is: 192.168.4.2
W (34037) wifi:<ba-add>idx:2 (ifx:1, 14:4f:8a:99:35:81), tid:0, ssn:16, winSize:64
I (34417) App Service: Core Temp...18.07
I (34417) TCP Service:  Can't transmit message 'cause did't connect any client
I (34417) App Service: Failed Send Message via WIFI
I (36997) TCP Service:  Connected to Client...192.168.4.2:18068
"正常发送温度数据"
I (49427) App Service: Core Temp...18.07

```

外部设备（PC，Linux）通过netcat链接 ESP32 AP(192.168.4.1:3333)，如下：  
```sh
$ nc 192.168.4.1 3333 
Core Temp..18.07
Core Temp..18.95
Core Temp..19.39
```
以上demo简单的使用了一下 wifi 模块，并建立一个tcp服务用于与外部设备的Client通信，后续计划在 Linux 下用 Qt 做一个简单的客户端App。