pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

StackView {
    id: root
    objectName: "pageStack"

    required property var backend
    required property int currentPage
    required property bool modeTransitioning
    signal modeRequested(string mode)
    signal editedCurveApplyRequested

    readonly property real slideDistance: Math.min(width * 0.12, 96)

    clip: true
    initialItem: controlPageComponent

    function synchronizePage() {
        if (busy)
            return
        if (currentPage === 1 && depth === 1) {
            push(infoPageComponent)
        } else if (currentPage === 0 && depth > 1) {
            pop()
        }
    }

    onCurrentPageChanged: synchronizePage()
    onBusyChanged: {
        if (!busy)
            synchronizePage()
    }

    Component {
        id: controlPageComponent

        ControlPage {
            backend: root.backend
            modeTransitioning: root.modeTransitioning
            onModeRequested: mode => root.modeRequested(mode)
            onEditedCurveApplyRequested: root.editedCurveApplyRequested()
        }
    }

    Component {
        id: infoPageComponent

        InfoPage {
            backend: root.backend
        }
    }

    pushEnter: Transition {
        SequentialAnimation {
            PropertyAction { property: "x"; value: root.slideDistance }
            PropertyAction { property: "opacity"; value: 0 }
            PauseAnimation { duration: 140 }
            ParallelAnimation {
                NumberAnimation {
                    property: "x"
                    from: root.slideDistance
                    to: 0
                    duration: 200
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    pushExit: Transition {
        SequentialAnimation {
            ParallelAnimation {
                NumberAnimation {
                    property: "x"
                    from: 0
                    to: -root.slideDistance * 0.55
                    duration: 140
                    easing.type: Easing.InCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 140
                    easing.type: Easing.InCubic
                }
            }
            PauseAnimation { duration: 200 }
        }
    }

    popEnter: Transition {
        SequentialAnimation {
            PropertyAction { property: "x"; value: -root.slideDistance }
            PropertyAction { property: "opacity"; value: 0 }
            PauseAnimation { duration: 140 }
            ParallelAnimation {
                NumberAnimation {
                    property: "x"
                    from: -root.slideDistance
                    to: 0
                    duration: 200
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    popExit: Transition {
        SequentialAnimation {
            ParallelAnimation {
                NumberAnimation {
                    property: "x"
                    from: 0
                    to: root.slideDistance * 0.55
                    duration: 140
                    easing.type: Easing.InCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 140
                    easing.type: Easing.InCubic
                }
            }
            PauseAnimation { duration: 200 }
        }
    }

}
