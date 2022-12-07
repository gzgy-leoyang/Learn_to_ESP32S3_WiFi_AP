#include "page_message.h"
#include <QDebug>

Page_message::
Page_message(QObject* parent) : QObject(parent)
{
    client = new tcp_client();
    connect( client,SIGNAL(sig_tcp_message(const char*,int)),
             this  ,SLOT(slot_esp_message(const char*,int)));
    connect( client,SIGNAL(sig_connection(bool)),
             this  ,SLOT(slot_connection(bool)));
}
void Page_message::
slot_esp_message( const char* _buf,const int _size )
{
    qDebug()<<"ESP32-S3 Message {"<<_size<<","<<_buf<<"}";
    QString str = QString( _buf );
    if ( str != m_core_temp ){
        m_core_temp = str;
        emit core_temp_changed( m_core_temp );
    }
    qDebug()<<m_core_temp;
}
void Page_message::
socked_clicked(void)
{
    qDebug("socket clicked..");
    if ( !client->is_connected() )
        client->connect_server(QString("192.168.4.1"),3333);
    else
        client->disconnect_server();
}
void Page_message::
slot_connection( const bool& _con )
{
    emit tcp_connect_changed( _con );
}

void Page_message::
ble_clicked(void)
{
    qDebug("Bluetooth clicked..");
}

