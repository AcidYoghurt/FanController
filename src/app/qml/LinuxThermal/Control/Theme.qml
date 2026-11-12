pragma Singleton
import QtQuick

QtObject {
    readonly property bool dark: AppSettings.dark

    readonly property color window: dark ? "#0c121b" : "#f2f5f7"
    readonly property color panel: dark ? "#151e2b" : "#ffffff"
    readonly property color panelHover: dark ? "#202c3c" : "#edf3f5"
    readonly property color inset: dark ? "#101925" : "#f6f8fa"
    readonly property color border: dark ? "#2a3748" : "#dce4e9"
    readonly property color borderStrong: dark ? "#74869a" : "#7b8d9c"
    readonly property color text: dark ? "#edf4fa" : "#182936"
    readonly property color muted: dark ? "#afbed0" : "#526777"
    readonly property color subtle: dark ? "#9aaabd" : "#617586"
    readonly property color accent: dark ? "#66dfc6" : "#007d6b"
    readonly property color accentForeground: dark ? "#10241f" : "#ffffff"
    readonly property color statusForeground: dark ? window : panel
    readonly property color cool: dark ? "#78baff" : "#2563a8"
    readonly property color success: dark ? "#7cdaa2" : "#237746"
    readonly property color warning: dark ? "#f4c078" : "#956014"
    readonly property color danger: dark ? "#ff959d" : "#b82d44"

    readonly property FontMetrics systemMetrics: FontMetrics {}
    readonly property real fontScale: Math.max(1, systemMetrics.font.pointSize > 0
                                               ? systemMetrics.font.pointSize / 10
                                               : systemMetrics.font.pixelSize / 13)
    readonly property real captionSize: 12 * fontScale
    readonly property real bodySize: 16 * fontScale
    readonly property real headingSize: bodySize
    readonly property real titleSize: 28 * fontScale
    readonly property real displaySize: 38 * fontScale
    readonly property int pageMargin: 28
    readonly property int cardPadding: 24
    readonly property int sectionSpacing: 20
    readonly property int cardRadius: 16
    readonly property int controlRadius: 10
    readonly property int animationFast: 140
    readonly property int animationMedium: 240
    readonly property int animationSlow: 360
    readonly property int animationReveal: 420
}
