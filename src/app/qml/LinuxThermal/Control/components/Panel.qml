import QtQuick

Rectangle {
    id: root
    default property alias contentData: content.data
    property real contentPadding: Theme.cardPadding
    property int entryDelay: 0
    property real hoverLift: 0
    property bool emphasized: false
    property real entranceOffset: 28
    property bool initialized: false
    property var viewport: null
    property bool revealed: false
    readonly property bool inViewport: {
        if (!visible || (Window.window && !Window.window.visible))
            return false
        if (!viewport)
            return true
        // 显式订阅布局与滚动变化，按未偏移的位置判断入场。
        const layoutPosition = y + (parent ? parent.y : 0) - viewport.contentY
        if (!isFinite(layoutPosition))
            return false
        const top = mapToItem(viewport, 0, 0).y - entranceOffset - hoverLift
        return top < viewport.height - 12 && top + height > 12
    }

    implicitWidth: 260
    implicitHeight: content.childrenRect.height + contentPadding * 2
    color: Theme.panel
    radius: Theme.cardRadius
    border.color: emphasized ? Theme.borderStrong : Theme.border
    border.width: 1
    opacity: 0
    transform: Translate { y: root.entranceOffset + root.hoverLift }
    Behavior on hoverLift { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutCubic } }
    Behavior on border.color { ColorAnimation { duration: Theme.animationMedium } }

    function revealIfNeeded(): void {
        if (initialized && inViewport && !revealed) {
            revealed = true
            enterAnimation.restart()
        }
    }
    Component.onCompleted: {
        initialized = true
        revealIfNeeded()
    }
    function synchronizePresentation(): void {
        if (!initialized)
            return
        revealIfNeeded()
        if (!inViewport && visible && revealed && enterAnimation.running) {
            enterAnimation.stop()
            opacity = 1
            entranceOffset = 0
        }
    }
    onInViewportChanged: Qt.callLater(root.synchronizePresentation)
    onVisibleChanged: {
        if (!visible) {
            enterAnimation.stop()
            revealed = false
            opacity = 0
            entranceOffset = 28
        } else {
            revealIfNeeded()
        }
    }
    SequentialAnimation {
        id: enterAnimation
        PauseAnimation { duration: root.entryDelay }
        ParallelAnimation {
            NumberAnimation { target: root; property: "opacity"; from: 0; to: 1; duration: Theme.animationMedium; easing.type: Easing.OutCubic }
            NumberAnimation { target: root; property: "entranceOffset"; from: 28; to: 0; duration: Theme.animationSlow; easing.type: Easing.OutCubic }
        }
    }
    Item {
        id: content
        x: root.contentPadding
        y: root.contentPadding
        width: Math.max(0, root.width - root.contentPadding * 2)
        height: childrenRect.height
    }
}
