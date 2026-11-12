import QtQuick

Item {
    id: root
    property string kind: "fan"
    property color color: Theme.accent
    implicitWidth: 24
    implicitHeight: 24
    Accessible.ignored: true

    readonly property string pathData: root.kind === "chevron"
        ? "M5 9L12 16L19 9"
        : root.kind === "chip"
        ? "M7 4H17V20H7Z M4 8H7 M4 12H7 M4 16H7 M17 8H20 M17 12H20 M17 16H20 M10 8H14V16H10Z"
        : root.kind === "curve"
        ? "M4 4V20H21 M7 16L11 13L15 12L20 5"
        : root.kind === "refresh"
        ? "M20 10A8 8 0 1 0 19 17 M20 4V10H14"
        : root.kind === "shield"
        ? "M12 3L20 6V12Q20 18 12 22Q4 18 4 12V6Z M8 12L11 15L16 9"
        : root.kind === "check"
        ? "M5 12L10 17L19 7"
        : root.kind === "sun"
        ? "M8 12A4 4 0 1 0 16 12A4 4 0 1 0 8 12 M12 1V4 M12 20V23 M1 12H4 M20 12H23 M4 4L6 6 M18 18L20 20 M4 20L6 18 M18 6L20 4"
        : "M10 12A2 2 0 1 0 14 12A2 2 0 1 0 10 12 M12 10C6 8 6 2 12 3C17 3 19 8 14 11 M14 12C19 8 24 12 20 17C17 21 12 20 12 14 M11 14C12 20 6 23 4 17C2 12 5 9 10 11"

    Image {
        anchors.fill: parent
        sourceSize.width: Math.ceil(root.width * Screen.devicePixelRatio)
        sourceSize.height: Math.ceil(root.height * Screen.devicePixelRatio)
        source: "data:image/svg+xml;utf8," + encodeURIComponent(
                    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">'
                    + '<path d="' + root.pathData + '" fill="none" stroke="' + root.color
                    + '" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"/></svg>')
    }
}
