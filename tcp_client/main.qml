import QtQuick 2.5
import QtQuick.Window 2.2
import qml.custom.Page_message_data 1.0

Window {
    visible: true
    width: 400
    height:400
    color: "transparent"
    flags: Qt.FramelessWindowHint

    Page_Message_Data{
      id:data

      onCore_temp_changed: {
        temp_txt.text = _str ;
      }

      onTcp_connect_changed: {
        server_connected_txt.text = _con?"Connect":"Disconnect"
        sock_btn.border.color = _con?"green":"#acf8f6"
      }

      onAccess_point_changed: {
        pa_connected_txt.text = _ap_name;
      }
    }

    Rectangle{
      id:root_rect
      color: "#112d4e"
      anchors.fill: parent
      radius:15
      border.color: "#3f72af"
      border.width: 2

      Item{
          anchors.fill: parent

          Rectangle{
            x:20
            y:100
            width:250
            height:80
            color: "#dbe2ef"
            radius: 5
            Image {
                id: temp_icon
                width:64
                height:64
                source: "qrc:/icons/temp.png"
                fillMode: Image.PreserveAspectFit
                clip: true
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
            }
            Text {
                id:temp_txt
                text:"00.00"
                font.pixelSize: 40
                color: "#3f72af"
                anchors.verticalCenter: temp_icon.verticalCenter
                anchors.left: temp_icon.right
                anchors.leftMargin: 20
            }
          }

          Rectangle{
            x:280
            y:100
            width:100
            height:200
            color: "#a8d8ea"
            radius: 5
            Image {
                id: tangle_icon
                width:64
                height:64
                source: "qrc:/icons/tangle.png"
                fillMode: Image.PreserveAspectFit
                clip: true
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top:parent.top
                anchors.topMargin: 10
            }
            Text {
                id:tangle_txt
                text:"00.0"
                font.pixelSize: 30
                color: "#3f72af"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top:tangle_icon.bottom
                anchors.topMargin: 20
            }
          }
          Rectangle{
            x:20
            y:190
            width:200
            height:110
            color: "#ffe2e2"
            radius: 5
            Image {
                id: rotate_icon
                width:64
                height:64
                source: "qrc:/icons/rotate.png"
                fillMode: Image.PreserveAspectFit
                clip: true
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
            }
            Text {
                id:rotate_txt
                text:"00.0"
                font.pixelSize: 40
                color: "#3f72af"
                anchors.verticalCenter: rotate_icon.verticalCenter
                anchors.left: rotate_icon.right
                anchors.leftMargin: 10
            }
          }

          Rectangle{
            id:sock_btn
            width: 60
            height: 60
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin:10
            border.color: "#acf8f6"
            border.width: 2
            color: "transparent"
            radius: 5
            Image {
                id: wifi_icon
                width:48
                height:48
                source: "qrc:/icons/wifi.png"
                fillMode: Image.PreserveAspectFit
                clip: true
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
            MouseArea{
              anchors.fill: parent
              onPressed: {
                  parent.border.color = "red"
              }
              onReleased: {
                  parent.border.color = "#acf8f6"
                  data.socked_clicked()
              }
            }
          }
          Rectangle{
            id:bt_btn
            width: 60
            height: 60
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
            border.width: 2
            radius:5
            border.color: "#acf8f6"
            color: "transparent"
            Image {
              id: bt_icon
              width:48
              height:48
              source: "qrc:/icons/bt.png"
              fillMode: Image.PreserveAspectFit
              clip: true
              anchors.horizontalCenter: parent.horizontalCenter
              anchors.verticalCenter: parent.verticalCenter
            }
            MouseArea{
              anchors.fill: parent
              onPressed: {
                parent.border.color = "red"
              }
              onReleased: {
                parent.border.color = "#acf8f6"
              }
            }
          }

          Item {
            id:status_bar
            width: parent.width
            height:25
            anchors.bottom:parent.bottom
            anchors.bottomMargin: 15
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 10
            Text {
                id:pa_connected_txt
                text:"Device"
                font.pixelSize:15
              color: "#f9f7f7"
                anchors.left: parent.left
                anchors.leftMargin: 5
                anchors.verticalCenter: parent.verticalCenter
            }

            Image {
                id: exit_icon
                width:48
                height:48
                source: "qrc:/icons/exit.png"
                fillMode: Image.PreserveAspectFit
                clip: true
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        data.exit_clicked();
                    }
                }
            }

            Text {
                id:server_connected_txt
                text:"Service"
                font.pixelSize:15
                color: "#f9f7f7"
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
            }
          }
      }
    }
}
