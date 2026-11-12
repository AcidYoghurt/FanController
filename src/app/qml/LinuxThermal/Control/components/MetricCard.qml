import QtQuick
import QtQuick.Layouts

Panel {
    id: root
    required property string title
    required property string value
    property string suffix: ""
    property string caption: ""
    property color accent: Theme.cool
    property real levelTemperature: -1
    property bool readingInitialized: false
    implicitWidth: 260
    hoverLift: cardHover.hovered ? -4 : 0
    emphasized: cardHover.hovered
    HoverHandler { id: cardHover }
    Component.onCompleted: readingInitialized = true
    onValueChanged: {
        if (readingInitialized && inViewport)
            readingPulse.restart()
    }
    SequentialAnimation {
        id: readingPulse
        NumberAnimation { target: readingText; property: "scale"; from: 1; to: 1.045; duration: 100; easing.type: Easing.OutCubic }
        NumberAnimation { target: readingText; property: "scale"; to: 1; duration: Theme.animationMedium; easing.type: Easing.OutBack }
    }

    ColumnLayout {
        width: parent.width
        spacing: 16
        RowLayout {
            Layout.fillWidth: true
            Text { Layout.fillWidth: true; text: root.title; color: Theme.muted; font.pixelSize: Theme.bodySize; textFormat: Text.PlainText }
            ThermalIcon { kind: "chip"; color: root.accent }
        }
        RowLayout {
            spacing: 8
            Text { id: readingText; transformOrigin: Item.Left; text: root.value; color: Theme.text; font.pixelSize: Theme.displaySize; font.weight: Font.Medium; textFormat: Text.PlainText }
            Text { text: root.suffix; color: Theme.muted; font.pixelSize: Theme.bodySize; Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: 8; textFormat: Text.PlainText }
            Item { Layout.fillWidth: true }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 4
            radius: 2
            color: Theme.border
            clip: true
            Rectangle {
                width: parent.width
                height: parent.height
                radius: 2
                color: root.accent
                transform: Scale {
                    xScale: Math.max(0, Math.min(1, root.levelTemperature / 100))
                    Behavior on xScale {
                        enabled: root.inViewport
                        NumberAnimation { duration: Theme.animationSlow; easing.type: Easing.OutCubic }
                    }
                }
            }
        }
        Text { Layout.fillWidth: true; text: root.caption; color: Theme.subtle; font.pixelSize: Theme.captionSize; visible: text.length > 0; textFormat: Text.PlainText; elide: Text.ElideRight }
    }
}
