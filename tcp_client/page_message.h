#ifndef PAGE_MESSAGE_H
#define PAGE_MESSAGE_H

#include <QObject>
#include "tcp_client.h"

class Page_message : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  _core_temp
               READ     get_core_temp
               NOTIFY   core_temp_changed )
    QString get_core_temp() const { return m_core_temp ;}

public:
    explicit Page_message(QObject *parent=0);

    Q_INVOKABLE void socked_clicked(void);
    Q_INVOKABLE void ble_clicked(void);

signals:
    void core_temp_changed ( const QString& _str );
    void tcp_connect_changed( const bool& _con );

public slots:
    void slot_esp_message( const char* _buf,const int _size );
    void slot_connection( const bool& _con );
private:
    tcp_client* client;
    QString m_core_temp;
};

#endif // PAGE_MESSAGE_H
