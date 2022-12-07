#include "app_main.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "Main";
// 应用数据
Data_store_t data_store;


// //////////////////////////////////////////
/// @brief app_main 
// app_main作为一个task,IDF框架的main()中建立的任务调用
// 再有该函数建立下一层任务
//
// @note 测试
// PC发送，echo "message..." | nc 192.168.4.1 3333
// PC接受，nc 192.168.4.1 3333
// //////////////////////////////////////////
void app_main(void)
{
  ESP_LOGI(TAG, "Starting TCP Service...");
  // 启动应用任务：读取Core Temp 并通过 Tcp Service 发送到远端的Client显示
  starting_app_service();
  // 启动Tcp Service，启动wifi AP模式，建立服务端的监听，接受Client链接请求
  // 并提供数据发送接口供其他任务调用
  starting_tcp_service();
  // 此次返回,将销毁main的对应任务,但不会影响刚刚建立的“子任务”
}
// end of code ///
