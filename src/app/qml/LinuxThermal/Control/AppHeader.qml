import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ToolBar {
    id: root

    required property bool connected
    required property bool busy
    required property bool refreshing
    required property string backendState
    required property string systemName
    required property int currentPage
    required property bool navigationEnabled
    signal pageRequested(int page)
    signal refreshRequested

    implicitHeight: 72
    background: Rectangle {
        color: Theme.panel
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        spacing: 14

        Rectangle {
            implicitWidth: 40; implicitHeight: 40; radius: 13
            gradient: Gradient {
                GradientStop { position: 0; color: "#ff8a57" }
                GradientStop { position: 1; color: Theme.accent }
            }
            Text { anchors.centerIn: parent; text: "T"; color: "white"; font.pixelSize: 19; font.weight: Font.Bold; textFormat: Text.PlainText }
        }

        ColumnLayout {
            Layout.preferredWidth: 232
            spacing: 2
            Text { text: qsTr("Linux Thermal Control"); color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold; textFormat: Text.PlainText; elide: Text.ElideRight; Layout.fillWidth: true }
            Text {
                text: root.systemName.length > 0
                      ? qsTr("%1 · 硬件感知散热").arg(root.systemName)
                      : qsTr("硬件感知的温度与风扇管理")
                color: Theme.muted
                font.pixelSize: 11
                textFormat: Text.PlainText
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        HeaderNavigation {
            currentIndex: root.currentPage
            navigationEnabled: root.navigationEnabled
            onPageRequested: page => root.pageRequested(page)
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            implicitWidth: connectionRow.implicitWidth + 24
            implicitHeight: 32
            radius: 16
            color: Qt.alpha(root.connected ? Theme.success : Theme.danger, Theme.dark ? 0.18 : 0.11)
            RowLayout {
                id: connectionRow
                anchors.centerIn: parent
                spacing: 7
                Rectangle { implicitWidth: 7; implicitHeight: 7; radius: 4; color: root.connected ? Theme.success : Theme.danger }
                Text {
                    text: root.connected ? qsTr("服务已连接") : qsTr("服务未连接")
                    color: root.connected ? Theme.success : Theme.danger
                    font.pixelSize: 12; font.weight: Font.DemiBold; textFormat: Text.PlainText
                }
            }
        }

        ToolButton {
            id: refreshButton

            readonly property bool animating: root.refreshing || finishSpin.running
            readonly property bool surfaceVisible: hovered || pressed

            implicitWidth: 40
            implicitHeight: 40
            padding: 0
            hoverEnabled: true
            focusPolicy: Qt.TabFocus

            Accessible.name: qsTr("刷新状态")
            Accessible.description: qsTr("重新读取硬件温度、风扇和后端状态")
            ToolTip.visible: hovered && !pressed && !animating
            ToolTip.text: qsTr("刷新")

            onClicked: {
                if (!root.busy && !animating)
                    root.refreshRequested()
            }

            background: Rectangle {
                radius: Theme.controlRadius
                color: Theme.panelHover
                visible: refreshButton.surfaceVisible
                border.width: 1
                border.color: Theme.borderStrong
            }

            contentItem: Item {
                implicitWidth: 24
                implicitHeight: 24

                Text {
                    id: refreshGlyph

                    anchors.centerIn: parent
                    text: "↻"
                    color: Theme.muted
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    textFormat: Text.PlainText
                    transformOrigin: Item.Center
                }
            }

            Connections {
                target: root

                function onRefreshingChanged() {
                    if (root.refreshing) {
                        finishSpin.stop()
                        refreshGlyph.rotation = 0
                        continuousSpin.restart()
                        return
                    }
                    if (!continuousSpin.running)
                        return

                    const angle = ((refreshGlyph.rotation % 360) + 360) % 360
                    continuousSpin.stop()
                    finishSpin.from = angle
                    finishSpin.to = 360
                    finishSpin.duration = Math.max(
                                180, Math.round((360 - angle) / 360 * 520))
                    finishSpin.restart()
                }
            }

            NumberAnimation {
                id: continuousSpin
                target: refreshGlyph
                property: "rotation"
                from: 0
                to: 360
                duration: 680
                loops: Animation.Infinite
                easing.type: Easing.Linear
            }

            NumberAnimation {
                id: finishSpin
                target: refreshGlyph
                property: "rotation"
                easing.type: Easing.OutCubic
                onStopped: refreshGlyph.rotation = 0
            }
        }
    }
}
