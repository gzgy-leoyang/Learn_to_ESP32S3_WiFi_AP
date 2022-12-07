#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

// //////////////////////////////////////////
/// @brief send_message 向客户端发送数据 
///
/// @param: _buf 指向数据缓冲区的指针
/// @param: _size 数据长度
///
/// @return: 
// //////////////////////////////////////////
extern esp_err_t send_message( const char* _buf,const int _size );

#endif
