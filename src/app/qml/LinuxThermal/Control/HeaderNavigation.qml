import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: root

    required property int currentIndex
    property bool navigationEnabled: true
    signal pageRequested(int index)

    implicitWidth: navigationRow.implicitWidth + 8
    implicitHeight: 40
    radius: 14
    color: Theme.window
    border.color: Theme.border
    border.width: 1

    RowLayout {
        id: navigationRow
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        Button {
            id: controlButton

            objectName: "controlNavigationButton"
            readonly property bool selected: root.currentIndex === 0
            text: qsTr("Control")
            padding: 0
            Layout.preferredWidth: 88
            Layout.fillHeight: true
            enabled: root.navigationEnabled
            Accessible.name: qsTr("Control 页面")
            Accessible.description: qsTr("查看温度、运行模式和自定义风扇曲线")
            onClicked: {
                if (!selected)
                    root.pageRequested(0)
            }

            background: Rectangle {
                radius: 10
                color: controlButton.selected
                       ? Qt.alpha(Theme.accent, Theme.dark ? 0.22 : 0.12)
                       : controlButton.hovered ? Theme.panelHover : "transparent"
                border.width: controlButton.selected ? 1 : 0
                border.color: Theme.accent
            }

            contentItem: Text {
                text: controlButton.text
                color: controlButton.selected ? Theme.accent : Theme.muted
                font.pixelSize: 12
                font.weight: controlButton.selected ? Font.DemiBold : Font.Medium
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
        }

        Button {
            id: infoButton

            objectName: "infoNavigationButton"
            readonly property bool selected: root.currentIndex === 1
            text: qsTr("Info")
            padding: 0
            Layout.preferredWidth: 68
            Layout.fillHeight: true
            enabled: root.navigationEnabled
            Accessible.name: qsTr("Info 页面")
            Accessible.description: qsTr("查看安全启动、风扇能力和控制后端")
            onClicked: {
                if (!selected)
                    root.pageRequested(1)
            }

            background: Rectangle {
                radius: 10
                color: infoButton.selected
                       ? Qt.alpha(Theme.cool, Theme.dark ? 0.22 : 0.12)
                       : infoButton.hovered ? Theme.panelHover : "transparent"
                border.width: infoButton.selected ? 1 : 0
                border.color: Theme.cool
            }

            contentItem: Text {
                text: infoButton.text
                color: infoButton.selected ? Theme.cool : Theme.muted
                font.pixelSize: 12
                font.weight: infoButton.selected ? Font.DemiBold : Font.Medium
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
        }
    }
}
