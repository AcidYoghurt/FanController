import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ToolBar {
    id: root
    required property bool connected
    required property bool busy
    required property bool refreshing
    required property string systemName
    required property int currentPage
    required property bool navigationEnabled
    signal pageRequested(int page)
    signal refreshRequested

    implicitHeight: 80
    background: Rectangle {
        color: Theme.panel
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
    }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        spacing: 16
        Rectangle {
            implicitWidth: 44; implicitHeight: 44; radius: 14
            color: Theme.accent
            ThermalIcon { anchors.centerIn: parent; width: 28; height: 28; color: Theme.accentForeground }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            spacing: 3
            Text { text: qsTr("Linux 热管理"); color: Theme.text; font.pixelSize: Theme.bodySize; font.weight: Font.Medium; textFormat: Text.PlainText }
            Text { Layout.fillWidth: true; text: root.systemName || qsTr("THERMAL CONTROL"); color: Theme.subtle; font.pixelSize: Theme.captionSize; elide: Text.ElideRight; textFormat: Text.PlainText }
        }
        HeaderNavigation {
            currentIndex: root.currentPage
            navigationEnabled: root.navigationEnabled
            onPageRequested: page => root.pageRequested(page)
        }
        Rectangle { implicitWidth: 1; implicitHeight: 24; color: Theme.border }
        RowLayout {
            spacing: 7
            Rectangle { implicitWidth: 7; implicitHeight: 7; radius: 4; color: root.connected ? Theme.success : Theme.danger }
            Text { text: root.connected ? qsTr("已连接") : qsTr("未连接"); color: root.connected ? Theme.success : Theme.danger; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText }
        }
        ToolButton {
            id: refreshButton
            implicitWidth: 40; implicitHeight: 40
            enabled: !root.busy && !root.refreshing
            hoverEnabled: true
            padding: 8
            Accessible.name: qsTr("刷新状态")
            ToolTip.visible: (hovered || visualFocus) && !pressed
            ToolTip.text: qsTr("刷新状态（F5）")
            onClicked: root.refreshRequested()
            background: Rectangle {
                radius: Theme.controlRadius
                color: refreshButton.hovered ? Theme.panelHover : "transparent"
                border.width: refreshButton.visualFocus ? 2 : 0
                border.color: Theme.accent
            }
            contentItem: ThermalIcon {
                id: refreshIcon
                kind: "refresh"
                color: Theme.muted
                RotationAnimator on rotation {
                    from: 0; to: 360; duration: 680; loops: Animation.Infinite
                    running: root.refreshing && root.visible && root.Window.window !== null && root.Window.window.visible
                }
            }
            HoverHandler { cursorShape: refreshButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
        }
        ToolButton {
            id: appearanceButton
            objectName: "appearanceButton"
            implicitWidth: 40; implicitHeight: 40
            padding: 8
            hoverEnabled: true
            Accessible.name: qsTr("外观")
            ToolTip.visible: hovered || visualFocus
            ToolTip.text: qsTr("外观")
            onClicked: appearancePopup.open()
            background: Rectangle {
                radius: Theme.controlRadius
                color: appearanceButton.hovered || appearancePopup.visible ? Theme.panelHover : "transparent"
                border.color: Theme.accent
                border.width: appearanceButton.visualFocus ? 2 : 0
            }
            contentItem: ThermalIcon { kind: "sun"; color: Theme.muted }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            AppearancePopup {
                id: appearancePopup
                x: parent.width - width
                y: parent.height + 12
                onClosed: appearanceButton.forceActiveFocus(Qt.PopupFocusReason)
            }
        }
    }
}
