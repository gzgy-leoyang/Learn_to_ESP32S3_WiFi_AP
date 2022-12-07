#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QObject>

#include "page_message.h"
#include "tcp_client.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qmlRegisterType<Page_message>( "qml.custom.Page_message_data",1,0,"Page_Message_Data");

    //Page_message* page = new Page_message();
    //tcp_client* client = new tcp_client();
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
