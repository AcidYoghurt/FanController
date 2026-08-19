pragma Singleton
import QtQuick

QtObject {
    readonly property bool dark: Application.styleHints.colorScheme === Qt.Dark

    readonly property color window: dark ? "#0b1016" : "#f3f6f9"
    readonly property color panel: dark ? "#141b24" : "#ffffff"
    readonly property color panelHover: dark ? "#1b2531" : "#f7f9fb"
    readonly property color border: dark ? "#293544" : "#dce3ea"
    readonly property color borderStrong: dark ? "#516174" : "#a6b2c0"
    readonly property color text: dark ? "#edf3f8" : "#17212b"
    readonly property color muted: dark ? "#a5b0bd" : "#5b6877"
    readonly property color subtle: dark ? "#758295" : "#8290a0"
    readonly property color accent: "#ff6136"
    readonly property color cool: "#38bdf8"
    readonly property color success: "#22c55e"
    readonly property color warning: "#f59e0b"
    readonly property color danger: "#ef4444"

    readonly property int pageMargin: 30
    readonly property int cardPadding: 22
    readonly property int sectionSpacing: 22
    readonly property int cardRadius: 18
    readonly property int controlRadius: 12
    readonly property int animationFast: 140
}
