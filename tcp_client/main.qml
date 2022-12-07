import QtQuick 2.5
import QtQuick.Window 2.2
import qml.custom.Page_message_data 1.0

Window {
    visible: true
    width: 400
    height:400

    Page_Message_Data{
        id:data
        onCore_temp_changed: {
          temp_txt.text = _str ;
        }

        onTcp_connect_changed: {
            sock_btn_txt.text = _con?"Disconnect":"Re-Connect"
        }
    }

    Item{
        anchors.fill: parent
        Text {
            id:temp_txt
            text:"Core Temp"
            font.pixelSize: 40
            color: "black"
            anchors.centerIn: parent
        }
        Rectangle{
          id:sock_btn
          width: 200
          height: 50
          anchors.left: parent.left
          anchors.bottom: parent.bottom
          border.color: "black"
          border.width: 2
          radius:10
          MouseArea{
              anchors.fill: parent
              onPressed: {
                  parent.border.color = "red"
              }
              onReleased: {
                  parent.border.color = "black"
                  console.debug("socked_clicked...")
                  data.socked_clicked()
              }
          }

          Text {
              id:sock_btn_txt
              text:"Start TCP Client"
              font.pixelSize:15
              color: "black"
              anchors.centerIn: parent
          }
        }
        Rectangle{
          id:bt_btn
          width: 200
          height: 50
          anchors.right: parent.right
          anchors.bottom: parent.bottom
          border.color: "black"
          border.width: 2
          radius:10
          MouseArea{
              anchors.fill: parent
              onPressed: {
                  parent.border.color = "red"
              }
              onReleased: {
                  parent.border.color = "black"
              }
          }
          Text {
              id:bt_btn_txt
              text:"Start BLE"
              font.pixelSize:15
              color: "black"
              anchors.centerIn: parent
          }
        }
    }
}
