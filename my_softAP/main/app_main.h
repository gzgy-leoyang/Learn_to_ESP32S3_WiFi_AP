#ifndef _APP_MAIN_H
#define _APP_MAIN_H

// //////////////////////////////////////////
/// @brief starting_tcp_service 
// //////////////////////////////////////////
extern void starting_tcp_service(void);

// //////////////////////////////////////////
/// @brief starting_app_service 
// //////////////////////////////////////////
extern void starting_app_service(void);

// //////////////////////////////////////////
/// @brief 应用数据
// //////////////////////////////////////////
typedef struct {
  float core_temp;
} Data_store_t;

extern Data_store_t data_store;

#endif
