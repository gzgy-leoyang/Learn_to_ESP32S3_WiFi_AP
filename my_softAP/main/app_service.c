#include <string.h>
#include <sys/param.h>
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

#include "app_main.h"
#include "tcp_server.h"

#define PORT                        3333//CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE              5//CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          5//CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             3//CONFIG_EXAMPLE_KEEPALIVE_COUNT

#define WIFI_SSID "lexin.a"      
#define WIFI_PASS "password.a"
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "App Service";

       void      starting_app_service(void);
static void      app_task(void *pvParameters);

static void      temprature_init(void);
static esp_err_t temprature_read( float* _temp );


// //////////////////////////////////////////
/// @brief starting_app_service 启动应用任务
// //////////////////////////////////////////
void starting_app_service(void)
{
  ESP_LOGI(TAG, "Create and Starting app task...");
  temprature_init();
  xTaskCreate(app_task, "app_service", 4096, (void*)AF_INET, 5, NULL);
}


// //////////////////////////////////////////
/// @brief app_task 应用任务
/// 读取 CoreTemp，当温度改变大于阈值时，或是满足间隔25s时，
/// 通过tcp service 发送温度数据到 Client
///
/// @param: pvParameters
// //////////////////////////////////////////
static void app_task(void *pvParameters)
{
  esp_err_t err = 0;
  float temp    = 0.0;
  char buf[20] = {0};
  int ss = 0;

  static float tt = 0;
  static int force_update_cnt = 0;

  while (1) {
    if ( (err =temprature_read(&temp)) == ESP_OK ){
      float delta = 0;
      if ( temp < data_store.core_temp )
        delta = data_store.core_temp - temp;
      else 
        delta = temp - data_store.core_temp;

      force_update_cnt++;
      // 间隔25s强制上传参数，或当温度改变大于阈值时上传
      if (((force_update_cnt % 5)==0) || 
          ( delta > 0.5 )){
        data_store.core_temp = temp;
        ESP_LOGI(TAG, "temp %.02f",temp);
        memset(buf,0,20);
        sprintf( buf,"temp %.02f\n",data_store.core_temp );
        ss = strlen(buf);
        if (send_message(buf,ss) < 0 ){
          ESP_LOGI(TAG, "Failed Send Message via WIFI");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelete(NULL);
}


// //////////////////////////////////////////
/// @brief temprature_init 初始化温度传感器模块
// //////////////////////////////////////////
static void temprature_init(void)
{
  temp_sensor_config_t temp_sensor = {
    .dac_offset = TSENS_DAC_L2,
    .clk_div = 6,
  };
  temp_sensor_set_config(temp_sensor);
  temp_sensor_start();
}

// //////////////////////////////////////////
/// @brief temprature_read 读取温度值
///
/// @param: _temp 指向数据结果的指针
///
/// @return: 
// //////////////////////////////////////////
static esp_err_t temprature_read( float* _temp )
{
  float tsens_out;
  esp_err_t err = 0;
  if ((err=temp_sensor_read_celsius(&tsens_out)) == ESP_OK ){
    *_temp = tsens_out;
    return ESP_OK;
  } else {
    // err
    ESP_LOGW(TAG, "Failed to read Temperature [%i]",err);
    return err;
  }
}
// end of code //
