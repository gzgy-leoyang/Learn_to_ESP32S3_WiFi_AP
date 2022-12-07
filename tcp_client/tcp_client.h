#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <QObject>

class tcp_client : public QObject
{
    Q_OBJECT
public:
    explicit tcp_client(QObject *parent = 0);
    int connect_server( const QString& _ip,const int& _port );
    int disconnect_server( void );
    bool is_connected(void) const { return m_connected;}

signals:
    void sig_tcp_message( const char* _msg,const int _size );
    void sig_connection( const bool& _con );

public slots:
    void slot_tcp_read(void);
    void slot_conneect(void);
    void slot_disconneect(void);

private:
    bool m_connected;
};

#endif // TCP_CLIENT_H
