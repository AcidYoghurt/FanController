import QtQuick

Rectangle {
    id: root

    default property alias contentData: content.data
    property real contentPadding: Theme.cardPadding

    implicitWidth: 260
    implicitHeight: content.childrenRect.height + contentPadding * 2
    color: Theme.panel
    radius: Theme.cardRadius
    border.color: Theme.border
    border.width: 1

    Item {
        id: content
        x: root.contentPadding
        y: root.contentPadding
        width: Math.max(0, root.width - root.contentPadding * 2)
        height: childrenRect.height
    }
}
