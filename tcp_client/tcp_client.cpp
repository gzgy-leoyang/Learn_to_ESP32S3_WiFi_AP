#include "tcp_client.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDebug>

QTcpSocket* sock = NULL;

static char buf[1024] = {0};

tcp_client::
tcp_client(QObject *parent) : QObject(parent)
{
    m_connected = false ;

    sock = new QTcpSocket(this);
    connect( sock, SIGNAL(readyRead()),this,SLOT(slot_tcp_read()));
    connect( sock, SIGNAL(connected()),this,SLOT(slot_conneect()));
    connect( sock, SIGNAL(disconnected()),this,SLOT(slot_disconneect()));
    //sock->connectToHost(QHostAddress("192.168.4.1"),3333);
}
int tcp_client::
connect_server( const QString& _ip,const int& _port )
{
    if ( !m_connected )
      sock->connectToHost(QHostAddress( _ip ),_port);
    return 0;
}

int tcp_client::
disconnect_server( void )
{
    if ( m_connected )
      sock->disconnectFromHost();
    return 0;
}

void tcp_client::
slot_tcp_read(void)
{
    int ret = sock->read(buf,1024);
    emit sig_tcp_message( buf,ret );
    return ;
}

void tcp_client::
slot_conneect(void)
{
    qDebug()<<"Socket connected";
    m_connected = true;
    emit sig_connection( m_connected );
}


void tcp_client::
slot_disconneect(void)
{
    qDebug()<<"Socket DISconnected";
    m_connected = false;
    emit sig_connection( m_connected );
}
