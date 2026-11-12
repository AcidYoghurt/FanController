pragma Singleton
import QtQuick
import QtCore

QtObject {
    property alias themeMode: preferences.themeMode
    readonly property bool dark: themeMode === 0
                                 ? Application.styleHints.colorScheme === Qt.Dark
                                 : themeMode === 2

    readonly property Settings storage: Settings {
        id: preferences
        category: "Appearance"
        property int themeMode: 0
    }
}
